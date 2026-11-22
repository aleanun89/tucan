// SPDX-License-Identifier: GPL-2.0-or-later
/* wordart.c - títulos con efectos tipo WordArt (mejora 122, D296). Ver wordart.h.
 *
 * Canalización: 1) el proveedor de máscaras dibuja el texto como cobertura de 8 bits;
 * 2) la forma (plano, arco, onda, subida, cursiva, perspectiva) decide, para cada píxel del
 * resultado, de qué punto de la máscara sale (muestreo bilineal); 3) con esa cobertura se
 * componen sombra, extrusión 3D, contorno (dilatación) y relleno sobre fondo blanco.
 * Sin coma flotante: seno por la aproximación racional de Bhaskara, arcotangente por CORDIC y
 * raíz cuadrada entera. */
#include "wordart.h"
#include "pdf_escribir.h"

extern const unsigned char fonte_sagui[256][16];

enum { F_PLANO, F_ARCO_ARR, F_ARCO_ABA, F_ONDA, F_SUBIDA, F_CURSIVA, F_PERS_D, F_PERS_I };
enum { R_SOLIDO, R_VERT, R_HORIZ, R_DIAG, R_ARCOIRIS, R_RAYAS, R_METAL };

typedef struct {
    const char *nome;
    uint8_t forma, relleno;
    uint32_t c1, c2;
    uint32_t cont; uint8_t cont_w;      /* contorno: color y grosor (0 = sin) */
    uint32_t c_sombra; uint8_t sombra;
    uint32_t c_3d; uint8_t prof3d;
    uint8_t fam, est;
    uint8_t fh;                          /* alto relativo del cuadro (% del cuerpo) */
} estilo_t;

static const estilo_t TAB[WORDART_ESTILOS] = {
    { "Negro liso",       F_PLANO,   R_SOLIDO,  0x000000, 0,        0,        0, 0x808080, 0, 0,        0, 0, 1, 130 },
    { "Azul degradado",   F_PLANO,   R_VERT,    0x80B0FF, 0x0030A0, 0x001860, 1, 0,        0, 0,        0, 0, 1, 130 },
    { "Rojo metálico",    F_PLANO,   R_METAL,   0xFFB0B0, 0xA00000, 0x500000, 1, 0x808080, 1, 0,        0, 0, 1, 130 },
    { "Oro con sombra",   F_PLANO,   R_VERT,    0xFFF080, 0xB07800, 0x503000, 1, 0x808080, 1, 0,        0, 1, 1, 135 },
    { "Verde en 3D",      F_PLANO,   R_SOLIDO,  0x30C030, 0,        0x104010, 1, 0,        0, 0x0C500C, 1, 0, 1, 140 },
    { "Arco iris curvo",  F_ARCO_ARR,R_ARCOIRIS,0,        0,        0x000000, 1, 0,        0, 0,        0, 0, 1, 230 },
    { "Arco abajo",       F_ARCO_ABA,R_VERT,    0xFFE000, 0xE05000, 0x502000, 1, 0,        0, 0,        0, 0, 1, 230 },
    { "Onda naranja",     F_ONDA,    R_VERT,    0xFFC060, 0xE06000, 0x602800, 1, 0x909090, 1, 0,        0, 0, 1, 190 },
    { "Subida morada",    F_SUBIDA,  R_SOLIDO,  0x9040D0, 0,        0x301060, 1, 0,        0, 0x401880, 1, 0, 1, 190 },
    { "Perspectiva der.", F_PERS_D,  R_VERT,    0x909090, 0x101010, 0,        0, 0,        0, 0,        0, 0, 1, 180 },
    { "Perspectiva izq.", F_PERS_I,  R_VERT,    0xFFF000, 0xE00000, 0x400000, 1, 0,        0, 0,        0, 0, 1, 180 },
    { "Contorno azul",    F_PLANO,   R_SOLIDO,  0xFFFFFF, 0,        0x2040D0, 2, 0,        0, 0,        0, 0, 1, 130 },
    { "Rayas rojas",      F_PLANO,   R_RAYAS,   0xE02020, 0xFFFFFF, 0x300000, 1, 0,        0, 0,        0, 0, 1, 130 },
    { "Cromo",            F_PLANO,   R_METAL,   0xF0F0F0, 0x505868, 0x303038, 1, 0x909090, 1, 0,        0, 0, 1, 135 },
    { "Fuego",            F_PLANO,   R_VERT,    0xFFF040, 0xE02000, 0x601000, 1, 0,        0, 0,        0, 0, 1, 130 },
    { "Hielo",            F_PLANO,   R_METAL,   0xFFFFFF, 0x3090E0, 0x104878, 1, 0xA0B0C0, 1, 0,        0, 0, 1, 135 },
    { "Neón",             F_PLANO,   R_SOLIDO,  0xFFFFFF, 0,        0xFF20C0, 2, 0xC080B0, 1, 0,        0, 0, 1, 135 },
    { "Madera en 3D",     F_PLANO,   R_HORIZ,   0xD09050, 0xA06020, 0x402008, 1, 0,        0, 0x502808, 1, 1, 1, 145 },
    { "Onda arco iris",   F_ONDA,    R_ARCOIRIS,0,        0,        0x202020, 1, 0x909090, 1, 0,        0, 0, 1, 190 },
    { "Cursiva dorada",   F_CURSIVA, R_VERT,    0xFFE880, 0xC08000, 0x604000, 1, 0x909090, 1, 0,        0, 1, 2, 135 },
};

const char *wordart_nome(int e) { return TAB[e >= 0 && e < WORDART_ESTILOS ? e : 0].nome; }
void wordart_padrao(int e, int *fam, int *est)
{
    const estilo_t *E = &TAB[e >= 0 && e < WORDART_ESTILOS ? e : 0];
    if (fam) *fam = E->fam;
    if (est) *est = E->est;
}

/* ------------------------------------------------------------ proveedor de máscaras */

static wordart_mascara_fn provedor;
void wordart_provedor(wordart_mascara_fn f) { provedor = f; }

/* La fuente 8x16 ampliada: bilineal sobre el mapa de bits y un escalón suave en 128. */
static bool prov_bitmap(const char *t, int fam, int est, int px, uint8_t **m, int *ml, int *ma)
{
    int n = (int)tam_texto(t);
    if (px < 8) px = 8;
    if (px > 600) px = 600;
    int alt = px * 13 / 10;
    int tot = 0;
    int adv[WORDART_TEXTO_MAX + 1];
    for (int i = 0; i < n && i < WORDART_TEXTO_MAX; i++) {
        adv[i] = pdfw_largura_char(fam, est & 3, (unsigned char)t[i]) * px / 1000;
        if (adv[i] < 2) adv[i] = 2;
        tot += adv[i];
    }
    if (n > WORDART_TEXTO_MAX) n = WORDART_TEXTO_MAX;
    int pad = px / 8 + 2;
    int largo = tot + 2 * pad;
    uint8_t *b = aloca_zerado((size_t)largo * (size_t)alt);
    if (!b) return false;
    int x0 = pad;
    int kk = alt / 16 > 1 ? alt / 16 : 1;
    for (int i = 0; i < n; i++) {
        unsigned char c = (unsigned char)t[i];
        int cw = adv[i];
        if (c > ' ' && c != 0xA0) {
            unsigned char g[16];
            for (int k = 0; k < 16; k++) g[k] = (est & 1) ? (unsigned char)(fonte_sagui[c][k] | (fonte_sagui[c][k] >> 1)) : fonte_sagui[c][k];
            int gw = cw + 1;
            for (int oy = 0; oy < alt; oy++) {
                int fy = ((2 * oy + 1) * 16 * 128) / alt - 128;
                int iy = fy >> 8, wy = fy & 255;
                for (int ox = 0; ox < gw; ox++) {
                    int xs = ox;
                    if (est & 2) xs += (oy - alt / 2) / 4;
                    int fx = ((2 * xs + 1) * 8 * 128) / gw - 128;
                    int ix = fx >> 8, wx = fx & 255;
                    int v[4];
                    for (int q = 0; q < 4; q++) {
                        int yy = iy + (q >> 1), xx = ix + (q & 1);
                        v[q] = (yy < 0 || yy > 15 || xx < 0 || xx > 7) ? 0 : (((g[yy] >> (7 - xx)) & 1) ? 255 : 0);
                    }
                    int top = v[0] * (256 - wx) + v[1] * wx, bot = v[2] * (256 - wx) + v[3] * wx;
                    int val = (top * (256 - wy) + bot * wy) >> 16;
                    int cov = 128 + (val - 128) * kk;
                    if (cov < 0) cov = 0;
                    if (cov > 255) cov = 255;
                    int px2 = x0 + ox;
                    if (px2 >= 0 && px2 < largo && cov > b[oy * largo + px2]) b[oy * largo + px2] = (uint8_t)cov;
                }
            }
        }
        x0 += cw;
    }
    *m = b; *ml = largo; *ma = alt;
    return true;
}

/* Recorta la máscara a la tinta (con 1 píxel de margen): el WordArt llena su cuadro. */
static void recortar(uint8_t **m, int *ml, int *ma)
{
    int L = *ml, A = *ma, x0 = L, x1 = -1, y0 = A, y1 = -1;
    for (int y = 0; y < A; y++)
        for (int x = 0; x < L; x++)
            if ((*m)[y * L + x] > 12) {
                if (x < x0) x0 = x;
                if (x > x1) x1 = x;
                if (y < y0) y0 = y;
                if (y > y1) y1 = y;
            }
    if (x1 < 0) return;                      /* sin tinta (solo espacios): se deja como está */
    x0 = x0 > 1 ? x0 - 1 : 0; y0 = y0 > 1 ? y0 - 1 : 0;
    x1 = x1 + 1 < L ? x1 + 1 : L - 1; y1 = y1 + 1 < A ? y1 + 1 : A - 1;
    int nl = x1 - x0 + 1, na = y1 - y0 + 1;
    uint8_t *n = aloca((size_t)nl * (size_t)na);
    if (!n) return;
    for (int y = 0; y < na; y++) memcpy(n + y * nl, *m + (y0 + y) * L + x0, (size_t)nl);
    libera(*m);
    *m = n; *ml = nl; *ma = na;
}

static bool obtener_mascara(const char *t, int fam, int est, int px, uint8_t **m, int *ml, int *ma)
{
    *m = NULL;
    bool ok = provedor && provedor(t, fam, est, px, m, ml, ma) && *m && *ml > 0 && *ma > 0;
    if (!ok) { libera(*m); *m = NULL; ok = prov_bitmap(t, fam, est, px, m, ml, ma); }
    if (ok) recortar(m, ml, ma);
    return ok;
}

void wordart_tamanho_natural(const char *texto, int estilo, int fam, int est, int tam, int *l_tw, int *a_tw)
{
    const estilo_t *E = &TAB[estilo >= 0 && estilo < WORDART_ESTILOS ? estilo : 0];
    if (tam < 8) tam = 8;
    int px = tam * 4 / 3;
    uint8_t *m = NULL; int ml = 0, ma = 0;
    if (!obtener_mascara(texto, fam, est, px, &m, &ml, &ma)) { ml = px * (int)tam_texto(texto) / 2; ma = px * 13 / 10; }
    libera(m);
    int h = ma * E->fh / 130;
    int w = ml * 110 / 100 + (E->prof3d + E->sombra) * px / 12;
    if (E->forma == F_ARCO_ARR || E->forma == F_ARCO_ABA) w = w * 3 / 4 > px * 5 ? w * 3 / 4 : px * 5;
    if (E->forma == F_SUBIDA || E->forma == F_CURSIVA) w += h / 4;
    *l_tw = w * 15;
    *a_tw = h * 15;
}

/* ------------------------------------------------------------ matemática entera */

/* sen(2π a / 1024) * 1024, a cualquiera. Bhaskara: error < 1,7 % */
static int isin(int a)
{
    a &= 1023;
    int neg = 0;
    if (a >= 512) { a -= 512; neg = 1; }
    long p = (long)a * (512 - a);
    int v = (int)(16L * p * 1024 / (1310720L - 4 * p));
    return neg ? -v : v;
}
static int icos(int a) { return isin(a + 256); }

static uint32_t raiz(uint32_t v)
{
    if (v < 2) return v;
    uint32_t x = v, y = (x + 1) / 2;
    while (y < x) { x = y; y = (x + v / x) / 2; }
    return x;
}

static const int atan_t[14] = { 32768, 19344, 10221, 5188, 2604, 1303, 652, 326, 163, 81, 41, 20, 10, 5 };

/* Ángulo de (x, y) respecto al eje x, en unidades de 1024 = vuelta entera, rango (-512, 512]. */
static int iatan2(int y, int x)
{
    int base = 0;
    if (x < 0) { x = -x; y = -y; base = 512; }   /* giro de media vuelta */
    long X = (long)x << 8, Y = (long)y << 8;
    long z = 0;
    for (int i = 0; i < 14; i++) {
        long xn, yn;
        if (Y > 0) { xn = X + (Y >> i); yn = Y - (X >> i); z += atan_t[i]; }
        else       { xn = X - (Y >> i); yn = Y + (X >> i); z -= atan_t[i]; }
        X = xn; Y = yn;
    }
    int ang = (int)(z / 256) + base;
    if (ang > 512) ang -= 1024;
    return ang;
}

/* ------------------------------------------------------------ muestreo y formas */

/* x * num / den en 1/256, sin desbordar 32 bits. */
static int esc256(int x, int num, int den)
{
    long p = (long)x * num;
    return (int)((p / den) * 256 + (p % den) * 256 / den);
}

static int muestra(const uint8_t *m, int ml, int ma, int fx, int fy)
{
    fx -= 128; fy -= 128;
    int ix = fx >> 8, iy = fy >> 8, wx = fx & 255, wy = fy & 255;
    int v[4];
    for (int q = 0; q < 4; q++) {
        int xx = ix + (q & 1), yy = iy + (q >> 1);
        v[q] = (xx < 0 || yy < 0 || xx >= ml || yy >= ma) ? 0 : m[yy * ml + xx];
    }
    int top = v[0] * (256 - wx) + v[1] * wx, bot = v[2] * (256 - wx) + v[3] * wx;
    return (top * (256 - wy) + bot * wy) >> 16;
}

typedef struct {
    int forma, iw, ih, ml, ma;
    int cx, cy, ro, th, phi;      /* arco */
    int amp, ht;                   /* onda, subida, perspectiva */
} geo_t;

static void geo_iniciar(geo_t *g, int forma, int iw, int ih, int ml, int ma)
{
    memset(g, 0, sizeof *g);
    g->forma = forma; g->iw = iw; g->ih = ih; g->ml = ml; g->ma = ma;
    g->ht = ih;
    if (forma == F_ARCO_ARR || forma == F_ARCO_ABA) {
        /* grosor del texto 40 % del alto; se busca el semiángulo que haga encajar el cuadro */
        int th = ih * 40 / 100;
        int mejor = 0, mejor_d = 1 << 30, ro_m = 0;
        for (int phi = 48; phi <= 256; phi += 8) {      /* 17 a 90 grados */
            int s = isin(phi), c = icos(phi);
            int ro = iw * 1024 / (2 * s);
            int h = (int)((long)ro * (1024 - c) / 1024 + (long)th * c / 1024);
            int d = h > ih ? h - ih : ih - h;
            if (d < mejor_d) { mejor_d = d; mejor = phi; ro_m = ro; }
        }
        if (ro_m < th + 4) ro_m = th + 4;
        g->phi = mejor; g->ro = ro_m; g->th = th;
        /* el centro del círculo: el borde superior del arco toca y = 0 */
        g->cx = iw / 2; g->cy = ro_m;
        int c = icos(mejor);
        int h = (int)((long)ro_m * (1024 - c) / 1024 + (long)th * c / 1024);
        if (h < ih) g->cy = ro_m + (ih - h) / 2;   /* si sobra alto, el arco se centra */
    } else if (forma == F_ONDA) {
        g->amp = ih * 18 / 100;
        g->ht = ih - 2 * g->amp;
    } else if (forma == F_SUBIDA) {
        g->amp = ih * 35 / 100;
        g->ht = ih - g->amp;
    } else if (forma == F_CURSIVA) {
        g->amp = ih * 3 / 10;                        /* desplazamiento horizontal total */
    }
}

/* Punto de la máscara (1/256 de píxel) de donde sale el píxel (x, y) del cuadro interior. false = fuera. */
static bool mapear(const geo_t *g, int x, int y, int *fx, int *fy)
{
    int ml = g->ml, ma = g->ma;
    switch (g->forma) {
    case F_PLANO:
        *fx = esc256(x, ml, g->iw); *fy = esc256(y, ma, g->ih);
        return true;
    case F_ARCO_ARR: case F_ARCO_ABA: {
        int yy = g->forma == F_ARCO_ABA ? g->ih - 1 - y : y;
        int dx = x - g->cx, dy = yy - g->cy;
        int rho = (int)raiz((uint32_t)(dx * dx + dy * dy));
        int ang = iatan2(dx, -dy);                  /* desde la vertical hacia arriba */
        if (ang < -g->phi || ang > g->phi) return false;
        int u256 = (int)((long)(ang + g->phi) * 256 / (2 * g->phi));   /* 0..256 */
        long v256 = (long)(g->ro - rho) * 256 / g->th;                  /* 0 arriba de las letras */
        if (v256 < 0 || v256 > 256) return false;
        if (g->forma == F_ARCO_ABA) v256 = 256 - v256;
        *fx = (int)((long)u256 * ml); *fy = (int)(v256 * ma);
        return true;
    }
    case F_ONDA: {
        int off = g->amp + g->amp * isin(x * 1536 / g->iw) / 1024;
        *fx = esc256(x, ml, g->iw); *fy = esc256(y - off, ma, g->ht);
        return true;
    }
    case F_SUBIDA: {
        int off = (int)((long)g->amp * (g->iw - x) / g->iw);
        *fx = esc256(x, ml, g->iw); *fy = esc256(y - off, ma, g->ht);
        return true;
    }
    case F_CURSIVA: {
        int w = g->iw - g->amp; if (w < 4) w = 4;
        int xs = x - (g->ih - 1 - y) * g->amp / g->ih;
        *fx = esc256(xs, ml, w); *fy = esc256(y, ma, g->ih);
        return true;
    }
    case F_PERS_D: case F_PERS_I: {
        int xx = g->forma == F_PERS_I ? g->iw - 1 - x : x;
        int h = g->ih * (100 - 55 * xx / g->iw) / 100;
        if (h < 4) h = 4;
        int top = (g->ih - h) / 2;
        *fx = esc256(x, ml, g->iw); *fy = esc256(y - top, ma, h);
        return true;
    }
    }
    return false;
}

/* ------------------------------------------------------------ color */

static uint32_t mezcla(uint32_t a, uint32_t b, int t)   /* t 0..255 : a -> b */
{
    int r = (int)(((a >> 16) & 255) * (255 - t) + ((b >> 16) & 255) * t) / 255;
    int g = (int)(((a >> 8) & 255) * (255 - t) + ((b >> 8) & 255) * t) / 255;
    int bl = (int)((a & 255) * (255 - t) + (b & 255) * t) / 255;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)bl;
}

static uint32_t arcoiris(int t)   /* t 0..255 : rojo a violeta */
{
    int seg = t * 6 / 256, f = (t * 6 % 256);
    int r = 0, g = 0, b = 0;
    switch (seg) {
    case 0: r = 255; g = f; break;
    case 1: r = 255 - f; g = 255; break;
    case 2: g = 255; b = f; break;
    case 3: g = 255 - f; b = 255; break;
    case 4: r = f; b = 255; break;
    default: r = 255; b = 255 - f; break;
    }
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static uint32_t relleno_en(const estilo_t *E, int x, int y, int iw, int ih)
{
    int tv = ih > 1 ? y * 255 / (ih - 1) : 0, th = iw > 1 ? x * 255 / (iw - 1) : 0;
    if (tv < 0) tv = 0;
    if (tv > 255) tv = 255;
    if (th < 0) th = 0;
    if (th > 255) th = 255;
    switch (E->relleno) {
    case R_VERT: return mezcla(E->c1, E->c2, tv);
    case R_HORIZ: return mezcla(E->c1, E->c2, th);
    case R_DIAG: return mezcla(E->c1, E->c2, (th + tv) / 2);
    case R_ARCOIRIS: return arcoiris(th * 5 / 6);
    case R_RAYAS: { int p = (x + y) / (ih / 8 + 3); return (p & 1) ? E->c2 : E->c1; }
    case R_METAL: { int d = tv * 2 - 255; if (d < 0) d = -d; return mezcla(E->c1, E->c2, 255 - d); }
    }
    return E->c1;
}

static void poner(uint32_t *p, uint32_t c, int alfa)
{
    if (alfa <= 0) return;
    if (alfa >= 255) { *p = c; return; }
    *p = mezcla(*p, c, alfa);
}

/* ------------------------------------------------------------ render */

bool wordart_renderizar(const char *texto, int estilo, int fam, int est, int l, int a, imagem_t *out)
{
    const estilo_t *E = &TAB[estilo >= 0 && estilo < WORDART_ESTILOS ? estilo : 0];
    if (l < 8) l = 8;
    if (a < 8) a = 8;
    if (l > 2400) l = 2400;
    if (a > 1600) a = 1600;
    if (!imagem_criar(out, l, a, 0xFFFFFF)) return false;
    int ow = E->cont_w ? E->cont_w * (1 + a / 70) : 0;
    int dep = E->prof3d ? E->prof3d * a / 13 + 1 : 0;
    int sh = E->sombra ? a / 18 + 2 : 0;
    int extra = dep > sh ? dep : sh;
    int ix = ow, iy = ow, iw = l - 2 * ow - extra, ih = a - 2 * ow - extra;
    if (iw < 8 || ih < 8) { ix = iy = 0; iw = l; ih = a; ow = 0; dep = sh = 0; }
    /* máscara del texto con un cuerpo parecido al grosor final de las letras */
    int grosor = ih;
    if (E->forma == F_ARCO_ARR || E->forma == F_ARCO_ABA) grosor = ih * 40 / 100;
    else if (E->forma == F_ONDA) grosor = ih * 64 / 100;
    else if (E->forma == F_SUBIDA) grosor = ih * 65 / 100;
    int px = grosor;
    if (px < 10) px = 10;
    uint8_t *m = NULL; int ml = 0, ma = 0;
    if (!obtener_mascara(texto, fam, est, px, &m, &ml, &ma)) { imagem_liberar(out); return false; }
    uint8_t *cov = aloca_zerado((size_t)l * (size_t)a);
    uint8_t *dil = aloca((size_t)l * (size_t)a);
    if (!cov || !dil) { libera(m); libera(cov); libera(dil); imagem_liberar(out); return false; }
    geo_t g;
    geo_iniciar(&g, E->forma, iw, ih, ml, ma);
    for (int y = 0; y < ih; y++)
        for (int x = 0; x < iw; x++) {
            int fx, fy;
            if (!mapear(&g, x, y, &fx, &fy)) continue;
            int v = muestra(m, ml, ma, fx, fy);
            cov[(iy + y) * l + ix + x] = (uint8_t)v;
        }
    libera(m);
    uint32_t *px32 = out->px;
    /* sombra: la misma silueta desplazada, mezclada al 55 % */
    if (sh) {
        for (int y = 0; y < a - sh; y++)
            for (int x = 0; x < l - sh; x++) {
                int c = cov[y * l + x];
                if (c) poner(&px32[(y + sh) * l + x + sh], E->c_sombra, c * 140 / 255);
            }
    }
    /* contorno: dilatación cuadrada (máximo separable) de radio ow */
    if (ow) {
        uint8_t *tmp = aloca((size_t)l * (size_t)a);
        if (tmp) {
            for (int y = 0; y < a; y++)
                for (int x = 0; x < l; x++) {
                    int mx = 0, x0 = x - ow < 0 ? 0 : x - ow, x1 = x + ow >= l ? l - 1 : x + ow;
                    for (int k = x0; k <= x1; k++) if (cov[y * l + k] > mx) mx = cov[y * l + k];
                    tmp[y * l + x] = (uint8_t)mx;
                }
            for (int y = 0; y < a; y++) {
                int y0 = y - ow < 0 ? 0 : y - ow, y1 = y + ow >= a ? a - 1 : y + ow;
                for (int x = 0; x < l; x++) {
                    int mx = 0;
                    for (int k = y0; k <= y1; k++) if (tmp[k * l + x] > mx) mx = tmp[k * l + x];
                    dil[y * l + x] = (uint8_t)mx;
                }
            }
            libera(tmp);
        } else memcpy(dil, cov, (size_t)l * (size_t)a);
    } else memcpy(dil, cov, (size_t)l * (size_t)a);
    /* extrusión 3D: copias de la silueta (con contorno) hacia abajo y a la derecha, de atrás hacia delante */
    if (dep) {
        for (int k = dep; k >= 1; k--) {
            uint32_t col = mezcla(E->c_3d, mezcla(E->c_3d, 0x000000, 90), 255 * k / dep / 2);
            for (int y = 0; y + k < a; y++)
                for (int x = 0; x + k < l; x++) {
                    int c = dil[y * l + x];
                    if (c) poner(&px32[(y + k) * l + x + k], col, c);
                }
        }
    }
    /* contorno y relleno */
    for (int y = 0; y < a; y++)
        for (int x = 0; x < l; x++) {
            int d = dil[y * l + x], c = cov[y * l + x];
            if (ow && d) poner(&px32[y * l + x], E->cont, d);
            if (c) poner(&px32[y * l + x], relleno_en(E, x - ix, y - iy, iw, ih), c);
        }
    libera(cov); libera(dil);
    return true;
}
