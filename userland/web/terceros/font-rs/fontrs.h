// SPDX-License-Identifier: Apache-2.0
/* fontrs.h - rasterizador de contornos por área acumulada.
 *
 * CÓDIGO DE TERCEIROS (D262). El algoritmo y la forma de estas funciones vienen del
 * font-rs de Raph Levien (https://github.com/raphlinus/font-rs, Copyright 2015 Google
 * Inc.), con licencia Apache-2.0 (texto en LICENSE, en esta carpeta; no hay fichero
 * NOTICE). No es una copia literal: una IA lo reprodujo de memoria en C (primero para el
 * Cipó de robson-apk, D141) y por eso se trata como derivado del font-rs. Ver ORIGEM.md
 * para lo que cambió (Apache-2.0, sección 4: los ficheros de esta carpeta están
 * modificados respecto al original).
 *
 * La idea: cada segmento del contorno suma, en cada píxel por el que pasa, la parte del
 * área (con signo, según el sentido del segmento) que queda a la derecha en esa fila; una
 * suma corriente por fila da la cobertura exacta de cada píxel (0 fuera, 1 dentro).
 *
 * Es un fichero de cabecera con funciones "static inline" para no tocar el Makefile: lo
 * incluyen userland/libfontes/fonte.c (glifos TrueType y CFF) y userland/web/canvas2d.c
 * (caminos del Canvas y del SVG). Comportamiento idéntico al que tenían esas funciones
 * antes de separarlas.
 */
#ifndef TERCEIROS_FONTRS_H
#define TERCEIROS_FONTRS_H

static inline float fontrs_piso(float v) { int i = (int)v; return (float)(v < (float)i ? i - 1 : i); }
static inline float fontrs_teto(float v) { int i = (int)v; return (float)(v > (float)i ? i + 1 : i); }
static inline float fontrs_abs(float v) { return v < 0 ? -v : v; }
static inline double fontrs_dpiso(double x) { int i = (int)x; return (double)(x < i ? i - 1 : i); }
static inline double fontrs_dteto(double x) { int i = (int)x; return (double)(x > i ? i + 1 : i); }
static inline double fontrs_dmin(double a, double b) { return a < b ? a : b; }
static inline double fontrs_dmax(double a, double b) { return a > b ? a : b; }

/* Fuentes: acumulador de w x h floats; segmento en píxeles del acumulador. */
static inline void fontrs_linha(float *acc, int w, int h, float x0, float y0, float x1, float y1)
{
    if (fontrs_abs(y0 - y1) < 1e-6f) return;
    float dir = 1.0f;
    if (y0 > y1) { float t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; dir = -1.0f; }
    float dxdy = (x1 - x0) / (y1 - y0);
    float x = x0;
    int ya = y0 < 0 ? 0 : (int)y0;
    if (y0 < 0) x -= y0 * dxdy;
    int yb = (int)fontrs_teto(y1); if (yb > h) yb = h;
    for (int y = ya; y < yb; y++) {
        float *l = acc + y * w;
        float lo = (float)y > y0 ? (float)y : y0, hi = (float)(y + 1) < y1 ? (float)(y + 1) : y1;
        float dy = hi - lo;
        float xn = x + dxdy * dy, d = dy * dir;
        float xa = x < xn ? x : xn, xb = x < xn ? xn : x;
        if (xa < 0) xa = 0;
        if (xb < 0) xb = 0;
        float xaf = fontrs_piso(xa); int xai = (int)xaf;
        float xbc = fontrs_teto(xb); int xbi = (int)xbc;
        if (xai < 0 || xbi >= w) { x = xn; continue; }
        if (xbi <= xai + 1) {
            float xm = 0.5f * (x + xn) - xaf;
            l[xai] += d - d * xm;
            l[xai + 1] += d * xm;
        } else {
            float s = 1.0f / (xb - xa);
            float x0f = xa - xaf;
            float a0 = 0.5f * s * (1.0f - x0f) * (1.0f - x0f);
            float x1f = xb - xbc + 1.0f;
            float am = 0.5f * s * x1f * x1f;
            l[xai] += d * a0;
            if (xbi == xai + 2) l[xai + 1] += d * (1.0f - a0 - am);
            else {
                float a1 = s * (1.5f - x0f);
                l[xai + 1] += d * (a1 - a0);
                for (int xi = xai + 2; xi < xbi - 1; xi++) l[xi] += d * s;
                float a2 = a1 + (float)(xbi - xai - 3) * s;
                l[xbi - 1] += d * (1.0f - a2 - am);
            }
            l[xbi] += d * am;
        }
        x = xn;
    }
}

/* Suma corriente por fila (regla "distinto de cero"): cobertura 0..255. */
static inline void fontrs_acumular(const float *acc, int w, int h, unsigned char *cob)
{
    for (int y = 0; y < h; y++) {
        float s = 0;
        for (int x = 0; x < w; x++) {
            s += acc[y * w + x];
            float v = fontrs_abs(s);
            if (v > 1) v = 1;
            cob[y * w + x] = (unsigned char)(v * 255.0f + 0.5f);
        }
    }
}

/* Canvas: variante que recorta a la caja del dibujo (lo que pasa por la izquierda cuenta
 * en la columna 0; por la derecha desaparece). Fila del acumulador: l + 2 floats.
 * Coordenadas ya relativas a la caja. */
static inline void fontrs_linha_recortada(float *acc, int passo, int l, int a, double x0, double y0, double x1, double y1)
{
    if (y0 == y1) return;
    double dir = 1;
    if (y0 > y1) { double t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; dir = -1; }
    if (y1 <= 0 || y0 >= a) return;
    double dxdy = (x1 - x0) / (y1 - y0);
    double x = x0;
    if (y0 < 0) { x -= y0 * dxdy; y0 = 0; }
    if (y1 > a) y1 = a;
    for (int y = (int)y0; y < a && y < y1; y++) {
        float *lin = acc + y * passo;
        double dy = fontrs_dmin(y + 1, y1) - fontrs_dmax(y, y0);
        double xn = x + dxdy * dy, d = dy * dir;
        double a0x = x < xn ? x : xn, a1x = x < xn ? xn : x;
        if (a1x <= 0) { lin[0] += (float)d; x = xn; continue; }
        if (a0x >= l) { x = xn; continue; }
        if (a0x < 0) {   /* parte a la izquierda: vertical en el borde, el resto como está */
            double f = -a0x / (a1x - a0x);
            lin[0] += (float)(d * f);
            d -= d * f; a0x = 0;
        }
        if (a1x > l) a1x = l;
        double fl = fontrs_dpiso(a0x); int x0i = (int)fl;
        double ce = fontrs_dteto(a1x); int x1i = (int)ce;
        if (x1i <= x0i + 1) {
            double xm = 0.5 * (a0x + a1x) - fl;
            lin[x0i] += (float)(d - d * xm);
            lin[x0i + 1] += (float)(d * xm);
        } else {
            double s = 1 / (a1x - a0x), x0f = a0x - fl, a0 = 0.5 * s * (1 - x0f) * (1 - x0f);
            double x1f = a1x - ce + 1, am = 0.5 * s * x1f * x1f;
            lin[x0i] += (float)(d * a0);
            if (x1i == x0i + 2) lin[x0i + 1] += (float)(d * (1 - a0 - am));
            else {
                double a1 = s * (1.5 - x0f);
                lin[x0i + 1] += (float)(d * (a1 - a0));
                for (int xi = x0i + 2; xi < x1i - 1; xi++) lin[xi] += (float)(d * s);
                double a2 = a1 + (x1i - x0i - 3) * s;
                lin[x1i - 1] += (float)(d * (1 - a2 - am));
            }
            lin[x1i] += (float)(d * am);
        }
        x = xn;
    }
}

/* Suma corriente con la regla elegida: 0 = distinto de cero, 1 = par o impar
 * (fill-rule: evenodd del Canvas y del SVG). Máscara l x a, 0..255. */
static inline void fontrs_acumular_regra(const float *acc, int passo, int l, int a, int parimpar, unsigned char *m)
{
    for (int y = 0; y < a; y++) {
        float s = 0; const float *lin = acc + y * passo;
        for (int x = 0; x < l; x++) {
            s += lin[x];
            double c = s < 0 ? -s : s;
            if (parimpar) { c -= 2 * fontrs_dpiso(c / 2); if (c > 1) c = 2 - c; }
            else if (c > 1) c = 1;
            m[y * l + x] = (unsigned char)(c * 255 + 0.5);
        }
    }
}

#endif
