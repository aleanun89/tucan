// SPDX-License-Identifier: GPL-2.0-or-later
/* deflado_continuo.c - DEFLATE con el flujo abierto entre llamadas (zlib con
 * vaciado de sincronización), mejora 097 (D252). Ver compressao.h. Código propio.
 *
 * Cada llamada escribe un bloque de Huffman fijo (BTYPE=01) con las
 * coincidencias LZ77 encontradas en la ventana de 32 KB (que incluye lo
 * comprimido en las llamadas anteriores), el fin de bloque y un bloque vacío
 * «guardado» (00 00 FF FF) que deja la salida alineada a byte. Sin
 * dependencias de la libsagui salvo memset/memcpy (así se prueba en el anfitrión). */
#include "compressao.h"

#define MASC (ZFLUJO_VENTANA - 1)
#define CADENA 24      /* candidatos mirados por posición */
#define BUENA  64      /* una coincidencia así de larga ya no se mejora */

static const uint16_t BASE_LON[29] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
static const uint8_t  EXTRA_LON[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
static const uint16_t BASE_DIS[30] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
static const uint8_t  EXTRA_DIS[30] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };

typedef struct { uint8_t *s; int n, max; uint32_t acum; int nb; bool cheio; } escritor_t;

static void bits_poner(escritor_t *w, uint32_t v, int n)   /* LSB primero, como DEFLATE */
{
    w->acum |= v << w->nb;
    w->nb += n;
    while (w->nb >= 8) {
        if (w->n < w->max) w->s[w->n++] = (uint8_t)w->acum; else w->cheio = true;
        w->acum >>= 8;
        w->nb -= 8;
    }
}

static void bits_alinear(escritor_t *w) { if (w->nb) bits_poner(w, 0, 8 - w->nb); }

/* Los códigos de Huffman se escriben empezando por el bit más significativo. */
static void huff_poner(escritor_t *w, uint32_t cod, int n)
{
    uint32_t r = 0;
    for (int i = 0; i < n; i++) r |= ((cod >> i) & 1u) << (n - 1 - i);
    bits_poner(w, r, n);
}

static void simbolo_fijo(escritor_t *w, int s)   /* literal o longitud (0-287) con el árbol fijo */
{
    if (s < 144) huff_poner(w, 0x30u + (uint32_t)s, 8);
    else if (s < 256) huff_poner(w, 0x190u + (uint32_t)(s - 144), 9);
    else if (s < 280) huff_poner(w, (uint32_t)(s - 256), 7);
    else huff_poner(w, 0xC0u + (uint32_t)(s - 280), 8);
}

static int cod_longitud(int l)   /* 3..258 -> índice 0..28 */
{
    int c = 28;
    while (BASE_LON[c] > l) c--;
    return c;
}

static int cod_distancia(int d)
{
    int c = 29;
    while (BASE_DIS[c] > d) c--;
    return c;
}

void zflujo_iniciar(zflujo_t *z)
{
    memset(z, 0, sizeof *z);
}

int zflujo_limite(int n) { return n + n / 8 + 32; }

typedef struct { zflujo_t *z; const uint8_t *e; uint32_t base; int n; } vista_t;

static uint8_t byte_en(const vista_t *v, uint32_t p)
{
    uint32_t off = p - v->base;
    return off < (uint32_t)v->n ? v->e[off] : v->z->ventana[p & MASC];
}

static uint32_t hash3(const vista_t *v, uint32_t p)
{
    uint32_t x = (uint32_t)byte_en(v, p) | (uint32_t)byte_en(v, p + 1) << 8 | (uint32_t)byte_en(v, p + 2) << 16;
    return (x * 2654435761u) >> 17;   /* 15 bits */
}

static void insertar(const vista_t *v, uint32_t p)
{
    uint32_t h = hash3(v, p);
    v->z->anterior[p & MASC] = v->z->cabeza[h];
    v->z->cabeza[h] = p + 1;
}

int zflujo_comprimir(zflujo_t *z, const uint8_t *e, int n, uint8_t *sal, int max)
{
    escritor_t w = { sal, 0, max, 0, 0, false };
    if (!z->cabecera) { bits_poner(&w, 0x78, 8); bits_poner(&w, 0x01, 8); z->cabecera = true; }
    vista_t v = { z, e, z->pos, n };
    bits_poner(&w, 2, 3);   /* BFINAL = 0, BTYPE = 01 (Huffman fijo) */
    int i = 0;
    while (i < n) {
        uint32_t p = z->pos + (uint32_t)i;
        int mejor = 0, mdist = 0, resto = n - i;
        if (resto >= 3) {
            int maxl = resto > 258 ? 258 : resto;
            uint32_t cand = z->cabeza[hash3(&v, p)];
            uint32_t dprev = 0;
            for (int cadena = CADENA; cand && cadena > 0; cadena--) {
                uint32_t c = cand - 1, d = p - c;
                if (d == 0 || d >= (uint32_t)ZFLUJO_VENTANA || d <= dprev) break;
                dprev = d;
                int l = 0;
                if (byte_en(&v, c + (uint32_t)mejor) == e[i + mejor] || mejor == 0) {
                    while (l < maxl && byte_en(&v, c + (uint32_t)l) == e[i + l]) l++;
                    if (l > mejor) { mejor = l; mdist = (int)d; if (l >= BUENA || l == maxl) break; }
                }
                cand = z->anterior[c & MASC];
            }
        }
        int paso = 1;
        if (mejor >= 3) {
            int lc = cod_longitud(mejor), dc = cod_distancia(mdist);
            simbolo_fijo(&w, 257 + lc);
            bits_poner(&w, (uint32_t)(mejor - BASE_LON[lc]), EXTRA_LON[lc]);
            huff_poner(&w, (uint32_t)dc, 5);
            bits_poner(&w, (uint32_t)(mdist - BASE_DIS[dc]), EXTRA_DIS[dc]);
            paso = mejor;
        } else {
            simbolo_fijo(&w, e[i]);
        }
        for (int k = 0; k < paso; k++)
            if (n - (i + k) >= 3) insertar(&v, p + (uint32_t)k);
        i += paso;
    }
    simbolo_fijo(&w, 256);          /* fin de bloque */
    bits_poner(&w, 0, 3);           /* bloque vacío «guardado»: BFINAL = 0, BTYPE = 00 */
    bits_alinear(&w);
    bits_poner(&w, 0x0000, 16);     /* LEN = 0 */
    bits_poner(&w, 0xFFFF, 16);     /* NLEN */
    /* la ventana recibe lo nuevo (los últimos 32 KB) */
    int desde = n > ZFLUJO_VENTANA ? n - ZFLUJO_VENTANA : 0;
    for (int j = desde; j < n; j++) z->ventana[(z->pos + (uint32_t)j) & MASC] = e[j];
    z->pos += (uint32_t)n;
    return w.cheio ? COMP_ERRO_CHEIO : w.n;
}
