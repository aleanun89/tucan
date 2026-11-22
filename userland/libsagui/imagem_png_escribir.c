// SPDX-License-Identifier: GPL-2.0-or-later
/* imagem_png_escribir.c - codificador de PNG en memoria (mejora 122, D296).
 *
 * RGB de 8 bits, filtro «Sub» en todas las filas (comprime mejor que sin filtro con
 * degradados y letras) y flujo zlib con deflar(). Sirve para incrustar imágenes en el RTF
 * (\pngblip) y para las que no se pueden guardar tal cual. Sin coma flotante. */
#include "imagem.h"
#include "compressao.h"

static void be32(uint8_t *d, uint32_t v) { d[0] = (uint8_t)(v >> 24); d[1] = (uint8_t)(v >> 16); d[2] = (uint8_t)(v >> 8); d[3] = (uint8_t)v; }

/* Escribe un trozo PNG (longitud, tipo, datos, CRC) en d; devuelve los bytes escritos. */
static int trozo(uint8_t *d, const char *tipo, const uint8_t *datos, int n)
{
    be32(d, (uint32_t)n);
    memcpy(d + 4, tipo, 4);
    if (n) memcpy(d + 8, datos, (size_t)n);
    uint32_t c = crc32_somar(0, d + 4, (size_t)n + 4);
    be32(d + 8 + n, c);
    return n + 12;
}

int imagem_codificar_png(const imagem_t *img, uint8_t **dados, int *tam)
{
    if (!img || !img->px || img->larg <= 0 || img->alt <= 0) return -1;
    long fila = 1L + 3L * img->larg;
    long bruto = fila * img->alt;
    if (bruto > 0x3FFFFFFF) return -1;
    uint8_t *raw = aloca((size_t)bruto);
    if (!raw) return -1;
    for (int y = 0; y < img->alt; y++) {
        uint8_t *f = raw + fila * y;
        const uint32_t *p = img->px + (size_t)y * (size_t)img->larg;
        f[0] = 1;   /* Sub */
        uint8_t ant[3] = { 0, 0, 0 };
        for (int x = 0; x < img->larg; x++) {
            uint8_t c[3] = { (uint8_t)(p[x] >> 16), (uint8_t)(p[x] >> 8), (uint8_t)p[x] };
            for (int k = 0; k < 3; k++) { f[1 + 3 * x + k] = (uint8_t)(c[k] - ant[k]); ant[k] = c[k]; }
        }
    }
    int lim = deflar_limite((int)bruto) + 16;
    uint8_t *z = aloca((size_t)lim);
    if (!z) { libera(raw); return -1; }
    z[0] = 0x78; z[1] = 0x9C;
    int r = deflar(raw, (int)bruto, z + 2, lim - 6, 6);
    if (r < 0) { libera(raw); libera(z); return -1; }
    int nz = 2 + r;
    uint32_t ad = adler32_somar(1, raw, (size_t)bruto);
    be32(z + nz, ad); nz += 4;
    libera(raw);
    uint8_t *o = aloca((size_t)nz + 8 + 25 + 12 + 12);
    if (!o) { libera(z); return -1; }
    static const uint8_t firma[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    memcpy(o, firma, 8);
    int n = 8;
    uint8_t ihdr[13];
    be32(ihdr, (uint32_t)img->larg); be32(ihdr + 4, (uint32_t)img->alt);
    ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
    n += trozo(o + n, "IHDR", ihdr, 13);
    n += trozo(o + n, "IDAT", z, nz);
    n += trozo(o + n, "IEND", NULL, 0);
    libera(z);
    *dados = o;
    *tam = n;
    return 0;
}
