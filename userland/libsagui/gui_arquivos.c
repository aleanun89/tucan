// SPDX-License-Identifier: GPL-2.0-or-later
/* gui_arquivos.c - diálogos comunes de abrir, guardar y elegir carpeta (mejora 103, D272).
 *
 * Tres diálogos modales con el aspecto clásico de Windows 95 (gris, biseles, selección azul
 * marino) que corren en el propio programa, como el resto de la libsagui:
 *
 *     dialogo_abrir  (titulo, filtros, inicial, salida, max)
 *     dialogo_guardar(titulo, filtros, inicial, salida, max)
 *     dialogo_carpeta(titulo, inicial, salida, max)
 *
 * "filtros" son pares «Nombre|patrón|Nombre|patrón» (el patrón admite varios separados por «;»):
 * "Texto (*.txt)|*.txt|Todos|*". NULL = todos los archivos. "inicial" es una carpeta o la ruta de
 * un archivo (carpeta + nombre propuesto). El teclado lo hace todo: Tab/Mayús+Tab cambia de
 * control, flechas y RePág/AvPág mueven la lista, Intro acepta o entra en la carpeta, Retroceso
 * (en la lista) sube un nivel, escribir en la lista salta al nombre, Esc cancela. En «Nombre de
 * archivo» se puede escribir una ruta entera (absoluta o relativa a la carpeta) o un patrón con
 * «*» o «?», que filtra la lista. */
#include "gui.h"
#include "icones.h"

#define DA_L   560
#define DA_A   372
#define CINZA  W98_FACE     /* D428: los colores salen del esquema */
#define AZUL95 W98_AZUL
#define FILA   18
#define RUTA_MAX 128

/* Geometría (ventana de abrir y guardar). */
#define X_COMBO 84
#define L_COMBO 260
#define Y_COMBO 8
#define X_SUBIR 352
#define X_NOVA  404
#define X_VISTA 472
#define LI_X 10
#define LI_Y 40
#define LI_L 540
#define LI_A 232
#define X_NOME 160
#define Y_NOME 278
#define L_NOME 290
#define X_BOT 462
#define Y_TIPO 308
#define Y_ESTADO 344

enum { F_NOME, F_LISTA, F_COMBO, F_TIPO, F_SUBIR, F_NOVA, F_VISTA, F_OK, F_CANC, F_N };

typedef struct {
    jan_t *j;
    int guardar;
    char pasta[RUTA_MAX];
    entrada_pasta_t *ent; int nent;
    int *vis; int nvis;
    int topo, sel;
    int detalhes;
    char fnome[8][56]; char fpad[8][72]; int nf, fsel;
    char padrao_extra[48];
    caixa_t nome;
    int foco;
    char aviso[80];
    char saida[RUTA_MAX];
    char busca[16]; unsigned busca_t;
} dlg_t;

typedef struct { char t[24][100]; char p[24][RUTA_MAX]; int n; } pop_t;

/* ------------------------------------------------------------ dibujo estilo Windows 95 */

static void bisel(jan_t *j, int x, int y, int l, int a, bool af)
{
    uint32_t e1 = af ? W98_SOMBRA : W98_LUZ, e2 = af ? W98_LUZ : W98_NEGRO;
    uint32_t i1 = af ? W98_OSCURO : W98_CLARO, i2 = af ? W98_CLARO : W98_SOMBRA;
    g_retangulo(j, x, y, l, 1, e1); g_retangulo(j, x, y, 1, a, e1);
    g_retangulo(j, x, y + a - 1, l, 1, e2); g_retangulo(j, x + l - 1, y, 1, a, e2);
    g_retangulo(j, x + 1, y + 1, l - 2, 1, i1); g_retangulo(j, x + 1, y + 1, 1, a - 2, i1);
    g_retangulo(j, x + 1, y + a - 2, l - 2, 1, i2); g_retangulo(j, x + l - 2, y + 1, 1, a - 2, i2);
}

static void pontilhado(jan_t *j, int x, int y, int l, int a)
{
    for (int i = 0; i < l; i += 2) { g_retangulo(j, x + i, y, 1, 1, 0); g_retangulo(j, x + i, y + a - 1, 1, 1, 0); }
    for (int i = 0; i < a; i += 2) { g_retangulo(j, x, y + i, 1, 1, 0); g_retangulo(j, x + l - 1, y + i, 1, 1, 0); }
}

static void botao95(jan_t *j, int x, int y, int l, int a, const char *t, bool foco)
{
    g_retangulo(j, x, y, l, a, CINZA);
    bisel(j, x, y, l, a, false);
    if (foco) { g_moldura(j, x, y, l, a, W98_NEGRO); }
    int w = g_largura_texto(t);
    g_texto(j, x + (l - w) / 2, y + (a - LETRA_A) / 2, t, W98_NEGRO);
    if (foco) pontilhado(j, x + 4, y + 4, l - 8, a - 8);
}

static void seta_baixo(jan_t *j, int x, int y)
{
    for (int i = 0; i < 4; i++) g_retangulo(j, x + i, y + i, 7 - 2 * i, 1, W98_NEGRO);
}

static void combo95(jan_t *j, int x, int y, int l, const char *t, bool com_pasta, bool foco)
{
    g_retangulo(j, x, y, l, 24, COR_CAMPO);
    bisel(j, x, y, l, 24, true);
    int tx = x + 4;
    if (com_pasta) { icone_desenhar(j, tx, y + 4, "pasta", 1); tx += 20; }
    int cols = (x + l - 24 - tx) / LETRA_L;
    int n = (int)tam_texto(t);
    if (n > cols) {   /* ruta larga: se ve el final, que es lo importante */
        g_texto(j, tx, y + 4, "...", COR_TEXTO);
        g_texto_n(j, tx + 24, y + 4, t + n - (cols - 3), cols - 3, COR_TEXTO);
    } else g_texto_n(j, tx, y + 4, t, cols, COR_TEXTO);
    if (foco) { g_retangulo(j, x + 3, y + 3, l - 25, 18, AZUL95);
        tx = x + 4;
        if (com_pasta) { icone_desenhar(j, tx, y + 4, "pasta", 1); tx += 20; }
        if (n > cols) {
            g_texto(j, tx, y + 4, "...", COR_SEL_TEXTO);
            g_texto_n(j, tx + 24, y + 4, t + n - (cols - 3), cols - 3, COR_SEL_TEXTO);
        } else g_texto_n(j, tx, y + 4, t, cols, COR_SEL_TEXTO);
    }
    g_retangulo(j, x + l - 22, y + 2, 20, 20, CINZA);
    bisel(j, x + l - 22, y + 2, 20, 20, false);
    seta_baixo(j, x + l - 16, y + 9);
}

static bool dentro(int px, int py, int x, int y, int l, int a)
{
    return px >= x && py >= y && px < x + l && py < y + a;
}

/* ------------------------------------------------------------ rutas */

/* Resuelve "." y ".." y deja una ruta absoluta sin barra final (la raíz es "/"). */
static void normalizar(const char *base, const char *in, char *out, int max)
{
    char tmp[256];
    tmp[0] = 0;
    if (in[0] != '/') {
        copia_texto(tmp, base, sizeof tmp);
        size_t n = tam_texto(tmp);
        if (n && tmp[n - 1] != '/' && n < sizeof tmp - 1) { tmp[n++] = '/'; tmp[n] = 0; }
    }
    size_t n0 = tam_texto(tmp);
    copia_texto(tmp + n0, in, sizeof tmp - n0);
    int ol = 0;
    out[0] = 0;
    const char *p = tmp;
    while (*p) {
        while (*p == '/') p++;
        const char *s = p;
        while (*p && *p != '/') p++;
        int sl = (int)(p - s);
        if (sl == 0 || (sl == 1 && s[0] == '.')) continue;
        if (sl == 2 && s[0] == '.' && s[1] == '.') {
            while (ol > 0 && out[ol - 1] != '/') ol--;
            if (ol > 0) ol--;
            out[ol] = 0;
            continue;
        }
        if (ol + 1 + sl >= max) break;
        out[ol++] = '/';
        memcpy(out + ol, s, (size_t)sl);
        ol += sl;
        out[ol] = 0;
    }
    if (ol == 0) { out[0] = '/'; out[1] = 0; }
}

static bool eh_pasta(const char *c)
{
    info_arquivo_t i;
    return info_arquivo(c, &i) == 0 && i.tipo == TIPO_PASTA;
}

static bool existe(const char *c, unsigned *tipo)
{
    info_arquivo_t i;
    if (info_arquivo(c, &i) != 0) return false;
    if (tipo) *tipo = i.tipo;
    return true;
}

/* ¿Se puede leer la carpeta? (D255: /sistema/conf y otras son solo de root.) */
static bool legivel(const char *c)
{
    entrada_pasta_t e;
    return listar(c, 0, &e) != ERRO_ACESSO;
}

static char minuscula(char c) { return c >= 'A' && c <= 'Z' ? (char)(c + 32) : c; }

static void minusculas(char *dst, const char *src, int max)
{
    int i = 0;
    for (; src[i] && i < max - 1; i++) dst[i] = minuscula(src[i]);
    dst[i] = 0;
}

/* ------------------------------------------------------------ carpetas recientes */

#define REC_N 6
#define REC_ARQ "/home/.config/dialogos.rec"
static char rec[REC_N][RUTA_MAX];
static int nrec, rec_lido;

static void rec_ler(void)
{
    if (rec_lido) return;
    rec_lido = 1;
    int fd = abrir(REC_ARQ, ABRIR_LER);
    if (fd < 0) return;
    char b[REC_N * RUTA_MAX];
    int n = ler(fd, b, sizeof b - 1);
    fechar(fd);
    if (n <= 0) return;
    b[n] = 0;
    char *p = b;
    while (*p && nrec < REC_N) {
        char *f = p;
        while (*f && *f != '\n') f++;
        int l = (int)(f - p);
        if (l > 0 && l < RUTA_MAX && p[0] == '/') { memcpy(rec[nrec], p, (size_t)l); rec[nrec][l] = 0; nrec++; }
        p = *f ? f + 1 : f;
    }
}

static void rec_gravar(void)
{
    int fd = abrir(REC_ARQ, ABRIR_ESCREVER | ABRIR_CRIAR | ABRIR_TRUNCAR);
    if (fd < 0) return;
    for (int i = 0; i < nrec; i++) { escrever(fd, rec[i], (unsigned)tam_texto(rec[i])); escrever(fd, "\n", 1); }
    fechar(fd);
}

static void rec_anotar(const char *pasta)
{
    rec_ler();
    int k = -1;
    for (int i = 0; i < nrec; i++) if (compara_texto(rec[i], pasta) == 0) k = i;
    if (k < 0) { k = nrec < REC_N ? nrec++ : REC_N - 1; }
    for (int i = k; i > 0; i--) memcpy(rec[i], rec[i - 1], RUTA_MAX);
    copia_texto(rec[0], pasta, RUTA_MAX);
    rec_gravar();
}

/* ------------------------------------------------------------ desplegable */

/* Lista emergente bajo (o sobre) un combo. Devuelve el índice, -1 = cancelado, -3 = cerraron la ventana. */
static int desplegar(jan_t *j, int x, int y, int l, int alto_combo, const pop_t *p, int sel)
{
    int filas = p->n < 10 ? p->n : 10;
    int a = filas * FILA + 4;
    int py = y + alto_combo;
    if (py + a > j->alt) py = y - a;
    if (py < 0) py = 0;
    int topo = 0;
    if (sel < 0) sel = 0;
    evento_t e;
    for (;;) {
        if (sel < topo) topo = sel;
        if (sel >= topo + filas) topo = sel - filas + 1;
        g_retangulo(j, x, py, l, a, COR_CAMPO);
        g_moldura(j, x, py, l, a, W98_NEGRO);
        for (int k = 0; k < filas; k++) {
            int i = topo + k, yy = py + 2 + k * FILA;
            if (i == sel) g_retangulo(j, x + 2, yy, l - 4, FILA, AZUL95);
            g_texto_n(j, x + 6, yy + 1, p->t[i], (l - 12) / LETRA_L, i == sel ? COR_SEL_TEXTO : COR_TEXTO);
        }
        jan_mostrar(j);
        for (;;) {
            if (proximo_evento(j->id, &e, true) != 1) continue;
            if (e.tipo == EV_FECHAR) return -3;
            if (e.tipo == EV_CLIQUE) {
                if (!dentro(e.x, e.y, x, py, l, a)) return -1;
                int i = topo + (e.y - py - 2) / FILA;
                if (e.y - py - 2 >= 0 && i < p->n) return i;
                continue;
            }
            if (e.tipo == EV_TECLA) {
                if (e.tecla == TECLA_ESC) return -1;
                if (e.tecla == '\n') return sel;
                if (e.tecla == TECLA_BAIXO && sel < p->n - 1) sel++;
                else if (e.tecla == TECLA_CIMA && sel > 0) sel--;
                else if (e.tecla == TECLA_HOME) sel = 0;
                else if (e.tecla == TECLA_END) sel = p->n - 1;
                else if (e.tecla == TECLA_PGDN) { sel += filas; if (sel > p->n - 1) sel = p->n - 1; }
                else if (e.tecla == TECLA_PGUP) { sel -= filas; if (sel < 0) sel = 0; }
                else continue;
                break;
            }
        }
    }
}

/* ------------------------------------------------------------ filtros */

static void filtros_ler(dlg_t *d, const char *f)
{
    d->nf = 0;
    d->fsel = 0;
    while (f && *f && d->nf < 8) {
        const char *s = f;
        if (*s == '>') { d->fsel = d->nf; s++; f++; }   /* «>Nombre»: tipo elegido de entrada */
        while (*f && *f != '|') f++;
        int l = (int)(f - s);
        if (l > 55) l = 55;
        memcpy(d->fnome[d->nf], s, (size_t)l);
        d->fnome[d->nf][l] = 0;
        if (!*f) break;
        f++;
        s = f;
        while (*f && *f != '|') f++;
        l = (int)(f - s);
        if (l > 71) l = 71;
        memcpy(d->fpad[d->nf], s, (size_t)l);
        d->fpad[d->nf][l] = 0;
        d->nf++;
        if (*f) f++;
    }
    if (d->nf == 0) {
        copia_texto(d->fnome[0], _("Todos los archivos (*.*)"), sizeof d->fnome[0]);
        copia_texto(d->fpad[0], "*", sizeof d->fpad[0]);
        d->nf = 1;
    }
}

/* ¿El nombre pasa el patrón (varios, separados por «;»)? Sin distinguir mayúsculas. */
static bool casa_padrao(const char *padroes, const char *nome)
{
    char n[64], p[72];
    minusculas(n, nome, sizeof n);
    const char *s = padroes;
    for (;;) {
        while (*s == ' ') s++;
        int l = 0;
        while (s[l] && s[l] != ';') l++;
        if (l > 0 && l < (int)sizeof p) {
            memcpy(p, s, (size_t)l);
            p[l] = 0;
            char m[72];
            minusculas(m, p, sizeof m);
            if (compara_texto(m, "*.*") == 0) return true;
            if (casar_glob(m, n)) return true;
        }
        if (!s[l]) break;
        s += l + 1;
    }
    return false;
}

static void refiltrar(dlg_t *d)
{
    if (d->vis) libera(d->vis);
    d->vis = aloca((size_t)(d->nent + 1) * sizeof(int));
    d->nvis = 0;
    if (!d->vis) return;
    const char *pad = d->padrao_extra[0] ? d->padrao_extra : d->fpad[d->fsel];
    for (int pasta = 1; pasta >= 0; pasta--)   /* primero las carpetas */
        for (int i = 0; i < d->nent; i++) {
            const char *n = d->ent[i].nome;
            bool ep = d->ent[i].tipo == TIPO_PASTA;
            if (n[0] == '.' || ep != (bool)pasta) continue;
            if (ep || casa_padrao(pad, n)) d->vis[d->nvis++] = i;
        }
    if (d->sel >= d->nvis) d->sel = d->nvis - 1;
    if (d->sel < 0) d->sel = 0;
    registrar("dialogo: %d elementos en %s", d->nvis, d->pasta);
}

/* ------------------------------------------------------------ carga de la carpeta */

static bool entrar(dlg_t *d, const char *destino)
{
    char nova[RUTA_MAX];
    normalizar(d->pasta, destino, nova, sizeof nova);
    entrada_pasta_t *v = NULL;
    int n = listar_tudo(nova, &v);
    if (n < 0) {
        copia_texto(d->aviso, n == ERRO_ACESSO ? _("Sin permiso para entrar en esa carpeta.") : _("No se puede abrir esa carpeta."), sizeof d->aviso);
        return false;
    }
    if (d->ent) libera(d->ent);
    d->ent = v;
    d->nent = n;
    copia_texto(d->pasta, nova, sizeof d->pasta);
    d->topo = d->sel = 0;
    d->aviso[0] = 0;
    refiltrar(d);
    return true;
}

static void tipo_texto(const entrada_pasta_t *e, char *s, int max)
{
    if (e->tipo == TIPO_PASTA) { copia_texto(s, _("Carpeta"), (size_t)max); return; }
    if (e->tipo == TIPO_PROGRAMA) { copia_texto(s, _("Programa"), (size_t)max); return; }
    const char *pt = NULL;
    for (const char *c = e->nome; *c; c++) if (*c == '.') pt = c;
    if (!pt || !pt[1]) { copia_texto(s, _("Archivo"), (size_t)max); return; }
    char x[12];
    int i = 0;
    for (pt++; *pt && i < 10; pt++, i++) x[i] = (*pt >= 'a' && *pt <= 'z') ? (char)(*pt - 32) : *pt;
    x[i] = 0;
    formatar_texto(s, (size_t)max, _("Archivo %s"), x);
}

/* ------------------------------------------------------------ dibujo del diálogo */

static int filas_visibles(const dlg_t *d)
{
    return (LI_Y + LI_A - 3 - (LI_Y + 2 + (d->detalhes ? FILA : 0))) / FILA;
}

static void limitar_topo(dlg_t *d)
{
    int v = filas_visibles(d), max = d->nvis - v;
    if (d->topo > max) d->topo = max;
    if (d->topo < 0) d->topo = 0;
}

/* Después de mover la selección: que la fila se vea. */
static void ver_sel(dlg_t *d)
{
    int v = filas_visibles(d);
    if (d->sel < d->topo) d->topo = d->sel;
    if (d->sel >= d->topo + v) d->topo = d->sel - v + 1;
    limitar_topo(d);
}

static void desenhar_lista(dlg_t *d)
{
    jan_t *j = d->j;
    g_retangulo(j, LI_X, LI_Y, LI_L, LI_A, COR_CAMPO);
    bisel(j, LI_X, LI_Y, LI_L, LI_A, true);
    int y0 = LI_Y + 2;
    if (d->detalhes) {
        static const int cx[3] = { 0, 300, 400 };
        static const char *const tt[3] = { N_("Nombre"), N_("Tamaño"), N_("Tipo") };
        for (int k = 0; k < 3; k++) {
            int x = LI_X + 2 + cx[k];
            int l = k == 2 ? LI_L - 4 - cx[2] : cx[k + 1] - cx[k];
            g_retangulo(j, x, y0, l, FILA, CINZA);
            bisel(j, x, y0, l, FILA, false);
            g_texto(j, x + 6, y0 + 1, _(tt[k]), 0);
        }
        y0 += FILA;
    }
    int v = filas_visibles(d);
    for (int k = 0; k < v && d->topo + k < d->nvis; k++) {
        int i = d->topo + k, y = y0 + k * FILA;
        const entrada_pasta_t *e = &d->ent[d->vis[i]];
        bool sel = i == d->sel;
        uint32_t cf = d->foco == F_LISTA ? AZUL95 : W98_SOMBRA;
        icone_desenhar(j, LI_X + 6, y + 1, e->tipo == TIPO_PASTA ? "pasta" : "formulario", 1);
        int tx = LI_X + 26;
        int cols = ((d->detalhes ? 300 : LI_L - 20) - 26) / LETRA_L;
        int nn = (int)tam_texto(e->nome);
        int wsel = (nn < cols ? nn : cols) * LETRA_L + 4;
        if (sel) g_retangulo(j, tx - 2, y, wsel, FILA, cf);
        g_texto_n(j, tx, y + 1, e->nome, cols, sel ? COR_SEL_TEXTO : COR_TEXTO);
        if (d->detalhes) {
            char t[24];
            if (e->tipo != TIPO_PASTA) {
                formatar_tamanho(t, sizeof t, e->tamanho);
                g_texto(j, LI_X + 2 + 306, y + 1, t, COR_TEXTO);
            }
            tipo_texto(e, t, sizeof t);
            g_texto_n(j, LI_X + 2 + 406, y + 1, t, 16, COR_TEXTO);
        }
        if (sel && d->foco == F_LISTA) pontilhado(j, tx - 2, y, wsel, FILA);
    }
    if (d->nvis > v) {
        int tr = LI_A - 6 - (d->detalhes ? FILA : 0), ax = LI_X + LI_L - 14;
        g_retangulo(j, ax, y0, 12, tr, W98_CLARO);
        int pa = tr * v / d->nvis;
        if (pa < 10) pa = 10;
        int py = (tr - pa) * d->topo / (d->nvis - v);
        g_retangulo(j, ax, y0 + py, 12, pa, CINZA);
        bisel(j, ax, y0 + py, 12, pa, false);
    }
}

static void desenhar_tudo(dlg_t *d)
{
    jan_t *j = d->j;
    limitar_topo(d);
    g_retangulo(j, 0, 0, DA_L, DA_A, CINZA);
    g_texto(j, 10, Y_COMBO + 4, _("Buscar en:"), 0);
    combo95(j, X_COMBO, Y_COMBO, L_COMBO, d->pasta, true, d->foco == F_COMBO);
    botao95(j, X_SUBIR, Y_COMBO, 48, 24, _("Subir"), d->foco == F_SUBIR);
    botao95(j, X_NOVA, Y_COMBO, 64, 24, _("Nueva"), d->foco == F_NOVA);
    botao95(j, X_VISTA, Y_COMBO, 78, 24, d->detalhes ? _("Lista") : _("Detalles"), d->foco == F_VISTA);
    desenhar_lista(d);
    g_texto(j, 10, Y_NOME + 4, _("Nombre de archivo:"), 0);
    d->nome.x = X_NOME; d->nome.y = Y_NOME; d->nome.l = L_NOME;
    d->nome.foco = d->foco == F_NOME;
    botao95(j, X_BOT, Y_NOME, 88, 24, d->guardar ? _("Guardar") : _("Abrir"), d->foco == F_OK);
    g_texto(j, 10, Y_TIPO + 4, d->guardar ? _("Guardar como tipo:") : _("Tipo:"), 0);
    combo95(j, X_NOME, Y_TIPO, L_NOME, d->padrao_extra[0] ? d->padrao_extra : d->fnome[d->fsel], false, d->foco == F_TIPO);
    botao95(j, X_BOT, Y_TIPO, 88, 24, _("Cancelar"), d->foco == F_CANC);
    bisel(j, 10, Y_ESTADO - 2, DA_L - 20, 22, true);
    if (d->aviso[0]) g_texto(j, 16, Y_ESTADO + 1, d->aviso, 0x800000);
    else {
        char t[80];
        formatar_texto(t, sizeof t, _("%d elementos"), d->nvis);
        g_texto(j, 16, Y_ESTADO + 1, t, W98_OSCURO);
    }
    caixa_desenhar(j, &d->nome);
    jan_mostrar(j);
}

/* ------------------------------------------------------------ acciones */

static void copiar_nome_sel(dlg_t *d)
{
    if (d->sel < 0 || d->sel >= d->nvis) return;
    const entrada_pasta_t *e = &d->ent[d->vis[d->sel]];
    if (e->tipo != TIPO_PASTA) caixa_definir(&d->nome, e->nome);
}


static void mover_sel(dlg_t *d, unsigned t)
{
    int v = filas_visibles(d), n = d->nvis;
    if (n == 0) return;
    if (t == TECLA_BAIXO && d->sel < n - 1) d->sel++;
    else if (t == TECLA_CIMA && d->sel > 0) d->sel--;
    else if (t == TECLA_HOME) d->sel = 0;
    else if (t == TECLA_END) d->sel = n - 1;
    else if (t == TECLA_PGDN) { d->sel += v; if (d->sel > n - 1) d->sel = n - 1; }
    else if (t == TECLA_PGUP) { d->sel -= v; if (d->sel < 0) d->sel = 0; }
    copiar_nome_sel(d);
}

static void subir_nivel(dlg_t *d)
{
    char ant[RUTA_MAX], nome[64];
    copia_texto(ant, d->pasta, sizeof ant);
    copia_texto(nome, nome_base(ant), sizeof nome);
    if (entrar(d, "..")) {   /* deja seleccionada la carpeta de la que se sale */
        for (int i = 0; i < d->nvis; i++)
            if (compara_texto(d->ent[d->vis[i]].nome, nome) == 0) { d->sel = i; break; }
    }
}

/* Salta al primer nombre que empieza por lo que se va tecleando (se olvida tras 1 s). */
static void saltar_nome(dlg_t *d, char c)
{
    unsigned ahora = tempo_ms();
    size_t n = tam_texto(d->busca);
    if (ahora - d->busca_t > 1000) n = 0;
    d->busca_t = ahora;
    if (n < sizeof d->busca - 1) { d->busca[n++] = minuscula(c); d->busca[n] = 0; }
    int ini = n == 1 ? d->sel + 1 : d->sel;   /* una sola letra repetida recorre las que coinciden */
    for (int k = 0; k < d->nvis; k++) {
        int i = (ini + k) % d->nvis;
        char m[64];
        minusculas(m, d->ent[d->vis[i]].nome, sizeof m);
        if (comeca_com(m, d->busca)) { d->sel = i; copiar_nome_sel(d); return; }
    }
}

static void escolher_pasta_combo(dlg_t *d, int *fim)
{
    pop_t *p = aloca(sizeof *p);
    if (!p) return;
    p->n = 0;
    /* los niveles de la ruta actual, de la raíz hacia dentro */
    char acum[RUTA_MAX];
    copia_texto(p->t[0], "/", sizeof p->t[0]);
    copia_texto(p->p[0], "/", sizeof p->p[0]);
    p->n = 1;
    int sel = 0, prof = 1;
    const char *s = d->pasta;
    acum[0] = 0;
    while (*s && p->n < 12) {
        while (*s == '/') s++;
        const char *f = s;
        while (*s && *s != '/') s++;
        int l = (int)(s - f);
        if (!l) break;
        size_t an = tam_texto(acum);
        if (an + 1 + (size_t)l >= sizeof acum) break;
        acum[an] = '/';
        memcpy(acum + an + 1, f, (size_t)l);
        acum[an + 1 + l] = 0;
        char nm[64];
        int ll = l < 63 ? l : 63;
        memcpy(nm, f, (size_t)ll);
        nm[ll] = 0;
        int sp = prof * 2 < 20 ? prof * 2 : 20;
        for (int q = 0; q < sp; q++) p->t[p->n][q] = ' ';
        copia_texto(p->t[p->n] + sp, nm, sizeof p->t[0] - (size_t)sp);
        copia_texto(p->p[p->n], acum, sizeof p->p[0]);
        sel = p->n++;
        prof++;
    }
    /* unidades y carpetas del sistema */
    static const char *const fijas[] = { "/", "/home", "/usb", "/jogos", "/apps" };
    char (*extra)[RUTA_MAX] = aloca(24 * RUTA_MAX);
    int ne = 0;
    if (extra) {
        for (unsigned i = 0; i < sizeof fijas / sizeof *fijas; i++)
            if (eh_pasta(fijas[i]) && legivel(fijas[i])) copia_texto(extra[ne++], fijas[i], RUTA_MAX);
        info_montagem_t mt[16];
        int nm = info_sistema(INFO_MONTAGENS, mt, 16);
        for (int i = 0; i < nm && ne < 20; i++) {
            if (!mt[i].ponto[0] || !legivel(mt[i].ponto)) continue;
            bool ja = false;
            for (int k = 0; k < ne; k++) if (compara_texto(extra[k], mt[i].ponto) == 0) ja = true;
            if (!ja) copia_texto(extra[ne++], mt[i].ponto, RUTA_MAX);
        }
        for (int i = 0; i < ne && p->n < 24; i++) {
            formatar_texto(p->t[p->n], sizeof p->t[0], _("Unidad: %s"), extra[i]);
            copia_texto(p->p[p->n], extra[i], sizeof p->p[0]);
            p->n++;
        }
        libera(extra);
    }
    rec_ler();
    for (int i = 0; i < nrec && p->n < 24; i++) {
        if (!legivel(rec[i])) continue;
        formatar_texto(p->t[p->n], sizeof p->t[0], _("Reciente: %s"), rec[i]);
        copia_texto(p->p[p->n], rec[i], sizeof p->p[0]);
        p->n++;
    }
    int r = desplegar(d->j, X_COMBO, Y_COMBO, L_COMBO, 24, p, sel);
    if (r == -3) *fim = 1;
    else if (r >= 0) entrar(d, p->p[r]);
    libera(p);
}

static void escolher_tipo(dlg_t *d, int *fim)
{
    pop_t *p = aloca(sizeof *p);
    if (!p) return;
    p->n = d->nf;
    for (int i = 0; i < d->nf; i++) copia_texto(p->t[i], d->fnome[i], sizeof p->t[0]);
    int r = desplegar(d->j, X_NOME, Y_TIPO, L_NOME, 24, p, d->fsel);
    if (r == -3) *fim = 1;
    else if (r >= 0) { d->fsel = r; d->padrao_extra[0] = 0; refiltrar(d); }
    libera(p);
}

static void nova_pasta(dlg_t *d)
{
    char n[64] = "";
    if (!pedir_texto(_("Nueva carpeta"), _("Nombre de la carpeta:"), n, sizeof n) || !n[0]) return;
    char c[RUTA_MAX];
    normalizar(d->pasta, n, c, sizeof c);
    if (criar_pasta(c) < 0) { copia_texto(d->aviso, _("No se pudo crear la carpeta."), sizeof d->aviso); return; }
    entrar(d, d->pasta);
    for (int i = 0; i < d->nvis; i++)
        if (compara_texto(d->ent[d->vis[i]].nome, nome_base(c)) == 0) { d->sel = i; break; }
}

/* Añade la extensión del tipo elegido (si es «*.ext» sencillo) a un nombre sin punto. */
static void poner_extensao(const dlg_t *d, char *c, int max)
{
    const char *b = nome_base(c);
    for (const char *k = b; *k; k++) if (*k == '.') return;
    const char *p = d->fpad[d->fsel];
    if (p[0] != '*' || p[1] != '.') return;
    for (const char *k = p; *k; k++) if (*k == ';') return;   /* varios patrones: no se adivina */
    char ext[16];
    int i = 0;
    for (p++; *p && *p != ';' && i < 14; p++, i++) {
        if (*p == '*' || *p == '?' || *p == '[') return;
        ext[i] = *p;
    }
    ext[i] = 0;
    if (i < 2) return;
    size_t n = tam_texto(c);
    if (n + (size_t)i < (size_t)max) copia_texto(c + n, ext, (size_t)(max - (int)n));
}

/* «¿Reemplazar?» al estilo Windows 95: «Sí» a la izquierda, «No» a la derecha y por defecto
 * (Intro = No; S o Y = Sí; Esc = No). */
static bool confirmar95(const char *texto)
{
    jan_t *j = jan_abrir(_("Guardar como"), 400, 130);
    if (!j) return false;
    jan_estilo(j, JAN_DIALOGO, 0, 0);
    int foco = 1;   /* 0 = Sí, 1 = No */
    bool res = false;
    for (;;) {
        g_retangulo(j, 0, 0, 400, 130, CINZA);
        int y = 16;
        for (const char *t = texto; *t;) {
            int n = 0;
            while (t[n] && t[n] != '\n') n++;
            g_texto_n(j, 16, y, t, n < 46 ? n : 46, 0);
            y += LETRA_A + 2;
            t += n + (t[n] ? 1 : 0);
        }
        botao95(j, 110, 88, 84, 26, _("Sí"), foco == 0);
        botao95(j, 206, 88, 84, 26, _("No"), foco == 1);
        jan_mostrar(j);
        evento_t e;
        do { } while (proximo_evento(j->id, &e, true) != 1);
        if (e.tipo == EV_FECHAR) break;
        if (e.tipo == EV_CLIQUE) {
            if (dentro(e.x, e.y, 110, 88, 84, 26)) { res = true; break; }
            if (dentro(e.x, e.y, 206, 88, 84, 26)) break;
        } else if (e.tipo == EV_TECLA) {
            unsigned t = e.tecla;
            if (t == TECLA_ESC) break;
            if (t == 's' || t == 'S' || t == 'y' || t == 'Y') { res = true; break; }
            if (t == 'n' || t == 'N') break;
            if (t == '\n' || t == ' ') { res = foco == 0; break; }
            if (t == '\t' || t == TECLA_ESQUERDA || t == TECLA_DIREITA) foco = !foco;
        }
    }
    jan_fechar(j);
    return res;
}

/* Intenta terminar con una ruta completa. 1 = hecho (d->saida), 0 = sigue el diálogo. */
static int aceitar_caminho(dlg_t *d, const char *c)
{
    unsigned tipo;
    if (existe(c, &tipo) && tipo == TIPO_PASTA) { caixa_definir(&d->nome, ""); entrar(d, c); return 0; }
    char pai[RUTA_MAX];
    copia_texto(pai, c, sizeof pai);
    int n = (int)tam_texto(pai);
    while (n > 1 && pai[n - 1] != '/') n--;
    if (n > 1) n--;
    pai[n] = 0;
    if (!d->guardar) {
        if (!existe(c, NULL)) { copia_texto(d->aviso, _("No se encuentra el archivo."), sizeof d->aviso); return 0; }
    } else {
        if (!eh_pasta(pai)) { copia_texto(d->aviso, _("La carpeta no existe."), sizeof d->aviso); return 0; }
        if (existe(c, NULL)) {
            char m[200];
            formatar_texto(m, sizeof m, _("%s ya existe.\n¿Quieres reemplazarlo?"), nome_base(c));
            bool si = confirmar95(m);
            if (!si) return 0;
        }
    }
    copia_texto(d->saida, c, sizeof d->saida);
    rec_anotar(pai);
    return 1;
}

static int aceitar_nome(dlg_t *d)
{
    char t[sizeof d->nome.texto];
    int a = 0, b = (int)tam_texto(d->nome.texto);
    while (d->nome.texto[a] == ' ') a++;
    while (b > a && d->nome.texto[b - 1] == ' ') b--;
    memcpy(t, d->nome.texto + a, (size_t)(b - a));
    t[b - a] = 0;
    d->aviso[0] = 0;
    if (!t[0]) {
        if (d->sel < 0 || d->sel >= d->nvis) return 0;
        const entrada_pasta_t *e = &d->ent[d->vis[d->sel]];
        char c[RUTA_MAX];
        normalizar(d->pasta, e->nome, c, sizeof c);
        return aceitar_caminho(d, c);
    }
    bool glob = false;
    for (char *k = t; *k; k++) if (*k == '*' || *k == '?') glob = true;
    if (glob && !d->guardar) {   /* un patrón: filtra la lista, como en Windows */
        copia_texto(d->padrao_extra, t, sizeof d->padrao_extra);
        caixa_definir(&d->nome, "");
        refiltrar(d);
        return 0;
    }
    char c[RUTA_MAX];
    normalizar(d->pasta, t, c, sizeof c);
    if (d->guardar && !existe(c, NULL)) poner_extensao(d, c, sizeof c);
    return aceitar_caminho(d, c);
}

static int ativar_lista(dlg_t *d)
{
    if (d->sel < 0 || d->sel >= d->nvis) return 0;
    const entrada_pasta_t *e = &d->ent[d->vis[d->sel]];
    if (e->tipo == TIPO_PASTA) { char c[RUTA_MAX]; normalizar(d->pasta, e->nome, c, sizeof c); caixa_definir(&d->nome, ""); entrar(d, c); return 0; }
    caixa_definir(&d->nome, e->nome);
    return aceitar_nome(d);
}

/* 1 = aceptado, 2 = cancelado, 0 = sigue. */
static int accionar(dlg_t *d, int f, int *fim)
{
    switch (f) {
    case F_COMBO: escolher_pasta_combo(d, fim); break;
    case F_TIPO: escolher_tipo(d, fim); break;
    case F_SUBIR: subir_nivel(d); break;
    case F_NOVA: nova_pasta(d); break;
    case F_VISTA: d->detalhes = !d->detalhes; break;
    case F_OK: return aceitar_nome(d) ? 1 : 0;
    case F_CANC: return 2;
    }
    return 0;
}

static bool dialogo_arquivo(int guardar, const char *titulo, const char *filtros, const char *inicial,
                            char *saida, int max)
{
    dlg_t *d = aloca_zerado(sizeof *d);
    if (!d) return false;
    filtros_ler(d, filtros);
    d->guardar = guardar;
    d->detalhes = 0;
    d->foco = F_NOME;
    char ini[RUTA_MAX], nome_ini[64] = "";
    copia_texto(ini, "/", sizeof ini);
    if (inicial && inicial[0]) {
        char c[RUTA_MAX];
        normalizar("/", inicial, c, sizeof c);
        unsigned ty;
        bool barra = inicial[tam_texto(inicial) - 1] == '/';
        if (existe(c, &ty) && ty == TIPO_PASTA) copia_texto(ini, c, sizeof ini);
        else if (!barra) {   /* carpeta + nombre propuesto */
            copia_texto(nome_ini, nome_base(c), sizeof nome_ini);
            int n = (int)tam_texto(c);
            while (n > 0 && c[n - 1] != '/') n--;
            if (n > 1) n--;
            c[n ? n : 1] = 0;
            if (n == 0) copia_texto(c, "/", sizeof c);
            if (eh_pasta(c)) copia_texto(ini, c, sizeof ini);
        }
    }
    copia_texto(d->pasta, "/", sizeof d->pasta);
    if (!entrar(d, ini)) entrar(d, "/");
    memset(&d->nome, 0, sizeof d->nome);
    caixa_definir(&d->nome, nome_ini);
    for (int i = 0; i < d->nvis; i++)
        if (nome_ini[0] && compara_texto(d->ent[d->vis[i]].nome, nome_ini) == 0) { d->sel = i; break; }
    d->aviso[0] = 0;
    d->j = jan_abrir(idioma_traducir_sl(titulo), DA_L, DA_A);   /* D313: el título lo da el programa en español */
    if (!d->j) { if (d->ent) libera(d->ent); if (d->vis) libera(d->vis); libera(d); return false; }
    jan_t *j = d->j;
    jan_estilo(j, JAN_DIALOGO, 0, 0);
    registrar("dialogo: %s en %s", guardar ? "guardar" : "abrir", d->pasta);
    desenhar_tudo(d);
    int fim = 0;
    evento_t e;
    while (!fim) {
        if (proximo_evento(j->id, &e, true) != 1) continue;
        bool rolou = false;
        if (e.tipo == EV_FECHAR) { fim = 2; break; }
        if (e.tipo == EV_RODA) {
            if (!dentro(e.x, e.y, LI_X, LI_Y, LI_L, LI_A)) continue;
            d->topo += (int)e.cliques * RODA_LINHAS;
            rolou = true;
        } else if (e.tipo == EV_CLIQUE) {
            int y0 = LI_Y + 2 + (d->detalhes ? FILA : 0);
            if (dentro(e.x, e.y, X_COMBO, Y_COMBO, L_COMBO, 24)) { d->foco = F_COMBO; accionar(d, F_COMBO, &fim); }
            else if (dentro(e.x, e.y, X_NOME, Y_TIPO, L_NOME, 24)) { d->foco = F_TIPO; accionar(d, F_TIPO, &fim); }
            else if (dentro(e.x, e.y, X_SUBIR, Y_COMBO, 48, 24)) { d->foco = F_SUBIR; accionar(d, F_SUBIR, &fim); }
            else if (dentro(e.x, e.y, X_NOVA, Y_COMBO, 64, 24)) { d->foco = F_NOVA; accionar(d, F_NOVA, &fim); }
            else if (dentro(e.x, e.y, X_VISTA, Y_COMBO, 78, 24)) { d->foco = F_VISTA; accionar(d, F_VISTA, &fim); }
            else if (dentro(e.x, e.y, X_BOT, Y_NOME, 88, 24)) { d->foco = F_OK; fim = accionar(d, F_OK, &fim); }
            else if (dentro(e.x, e.y, X_BOT, Y_TIPO, 88, 24)) { fim = 2; }
            else if (dentro(e.x, e.y, X_NOME, Y_NOME, L_NOME, 24)) {
                d->foco = F_NOME;
                d->nome.foco = true;
                caixa_tratar(j, &d->nome, &e);
            } else if (dentro(e.x, e.y, LI_X, y0, LI_L, LI_A)) {
                d->foco = F_LISTA;
                int i = d->topo + (e.y - y0) / FILA;
                if (e.y >= y0 && i < d->nvis) {
                    d->sel = i;
                    copiar_nome_sel(d);
                    if (e.cliques >= 2) fim = ativar_lista(d);
                }
            }
        } else if (e.tipo == EV_TECLA) {
            unsigned t = e.tecla;
            if (t == TECLA_ESC) { fim = 2; break; }
            if (t == '\t') {
                d->foco = (d->foco + ((e.modificadores & 1) ? F_N - 1 : 1)) % F_N;
            } else if ((e.modificadores & 4) && t == TECLA_CIMA) {
                subir_nivel(d);
            } else if (d->foco == F_NOME) {
                if (t == TECLA_CIMA || t == TECLA_BAIXO || t == TECLA_PGUP || t == TECLA_PGDN) mover_sel(d, t);
                else if (t == '\n') fim = aceitar_nome(d);
                else caixa_tratar(j, &d->nome, &e);
            } else if (d->foco == F_LISTA) {
                if (t == '\n') fim = ativar_lista(d);
                else if (t == '\b') subir_nivel(d);
                else if (t == TECLA_CIMA || t == TECLA_BAIXO || t == TECLA_PGUP || t == TECLA_PGDN
                         || t == TECLA_HOME || t == TECLA_END) mover_sel(d, t);
                else if (t >= 0x20 && t < 0x100) saltar_nome(d, (char)t);
            } else if (t == '\n' || t == ' ' || (t == TECLA_BAIXO && (d->foco == F_COMBO || d->foco == F_TIPO))) {
                int r = accionar(d, d->foco, &fim);
                if (r) fim = r;
            }
        } else continue;
        if (fim) break;
        if (!rolou) ver_sel(d); else limitar_topo(d);
        desenhar_tudo(d);
    }
    jan_fechar(j);
    bool ok = fim == 1;
    if (ok) copia_texto(saida, d->saida, (size_t)max);
    if (d->ent) libera(d->ent);
    if (d->vis) libera(d->vis);
    libera(d);
    return ok;
}

bool dialogo_abrir(const char *titulo, const char *filtros, const char *inicial, char *saida, int max)
{
    return dialogo_arquivo(0, titulo, filtros, inicial, saida, max);
}

bool dialogo_guardar(const char *titulo, const char *filtros, const char *inicial, char *saida, int max)
{
    return dialogo_arquivo(1, titulo, filtros, inicial, saida, max);
}

/* ------------------------------------------------------------ elegir carpeta (árbol) */

#define AR_L 400
#define AR_A 350
#define AR_X 10
#define AR_Y 30
#define AR_W 380
#define AR_H 236
#define AR_MAX 300

typedef struct {
    char caminho[RUTA_MAX];
    char nome[64];
    unsigned char prof, aberto, semperm;
} no_t;

typedef struct {
    jan_t *j;
    no_t *n; int qtd;
    int sel, topo, foco;
    char aviso[80];
} arv_t;

enum { A_ARV, A_NOVA, A_OK, A_CANC, A_N };

static int arv_filas(void) { return (AR_H - 4) / FILA; }

/* Abre el nodo i: inserta debajo las subcarpetas (sin las ocultas). */
static void arv_expandir(arv_t *a, int i)
{
    no_t *no = &a->n[i];
    if (no->aberto) return;
    entrada_pasta_t *v = NULL;
    int n = listar_tudo(no->caminho, &v);
    if (n < 0) { no->semperm = 1; copia_texto(a->aviso, n == ERRO_ACESSO ? _("Sin permiso para entrar en esa carpeta.") : _("No se puede abrir esa carpeta."), sizeof a->aviso); return; }
    no->aberto = 1;
    a->aviso[0] = 0;
    int k = 0;
    for (int e = 0; e < n; e++) {
        if (v[e].tipo != TIPO_PASTA || v[e].nome[0] == '.') continue;
        if (a->qtd + 1 >= AR_MAX) break;
        int pos = i + 1 + k++;
        memmove(&a->n[pos + 1], &a->n[pos], (size_t)(a->qtd - pos) * sizeof(no_t));
        a->qtd++;
        no_t *f = &a->n[pos];
        memset(f, 0, sizeof *f);
        copia_texto(f->nome, v[e].nome, sizeof f->nome);
        caminho_juntar(f->caminho, sizeof f->caminho, a->n[i].caminho, v[e].nome);
        f->prof = (unsigned char)(a->n[i].prof + 1);
    }
    if (v) libera(v);
}

static void arv_recolher(arv_t *a, int i)
{
    if (!a->n[i].aberto) return;
    int k = i + 1;
    while (k < a->qtd && a->n[k].prof > a->n[i].prof) k++;
    memmove(&a->n[i + 1], &a->n[k], (size_t)(a->qtd - k) * sizeof(no_t));
    a->qtd -= k - (i + 1);
    a->n[i].aberto = 0;
}

static void arv_ver(arv_t *a)
{
    int v = arv_filas();
    if (a->sel < a->topo) a->topo = a->sel;
    if (a->sel >= a->topo + v) a->topo = a->sel - v + 1;
    int max = a->qtd - v;
    if (a->topo > max) a->topo = max;
    if (a->topo < 0) a->topo = 0;
}

static void arv_desenhar(arv_t *a)
{
    jan_t *j = a->j;
    g_retangulo(j, 0, 0, AR_L, AR_A, CINZA);
    g_texto(j, 10, 8, _("Elige una carpeta:"), 0);
    g_retangulo(j, AR_X, AR_Y, AR_W, AR_H, COR_CAMPO);
    bisel(j, AR_X, AR_Y, AR_W, AR_H, true);
    int v = arv_filas();
    for (int k = 0; k < v && a->topo + k < a->qtd; k++) {
        int i = a->topo + k, y = AR_Y + 2 + k * FILA;
        const no_t *no = &a->n[i];
        int x = AR_X + 6 + no->prof * 16;
        /* cuadro [+] o [-] de Windows 95 */
        g_moldura(j, x, y + 4, 9, 9, W98_SOMBRA);
        g_retangulo(j, x + 2, y + 8, 5, 1, 0);
        if (!no->aberto && !no->semperm) g_retangulo(j, x + 4, y + 6, 1, 5, 0);
        icone_desenhar(j, x + 14, y + 1, "pasta", 1);
        int w = (int)tam_texto(no->nome) * LETRA_L + 4;
        if (i == a->sel) g_retangulo(j, x + 32, y, w, FILA, a->foco == A_ARV ? AZUL95 : W98_SOMBRA);
        g_texto(j, x + 34, y + 1, no->nome, i == a->sel ? COR_SEL_TEXTO : (no->semperm ? W98_SOMBRA : COR_TEXTO));
        if (i == a->sel && a->foco == A_ARV) pontilhado(j, x + 32, y, w, FILA);
    }
    if (a->qtd > v) {
        int tr = AR_H - 6, ax = AR_X + AR_W - 14;
        g_retangulo(j, ax, AR_Y + 2, 12, tr, W98_CLARO);
        int pa = tr * v / a->qtd;
        if (pa < 10) pa = 10;
        int py = (tr - pa) * a->topo / (a->qtd - v);
        g_retangulo(j, ax, AR_Y + 2 + py, 12, pa, CINZA);
        bisel(j, ax, AR_Y + 2 + py, 12, pa, false);
    }
    g_texto(j, 10, AR_Y + AR_H + 8, _("Carpeta:"), 0);
    bisel(j, 90, AR_Y + AR_H + 4, AR_L - 100, 24, true);
    g_retangulo(j, 92, AR_Y + AR_H + 6, AR_L - 104, 20, COR_CAMPO);
    g_texto_n(j, 96, AR_Y + AR_H + 8, a->n[a->sel].caminho, (AR_L - 112) / LETRA_L, COR_TEXTO);
    int yb = AR_Y + AR_H + 38;
    botao95(j, 10, yb, 120, 24, _("Nueva carpeta"), a->foco == A_NOVA);
    botao95(j, 206, yb, 88, 24, _("Aceptar"), a->foco == A_OK);
    botao95(j, 302, yb, 88, 24, _("Cancelar"), a->foco == A_CANC);
    if (a->aviso[0]) g_texto_n(j, 10, AR_A - 24, a->aviso, (AR_L - 20) / LETRA_L, 0x800000);
    jan_mostrar(j);
}

static void arv_nova(arv_t *a)
{
    char nome[64] = "";
    if (!pedir_texto(_("Nueva carpeta"), _("Nombre de la carpeta:"), nome, sizeof nome) || !nome[0]) return;
    char c[RUTA_MAX];
    caminho_juntar(c, sizeof c, a->n[a->sel].caminho, nome);
    if (criar_pasta(c) < 0) { copia_texto(a->aviso, _("No se pudo crear la carpeta."), sizeof a->aviso); return; }
    arv_recolher(a, a->sel);
    arv_expandir(a, a->sel);
    for (int i = a->sel + 1; i < a->qtd && a->n[i].prof > a->n[a->sel].prof; i++)
        if (compara_texto(a->n[i].nome, nome) == 0) { a->sel = i; break; }
}

bool dialogo_carpeta(const char *titulo, const char *inicial, char *saida, int max)
{
    arv_t *a = aloca_zerado(sizeof *a);
    if (!a) return false;
    a->n = aloca_zerado(AR_MAX * sizeof(no_t));
    if (!a->n) { libera(a); return false; }
    copia_texto(a->n[0].caminho, "/", sizeof a->n[0].caminho);
    copia_texto(a->n[0].nome, "/", sizeof a->n[0].nome);
    a->qtd = 1;
    arv_expandir(a, 0);
    /* despliega el camino hasta la carpeta inicial y la deja seleccionada */
    char ini[RUTA_MAX];
    normalizar("/", inicial && inicial[0] ? inicial : "/", ini, sizeof ini);
    const char *s = ini;
    while (*s) {
        while (*s == '/') s++;
        const char *f = s;
        while (*s && *s != '/') s++;
        int l = (int)(s - f);
        if (!l) break;
        int achou = -1;
        for (int i = a->sel + 1; i < a->qtd && a->n[i].prof > a->n[a->sel].prof; i++)
            if ((int)tam_texto(a->n[i].nome) == l && compara_texto_n(a->n[i].nome, f, (size_t)l) == 0) { achou = i; break; }
        if (achou < 0) break;
        a->sel = achou;
        arv_expandir(a, achou);
    }
    a->aviso[0] = 0;
    a->j = jan_abrir(idioma_traducir_sl(titulo), AR_L, AR_A);
    if (!a->j) { libera(a->n); libera(a); return false; }
    jan_t *j = a->j;
    jan_estilo(j, JAN_DIALOGO, 0, 0);
    registrar("dialogo: carpeta en %s", a->n[a->sel].caminho);
    arv_ver(a);
    arv_desenhar(a);
    int fim = 0;
    evento_t e;
    while (!fim) {
        if (proximo_evento(j->id, &e, true) != 1) continue;
        if (e.tipo == EV_FECHAR) { fim = 2; break; }
        if (e.tipo == EV_RODA) {
            if (!dentro(e.x, e.y, AR_X, AR_Y, AR_W, AR_H)) continue;
            a->topo += (int)e.cliques * RODA_LINHAS;
            int mx = a->qtd - arv_filas();
            if (a->topo > mx) a->topo = mx;
            if (a->topo < 0) a->topo = 0;
            arv_desenhar(a);
            continue;
        }
        int yb = AR_Y + AR_H + 38, boton = -1;
        if (e.tipo == EV_CLIQUE) {
            if (dentro(e.x, e.y, 10, yb, 120, 24)) boton = A_NOVA;
            else if (dentro(e.x, e.y, 206, yb, 88, 24)) boton = A_OK;
            else if (dentro(e.x, e.y, 302, yb, 88, 24)) boton = A_CANC;
            else if (dentro(e.x, e.y, AR_X, AR_Y, AR_W, AR_H)) {
                a->foco = A_ARV;
                int i = a->topo + (e.y - AR_Y - 2) / FILA;
                if (e.y >= AR_Y + 2 && i < a->qtd) {
                    a->sel = i;
                    int bx = AR_X + 6 + a->n[i].prof * 16;
                    if ((e.x >= bx && e.x < bx + 10) || e.cliques >= 2) {
                        if (a->n[i].aberto) arv_recolher(a, i); else arv_expandir(a, i);
                    }
                }
            }
        } else if (e.tipo == EV_TECLA) {
            unsigned t = e.tecla;
            if (t == TECLA_ESC) { fim = 2; break; }
            if (t == '\t') a->foco = (a->foco + ((e.modificadores & 1) ? A_N - 1 : 1)) % A_N;
            else if (a->foco == A_ARV) {
                if (t == TECLA_BAIXO && a->sel < a->qtd - 1) a->sel++;
                else if (t == TECLA_CIMA && a->sel > 0) a->sel--;
                else if (t == TECLA_HOME) a->sel = 0;
                else if (t == TECLA_END) a->sel = a->qtd - 1;
                else if (t == TECLA_PGDN) { a->sel += arv_filas(); if (a->sel > a->qtd - 1) a->sel = a->qtd - 1; }
                else if (t == TECLA_PGUP) { a->sel -= arv_filas(); if (a->sel < 0) a->sel = 0; }
                else if (t == TECLA_DIREITA || t == '+') {
                    if (!a->n[a->sel].aberto) arv_expandir(a, a->sel);
                    else if (a->sel + 1 < a->qtd && a->n[a->sel + 1].prof > a->n[a->sel].prof) a->sel++;
                } else if (t == TECLA_ESQUERDA || t == '-') {
                    if (a->n[a->sel].aberto) arv_recolher(a, a->sel);
                    else {
                        int k = a->sel;
                        while (k > 0 && a->n[k].prof >= a->n[a->sel].prof) k--;
                        a->sel = k;
                    }
                } else if (t == '\n') boton = A_OK;
            } else if (t == '\n' || t == ' ') boton = a->foco;
        } else continue;
        if (boton == A_OK) { fim = 1; break; }
        if (boton == A_CANC) { fim = 2; break; }
        if (boton == A_NOVA) arv_nova(a);
        arv_ver(a);
        arv_desenhar(a);
    }
    bool ok = fim == 1;
    if (ok) {
        copia_texto(saida, a->n[a->sel].caminho, (size_t)max);
        rec_anotar(a->n[a->sel].caminho);
    }
    jan_fechar(j);
    libera(a->n);
    libera(a);
    return ok;
}
