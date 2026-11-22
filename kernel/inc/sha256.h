// SPDX-License-Identifier: GPL-2.0-or-later
/* sha256.h - SHA-256 (FIPS 180-4), D060.
 *
 * El mismo archivo (kernel/lib/sha256.c) sirve al kernel, que mezcla la
 * entropía del generador aleatorio (D061), y a la libtls de los programas. Por eso
 * no depende de ninguna otra cabecera: solo de los tipos básicos del C. */
#ifndef SAGUI_SHA256_H
#define SAGUI_SHA256_H

#define SHA256_TAM   32
#define SHA256_BLOCO 64

typedef struct {
    unsigned int       h[8];
    unsigned char      bloco[SHA256_BLOCO];
    unsigned int       usados;     /* bytes en "bloque" */
    unsigned long long total;      /* bytes procesados */
} sha256_t;

void sha256_iniciar(sha256_t *c);
void sha256_somar(sha256_t *c, const void *dados, unsigned n);
void sha256_fim(sha256_t *c, unsigned char saida[SHA256_TAM]);   /* "c" puede reiniciarse después */
void sha256(const void *dados, unsigned n, unsigned char saida[SHA256_TAM]);

#endif
