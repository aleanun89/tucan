// SPDX-License-Identifier: GPL-2.0-or-later
/* hash.c - SHA-384/512 (FIPS 180-4, 6.4 e 6.5), a tabela de hashes, HMAC
 * (RFC 2104), HKDF (RFC 5869) e o PRF do TLS 1.2 (RFC 5246, 5). D060.
 *
 * O SHA-256 está em kernel/lib/sha256.c: o mesmo código serve ao kernel
 * (gerador aleatório) e aos programas. */
#include "cripto.h"

/* ------------------------------------------------------------ utilidades */

bool cripto_igual(const void *a, const void *b, unsigned n)
{
    const volatile uint8_t *x = a, *y = b;
    uint8_t d = 0;
    for (unsigned i = 0; i < n; i++) d |= x[i] ^ y[i];   /* sem sair cedo: o tempo não diz onde diferiu */
    return d == 0;
}

void cripto_apagar(void *p, unsigned n)
{
    volatile uint8_t *v = p;
    while (n--) *v++ = 0;
}

void cripto_aleatorio(void *buf, unsigned n)
{
    uint8_t *p = buf;
    while (n) {
        int r = bytes_aleatorios(p, n > 4096 ? 4096 : n);
        if (r <= 0) {   /* kernel sem a chamada: não há como continuar com segurança */
            imprimir_em(ERROS, "cripto: el núcleo no da números aleatorios\n");
            sair(70);
        }
        p += r;
        n -= (unsigned)r;
    }
}

/* ------------------------------------------------------------ SHA-512 */

static const uint64_t K512[80] = {
    0x428a2f98d728ae22ull, 0x7137449123ef65cdull, 0xb5c0fbcfec4d3b2full, 0xe9b5dba58189dbbcull,
    0x3956c25bf348b538ull, 0x59f111f1b605d019ull, 0x923f82a4af194f9bull, 0xab1c5ed5da6d8118ull,
    0xd807aa98a3030242ull, 0x12835b0145706fbeull, 0x243185be4ee4b28cull, 0x550c7dc3d5ffb4e2ull,
    0x72be5d74f27b896full, 0x80deb1fe3b1696b1ull, 0x9bdc06a725c71235ull, 0xc19bf174cf692694ull,
    0xe49b69c19ef14ad2ull, 0xefbe4786384f25e3ull, 0x0fc19dc68b8cd5b5ull, 0x240ca1cc77ac9c65ull,
    0x2de92c6f592b0275ull, 0x4a7484aa6ea6e483ull, 0x5cb0a9dcbd41fbd4ull, 0x76f988da831153b5ull,
    0x983e5152ee66dfabull, 0xa831c66d2db43210ull, 0xb00327c898fb213full, 0xbf597fc7beef0ee4ull,
    0xc6e00bf33da88fc2ull, 0xd5a79147930aa725ull, 0x06ca6351e003826full, 0x142929670a0e6e70ull,
    0x27b70a8546d22ffcull, 0x2e1b21385c26c926ull, 0x4d2c6dfc5ac42aedull, 0x53380d139d95b3dfull,
    0x650a73548baf63deull, 0x766a0abb3c77b2a8ull, 0x81c2c92e47edaee6ull, 0x92722c851482353bull,
    0xa2bfe8a14cf10364ull, 0xa81a664bbc423001ull, 0xc24b8b70d0f89791ull, 0xc76c51a30654be30ull,
    0xd192e819d6ef5218ull, 0xd69906245565a910ull, 0xf40e35855771202aull, 0x106aa07032bbd1b8ull,
    0x19a4c116b8d2d0c8ull, 0x1e376c085141ab53ull, 0x2748774cdf8eeb99ull, 0x34b0bcb5e19b48a8ull,
    0x391c0cb3c5c95a63ull, 0x4ed8aa4ae3418acbull, 0x5b9cca4f7763e373ull, 0x682e6ff3d6b2b8a3ull,
    0x748f82ee5defb2fcull, 0x78a5636f43172f60ull, 0x84c87814a1f0ab72ull, 0x8cc702081a6439ecull,
    0x90befffa23631e28ull, 0xa4506cebde82bde9ull, 0xbef9a3f7b2c67915ull, 0xc67178f2e372532bull,
    0xca273eceea26619cull, 0xd186b8c721c0c207ull, 0xeada7dd6cde0eb1eull, 0xf57d4f7fee6ed178ull,
    0x06f067aa72176fbaull, 0x0a637dc5a2c898a6ull, 0x113f9804bef90daeull, 0x1b710b35131c471bull,
    0x28db77f523047d84ull, 0x32caab7b40c72493ull, 0x3c9ebe0a15c9bebcull, 0x431d67c49c100d4cull,
    0x4cc5d4becb3e42b6ull, 0x597f299cfc657e2aull, 0x5fcb6fab3ad6faecull, 0x6c44198c4a475817ull,
};

#define ROTD64(x, n) (((x) >> (n)) | ((x) << (64 - (n))))

static void comprimir512(uint64_t h[8], const uint8_t *b)
{
    uint64_t w[80];
    for (int t = 0; t < 16; t++) {
        uint64_t v = 0;
        for (int k = 0; k < 8; k++) v = v << 8 | b[8 * t + k];
        w[t] = v;
    }
    for (int t = 16; t < 80; t++) {
        uint64_t s0 = ROTD64(w[t - 15], 1) ^ ROTD64(w[t - 15], 8) ^ (w[t - 15] >> 7);
        uint64_t s1 = ROTD64(w[t - 2], 19) ^ ROTD64(w[t - 2], 61) ^ (w[t - 2] >> 6);
        w[t] = w[t - 16] + s0 + w[t - 7] + s1;
    }
    uint64_t a = h[0], bb = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
    for (int t = 0; t < 80; t++) {
        uint64_t t1 = hh + (ROTD64(e, 14) ^ ROTD64(e, 18) ^ ROTD64(e, 41)) + ((e & f) ^ (~e & g)) + K512[t] + w[t];
        uint64_t t2 = (ROTD64(a, 28) ^ ROTD64(a, 34) ^ ROTD64(a, 39)) + ((a & bb) ^ (a & c) ^ (bb & c));
        hh = g; g = f; f = e; e = d + t1;
        d = c; c = bb; bb = a; a = t1 + t2;
    }
    h[0] += a; h[1] += bb; h[2] += c; h[3] += d;
    h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
}

void sha512_iniciar(sha512_t *c)
{
    static const uint64_t H0[8] = { 0x6a09e667f3bcc908ull, 0xbb67ae8584caa73bull, 0x3c6ef372fe94f82bull,
                                    0xa54ff53a5f1d36f1ull, 0x510e527fade682d1ull, 0x9b05688c2b3e6c1full,
                                    0x1f83d9abfb41bd6bull, 0x5be0cd19137e2179ull };
    memcpy(c->h, H0, sizeof H0);
    c->usados = 0;
    c->total = 0;
    c->tam = 64;
}

void sha384_iniciar(sha512_t *c)
{
    static const uint64_t H0[8] = { 0xcbbb9d5dc1059ed8ull, 0x629a292a367cd507ull, 0x9159015a3070dd17ull,
                                    0x152fecd8f70e5939ull, 0x67332667ffc00b31ull, 0x8eb44a8768581511ull,
                                    0xdb0c2e0d64f98fa7ull, 0x47b5481dbefa4fa4ull };
    memcpy(c->h, H0, sizeof H0);
    c->usados = 0;
    c->total = 0;
    c->tam = 48;
}

void sha512_somar(sha512_t *c, const void *dados, unsigned n)
{
    const uint8_t *p = dados;
    c->total += n;
    while (n) {
        if (c->usados == 0 && n >= 128) {
            comprimir512(c->h, p);
            p += 128;
            n -= 128;
            continue;
        }
        unsigned k = 128 - c->usados;
        if (k > n) k = n;
        memcpy(c->bloco + c->usados, p, k);
        c->usados += k;
        p += k;
        n -= k;
        if (c->usados == 128) {
            comprimir512(c->h, c->bloco);
            c->usados = 0;
        }
    }
}

void sha512_fim(sha512_t *c, uint8_t *saida)
{
    uint64_t bits = c->total * 8;   /* mensagens < 2^61 bytes: a metade alta do tamanho de 128 bits é zero */
    uint8_t pad[144];
    unsigned np = (c->usados < 112 ? 112 : 240) - c->usados;
    memset(pad, 0, sizeof pad);
    pad[0] = 0x80;
    for (int i = 0; i < 8; i++) pad[np + 8 + i] = (uint8_t)(bits >> (56 - 8 * i));
    sha512_somar(c, pad, np + 16);
    for (unsigned i = 0; i < c->tam; i++) saida[i] = (uint8_t)(c->h[i / 8] >> (56 - 8 * (i % 8)));
}

/* ------------------------------------------------------------ tabela */

static void i256(hash_ctx_t *c) { sha256_iniciar(&c->s256); }
static void s256(hash_ctx_t *c, const void *d, unsigned n) { sha256_somar(&c->s256, d, n); }
static void f256(hash_ctx_t *c, uint8_t *o) { sha256_fim(&c->s256, o); }
static void i384(hash_ctx_t *c) { sha384_iniciar(&c->s512); }
static void i512(hash_ctx_t *c) { sha512_iniciar(&c->s512); }
static void s512(hash_ctx_t *c, const void *d, unsigned n) { sha512_somar(&c->s512, d, n); }
static void f512(hash_ctx_t *c, uint8_t *o) { sha512_fim(&c->s512, o); }

const hash_alg_t HASH_SHA256 = { "SHA-256", 32, 64, i256, s256, f256 };
const hash_alg_t HASH_SHA384 = { "SHA-384", 48, 128, i384, s512, f512 };
const hash_alg_t HASH_SHA512 = { "SHA-512", 64, 128, i512, s512, f512 };

void hash_calcular(const hash_alg_t *h, const void *d, unsigned n, uint8_t *saida)
{
    hash_ctx_t c;
    h->iniciar(&c);
    h->somar(&c, d, n);
    h->fim(&c, saida);
}

/* ------------------------------------------------------------ HMAC */

void hmac_iniciar(hmac_t *m, const hash_alg_t *h, const void *chave, unsigned n)
{
    uint8_t k[HASH_BLOCO_MAX], pad[HASH_BLOCO_MAX];
    memset(k, 0, sizeof k);
    if (n > h->bloco) hash_calcular(h, chave, n, k);   /* chave longa: usa o hash dela */
    else memcpy(k, chave, n);
    m->h = h;
    for (unsigned i = 0; i < h->bloco; i++) pad[i] = k[i] ^ 0x36;
    h->iniciar(&m->interno);
    h->somar(&m->interno, pad, h->bloco);
    for (unsigned i = 0; i < h->bloco; i++) pad[i] = k[i] ^ 0x5c;
    h->iniciar(&m->externo);
    h->somar(&m->externo, pad, h->bloco);
    cripto_apagar(k, sizeof k);
    cripto_apagar(pad, sizeof pad);
}

void hmac_somar(hmac_t *m, const void *d, unsigned n) { m->h->somar(&m->interno, d, n); }

void hmac_fim(hmac_t *m, uint8_t *saida)
{
    uint8_t t[HASH_MAX];
    m->h->fim(&m->interno, t);
    m->h->somar(&m->externo, t, m->h->tam);
    m->h->fim(&m->externo, saida);
    cripto_apagar(t, sizeof t);
}

void hmac(const hash_alg_t *h, const void *chave, unsigned nc, const void *d, unsigned n, uint8_t *saida)
{
    hmac_t m;
    hmac_iniciar(&m, h, chave, nc);
    hmac_somar(&m, d, n);
    hmac_fim(&m, saida);
    cripto_apagar(&m, sizeof m);
}

/* ------------------------------------------------------------ HKDF */

void hkdf_extrair(const hash_alg_t *h, const void *sal, unsigned ns, const void *ikm, unsigned ni, uint8_t *prk)
{
    uint8_t zeros[HASH_MAX];
    if (!sal || !ns) {   /* sem sal: HashLen zeros (RFC 5869, 2.2) */
        memset(zeros, 0, sizeof zeros);
        sal = zeros;
        ns = h->tam;
    }
    hmac(h, sal, ns, ikm, ni, prk);
}

void hkdf_expandir(const hash_alg_t *h, const uint8_t *prk, const void *info, unsigned ninfo, uint8_t *saida, unsigned n)
{
    uint8_t t[HASH_MAX];
    unsigned nt = 0;
    for (uint8_t i = 1; n; i++) {
        hmac_t m;
        hmac_iniciar(&m, h, prk, h->tam);
        hmac_somar(&m, t, nt);
        hmac_somar(&m, info, ninfo);
        hmac_somar(&m, &i, 1);
        hmac_fim(&m, t);
        nt = h->tam;
        unsigned k = n < nt ? n : nt;
        memcpy(saida, t, k);
        saida += k;
        n -= k;
    }
    cripto_apagar(t, sizeof t);
}

/* ------------------------------------------------------------ PRF do TLS 1.2 */

/* P_hash(segredo, rótulo + semente): A(i) = HMAC(segredo, A(i-1)),
 * saída = HMAC(segredo, A(1) + rótulo + semente) || HMAC(segredo, A(2) + ...) ... */
void tls12_prf(const hash_alg_t *h, const void *segredo, unsigned ns, const char *rotulo,
               const void *semente, unsigned nsem, uint8_t *saida, unsigned n)
{
    uint8_t a[HASH_MAX], t[HASH_MAX];
    unsigned nr = (unsigned)tam_texto(rotulo);
    hmac_t m;
    hmac_iniciar(&m, h, segredo, ns);
    hmac_somar(&m, rotulo, nr);
    hmac_somar(&m, semente, nsem);
    hmac_fim(&m, a);   /* A(1) */
    while (n) {
        hmac_iniciar(&m, h, segredo, ns);
        hmac_somar(&m, a, h->tam);
        hmac_somar(&m, rotulo, nr);
        hmac_somar(&m, semente, nsem);
        hmac_fim(&m, t);
        unsigned k = n < h->tam ? n : h->tam;
        memcpy(saida, t, k);
        saida += k;
        n -= k;
        hmac(h, segredo, ns, a, h->tam, a);   /* A(i+1) */
    }
    cripto_apagar(a, sizeof a);
    cripto_apagar(t, sizeof t);
    cripto_apagar(&m, sizeof m);
}
