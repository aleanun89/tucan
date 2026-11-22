// SPDX-License-Identifier: GPL-2.0-or-later
/* dialogos_cf.c - diálogos comunes de color y de fuente y el combo de fuentes (D327).
 *
 * Los tres son modales y de estilo Windows 98, como los de archivos (D272) y los de Pluma (D291).
 * Sin coma flotante (la libsagui no usa la FPU): el HSL de Windows (0-240) va en enteros.
 *   dialogo_cor            Color: 48 básicos, 16 personalizados (/home/.config/colores.conf), espectro,
 *                          luminosidad, Tono/Sat/Lum y Rojo/Verde/Azul sincronizados.
 *   dialogo_fonte_sistema  Fuente: lista filtrable de las del sistema, estilo, tamaño, efectos, color y muestra.
 *   combo_fuentes          lista desplegable con buscador y cada nombre con su propia fuente. */
#include "gui.h"
#include "pdf_escribir.h"
#include "fontes.h"

/* El proveedor de fuentes del sistema (libfontes, con la x87) no está en la libsagui: si el programa lo
 * enlaza se instala, y si no, las familias son solo las tres clásicas. */
extern void fuentes_doc_instalar(void) __attribute__((weak));

/* El printf de la libsagui no rellena con ceros: seis cifras hexadecimales a mano. */
static const char *hex6(char *d, uint32_t v)
{
    for (int i = 0; i < 6; i++) d[i] = "0123456789ABCDEF"[(v >> (20 - 4 * i)) & 15];
    d[6] = 0;
    return d;
}

static bool en(int px, int py, int x, int y, int l, int a) { return px >= x && py >= y && px < x + l && py < y + a; }
static void etiq(jan_t *j, int x, int y, const char *t) { rotulo(j, x, y, t, W98_NEGRO); }
static void fondo98(jan_t *j) { g_retangulo(j, 0, 0, j->larg, j->alt, W98_FACE); }

/* ------------------------------------------------------------ HSL <-> RGB (escala de Windows, 0-240) */

void cor_para_hsl(uint32_t rgb, int *h, int *s, int *l)
{
    int r = (int)((rgb >> 16) & 255), g = (int)((rgb >> 8) & 255), b = (int)(rgb & 255);
    int mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
    int mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
    int sum = mx + mn, d = mx - mn;
    *l = (sum * 240 + 255) / 510;
    if (!d) { *s = 0; *h = 160; return; }          /* gris: Windows deja el tono en 160 */
    int den = sum <= 255 ? sum : 510 - sum;
    int sa = (d * 240 + den / 2) / den;
    *s = sa > 240 ? 240 : sa;
    int num, base;
    if (mx == r) { num = 40 * (g - b); base = 0; }
    else if (mx == g) { num = 40 * (b - r); base = 80; }
    else { num = 40 * (r - g); base = 160; }
    int hh = base + (num >= 0 ? (num + d / 2) / d : -((-num + d / 2) / d));
    while (hh < 0) hh += 240;
    while (hh >= 240) hh -= 240;
    *h = hh;
}

static int canal_hsl(int t1, int t2, int h)
{
    h %= 240;
    if (h < 0) h += 240;
    if (h < 40) return t1 + (t2 - t1) * h / 40;
    if (h < 120) return t2;
    if (h < 160) return t1 + (t2 - t1) * (160 - h) / 40;
    return t1;
}

void cor_de_hsl(int h, int s, int l, uint32_t *rgb)
{
    if (h < 0) h = 0;
    if (h > 240) h = 240;
    if (s < 0) s = 0;
    if (s > 240) s = 240;
    if (l < 0) l = 0;
    if (l > 240) l = 240;
    int r, g, b;
    if (!s) r = g = b = l;
    else {
        int t2 = l <= 120 ? l * (240 + s) / 240 : l + s - l * s / 240;
        int t1 = 2 * l - t2;
        r = canal_hsl(t1, t2, h + 80);
        g = canal_hsl(t1, t2, h);
        b = canal_hsl(t1, t2, h - 80);
    }
    r = (r * 255 + 120) / 240; g = (g * 255 + 120) / 240; b = (b * 255 + 120) / 240;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    *rgb = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

/* ------------------------------------------------------------ Color */

#define CD_CEL_L 34          /* celda de la rejilla: 34 x 22 (la muestra, 28 x 16) */
#define CD_CEL_A 22
#define CD_BX 12             /* rejilla de básicos: 8 x 6 */
#define CD_BY 28
#define CD_PX 12             /* rejilla de personalizados: 8 x 2 */
#define CD_PY 188
#define CD_ESP_X 312         /* espectro 180 x 160 */
#define CD_ESP_Y 12
#define CD_ESP_L 180
#define CD_ESP_A 160
#define CD_LUM_X 506         /* barra de luminosidad 14 x 160 */
#define CD_LUM_L 14
#define CD_ANCHO_C 296
#define CD_ANCHO_E 624
#define CD_ALTO 314
#define COLORES_CONF "/home/.config/colores.conf"

static const uint32_t cd_basicos[48] = {
    0xFF8080, 0xFFFF80, 0x80FF80, 0x00FF80, 0x80FFFF, 0x0080FF, 0xFF80C0, 0xFF80FF,
    0xFF0000, 0xFFFF00, 0x80FF00, 0x00FF40, 0x00FFFF, 0x0080C0, 0x8080C0, 0xFF00FF,
    0x804040, 0xFF8040, 0x00FF00, 0x008080, 0x004080, 0x8080FF, 0x800040, 0xFF0080,
    0x800000, 0xFF8000, 0x008000, 0x008040, 0x0000FF, 0x0000A0, 0x800080, 0x8000FF,
    0x400000, 0x804000, 0x004000, 0x004040, 0x000080, 0x000040, 0x400040, 0x400080,
    0x000000, 0x808000, 0x808040, 0x808080, 0x408080, 0xC0C0C0, 0x400040, 0xFFFFFF,
};

typedef struct {
    uint32_t cor;
    int h, s, l;
    uint32_t pers[16];
    int sel_bas, sel_per, prox;
    bool exp;
    caixa_t cx[6];        /* Tono, Sat, Lum, Rojo, Verde, Azul */
    int arr;              /* 0 nada, 1 espectro, 2 barra (arrastrando) */
} cd_t;

static void cd_cargar(cd_t *d)
{
    for (int i = 0; i < 16; i++) d->pers[i] = 0xFFFFFF;
    int fd = abrir(COLORES_CONF, ABRIR_LER);
    if (fd < 0) return;
    char b[16 * 8 + 16];
    int n = ler(fd, b, sizeof b - 1);
    fechar(fd);
    if (n <= 0) return;
    b[n] = 0;
    int k = 0;
    for (const char *p = b; *p && k < 16;) {
        uint32_t v = 0;
        int dig = 0;
        while ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')) {
            v = v * 16 + (uint32_t)(*p <= '9' ? *p - '0' : (*p | 32) - 'a' + 10);
            p++; dig++;
        }
        if (dig == 6) d->pers[k++] = v;
        while (*p && *p != '\n') p++;
        if (*p) p++;
    }
}

static void cd_guardar(const cd_t *d)
{
    criar_pasta("/home/.config");
    int fd = abrir(COLORES_CONF, ABRIR_ESCREVER | ABRIR_CRIAR | ABRIR_TRUNCAR);
    if (fd < 0) return;
    char b[16 * 8 + 4];
    int n = 0;
    for (int i = 0; i < 16; i++) { hex6(b + n, d->pers[i] & 0xFFFFFF); n += 6; b[n++] = '\n'; }
    escrever(fd, b, (unsigned)n);
    fechar(fd);
}

static void cd_log(const cd_t *d)
{
    char hx[8];
    registrar("dialogo: color rgb=%s hsl=%d,%d,%d\n", hex6(hx, d->cor), d->h, d->s, d->l);
}

/* Escribe los valores en los cuadros, salvo en el que se está editando (excepto, -1 = todos). */
static void cd_campos(cd_t *d, int excepto)
{
    char t[8];
    int v[6] = { d->h, d->s, d->l, (int)((d->cor >> 16) & 255), (int)((d->cor >> 8) & 255), (int)(d->cor & 255) };
    for (int i = 0; i < 6; i++) {
        if (i == excepto) continue;
        formatar_texto(t, sizeof t, "%d", v[i]);
        caixa_definir(&d->cx[i], t);
    }
}

static void cd_poner_rgb(cd_t *d, uint32_t rgb, int excepto)
{
    d->cor = rgb & 0xFFFFFF;
    cor_para_hsl(d->cor, &d->h, &d->s, &d->l);
    cd_campos(d, excepto);
    cd_log(d);
}

static void cd_poner_hsl(cd_t *d, int h, int s, int l, int excepto)
{
    d->h = h < 0 ? 0 : h > 239 ? 239 : h;
    d->s = s < 0 ? 0 : s > 240 ? 240 : s;
    d->l = l < 0 ? 0 : l > 240 ? 240 : l;
    cor_de_hsl(d->h, d->s, d->l, &d->cor);
    cd_campos(d, excepto);
    cd_log(d);
}

static void cd_celda(jan_t *j, int x, int y, uint32_t c, bool sel)
{
    g_retangulo(j, x, y, CD_CEL_L, CD_CEL_A, W98_FACE);
    if (sel) { g_moldura(j, x, y, CD_CEL_L, CD_CEL_A, W98_NEGRO); g_moldura(j, x + 1, y + 1, CD_CEL_L - 2, CD_CEL_A - 2, W98_NEGRO); }
    g_retangulo(j, x + 4, y + 3, CD_CEL_L - 8, CD_CEL_A - 6, c);
    g_bisel(j, x + 3, y + 2, CD_CEL_L - 6, CD_CEL_A - 4, true);
}

static void cd_rejillas(jan_t *j, cd_t *d)
{
    for (int i = 0; i < 48; i++)
        cd_celda(j, CD_BX + (i % 8) * CD_CEL_L, CD_BY + (i / 8) * CD_CEL_A, cd_basicos[i], i == d->sel_bas);
    for (int i = 0; i < 16; i++)
        cd_celda(j, CD_PX + (i % 8) * CD_CEL_L, CD_PY + (i / 8) * CD_CEL_A, d->pers[i], i == d->sel_per);
    jan_mostrar_area(j, CD_BX, CD_BY, 8 * CD_CEL_L, CD_PY + 2 * CD_CEL_A - CD_BY);
}

static int cd_cruz_x(const cd_t *d) { return d->h * (CD_ESP_L - 1) / 239; }
static int cd_cruz_y(const cd_t *d) { return (240 - d->s) * (CD_ESP_A - 1) / 240; }
static int cd_flecha_y(const cd_t *d) { return (240 - d->l) * (CD_ESP_A - 1) / 240; }

/* El espectro se escribe directo en los píxeles de la ventana (28 800 puntos). */
static void cd_espectro(jan_t *j, cd_t *d)
{
    for (int y = 0; y < CD_ESP_A; y++) {
        int s = 240 - y * 240 / (CD_ESP_A - 1);
        for (int x = 0; x < CD_ESP_L; x++) {
            uint32_t c;
            cor_de_hsl(x * 239 / (CD_ESP_L - 1), s, 120, &c);
            int px = CD_ESP_X + x, py = CD_ESP_Y + y;
            if (px < j->larg && py < j->alt) j->px[py * j->larg + px] = c;
        }
    }
    g_bisel(j, CD_ESP_X - 2, CD_ESP_Y - 2, CD_ESP_L + 4, CD_ESP_A + 4, true);
    int cx = CD_ESP_X + cd_cruz_x(d), cy = CD_ESP_Y + cd_cruz_y(d);
    g_linha(j, cx - 9, cy, cx - 3, cy, W98_NEGRO); g_linha(j, cx + 3, cy, cx + 9, cy, W98_NEGRO);
    g_linha(j, cx, cy - 9, cx, cy - 3, W98_NEGRO); g_linha(j, cx, cy + 3, cx, cy + 9, W98_NEGRO);
}

static void cd_barra(jan_t *j, cd_t *d)
{
    for (int y = 0; y < CD_ESP_A; y++) {
        uint32_t c;
        cor_de_hsl(d->h, d->s, 240 - y * 240 / (CD_ESP_A - 1), &c);
        g_retangulo(j, CD_LUM_X, CD_ESP_Y + y, CD_LUM_L, 1, c);
    }
    g_bisel(j, CD_LUM_X - 2, CD_ESP_Y - 2, CD_LUM_L + 4, CD_ESP_A + 4, true);
    int ay = CD_ESP_Y + cd_flecha_y(d);
    g_retangulo(j, CD_LUM_X + CD_LUM_L + 4, ay - 5, 12, 11, W98_FACE);       /* borra la flecha anterior */
    for (int i = 0; i < 5; i++) g_retangulo(j, CD_LUM_X + CD_LUM_L + 4 + i, ay - i, 1, 2 * i + 1, W98_NEGRO);
}

static const char *const cd_nombres[6] = { "Tono:", "Sat:", "Lum:", "Rojo:", "Verde:", "Azul:" };
static const int cd_cx_x[6] = { 464, 464, 464, 568, 568, 568 };
static const int cd_cx_y[6] = { 180, 208, 236, 180, 208, 236 };
static const int cd_et_x[6] = { 420, 420, 420, 516, 516, 516 };

static void cd_derecha(jan_t *j, cd_t *d)
{
    g_retangulo(j, CD_CEL_L * 8 + 28, 0, j->larg - (CD_CEL_L * 8 + 28), j->alt, W98_FACE);
    cd_espectro(j, d);
    cd_barra(j, d);
    /* muestra Color|Sólido */
    g_retangulo(j, 312, 184, 76, 44, d->cor);
    g_bisel(j, 310, 182, 80, 48, true);
    g_linha(j, 350, 184, 350, 227, W98_SOMBRA);
    etiq(j, 312, 232, "Color|Sólido");
    for (int i = 0; i < 6; i++) {
        etiq(j, cd_et_x[i], cd_cx_y[i] + 4, cd_nombres[i]);
        caixa_desenhar(j, &d->cx[i]);
    }
}

static void cd_dibujar(jan_t *j, cd_t *d, botao_t *ok, botao_t *can, botao_t *def, botao_t *agr)
{
    fondo98(j);
    etiq(j, 12, 8, "Colores básicos:");
    etiq(j, 12, 168, "Colores personalizados:");
    cd_rejillas(j, d);
    boton98(j, def); boton98(j, ok); boton98(j, can);
    if (d->exp) { cd_derecha(j, d); boton98(j, agr); }
    jan_mostrar(j);
}

static void cd_ancho(jan_t *j, int l)
{
    uint32_t *px;
    if (redimensionar_janela(j->id, l, CD_ALTO, &px) == 0) { j->px = px; j->larg = l; j->alt = CD_ALTO; }
}

bool dialogo_cor(const char *titulo, uint32_t *cor)
{
    static cd_t d;
    memset(&d, 0, sizeof d);
    cd_cargar(&d);
    d.sel_bas = -1; d.sel_per = -1;
    uint32_t ini = *cor & 0xFFFFFF;
    for (int i = 0; i < 48; i++) if (cd_basicos[i] == ini) { d.sel_bas = i; break; }
    jan_t *j = jan_abrir(titulo && titulo[0] ? titulo : "Color", CD_ANCHO_C, CD_ALTO);
    if (!j) return false;
    jan_estilo(j, JAN_DIALOGO, 0, 0);
    for (int i = 0; i < 6; i++) { d.cx[i].x = cd_cx_x[i]; d.cx[i].y = cd_cx_y[i]; d.cx[i].l = i < 3 ? 44 : 44; }
    cd_poner_rgb(&d, ini, -1);
    botao_t def = { 12, 240, 272, 26, "Definir colores personalizados >>", false };
    botao_t ok = { 12, 276, 132, 26, "Aceptar", false }, can = { 152, 276, 132, 26, "Cancelar", false };
    botao_t agr = { 312, 276, 272, 26, "Agregar a colores personalizados", false };
    char hx[8];
    registrar("dialogo: color abierto inicial=%s\n", hex6(hx, ini));
    bool aceptar = false, redib = true;
    int foco = 0;
    evento_t e;
    for (;;) {
        if (redib) { cd_dibujar(j, &d, &ok, &can, &def, &agr); redib = false; }
        if (proximo_evento(j->id, &e, true) != 1) continue;
        if (e.tipo == EV_FECHAR) break;
        if (e.tipo == EV_TECLA && e.tecla == TECLA_ESC) break;
        if (e.tipo == EV_TECLA && e.tecla == '\n') { aceptar = true; break; }
        if (boton98_tratar(j, &ok, &e)) { aceptar = true; break; }
        if (boton98_tratar(j, &can, &e)) break;
        if (!d.exp && boton98_tratar(j, &def, &e)) {
            d.exp = true;
            cd_ancho(j, CD_ANCHO_E);
            registrar("dialogo: color definir personalizados\n");
            redib = true;
            continue;
        }
        if (d.exp && boton98_tratar(j, &agr, &e)) {
            int k = d.sel_per >= 0 ? d.sel_per : d.prox;
            d.pers[k] = d.cor;
            d.sel_per = k; d.sel_bas = -1;
            d.prox = (k + 1) % 16;
            cd_guardar(&d);
            registrar("dialogo: color personalizado %d = %s\n", k, hex6(hx, d.cor));
            redib = true;
            continue;
        }
        /* rejillas */
        if (e.tipo == EV_CLIQUE && (e.botoes & 1)) {
            int sel = -1, tabla = 0;
            if (en(e.x, e.y, CD_BX, CD_BY, 8 * CD_CEL_L, 6 * CD_CEL_A)) { sel = (e.y - CD_BY) / CD_CEL_A * 8 + (e.x - CD_BX) / CD_CEL_L; tabla = 1; }
            else if (en(e.x, e.y, CD_PX, CD_PY, 8 * CD_CEL_L, 2 * CD_CEL_A)) { sel = (e.y - CD_PY) / CD_CEL_A * 8 + (e.x - CD_PX) / CD_CEL_L; tabla = 2; }
            if (tabla) {
                d.sel_bas = tabla == 1 ? sel : -1;
                d.sel_per = tabla == 2 ? sel : -1;
                cd_poner_rgb(&d, tabla == 1 ? cd_basicos[sel] : d.pers[sel], -1);
                if (e.cliques == 2) { aceptar = true; break; }
                redib = true;
                continue;
            }
            if (d.exp) {
                if (en(e.x, e.y, CD_ESP_X, CD_ESP_Y, CD_ESP_L, CD_ESP_A)) d.arr = 1;
                else if (en(e.x, e.y, CD_LUM_X - 4, CD_ESP_Y, CD_LUM_L + 24, CD_ESP_A)) d.arr = 2;
            }
        }
        if (d.arr && (e.tipo == EV_CLIQUE || e.tipo == EV_MOVER)) {
            if (d.arr == 1) {
                int x = e.x - CD_ESP_X, y = e.y - CD_ESP_Y;
                x = x < 0 ? 0 : x >= CD_ESP_L ? CD_ESP_L - 1 : x;
                y = y < 0 ? 0 : y >= CD_ESP_A ? CD_ESP_A - 1 : y;
                cd_poner_hsl(&d, x * 239 / (CD_ESP_L - 1), 240 - y * 240 / (CD_ESP_A - 1), d.l, -1);
            } else {
                int y = e.y - CD_ESP_Y;
                y = y < 0 ? 0 : y >= CD_ESP_A ? CD_ESP_A - 1 : y;
                cd_poner_hsl(&d, d.h, d.s, 240 - y * 240 / (CD_ESP_A - 1), -1);
            }
            d.sel_bas = d.sel_per = -1;
            redib = true;
            if (e.tipo == EV_MOVER) { cd_rejillas(j, &d); cd_derecha(j, &d); jan_mostrar(j); redib = false; }
            continue;
        }
        if (e.tipo == EV_SOLTOU) d.arr = 0;
        /* cuadros numéricos */
        if (d.exp) {
            if (e.tipo == EV_TECLA && e.tecla == '\t') {
                d.cx[foco].foco = false;
                foco = (foco + 1) % 6;
                d.cx[foco].foco = true;
                redib = true;
                continue;
            }
            for (int i = 0; i < 6; i++) {
                char antes[128];
                copia_texto(antes, d.cx[i].texto, sizeof antes);
                if (e.tipo == EV_CLIQUE && en(e.x, e.y, d.cx[i].x, d.cx[i].y, d.cx[i].l, 24)) foco = i;
                if (e.tipo == EV_TECLA && !d.cx[i].foco) continue;
                if (e.tipo == EV_TECLA && e.tecla >= 0x20 && !(e.tecla >= '0' && e.tecla <= '9')) continue;   /* solo cifras */
                caixa_tratar(j, &d.cx[i], &e);
                if (compara_texto(antes, d.cx[i].texto) != 0) {
                    if (tam_texto(d.cx[i].texto) > 3) { copia_texto(d.cx[i].texto, antes, sizeof d.cx[i].texto); d.cx[i].cursor = (int)tam_texto(antes); continue; }
                    int v = 0;
                    for (const char *s = d.cx[i].texto; *s >= '0' && *s <= '9'; s++) v = v * 10 + (*s - '0');
                    if (!d.cx[i].texto[0]) continue;
                    d.sel_bas = d.sel_per = -1;
                    if (i < 3) {
                        int h = d.h, s2 = d.s, l = d.l;
                        if (i == 0) h = v > 239 ? 239 : v;
                        if (i == 1) s2 = v > 240 ? 240 : v;
                        if (i == 2) l = v > 240 ? 240 : v;
                        cd_poner_hsl(&d, h, s2, l, i);
                    } else {
                        int r = (int)((d.cor >> 16) & 255), g = (int)((d.cor >> 8) & 255), b = (int)(d.cor & 255);
                        if (v > 255) v = 255;
                        if (i == 3) r = v;
                        if (i == 4) g = v;
                        if (i == 5) b = v;
                        cd_poner_rgb(&d, ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b, i);
                    }
                    cd_rejillas(j, &d);
                    cd_derecha(j, &d);
                    jan_mostrar(j);
                }
            }
        }
    }
    if (aceptar) *cor = d.cor;
    jan_fechar(j);
    return aceptar;
}

/* ------------------------------------------------------------ las familias del sistema */

static char cf_nombres[PDFW_FAM_MAX + 1][64];
static int cf_n;

static void cf_instalar(void)
{
    if (fuentes_doc_instalar) fuentes_doc_instalar();
}

/* Vuelve a leer las familias (y las carpetas si actualizar); devuelve cuántas. */
static int cf_cargar(bool actualizar)
{
    cf_instalar();
    const fuentes_prov_t *p = fuentes_prov_obter();
    if (actualizar && p && p->actualizar) p->actualizar();
    int n = p && p->cantidad ? p->cantidad() : 3;
    if (n < 3) n = 3;
    if (n > PDFW_FAM_MAX + 1) n = PDFW_FAM_MAX + 1;
    for (int i = 0; i < n; i++) {
        /* D356: una familia sin archivos (se quitó la fuente) queda con nombre vacío y no sale en las listas */
        const char *nm = i < 3 ? pdfw_nombre_familia(i) : (p && p->nombre ? p->nombre(i) : NULL);
        copia_texto(cf_nombres[i], nm ? nm : "", sizeof cf_nombres[i]);
    }
    cf_n = n;
    return n;
}

int fuente_estilos(int fam)
{
    const fuentes_prov_t *p = fuentes_prov_obter();
    int m = p && p->estilos ? p->estilos(fam) : 15;
    return m | 1;
}

int fuente_familia_por_nombre(const char *nombre)
{
    cf_instalar();
    for (int i = 0; i < 3; i++) {
        const char *n = pdfw_nombre_familia(i);
        const char *a = n, *b = nombre;
        while (*a && *b && ((*a | 32) == (*b | 32))) { a++; b++; }
        if (!*a && !*b) return i;
    }
    const fuentes_prov_t *p = fuentes_prov_obter();
    return p && p->resolver ? p->resolver(nombre) : -1;
}

static char minus(char c) { return c >= 'A' && c <= 'Z' ? (char)(c + 32) : c; }

/* ¿El nombre contiene el filtro (sin distinguir mayúsculas)? */
static bool cf_casa(const char *nombre, const char *filtro)
{
    if (!nombre[0]) return false;
    if (!filtro[0]) return true;
    for (const char *s = nombre; *s; s++) {
        const char *a = s, *b = filtro;
        while (*a && *b && minus(*a) == minus(*b)) { a++; b++; }
        if (!*b) return true;
    }
    return false;
}

/* Una fila con el nombre dibujado con su propia fuente. */
static void cf_fila(jan_t *j, int x, int y, int l, int fam, const char *nombre, bool sel)
{
    g_retangulo(j, x, y, l, LISTA_ALTURA_ITEM, sel ? COR_SELECAO : COR_CAMPO);
    rte_fmt_t f = { (uint8_t)fam, 0, 11, sel ? COR_SEL_TEXTO : 0x000000 };
    rte_amostra(j, x, y, l, LISTA_ALTURA_ITEM, &f, nombre);
}

/* ------------------------------------------------------------ Fuente */

static const int fd_tams[] = { 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72 };
#define FD_N_TAMS ((int)(sizeof fd_tams / sizeof fd_tams[0]))

static char fd_it[PDFW_FAM_MAX + 1][64];   /* familias que pasan el filtro */
static int  fd_map[PDFW_FAM_MAX + 1];      /* su número de familia */

static void fd_lista_desenhar(jan_t *j, lista_t *l)
{
    lista_desenhar(j, l);
    int vis = (l->a - 4) / LISTA_ALTURA_ITEM;
    for (int k = 0; k < vis && l->topo + k < l->qtd; k++) {
        int i = l->topo + k;
        cf_fila(j, l->x + 3, l->y + 2 + k * LISTA_ALTURA_ITEM, l->l - 12, fd_map[i], l->itens[i], i == l->sel);
    }
    jan_mostrar_area(j, l->x, l->y, l->l, l->a);
}

static int fd_filtrar(const char *filtro, int fam_sel, int *sel)
{
    int n = 0;
    *sel = -1;
    for (int i = 0; i < cf_n; i++) {
        if (!cf_casa(cf_nombres[i], filtro)) continue;
        copia_texto(fd_it[n], cf_nombres[i], 64);
        fd_map[n] = i;
        if (i == fam_sel) *sel = n;
        n++;
    }
    return n;
}

static void fd_ajustar_topo(lista_t *l)
{
    int vis = (l->a - 4) / LISTA_ALTURA_ITEM;
    if (l->sel < l->topo) l->topo = l->sel < 0 ? 0 : l->sel;
    if (l->sel >= l->topo + vis) l->topo = l->sel - vis + 1;
    if (l->topo > l->qtd - vis) l->topo = l->qtd - vis > 0 ? l->qtd - vis : 0;
    if (l->topo < 0) l->topo = 0;
}

static const char *const fd_nombres_estilo[4] = { "Normal", "Negrita", "Cursiva", "Negrita cursiva" };

/* est de rte: 1 negrita, 2 cursiva. Lista de la familia: los estilos que tiene. */
static int fd_estilos(char (*it)[64], int *map, int fam, int est_deseado, int *sel)
{
    static const int orden[4] = { 0, 2, 1, 3 };    /* Normal, Cursiva, Negrita, Negrita cursiva (el orden de Windows) */
    int mask = fuente_estilos(fam), n = 0;
    *sel = 0;
    for (int k = 0; k < 4; k++) {
        int e = orden[k];
        if (!(mask & (1 << e))) continue;
        copia_texto(it[n], fd_nombres_estilo[e], 64);
        map[n] = e;
        if (e == est_deseado) *sel = n;
        n++;
    }
    return n;
}

static void fd_tam_texto(caixa_t *c, int tam)
{
    char t[8];
    formatar_texto(t, sizeof t, "%d", tam);
    caixa_definir(c, t);
}

static void fd_marco_caja(jan_t *j, int x, int y, int l, const char *t)
{
    g_retangulo(j, x, y, l, 24, COR_CAMPO);
    g_bisel(j, x - 1, y - 1, l + 2, 26, true);
    g_texto_n(j, x + 5, y + 4, t, (l - 10) / LETRA_L, W98_NEGRO);
}

bool dialogo_fonte_sistema(const char *titulo, fonte_sel_t *f)
{
    static char it_est[4][64], it_tam[FD_N_TAMS][64];
    static int map_est[4];
    cf_cargar(true);
    registrar("dialogo: fuentes %d familias\n", cf_n);
    int fam = f->fam;
    if (fam < 0 && f->familia[0]) fam = fuente_familia_por_nombre(f->familia);
    if (fam < 0 || fam >= cf_n) fam = 0;
    int tam = f->tam < 1 ? 12 : f->tam > 127 ? 127 : f->tam;
    int est = f->estilo & 3;
    uint32_t cor = f->cor & 0xFFFFFF;
    bool sub = (f->efeitos & FONTE_EFEITO_SUBLINHADO) != 0, ris = (f->efeitos & FONTE_EFEITO_RISCADO) != 0;

    jan_t *j = jan_abrir(titulo && titulo[0] ? titulo : "Fuente", 432, 340);
    if (!j) return false;
    jan_estilo(j, JAN_DIALOGO, 0, 0);

    caixa_t c_fam, c_tam;
    memset(&c_fam, 0, sizeof c_fam); memset(&c_tam, 0, sizeof c_tam);
    c_fam.x = 14; c_fam.y = 28; c_fam.l = 190; c_fam.foco = true;
    c_tam.x = 356; c_tam.y = 28; c_tam.l = 60;
    caixa_definir(&c_fam, cf_nombres[fam]);
    fd_tam_texto(&c_tam, tam);

    int sel_fam;
    int n_fam = fd_filtrar("", fam, &sel_fam);
    int n_est, sel_est;
    n_est = fd_estilos(it_est, map_est, fam, est, &sel_est);
    est = map_est[sel_est];
    for (int i = 0; i < FD_N_TAMS; i++) formatar_texto(it_tam[i], 64, "%d", fd_tams[i]);
    int sel_tam = -1;
    for (int i = 0; i < FD_N_TAMS; i++) if (fd_tams[i] == tam) sel_tam = i;
    lista_t l_fam = { 14, 60, 190, 124, fd_it, n_fam, sel_fam, 0, NULL, fd_lista_desenhar };
    lista_t l_est = { 214, 60, 130, 124, it_est, n_est, sel_est, 0, NULL, NULL };
    lista_t l_tam = { 356, 60, 60, 124, it_tam, FD_N_TAMS, sel_tam, 0, NULL, NULL };
    fd_ajustar_topo(&l_fam); fd_ajustar_topo(&l_tam);
    casilla_t c_ris = { 28, 220, ris }, c_sub = { 28, 244, sub };
    botao_t b_cor = { 90, 266, 110, 24, "", false };
    botao_t ok = { 230, 304, 92, 26, "Aceptar", false }, cancel = { 328, 304, 92, 26, "Cancelar", false };
    int foco = 0;        /* 0 cuadro de familia, 1 lista de estilos, 2 cuadro de tamaño */
    bool aceptar = false, redib = true;
    evento_t e;
    for (;;) {
        if (redib) {
            fondo98(j);
            etiq(j, 14, 10, "Fuente:"); etiq(j, 214, 10, "Estilo de fuente:"); etiq(j, 356, 10, "Tamaño:");
            caixa_desenhar(j, &c_fam);
            fd_marco_caja(j, 214, 28, 130, sel_est >= 0 ? it_est[sel_est] : "");
            caixa_desenhar(j, &c_tam);
            lista_en_marco(j, &l_fam); lista_en_marco(j, &l_est); lista_en_marco(j, &l_tam);
            g_bisel(j, 14, 194, 200, 100, true);
            g_retangulo(j, 22, 190, 56, 16, W98_FACE);
            etiq(j, 24, 190, "Efectos");
            casilla98(j, &c_ris, "Tachado"); casilla98(j, &c_sub, "Subrayado");
            etiq(j, 28, 270, "Color:");
            g_retangulo(j, 90, 266, 110, 24, W98_FACE);
            g_bisel(j, 90, 266, 110, 24, false);
            g_retangulo(j, 96, 271, 82, 14, cor);
            g_moldura(j, 96, 271, 82, 14, W98_NEGRO);
            g_flecha(j, 184, 276, 1, W98_NEGRO);
            GR_REG(j, "botao", &b_cor, 90, 266, 110, 24, "Color", 0, GRF_CONTROL);
            g_bisel(j, 226, 194, 190, 100, true);
            g_retangulo(j, 234, 190, 56, 16, W98_FACE);
            etiq(j, 236, 190, "Muestra");
            g_retangulo(j, 236, 216, 170, 66, COR_BRANCO);
            g_bisel(j, 234, 214, 174, 70, true);
            rte_fmt_t g = { (uint8_t)fam, (uint8_t)(est | (sub ? RTE_SUBLINHADO : 0) | (ris ? RTE_RISCADO : 0)), (uint8_t)(tam > 24 ? 24 : tam), cor };
            rte_amostra(j, 236, 216, 170, 66, &g, "AaBbYyZz");
            boton98(j, &ok); boton98(j, &cancel);
            jan_mostrar(j);
            redib = false;
        }
        if (proximo_evento(j->id, &e, true) != 1) continue;
        if (e.tipo == EV_FECHAR) break;
        if (e.tipo == EV_TECLA && e.tecla == TECLA_ESC) break;
        if (e.tipo == EV_TECLA && e.tecla == '\n') { aceptar = true; break; }
        if (boton98_tratar(j, &ok, &e)) { aceptar = true; break; }
        if (boton98_tratar(j, &cancel, &e)) break;
        if (e.tipo == EV_CLIQUE && (e.botoes & 1) && en(e.x, e.y, b_cor.x, b_cor.y, b_cor.l, b_cor.a)) {
            uint32_t c = cor;
            if (dialogo_cor("Color", &c)) cor = c;
            char hx[8];
            registrar("dialogo: fuente color %s\n", hex6(hx, cor));
            redib = true;
            continue;
        }
        if (casilla98_tratar(j, &c_ris, "Tachado", &e) || casilla98_tratar(j, &c_sub, "Subrayado", &e)) {
            ris = c_ris.marcada; sub = c_sub.marcada;
            registrar("dialogo: fuente efectos subrayado=%d tachado=%d\n", sub, ris);
            redib = true;
            continue;
        }
        if (e.tipo == EV_TECLA && e.tecla == '\t') {
            foco = (foco + 1) % 3;
            c_fam.foco = foco == 0; c_tam.foco = foco == 2;
            redib = true;
            continue;
        }
        if (e.tipo == EV_CLIQUE) {
            if (en(e.x, e.y, c_fam.x, c_fam.y, c_fam.l, 24) || en(e.x, e.y, l_fam.x, l_fam.y, l_fam.l, l_fam.a)) foco = 0;
            else if (en(e.x, e.y, l_est.x, l_est.y, l_est.l, l_est.a)) foco = 1;
            else if (en(e.x, e.y, c_tam.x, c_tam.y, c_tam.l, 24) || en(e.x, e.y, l_tam.x, l_tam.y, l_tam.l, l_tam.a)) foco = 2;
            c_fam.foco = foco == 0 && !en(e.x, e.y, l_fam.x, l_fam.y, l_fam.l, l_fam.a); c_tam.foco = foco == 2 && !en(e.x, e.y, l_tam.x, l_tam.y, l_tam.l, l_tam.a);
        }
        bool nav = e.tipo == EV_TECLA && (e.tecla == TECLA_CIMA || e.tecla == TECLA_BAIXO || e.tecla == TECLA_PGUP || e.tecla == TECLA_PGDN ||
                                          e.tecla == TECLA_HOME || e.tecla == TECLA_END);
        bool cambio = false;
        int ant_fam = l_fam.sel, ant_est = l_est.sel, ant_tam = l_tam.sel;
        /* teclado: según el foco */
        if (e.tipo == EV_TECLA) {
            if (foco == 0) {
                if (nav) lista_tratar(j, &l_fam, &e);
                else {
                    char antes[128];
                    copia_texto(antes, c_fam.texto, sizeof antes);
                    caixa_tratar(j, &c_fam, &e);
                    if (compara_texto(antes, c_fam.texto) != 0) {
                        int s;
                        n_fam = fd_filtrar(c_fam.texto, fam, &s);
                        l_fam.qtd = n_fam; l_fam.sel = n_fam > 0 ? (s >= 0 ? s : 0) : -1; l_fam.topo = 0;
                        fd_ajustar_topo(&l_fam);
                        registrar("dialogo: fuente filtro \"%s\" %d coincidencias\n", c_fam.texto, n_fam);
                        ant_fam = -2;      /* fuerza el cambio de familia si la selección cambió */
                        if (n_fam > 0 && fd_map[l_fam.sel] == fam) ant_fam = l_fam.sel;
                        redib = true;
                    }
                }
            } else if (foco == 1) lista_tratar(j, &l_est, &e);
            else {
                if (nav) lista_tratar(j, &l_tam, &e);
                else {
                    char antes[128];
                    copia_texto(antes, c_tam.texto, sizeof antes);
                    if (e.tecla >= 0x20 && !(e.tecla >= '0' && e.tecla <= '9')) continue;
                    caixa_tratar(j, &c_tam, &e);
                    if (compara_texto(antes, c_tam.texto) != 0) {
                        if (tam_texto(c_tam.texto) > 3) { caixa_definir(&c_tam, antes); continue; }
                        int v = 0;
                        for (const char *s = c_tam.texto; *s >= '0' && *s <= '9'; s++) v = v * 10 + (*s - '0');
                        if (v >= 1 && v <= 127) {
                            tam = v;
                            l_tam.sel = -1;
                            for (int i = 0; i < FD_N_TAMS; i++) if (fd_tams[i] == v) l_tam.sel = i;
                            fd_ajustar_topo(&l_tam);
                            ant_tam = l_tam.sel;
                            cambio = true;
                        }
                        redib = true;
                    }
                }
            }
        } else {
            lista_tratar(j, &l_fam, &e); lista_tratar(j, &l_est, &e); lista_tratar(j, &l_tam, &e);
            if (e.tipo == EV_CLIQUE) { caixa_tratar(j, &c_fam, &e); caixa_tratar(j, &c_tam, &e); }
        }
        if (l_fam.sel != ant_fam && l_fam.sel >= 0 && l_fam.sel < n_fam) {
            fam = fd_map[l_fam.sel];
            if (!(e.tipo == EV_TECLA && foco == 0 && !nav)) caixa_definir(&c_fam, cf_nombres[fam]);   /* al teclear se conserva el filtro */
            n_est = fd_estilos(it_est, map_est, fam, est, &sel_est);
            l_est.qtd = n_est; l_est.sel = sel_est; l_est.topo = 0;
            est = map_est[sel_est];
            cambio = true;
        }
        if (l_est.sel != ant_est && l_est.sel >= 0) { sel_est = l_est.sel; est = map_est[sel_est]; cambio = true; }
        if (l_tam.sel != ant_tam && l_tam.sel >= 0) { tam = fd_tams[l_tam.sel]; fd_tam_texto(&c_tam, tam); cambio = true; }
        if (cambio) {
            registrar("dialogo: fuente %s est=%d tam=%d\n", cf_nombres[fam], est, tam);
            redib = true;
        } else if (l_fam.sel != ant_fam || l_est.sel != ant_est || l_tam.sel != ant_tam) redib = true;
    }
    if (aceptar) {
        f->fam = fam;
        copia_texto(f->familia, cf_nombres[fam], sizeof f->familia);
        f->estilo = (uint8_t)(est & 3);
        f->efeitos = (uint8_t)((sub ? FONTE_EFEITO_SUBLINHADO : 0) | (ris ? FONTE_EFEITO_RISCADO : 0));
        f->tam = tam;
        f->cor = cor;
        char hx[8];
        registrar("dialogo: fuente aceptada %s est=%d tam=%d efectos=%d color=%s\n", f->familia, est, tam, f->efeitos, hex6(hx, cor));
    }
    jan_fechar(j);
    return aceptar;
}

/* ------------------------------------------------------------ combo_fuentes */

void combo_fuentes_iniciar(combo_fuentes_t *c, int x, int y, int l)
{
    memset(c, 0, sizeof *c);
    c->x = x; c->y = y; c->l = l; c->a = 21;
    copia_texto(c->texto, pdfw_nombre_familia(0), sizeof c->texto);
}

void combo_fuentes_definir(combo_fuentes_t *c, int fam)
{
    const char *n = pdfw_nombre_familia(fam >= 0 ? fam : 0);
    c->fam = fam >= 0 ? fam : 0;
    copia_texto(c->texto, n ? n : "", sizeof c->texto);
}

void combo_fuentes_desenhar(jan_t *j, combo_fuentes_t *c)
{
    g_retangulo(j, c->x, c->y, c->l, c->a, COR_CAMPO);
    g_bisel(j, c->x, c->y, c->l, c->a, true);
    int cols = (c->l - 4 - 17 - 4) / LETRA_L;
    g_texto_n(j, c->x + 4, c->y + 2, c->texto, cols, W98_NEGRO);
    int bx = c->x + c->l - 18;
    g_retangulo(j, bx, c->y + 2, 16, c->a - 4, W98_FACE);
    g_bisel(j, bx, c->y + 2, 16, c->a - 4, false);
    g_flecha(j, bx + 5, c->y + c->a / 2 - 2, 1, W98_NEGRO);
    jan_mostrar_area(j, c->x, c->y, c->l, c->a);
    GR_REG(j, "combo", c, c->x, c->y, c->l, c->a, c->texto, c->l - 4 - 17 - 4, GRF_CONTROL);
}

#define CB_FILAS 10
#define CB_ANCHO_MIN 260

/* La lista emergente: buscador arriba y las familias debajo, cada una con la suya. Devuelve la familia o -1. */
static int combo_fuentes_abrir(jan_t *j, combo_fuentes_t *c)
{
    static int map[PDFW_FAM_MAX + 1];
    cf_cargar(true);
    registrar("dialogo: fuentes %d familias\n", cf_n);
    int l = c->l < CB_ANCHO_MIN ? CB_ANCHO_MIN : c->l;
    if (l > j->larg - 4) l = j->larg - 4;
    int px = c->x;
    if (px + l > j->larg) px = j->larg - l;
    if (px < 0) px = 0;
    int py = c->y + c->a;
    int filas = CB_FILAS;
    int resto = j->alt - py - 28 - 4;
    if (resto / LISTA_ALTURA_ITEM < 4) {                    /* poco sitio debajo: se abre hacia arriba */
        int arriba = c->y - 28 - 4;
        if (arriba > resto) { filas = arriba / LISTA_ALTURA_ITEM; if (filas > CB_FILAS) filas = CB_FILAS; }
    }
    if (filas > resto / LISTA_ALTURA_ITEM && resto / LISTA_ALTURA_ITEM >= 4) filas = resto / LISTA_ALTURA_ITEM;
    if (filas < 3) filas = 3;
    if (filas > CB_FILAS) filas = CB_FILAS;
    int a = 28 + filas * LISTA_ALTURA_ITEM + 4;
    if (py + a > j->alt) py = c->y - a;
    if (py < 0) py = 0;
    /* guarda lo que hay debajo */
    uint32_t *copia = aloca((size_t)l * (size_t)a * sizeof(uint32_t));
    if (copia) for (int y = 0; y < a; y++) for (int x = 0; x < l; x++) copia[y * l + x] = j->px[(py + y) * j->larg + px + x];

    caixa_t busca;
    memset(&busca, 0, sizeof busca);
    busca.x = px + 3; busca.y = py + 2; busca.l = l - 6; busca.foco = true;
    int n = 0, sel = -1, topo = 0, res = -1;
    for (int i = 0; i < cf_n; i++) { if (!cf_nombres[i][0]) continue; map[n++] = i; if (i == c->fam) sel = n - 1; }
    if (sel < 0) sel = 0;
    if (sel >= filas) topo = sel - filas + 1;
    bool redib = true;
    evento_t e;
    for (;;) {
        if (redib) {
            g_retangulo(j, px, py, l, a, COR_BRANCO);
            g_moldura(j, px, py, l, a, W98_NEGRO);
            caixa_desenhar(j, &busca);
            if (sel < topo) topo = sel;
            if (sel >= topo + filas) topo = sel - filas + 1;
            if (topo > n - filas) topo = n - filas > 0 ? n - filas : 0;
            if (topo < 0) topo = 0;
            for (int k = 0; k < filas && topo + k < n; k++) {
                int i = topo + k;
                cf_fila(j, px + 2, py + 28 + k * LISTA_ALTURA_ITEM, l - 4 - 10, map[i], cf_nombres[map[i]], i == sel);
            }
            if (!n) g_texto(j, px + 8, py + 34, "Sin coincidencias", W98_SOMBRA);
            /* barra de desplazamiento fina */
            g_retangulo(j, px + l - 12, py + 28, 10, filas * LISTA_ALTURA_ITEM, 0xE0E0E0);
            if (n > filas) {
                int th = filas * LISTA_ALTURA_ITEM * filas / n;
                if (th < 12) th = 12;
                int ty = py + 28 + (filas * LISTA_ALTURA_ITEM - th) * topo / (n - filas);
                g_retangulo(j, px + l - 12, ty, 10, th, W98_FACE);
                g_bisel(j, px + l - 12, ty, 10, th, false);
            }
            jan_mostrar_area(j, px, py, l, a);
            redib = false;
        }
        if (proximo_evento(j->id, &e, true) != 1) continue;
        if (e.tipo == EV_FECHAR) break;
        if (e.tipo == EV_CLIQUE) {
            if (!en(e.x, e.y, px, py, l, a)) break;
            if (e.y >= py + 28 && e.x < px + l - 12) {
                int i = topo + (e.y - py - 28) / LISTA_ALTURA_ITEM;
                if (i < n && i >= 0) { res = map[i]; break; }
            }
            caixa_tratar(j, &busca, &e);
            continue;
        }
        if (e.tipo == EV_RODA) {
            if (!en(e.x, e.y, px, py, l, a)) continue;
            int d = (int)e.cliques * RODA_LINHAS;
            topo += d;
            if (topo > n - filas) topo = n - filas > 0 ? n - filas : 0;
            if (topo < 0) topo = 0;
            if (sel < topo) sel = topo;
            if (sel >= topo + filas) sel = topo + filas - 1;
            redib = true;
            continue;
        }
        if (e.tipo != EV_TECLA) continue;
        if (e.tecla == TECLA_ESC) break;
        if (e.tecla == '\n') { if (n > 0) res = map[sel]; break; }
        if (e.tecla == TECLA_BAIXO && sel < n - 1) sel++;
        else if (e.tecla == TECLA_CIMA && sel > 0) sel--;
        else if (e.tecla == TECLA_HOME) sel = 0;
        else if (e.tecla == TECLA_END) sel = n > 0 ? n - 1 : 0;
        else if (e.tecla == TECLA_PGDN) { sel += filas; if (sel > n - 1) sel = n - 1; }
        else if (e.tecla == TECLA_PGUP) { sel -= filas; if (sel < 0) sel = 0; }
        else {
            char antes[128];
            copia_texto(antes, busca.texto, sizeof antes);
            caixa_tratar(j, &busca, &e);
            if (compara_texto(antes, busca.texto) == 0) continue;
            n = 0; sel = 0; topo = 0;
            for (int i = 0; i < cf_n; i++) if (cf_casa(cf_nombres[i], busca.texto)) map[n++] = i;
            registrar("dialogo: fuentes filtro \"%s\" %d coincidencias\n", busca.texto, n);
        }
        if (sel < 0) sel = 0;
        redib = true;
    }
    if (copia) {
        for (int y = 0; y < a; y++) for (int x = 0; x < l; x++) j->px[(py + y) * j->larg + px + x] = copia[y * l + x];
        libera(copia);
        jan_mostrar_area(j, px, py, l, a);
    }
    return res;
}

int combo_fuentes_tratar(jan_t *j, combo_fuentes_t *c, const evento_t *e)
{
    if (e->tipo != EV_CLIQUE || !(e->botoes & 1) || !en(e->x, e->y, c->x, c->y, c->l, c->a)) return COMBO_NADA;
    combo_fuentes_desenhar(j, c);
    int r = combo_fuentes_abrir(j, c);
    if (r < 0) return COMBO_NADA;
    combo_fuentes_definir(c, r);
    combo_fuentes_desenhar(j, c);
    registrar("combo_fuentes: elegida %s\n", c->texto);
    return COMBO_SELECIONOU;
}
