// SPDX-License-Identifier: GPL-2.0-or-later
/* inflado_continuo.c - INFLATE con el flujo abierto entre llamadas (el lector
 * del zflujo_* de deflado_continuo.c), mejora 097 fase 2 (D254). Ver compressao.h.
 *
 * Es lo que piden ZRLE y Tight del RFB: un único flujo zlib por conexión (cuatro
 * en Tight) cuyo diccionario, los últimos 32 KB descomprimidos, se conserva de un
 * rectángulo al siguiente. Cada llamada recibe un trozo completo que termina en
 * un vaciado de sincronización (el bloque guardado vacío 00 00 FF FF, alineado a
 * byte), así que basta con decodificar bloques hasta que se acaba la entrada. Las
 * referencias hacia atrás que caen antes del principio del trozo se resuelven
 * con la ventana guardada.
 *
 * Código propio a partir de la RFC 1950/1951. Sin dependencias de la libsagui
 * salvo memcpy/memmove/memset (así se prueba en el anfitrión). */
#include "compressao.h"

#define RAPIDO 9   /* bits de la tabla de búsqueda directa */

static const uint16_t BASE_LON[29] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
static const uint8_t  EXTRA_LON[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
static const uint16_t BASE_DIS[30] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
static const uint8_t  EXTRA_DIS[30] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };
static const uint8_t  ORDEN_CL[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

typedef struct {
    const uint8_t *ent;
    int      n, pos;
    uint32_t bits;
    int      qtd;
    bool     roto;   /* se pidieron más bits de los que hay */
} lector_t;

typedef struct {
    uint16_t contagem[16];
    uint16_t simbolo[288];
    uint16_t rapida[1 << RAPIDO];   /* (longitud << 12 | símbolo); 0 = código largo */
} huff_t;

static void llenar(lector_t *b)
{
    while (b->qtd <= 24 && b->pos < b->n) { b->bits |= (uint32_t)b->ent[b->pos++] << b->qtd; b->qtd += 8; }
}

static int bits_leer(lector_t *b, int n)
{
    if (n == 0) return 0;
    llenar(b);
    if (b->qtd < n) { b->roto = true; return 0; }
    int v = (int)(b->bits & ((1u << n) - 1));
    b->bits >>= n;
    b->qtd -= n;
    return v;
}

static uint32_t invertir(uint32_t v, int n)
{
    uint32_t r = 0;
    while (n--) { r = r << 1 | (v & 1); v >>= 1; }
    return r;
}

/* Tabla canónica a partir de las longitudes; < 0 si sobran códigos. */
static int construir(huff_t *h, const uint8_t *len, int n)
{
    uint16_t desp[16];
    uint32_t sig[16];
    memset(h->contagem, 0, sizeof h->contagem);
    for (int s = 0; s < n; s++) h->contagem[len[s]]++;
    h->contagem[0] = 0;
    int resto = 1;
    for (int l = 1; l < 16; l++) {
        resto <<= 1;
        resto -= h->contagem[l];
        if (resto < 0) return -1;
    }
    desp[1] = 0;
    for (int l = 1; l < 15; l++) desp[l + 1] = (uint16_t)(desp[l] + h->contagem[l]);
    for (int s = 0; s < n; s++) if (len[s]) h->simbolo[desp[len[s]]++] = (uint16_t)s;
    memset(h->rapida, 0, sizeof h->rapida);
    uint32_t cod = 0;
    for (int l = 1; l < 16; l++) { cod = (cod + h->contagem[l - 1]) << 1; sig[l] = cod; }
    for (int s = 0; s < n; s++) {
        int l = len[s];
        if (!l || l > RAPIDO) continue;
        uint32_t r = invertir(sig[l]++, l);
        for (uint32_t k = r; k < (1u << RAPIDO); k += 1u << l) h->rapida[k] = (uint16_t)(l << 12 | s);
    }
    return 0;
}

static int decodificar(lector_t *b, const huff_t *h)
{
    llenar(b);
    uint16_t r = h->rapida[b->bits & ((1u << RAPIDO) - 1)];
    if (r) {
        int l = r >> 12;
        if (l > b->qtd) { b->roto = true; return -1; }
        b->bits >>= l;
        b->qtd -= l;
        return r & 0xFFF;
    }
    int codigo = 0, primero = 0, indice = 0;
    for (int l = 1; l < 16; l++) {
        codigo |= bits_leer(b, 1);
        if (b->roto) return -1;
        int c = h->contagem[l];
        if (codigo - primero < c) return h->simbolo[indice + codigo - primero];
        indice += c;
        primero = (primero + c) << 1;
        codigo <<= 1;
    }
    return -1;
}

static void tablas_fijas(huff_t *hc, huff_t *hd)
{
    uint8_t len[288];
    int s = 0;
    for (; s < 144; s++) len[s] = 8;
    for (; s < 256; s++) len[s] = 9;
    for (; s < 280; s++) len[s] = 7;
    for (; s < 288; s++) len[s] = 8;
    construir(hc, len, 288);
    for (s = 0; s < 30; s++) len[s] = 5;
    construir(hd, len, 30);
}

void zinflar_iniciar(zinflar_t *z)
{
    memset(z, 0, sizeof *z);
}

/* Guarda en la ventana los últimos 32 KB de lo producido. */
static void ventana_poner(zinflar_t *z, const uint8_t *s, int k)
{
    if (k >= ZINFLAR_VENTANA) {
        memcpy(z->ventana, s + k - ZINFLAR_VENTANA, ZINFLAR_VENTANA);
        z->n = ZINFLAR_VENTANA;
        return;
    }
    if (z->n + (uint32_t)k > ZINFLAR_VENTANA) {
        uint32_t sobra = z->n + (uint32_t)k - ZINFLAR_VENTANA;
        memmove(z->ventana, z->ventana + sobra, z->n - sobra);
        z->n -= sobra;
    }
    memcpy(z->ventana + z->n, s, (size_t)k);
    z->n += (uint32_t)k;
}

int zinflar(zinflar_t *z, const uint8_t *ent, int n_ent, uint8_t *saida, int max)
{
    huff_t hc, hd;
    lector_t b = { ent, n_ent, 0, 0, 0, false };
    uint8_t len[320];
    int k = 0, ultimo = 0;
    if (!z->cabecera) {
        if (n_ent < 2) return COMP_ERRO_TRUNCADO;
        if ((ent[0] & 15) != 8 || ((ent[0] << 8) | ent[1]) % 31 != 0 || (ent[1] & 0x20)) return COMP_ERRO_FORMATO;
        b.pos = 2;
        z->cabecera = true;
    }
    while (!ultimo && (n_ent - b.pos) * 8 + b.qtd >= 8) {
        ultimo = bits_leer(&b, 1);
        int tipo = bits_leer(&b, 2);
        if (b.roto) return COMP_ERRO_TRUNCADO;
        if (tipo == 0) {   /* guardado: alinear a byte y copiar */
            b.bits >>= b.qtd & 7;
            b.qtd -= b.qtd & 7;
            int l = bits_leer(&b, 16), nl = bits_leer(&b, 16);
            if (b.roto) return COMP_ERRO_TRUNCADO;
            if ((l ^ 0xFFFF) != nl) return COMP_ERRO_FORMATO;
            b.pos -= b.qtd / 8;   /* los bytes enteros que ya estaban en el acumulador */
            b.bits = 0; b.qtd = 0;
            if (b.pos + l > n_ent) return COMP_ERRO_TRUNCADO;
            if (k + l > max) return COMP_ERRO_CHEIO;
            memcpy(saida + k, ent + b.pos, (size_t)l);
            b.pos += l;
            k += l;
            continue;
        }
        if (tipo == 1) tablas_fijas(&hc, &hd);
        else if (tipo == 2) {
            int nlit = bits_leer(&b, 5) + 257, ndis = bits_leer(&b, 5) + 1, ncod = bits_leer(&b, 4) + 4;
            if (nlit > 286 || ndis > 30) return COMP_ERRO_FORMATO;
            for (int i = 0; i < 19; i++) len[ORDEN_CL[i]] = 0;
            for (int i = 0; i < ncod; i++) len[ORDEN_CL[i]] = (uint8_t)bits_leer(&b, 3);
            if (b.roto) return COMP_ERRO_TRUNCADO;
            if (construir(&hc, len, 19) < 0) return COMP_ERRO_FORMATO;
            int i = 0;
            while (i < nlit + ndis) {
                int sim = decodificar(&b, &hc), rep, val = 0;
                if (sim < 0) return b.roto ? COMP_ERRO_TRUNCADO : COMP_ERRO_FORMATO;
                if (sim < 16) { len[i++] = (uint8_t)sim; continue; }
                if (sim == 16) { if (!i) return COMP_ERRO_FORMATO; val = len[i - 1]; rep = 3 + bits_leer(&b, 2); }
                else if (sim == 17) rep = 3 + bits_leer(&b, 3);
                else rep = 11 + bits_leer(&b, 7);
                if (i + rep > nlit + ndis) return COMP_ERRO_FORMATO;
                while (rep--) len[i++] = (uint8_t)val;
            }
            if (b.roto) return COMP_ERRO_TRUNCADO;
            if (len[256] == 0) return COMP_ERRO_FORMATO;
            if (construir(&hc, len, nlit) < 0 || construir(&hd, len + nlit, ndis) < 0) return COMP_ERRO_FORMATO;
        } else return COMP_ERRO_FORMATO;
        for (;;) {
            int sim = decodificar(&b, &hc);
            if (sim < 0) return b.roto ? COMP_ERRO_TRUNCADO : COMP_ERRO_FORMATO;
            if (sim < 256) {
                if (k >= max) return COMP_ERRO_CHEIO;
                saida[k++] = (uint8_t)sim;
            } else if (sim == 256) break;
            else {
                sim -= 257;
                if (sim >= 29) return COMP_ERRO_FORMATO;
                int l = BASE_LON[sim] + bits_leer(&b, EXTRA_LON[sim]);
                int ds = decodificar(&b, &hd);
                if (ds < 0 || ds >= 30) return b.roto ? COMP_ERRO_TRUNCADO : COMP_ERRO_FORMATO;
                int dist = BASE_DIS[ds] + bits_leer(&b, EXTRA_DIS[ds]);
                if (b.roto) return COMP_ERRO_TRUNCADO;
                if (dist > k + (int)z->n) return COMP_ERRO_FORMATO;
                if (k + l > max) return COMP_ERRO_CHEIO;
                for (int i = 0; i < l; i++, k++) {
                    int o = k - dist;
                    saida[k] = o >= 0 ? saida[o] : z->ventana[(int)z->n + o];
                }
            }
        }
    }
    ventana_poner(z, saida, k);
    return k;
}
