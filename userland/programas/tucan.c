// SPDX-License-Identifier: GPL-2.0-or-later
/* tucan [archivo] - el editor de presentaciones de SaguiOS (D369), con la cinta de la suite (Pluma, Macaco).
 *
 * Lista de diapositivas en miniatura a la izquierda (arrastrar para reordenar, duplicar, eliminar, ocultar),
 * lienzo 16:9 o 4:3 en el centro, notas del orador debajo y cinta con Inicio, Insertar, Diseño, Transiciones,
 * Presentación y Ver. Objetos: cuadros de texto (con viñetas y numeración por niveles), formas, imágenes,
 * WordArt, tablas y gráficos; mover, redimensionar, agrupar, alinear, orden, copiar/pegar, deshacer/rehacer.
 * Presentación a pantalla completa con transiciones, animaciones de entrada, láser, pantalla negra e ir a
 * una diapositiva. Formatos: .tcn (propio), .pptx (leer y escribir), PDF y PNG (escribir).
 *
 * Modos sin ventana (para pruebas y para convertir): ver tucan_modos.h. El modelo y los ficheros están en la
 * libsagui (presentacion.h). Diseño y límites: docs/tucan.md. */
#include "sagui.h"
#include "gui.h"
#include "fontes.h"
#include "pdf_escribir.h"
#include "wordart.h"
#include "presentacion.h"
#include "compressao.h"
#include "agentes.h"
#include "icones.h"

#define THUMB_W   160
#define NOTAS_A   96
#define ESTADO_A  20
#define TK_F1     0x10A
#define TK_F5     0x10E
#define UNDO_MAX  40

/* ------------------------------------------------------------ iconos 16x16 */

#define R(x_, y_, w_, h_, c_) g_retangulo(j, x + (x_), y + (y_), (w_), (h_), (c_))
#define COR(a, c) ((a) ? (c) : 0x808080)

static void folha(jan_t *jj, int x, int y, bool a, int dx, int dy, uint32_t fundo)
{
    jan_t *j = jj;
    R(dx, dy, 9, 11, COR(a, W98_NEGRO)); R(dx + 1, dy + 1, 7, 9, COR(a, fundo));
    R(dx + 6, dy, 3, 3, COR(a, W98_FACE)); R(dx + 6, dy + 2, 3, 1, COR(a, W98_NEGRO)); R(dx + 6, dy, 1, 3, COR(a, W98_NEGRO));
}
static void ic_guardar(jan_t *j, int x, int y, bool a)
{
    R(1, 1, 14, 14, COR(a, W98_NEGRO)); R(2, 2, 12, 12, COR(a, 0x0000A0));
    R(4, 2, 8, 5, COR(a, W98_LUZ)); R(5, 3, 3, 3, COR(a, W98_SOMBRA)); R(4, 9, 8, 5, COR(a, W98_FACE)); R(6, 10, 4, 3, COR(a, W98_OSCURO));
}
static void ic_pdf(jan_t *j, int x, int y, bool a)
{
    folha(j, x, y, a, 3, 1, W98_LUZ);
    R(1, 8, 14, 7, COR(a, 0xC00000));
    R(2, 9, 1, 5, COR(a, W98_LUZ)); R(2, 9, 3, 1, COR(a, W98_LUZ)); R(4, 9, 1, 3, COR(a, W98_LUZ)); R(2, 11, 3, 1, COR(a, W98_LUZ));
    R(6, 9, 1, 5, COR(a, W98_LUZ)); R(6, 9, 3, 1, COR(a, W98_LUZ)); R(8, 9, 1, 5, COR(a, W98_LUZ)); R(6, 13, 3, 1, COR(a, W98_LUZ));
    R(10, 9, 1, 5, COR(a, W98_LUZ)); R(10, 9, 3, 1, COR(a, W98_LUZ)); R(10, 11, 3, 1, COR(a, W98_LUZ));
}
static void ic_cortar(jan_t *j, int x, int y, bool a)
{
    g_linha(j, x + 4, y + 1, x + 10, y + 10, COR(a, W98_NEGRO)); g_linha(j, x + 11, y + 1, x + 5, y + 10, COR(a, W98_NEGRO));
    R(2, 10, 4, 4, COR(a, 0xC00000)); R(3, 11, 2, 2, COR(a, W98_FACE)); R(9, 10, 4, 4, COR(a, 0xC00000)); R(10, 11, 2, 2, COR(a, W98_FACE));
}
static void ic_copiar(jan_t *j, int x, int y, bool a) { folha(j, x, y, a, 1, 1, W98_LUZ); folha(j, x, y, a, 6, 4, W98_LUZ); }
static void ic_pegar(jan_t *j, int x, int y, bool a)
{
    R(1, 2, 11, 13, COR(a, 0x804000)); R(2, 3, 9, 11, COR(a, 0xC08040)); R(4, 1, 5, 3, COR(a, W98_OSCURO));
    folha(j, x, y, a, 6, 5, W98_LUZ);
}
static void ic_deshacer(jan_t *j, int x, int y, bool a)
{
    R(3, 3, 1, 1, COR(a, 0x000080)); R(2, 4, 3, 1, COR(a, 0x000080)); R(1, 5, 5, 1, COR(a, 0x000080));
    R(3, 6, 8, 2, COR(a, 0x000080)); R(10, 7, 3, 3, COR(a, 0x000080)); R(11, 10, 2, 2, COR(a, 0x000080)); R(3, 10, 8, 2, COR(a, 0x000080));
    R(4, 8, 6, 1, COR(a, 0x000080));
}
static void ic_rehacer(jan_t *j, int x, int y, bool a)
{
    R(12, 3, 1, 1, COR(a, 0x000080)); R(11, 4, 3, 1, COR(a, 0x000080)); R(10, 5, 5, 1, COR(a, 0x000080));
    R(4, 6, 8, 2, COR(a, 0x000080)); R(2, 7, 3, 3, COR(a, 0x000080)); R(2, 10, 2, 2, COR(a, 0x000080)); R(4, 10, 8, 2, COR(a, 0x000080));
    R(5, 8, 6, 1, COR(a, 0x000080));
}
static void linhas_alin(jan_t *j, int x, int y, bool a, const int *xs, const int *ls)
{
    for (int i = 0; i < 5; i++) R(xs[i], 2 + i * 3, ls[i], 2, COR(a, W98_NEGRO));
}
static void ic_esq(jan_t *j, int x, int y, bool a) { static const int xs[5] = { 2, 2, 2, 2, 2 }, ls[5] = { 12, 8, 12, 8, 12 }; linhas_alin(j, x, y, a, xs, ls); }
static void ic_centro(jan_t *j, int x, int y, bool a) { static const int xs[5] = { 2, 4, 2, 4, 2 }, ls[5] = { 12, 8, 12, 8, 12 }; linhas_alin(j, x, y, a, xs, ls); }
static void ic_dir(jan_t *j, int x, int y, bool a) { static const int xs[5] = { 2, 6, 2, 6, 2 }, ls[5] = { 12, 8, 12, 8, 12 }; linhas_alin(j, x, y, a, xs, ls); }
static void ic_vinetas(jan_t *j, int x, int y, bool a)
{
    for (int i = 0; i < 3; i++) { R(2, 3 + i * 4, 2, 2, COR(a, W98_NEGRO)); R(6, 3 + i * 4, 8, 2, COR(a, W98_NEGRO)); }
}
static void ic_numeracion(jan_t *j, int x, int y, bool a)
{
    for (int i = 0; i < 3; i++) { R(2, 2 + i * 4, 1, 3, COR(a, 0x000080)); R(6, 3 + i * 4, 8, 2, COR(a, W98_NEGRO)); }
}
static void ic_nivel_mas(jan_t *j, int x, int y, bool a)
{
    R(7, 2, 7, 2, COR(a, W98_NEGRO)); R(7, 6, 7, 2, COR(a, W98_NEGRO)); R(2, 10, 12, 2, COR(a, W98_NEGRO));
    R(2, 4, 1, 5, COR(a, 0x000080)); R(3, 5, 1, 3, COR(a, 0x000080)); R(4, 6, 1, 1, COR(a, 0x000080));
}
static void ic_nivel_menos(jan_t *j, int x, int y, bool a)
{
    R(7, 2, 7, 2, COR(a, W98_NEGRO)); R(7, 6, 7, 2, COR(a, W98_NEGRO)); R(2, 10, 12, 2, COR(a, W98_NEGRO));
    R(4, 4, 1, 5, COR(a, 0x000080)); R(3, 5, 1, 3, COR(a, 0x000080)); R(2, 6, 1, 1, COR(a, 0x000080));
}
static void ic_nueva(jan_t *j, int x, int y, bool a)
{
    R(1, 3, 13, 10, COR(a, W98_NEGRO)); R(2, 4, 11, 8, COR(a, W98_LUZ)); R(3, 5, 6, 1, COR(a, 0xFF8000)); R(3, 7, 9, 1, COR(a, W98_SOMBRA)); R(3, 9, 7, 1, COR(a, W98_SOMBRA));
    R(11, 0, 1, 6, COR(a, 0x008000)); R(9, 2, 5, 1, COR(a, 0x008000));
}
static void ic_dup(jan_t *j, int x, int y, bool a)
{
    R(1, 1, 9, 7, COR(a, W98_NEGRO)); R(2, 2, 7, 5, COR(a, W98_LUZ));
    R(5, 6, 9, 8, COR(a, W98_NEGRO)); R(6, 7, 7, 6, COR(a, 0xFFE0B0));
}
static void ic_borrar(jan_t *j, int x, int y, bool a)
{
    g_linha(j, x + 3, y + 3, x + 12, y + 12, COR(a, 0xC00000)); g_linha(j, x + 4, y + 3, x + 13, y + 12, COR(a, 0xC00000));
    g_linha(j, x + 12, y + 3, x + 3, y + 12, COR(a, 0xC00000)); g_linha(j, x + 13, y + 3, x + 4, y + 12, COR(a, 0xC00000));
}
static void ic_forma(jan_t *j, int x, int y, bool a)
{
    R(1, 6, 8, 8, COR(a, 0x2E75B6)); R(2, 7, 6, 6, COR(a, 0x9DC3E6));
    R(8, 1, 6, 6, COR(a, 0xED7D31)); R(9, 2, 4, 4, COR(a, 0xF8CBAD));
}
static void ic_frente(jan_t *j, int x, int y, bool a)
{
    R(1, 1, 8, 8, COR(a, W98_NEGRO)); R(2, 2, 6, 6, COR(a, 0x9DC3E6)); R(6, 6, 8, 8, COR(a, W98_NEGRO)); R(7, 7, 6, 6, COR(a, 0xED7D31));
}
static void ic_atras(jan_t *j, int x, int y, bool a)
{
    R(6, 6, 8, 8, COR(a, W98_NEGRO)); R(7, 7, 6, 6, COR(a, 0x9DC3E6)); R(1, 1, 8, 8, COR(a, W98_NEGRO)); R(2, 2, 6, 6, COR(a, 0xED7D31));
}
static void ic_ayuda(jan_t *j, int x, int y, bool a)
{
    R(3, 1, 10, 14, COR(a, 0x2E75B6)); R(4, 2, 8, 12, COR(a, 0xDDEBF7));
    R(6, 4, 4, 1, COR(a, W98_NEGRO)); R(9, 5, 1, 2, COR(a, W98_NEGRO)); R(7, 7, 2, 1, COR(a, W98_NEGRO)); R(7, 9, 2, 2, COR(a, W98_NEGRO));
}
static void ic_texto(jan_t *j, int x, int y, bool a)
{
    R(1, 2, 14, 12, COR(a, W98_SOMBRA)); R(2, 3, 12, 10, COR(a, W98_LUZ));
    R(4, 4, 8, 2, COR(a, W98_NEGRO)); R(7, 6, 2, 6, COR(a, W98_NEGRO));
}
static void ic_imagen(jan_t *j, int x, int y, bool a)
{
    R(1, 2, 14, 12, COR(a, W98_NEGRO)); R(2, 3, 12, 10, COR(a, 0x80C0FF)); R(10, 4, 3, 3, COR(a, 0xFFE000));
    R(2, 10, 12, 3, COR(a, 0x208020)); R(4, 8, 4, 2, COR(a, 0x208020)); R(5, 7, 2, 1, COR(a, 0x208020));
}
static void ic_tabla(jan_t *j, int x, int y, bool a)
{
    R(1, 2, 14, 12, COR(a, W98_NEGRO)); R(2, 3, 12, 10, COR(a, W98_LUZ));
    R(2, 3, 12, 3, COR(a, 0x2E75B6)); R(7, 3, 1, 10, COR(a, W98_NEGRO)); R(2, 8, 12, 1, COR(a, W98_NEGRO)); R(2, 11, 12, 1, COR(a, W98_NEGRO));
}
static void ic_grafico(jan_t *j, int x, int y, bool a)
{
    R(1, 1, 1, 13, COR(a, W98_NEGRO)); R(1, 13, 14, 1, COR(a, W98_NEGRO));
    R(3, 8, 3, 5, COR(a, 0x2E75B6)); R(7, 4, 3, 9, COR(a, 0xED7D31)); R(11, 6, 3, 7, COR(a, 0x70AD47));
}
static void ic_wordart(jan_t *j, int x, int y, bool a)
{
    static const int p[][3] = { { 7, 2, 2 }, { 6, 4, 2 }, { 8, 4, 2 }, { 5, 6, 2 }, { 9, 6, 2 }, { 4, 8, 2 }, { 10, 8, 2 }, { 3, 10, 3 }, { 10, 10, 3 } };
    for (int i = 0; i < 9; i++) R(p[i][0] + 1, p[i][1] + 1, p[i][2], 2, COR(a, 0xFF8000));
    for (int i = 0; i < 9; i++) R(p[i][0], p[i][1], p[i][2], 2, COR(a, 0x0030C0));
    R(5, 8, 6, 1, COR(a, 0x0030C0)); R(6, 9, 4, 1, COR(a, 0x0030C0));
}
static void ic_presentar(jan_t *j, int x, int y, bool a)
{
    R(1, 2, 14, 9, COR(a, W98_NEGRO)); R(2, 3, 12, 7, COR(a, 0x14110F)); R(6, 4, 1, 5, COR(a, 0xFFA726)); R(7, 5, 1, 3, COR(a, 0xFFA726)); R(8, 6, 1, 1, COR(a, 0xFFA726));
    R(7, 11, 2, 3, COR(a, W98_SOMBRA)); R(4, 14, 8, 1, COR(a, W98_SOMBRA));
}

static void ic_ia(jan_t *jj, int x, int y, bool a) { (void)a; icone_desenhar(jj, x, y, "robo", 1); }

#include "tucan_cinta.h"
#include "tucan_modos.h"

/* ------------------------------------------------------------ estado */

static jan_t *j;
static pr_deck_t *dk;
static int cur;                              /* diapositiva actual */
static bool sel[PR_OBJ_MAX];
static char caminho[220];
static bool modificado;
static unsigned deck_ver;                    /* sube con cada cambio que afecta a todas las diapositivas */
static cinta_t cinta;
static area_t notas;
static bool notas_vis = true, mini_vis = true, notas_sucia;
static bool foco_mini;
static int edit_obj = -1, edit_pos;          /* edición de texto en el lienzo */
static int mini_top;                         /* desplazamiento de la lista de miniaturas, en píxeles */
static bool laser_opc;
static bool cinta_log;
static char ult_titulo[160];
static bool sair_pedido;
static ag_panel_t *ag;                       /* el panel del asistente IA (D355): el lienzo cede su ancho */
static bool ag_visible, ag_foco;
static int  ag_version;                      /* sube cada vez que una herramienta cambia la presentación */
#define AG_AGENTE "Tucan IA"

/* rectángulos de la ventana */
static int r_mini_x, r_mini_y, r_mini_w, r_mini_h;
static int r_cv_x, r_cv_y, r_cv_w, r_cv_h;   /* zona gris del lienzo */
static int sx, sy, sw, sh;                    /* la diapositiva en pantalla */
static int r_not_x, r_not_y, r_not_w, r_not_h;
static int r_est_y;

/* cachés: lienzo y miniaturas */
static uint32_t *lienzo;
static int lienzo_w, lienzo_h, lienzo_idx = -1;
static unsigned lienzo_ver, lienzo_dv;
#define MINI_MAX 36
static struct { pr_diap_t *s; uint32_t *px; unsigned ver, dv, uso; int w, h; } minis[MINI_MAX];
static unsigned mini_uso;

/* deshacer / rehacer: el texto interno de toda la presentación */
static char *und[UNDO_MAX], *red[UNDO_MAX];
static int und_n, red_n;

/* portapapeles de objetos */
static pr_obj_t clip[PR_OBJ_MAX];
static int clip_n;

/* arrastres */
enum { DR_NADA = 0, DR_MOVER, DR_REDIM, DR_MARCO, DR_MINI };
static int drag, drag_h, drag_dx, drag_dy, drag_x0, drag_y0, drag_mx, drag_my, drag_ini;
static int16_t drag_orig[PR_OBJ_MAX][4];
static int mini_destino = -1;

/* presentación */
static bool pres, pres_negra, pres_blanca, pres_laser;
static int pres_i, pres_paso, pres_mx = -1, pres_my = -1;
static char pres_num[8];
static uint32_t pres_t0;
static bool pres_entrando;

static void redibujar_todo(void);
static void accion(int id);

static void log_diap(void) { registrar("tucan: diapositiva %d de %d\n", cur + 1, dk->n); }

/* ------------------------------------------------------------ utilidades */

static pr_diap_t *diap(void) { return dk->d[cur]; }

static int primer_sel(void)
{
    pr_diap_t *s = diap();
    for (int i = 0; i < s->n; i++) if (sel[i]) return i;
    return -1;
}
static int n_sel(void)
{
    int n = 0;
    pr_diap_t *s = diap();
    for (int i = 0; i < s->n; i++) if (sel[i]) n++;
    return n;
}
static void sel_limpiar(void) { memset(sel, 0, sizeof sel); }

static void fondo_cambio(void) { deck_ver++; for (int i = 0; i < dk->n; i++) pr_tocar(dk->d[i]); }

/* ---- deshacer ---- */
static void undo_push(void)
{
    char *t = pr_a_texto(dk, NULL);
    if (!t) return;
    if (und_n == UNDO_MAX) { libera(und[0]); memmove(und, und + 1, (UNDO_MAX - 1) * sizeof und[0]); und_n--; }
    und[und_n++] = t;
    for (int i = 0; i < red_n; i++) libera(red[i]);
    red_n = 0;
}
static void cambio(void) { undo_push(); modificado = true; }

static void notas_a_diap(void)
{
    if (!notas_sucia) return;
    notas_sucia = false;
    pr_diap_t *s = diap();
    size_t n = notas.buf ? tam_texto(notas.buf) : 0;
    char *c = aloca(n + 1);
    if (!c) return;
    if (n) memcpy(c, notas.buf, n);
    c[n] = 0;
    libera(s->notas);
    s->notas = c;
}
static void diap_a_notas(void)
{
    const char *t = diap()->notas ? diap()->notas : "";
    area_definir(&notas, t, (int)tam_texto(t));
    notas_sucia = false;
}

static void restaurar(char *texto)
{
    pr_deck_t *nd = pr_de_texto(texto, (int)tam_texto(texto), dk);
    if (!nd) return;
    pr_liberar(dk);
    dk = nd;
    if (cur >= dk->n) cur = dk->n - 1;
    sel_limpiar(); edit_obj = -1;
    fondo_cambio();
    diap_a_notas();
}
static void deshacer(void)
{
    notas_a_diap();
    if (!und_n) return;
    char *now = pr_a_texto(dk, NULL);
    char *t = und[--und_n];
    if (now && red_n < UNDO_MAX) red[red_n++] = now; else libera(now);
    restaurar(t);
    libera(t);
    modificado = true;
    registrar("tucan: deshacer (%d restantes)\n", und_n);
}
static void rehacer(void)
{
    if (!red_n) return;
    char *now = pr_a_texto(dk, NULL);
    char *t = red[--red_n];
    if (now && und_n < UNDO_MAX) und[und_n++] = now; else libera(now);
    restaurar(t);
    libera(t);
    modificado = true;
    registrar("tucan: rehacer\n");
}

/* ---- geometría ---- */
static int px_x(int pt) { return sx + pt * sw / dk->ancho; }
static int px_y(int pt) { return sy + pt * sh / dk->alto; }

static int ag_larg(void)
{
    int l = j->larg * 40 / 100;
    return l < 360 ? l : 360;
}
static void ag_rect(int *x, int *y, int *l, int *a)
{
    *l = ag_larg(); *x = j->larg - *l; *y = cinta_altura(&cinta);
    *a = j->alt - ESTADO_A - *y;
}
static void ag_colocar(void);

static void layout(void)
{
    int base = cinta_altura(&cinta);
    r_est_y = j->alt - ESTADO_A;
    r_mini_x = 0; r_mini_y = base; r_mini_w = mini_vis ? THUMB_W : 0; r_mini_h = r_est_y - base;
    r_not_h = notas_vis ? NOTAS_A : 0;
    r_cv_x = r_mini_w; r_cv_y = base; r_cv_w = j->larg - r_mini_w - (ag_visible ? ag_larg() : 0); r_cv_h = r_est_y - base - r_not_h;
    r_not_x = r_cv_x; r_not_y = r_cv_y + r_cv_h; r_not_w = r_cv_w;
    int mw = r_cv_w - 28, mh = r_cv_h - 28;
    if (mw < 40) mw = 40;
    if (mh < 30) mh = 30;
    sw = mw; sh = sw * dk->alto / dk->ancho;
    if (sh > mh) { sh = mh; sw = sh * dk->ancho / dk->alto; }
    sx = r_cv_x + (r_cv_w - sw) / 2; sy = r_cv_y + (r_cv_h - sh) / 2;
    if (notas_vis) {
        notas_a_diap();
        area_iniciar(&notas, r_not_x + 4, r_not_y + 20, r_not_w - 8, r_not_h - 24);
        diap_a_notas();
    }
    ag_colocar();
    if (cinta_log) {
        int px = 0, py = 0, pl = 0, pa = 0;
        if (ag_visible) ag_rect(&px, &py, &pl, &pa);
        registrar("tucan: layout base=%d mini=%d,%d,%d,%d lienzo=%d,%d,%d,%d diap=%d,%d,%d,%d notas=%d,%d,%d,%d panel=%d,%d,%d,%d ventana=%dx%d\n", base,
                  r_mini_x, r_mini_y, r_mini_w, r_mini_h, r_cv_x, r_cv_y, r_cv_w, r_cv_h, sx, sy, sw, sh, r_not_x, r_not_y, r_not_w, r_not_h, px, py, pl, pa, j->larg, j->alt);
    }
}

/* ------------------------------------------------------------ cachés de dibujo */

static void cache_lienzo(void)
{
    pr_diap_t *s = diap();
    if (lienzo && lienzo_w == sw && lienzo_h == sh && lienzo_idx == cur && lienzo_ver == s->version && lienzo_dv == deck_ver) return;
    if (!lienzo || lienzo_w != sw || lienzo_h != sh) {
        libera(lienzo);
        lienzo = aloca((size_t)sw * (size_t)sh * 4);
        lienzo_w = sw; lienzo_h = sh;
    }
    if (!lienzo) return;
    pr_dibujar(dk, cur, lienzo, sw, sh, PRD_EDITOR, -1, -1);
    lienzo_idx = cur; lienzo_ver = s->version; lienzo_dv = deck_ver;
}

static int mini_alto(void) { return (THUMB_W - 32) * dk->alto / dk->ancho; }
#define MINI_FILA (mini_alto() + 14)

static uint32_t *mini_de(int i)
{
    pr_diap_t *s = dk->d[i];
    int w = THUMB_W - 32, h = mini_alto();
    int libre = -1, viejo = 0;
    for (int k = 0; k < MINI_MAX; k++) {
        if (minis[k].s == s && minis[k].px && minis[k].w == w && minis[k].h == h) {
            if (minis[k].ver != s->version || minis[k].dv != deck_ver) {
                pr_dibujar(dk, i, minis[k].px, w, h, 0, -1, -1);
                minis[k].ver = s->version; minis[k].dv = deck_ver;
            }
            minis[k].uso = ++mini_uso;
            return minis[k].px;
        }
        if (!minis[k].px && libre < 0) libre = k;
        if (minis[k].uso < minis[viejo].uso) viejo = k;
    }
    int k = libre >= 0 ? libre : viejo;
    libera(minis[k].px);
    minis[k].px = aloca((size_t)w * (size_t)h * 4);
    if (!minis[k].px) { minis[k].s = NULL; return NULL; }
    minis[k].s = s; minis[k].w = w; minis[k].h = h;
    pr_dibujar(dk, i, minis[k].px, w, h, 0, -1, -1);
    minis[k].ver = s->version; minis[k].dv = deck_ver; minis[k].uso = ++mini_uso;
    return minis[k].px;
}

/* ------------------------------------------------------------ dibujo de la interfaz */

static void titulo(void)
{
    char t[200];
    const char *b = caminho[0] ? acha_ultimo_char(caminho, '/') : NULL;
    formatar_texto(t, sizeof t, _("%s - Tucán%s"), caminho[0] ? (b ? b + 1 : caminho) : _("Presentación1"), modificado ? " *" : "");
    if (compara_texto(t, ult_titulo) == 0) return;
    copia_texto(ult_titulo, t, sizeof ult_titulo);
    titulo_janela(j->id, t);
    registrar("tucan: titulo %s\n", t);
}

static void obj_rect_px(const pr_obj_t *o, int dx, int dy, int *x, int *y, int *w, int *h)
{
    *x = px_x(o->x + dx); *y = px_y(o->y + dy);
    *w = o->w * sw / dk->ancho; *h = o->h * sh / dk->alto;
    if (*w < 2) *w = 2;
    if (*h < 2) *h = 2;
}

static void marco(int x, int y, int w, int h, uint32_t c)
{
    g_moldura(j, x, y, w, h, c);
}

static void manijas(int bx, int by, int bw, int bh, int *hx, int *hy)
{
    hx[0] = bx; hy[0] = by; hx[1] = bx + bw / 2; hy[1] = by; hx[2] = bx + bw; hy[2] = by; hx[3] = bx + bw; hy[3] = by + bh / 2;
    hx[4] = bx + bw; hy[4] = by + bh; hx[5] = bx + bw / 2; hy[5] = by + bh; hx[6] = bx; hy[6] = by + bh; hx[7] = bx; hy[7] = by + bh / 2;
}

static bool caja_sel(int *x, int *y, int *w, int *h)
{
    pr_diap_t *s = diap();
    int x0 = 1 << 30, y0 = 1 << 30, x1 = -(1 << 30), y1 = -(1 << 30);
    bool alguno = false;
    for (int i = 0; i < s->n; i++) {
        if (!sel[i]) continue;
        int a, b, c, d;
        obj_rect_px(&s->obj[i], drag == DR_MOVER ? drag_dx : 0, drag == DR_MOVER ? drag_dy : 0, &a, &b, &c, &d);
        if (a < x0) x0 = a;
        if (b < y0) y0 = b;
        if (a + c > x1) x1 = a + c;
        if (b + d > y1) y1 = b + d;
        alguno = true;
    }
    if (alguno) { *x = x0; *y = y0; *w = x1 - x0; *h = y1 - y0; }
    return alguno;
}

static void dibujar_lienzo(void)
{
    g_retangulo(j, r_cv_x, r_cv_y, r_cv_w, r_cv_h, 0x7F8794);
    cache_lienzo();
    g_retangulo(j, sx + 3, sy + 3, sw, sh, 0x5A616B);   /* sombra */
    if (lienzo) {
        for (int y = 0; y < sh; y++) {
            int Y = sy + y;
            if (Y < 0 || Y >= j->alt) continue;
            memcpy(j->px + Y * j->larg + sx, lienzo + y * sw, (size_t)sw * 4);
        }
    }
    marco(sx - 1, sy - 1, sw + 2, sh + 2, 0x303030);
    pr_diap_t *s = diap();
    /* el objeto en edición: marco punteado y cursor */
    if (edit_obj >= 0 && edit_obj < s->n) {
        pr_obj_t *o = &s->obj[edit_obj];
        int x, y, w, h;
        obj_rect_px(o, 0, 0, &x, &y, &w, &h);
        for (int k = 0; k < w; k += 6) { g_retangulo(j, x + k, y, 3, 1, 0x2A6FD8); g_retangulo(j, x + k, y + h - 1, 3, 1, 0x2A6FD8); }
        for (int k = 0; k < h; k += 6) { g_retangulo(j, x, y + k, 1, 3, 0x2A6FD8); g_retangulo(j, x + w - 1, y + k, 1, 3, 0x2A6FD8); }
        pr_comp_t c;
        if (pr_componer(dk, o, o->texto ? o->texto : "", &c)) {
            int cx, cy;
            pr_cursor_xy(dk, o, &c, edit_pos, &cx, &cy);
            int hpx = o->tam * 120 / 100 * sh / dk->alto;
            int cxp = x + cx / 100 * sw / dk->ancho + (cx % 100) * sw / dk->ancho / 100, cyp = y + cy / 100 * sh / dk->alto;
            if (hpx < 6) hpx = 6;
            g_retangulo(j, cxp, cyp, 2, hpx, 0x2A6FD8);
            pr_comp_liberar(&c);
        }
        return;
    }
    /* selección: marco y manijas */
    int bx, by, bw, bh;
    if (caja_sel(&bx, &by, &bw, &bh)) {
        marco(bx, by, bw, bh, 0x2A6FD8);
        if (drag == DR_MOVER) { marco(bx + 1, by + 1, bw - 2, bh - 2, 0xA0C8FF); }
        if (n_sel() == 1 && drag != DR_MOVER) {
            int hx[8], hy[8];
            manijas(bx, by, bw, bh, hx, hy);
            for (int k = 0; k < 8; k++) { g_retangulo(j, hx[k] - 3, hy[k] - 3, 7, 7, 0x2A6FD8); g_retangulo(j, hx[k] - 2, hy[k] - 2, 5, 5, W98_LUZ); }
        }
    }
    if (drag == DR_MOVER) {   /* el contorno de cada objeto que se mueve */
        for (int i = 0; i < s->n; i++) if (sel[i]) { int a, b, c, d; obj_rect_px(&s->obj[i], drag_dx, drag_dy, &a, &b, &c, &d); marco(a, b, c, d, 0xFFFFFF); marco(a - 1, b - 1, c + 2, d + 2, 0x2A6FD8); }
    }
    if (drag == DR_MARCO) {
        int a = drag_x0 < drag_mx ? drag_x0 : drag_mx, b = drag_y0 < drag_my ? drag_y0 : drag_my;
        marco(a, b, (drag_x0 < drag_mx ? drag_mx - drag_x0 : drag_x0 - drag_mx) + 1, (drag_y0 < drag_my ? drag_my - drag_y0 : drag_y0 - drag_my) + 1, 0x2A6FD8);
    }
}

static void dibujar_minis(void)
{
    if (!mini_vis) return;
    g_retangulo(j, r_mini_x, r_mini_y, r_mini_w, r_mini_h, 0xD4D8DE);
    g_retangulo(j, r_mini_x + r_mini_w - 1, r_mini_y, 1, r_mini_h, 0x808080);
    int fila = MINI_FILA, mw = THUMB_W - 32, mh = mini_alto();
    int tope = mini_top;
    for (int i = 0; i < dk->n; i++) {
        int y = r_mini_y + 6 + i * fila - tope;
        if (y + mh < r_mini_y || y > r_mini_y + r_mini_h) continue;
        bool actual = i == cur;
        if (actual) g_retangulo(j, r_mini_x + 1, y - 4, r_mini_w - 3, mh + 8, 0xB8CBE8);
        char n[8];
        formatar_texto(n, sizeof n, "%d", i + 1);
        if (y >= r_mini_y && y + 16 < r_mini_y + r_mini_h) g_texto(j, r_mini_x + 4, y + 2, n, W98_NEGRO);
        uint32_t *px = mini_de(i);
        int x0 = r_mini_x + 26;
        for (int yy = 0; yy < mh; yy++) {
            int Y = y + yy;
            if (Y < r_mini_y || Y >= r_mini_y + r_mini_h || !px) continue;
            memcpy(j->px + Y * j->larg + x0, px + yy * mw, (size_t)mw * 4);
        }
        if (y >= r_mini_y && y + mh <= r_mini_y + r_mini_h) {
            marco(x0 - 1, y - 1, mw + 2, mh + 2, actual ? 0x2A6FD8 : 0x606060);
            if (actual) marco(x0 - 2, y - 2, mw + 4, mh + 4, 0x2A6FD8);
        }
        if (dk->d[i]->oculta && y >= r_mini_y && y + mh <= r_mini_y + r_mini_h) {
            g_linha(j, x0, y, x0 + mw - 1, y + mh - 1, 0xC00000); g_linha(j, x0, y + mh - 1, x0 + mw - 1, y, 0xC00000);
        }
    }
    if (drag == DR_MINI && mini_destino >= 0) {
        int y = r_mini_y + 6 + mini_destino * fila - tope - 3;
        if (y >= r_mini_y && y < r_mini_y + r_mini_h) g_retangulo(j, r_mini_x + 4, y, r_mini_w - 10, 3, 0xE53935);
    }
}

static void dibujar_notas(void)
{
    if (!notas_vis) return;
    g_retangulo(j, r_not_x, r_not_y, r_not_w, r_not_h, W98_FACE);
    g_retangulo(j, r_not_x, r_not_y, r_not_w, 1, 0x808080);
    g_texto(j, r_not_x + 6, r_not_y + 3, _("Notas del orador"), W98_NEGRO);
    area_desenhar(j, &notas);
}

static void dibujar_estado(void)
{
    g_retangulo(j, 0, r_est_y, j->larg, ESTADO_A, W98_FACE);
    g_retangulo(j, 0, r_est_y, j->larg, 1, 0x808080);
    char t[200];
    formatar_texto(t, sizeof t, _("Diapositiva %d de %d   %s   Tema: %s%s"), cur + 1, dk->n, dk->ancho == 960 ? "16:9" : "4:3", pr_temas[dk->tema].nombre, modificado ? _("   (modificada)") : "");
    g_texto(j, 8, r_est_y + 2, t, W98_NEGRO);
}

/* La cinta refleja el formato del objeto elegido. */
static void ui_sincronizar(void)
{
    pr_diap_t *s = diap();
    int i = edit_obj >= 0 ? edit_obj : primer_sel();
    pr_obj_t *o = i >= 0 && i < s->n ? &s->obj[i] : NULL;
    cinta_marcar(&cinta, A_NEGRITA, o && (o->est & PR_NEGRITA));
    cinta_marcar(&cinta, A_CURSIVA, o && (o->est & PR_CURSIVA));
    cinta_marcar(&cinta, A_SUBRAYADO, o && (o->est & PR_SUBRAYADO));
    cinta_marcar(&cinta, o && o->alin == 1 ? A_ALIN_CEN : o && o->alin == 2 ? A_ALIN_DER : A_ALIN_IZQ, true);
    cinta_marcar(&cinta, A_VINETAS, o && o->lista == PR_LISTA_VINETA);
    cinta_marcar(&cinta, A_NUMERACION, o && o->lista == PR_LISTA_NUMERO);
    cinta_marcar(&cinta, A_DIAP_OCULTAR, s->oculta);
    cinta_marcar(&cinta, A_AUTO, dk->auto_avance);
    cinta_marcar(&cinta, A_LASER, laser_opc);
    cinta_marcar(&cinta, A_NUM_DIAP, dk->num);
    cinta_marcar(&cinta, A_NOTAS, notas_vis);
    cinta_marcar(&cinta, A_ASISTENTE, ag_visible);
    cinta_marcar(&cinta, A_MINIATURAS, mini_vis);
    cinta_marcar(&cinta, dk->ancho == 960 ? A_TAM_169 : A_TAM_43, true);
    cinta_ctrl_t *kf = cinta_buscar(&cinta, A_C_FONTE), *kt = cinta_buscar(&cinta, A_C_TAM), *kg = cinta_buscar(&cinta, A_C_GROSOR);
    if (kf && o) combo_fuentes_definir(&kf->cf, pr_familia(pr_fuente_obj(dk, o)));
    if (kt && !kt->cb.foco && o) { char t[8]; formatar_texto(t, sizeof t, "%d", o->tam); combo_definir(&kt->cb, t); }
    if (kg && !kg->cb.foco && o) { char t[8]; formatar_texto(t, sizeof t, "%d", o->borde); combo_definir(&kg->cb, t); }
    cinta_ctrl_t *kc = cinta_buscar(&cinta, A_C_COR), *kr = cinta_buscar(&cinta, A_C_RELLENO), *kb = cinta_buscar(&cinta, A_C_BORDE);
    if (o) { if (kc) kc->cor = pr_color_texto(dk, o); if (kr) kr->cor = pr_color_relleno(dk, o); if (kb) kb->cor = pr_color_borde(dk, o); }
    cinta_ctrl_t *kx = cinta_buscar(&cinta, A_C_TRANS), *kd = cinta_buscar(&cinta, A_C_DUR), *ka = cinta_buscar(&cinta, A_C_AVANZAR), *kn = cinta_buscar(&cinta, A_C_ANIM);
    if (kx) kx->sel = s->trans;
    if (kn) kn->sel = o ? o->anim : 0;
    if (kd && !kd->cb.foco) { int dd = s->dur_trans ? s->dur_trans : 5; combo_definir(&kd->cb, dd <= 3 ? durs_txt[0] : dd <= 5 ? durs_txt[1] : dd <= 10 ? durs_txt[2] : durs_txt[3]); }
    if (ka && !ka->cb.foco) { int q = s->auto_seg; combo_definir(&ka->cb, q == 0 ? avanzar_txt[0] : q <= 3 ? avanzar_txt[1] : q <= 5 ? avanzar_txt[2] : q <= 10 ? avanzar_txt[3] : avanzar_txt[4]); }
    cinta_ctrl_t *kth = cinta_buscar(&cinta, A_C_TEMA);
    if (kth) kth->sel = dk->tema;
    cinta_ctrl_t *kft = cinta_buscar(&cinta, A_C_FTIT), *kfc = cinta_buscar(&cinta, A_C_FCUE);
    if (kft) combo_fuentes_definir(&kft->cf, pr_familia(dk->f_titulo));
    if (kfc) combo_fuentes_definir(&kfc->cf, pr_familia(dk->f_cuerpo));
}

static void redibujar_todo(void)
{
    g_retangulo(j, 0, 0, j->larg, j->alt, W98_FACE);
    ui_sincronizar();
    cinta_desenhar(j, &cinta);
    dibujar_minis();
    dibujar_lienzo();
    dibujar_notas();
    dibujar_estado();
    if (ag_visible && ag) ag_panel_desenhar(ag);
    titulo();
    jan_mostrar(j);
}

static void redibujar_lienzo(void)
{
    dibujar_lienzo();
    jan_mostrar_area(j, r_cv_x, r_cv_y, r_cv_w, r_cv_h);
}

/* ------------------------------------------------------------ operaciones sobre objetos */

static void ir_a(int i)
{
    if (i < 0) i = 0;
    if (i >= dk->n) i = dk->n - 1;
    if (i == cur) return;
    notas_a_diap();
    cur = i;
    sel_limpiar(); edit_obj = -1;
    diap_a_notas();
    /* que la miniatura se vea */
    int fila = MINI_FILA, y = 6 + cur * fila;
    if (y - mini_top < 0) mini_top = y - 6;
    if (y + mini_alto() - mini_top > r_mini_h) mini_top = y + mini_alto() - r_mini_h + 6;
    if (mini_top < 0) mini_top = 0;
    log_diap();
}

static void insertar_diap(int layout)
{
    notas_a_diap();
    cambio();
    pr_diap_t *s = pr_diap_nueva(dk, cur + 1, layout);
    if (!s) { mensagem("Tucán", _("No se puede añadir otra diapositiva.")); return; }
    int antes = cur;
    cur = -1;
    ir_a(antes + 1);
    registrar("tucan: diapositiva nueva con el diseño %d\n", layout);
}

static pr_obj_t *objeto_nuevo(int tipo, int w, int h)
{
    pr_diap_t *s = diap();
    pr_obj_t *o = pr_obj_nuevo(s, tipo);
    if (!o) { mensagem("Tucán", _("Hay demasiados objetos en la diapositiva.")); return NULL; }
    o->w = (int16_t)w; o->h = (int16_t)h;
    o->x = (int16_t)((dk->ancho - w) / 2); o->y = (int16_t)((dk->alto - h) / 2);
    sel_limpiar();
    sel[s->n - 1] = true;
    return o;
}

static void forma_nueva(int forma)
{
    cambio();
    pr_obj_t *o = objeto_nuevo(PR_FORMA, forma == PF_LINEA ? 240 : 200, forma == PF_LINEA ? 6 : 130);
    if (!o) return;
    o->forma = (uint8_t)forma;
    o->rellena = forma == PF_LINEA ? 0 : 1; o->borde = 2; o->tam = 20; o->alin = 1; o->valign = 1;
    registrar("tucan: forma nueva %s\n", pr_nombre_forma(forma));
}

static void texto_nuevo(void)
{
    cambio();
    pr_obj_t *o = objeto_nuevo(PR_TEXTO, 360, 70);
    if (!o) return;
    o->tam = 24;
    edit_obj = diap()->n - 1; edit_pos = 0;
    registrar("tucan: cuadro de texto nuevo\n");
}

static void imagen_nueva(void)
{
    char c[220] = "/home";
    if (!dialogo_abrir(_("Insertar imagen"), _("Imágenes (*.bmp;*.png;*.jpg;*.jpeg;*.gif)|*.bmp;*.png;*.jpg;*.jpeg;*.gif|Todos los archivos (*.*)|*"), c, c, sizeof c) || !c[0]) return;
    int n = 0;
    uint8_t *dat = carregar_arquivo(c, &n);
    if (!dat) { mensagem("Tucán", _("No se ha podido leer el archivo.")); return; }
    cambio();
    int m = pr_medio_anadir(dk, dat, n);
    libera(dat);
    if (m < 0) { mensagem("Tucán", _("No se ha podido leer la imagen.")); return; }
    const imagem_t *im = pr_medio_imagen(dk, m);
    int w = im ? im->larg : 300, h = im ? im->alt : 200;
    int maxw = dk->ancho * 60 / 100, maxh = dk->alto * 70 / 100;
    if (w > maxw) { h = h * maxw / w; w = maxw; }
    if (h > maxh) { w = w * maxh / h; h = maxh; }
    pr_obj_t *o = objeto_nuevo(PR_IMAGEN, w < 20 ? 20 : w, h < 20 ? 20 : h);
    if (o) o->medio = m;
    registrar("tucan: imagen insertada %dx%d\n", w, h);
}

static void wordart_pedir(pr_obj_t *o)
{
    char t[WORDART_TEXTO_MAX];
    copia_texto(t, o->texto && o->texto[0] ? o->texto : "Tucán", sizeof t);
    if (!pedir_texto(_("WordArt"), _("Texto:"), t, sizeof t) || !t[0]) return;
    char e[8];
    formatar_texto(e, sizeof e, "%d", o->wa_estilo);
    if (pedir_texto(_("WordArt"), _("Estilo (0 a 19):"), e, sizeof e)) { int v = texto_para_int(e); if (v >= 0 && v < WORDART_ESTILOS) o->wa_estilo = (uint8_t)v; }
    pr_obj_poner_texto(o, t);
}

static void wordart_nuevo(void)
{
    pr_obj_t tmp;
    memset(&tmp, 0, sizeof tmp);
    tmp.wa_estilo = 3;
    char t[WORDART_TEXTO_MAX] = "Tucán";
    if (!pedir_texto(_("WordArt"), _("Texto:"), t, sizeof t) || !t[0]) return;
    char e[8] = "3";
    if (pedir_texto(_("WordArt"), _("Estilo (0 a 19):"), e, sizeof e)) { int v = texto_para_int(e); if (v >= 0 && v < WORDART_ESTILOS) tmp.wa_estilo = (uint8_t)v; }
    cambio();
    pr_obj_t *o = objeto_nuevo(PR_WORDART, 520, 130);
    if (!o) return;
    o->wa_estilo = tmp.wa_estilo;
    pr_obj_poner_texto(o, t);
    registrar("tucan: wordart nuevo estilo %d\n", o->wa_estilo);
}

static void tabla_nueva(void)
{
    char t[16] = "3x3";
    if (!pedir_texto(_("Tabla"), _("Filas x columnas (por ejemplo 3x4):"), t, sizeof t)) return;
    int f = 0, c = 0, k = 0;
    while (t[k] >= '0' && t[k] <= '9') f = f * 10 + (t[k++] - '0');
    while (t[k] && (t[k] < '0' || t[k] > '9')) k++;
    while (t[k] >= '0' && t[k] <= '9') c = c * 10 + (t[k++] - '0');
    if (f < 1 || c < 1 || f > 20 || c > 10) { mensagem("Tucán", _("La tabla puede tener de 1 a 20 filas y de 1 a 10 columnas.")); return; }
    cambio();
    pr_obj_t *o = objeto_nuevo(PR_TABLA, dk->ancho * 70 / 100, f * 38);
    if (!o) return;
    o->filas = (uint8_t)f; o->cols = (uint8_t)c; o->tam = 18;
    char *tx = aloca((size_t)f * (size_t)c + 1);
    if (tx) { for (int i = 0; i < f * c - 1; i++) tx[i] = '\x1f'; tx[f * c - 1] = 0; pr_obj_poner_texto(o, tx); libera(tx); }
    edit_obj = diap()->n - 1; edit_pos = 0;
    registrar("tucan: tabla nueva %dx%d\n", f, c);
}

static void grafico_nuevo(int tipo)
{
    cambio();
    pr_obj_t *o = objeto_nuevo(PR_GRAFICO, 420, 300);
    if (!o) return;
    o->graf = (uint8_t)tipo;
    pr_obj_poner_texto(o, "Uno=30,Dos=45,Tres=25,Cuatro=40");
    registrar("tucan: grafico nuevo tipo %d\n", tipo);
}

static void grafico_editar(pr_obj_t *o)
{
    char t[200];
    copia_texto(t, o->texto ? o->texto : "", sizeof t);
    if (!pedir_texto(_("Gráfico"), _("Datos (etiqueta=valor, separados por comas):"), t, sizeof t)) return;
    pr_obj_poner_texto(o, t);
}

static void borrar_sel(void)
{
    pr_diap_t *s = diap();
    if (n_sel() == 0) return;
    cambio();
    for (int i = s->n - 1; i >= 0; i--) if (sel[i]) pr_obj_borrar(s, i);
    sel_limpiar(); edit_obj = -1;
    registrar("tucan: objetos borrados\n");
}

static void copiar_sel(bool cortar)
{
    pr_diap_t *s = diap();
    if (n_sel() == 0) return;
    for (int i = 0; i < clip_n; i++) pr_obj_liberar_campos(&clip[i]);
    clip_n = 0;
    for (int i = 0; i < s->n && clip_n < PR_OBJ_MAX; i++) if (sel[i]) { if (pr_obj_copiar(&clip[clip_n], &s->obj[i])) clip_n++; }
    if (clip_n && clip[0].texto && clip[0].texto[0] && clip[0].tipo != PR_TABLA) copiar_texto(clip[0].texto, (unsigned)tam_texto(clip[0].texto));
    if (cortar) borrar_sel();
    registrar("tucan: copiados %d objetos\n", clip_n);
}

static void pegar(void)
{
    pr_diap_t *s = diap();
    if (!clip_n) return;
    cambio();
    sel_limpiar();
    for (int i = 0; i < clip_n; i++) {
        pr_obj_t *o = pr_obj_nuevo(s, clip[i].tipo);
        if (!o) break;
        char *tx = o->texto;
        *o = clip[i];
        o->texto = tx;
        pr_obj_poner_texto(o, clip[i].texto ? clip[i].texto : "");
        o->x = (int16_t)(o->x + 14); o->y = (int16_t)(o->y + 14);
        sel[s->n - 1] = true;
    }
    pr_tocar(s);
    registrar("tucan: pegados %d objetos\n", clip_n);
}

static void sel_expandir_grupos(void)
{
    pr_diap_t *s = diap();
    for (int i = 0; i < s->n; i++)
        if (sel[i] && s->obj[i].grupo)
            for (int k = 0; k < s->n; k++) if (s->obj[k].grupo == s->obj[i].grupo) sel[k] = true;
}

static void orden(bool frente)
{
    pr_diap_t *s = diap();
    if (n_sel() == 0) return;
    cambio();
    bool nuevo[PR_OBJ_MAX] = { false };
    int mov = 0;
    if (frente) {
        for (int i = s->n - 1; i >= 0; i--) if (sel[i]) { pr_obj_orden(s, i + mov, s->n - 1); mov++; }
        int k = 0;
        for (int i = s->n - 1; i >= 0 && k < mov; i--, k++) nuevo[i] = true;
    } else {
        for (int i = 0; i < s->n; i++) if (sel[i]) { pr_obj_orden(s, i, mov); mov++; }
        for (int i = 0; i < mov; i++) nuevo[i] = true;
    }
    memcpy(sel, nuevo, sizeof sel);
    registrar("tucan: orden %s\n", frente ? "al frente" : "al fondo");
}

static void agrupar(bool unir)
{
    pr_diap_t *s = diap();
    if (n_sel() < (unir ? 2 : 1)) return;
    cambio();
    int g = 0;
    if (unir) {
        for (int i = 0; i < s->n; i++) if (s->obj[i].grupo > g) g = s->obj[i].grupo;
        g++;
    }
    for (int i = 0; i < s->n; i++) if (sel[i]) s->obj[i].grupo = (int16_t)g;
    pr_tocar(s);
    registrar("tucan: %s\n", unir ? "agrupados" : "desagrupados");
}

static void alinear(int modo)
{
    pr_diap_t *s = diap();
    int n = n_sel();
    if (!n) return;
    cambio();
    int x0 = 1 << 20, y0 = 1 << 20, x1 = -(1 << 20), y1 = -(1 << 20);
    if (n == 1) { x0 = 0; y0 = 0; x1 = dk->ancho; y1 = dk->alto; }
    else for (int i = 0; i < s->n; i++) if (sel[i]) {
        pr_obj_t *o = &s->obj[i];
        if (o->x < x0) x0 = o->x;
        if (o->y < y0) y0 = o->y;
        if (o->x + o->w > x1) x1 = o->x + o->w;
        if (o->y + o->h > y1) y1 = o->y + o->h;
    }
    for (int i = 0; i < s->n; i++) if (sel[i]) {
        pr_obj_t *o = &s->obj[i];
        switch (modo) {
        case 0: o->x = (int16_t)x0; break;
        case 1: o->x = (int16_t)((x0 + x1 - o->w) / 2); break;
        case 2: o->x = (int16_t)(x1 - o->w); break;
        case 3: o->y = (int16_t)y0; break;
        case 4: o->y = (int16_t)((y0 + y1 - o->h) / 2); break;
        case 5: o->y = (int16_t)(y1 - o->h); break;
        }
    }
    pr_tocar(s);
    registrar("tucan: alinear %d\n", modo);
}

/* Aplica un cambio a todos los objetos elegidos (o al que se edita). */
#define PARA_SEL(o) for (int _i = 0; _i < diap()->n; _i++) for (pr_obj_t *o = (sel[_i] || _i == edit_obj) ? &diap()->obj[_i] : NULL; o; o = NULL)

static void nivel_cambiar(int delta)
{
    pr_diap_t *s = diap();
    int cual = edit_obj >= 0 ? edit_obj : primer_sel();
    if (cual < 0 || cual >= s->n || !s->obj[cual].texto) return;
    if (s->obj[cual].tipo != PR_TEXTO && s->obj[cual].tipo != PR_FORMA) return;
    cambio();
    pr_obj_t *o = &s->obj[cual];
    const char *t = o->texto;
    int len = (int)tam_texto(t);
    /* con el cursor en el texto solo cambia ese párrafo; si no, todos */
    int pa = 0, pb = len;
    if (edit_obj == cual) {
        pa = edit_pos; while (pa > 0 && t[pa - 1] != '\n') pa--;
        pb = edit_pos; while (pb < len && t[pb] != '\n') pb++;
    }
    char *nt = aloca((size_t)len + 64 + 1);
    if (!nt) return;
    int k = 0, nuevo_pos = edit_pos;
    for (int i = 0; i <= len; ) {
        int fin = i; while (fin < len && t[fin] != '\n') fin++;
        bool en = i >= pa && i <= pb;
        int lvl = 0; while (i + lvl < fin && t[i + lvl] == '\t') lvl++;
        int nl = en ? lvl + delta : lvl;
        if (nl < 0) nl = 0;
        if (nl > 4) nl = 4;
        if (k + nl + (fin - i - lvl) + 2 > len + 64) break;
        for (int q = 0; q < nl; q++) nt[k++] = '\t';
        memcpy(nt + k, t + i + lvl, (size_t)(fin - i - lvl)); k += fin - i - lvl;
        if (edit_obj == cual && en) nuevo_pos += nl - lvl;
        if (fin < len) nt[k++] = '\n';
        i = fin + 1;
    }
    nt[k] = 0;
    pr_obj_poner_texto(o, nt);
    libera(nt);
    if (edit_obj == cual) edit_pos = nuevo_pos < 0 ? 0 : nuevo_pos > k ? k : nuevo_pos;
    pr_tocar(s);
    registrar("tucan: nivel %d\n", delta);
}

/* ------------------------------------------------------------ edición de texto en el lienzo */

static bool es_editable(const pr_obj_t *o) { return o->tipo == PR_TEXTO || o->tipo == PR_FORMA || o->tipo == PR_TABLA; }

static void editar_obj(int i, int pos)
{
    pr_diap_t *s = diap();
    if (i < 0 || i >= s->n || !es_editable(&s->obj[i])) return;
    if (edit_obj != i) undo_push();
    edit_obj = i;
    int len = (int)tam_texto(s->obj[i].texto);
    edit_pos = pos < 0 || pos > len ? len : pos;
    sel_limpiar(); sel[i] = true;
    notas.foco = false; foco_mini = false;
}

static void txt_insertar(pr_obj_t *o, int *pos, const char *ins, int n)
{
    int len = (int)tam_texto(o->texto);
    if (len + n > PR_TEXTO_MAX || n <= 0) return;
    char *nt = aloca((size_t)len + (size_t)n + 1);
    if (!nt) return;
    memcpy(nt, o->texto, (size_t)*pos);
    memcpy(nt + *pos, ins, (size_t)n);
    memcpy(nt + *pos + n, o->texto + *pos, (size_t)(len - *pos) + 1);
    libera(o->texto);
    o->texto = nt;
    *pos += n;
}

static void txt_borrar(pr_obj_t *o, int pos, int n)
{
    int len = (int)tam_texto(o->texto);
    if (pos < 0 || pos + n > len || n <= 0) return;
    memmove(o->texto + pos, o->texto + pos + n, (size_t)(len - pos - n) + 1);
}

static void edit_mover_linea(pr_obj_t *o, int dir)
{
    pr_comp_t c;
    if (!pr_componer(dk, o, o->texto, &c)) return;
    int x, y;
    pr_cursor_xy(dk, o, &c, edit_pos, &x, &y);
    int lh = o->tam * 120;
    int np = pr_pos_en(dk, o, &c, x, y + dir * lh + lh / 2 - PR_INSET_Y * 100 + PR_INSET_Y * 100);
    if (dir > 0 && y + lh >= c.l[c.n - 1].y + PR_INSET_Y * 100 + lh) np = (int)tam_texto(o->texto);
    if (dir < 0 && y <= PR_INSET_Y * 100) np = 0;
    edit_pos = np;
    pr_comp_liberar(&c);
}

/* true = la tecla se usó */
static bool edit_tecla(const evento_t *e)
{
    pr_diap_t *s = diap();
    if (edit_obj < 0 || edit_obj >= s->n) { edit_obj = -1; return false; }
    pr_obj_t *o = &s->obj[edit_obj];
    unsigned t = e->tecla;
    bool tabla = o->tipo == PR_TABLA, ctrl = (e->modificadores & 2) != 0, shift = (e->modificadores & 1) != 0;
    int len = (int)tam_texto(o->texto);
    if (t == TECLA_ESC) { edit_obj = -1; registrar("tucan: fin de la edicion\n"); return true; }
    if (ctrl) {
        if (t == 22) {   /* Ctrl+V */
            char b[400];
            int n = colar_texto(b, sizeof b);
            if (n > 0) { for (int i = 0; i < n; i++) { if (b[i] == '\r') b[i] = '\n'; if (tabla && (b[i] == '\n' || b[i] == '\x1f')) b[i] = ' '; } txt_insertar(o, &edit_pos, b, n); pr_tocar(s); modificado = true; }
            return true;
        }
        if (t == 1) return true;
        return false;
    }
    switch (t) {
    case TECLA_ESQUERDA:
        if (edit_pos > 0 && !(tabla && o->texto[edit_pos - 1] == '\x1f')) edit_pos--;
        else if (tabla && edit_pos > 0) edit_pos--;
        return true;
    case TECLA_DIREITA: if (edit_pos < len) edit_pos++; return true;
    case TECLA_HOME: while (edit_pos > 0 && o->texto[edit_pos - 1] != '\n' && !(tabla && o->texto[edit_pos - 1] == '\x1f')) edit_pos--; return true;
    case TECLA_END: while (edit_pos < len && o->texto[edit_pos] != '\n' && !(tabla && o->texto[edit_pos] == '\x1f')) edit_pos++; return true;
    case TECLA_CIMA:
        if (tabla) { int c = o->cols, p = edit_pos, cell = 0; for (int i = 0; i < p; i++) if (o->texto[i] == '\x1f') cell++; if (cell >= c) { int back = c; while (back > 0 && edit_pos > 0) { edit_pos--; if (o->texto[edit_pos] == '\x1f') back--; } while (edit_pos > 0 && o->texto[edit_pos - 1] != '\x1f') edit_pos--; } }
        else edit_mover_linea(o, -1);
        return true;
    case TECLA_BAIXO:
        if (tabla) { int c = o->cols, cell = 0; for (int i = 0; i < edit_pos; i++) if (o->texto[i] == '\x1f') cell++; if (cell + c < o->filas * c) { int adv = c; while (adv > 0 && edit_pos < len) { if (o->texto[edit_pos] == '\x1f') adv--; edit_pos++; } while (edit_pos < len && o->texto[edit_pos] != '\x1f') edit_pos++; } }
        else edit_mover_linea(o, 1);
        return true;
    case '\b':
        if (edit_pos > 0 && !(tabla && o->texto[edit_pos - 1] == '\x1f')) { txt_borrar(o, edit_pos - 1, 1); edit_pos--; pr_tocar(s); modificado = true; }
        return true;
    case TECLA_DELETE:
        if (edit_pos < len && !(tabla && o->texto[edit_pos] == '\x1f')) { txt_borrar(o, edit_pos, 1); pr_tocar(s); modificado = true; }
        return true;
    case '\t':
        if (tabla) {   /* siguiente celda (Mayús: la anterior) */
            if (shift) { while (edit_pos > 0 && o->texto[edit_pos - 1] != '\x1f') edit_pos--; if (edit_pos > 0) { edit_pos--; while (edit_pos > 0 && o->texto[edit_pos - 1] != '\x1f') edit_pos--; } }
            else { while (edit_pos < len && o->texto[edit_pos] != '\x1f') edit_pos++; if (edit_pos < len) edit_pos++; }
        } else nivel_cambiar(shift ? -1 : 1);
        return true;
    case '\n': case '\r':
        if (tabla) return true;
        txt_insertar(o, &edit_pos, "\n", 1);
        pr_tocar(s); modificado = true;
        return true;
    default:
        if (t >= 32 && t < 256 && t != 127) {
            char c = (char)t;
            if (tabla && c == '\x1f') return true;
            txt_insertar(o, &edit_pos, &c, 1);
            pr_tocar(s); modificado = true;
            return true;
        }
    }
    return false;
}

/* ------------------------------------------------------------ guardar, abrir y exportar */

static const char *const filtros_abrir = N_("Presentaciones (*.tcn;*.pptx)|*.tcn;*.pptx|Todos los archivos (*.*)|*");
static const char *const filtros_guardar = N_("Presentación de Tucán (*.tcn)|*.tcn|PowerPoint (*.pptx)|*.pptx");

static void quitar_ext(char *c)
{
    char *p = acha_ultimo_char(c, '.'), *b = acha_ultimo_char(c, '/');
    if (p && (!b || p > b)) *p = 0;
}

static bool guardar_en(const char *c)
{
    notas_a_diap();
    int rc = termina_en(c, ".pptx") ? pr_exportar_pptx(dk, c) : pr_guardar(dk, c);
    if (rc) { mensagem("Tucán", _("No se ha podido guardar la presentación.")); return false; }
    if (!termina_en(c, ".pptx")) { copia_texto(caminho, c, sizeof caminho); modificado = false; }
    registrar("tucan: guardado %s\n", c);
    return true;
}

static bool guardar_como(void)
{
    char c[220];
    copia_texto(c, caminho[0] ? caminho : "/home/presentacion", sizeof c);
    if (!dialogo_guardar(_("Guardar como"), _(filtros_guardar), c, c, sizeof c) || !c[0]) return false;
    return guardar_en(c);
}
static bool guardar(void) { return caminho[0] ? guardar_en(caminho) : guardar_como(); }

static bool pode_descartar(void)
{
    if (!modificado) return true;
    return confirmar("Tucán", _("La presentación tiene cambios sin guardar.\n¿Guardarlos antes de continuar?")) ? guardar() : true;
}

static void cargar(pr_deck_t *nd, const char *c)
{
    pr_liberar(dk);
    dk = nd;
    cur = 0; sel_limpiar(); edit_obj = -1;
    for (int i = 0; i < und_n; i++) libera(und[i]);
    for (int i = 0; i < red_n; i++) libera(red[i]);
    und_n = red_n = 0;
    copia_texto(caminho, c ? c : "", sizeof caminho);
    modificado = false;
    fondo_cambio();
    diap_a_notas();
}

static void abrir_ruta(const char *c)
{
    int err = 0;
    pr_deck_t *nd = pr_abrir(c, &err);
    if (!nd) { mensagem("Tucán", _("No se ha podido abrir el archivo.")); return; }
    bool pptx = termina_en(c, ".pptx");
    cargar(nd, pptx ? "" : c);
    if (pptx) modificado = true;
    registrar("tucan: abierto %s (%d diapositivas)\n", c, dk->n);
    if (pr_avisos[0]) {
        char m[300];
        formatar_texto(m, sizeof m, _("Importado con avisos: %s"), pr_avisos);
        mensagem("Tucán", m);
    }
}

static void exportar(int id)
{
    notas_a_diap();
    char c[220];
    copia_texto(c, caminho[0] ? caminho : "/home/presentacion", sizeof c);
    quitar_ext(c);
    const char *ext = id == A_PDF || id == A_PDF_NOTAS ? ".pdf" : id == A_PPTX_EXP ? ".pptx" : ".png";
    size_t l = tam_texto(c);
    if (id == A_PNG_TODAS) copia_texto(c + l, "-1.png", sizeof c - l); else copia_texto(c + l, ext, sizeof c - l);
    const char *filtro = id == A_PPTX_EXP ? _("PowerPoint (*.pptx)|*.pptx") : ext[1] == 'p' && ext[2] == 'd' ? _("Documento PDF (*.pdf)|*.pdf") : _("Imagen PNG (*.png)|*.png");
    if (!dialogo_guardar(_("Exportar"), filtro, c, c, sizeof c) || !c[0]) return;
    int rc;
    if (id == A_PDF || id == A_PDF_NOTAS) rc = pr_exportar_pdf(dk, c, id == A_PDF_NOTAS);
    else if (id == A_PPTX_EXP) rc = pr_exportar_pptx(dk, c);
    else if (id == A_PNG_ACTUAL) rc = pr_exportar_png(dk, cur, c, 1280);
    else {   /* todas: nombre-N.png */
        char base[220];
        copia_texto(base, c, sizeof base);
        char *p = acha_ultimo_char(base, '-');
        char *b = acha_ultimo_char(base, '/');
        if (p && (!b || p > b)) *p = 0; else quitar_ext(base);
        rc = 0;
        for (int i = 0; i < dk->n && !rc; i++) {
            char f[260];
            formatar_texto(f, sizeof f, "%s-%d.png", base, i + 1);
            rc = pr_exportar_png(dk, i, f, 1280);
        }
    }
    if (rc) mensagem("Tucán", _("No se ha podido exportar."));
    else { char m[300]; formatar_texto(m, sizeof m, _("Exportado: %s"), c); mensagem("Tucán", m); }
    registrar("tucan: exportado %s rc=%d\n", c, rc);
}

#include "tucan_agente.h"
#include "tucan_pres.h"

/* ------------------------------------------------------------ órdenes */

static void aplicar_color_texto(uint32_t c) { cambio(); PARA_SEL(o) o->col = c; pr_tocar(diap()); }

static void accion(int id)
{
    pr_diap_t *s = diap();
    cinta_ctrl_t *k = cinta_buscar(&cinta, id);
    if (id >= A_LAYOUT0 && id < A_LAYOUT0 + PR_LY_N) { insertar_diap(id - A_LAYOUT0); return; }
    if (id >= A_APLICAR0 && id < A_APLICAR0 + PR_LY_N) { cambio(); pr_aplicar_layout(dk, s, id - A_APLICAR0); sel_limpiar(); edit_obj = -1; return; }
    if (id >= A_FORMA0 && id < A_FORMA0 + PF_N) { forma_nueva(id - A_FORMA0); return; }
    if (id >= A_ALINEAR0 && id < A_ALINEAR0 + 6) { alinear(id - A_ALINEAR0); return; }
    if (id >= A_GRAF0 && id < A_GRAF0 + PR_GRAF_N) { grafico_nuevo(id - A_GRAF0); return; }
    switch (id) {
    case A_NUEVO:
        if (!pode_descartar()) return;
        { pr_deck_t *nd = pr_nueva(true); if (nd) { cargar(nd, ""); registrar("tucan: nueva presentacion\n"); } }
        break;
    case A_ABRIR: {
        if (!pode_descartar()) return;
        char c[220];
        copia_texto(c, caminho[0] ? caminho : "/home", sizeof c);
        if (!dialogo_abrir(_("Abrir"), _(filtros_abrir), c, c, sizeof c) || !c[0]) break;
        abrir_ruta(c);
        break;
    }
    case A_GUARDAR: guardar(); break;
    case A_GUARDAR_COMO: guardar_como(); break;
    case A_PDF: case A_PDF_NOTAS: case A_PPTX_EXP: case A_PNG_ACTUAL: case A_PNG_TODAS: exportar(id); break;
    case A_SALIR: if (pode_descartar()) sair_pedido = true; break;
    case A_DESHACER: deshacer(); break;
    case A_REHACER: rehacer(); break;
    case A_CORTAR: copiar_sel(true); break;
    case A_COPIAR: copiar_sel(false); break;
    case A_PEGAR: pegar(); break;
    case A_BORRAR_OBJ: borrar_sel(); break;
    case A_TODO: sel_limpiar(); for (int i = 0; i < s->n; i++) sel[i] = true; break;
    case A_DIAP_NUEVA: insertar_diap(s->layout == PR_LY_TITULO || s->layout == PR_LY_BLANCO ? PR_LY_TITULO_CONTENIDO : s->layout); break;
    case A_DIAP_DUP:
        notas_a_diap(); cambio();
        if (pr_diap_duplicar(dk, cur)) { int a = cur; cur = -1; ir_a(a + 1); registrar("tucan: diapositiva duplicada\n"); }
        break;
    case A_DIAP_BORRAR:
        if (dk->n <= 1) break;
        cambio();
        pr_diap_borrar(dk, cur);
        if (cur >= dk->n) cur = dk->n - 1;
        sel_limpiar(); edit_obj = -1; diap_a_notas(); log_diap();
        break;
    case A_DIAP_OCULTAR: cambio(); s->oculta = !s->oculta; pr_tocar(s); registrar("tucan: diapositiva %s\n", s->oculta ? "oculta" : "visible"); break;
    case A_DIAP_SUBIR: if (cur > 0) { cambio(); pr_diap_mover(dk, cur, cur - 1); cur--; } break;
    case A_DIAP_BAJAR: if (cur + 1 < dk->n) { cambio(); pr_diap_mover(dk, cur, cur + 1); cur++; } break;
    case A_NEGRITA: cambio(); PARA_SEL(o) o->est ^= PR_NEGRITA; pr_tocar(s); break;
    case A_CURSIVA: cambio(); PARA_SEL(o) o->est ^= PR_CURSIVA; pr_tocar(s); break;
    case A_SUBRAYADO: cambio(); PARA_SEL(o) o->est ^= PR_SUBRAYADO; pr_tocar(s); break;
    case A_ALIN_IZQ: cambio(); PARA_SEL(o) o->alin = 0; pr_tocar(s); break;
    case A_ALIN_CEN: cambio(); PARA_SEL(o) o->alin = 1; pr_tocar(s); break;
    case A_ALIN_DER: cambio(); PARA_SEL(o) o->alin = 2; pr_tocar(s); break;
    case A_VINETAS: cambio(); PARA_SEL(o) o->lista = o->lista == PR_LISTA_VINETA ? PR_LISTA_NADA : PR_LISTA_VINETA; pr_tocar(s); break;
    case A_NUMERACION: cambio(); PARA_SEL(o) o->lista = o->lista == PR_LISTA_NUMERO ? PR_LISTA_NADA : PR_LISTA_NUMERO; pr_tocar(s); break;
    case A_NIVEL_MAS: nivel_cambiar(1); break;
    case A_NIVEL_MENOS: nivel_cambiar(-1); break;
    case A_TEXTO_NUEVO: texto_nuevo(); break;
    case A_IMAGEN: imagen_nueva(); break;
    case A_WORDART: wordart_nuevo(); break;
    case A_WORDART_EDITAR: { int i = primer_sel(); if (i >= 0 && s->obj[i].tipo == PR_WORDART) { cambio(); wordart_pedir(&s->obj[i]); pr_tocar(s); } break; }
    case A_TABLA: tabla_nueva(); break;
    case A_NUM_DIAP: cambio(); dk->num = !dk->num; fondo_cambio(); break;
    case A_PIE: {
        char t[80];
        copia_texto(t, dk->pie, sizeof t);
        if (pedir_texto(_("Pie de página"), _("Texto al pie de las diapositivas:"), t, sizeof t)) { cambio(); copia_texto(dk->pie, t, sizeof dk->pie); dk->con_pie = t[0] != 0; fondo_cambio(); }
        break;
    }
    case A_FRENTE: orden(true); break;
    case A_ATRAS: orden(false); break;
    case A_AGRUPAR: agrupar(true); break;
    case A_DESAGRUPAR: agrupar(false); break;
    case A_TAM_169: cambio(); pr_tamano(dk, true); fondo_cambio(); layout(); lienzo_idx = -1; break;
    case A_TAM_43: cambio(); pr_tamano(dk, false); fondo_cambio(); layout(); lienzo_idx = -1; break;
    case A_C_TEMA: if (k) { cambio(); pr_aplicar_tema(dk, k->sel); fondo_cambio(); registrar("tucan: tema %s\n", pr_temas[dk->tema].nombre); } break;
    case A_FONDO_TEMA: cambio(); s->fondo_tipo = PR_FONDO_TEMA; pr_tocar(s); break;
    case A_FONDO_COLOR: {
        uint32_t c = s->fondo1;
        if (dialogo_cor(_("Color de fondo"), &c)) { cambio(); s->fondo_tipo = PR_FONDO_COLOR; s->fondo1 = c; pr_tocar(s); }
        break;
    }
    case A_FONDO_DEGRADADO: {
        uint32_t c1 = s->fondo1, c2 = s->fondo2;
        if (dialogo_cor(_("Color de arriba"), &c1) && dialogo_cor(_("Color de abajo"), &c2)) { cambio(); s->fondo_tipo = PR_FONDO_DEGRADADO; s->fondo1 = c1; s->fondo2 = c2; pr_tocar(s); }
        break;
    }
    case A_FONDO_IMAGEN: {
        char c[220] = "/home";
        if (!dialogo_abrir(_("Imagen de fondo"), _("Imágenes (*.bmp;*.png;*.jpg;*.jpeg;*.gif)|*.bmp;*.png;*.jpg;*.jpeg;*.gif"), c, c, sizeof c) || !c[0]) break;
        int n = 0;
        uint8_t *dat = carregar_arquivo(c, &n);
        if (!dat) break;
        cambio();
        int m = pr_medio_anadir(dk, dat, n);
        libera(dat);
        if (m >= 0) { s->fondo_tipo = PR_FONDO_IMAGEN; s->fondo_medio = m; pr_tocar(s); }
        break;
    }
    case A_TRANS_TODAS: cambio(); for (int i = 0; i < dk->n; i++) { dk->d[i]->trans = s->trans; dk->d[i]->dur_trans = s->dur_trans; dk->d[i]->auto_seg = s->auto_seg; } break;
    case A_C_TRANS: if (k) { cambio(); s->trans = (uint8_t)k->sel; pr_tocar(s); registrar("tucan: transicion %d\n", s->trans); } break;
    case A_C_ANIM: if (k) { cambio(); PARA_SEL(o) o->anim = (uint8_t)k->sel; pr_tocar(s); registrar("tucan: animacion %d\n", k->sel); } break;
    case A_C_DUR: if (k) {
        int q = 5;
        if (compara_texto(k->cb.texto, durs_txt[0]) == 0) q = 3; else if (compara_texto(k->cb.texto, durs_txt[2]) == 0) q = 10; else if (compara_texto(k->cb.texto, durs_txt[3]) == 0) q = 20;
        cambio(); s->dur_trans = (uint8_t)q; }
        break;
    case A_C_AVANZAR: if (k) {
        int q = 0;
        for (int i = 1; i < 5; i++) if (compara_texto(k->cb.texto, avanzar_txt[i]) == 0) { static const int v[5] = { 0, 3, 5, 10, 30 }; q = v[i]; }
        cambio(); s->auto_seg = q; }
        break;
    case A_AUTO: cambio(); dk->auto_avance = !dk->auto_avance; break;
    case A_LASER: laser_opc = !laser_opc; break;
    case A_NOTAS: notas_vis = !notas_vis; layout(); break;
    case A_MINIATURAS: mini_vis = !mini_vis; layout(); break;
    case A_ASISTENTE: ag_alternar(); layout(); break;
    case A_PRESENTAR: presentar(0); return;
    case A_PRESENTAR_ACTUAL: presentar(cur); return;
    case A_AYUDA:
        mensagem("Tucán", _("Atajos: Ctrl+M diapositiva nueva, Ctrl+D duplicar, Ctrl+Z/Y deshacer y rehacer, Ctrl+C/X/V copiar, cortar y pegar objetos, Ctrl+G agrupar, F5 presentar, Mayús+F5 desde la actual. Doble clic en un texto para escribir; Tab baja el nivel de un párrafo. En la presentación: flechas, clic o rueda para avanzar; B pantalla negra, L láser, número+Intro ir a una diapositiva, Esc salir."));
        break;
    case A_ACERCA: mensagem(_("Acerca de Tucán"), _("Tucán - presentaciones de SaguiOS (D369).\nFormatos: .tcn, .pptx, PDF y PNG.")); break;
    case A_C_TAM: if (k) { int v = texto_para_int(k->cb.texto); if (v >= 1 && v <= 250) { cambio(); PARA_SEL(o) o->tam = (uint8_t)v; pr_tocar(s); } } break;
    case A_C_FONTE: if (k) { cambio(); PARA_SEL(o) copia_texto(o->fuente, pdfw_nombre_familia(k->cf.fam), sizeof o->fuente); pr_tocar(s); registrar("tucan: fuente %s\n", pdfw_nombre_familia(k->cf.fam)); } break;
    case A_C_COR: if (k) aplicar_color_texto(k->cor); break;
    case A_C_RELLENO: if (k) { cambio(); PARA_SEL(o) { o->relleno = k->cor; o->rellena = 1; } pr_tocar(s); } break;
    case A_C_BORDE: if (k) { cambio(); PARA_SEL(o) { o->borde_col = k->cor; if (!o->borde) o->borde = 2; } pr_tocar(s); } break;
    case A_C_GROSOR: if (k) { int v = texto_para_int(k->cb.texto); if (v >= 0 && v <= 40) { cambio(); PARA_SEL(o) o->borde = (uint8_t)v; pr_tocar(s); } } break;
    case A_C_FTIT: if (k) { cambio(); copia_texto(dk->f_titulo, pdfw_nombre_familia(k->cf.fam), sizeof dk->f_titulo); fondo_cambio(); } break;
    case A_C_FCUE: if (k) { cambio(); copia_texto(dk->f_cuerpo, pdfw_nombre_familia(k->cf.fam), sizeof dk->f_cuerpo); fondo_cambio(); } break;
    default: break;
    }
}

/* ------------------------------------------------------------ ratón */

static int objeto_en(int mx, int my)
{
    pr_diap_t *s = diap();
    for (int i = s->n - 1; i >= 0; i--) {
        int x, y, w, h;
        obj_rect_px(&s->obj[i], 0, 0, &x, &y, &w, &h);
        int tol = (s->obj[i].tipo == PR_FORMA && s->obj[i].forma == PF_LINEA) ? 5 : 0;
        if (mx >= x - tol && mx < x + w + tol && my >= y - tol && my < y + h + tol) return i;
    }
    return -1;
}

static int manija_en(int mx, int my)
{
    int bx, by, bw, bh;
    if (n_sel() != 1 || !caja_sel(&bx, &by, &bw, &bh)) return -1;
    int hx[8], hy[8];
    manijas(bx, by, bw, bh, hx, hy);
    for (int k = 0; k < 8; k++) if (mx >= hx[k] - 5 && mx <= hx[k] + 5 && my >= hy[k] - 5 && my <= hy[k] + 5) return k;
    return -1;
}

static int mini_en(int mx, int my)
{
    if (!mini_vis || mx < r_mini_x || mx >= r_mini_x + r_mini_w || my < r_mini_y || my >= r_mini_y + r_mini_h) return -2;
    int i = (my - r_mini_y - 6 + mini_top + 4) / MINI_FILA;
    if (my - r_mini_y - 6 + mini_top + 4 < 0) return -1;
    return i >= 0 && i < dk->n ? i : -1;
}

static void menu_diap(int x, int y)
{
    menu_contextual(j->id, x, y, MENU_("Diapositiva nueva\nDuplicar diapositiva\nEliminar diapositiva\nOcultar o mostrar\n-\nSubir\nBajar"), 7001);
}
static void menu_objeto(int x, int y)
{
    menu_contextual(j->id, x, y, MENU_("Cortar\nCopiar\nPegar\n-\nTraer al frente\nEnviar al fondo\n-\nEliminar"), 7002);
}

static void aplicar_arrastre(void)
{
    pr_diap_t *s = diap();
    if (drag == DR_MOVER && (drag_dx || drag_dy)) {
        cambio();
        for (int i = 0; i < s->n; i++) if (sel[i]) { s->obj[i].x = (int16_t)(s->obj[i].x + drag_dx * dk->ancho / sw); s->obj[i].y = (int16_t)(s->obj[i].y + drag_dy * dk->alto / sh); }
        pr_tocar(s);
        registrar("tucan: objetos movidos\n");
    } else if (drag == DR_REDIM) {
        int i = primer_sel();
        if (i >= 0 && (drag_dx || drag_dy)) {
            pr_obj_t *o = &s->obj[i];
            cambio();
            int dx = drag_dx * dk->ancho / sw, dy = drag_dy * dk->alto / sh;
            int x = drag_orig[i][0], y = drag_orig[i][1], w = drag_orig[i][2], h = drag_orig[i][3];
            int hh = drag_h;
            bool aspecto = o->tipo == PR_IMAGEN && (hh == 0 || hh == 2 || hh == 4 || hh == 6);
            if (hh == 0 || hh == 6 || hh == 7) { x += dx; w -= dx; }
            if (hh == 2 || hh == 3 || hh == 4) w += dx;
            if (hh == 0 || hh == 1 || hh == 2) { y += dy; h -= dy; }
            if (hh == 4 || hh == 5 || hh == 6) h += dy;
            if (aspecto && drag_orig[i][2] > 0) { int nh = w * drag_orig[i][3] / drag_orig[i][2]; if (hh == 0 || hh == 2) y += h - nh; h = nh; }
            if (w < 8) w = 8;
            if (h < 4) h = 4;
            o->x = (int16_t)x; o->y = (int16_t)y; o->w = (int16_t)w; o->h = (int16_t)h;
            pr_tocar(s);
            registrar("tucan: objeto redimensionado %dx%d\n", w, h);
        }
    } else if (drag == DR_MARCO) {
        int x0 = drag_x0 < drag_mx ? drag_x0 : drag_mx, x1 = drag_x0 < drag_mx ? drag_mx : drag_x0;
        int y0 = drag_y0 < drag_my ? drag_y0 : drag_my, y1 = drag_y0 < drag_my ? drag_my : drag_y0;
        sel_limpiar();
        for (int i = 0; i < s->n; i++) {
            int x, y, w, h;
            obj_rect_px(&s->obj[i], 0, 0, &x, &y, &w, &h);
            if (x < x1 && x + w > x0 && y < y1 && y + h > y0) sel[i] = true;
        }
        sel_expandir_grupos();
    }
}

static void raton_clic(const evento_t *e)
{
    int mx = e->x, my = e->y;
    bool shift = (e->modificadores & 1) != 0;
    int mi = mini_en(mx, my);
    if (mi != -2) {   /* lista de miniaturas */
        foco_mini = true; notas.foco = false; edit_obj = -1;
        if (e->botoes & 2) { if (mi >= 0) ir_a(mi); menu_diap(mx, my); return; }
        if (mi >= 0) { ir_a(mi); drag = DR_MINI; drag_ini = mi; drag_x0 = mx; drag_y0 = my; mini_destino = -1; }
        return;
    }
    if (notas_vis && mx >= r_not_x && mx < r_not_x + r_not_w && my >= r_not_y && my < r_not_y + r_not_h) {
        notas.foco = true; foco_mini = false; edit_obj = -1;
        if (area_tratar(j, &notas, e)) notas_sucia = true;
        return;
    }
    if (mx < r_cv_x || mx >= r_cv_x + r_cv_w || my < r_cv_y || my >= r_cv_y + r_cv_h) return;
    notas.foco = false; foco_mini = false;
    pr_diap_t *s = diap();
    if (edit_obj >= 0) {   /* clic dentro del objeto en edición: mueve el cursor; fuera: termina */
        pr_obj_t *o = &s->obj[edit_obj];
        int x, y, w, h;
        obj_rect_px(o, 0, 0, &x, &y, &w, &h);
        if (mx >= x && mx < x + w && my >= y && my < y + h) {
            pr_comp_t c;
            if (pr_componer(dk, o, o->texto, &c)) {
                int rx = (mx - x) * dk->ancho * 100 / sw, ry = (my - y) * dk->alto * 100 / sh;
                edit_pos = pr_pos_en(dk, o, &c, rx, ry);
                pr_comp_liberar(&c);
            }
            return;
        }
        edit_obj = -1;
    }
    int mh = manija_en(mx, my);
    if (mh >= 0 && (e->botoes & 1)) {
        drag = DR_REDIM; drag_h = mh; drag_x0 = mx; drag_y0 = my; drag_dx = drag_dy = 0;
        for (int i = 0; i < s->n; i++) { drag_orig[i][0] = s->obj[i].x; drag_orig[i][1] = s->obj[i].y; drag_orig[i][2] = s->obj[i].w; drag_orig[i][3] = s->obj[i].h; }
        return;
    }
    int i = objeto_en(mx, my);
    if (e->botoes & 2) {
        if (i >= 0 && !sel[i]) { sel_limpiar(); sel[i] = true; sel_expandir_grupos(); }
        menu_objeto(mx, my);
        return;
    }
    if (i < 0) {
        if (!shift) sel_limpiar();
        drag = DR_MARCO; drag_x0 = drag_mx = mx; drag_y0 = drag_my = my;
        return;
    }
    if (e->cliques >= 2) {   /* doble clic: editar */
        pr_obj_t *o = &s->obj[i];
        if (es_editable(o)) {
            pr_comp_t c; int pos = -1;
            int x, y, w, h;
            obj_rect_px(o, 0, 0, &x, &y, &w, &h);
            if (o->tipo != PR_TABLA && pr_componer(dk, o, o->texto, &c)) { pos = pr_pos_en(dk, o, &c, (mx - x) * dk->ancho * 100 / sw, (my - y) * dk->alto * 100 / sh); pr_comp_liberar(&c); }
            if (o->tipo == PR_TABLA) {   /* la celda bajo el ratón */
                int col = (mx - x) * o->cols / w, fila = (my - y) * o->filas / h, cell = fila * o->cols + col, p = 0;
                for (int q = 0; q < cell && o->texto[p]; p++) if (o->texto[p] == '\x1f') q++;
                if (col >= o->cols) col = o->cols - 1;
                pos = p;
                while (o->texto[pos] && o->texto[pos] != '\x1f') pos++;
            }
            editar_obj(i, pos);
        } else if (o->tipo == PR_WORDART) { cambio(); wordart_pedir(o); pr_tocar(s); }
        else if (o->tipo == PR_GRAFICO) { cambio(); grafico_editar(o); pr_tocar(s); }
        return;
    }
    bool ya = sel[i];
    if (shift) sel[i] = !sel[i];
    else if (!ya) { sel_limpiar(); sel[i] = true; }
    sel_expandir_grupos();
    if (!sel[i]) return;
    drag = DR_MOVER; drag_x0 = mx; drag_y0 = my; drag_dx = drag_dy = 0;
    for (int q = 0; q < s->n; q++) { drag_orig[q][0] = s->obj[q].x; drag_orig[q][1] = s->obj[q].y; drag_orig[q][2] = s->obj[q].w; drag_orig[q][3] = s->obj[q].h; }
    registrar("tucan: objeto elegido %d\n", i);
}

static void raton_mover(const evento_t *e)
{
    if (drag == DR_MOVER || drag == DR_REDIM) {
        drag_dx = e->x - drag_x0; drag_dy = e->y - drag_y0;
        if (drag == DR_REDIM) {   /* vista previa: el objeto cambia ya, sin guardar */
            pr_diap_t *s = diap();
            int i = primer_sel();
            if (i >= 0) {
                pr_obj_t *o = &s->obj[i];
                int dx = drag_dx * dk->ancho / sw, dy = drag_dy * dk->alto / sh;
                int x = drag_orig[i][0], y = drag_orig[i][1], w = drag_orig[i][2], h = drag_orig[i][3], hh = drag_h;
                bool aspecto = o->tipo == PR_IMAGEN && (hh == 0 || hh == 2 || hh == 4 || hh == 6);
                if (hh == 0 || hh == 6 || hh == 7) { x += dx; w -= dx; }
                if (hh == 2 || hh == 3 || hh == 4) w += dx;
                if (hh == 0 || hh == 1 || hh == 2) { y += dy; h -= dy; }
                if (hh == 4 || hh == 5 || hh == 6) h += dy;
                if (aspecto && drag_orig[i][2] > 0) { int nh = w * drag_orig[i][3] / drag_orig[i][2]; if (hh == 0 || hh == 2) y += h - nh; h = nh; }
                if (w < 8) w = 8;
                if (h < 4) h = 4;
                int bx = px_x(x), by = px_y(y), bw = w * sw / dk->ancho, bh = h * sh / dk->alto;
                dibujar_lienzo();
                marco(bx, by, bw, bh, 0xE53935);
                jan_mostrar_area(j, r_cv_x, r_cv_y, r_cv_w, r_cv_h);
                return;
            }
        }
        redibujar_lienzo();
    } else if (drag == DR_MARCO) {
        drag_mx = e->x; drag_my = e->y;
        redibujar_lienzo();
    } else if (drag == DR_MINI) {
        int d = (e->y - r_mini_y - 6 + mini_top + MINI_FILA / 2) / MINI_FILA;
        if (d < 0) d = 0;
        if (d > dk->n) d = dk->n;
        if (e->y - drag_y0 > 4 || drag_y0 - e->y > 4) mini_destino = d;
        redibujar_todo();
    }
}

static void raton_soltar(const evento_t *e)
{
    (void)e;
    if (drag == DR_MINI) {
        if (mini_destino >= 0) {
            int a = mini_destino > drag_ini ? mini_destino - 1 : mini_destino;
            if (a != drag_ini) { notas_a_diap(); cambio(); pr_diap_mover(dk, drag_ini, a); cur = a; registrar("tucan: diapositiva movida de %d a %d\n", drag_ini + 1, a + 1); }
        }
        mini_destino = -1; drag = DR_NADA;
        redibujar_todo();
        return;
    }
    if (drag != DR_NADA) {
        aplicar_arrastre();
        drag = DR_NADA;
        redibujar_todo();
    }
}

/* ------------------------------------------------------------ teclado */

static void atajo_ctrl(unsigned t, bool *usada)
{
    *usada = true;
    switch (t) {
    case 14: accion(A_NUEVO); break;
    case 15: accion(A_ABRIR); break;
    case 19: accion(A_GUARDAR); break;
    case 13: accion(A_DIAP_NUEVA); break;
    case 4: accion(A_DIAP_DUP); break;
    case 26: deshacer(); break;
    case 25: rehacer(); break;
    case 7: accion(A_AGRUPAR); break;
    case 2: accion(A_NEGRITA); break;
    case 9: accion(A_CURSIVA); break;
    case 21: accion(A_SUBRAYADO); break;
    default: *usada = false;
    }
    if (*usada) return;
    if (notas.foco) return;
    *usada = true;
    switch (t) {
    case 3: accion(A_COPIAR); break;
    case 24: accion(A_CORTAR); break;
    case 22: accion(A_PEGAR); break;
    case 1: accion(A_TODO); break;
    default: *usada = false;
    }
}

static bool teclado_lienzo(const evento_t *e)
{
    pr_diap_t *s = diap();
    unsigned t = e->tecla;
    bool shift = (e->modificadores & 1) != 0;
    int paso = shift ? 10 : 1;
    switch (t) {
    case TECLA_PGUP: ir_a(cur - 1); return true;
    case TECLA_PGDN: ir_a(cur + 1); return true;
    case TECLA_HOME: ir_a(0); return true;
    case TECLA_END: ir_a(dk->n - 1); return true;
    case TECLA_DELETE: if (foco_mini && n_sel() == 0) accion(A_DIAP_BORRAR); else borrar_sel(); return true;
    case '\b': if (n_sel()) borrar_sel(); return true;
    case TECLA_ESC: sel_limpiar(); return true;
    case TK_F5: presentar(shift ? cur : 0); return true;
    case TK_F1: accion(A_AYUDA); return true;
    case TECLA_F12: accion(A_ASISTENTE); return true;
    case TECLA_CIMA: case TECLA_BAIXO: case TECLA_ESQUERDA: case TECLA_DIREITA:
        if (n_sel() == 0 || foco_mini) { ir_a(cur + (t == TECLA_CIMA || t == TECLA_ESQUERDA ? -1 : 1)); return true; }
        cambio();
        for (int i = 0; i < s->n; i++) if (sel[i]) {
            if (t == TECLA_CIMA) s->obj[i].y = (int16_t)(s->obj[i].y - paso);
            if (t == TECLA_BAIXO) s->obj[i].y = (int16_t)(s->obj[i].y + paso);
            if (t == TECLA_ESQUERDA) s->obj[i].x = (int16_t)(s->obj[i].x - paso);
            if (t == TECLA_DIREITA) s->obj[i].x = (int16_t)(s->obj[i].x + paso);
        }
        pr_tocar(s);
        return true;
    case '\t': {   /* siguiente objeto */
        if (!s->n) return true;
        int i = primer_sel();
        int n = shift ? (i <= 0 ? s->n - 1 : i - 1) : (i + 1) % s->n;
        sel_limpiar(); sel[n] = true;
        return true;
    }
    case '\n': case '\r': {   /* Intro: edita el objeto elegido */
        int i = primer_sel();
        if (i >= 0 && es_editable(&s->obj[i])) editar_obj(i, -1);
        return true;
    }
    default:
        if (t >= 32 && t < 256 && t != 127 && !(e->modificadores & 6)) {   /* escribir con un objeto de texto elegido empieza a editar */
            int i = primer_sel();
            if (i >= 0 && n_sel() == 1 && (s->obj[i].tipo == PR_TEXTO || s->obj[i].tipo == PR_FORMA)) { editar_obj(i, -1); edit_tecla(e); return true; }
        }
    }
    return false;
}

/* ------------------------------------------------------------ programa */

static void cinta_orden(int id)
{
    cinta_ctrl_t *k = cinta_buscar(&cinta, id);
    if (k && (k->tipo == CC_COMBO && id == A_C_TAM)) { /* el combo de tamaño: se aplica en accion() */ }
    accion(id);
    if (!pres) redibujar_todo();
}

int principal(int argc, char **argv)
{
    ag_app_opciones(&argc, argv);          /* --url y --modelo del asistente (pruebas) */
    wordart_fuentes_instalar();
    fuentes_doc_instalar();
    for (int i = 0; i < pr_n_temas(); i++) lista_temas[i] = pr_temas[i].nombre;
    pr_hints[0] = _("Clic para agregar un título");
    pr_hints[1] = _("Clic para agregar texto");
    pr_hints[2] = _("Clic para agregar un subtítulo");
    if (argc >= 4 && compara_texto(argv[1], "--convertir") == 0) {
        bool notas_pdf = false;
        int diap = 0, ancho = 1280;
        for (int i = 4; i < argc; i++) {
            if (compara_texto(argv[i], "--notas") == 0) notas_pdf = true;
            else if (compara_texto(argv[i], "--diap") == 0 && i + 1 < argc) diap = num_arg(argv[++i]) - 1;
            else if (compara_texto(argv[i], "--ancho") == 0 && i + 1 < argc) ancho = num_arg(argv[++i]);
        }
        return modo_convertir(argv[2], argv[3], notas_pdf, diap < 0 ? 0 : diap, ancho);
    }
    if (argc >= 3 && compara_texto(argv[1], "--info") == 0) return modo_info(argv[2]);
    if (argc >= 3 && compara_texto(argv[1], "--demo") == 0) return modo_demo(argv[2]);
    if (argc >= 2 && compara_texto(argv[1], "--fuentes") == 0) return modo_fuentes();
    if (argc >= 2 && compara_texto(argv[1], "--bench") == 0) return modo_bench();

    while (argc >= 2 && argv[1][0] == '-' && argv[1][1] == '-') {
        if (compara_texto(argv[1], "--cinta-log") == 0) cinta_log = true;
        else break;
        argv++; argc--;
    }
    int jl = 980, ja = 680;
    cinta_tam_janela(&jl, &ja);
    j = jan_abrir(_("Presentación1 - Tucán"), jl, ja);
    if (!j) return 1;
    jan_estilo(j, JAN_REDIMENSIONAVEL | JAN_TELA_CHEIA_OK | JAN_ESC_LIVRE, 640, 440);
    dk = pr_nueva(true);
    if (!dk) return 1;
    ag_registrar_herramientas();
    /* los menús desplegables de la cinta (traducidos al abrir) */
    cc_diapositivas[0].menu = MENU_(menu_layout);
    cc_diapositivas[1].menu = MENU_(menu_layout);
    cc_dibujo[0].menu = MENU_(menu_formas);
    cc_ins_basicos[2].menu = MENU_(menu_formas);
    cc_organizar[4].menu = MENU_(menu_alinear);
    cc_ins_objetos[1].menu = MENU_(menu_graf);
    cinta.acceso = cc_acceso; cinta.n_acceso = 3;
    cinta_iniciar(&cinta, 0, 0, j->larg, cp_tucan, N_PEST);
    cinta.archivo = MENU_(archivo_menu);
    cinta.archivo_ids = archivo_ids;
    cinta.depurar = cinta_log;
    cinta_traducir(&cinta);
    cinta_buscar(&cinta, A_C_COR)->cor = 0x000000;
    cinta_buscar(&cinta, A_C_RELLENO)->cor = 0x2E75B6;
    cinta_buscar(&cinta, A_C_BORDE)->cor = 0x1F3864;
    area_iniciar(&notas, 0, 0, 100, 60);
    if (argc >= 2) {
        int err = 0;
        pr_deck_t *nd = pr_abrir(argv[1], &err);
        if (nd) {
            bool pptx = termina_en(argv[1], ".pptx");
            cargar(nd, pptx ? "" : argv[1]);
            if (pptx) modificado = true;
            registrar("tucan: abierto %s (%d diapositivas)\n", argv[1], dk->n);
        } else mensagem("Tucán", _("No se ha podido abrir el archivo."));
    }
    layout();
    redibujar_todo();
    registrar("tucan: pronto\n");

    evento_t e;
    while (!sair_pedido) {
        int espera = -1;
        if (pres) espera = pres_espera();
        int r = esperar_evento(j->id, &e, espera);
        if (r != 1) { if (pres) pres_tiempo(); continue; }
        if (e.tipo == EV_FECHAR) { if (pres) { pres_salir(); continue; } if (pode_descartar()) break; continue; }
        if (e.tipo == EV_REDIMENSIONAR) {
            bool cheia = e.tecla == JAN_ESTADO_TELA_CHEIA;
            if (jan_redimensionar(j, &e)) {
                if (pres && !cheia && !pres_entrando) pres_salir();
                if (cheia) pres_entrando = false;
                if (pres) pres_pintar(-1);
                else { cinta_largura(&cinta, 0, 0, j->larg); layout(); redibujar_todo(); }
            }
            continue;
        }
        if (pres) { pres_evento(&e); continue; }
        if (e.tipo == EV_TECLA && e.tecla == TECLA_F12) { accion(A_ASISTENTE); redibujar_todo(); continue; }
        {
            int v0 = ag_version;
            if (ag_evento(&e)) { if (ag_version != v0) { layout(); } redibujar_todo(); continue; }
        }
        if (e.tipo == EV_MENU) {
            if (e.tecla == 7001 || e.tecla == 7002) {
                static const int d_[7] = { A_DIAP_NUEVA, A_DIAP_DUP, A_DIAP_BORRAR, A_DIAP_OCULTAR, 0, A_DIAP_SUBIR, A_DIAP_BAJAR };
                static const int o_[8] = { A_CORTAR, A_COPIAR, A_PEGAR, 0, A_FRENTE, A_ATRAS, 0, A_BORRAR_OBJ };
                if (e.x >= 0 && e.x < (e.tecla == 7001 ? 7 : 8)) { int a = e.tecla == 7001 ? d_[e.x] : o_[e.x]; if (a) accion(a); layout(); redibujar_todo(); }
                continue;
            }
        }
        /* la cinta tiene prioridad con el ratón y las teclas de acceso */
        {
            int id = cinta_tratar(j, &cinta, &e);
            if (cinta_cambio(&cinta)) { layout(); redibujar_todo(); }
            if (id > 0) { cinta_orden(id); continue; }
            if (id == CINTA_TRATADO) continue;
            if (e.tipo == EV_MENU) continue;
        }
        if (e.tipo == EV_RODA) {
            int d = (int)e.cliques;
            if (mini_vis && e.x < r_mini_w) {
                mini_top += d * 40;
                int max = dk->n * MINI_FILA - r_mini_h + 12;
                if (mini_top > max) mini_top = max;
                if (mini_top < 0) mini_top = 0;
                redibujar_todo();
            } else if (notas_vis && e.y >= r_not_y) { if (area_tratar(j, &notas, &e)) notas_sucia = true; redibujar_todo(); }
            else { ir_a(cur + (d > 0 ? 1 : -1)); redibujar_todo(); }
            continue;
        }
        if (e.tipo == EV_CLIQUE) { raton_clic(&e); redibujar_todo(); continue; }
        if (e.tipo == EV_MOVER) { if (drag) raton_mover(&e); continue; }
        if (e.tipo == EV_SOLTOU) { raton_soltar(&e); continue; }
        if (e.tipo == EV_TECLA) {
            bool usada = false;
            if (edit_obj >= 0) {
                unsigned t_ = e.tecla;
                if ((e.modificadores & 2) && (t_ == 14 || t_ == 15 || t_ == 19 || t_ == 13 || t_ == 4 || t_ == 26 || t_ == 25 || t_ == 2 || t_ == 9 || t_ == 21)) atajo_ctrl(t_, &usada);
                if (!usada && !edit_tecla(&e)) teclado_lienzo(&e);
                if (edit_obj >= 0 && e.tecla != TECLA_ESC) {   /* saltos y tabuladores como | y > para que la línea de registro sea una */
                    char lt[240];
                    copia_texto(lt, diap()->obj[edit_obj].texto, sizeof lt);
                    for (char *q = lt; *q; q++) { if (*q == '\n') *q = '|'; else if (*q == '\t') *q = '>'; else if (*q == '\x1f') *q = '/'; }
                    registrar("tucan: texto editado [%s]\n", lt);
                }
            } else if (notas.foco) {
                if (e.modificadores & 2) atajo_ctrl(e.tecla, &usada);
                if (!usada) {
                    if (e.tecla == TECLA_ESC) notas.foco = false;
                    else if (area_tratar(j, &notas, &e)) { notas_sucia = true; modificado = true; }
                }
            } else {
                if (e.modificadores & 2) atajo_ctrl(e.tecla, &usada);
                if (!usada) teclado_lienzo(&e);
            }
            if (sair_pedido) break;
            if (!pres) redibujar_todo();
            continue;
        }
    }
    if (pres) pres_salir();
    if (ag) ag_panel_liberar(ag);
    ag_quitar_herramientas();
    jan_fechar(j);
    registrar("tucan: fechado\n");
    return 0;
}
