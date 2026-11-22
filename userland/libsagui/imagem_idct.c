// SPDX-License-Identifier: GPL-2.0-or-later
/* imagem_idct.c - la IDCT entera y la conversión de color, compartidas por el
 * JPEG (la mejora 047) y por el MPEG-1 (la mejora 048).
 *
 * La IDCT es la misma en los dos: es la transformada inversa de la DCT de 8x8
 * de la norma JPEG (ISO/IEC 10918-1), que el MPEG-1 (ISO/IEC 11172-2) también
 * usa. El camino en coma flotante no sirve aquí: la libsagui se compila sin
 * coma flotante (-mgeneral-regs-only), por eso la IDCT es toda de enteros. */
#include "imagem.h"

/* Coseno de la IDCT: idct_cos[u][x] = 2048 * c(u) * cos((2x+1)*u*pi/16),
 * con c(0) = 1/sqrt(2) y c(u) = 1 para u > 0. */
static const int16_t idct_cos[8][8] = {
    {  1448,  1448,  1448,  1448,  1448,  1448,  1448,  1448 },
    {  2009,  1703,  1138,   400,  -400, -1138, -1703, -2009 },
    {  1892,   784,  -784, -1892, -1892,  -784,   784,  1892 },
    {  1703,  -400, -2009, -1138,  1138,  2009,   400, -1703 },
    {  1448, -1448, -1448,  1448,  1448, -1448, -1448,  1448 },
    {  1138, -2009,   400,  1703, -1703,  -400,  2009, -1138 },
    {   784, -1892,  1892,  -784,  -784,  1892, -1892,   784 },
    {   400, -1138,  1703, -2009,  2009, -1703,  1138,  -400 }
};

/* Orden en zigzag: el coeficiente k de la secuencia va al índice natural
 * imagem_zigzag[k]. Es la misma tabla en el JPEG y en el MPEG-1. */
const uint8_t imagem_zigzag[64] = {
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

/* IDCT entera de un bloque de 8x8. Los coeficientes están en orden natural
 * (coef[v*8+u]) y el DC ya se ha quitado (vale 0 aquí): quien llama lo suma al
 * final, porque el DC pesa lo mismo en todos los píxeles del bloque.
 *
 * La salida es CON SIGNO: la parte AC de un bloque puede ser negativa (y el
 * valor final sólo queda entre 0 y 255 después de sumar el DC y el nivel 128).
 * Limitar aquí estropearía la imagen. */
void imagem_idct(const int32_t *coef, int32_t *saida, int passo)
{
    int32_t p[64];   /* primera pasada (columnas) */
    int32_t t[64];

    /* Columnas: p(u,y) = suma_v coef[v*8+u] * cos[v][y]. La suma vale
     * 2048 * 2 * f1D, o sea 4096 * f1D; con >>12 queda exactamente la IDCT de
     * una dimensión. */
    for (int u = 0; u < 8; u++) {
        for (int y = 0; y < 8; y++) {
            int32_t s = 0;
            for (int v = 0; v < 8; v++) s += coef[v * 8 + u] * idct_cos[v][y];
            p[y * 8 + u] = s >> 12;
        }
    }

    /* Filas: cada término con un desplazamiento, para no desbordar el int32 en
     * el peor caso. La suma final vale 1024 * f, y f = t >> 10. */
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int32_t s = 0;
            for (int u = 0; u < 8; u++)
                s += (p[y * 8 + u] * idct_cos[u][x]) >> 2;
            t[y * 8 + x] = s;
        }
    }

    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++)
            saida[y * passo + x] = (t[y * 8 + x] + 512) >> 10;
}

/* Convierte YCbCr en RGB, en punto fijo (el mismo que usa el JPEG). */
uint32_t imagem_ycbcr(int32_t y, int32_t cb, int32_t cr)
{
    int32_t cb2 = cb - 128, cr2 = cr - 128;
    int32_t r = y + ((91881 * cr2) >> 16);
    int32_t g = y - ((22554 * cb2 + 46802 * cr2) >> 16);
    int32_t b = y + ((116130 * cb2) >> 16);
    if (r < 0) r = 0; else if (r > 255) r = 255;
    if (g < 0) g = 0; else if (g > 255) g = 255;
    if (b < 0) b = 0; else if (b > 255) b = 255;
    return (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
}
