// SPDX-License-Identifier: GPL-2.0-or-later
/* texto_rico.c - visor de texto con estilos (Markdown) por secciones (D239).
 *
 * Un componente de la libsagui para mostrar texto con formato: negrita,
 * cursiva, títulos, listas, citas, código en línea y bloques de código con
 * fondo distinto y un botón «Copiar» que deja el bloque en el portapapeles
 * del sistema (D074). Lo usa el chat con IA (chatia), pero no sabe nada de la
 * IA: cualquier programa que quiera mostrar Markdown en una parte de su
 * ventana puede usarlo.
 *
 * El documento es una lista de secciones (en el chat, un mensaje cada una),
 * con un rótulo opcional («Tú», «Asistente»). Cada sección guarda su texto
 * fuente y se maqueta por separado en una lista de elementos con posición
 * relativa a la sección: trozos de texto con estilo, rectángulos de fondo y
 * botones «Copiar». Añadir texto al final de la última sección (lo que pasa
 * con una respuesta en streaming) solo vuelve a maquetar esa sección, así que
 * un bloque de código a medio llegar ya se ve como código, y una negrita sin
 * cerrar se ve tal cual hasta que llega el cierre.
 *
 * El texto es Latin-1, como en todo el sistema, y la letra es la de 8x16 del
 * sistema: la negrita es la letra pintada dos veces con un píxel de
 * diferencia y la cursiva, la letra inclinada fila a fila. Todo se recorta al
 * rectángulo del componente: nada se dibuja fuera de él. */
#include "gui.h"

extern const unsigned char fonte_sagui[256][16];

#define RC_LINHA        18      /* alto de una línea de texto */
#define RC_LINHA_COD    17      /* alto de una línea de un bloque de código */
#define RC_MARGEM       8       /* margen interior a izquierda y derecha */
#define RC_BARRA        10      /* barra de desplazamiento */
#define RC_HUECO        6       /* espacio entre bloques */
#define RC_CAB_COD      22      /* cabecera del bloque de código (lenguaje y «Copiar») */
#define RC_BOTAO_L      64
#define RC_BOTAO_A      18
#define RC_PROF_MAX     4       /* niveles de anidamiento de las listas y del énfasis */

/* Colores (los del visor de Markdown, D045). */
#define RC_FUNDO        0xFFFFFF
#define RC_TIT1         0x14532D
#define RC_TIT2         0x0B7A3E
#define RC_TIT3         0x1F4E8C
#define RC_LINHA_TIT    0xC8CDD2
#define RC_COD          0x9A2A3A
#define RC_FUNDO_COD    0xE8EAED
#define RC_BLOCO        0xF1F3F5
#define RC_BLOCO_BORDA  0x9AA3AC
#define RC_BLOCO_TEXTO  0x24292E
#define RC_ENLACE       0x1A5FB4
#define RC_CITA_BARRA   0x9CC5A8
#define RC_CITA_TEXTO   0x3C5A45
#define RC_SEPARADOR    0xE1E4E8

enum { EL_TEXTO, EL_RET, EL_COPIAR };

typedef struct {
    uint8_t  tipo;
    uint8_t  est;          /* RICO_* (solo el texto) */
    int16_t  x, l, a;      /* x relativo al borde izquierdo del contenido */
    int      y;            /* relativo a lo alto de la sección */
    int      off, n;       /* texto: en el pool; copiar: el código en la fuente */
    uint32_t cor;
} rico_el_t;

typedef struct {
    char      *fonte;      /* el texto tal cual llegó (Latin-1) */
    int        n, cap;
    char       rotulo[32];
    uint32_t   cor_rotulo;
    bool       markdown;   /* false: texto plano (cada línea, tal cual) */
    rico_el_t *el;
    int        nel, cap_el;
    char      *pool;       /* el texto que se ve (sin las marcas) */
    int        npool, cap_pool;
    int        alt;        /* alto en píxeles */
    int        y;          /* posición en el documento */
    rico_contagem_t conta;
} rico_secao_t;

struct rico {
    int x, y, l, a;
    rico_secao_t *s;
    int ns, cap_s;
    int topo;              /* píxeles desplazados */
    int total;             /* alto del documento */
    int sec_copiada, el_copiado;   /* el último botón pulsado (dice «Copiado») */
    int copiado_n;
};

/* ------------------------------------------------------------ memoria */

static bool crescer(void **p, int *cap, int precisa, int tam)
{
    if (precisa <= *cap) return true;
    int nova = *cap ? *cap : 64;
    while (nova < precisa) nova *= 2;
    void *q = realoca(*p, (size_t)nova * (size_t)tam);
    if (!q) return false;
    *p = q;
    *cap = nova;
    return true;
}

/* La sección que se está maquetando y su anchura en columnas. */
static rico_secao_t *M;
static int M_cols;

static int novo_el(int tipo)
{
    if (!crescer((void **)&M->el, &M->cap_el, M->nel + 1, sizeof(rico_el_t))) return -1;
    rico_el_t *e = &M->el[M->nel];
    memset(e, 0, sizeof *e);
    e->tipo = (uint8_t)tipo;
    return M->nel++;
}

static void el_ret(int x, int y, int l, int a, uint32_t cor)
{
    int k = novo_el(EL_RET);
    if (k < 0) return;
    rico_el_t *e = &M->el[k];
    e->x = (int16_t)x; e->y = y; e->l = (int16_t)l; e->a = (int16_t)a; e->cor = cor;
}

static void el_texto(int x, int y, const char *t, int n, uint8_t est, uint32_t cor)
{
    if (n <= 0) return;
    if (!crescer((void **)&M->pool, &M->cap_pool, M->npool + n, 1)) return;
    int k = novo_el(EL_TEXTO);
    if (k < 0) return;
    memcpy(M->pool + M->npool, t, (size_t)n);
    rico_el_t *e = &M->el[k];
    e->x = (int16_t)x; e->y = y; e->off = M->npool; e->n = n; e->est = est; e->cor = cor;
    M->npool += n;
}

/* ------------------------------------------------------------ celdas
 *
 * El texto de un bloque, ya sin las marcas de Markdown, va a un vector de
 * celdas (carácter y estilo) antes de partirlo en líneas. */

static char    *cel_c;
static uint8_t *cel_e;
static int      ncel, cap_c, cap_e;

static void por(char c, uint8_t est)
{
    if (!crescer((void **)&cel_c, &cap_c, ncel + 1, 1) ||
        !crescer((void **)&cel_e, &cap_e, ncel + 1, 1)) return;
    if (c == '\t' || c == '\r') c = ' ';
    cel_c[ncel] = c;
    cel_e[ncel] = est;
    ncel++;
}

static bool alfanum(char c)
{
    unsigned char u = (unsigned char)c;
    return (u >= '0' && u <= '9') || (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || u >= 0xC0;
}

static bool pontuacao(char c)
{
    return c && acha_char("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", c) != NULL;
}

/* Cuántas veces seguidas aparece c desde i. */
static int sequencia(const char *s, int n, int i, char c)
{
    int k = 0;
    while (i + k < n && s[i + k] == c) k++;
    return k;
}

/* Busca el cierre de un énfasis de k marcas c que empieza en "de": k marcas
 * seguidas (ni más ni menos), pegadas al texto de la izquierda. */
static int acha_fecho(const char *s, int n, int de, char c, int k)
{
    for (int i = de; i < n; i++) {
        if (s[i] == '`') {                    /* el código en línea no se mira por dentro */
            int q = sequencia(s, n, i, '`');
            int j = i + q;
            while (j < n && !(s[j] == '`' && sequencia(s, n, j, '`') == q)) j++;
            if (j < n) { i = j + q - 1; continue; }
            i += q - 1;
            continue;
        }
        if (s[i] != c) continue;
        int q = sequencia(s, n, i, c);
        if (q == k && i > de && s[i - 1] != ' ' && !(c == '_' && i + q < n && alfanum(s[i + q])))
            return i;
        i += q - 1;
    }
    return -1;
}

static void inline_md(const char *s, int n, uint8_t est, int prof)
{
    int i = 0;
    while (i < n) {
        char c = s[i];
        if (c == '\\' && i + 1 < n && pontuacao(s[i + 1])) { por(s[i + 1], est); i += 2; continue; }
        if (c == '`') {
            int k = sequencia(s, n, i, '`');
            int j = i + k;
            while (j < n && !(s[j] == '`' && sequencia(s, n, j, '`') == k)) j++;
            if (j >= n) {                     /* sin cierre (aún): las comillas tal cual */
                for (int q = 0; q < k; q++) por('`', est);
                i += k;
                continue;
            }
            int a = i + k, b = j;
            if (b - a >= 2 && s[a] == ' ' && s[b - 1] == ' ') { a++; b--; }
            for (int q = a; q < b; q++) por(s[q], (uint8_t)(est | RICO_CODIGO));
            M->conta.codigo_linha++;
            i = j + k;
            continue;
        }
        if ((c == '*' || c == '_') && prof < RC_PROF_MAX) {
            int k = sequencia(s, n, i, c);
            bool abre = i + k < n && s[i + k] != ' ' && !(c == '_' && i > 0 && alfanum(s[i - 1]));
            if (abre && k <= 3) {
                int f = acha_fecho(s, n, i + k, c, k);
                if (f > 0) {
                    uint8_t e2 = est;
                    if (k == 1) { e2 |= RICO_CURSIVA; M->conta.cursiva++; }
                    else if (k == 2) { e2 |= RICO_NEGRITA; M->conta.negrita++; }
                    else { e2 |= RICO_NEGRITA | RICO_CURSIVA; M->conta.negrita++; M->conta.cursiva++; }
                    inline_md(s + i + k, f - i - k, e2, prof + 1);
                    i = f + k;
                    continue;
                }
            }
            for (int q = 0; q < k; q++) por(c, est);
            i += k;
            continue;
        }
        if ((c == '[' || (c == '!' && i + 1 < n && s[i + 1] == '[')) && prof < RC_PROF_MAX) {
            int a = c == '!' ? i + 2 : i + 1;
            int b = a;
            while (b < n && s[b] != ']') b++;
            if (b + 1 < n && s[b + 1] == '(') {
                int u = b + 2;
                while (u < n && s[u] != ')') u++;
                if (u < n) {                  /* [texto](dirección): se ve el texto */
                    inline_md(s + a, b - a, (uint8_t)(est | RICO_ENLACE), prof + 1);
                    i = u + 1;
                    continue;
                }
            }
        }
        por(c, est);
        i++;
    }
}

/* ------------------------------------------------------------ líneas */

static uint32_t cor_de(uint8_t est, uint32_t base)
{
    if (est & RICO_CODIGO) return RC_COD;
    if (est & RICO_ENLACE) return RC_ENLACE;
    return base;
}

/* Las celdas [a, b) en una línea a partir de x (en píxeles). */
static void emitir(int a, int b, int x, int y, uint32_t cor)
{
    int i = a;
    while (i < b) {
        int j = i;
        while (j < b && cel_e[j] == cel_e[i]) j++;
        int xx = x + (i - a) * LETRA_L;
        if (cel_e[i] & RICO_CODIGO) el_ret(xx - 1, y - 1, (j - i) * LETRA_L + 2, LETRA_A + 2, RC_FUNDO_COD);
        el_texto(xx, y, cel_c + i, j - i, cel_e[i], cor_de(cel_e[i], cor));
        i = j;
    }
}

/* Parte las celdas en líneas de "cols" columnas por los espacios (o a la
 * fuerza, si una palabra no cabe) y las coloca desde (x, *y). */
static void fluir(int x, int cols, int *y, uint32_t cor)
{
    if (cols < 4) cols = 4;
    int i = 0;
    if (ncel == 0) { *y += RC_LINHA; return; }
    while (i < ncel) {
        int j;
        if (ncel - i <= cols) j = ncel;
        else {
            j = i + cols;
            int k = j;
            while (k > i && cel_c[k] != ' ') k--;
            if (k > i) j = k;
        }
        int fim = j;
        while (fim > i && cel_c[fim - 1] == ' ' && !(cel_e[fim - 1] & RICO_CODIGO)) fim--;
        emitir(i, fim, x, *y, cor);
        *y += RC_LINHA;
        i = j;
        while (i < ncel && cel_c[i] == ' ' && !(cel_e[i] & RICO_CODIGO)) i++;
    }
}

/* Una línea de la fuente: [s, s + n). */
typedef struct { const char *s; int n; } lin_t;

static int recuo(lin_t l)
{
    int k = 0;
    while (k < l.n && (l.s[k] == ' ' || l.s[k] == '\t')) k += 1;
    return k;
}

static bool vazia(lin_t l) { return recuo(l) == l.n; }

static lin_t proxima(const char **p, const char *fim)
{
    lin_t l = { *p, 0 };
    while (*p < fim && **p != '\n') (*p)++;
    l.n = (int)(*p - l.s);
    if (l.n && l.s[l.n - 1] == '\r') l.n--;
    if (*p < fim) (*p)++;
    return l;
}

/* ``` o ~~~ (tres o más), con hasta tres espacios delante. */
static int eh_cerca(lin_t l, char *ch)
{
    int i = recuo(l);
    if (i > 3 || i >= l.n || (l.s[i] != '`' && l.s[i] != '~')) return 0;
    int k = sequencia(l.s, l.n, i, l.s[i]);
    if (k < 3) return 0;
    *ch = l.s[i];
    return k;
}

static int eh_titulo(lin_t l)
{
    int i = recuo(l);
    if (i > 3) return 0;
    int k = sequencia(l.s, l.n, i, '#');
    if (k < 1 || k > 6) return 0;
    if (i + k < l.n && l.s[i + k] != ' ') return 0;
    return k;
}

static bool eh_regra(lin_t l)
{
    int i = recuo(l), q = 0;
    if (i > 3 || i >= l.n) return false;
    char c = l.s[i];
    if (c != '-' && c != '*' && c != '_') return false;
    for (; i < l.n; i++) {
        if (l.s[i] == c) q++;
        else if (l.s[i] != ' ') return false;
    }
    return q >= 3;
}

/* Marca de lista: "- ", "* ", "+ " o "12. " / "12) ". Devuelve dónde empieza el
 * texto (0 = no es lista) y la marca que se ve. */
static int marcador(lin_t l, char *marca, int max)
{
    int i = recuo(l);
    if (i >= l.n) return 0;
    char c = l.s[i];
    if ((c == '-' || c == '*' || c == '+') && (i + 1 == l.n || l.s[i + 1] == ' ')) {
        marca[0] = 0;
        return i + 1 < l.n ? i + 2 : i + 1;
    }
    int k = i;
    while (k < l.n && k - i < 9 && l.s[k] >= '0' && l.s[k] <= '9') k++;
    if (k > i && k < l.n && (l.s[k] == '.' || l.s[k] == ')') && (k + 1 == l.n || l.s[k + 1] == ' ')) {
        int n = k - i + 1;
        if (n > max - 1) n = max - 1;
        memcpy(marca, l.s + i, (size_t)n);
        marca[n] = 0;
        return k + 1 < l.n ? k + 2 : k + 1;
    }
    return 0;
}

static bool eh_citacao(lin_t l)
{
    int i = recuo(l);
    return i <= 3 && i < l.n && l.s[i] == '>';
}

static bool eh_tabela(lin_t l)
{
    int i = recuo(l);
    return i < l.n && l.s[i] == '|';
}

/* Una línea que empieza un bloque nuevo (corta la lista o el párrafo). */
static bool inicia_bloco(lin_t l)
{
    char ch, m[12];
    return eh_cerca(l, &ch) || eh_titulo(l) || eh_regra(l) || marcador(l, m, sizeof m) ||
           eh_citacao(l) || eh_tabela(l);
}

/* Un bloque de código: cabecera con el lenguaje y el botón «Copiar», y las
 * líneas tal cual (los tabuladores pasan a columnas de cuatro). */
static void bloco_codigo(const char **p, const char *fim, lin_t abre, char ch, int k, int x0, int larg, int *y)
{
    /* El lenguaje (lo que va después de las marcas). */
    int i = recuo(abre) + k;
    while (i < abre.n && abre.s[i] == ' ') i++;
    int li = i;
    while (i < abre.n && abre.s[i] != ' ') i++;
    int topo = *y;
    int fundo = novo_el(EL_RET);
    if (li < i) el_texto(x0 + 8, topo + 3, abre.s + li, i - li < 20 ? i - li : 20, 0, COR_TEXTO_FRACO);
    int bt = novo_el(EL_COPIAR);
    *y += RC_CAB_COD;
    const char *ini = *p, *fim_cod = *p;
    int cols = (larg - 16) / LETRA_L;
    if (cols < 4) cols = 4;
    char linha[256];
    while (*p < fim) {
        lin_t l = proxima(p, fim);
        char ch2;
        int k2 = eh_cerca(l, &ch2);
        if (k2 >= k && ch2 == ch) {
            /* El cierre: las mismas marcas (o más) y nada detrás. */
            int q = recuo(l) + k2;
            while (q < l.n && l.s[q] == ' ') q++;
            if (q == l.n) break;
        }
        fim_cod = l.s + l.n;
        /* Expande tabuladores y parte a la fuerza lo que no cabe. */
        int n = 0, col = 0;
        for (int q = 0; q < l.n; q++) {
            if (l.s[q] == '\t') { do { if (n < (int)sizeof linha) linha[n++] = ' '; col++; } while (col % 4); }
            else { if (n < (int)sizeof linha) linha[n++] = l.s[q]; col++; }
        }
        if (n == 0) *y += RC_LINHA_COD;
        for (int q = 0; q < n; q += cols) {
            int m = n - q < cols ? n - q : cols;
            el_texto(x0 + 8, *y, linha + q, m, RICO_MONO, RC_BLOCO_TEXTO);
            *y += RC_LINHA_COD;
        }
    }
    *y += 6;
    if (fundo >= 0) {
        rico_el_t *f = &M->el[fundo];
        f->x = (int16_t)x0; f->y = topo; f->l = (int16_t)larg; f->a = (int16_t)(*y - topo); f->cor = RC_BLOCO;
    }
    if (bt >= 0) {
        rico_el_t *b = &M->el[bt];
        b->x = (int16_t)(x0 + larg - RC_BOTAO_L - 4); b->y = topo + 2;
        b->l = RC_BOTAO_L; b->a = RC_BOTAO_A;
        b->off = (int)(ini - M->fonte);
        b->n = fim_cod > ini ? (int)(fim_cod - ini) : 0;
    }
    /* La moldura del bloque, encima del fondo. */
    el_ret(x0, topo, larg, 1, RC_BLOCO_BORDA);
    el_ret(x0, *y - 1, larg, 1, RC_BLOCO_BORDA);
    el_ret(x0, topo, 1, *y - topo, RC_BLOCO_BORDA);
    el_ret(x0 + larg - 1, topo, 1, *y - topo, RC_BLOCO_BORDA);
    M->conta.blocos_codigo++;
}

/* Maqueta una sección entera; la altura queda en s->alt. */
static void maquetar(rico_t *r, rico_secao_t *s)
{
    M = s;
    s->nel = 0;
    s->npool = 0;
    memset(&s->conta, 0, sizeof s->conta);
    int larg = r->l - RC_BARRA - 2 * RC_MARGEM - 2;
    if (larg < 64) larg = 64;
    M_cols = larg / LETRA_L;
    int y = 4;
    if (s->rotulo[0]) {
        el_texto(0, y, s->rotulo, (int)tam_texto(s->rotulo), RICO_NEGRITA, s->cor_rotulo);
        y += RC_LINHA + 2;
    }
    const char *p = s->fonte, *fim = s->fonte + s->n;
    bool primeiro = true;
    while (p < fim) {
        lin_t l = proxima(&p, fim);
        if (!s->markdown) {
            ncel = 0;
            for (int q = 0; q < l.n; q++) por(l.s[q], 0);
            fluir(0, M_cols, &y, COR_TEXTO);
            continue;
        }
        if (vazia(l)) { if (!primeiro) y += RC_HUECO; primeiro = true; continue; }
        primeiro = false;
        char ch, marca[12];
        int k, nivel, ini;
        if ((k = eh_cerca(l, &ch))) {
            y += 2;
            bloco_codigo(&p, fim, l, ch, k, 0, larg, &y);
            y += 2;
        } else if ((nivel = eh_titulo(l))) {
            int i = recuo(l) + nivel;
            while (i < l.n && l.s[i] == ' ') i++;
            int f = l.n;
            while (f > i && (l.s[f - 1] == '#' || l.s[f - 1] == ' ')) f--;
            uint32_t cor = nivel == 1 ? RC_TIT1 : nivel == 2 ? RC_TIT2 : RC_TIT3;
            y += 4;
            ncel = 0;
            inline_md(l.s + i, f - i, RICO_NEGRITA | RICO_TITULO, 0);
            fluir(0, M_cols, &y, cor);
            if (nivel <= 2) { el_ret(0, y, larg, 1, RC_LINHA_TIT); y += 3; }
            M->conta.titulos++;
        } else if (eh_regra(l)) {
            el_ret(0, y + RC_LINHA / 2, larg, 1, RC_BLOCO_BORDA);
            y += RC_LINHA;
        } else if ((ini = marcador(l, marca, sizeof marca))) {
            /* Un elemento de lista: el nivel sale de la sangría. */
            int prof = recuo(l) / 2;
            if (prof > RC_PROF_MAX) prof = RC_PROF_MAX;
            int x = prof * 16 + 4;
            int xt = x + (marca[0] ? ((int)tam_texto(marca) + 1) * LETRA_L : 14);
            if (marca[0]) el_texto(x, y, marca, (int)tam_texto(marca), 0, COR_TEXTO);
            else el_ret(x + 2, y + 6, 5, 5, COR_TEXTO);
            ncel = 0;
            inline_md(l.s + ini, l.n - ini, 0, 0);
            /* Las líneas siguientes sin marca siguen el mismo elemento. */
            while (p < fim) {
                const char *volta = p;
                lin_t c = proxima(&p, fim);
                if (vazia(c) || inicia_bloco(c)) { p = volta; break; }
                por(' ', 0);
                int q = recuo(c);
                inline_md(c.s + q, c.n - q, 0, 0);
            }
            fluir(xt, (larg - xt) / LETRA_L, &y, COR_TEXTO);
            M->conta.itens_lista++;
        } else if (eh_citacao(l)) {
            int topo = y;
            lin_t c = l;
            for (;;) {
                int q = recuo(c) + 1;
                if (q < c.n && c.s[q] == ' ') q++;
                ncel = 0;
                inline_md(c.s + q, c.n - q, 0, 0);
                fluir(14, M_cols - 2, &y, RC_CITA_TEXTO);
                if (p >= fim) break;
                const char *volta = p;
                c = proxima(&p, fim);
                if (!eh_citacao(c)) { p = volta; break; }
            }
            el_ret(2, topo, 3, y - topo, RC_CITA_BARRA);
        } else if (eh_tabela(l)) {
            /* Las tablas se ven como están escritas (la letra es de ancho fijo). */
            ncel = 0;
            for (int q = 0; q < l.n; q++) por(l.s[q], RICO_MONO);
            fluir(0, M_cols, &y, COR_TEXTO);
        } else {
            /* Un párrafo: cada salto de línea de la fuente es un salto en
             * pantalla (en un chat, el modelo los pone a propósito), y la
             * sangría se respeta (código pegado sin cercas). */
            ncel = 0;
            inline_md(l.s, l.n, 0, 0);
            fluir(0, M_cols, &y, COR_TEXTO);
        }
    }
    s->alt = y + 6;
    M = NULL;
}

/* Recalcula las posiciones de las secciones y el alto del documento. */
static void reposicionar(rico_t *r, int desde)
{
    int y = desde > 0 ? r->s[desde - 1].y + r->s[desde - 1].alt : 0;
    for (int i = desde; i < r->ns; i++) { r->s[i].y = y; y += r->s[i].alt; }
    r->total = y;
}

static int max_topo(rico_t *r)
{
    int m = r->total - (r->a - 2);
    return m > 0 ? m : 0;
}

/* ------------------------------------------------------------ API */

rico_t *rico_criar(int x, int y, int l, int a)
{
    rico_t *r = aloca_zerado(sizeof *r);
    if (!r) return NULL;
    r->x = x; r->y = y; r->l = l; r->a = a;
    r->sec_copiada = r->el_copiado = -1;
    return r;
}

static void liberar_secao(rico_secao_t *s)
{
    libera(s->fonte);
    libera(s->el);
    libera(s->pool);
    memset(s, 0, sizeof *s);
}

void rico_limpar(rico_t *r)
{
    for (int i = 0; i < r->ns; i++) liberar_secao(&r->s[i]);
    r->ns = 0;
    r->topo = 0;
    r->total = 0;
    r->sec_copiada = r->el_copiado = -1;
}

void rico_liberar(rico_t *r)
{
    if (!r) return;
    rico_limpar(r);
    libera(r->s);
    libera(r);
}

void rico_mover(rico_t *r, int x, int y, int l, int a)
{
    bool largura = l != r->l;
    r->x = x; r->y = y; r->l = l; r->a = a;
    if (largura) for (int i = 0; i < r->ns; i++) maquetar(r, &r->s[i]);
    reposicionar(r, 0);
    if (r->topo > max_topo(r)) r->topo = max_topo(r);
}

int rico_secao(rico_t *r, const char *rotulo, uint32_t cor, bool markdown)
{
    if (!crescer((void **)&r->s, &r->cap_s, r->ns + 1, sizeof(rico_secao_t))) return -1;
    rico_secao_t *s = &r->s[r->ns];
    memset(s, 0, sizeof *s);
    copia_texto(s->rotulo, rotulo ? rotulo : "", sizeof s->rotulo);
    s->cor_rotulo = cor;
    s->markdown = markdown;
    maquetar(r, s);
    r->ns++;
    reposicionar(r, r->ns - 1);
    return r->ns - 1;
}

void rico_juntar(rico_t *r, const char *t, int n)
{
    if (r->ns == 0 || n <= 0) return;
    rico_secao_t *s = &r->s[r->ns - 1];
    if (!crescer((void **)&s->fonte, &s->cap, s->n + n + 1, 1)) return;
    memcpy(s->fonte + s->n, t, (size_t)n);
    s->n += n;
    s->fonte[s->n] = 0;
    maquetar(r, s);
    reposicionar(r, r->ns - 1);
}

void rico_remover_ultima(rico_t *r)
{
    if (r->ns == 0) return;
    if (r->sec_copiada == r->ns - 1) r->sec_copiada = r->el_copiado = -1;
    liberar_secao(&r->s[--r->ns]);
    reposicionar(r, r->ns > 0 ? r->ns - 1 : 0);
    if (r->ns == 0) r->total = 0;
    if (r->topo > max_topo(r)) r->topo = max_topo(r);
}

int rico_secoes(const rico_t *r) { return r->ns; }

const char *rico_texto(const rico_t *r, int i, int *n)
{
    if (i < 0 || i >= r->ns) { if (n) *n = 0; return ""; }
    if (n) *n = r->s[i].n;
    return r->s[i].fonte ? r->s[i].fonte : "";
}

bool rico_no_fim(const rico_t *r)
{
    rico_t *q = (rico_t *)r;
    return r->topo >= max_topo(q) - RC_LINHA;
}

void rico_ir_fim(rico_t *r) { r->topo = max_topo(r); }

int rico_copiado(const rico_t *r) { return r->copiado_n; }

void rico_contar(const rico_t *r, int i, rico_contagem_t *c)
{
    memset(c, 0, sizeof *c);
    if (i >= 0 && i < r->ns) *c = r->s[i].conta;
}

/* La sección bajo el punto (coordenadas de la ventana), o -1. */
int rico_secao_em(const rico_t *r, int x, int y)
{
    if (x < r->x || x >= r->x + r->l || y < r->y || y >= r->y + r->a) return -1;
    int dy = y - r->y - 1 + r->topo;
    for (int i = 0; i < r->ns; i++)
        if (dy >= r->s[i].y && dy < r->s[i].y + r->s[i].alt) return i;
    return -1;
}

/* El botón «Copiar» número k de la sección i, en coordenadas de la ventana.
 * false si no existe o no está a la vista. */
bool rico_botao_copiar(const rico_t *r, int i, int k, retangulo_t *ret)
{
    if (i < 0 || i >= r->ns) return false;
    const rico_secao_t *s = &r->s[i];
    for (int e = 0; e < s->nel; e++) {
        if (s->el[e].tipo != EL_COPIAR || k-- > 0) continue;
        int x = r->x + 1 + RC_MARGEM + s->el[e].x;
        int y = r->y + 1 + s->y + s->el[e].y - r->topo;
        if (y < r->y || y + s->el[e].a > r->y + r->a) return false;
        ret->x = x; ret->y = y; ret->largura = s->el[e].l; ret->altura = s->el[e].a;
        return true;
    }
    return false;
}

/* ------------------------------------------------------------ dibujo */

typedef struct { int x0, y0, x1, y1; } corte_t;

static void ret_cortado(jan_t *j, const corte_t *c, int x, int y, int l, int a, uint32_t cor)
{
    int x1 = x + l, y1 = y + a;
    if (x < c->x0) x = c->x0;
    if (y < c->y0) y = c->y0;
    if (x1 > c->x1) x1 = c->x1;
    if (y1 > c->y1) y1 = c->y1;
    if (x < x1 && y < y1) g_retangulo(j, x, y, x1 - x, y1 - y, cor);
}

static inline void px_cortado(jan_t *j, const corte_t *c, int x, int y, uint32_t cor)
{
    if (x >= c->x0 && x < c->x1 && y >= c->y0 && y < c->y1) j->px[y * j->larg + x] = cor;
}

static void texto_cortado(jan_t *j, const corte_t *c, int x, int y, const char *t, int n,
                          uint8_t est, uint32_t cor)
{
    if (y + LETRA_A <= c->y0 || y >= c->y1) return;
    for (int k = 0; k < n; k++, x += LETRA_L) {
        if (x >= c->x1) break;
        if (x + LETRA_L + 2 <= c->x0) continue;
        const unsigned char *g = fonte_sagui[(uint8_t)t[k]];
        for (int lin = 0; lin < LETRA_A; lin++) {
            int yy = y + lin;
            if (yy < c->y0 || yy >= c->y1) continue;
            unsigned bits = g[lin];
            if (!bits) continue;
            int d = (est & RICO_CURSIVA) ? (LETRA_A - 1 - lin) / 4 - 1 : 0;
            for (int col = 0; col < 8; col++) {
                if (!(bits & (0x80u >> col))) continue;
                px_cortado(j, c, x + col + d, yy, cor);
                if (est & RICO_NEGRITA) px_cortado(j, c, x + col + d + 1, yy, cor);
            }
        }
        if (est & RICO_ENLACE)
            for (int col = 0; col < LETRA_L; col++) px_cortado(j, c, x + col, y + LETRA_A - 2, cor);
    }
}

void rico_desenhar(jan_t *j, rico_t *r)
{
    if (r->topo > max_topo(r)) r->topo = max_topo(r);
    if (r->topo < 0) r->topo = 0;
    g_retangulo(j, r->x, r->y, r->l, r->a, RC_FUNDO);
    g_moldura(j, r->x, r->y, r->l, r->a, COR_BORDA);
    corte_t c = { r->x + 1, r->y + 1, r->x + r->l - RC_BARRA - 1, r->y + r->a - 1 };
    if (c.x1 > j->larg) c.x1 = j->larg;
    if (c.y1 > j->alt) c.y1 = j->alt;
    if (c.x0 < 0) c.x0 = 0;
    if (c.y0 < 0) c.y0 = 0;
    int bx = r->x + 1 + RC_MARGEM;
    for (int i = 0; i < r->ns; i++) {
        rico_secao_t *s = &r->s[i];
        int by = r->y + 1 + s->y - r->topo;
        if (by + s->alt <= c.y0 || by >= c.y1) continue;
        if (i > 0) ret_cortado(j, &c, c.x0 + 4, by, c.x1 - c.x0 - 8, 1, RC_SEPARADOR);
        for (int e = 0; e < s->nel; e++) {
            rico_el_t *el = &s->el[e];
            int x = bx + el->x, y = by + el->y;
            if (el->tipo == EL_RET) ret_cortado(j, &c, x, y, el->l, el->a, el->cor);
            else if (el->tipo == EL_TEXTO) texto_cortado(j, &c, x, y, s->pool + el->off, el->n, el->est, el->cor);
            else {
                bool feito = i == r->sec_copiada && e == r->el_copiado;
                const char *t = feito ? "Copiado" : "Copiar";
                ret_cortado(j, &c, x, y, el->l, el->a, feito ? 0xD6EEDD : COR_BOTAO);
                ret_cortado(j, &c, x, y, el->l, 1, COR_BORDA);
                ret_cortado(j, &c, x, y + el->a - 1, el->l, 1, COR_BORDA);
                ret_cortado(j, &c, x, y, 1, el->a, COR_BORDA);
                ret_cortado(j, &c, x + el->l - 1, y, 1, el->a, COR_BORDA);
                int n = (int)tam_texto(t);
                texto_cortado(j, &c, x + (el->l - n * LETRA_L) / 2, y + 1, t, n, 0, COR_TEXTO);
            }
        }
    }
    /* La barra de desplazamiento. */
    int bxr = r->x + r->l - RC_BARRA - 1, trilho = r->a - 2;
    g_retangulo(j, bxr, r->y + 1, RC_BARRA, trilho, 0xEDEFF1);
    if (r->total > trilho && trilho > 0) {
        int pa = (int)((long long)trilho * trilho / r->total);
        if (pa < 16) pa = 16;
        if (pa > trilho) pa = trilho;
        int mt = max_topo(r);
        int py = mt > 0 ? (int)((long long)(trilho - pa) * r->topo / mt) : 0;
        g_retangulo(j, bxr + 2, r->y + 1 + py, RC_BARRA - 4, pa, RC_BLOCO_BORDA);
    }
    jan_mostrar_area(j, r->x, r->y, r->l, r->a);
    GR_REG(j, "rico", r, r->x, r->y, r->l, r->a, "", 0, 0);
}

int rico_tratar(jan_t *j, rico_t *r, const evento_t *e)
{
    bool dentro = e->x >= r->x && e->x < r->x + r->l && e->y >= r->y && e->y < r->y + r->a;
    int antes = r->topo;
    if (e->tipo == EV_RODA) {
        if (!dentro) return RICO_NADA;
        r->topo += (int)e->cliques * RODA_LINHAS * RC_LINHA;
    } else if (e->tipo == EV_TECLA) {
        int pagina = r->a - 2 * RC_LINHA;
        if (pagina < RC_LINHA) pagina = RC_LINHA;
        if (e->tecla == TECLA_PGUP) r->topo -= pagina;
        else if (e->tecla == TECLA_PGDN) r->topo += pagina;
        else if (e->tecla == TECLA_HOME && (e->modificadores & 2)) r->topo = 0;
        else if (e->tecla == TECLA_END && (e->modificadores & 2)) r->topo = max_topo(r);
        else return RICO_NADA;
    } else if (e->tipo == EV_CLIQUE && (e->botoes & 1) && dentro) {
        if (e->x >= r->x + r->l - RC_BARRA - 1) {
            /* Clic en la barra: salta a esa altura del documento. */
            int trilho = r->a - 2;
            r->topo = (int)((long long)max_topo(r) * (e->y - r->y - 1) / (trilho > 1 ? trilho - 1 : 1));
        } else {
            int i = rico_secao_em(r, e->x, e->y);
            if (i < 0) return RICO_NADA;
            rico_secao_t *s = &r->s[i];
            int bx = r->x + 1 + RC_MARGEM, by = r->y + 1 + s->y - r->topo;
            for (int k = 0; k < s->nel; k++) {
                rico_el_t *el = &s->el[k];
                if (el->tipo != EL_COPIAR) continue;
                if (e->x >= bx + el->x && e->x < bx + el->x + el->l &&
                    e->y >= by + el->y && e->y < by + el->y + el->a) {
                    int n = el->n;
                    if (el->off + n > s->n) n = s->n - el->off;
                    if (n < 0) n = 0;
                    r->copiado_n = copiar_texto(s->fonte + el->off, (unsigned)n);
                    r->sec_copiada = i;
                    r->el_copiado = k;
                    rico_desenhar(j, r);
                    return RICO_COPIOU;
                }
            }
            return RICO_NADA;
        }
    } else return RICO_NADA;
    if (r->topo > max_topo(r)) r->topo = max_topo(r);
    if (r->topo < 0) r->topo = 0;
    if (r->topo != antes) rico_desenhar(j, r);
    return RICO_ROLOU;
}
