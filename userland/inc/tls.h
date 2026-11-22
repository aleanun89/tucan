// SPDX-License-Identifier: GPL-2.0-or-later
/* tls.h - certificados X.509, el almacén de raíces y el cliente TLS 1.2/1.3 (D062, D063).
 *
 * Usado por curl, wget, certificados y Cipó. Todo en userland/libtls/. */
#ifndef SAGUI_TLS_H
#define SAGUI_TLS_H
#include "cripto.h"

/* ---- X.509 (RFC 5280) ---- */
typedef struct { const uint8_t *p; unsigned n; } fatia_t;

/* algoritmos de firma (de los certificados y del TLS) */
#define ASSIN_RSA_SHA256  1
#define ASSIN_RSA_SHA384  2
#define ASSIN_RSA_SHA512  3
#define ASSIN_PSS_SHA256  4
#define ASSIN_PSS_SHA384  5
#define ASSIN_PSS_SHA512  6
#define ASSIN_ECDSA_SHA256 7
#define ASSIN_ECDSA_SHA384 8
#define ASSIN_ECDSA_SHA512 9
#define ASSIN_RSA_SHA1    10   /* reconocido sólo para mostrar (raíces antiguas); nunca aceptado */

#define CHAVE_RSA 1
#define CHAVE_EC  2

/* keyUsage (bits en el orden del RFC 5280, 4.2.1.3) */
#define USO_ASSINATURA   0x80   /* digitalSignature */
#define USO_CERT         0x04   /* keyCertSign */

typedef struct {
    const uint8_t *der;      /* apunta al certificado entero (no es copia) */
    unsigned nder;
    fatia_t tbs;             /* la parte firmada */
    int     alg;             /* ASSIN_*; 0 = desconocido */
    fatia_t assinatura;
    fatia_t serial, emissor, sujeito;   /* Name en DER crudo (para comparar) */
    uint64_t antes, depois;  /* AAAAMMDDhhmmss (UTC) */
    int     tipo_chave;      /* CHAVE_RSA o CHAVE_EC */
    int     curva;           /* CURVA_P256 / CURVA_P384 */
    unsigned bits;           /* tamaño de la clave */
    fatia_t rsa_n, rsa_e, ec_ponto, spki;
    bool    tem_basico, eh_ca;
    int     caminho_max;     /* pathLenConstraint; -1 = sin límite */
    bool    tem_uso;
    unsigned uso;            /* USO_* (primer byte del BIT STRING) */
    bool    tem_eku, eku_servidor;   /* extKeyUsage presente / incluye serverAuth (o any) */
    fatia_t san;             /* GeneralNames (contenido de la SEQUENCE) */
    int     versao;
} cert_t;

/* Códigos de error de la validación (mensajes en x509_erro_texto) */
#define X509_OK            0
#define X509_FORMATO      -1
#define X509_CADUCADO     -2
#define X509_AINDA_NAO    -3
#define X509_NOME         -4
#define X509_EMISSOR      -5
#define X509_ASSINATURA   -6
#define X509_NAO_CA       -7
#define X509_USO          -8
#define X509_ALGORITMO    -9
#define X509_CAMINHO     -10
#define X509_CRITICA     -11
#define X509_DATA_SISTEMA -12

int  x509_ler(cert_t *c, const uint8_t *der, unsigned n);          /* 0 o X509_* */
const char *x509_erro_texto(int erro);
/* "CN=..., O=..., C=..." en Latin-1 */
void x509_nome_texto(fatia_t nome, char *s, unsigned max);
/* Sólo el CN (o el O, si no hay CN) */
void x509_nome_curto(fatia_t nome, char *s, unsigned max);
void x509_data_texto(uint64_t d, char *s, unsigned max);          /* "2026-11-07 09:10" */
void x509_huella(const cert_t *c, char *s, unsigned max);          /* SHA-256 en hexa con ":" */
const char *x509_alg_texto(int alg);
/* Lista los nombres DNS/IP del SAN: llama a f para cada uno */
void x509_san_listar(const cert_t *c, void (*f)(void *ctx, const char *nome), void *ctx);
bool x509_nome_confere(const cert_t *c, const char *host);
/* Verifica que "c" fue firmado con la clave de "emissor" */
bool x509_assinado_por(const cert_t *c, const cert_t *emissor);
/* Verifica una firma con la clave pública del certificado (usado también
 * por el TLS: CertificateVerify e ServerKeyExchange). esquema = ASSIN_*. */
bool x509_verificar(const cert_t *c, int esquema, const uint8_t *dados, unsigned n,
                    const uint8_t *assin, unsigned na);
/* PEM: busca el siguiente "-----BEGIN CERTIFICATE-----" a partir de *pos y
 * decodifica a "der" (hasta max). Devuelve el tamaño, 0 = terminó, <0 = error. */
int  pem_proximo(const char *texto, unsigned n, unsigned *pos, uint8_t *der, unsigned max);
uint64_t data_agora(void);   /* reloj del sistema como AAAAMMDDhhmmss */
/* Detalles de un certificado, una línea por campo, con "prefijo" delante. */
void x509_mostrar(int fd, const cert_t *c, const char *prefixo);

/* ---- almacén de raíces (D063) ---- */
#define RAIZES_PASTA       "/etc/ssl/certs"
#define RAIZES_PERSISTENTE "/home/.certificados"   /* si /home es un disco montado */
#define RAIZES_MAX 64
typedef struct {
    int      n;
    cert_t   c[RAIZES_MAX];
    char     arquivo[RAIZES_MAX][96];   /* de dónde vino */
} almacen_t;
int  almacen_carregar(almacen_t *a);   /* lee las carpetas; devuelve cuántos */
bool almacen_persistente(void);        /* /home es un sistema de archivos grabable montado */

/* Validación de la cadena enviada por el servidor: cadeia[0] es el certificado del
 * servidor. "usado" (puede ser NULL) recibe el índice de la raíz usada. */
int  x509_validar(const cert_t *cadeia, int n, const almacen_t *raizes, const char *host,
                  uint64_t agora, int *raiz_usada);

/* ---- cliente TLS ---- */
#define TLS12 0x0303
#define TLS13 0x0304

typedef struct {
    const char *host;              /* SNI y nombre a comprobar (IP: comprueba el SAN de IP) */
    bool inseguro;                 /* -k: no valida la cadena (pero el resto continúa) */
    const almacen_t *raizes;
    int  versao_min, versao_max;   /* 0 = TLS 1.2 a 1.3 */
    const char *alpn;              /* "http/1.1" o NULL */
} tls_config_t;

typedef struct tls tls_t;

/* Errores: valores <0 */
#define TLS_ERRO_REDE       -100   /* la conexión se cayó */
#define TLS_ERRO_PROTOCOLO  -101   /* mensaje inesperado o mal formado */
#define TLS_ERRO_ALERTA     -102   /* el servidor envió una alerta */
#define TLS_ERRO_CERT       -103   /* validación del certificado */
#define TLS_ERRO_CRIPTO     -104   /* firma, Finished o etiqueta erróneas */
#define TLS_ERRO_VERSAO     -105   /* sin versión ni suite en común */
#define TLS_ERRO_MEMORIA    -106

tls_t *tls_novo(const tls_config_t *cfg, int fd);
int    tls_conectar(tls_t *t);                         /* hace el handshake; 0 = ok */
int    tls_ler(tls_t *t, void *buf, unsigned n);       /* bytes; 0 = fin; <0 = error */
int    tls_escrever(tls_t *t, const void *buf, unsigned n);
void   tls_fechar(tls_t *t);                           /* close_notify y libera (no cierra el fd) */
const char *tls_erro(const tls_t *t);                  /* texto del último error, en español */
/* Información para -v e "certificados probar" */
const char *tls_versao_texto(const tls_t *t);          /* "TLSv1.3" */
const char *tls_suite_texto(const tls_t *t);
const char *tls_grupo_texto(const tls_t *t);
const char *tls_alpn(const tls_t *t);                  /* "" se el servidor no eligió */
int    tls_cadeia(const tls_t *t, const cert_t **certs);   /* certificados recibidos */
int    tls_validacao(const tls_t *t);                  /* resultado X509_* (incluso con -k) */
int    tls_raiz(const tls_t *t);                       /* índice en el almacén, -1 si ninguno */
unsigned tls_ms_handshake(const tls_t *t);
int    tls_pendente(const tls_t *t);                   /* bytes descifrados a la espera de tls_ler */

/* ---- conexión HTTP o HTTPS (curl, wget, Cipó) ---- */
typedef struct {
    int    fd;
    tls_t *tls;          /* NULL = HTTP sin cifrar */
    char   erro[200];
} conexao_t;
/* Abre TCP y, si la URL es https, hace el handshake con "cfg" (host = el de la URL).
 * Devuelve 0 o un error (<0): el texto queda en c->erro. Con TLS_ERRO_CERT la
 * conexión queda abierta para quien quiera mostrar la cadena (tls_cadeia). */
int  conexao_abrir(conexao_t *c, const url_t *u, const tls_config_t *cfg);
int  conexao_ler(void *c, void *buf, unsigned n);           /* un http_ler_t */
int  conexao_escrever(conexao_t *c, const void *buf, unsigned n);
void conexao_fechar(conexao_t *c);
/* Carga el almacén predeterminado (una vez) y rellena cfg; cacert: archivo PEM extra (o NULL). */
bool tls_config_padrao(tls_config_t *cfg, const char *cacert, char *erro, unsigned max);

#endif
