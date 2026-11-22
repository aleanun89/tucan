// SPDX-License-Identifier: GPL-2.0-or-later
/* gui_componentes.c - nuevos componentes del kit gráfico de libsagui (mejora 042).
 *
 * Van aquí el lienzo (superficie con elementos retenidos), la barra de
 * progreso y la casilla de verificación, que son los que más faltaban: cada
 * programa grande (Paint, el visor, Mico, el Administrador de tareas) se había
 * hecho sus propios controles. El patrón es el de siempre: estructura +
 * x_desenhar + x_tratar.
 *
 * Todo corre en el propio programa (anillo 3): el kernel sólo entrega el área
 * de píxeles de la ventana y los eventos.
 *
 * Sin coma flotante: libsagui se compila con -mgeneral-regs-only (D076), así
 * que el zoom va en punto fijo 16.16 y los dibujos se escalan con enteros.
 */
#include "gui.h"
#include "imagem.h"

extern const unsigned char fonte_sagui[256][16];   /* la letra de 8x16 del sistema */

/* ---- utilidades ---- */

/* Raíz cuadrada entera (método de Newton con estimaciones iniciales). */
static int raiz_int(int n)
{
    if (n <= 0) return 0;
    int x = n, y = (x + 1) / 2;
    while (y < x) { x = y; y = (x + n / x) / 2; }
    return x;
}

/* ---- lienzo ---- */

/* Un lienzo guarda elementos (líneas, rectángulos, elipses, polígonos, texto
 * e imágenes) en coordenadas del propio lienzo, que se ven a través de una
 * ventana con desplazamiento y zoom. Los elementos se guardan en un vector que
 * crece; el orden es el de dibujo (el último encima). */
#define LENZO_MAX 512
#define LENZO_PONTOS_MAX 64

typedef enum {
    LENZO_LINHA,
    LENZO_RETANGULO,
    LENZO_ELIPSE,
    LENZO_POLIGONO,
    LENZO_TEXTO,
    LENZO_IMAGEM
} lenzo_tipo_t;

typedef struct {
    lenzo_tipo_t tipo;
    uint32_t    cor;         /* relleno */
    uint32_t    cor_linha;   /* contorno (0 = sin contorno) */
    int         espessura;
    /* Rectángulo del elemento, en coordenadas del lienzo. */
    int         x, y, l, a;
    /* Texto. */
    const char *texto;
    /* Polígono: puntos (x, y) en coordenadas del lienzo. */
    int         pontos[LENZO_PONTOS_MAX * 2];
    int         n_pontos;
    /* Imagen. */
    imagem_t   *img;
    int         img_x, img_y;   /* esquina de la imagen dentro del lienzo */
    int         img_l, img_a;   /* tamaño dibujado (0 = el natural) */
    bool        preenchido;
    bool        suave;
} lenzo_elem_t;

struct lenzo {
    int  x, y, l, a;             /* posición y tamaño en la ventana */
    int  desl_x, desl_y;         /* desplazamiento (en píxeles del lienzo) */
    int  zoom;                  /* por 100: 100 = normal (sin coma flotante, D076) */
    uint32_t fundo;
    lenzo_elem_t *elem;          /* elementos retenidos */
    int  n, cap;
    int  mouse_x, mouse_y;       /* última posición del ratón, en el lienzo */
    bool tem_mouse;
    int  sob_mouse;              /* elemento bajo el ratón (-1 = ninguno) */
    int  selecionado;            /* elemento elegido (-1 = ninguno) */
};

#define ZOOM_NORMAL 100

lenzo_t *lenzo_criar(int x, int y, int l, int a)
{
    lenzo_t *c = aloca_zerado(sizeof *c);
    if (!c) return NULL;
    c->x = x; c->y = y; c->l = l; c->a = a;
    c->zoom = ZOOM_NORMAL;
    c->fundo = COR_FUNDO;
    c->sob_mouse = -1;
    c->selecionado = -1;
    return c;
}

void lenzo_liberar(lenzo_t *c)
{
    if (!c) return;
    for (int i = 0; i < c->n; i++)
        if (c->elem[i].img) { imagem_liberar(c->elem[i].img); libera(c->elem[i].img); }
    if (c->elem) libera(c->elem);
    libera(c);
}

/* Amplía el vector de elementos; false si no hay memoria. */
static bool lenzo_crescer(lenzo_t *c)
{
    if (c->n < c->cap) return true;
    int nova = c->cap ? c->cap * 2 : 32;
    lenzo_elem_t *v = aloca_zerado((size_t)nova * sizeof *v);
    if (!v) return false;
    if (c->elem) {
        memcpy(v, c->elem, (size_t)c->n * sizeof *v);
        libera(c->elem);
    }
    c->elem = v;
    c->cap = nova;
    return true;
}

/* Índice del elemento nuevo, o -1 si no se pudo. */
static int lenzo_novo(lenzo_t *c, lenzo_tipo_t tipo)
{
    if (!lenzo_crescer(c)) return -1;
    int i = c->n++;
    memset(&c->elem[i], 0, sizeof c->elem[i]);
    c->elem[i].tipo = tipo;
    c->elem[i].espessura = 1;
    return i;
}

/* La línea se guarda en x, y (origen) y l, a (destino), como coordenadas
 * absolutas del lienzo. */
int lenzo_linha(lenzo_t *c, int x0, int y0, int x1, int y1, uint32_t cor, int espessura)
{
    int i = lenzo_novo(c, LENZO_LINHA);
    if (i < 0) return -1;
    c->elem[i].x = x0; c->elem[i].y = y0;
    c->elem[i].l = x1; c->elem[i].a = y1;
    c->elem[i].cor = cor;
    c->elem[i].espessura = espessura > 0 ? espessura : 1;
    return i;
}

int lenzo_retangulo(lenzo_t *c, int x, int y, int l, int a, uint32_t cor,
                    uint32_t cor_linha, bool preenchido, int espessura)
{
    int i = lenzo_novo(c, LENZO_RETANGULO);
    if (i < 0) return -1;
    c->elem[i].x = x; c->elem[i].y = y; c->elem[i].l = l; c->elem[i].a = a;
    c->elem[i].cor = cor;
    c->elem[i].cor_linha = cor_linha;
    c->elem[i].preenchido = preenchido;
    c->elem[i].espessura = espessura > 0 ? espessura : 1;
    return i;
}

int lenzo_elipse(lenzo_t *c, int x, int y, int l, int a, uint32_t cor,
                 uint32_t cor_linha, bool preenchido, int espessura)
{
    int i = lenzo_novo(c, LENZO_ELIPSE);
    if (i < 0) return -1;
    c->elem[i].x = x; c->elem[i].y = y; c->elem[i].l = l; c->elem[i].a = a;
    c->elem[i].cor = cor;
    c->elem[i].cor_linha = cor_linha;
    c->elem[i].preenchido = preenchido;
    c->elem[i].espessura = espessura > 0 ? espessura : 1;
    return i;
}

int lenzo_poligono(lenzo_t *c, const int *pontos, int n, uint32_t cor,
                   uint32_t cor_linha, bool preenchido, int espessura)
{
    if (n < 2 || n > LENZO_PONTOS_MAX) return -1;
    int i = lenzo_novo(c, LENZO_POLIGONO);
    if (i < 0) return -1;
    memcpy(c->elem[i].pontos, pontos, (size_t)n * 2 * sizeof *c->elem[i].pontos);
    c->elem[i].n_pontos = n;
    c->elem[i].cor = cor;
    c->elem[i].cor_linha = cor_linha;
    c->elem[i].preenchido = preenchido;
    c->elem[i].espessura = espessura > 0 ? espessura : 1;
    return i;
}

int lenzo_texto(lenzo_t *c, int x, int y, const char *texto, uint32_t cor)
{
    int i = lenzo_novo(c, LENZO_TEXTO);
    if (i < 0) return -1;
    c->elem[i].x = x; c->elem[i].y = y;
    c->elem[i].texto = texto;
    c->elem[i].cor = cor;
    return i;
}

/* Añade una imagen al lienzo. El lienzo guarda una copia, así que el
 * llamante puede liberar la suya. */
int lenzo_imagem(lenzo_t *c, int x, int y, const imagem_t *img, int l, int a)
{
    if (!img) return -1;
    int i = lenzo_novo(c, LENZO_IMAGEM);
    if (i < 0) return -1;
    imagem_t *copia = aloca_zerado(sizeof *copia);
    if (!copia) { c->n--; return -1; }
    if (!imagem_copiar(copia, img)) { libera(copia); c->n--; return -1; }
    c->elem[i].img = copia;
    c->elem[i].img_x = x; c->elem[i].img_y = y;
    c->elem[i].img_l = l; c->elem[i].img_a = a;
    return i;
}

void lenzo_limpar(lenzo_t *c)
{
    if (!c) return;
    for (int i = 0; i < c->n; i++)
        if (c->elem[i].img) { imagem_liberar(c->elem[i].img); libera(c->elem[i].img); c->elem[i].img = NULL; }
    c->n = 0;
    c->sob_mouse = -1;
    c->selecionado = -1;
}

/* Escala un valor del lienzo a píxeles de pantalla (zoom en tanto por uno). */
static int lenzo_escala(lenzo_t *c, int v)
{
    return (v * c->zoom) / 100;
}

/* Escala un valor del lienzo a coordenadas del lienzo (para el zoom
 * inverso, al detectar el elemento bajo el ratón). */
static int lenzo_desescala(lenzo_t *c, int v)
{
    if (c->zoom <= 0) return v;
    return (v * 100) / c->zoom;
}

/* ¿Cae (lx, ly) dentro del elemento? */
static bool lenzo_contem(const lenzo_elem_t *el, int lx, int ly)
{
    switch (el->tipo) {
    case LENZO_LINHA:
        return lx >= el->x && lx <= el->l && ly >= el->y && ly <= el->a;
    case LENZO_RETANGULO:
    case LENZO_IMAGEM:
        return lx >= el->x && lx < el->x + el->l && ly >= el->y && ly < el->y + el->a;
    case LENZO_ELIPSE: {
        /* Elipse inscrita en el rectángulo: (dx/rx)^2 + (dy/ry)^2 <= 1.
         * En enteros de 32 bits: se compara dx^2*ry^2 + dy^2*rx^2 con
         * rx^2*ry^2 (todo cabe en 32 bits para los tamaños usuales). */
        int rx = el->l / 2, ry = el->a / 2;
        if (rx <= 0 || ry <= 0) return false;
        int dx = lx - (el->x + rx), dy = ly - (el->y + ry);
        long long esq = (long long)dx * dx * ry * ry + (long long)dy * dy * rx * rx;
        return esq <= (long long)rx * rx * ry * ry;
    }
    case LENZO_POLIGONO: {
        /* Diferencia de winding number (rayo horizontal). */
        bool dentro = false;
        for (int i = 0, j = el->n_pontos - 1; i < el->n_pontos; j = i++) {
            int yi = el->pontos[i * 2 + 1], yj = el->pontos[j * 2 + 1];
            int xi = el->pontos[i * 2],     xj = el->pontos[j * 2];
            if ((yi > ly) != (yj > ly)) {
                int x = xi + (ly - yi) * (xj - xi) / (yj - yi);
                if (lx < x) dentro = !dentro;
            }
        }
        return dentro;
    }
    case LENZO_TEXTO:
        return lx >= el->x && ly >= el->y &&
               ly < el->y + LETRA_A && lx < el->x + g_largura_texto(el->texto);
    }
    return false;
}

/* Índice del elemento de más arriba bajo (lx, ly), o -1. */
int lenzo_em_baixo_de(lenzo_t *c, int lx, int ly)
{
    for (int i = c->n - 1; i >= 0; i--)
        if (lenzo_contem(&c->elem[i], lx, ly)) return i;
    return -1;
}

/* Texto escalado: se dibujan los glifos de la fuente 8x16 ampliados. Con
 * escala 1 sale igual que g_texto. */
static void texto_escalado(jan_t *j, int x, int y, const char *t, uint32_t cor, int esc)
{
    if (esc <= 1) { g_texto(j, x, y, t, cor); return; }
    for (int k = 0; t[k] && x < j->larg; k++, x += LETRA_L * esc) {
        const unsigned char *g = fonte_sagui[(uint8_t)t[k]];
        for (int lin = 0; lin < 16; lin++)
            for (int col = 0; col < 8; col++)
                if (g[lin] & (0x80 >> col))
                    g_retangulo(j, x + col * esc, y + lin * esc, esc, esc, cor);
    }
}

/* Relleno de un polígono por líneas horizontales (regla del par-impar). */
static void poligono_preencher(jan_t *j, lenzo_t *c, const lenzo_elem_t *el, int ox, int oy)
{
    int xs[LENZO_PONTOS_MAX], nxs = 0;
    int ymin = el->pontos[1], ymax = el->pontos[1];
    for (int i = 1; i < el->n_pontos; i++) {
        int v = el->pontos[i * 2 + 1];
        if (v < ymin) ymin = v;
        if (v > ymax) ymax = v;
    }
    for (int fy = ymin; fy <= ymax; fy++) {
        nxs = 0;
        for (int i = 0, j = el->n_pontos - 1; i < el->n_pontos; j = i++) {
            int xi = el->pontos[i * 2], yi = el->pontos[i * 2 + 1];
            int xj = el->pontos[j * 2], yj = el->pontos[j * 2 + 1];
            if ((yi > fy) != (yj > fy))
                if (nxs < LENZO_PONTOS_MAX)
                    xs[nxs++] = xi + (fy - yi) * (xj - xi) / (yj - yi);
        }
        for (int k = 0; k + 1 < nxs; k += 2) {
            int x0 = ox + lenzo_escala(c, xs[k]);
            int x1 = ox + lenzo_escala(c, xs[k + 1]);
            if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
            g_retangulo(j, x0, oy + lenzo_escala(c, fy), x1 - x0, 1, el->cor);
        }
    }
}

/* Elipse: para cada fila, el ancho viene de la ecuación (dx/rx)^2 +
 * (dy/ry)^2 = 1. Se dibujan las filas y, si hay contorno, las dos columnas
 * de los extremos. */
static void elipse_desenhar(jan_t *j, lenzo_t *c, const lenzo_elem_t *el, int ox, int oy)
{
    int rx = el->l / 2, ry = el->a / 2;
    if (rx <= 0 || ry <= 0) return;
    for (int f = 0; f < el->a; f++) {
        int dy = f - ry;
        /* dx = rx * sqrt(1 - (dy/ry)^2) = rx * sqrt(ry^2 - dy^2) / ry.
         * Todo en 32 bits: la división de 64 bits pediría libgcc (__divdi3),
         * que los programas de SaguiOS no llevan. */
        int t = ry * ry - dy * dy;
        if (t <= 0) continue;
        int dx = (rx * raiz_int(t)) / ry;
        if (dx <= 0) continue;
        int x0 = ox + lenzo_escala(c, el->x + rx - dx);
        int larg = lenzo_escala(c, dx * 2);
        int y = oy + lenzo_escala(c, el->y + f);
        if (el->preenchido) g_retangulo(j, x0, y, larg, 1, el->cor);
        if (el->cor_linha) {
            g_retangulo(j, x0, y, 1, 1, el->cor_linha);
            g_retangulo(j, x0 + larg - 1, y, 1, 1, el->cor_linha);
        }
    }
}

void lenzo_desenhar(jan_t *j, lenzo_t *c)
{
    if (!c) return;
    g_retangulo(j, c->x, c->y, c->l, c->a, c->fundo);

    for (int i = 0; i < c->n; i++) {
        lenzo_elem_t *el = &c->elem[i];
        int ox = c->x - c->desl_x, oy = c->y - c->desl_y;   /* origen en la ventana */
        switch (el->tipo) {
        case LENZO_LINHA: {
            int x0 = ox + lenzo_escala(c, el->x), y0 = oy + lenzo_escala(c, el->y);
            int x1 = ox + lenzo_escala(c, el->l), y1 = oy + lenzo_escala(c, el->a);
            g_linha(j, x0, y0, x1, y1, el->cor);
            if (el->espessura > 1)
                g_linha(j, x0, y0 + 1, x1, y1 + 1, el->cor);
            break;
        }
        case LENZO_RETANGULO: {
            int x = ox + lenzo_escala(c, el->x), y = oy + lenzo_escala(c, el->y);
            int l = lenzo_escala(c, el->l), a = lenzo_escala(c, el->a);
            if (el->preenchido) g_retangulo(j, x, y, l, a, el->cor);
            if (el->cor_linha) {
                g_moldura(j, x, y, l, a, el->cor_linha);
                if (el->espessura > 1) g_moldura(j, x + 1, y + 1, l - 2, a - 2, el->cor_linha);
            }
            break;
        }
        case LENZO_ELIPSE:
            elipse_desenhar(j, c, el, ox, oy);
            break;
        case LENZO_POLIGONO: {
            if (el->preenchido) poligono_preencher(j, c, el, ox, oy);
            if (el->cor_linha) {
                for (int k = 0; k < el->n_pontos; k++) {
                    int m = (k + 1) % el->n_pontos;
                    g_linha(j, ox + lenzo_escala(c, el->pontos[k * 2]),
                            oy + lenzo_escala(c, el->pontos[k * 2 + 1]),
                            ox + lenzo_escala(c, el->pontos[m * 2]),
                            oy + lenzo_escala(c, el->pontos[m * 2 + 1]),
                            el->cor_linha);
                }
            }
            break;
        }
        case LENZO_TEXTO: {
            int x = ox + lenzo_escala(c, el->x), y = oy + lenzo_escala(c, el->y);
            int esc = lenzo_escala(c, 1);
            if (esc < 1) esc = 1;
            texto_escalado(j, x, y, el->texto, el->cor, esc);
            break;
        }
        case LENZO_IMAGEM: {
            int x = ox + lenzo_escala(c, el->img_x), y = oy + lenzo_escala(c, el->img_y);
            int l = el->img_l ? lenzo_escala(c, el->img_l) : el->img->larg;
            int a = el->img_a ? lenzo_escala(c, el->img_a) : el->img->alt;
            if (l <= 0 || a <= 0) break;
            if (l != el->img->larg || a != el->img->alt) {
                imagem_t tmp;
                if (imagem_escalar(&tmp, el->img, l, a, el->suave)) {
                    imagem_desenhar(j->px, j->larg, x, y, l, a, &tmp, 0, 0, l, a, true);
                    imagem_liberar(&tmp);
                }
            } else {
                imagem_desenhar(j->px, j->larg, x, y, l, a, el->img, 0, 0, l, a, true);
            }
            break;
        }
        }
    }
    g_moldura(j, c->x, c->y, c->l, c->a, COR_BORDA);
    GR_REG(j, "lenzo", c, c->x, c->y, c->l, c->a, "", 0, 0);
}

/* Trata un evento del lienzo. Devuelve true si el lienzo cambió (hay que
 * redibujarlo). */
bool lenzo_tratar(jan_t *j, lenzo_t *c, const evento_t *e)
{
    if (!c || !e) return false;
    bool dentro = e->x >= c->x && e->x < c->x + c->l && e->y >= c->y && e->y < c->y + c->a;

    if (e->tipo == EV_MOVER) {
        if (!dentro) return false;
        int lx = lenzo_desescala(c, e->x - c->x + c->desl_x);
        int ly = lenzo_desescala(c, e->y - c->y + c->desl_y);
        c->mouse_x = lx; c->mouse_y = ly;
        c->tem_mouse = true;
        int sob = lenzo_em_baixo_de(c, lx, ly);
        if (sob != c->sob_mouse) { c->sob_mouse = sob; return true; }
        return false;
    }
    if (e->tipo == EV_CLIQUE) {
        if (!dentro) return false;
        int lx = lenzo_desescala(c, e->x - c->x + c->desl_x);
        int ly = lenzo_desescala(c, e->y - c->y + c->desl_y);
        c->selecionado = lenzo_em_baixo_de(c, lx, ly);
        return true;
    }
    if (e->tipo == EV_RODA) {
        if (!dentro) return false;
        int entalhes = (int)e->cliques;   /* con signo: + para abajo */
        if (entalhes == 0) entalhes = 1;
        if (e->modificadores & 2) {       /* Ctrl: zoom */
            int passo = c->zoom / 10;
            if (passo < 1) passo = 1;
            c->zoom += entalhes > 0 ? passo : -passo;
            if (c->zoom < 25) c->zoom = 25;
            if (c->zoom > 400) c->zoom = 400;
        } else {
            c->desl_y += entalhes * 24;
            if (c->desl_y < 0) c->desl_y = 0;
        }
        return true;
    }
    (void)j;
    return false;
}

void lenzo_deslocar(lenzo_t *c, int dx, int dy)
{
    if (!c) return;
    c->desl_x += dx;
    c->desl_y += dy;
    if (c->desl_x < 0) c->desl_x = 0;
    if (c->desl_y < 0) c->desl_y = 0;
}

void lenzo_zoom(lenzo_t *c, int por_100)
{
    if (!c) return;
    if (por_100 < 25) por_100 = 25;
    if (por_100 > 400) por_100 = 400;
    c->zoom = por_100;
}

int lenzo_elementos(lenzo_t *c) { return c ? c->n : 0; }
int lenzo_sobre_mouse(lenzo_t *c) { return c ? c->sob_mouse : -1; }
int lenzo_selecionado(lenzo_t *c) { return c ? c->selecionado : -1; }

/* ---- barra de progreso ---- */

/* Barra de progreso: determinada (0-100 %) o indeterminada (una banda que
 * se mueve). Horizontal o vertical. */
struct progresso {
    int  x, y, l, a;
    bool vertical;
    int  valor;          /* 0..100 */
    bool indeterminado;
    int  fase;           /* posición de la banda en el modo indeterminado */
    bool mostrar_texto;
    uint32_t cor, cor_fundo, cor_texto;
};

progresso_t *progresso_criar(int x, int y, int l, int a, bool vertical)
{
    progresso_t *p = aloca_zerado(sizeof *p);
    if (!p) return NULL;
    p->x = x; p->y = y; p->l = l; p->a = a;
    p->vertical = vertical;
    p->cor = COR_DESTAQUE;
    p->cor_fundo = COR_BOTAO;
    p->cor_texto = COR_TEXTO;
    return p;
}

void progresso_definir(progresso_t *p, int valor)
{
    if (!p) return;
    if (valor < 0) valor = 0;
    if (valor > 100) valor = 100;
    p->valor = valor;
}

void progresso_indeterminado(progresso_t *p, bool ligar)
{
    if (!p) return;
    p->indeterminado = ligar;
    p->fase = 0;
}

void progresso_texto(progresso_t *p, bool mostrar)
{
    if (!p) return;
    p->mostrar_texto = mostrar;
}

void progresso_desenhar(jan_t *j, progresso_t *p)
{
    if (!p) return;
    g_retangulo(j, p->x, p->y, p->l, p->a, p->cor_fundo);
    g_moldura(j, p->x, p->y, p->l, p->a, COR_BORDA);

    if (p->indeterminado) {
        /* Una banda del 30 % que va y viene. */
        int comp = p->vertical ? p->a : p->l;
        int banda = comp / 3;
        if (banda < 4) banda = 4;
        if (banda > comp) banda = comp;
        int pos = (p->fase * comp) / 120;
        if (pos + banda > comp) pos = comp - banda;
        if (pos < 0) pos = 0;
        if (p->vertical)
            g_retangulo(j, p->x + 1, p->y + p->a - pos - banda + 1, p->l - 2, banda, p->cor);
        else
            g_retangulo(j, p->x + pos + 1, p->y + 1, banda, p->a - 2, p->cor);
    } else {
        int total = p->vertical ? p->a : p->l;
        int cheio = total * p->valor / 100;
        if (cheio > 0) {
            if (p->vertical)
                g_retangulo(j, p->x + 1, p->y + p->a - cheio, p->l - 2, cheio, p->cor);
            else
                g_retangulo(j, p->x + 1, p->y + 1, cheio, p->a - 2, p->cor);
        }
    }
    if (p->mostrar_texto) {
        char buf[16];
        int n = formatar_texto(buf, sizeof buf, "%d%%", p->indeterminado ? 0 : p->valor);
        int tx = p->x + (p->l - n * LETRA_L) / 2;
        int ty = p->y + (p->a - LETRA_A) / 2;
        /* El texto va sobre un rectángulo del color de fondo para que se lea. */
        g_retangulo(j, tx - 2, ty - 1, n * LETRA_L + 4, LETRA_A + 2, p->cor_fundo);
        g_texto(j, tx, ty, buf, p->cor_texto);
    }
    GR_REG(j, "progresso", p, p->x, p->y, p->l, p->a, "", 0, 0);
}

/* Avanza la fase de la barra indeterminada; se llama en el bucle de eventos.
 * Devuelve true si hay que redibujar. */
bool progresso_avancar(progresso_t *p)
{
    if (!p || !p->indeterminado) return false;
    p->fase = (p->fase + 1) % 120;
    return true;
}

void progresso_rect(progresso_t *p, retangulo_t *r)
{
    if (!p || !r) return;
    r->x = p->x; r->y = p->y;
    r->largura = p->l; r->altura = p->a;
}

/* Suelta la barra: se usa al redimensionar la ventana, que cambia su sitio. */
void progresso_liberar(progresso_t *p)
{
    if (p) libera(p);
}

/* ---- casilla de verificación ---- */

void casilla_desenhar(jan_t *j, casilla_t *c)
{
    if (!c) return;
    int s = 14;
    g_retangulo(j, c->x, c->y, s, s, COR_CAMPO);
    g_moldura(j, c->x, c->y, s, s, COR_BORDA);
    if (c->marcada) {
        g_linha(j, c->x + 3, c->y + 7, c->x + 6, c->y + 10, COR_DESTAQUE);
        g_linha(j, c->x + 6, c->y + 10, c->x + 11, c->y + 3, COR_DESTAQUE);
    }
    GR_REG(j, "casilla", c, c->x, c->y, 14, 14, "", 0, 0);
}

bool casilla_tratar(casilla_t *c, const evento_t *e)
{
    if (!c || !e || e->tipo != EV_CLIQUE) return false;
    int s = 14;
    if (e->x < c->x || e->x >= c->x + s || e->y < c->y || e->y >= c->y + s) return false;
    c->marcada = !c->marcada;
    return true;
}
