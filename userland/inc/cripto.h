// SPDX-License-Identifier: GPL-2.0-or-later
/* cripto.h - primitivas criptográficas de libtls (D060).
 *
 * Todo escrito en SaguiOS a partir de las especificaciones (FIPS 180-4, FIPS
 * 197, SP 800-38D, RFC 2104, 5869, 5246, 7748, 8439, 8017, 8032, SEC1/FIPS 186);
 * ninguna línea viene de otra biblioteca. Las pruebas con los vectores oficiales
 * están en userland/testes/pruebacripto.c.
 *
 * Tiempo constante: AES (S-box calculada por aritmética en GF(2^8) "rebanada
 * en bits", sin tablas), GHASH, ChaCha20, Poly1305, X25519 y la
 * multiplicación de punto del ECDHE no tienen desvíos ni accesos a la memoria que
 * dependan de secretos; las comparaciones de etiquetas tampoco. Las comprobaciones
 * de firma (RSA, ECDSA) sólo tratan con datos públicos y no necesitan
 * eso. Límites en D060. */
#ifndef SAGUI_CRIPTO_H
#define SAGUI_CRIPTO_H
#include "sagui.h"
#include "sha256.h"

/* ---- utilidades ---- */
bool cripto_igual(const void *a, const void *b, unsigned n);   /* tiempo constante */
void cripto_apagar(void *p, unsigned n);                       /* pone a cero sin que el compilador lo quite */
/* Bytes aleatorios del kernel (SIS_ALEATORIO, D061). */
void cripto_aleatorio(void *buf, unsigned n);

/* ---- hashes ---- */
#define SHA384_TAM 48
#define SHA512_TAM 64
#define HASH_MAX   64
#define HASH_BLOCO_MAX 128
typedef struct {
    uint64_t h[8];
    uint8_t  bloco[128];
    unsigned usados;
    uint64_t total;
    unsigned tam;       /* 48 o 64 */
} sha512_t;
void sha512_iniciar(sha512_t *c);
void sha384_iniciar(sha512_t *c);
void sha512_somar(sha512_t *c, const void *d, unsigned n);
void sha512_fim(sha512_t *c, uint8_t *saida);   /* 48 o 64 bytes según el iniciar */

/* BLAKE2s (RFC 7693), D363: el hash del WireGuard. tam = bytes de salida (1-32);
 * con clave (hasta 32 bytes) es el MAC del WireGuard (16 bytes de salida). */
typedef struct {
    uint32_t h[8], t[2];
    uint8_t  bloco[64];
    unsigned usados, tam;
} blake2s_t;
void blake2s_iniciar(blake2s_t *c, unsigned tam);
void blake2s_iniciar_chave(blake2s_t *c, unsigned tam, const void *chave, unsigned nchave);
void blake2s_somar(blake2s_t *c, const void *dados, unsigned n);
void blake2s_fim(blake2s_t *c, uint8_t *saida);   /* borra el contexto */
void blake2s(uint8_t *saida, unsigned tam, const void *chave, unsigned nchave, const void *dados, unsigned n);

typedef union { sha256_t s256; sha512_t s512; blake2s_t b2s; } hash_ctx_t;
typedef struct {
    const char *nome;
    unsigned tam, bloco;
    void (*iniciar)(hash_ctx_t *c);
    void (*somar)(hash_ctx_t *c, const void *d, unsigned n);
    void (*fim)(hash_ctx_t *c, uint8_t *saida);
} hash_alg_t;
extern const hash_alg_t HASH_SHA256, HASH_SHA384, HASH_SHA512, HASH_BLAKE2S;
void hash_calcular(const hash_alg_t *h, const void *d, unsigned n, uint8_t *saida);

/* ---- HMAC (RFC 2104), HKDF (RFC 5869) y el PRF del TLS 1.2 (RFC 5246) ---- */
typedef struct {
    const hash_alg_t *h;
    hash_ctx_t interno, externo;
} hmac_t;
void hmac_iniciar(hmac_t *m, const hash_alg_t *h, const void *chave, unsigned n);
void hmac_somar(hmac_t *m, const void *d, unsigned n);
void hmac_fim(hmac_t *m, uint8_t *saida);
void hmac(const hash_alg_t *h, const void *chave, unsigned nc, const void *d, unsigned n, uint8_t *saida);
void hkdf_extrair(const hash_alg_t *h, const void *sal, unsigned ns, const void *ikm, unsigned ni, uint8_t *prk);
void hkdf_expandir(const hash_alg_t *h, const uint8_t *prk, const void *info, unsigned ninfo, uint8_t *saida, unsigned n);
void tls12_prf(const hash_alg_t *h, const void *segredo, unsigned ns, const char *rotulo,
               const void *semente, unsigned nsem, uint8_t *saida, unsigned n);

/* ---- AES (FIPS 197) y GCM (SP 800-38D) ---- */
typedef struct {
    uint8_t  rk[15][16];   /* claves de ronda */
    int      rodadas;      /* 10 o 14 */
} aes_t;
bool aes_chave(aes_t *a, const uint8_t *chave, unsigned n);   /* 16 o 32 bytes */
/* Cifra 1 o 2 bloques de una vez (el S-box rebanado procesa 32 bytes a la vez). */
void aes_cifrar(const aes_t *a, const uint8_t *entrada, uint8_t *saida, int blocos);
bool aes_hardware(void);              /* usando AES-NI (el CPUID tiene AES y SSE2) */
void aes_forcar_software(bool sim);   /* pruebas: compara las dos versiones */

typedef struct {
    aes_t    aes;
    uint32_t h[4];          /* H = AES(0), big-endian en 4 palabras */
} gcm_t;
bool gcm_chave(gcm_t *g, const uint8_t *chave, unsigned n);
void gcm_cifrar(const gcm_t *g, const uint8_t iv[12], const uint8_t *aad, unsigned naad,
                const uint8_t *entrada, uint8_t *saida, unsigned n, uint8_t tag[16]);
bool gcm_decifrar(const gcm_t *g, const uint8_t iv[12], const uint8_t *aad, unsigned naad,
                  const uint8_t *entrada, uint8_t *saida, unsigned n, const uint8_t tag[16]);

/* ---- ChaCha20 y Poly1305 (RFC 8439) ---- */
void chacha20_bloco(const uint8_t chave[32], uint32_t contador, const uint8_t nonce[12], uint8_t saida[64]);
void chacha20_xor(const uint8_t chave[32], uint32_t contador, const uint8_t nonce[12],
                  const uint8_t *entrada, uint8_t *saida, unsigned n);
void poly1305(const uint8_t chave[32], const uint8_t *msg, unsigned n, uint8_t tag[16]);
void chachapoly_cifrar(const uint8_t chave[32], const uint8_t nonce[12], const uint8_t *aad, unsigned naad,
                       const uint8_t *entrada, uint8_t *saida, unsigned n, uint8_t tag[16]);
bool chachapoly_decifrar(const uint8_t chave[32], const uint8_t nonce[12], const uint8_t *aad, unsigned naad,
                         const uint8_t *entrada, uint8_t *saida, unsigned n, const uint8_t tag[16]);
/* HChaCha20 y XChaCha20-Poly1305 (draft-irtf-cfrg-xchacha), D363: las cookies del WireGuard.
 * Nonce de 24 bytes; el resto, como chachapoly_*. */
void hchacha20(const uint8_t chave[32], const uint8_t nonce[16], uint8_t subchave[32]);
void xchachapoly_cifrar(const uint8_t chave[32], const uint8_t nonce[24], const uint8_t *aad, unsigned naad,
                        const uint8_t *entrada, uint8_t *saida, unsigned n, uint8_t tag[16]);
bool xchachapoly_decifrar(const uint8_t chave[32], const uint8_t nonce[24], const uint8_t *aad, unsigned naad,
                          const uint8_t *entrada, uint8_t *saida, unsigned n, const uint8_t tag[16]);

/* ---- X25519 (RFC 7748) ---- */
void x25519(uint8_t saida[32], const uint8_t escalar[32], const uint8_t ponto[32]);
void x25519_base(uint8_t saida[32], const uint8_t escalar[32]);

/* ---- Ed25519 (RFC 8032), D250: firmas del SSH ----
 * La clave privada es la semilla de 32 bytes; la pública, el punto comprimido.
 * La firma no tiene saltos ni índices que dependan de la semilla (escalera con
 * intercambio por máscara); la verificación solo ve datos públicos. */
void ed25519_publica(uint8_t publica[32], const uint8_t semente[32]);
void ed25519_assinar(uint8_t assin[64], const void *msg, unsigned n, const uint8_t semente[32], const uint8_t publica[32]);
bool ed25519_verificar(const uint8_t assin[64], const void *msg, unsigned n, const uint8_t publica[32]);

/* ---- chacha20-poly1305@openssh.com (PROTOCOL.chacha20poly1305 de OpenSSH), D250 ----
 * chave: 64 bytes (los 32 primeros cifran el paquete; los 32 últimos, su
 * tamaño). seq: el número de secuencia del paquete. Un paquete cifrado es
 * tamaño(4) || cuerpo(n) || etiqueta(16). */
uint32_t ssh_chapoly_tamanho(const uint8_t chave[64], uint32_t seq, const uint8_t cifrado[4]);
/* Cifra en el sitio pkt[0..4+n) (tamaño en claro big-endian + cuerpo) y pone la etiqueta en pkt[4+n..]. */
void ssh_chapoly_selar(const uint8_t chave[64], uint32_t seq, uint8_t *pkt, unsigned n);
/* Comprueba la etiqueta de pkt (4 + n + 16 bytes) y, solo si vale, descifra el cuerpo en el sitio. */
bool ssh_chapoly_abrir(const uint8_t chave[64], uint32_t seq, uint8_t *pkt, unsigned n);

/* ---- RSA (RFC 8017): sólo verificación; módulos de 1024 a 4096 bits ---- */
#define RSA_MAX_BYTES 512
bool rsa_verificar_pkcs1(const uint8_t *n, unsigned nn, const uint8_t *e, unsigned ne,
                         const hash_alg_t *h, const uint8_t *hash, const uint8_t *assin, unsigned na);
/* sal: tamaño exigido en bytes, o -1 para aceptar cualquiera */
bool rsa_verificar_pss(const uint8_t *n, unsigned nn, const uint8_t *e, unsigned ne,
                       const hash_alg_t *h, const uint8_t *hash, const uint8_t *assin, unsigned na, int sal);

/* ---- curvas elípticas P-256 y P-384 (SEC1, FIPS 186-4) ---- */
#define CURVA_P256 23   /* los números del TLS (NamedGroup) */
#define CURVA_P384 24
unsigned ec_tam(int curva);   /* bytes de una coordenada: 32 o 48; 0 = curva desconocida */
/* punto: 0x04 || x || y. r y s: enteros big-endian (hasta ec_tam bytes, sin ceros a la izquierda obligatorios). */
bool ecdsa_verificar(int curva, const uint8_t *ponto, unsigned np, const uint8_t *hash, unsigned nh,
                     const uint8_t *r, unsigned nr, const uint8_t *s, unsigned ns);
/* ECDHE: genera la clave (privada aleatoria) y calcula el secreto (la coordenada x). */
bool ecdh_gerar(int curva, uint8_t *privada, uint8_t *publica);   /* publica: 1 + 2*tam bytes */
bool ecdh_segredo(int curva, const uint8_t *privada, const uint8_t *publica_outro, unsigned np, uint8_t *segredo);
/* Sólo para las pruebas: multiplica un escalar dado (sin aleatoriedad). */
bool ec_multiplicar_base(int curva, const uint8_t *escalar, uint8_t *publica);

#endif
