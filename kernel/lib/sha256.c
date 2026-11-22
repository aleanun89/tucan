// SPDX-License-Identifier: GPL-2.0-or-later
/* sha256.c - SHA-256 escrito a partir de FIPS 180-4 (secciones 4.1.2, 4.2.2,
 * 5.3.3 y 6.2), D060.
 *
 * Se compila dos veces: en el kernel (generador aleatorio, D061) y en la libtls de los
 * programas (TLS, certificados). No usa memcpy ni nada de libc/libsagui
 * para servir a los dos lados sin adaptación. No hay ramificación que dependa de
 * los datos: el tiempo solo depende del tamaño. */
#include "sha256.h"

static const unsigned int K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

#define ROTD(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static void comprimir(unsigned int h[8], const unsigned char *b)
{
    unsigned int w[64];
    for (int t = 0; t < 16; t++)
        w[t] = (unsigned int)b[4 * t] << 24 | (unsigned int)b[4 * t + 1] << 16 |
               (unsigned int)b[4 * t + 2] << 8 | b[4 * t + 3];
    for (int t = 16; t < 64; t++) {
        unsigned int s0 = ROTD(w[t - 15], 7) ^ ROTD(w[t - 15], 18) ^ (w[t - 15] >> 3);
        unsigned int s1 = ROTD(w[t - 2], 17) ^ ROTD(w[t - 2], 19) ^ (w[t - 2] >> 10);
        w[t] = w[t - 16] + s0 + w[t - 7] + s1;
    }
    unsigned int a = h[0], bb = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
    for (int t = 0; t < 64; t++) {
        unsigned int t1 = hh + (ROTD(e, 6) ^ ROTD(e, 11) ^ ROTD(e, 25)) + ((e & f) ^ (~e & g)) + K[t] + w[t];
        unsigned int t2 = (ROTD(a, 2) ^ ROTD(a, 13) ^ ROTD(a, 22)) + ((a & bb) ^ (a & c) ^ (bb & c));
        hh = g; g = f; f = e; e = d + t1;
        d = c; c = bb; bb = a; a = t1 + t2;
    }
    h[0] += a; h[1] += bb; h[2] += c; h[3] += d;
    h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
}

void sha256_iniciar(sha256_t *c)
{
    static const unsigned int H0[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    for (int i = 0; i < 8; i++) c->h[i] = H0[i];
    c->usados = 0;
    c->total = 0;
}

void sha256_somar(sha256_t *c, const void *dados, unsigned n)
{
    const unsigned char *p = dados;
    c->total += n;
    while (n) {
        if (c->usados == 0 && n >= SHA256_BLOCO) {   /* bloque entero: sin copiar */
            comprimir(c->h, p);
            p += SHA256_BLOCO;
            n -= SHA256_BLOCO;
            continue;
        }
        unsigned k = SHA256_BLOCO - c->usados;
        if (k > n) k = n;
        for (unsigned i = 0; i < k; i++) c->bloco[c->usados + i] = p[i];
        c->usados += k;
        p += k;
        n -= k;
        if (c->usados == SHA256_BLOCO) {
            comprimir(c->h, c->bloco);
            c->usados = 0;
        }
    }
}

void sha256_fim(sha256_t *c, unsigned char saida[SHA256_TAM])
{
    unsigned long long bits = c->total * 8;
    unsigned char pad = 0x80;
    sha256_somar(c, &pad, 1);
    pad = 0;
    while (c->usados != 56) sha256_somar(c, &pad, 1);
    unsigned char tam[8];
    for (int i = 0; i < 8; i++) tam[i] = (unsigned char)(bits >> (56 - 8 * i));
    sha256_somar(c, tam, 8);
    for (int i = 0; i < 8; i++) {
        saida[4 * i] = (unsigned char)(c->h[i] >> 24);
        saida[4 * i + 1] = (unsigned char)(c->h[i] >> 16);
        saida[4 * i + 2] = (unsigned char)(c->h[i] >> 8);
        saida[4 * i + 3] = (unsigned char)c->h[i];
    }
}

void sha256(const void *dados, unsigned n, unsigned char saida[SHA256_TAM])
{
    sha256_t c;
    sha256_iniciar(&c);
    sha256_somar(&c, dados, n);
    sha256_fim(&c, saida);
}
