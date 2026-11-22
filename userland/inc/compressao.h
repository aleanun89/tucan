// SPDX-License-Identifier: GPL-2.0-or-later
/* compressao.h - compressão e empacotamento da libsagui (mejora 056, D103):
 * CRC-32, Adler-32, DEFLATE (RFC 1951), gzip (RFC 1952), tar (ustar + extensões
 * do GNU/pax na leitura) e zip (APPNOTE da PKWARE: stored e deflate, sem
 * cifrado nem ZIP64). Tudo trabalha em memória: o chamador entrega o arquivo
 * inteiro e recebe o resultado inteiro. */
#ifndef SAGUI_COMPRESSAO_H
#define SAGUI_COMPRESSAO_H

#include "sagui.h"

/* Erros (sempre negativos, para não se confundirem com tamanhos). */
#define COMP_ERRO_FORMATO       -100   /* dados corrompidos ou não são do formato */
#define COMP_ERRO_CHEIO         -101   /* a saída não cabe no buffer dado */
#define COMP_ERRO_CRC           -102   /* a soma de verificação não confere */
#define COMP_ERRO_TRUNCADO      -103   /* o arquivo acaba antes da hora */
#define COMP_ERRO_NAO_SUPORTADO -104   /* cifrado, ZIP64 ou método de compressão desconhecido */
#define COMP_ERRO_MEMORIA       -105
#define COMP_ERRO_GRANDE        -106   /* passa do limite de segurança de tamanho */
const char *comp_nome_erro(int erro);

#define COMP_LIMITE_MAX (512u * 1024 * 1024)   /* o que descomprimimos no máximo (contra bombas) */

/* ---- somas de verificação (começar com 0 e 1) ---- */
uint32_t crc32_somar(uint32_t crc, const void *dados, size_t n);
uint32_t adler32_somar(uint32_t adler, const void *dados, size_t n);

/* ---- DEFLATE cru ---- */
/* Descomprime. Devolve os bytes escritos ou um COMP_ERRO_*; *usados (opcional)
 * diz quantos bytes da entrada o fluxo ocupou (para o que vem depois dele). */
int  inflar_usados(const uint8_t *ent, int n_ent, uint8_t *saida, int max_saida, int *usados);
int  inflar(const uint8_t *ent, int n_ent, uint8_t *saida, int max_saida);
/* Igual, pero *produzidos dice cuantos bytes salieron aunque se devuelva un
 * error (COMP_ERRO_CHEIO = no cabe; TRUNCADO o FORMATO = flujo roto). */
int  inflar_parcial(const uint8_t *ent, int n_ent, uint8_t *saida, int max_saida, int *produzidos);
/* Comprime: nivel 0 (só guardar), 1 (rápido) a 9 (menor). Devolve os bytes
 * escritos ou COMP_ERRO_CHEIO; deflar_limite(n) é um tamanho de saída sempre suficiente. */
int  deflar(const uint8_t *ent, int n, uint8_t *saida, int max_saida, int nivel);
int  deflar_limite(int n);

/* ---- DEFLATE continuo (mejora 097, D252) ---- */
/* Un flujo zlib (RFC 1950) que sigue abierto entre llamadas y conserva la
 * ventana de 32 KB: cada llamada comprime un trozo y lo cierra con un vaciado
 * (Z_SYNC_FLUSH, termina en un límite de byte), de modo que el receptor, con un
 * único inflate que nunca se reinicia (zlib.decompressobj), lo descomprime
 * trozo a trozo. Es lo que exige la codificación ZRLE del RFB. Usa códigos de
 * Huffman fijos y búsqueda de coincidencias en toda la ventana. El estado ocupa
 * unos 300 KB: reservarlo con aloca. */
#define ZFLUJO_VENTANA 32768
typedef struct {
    uint8_t  ventana[ZFLUJO_VENTANA];
    uint32_t cabeza[ZFLUJO_VENTANA];    /* hash de 3 bytes -> última posición + 1 */
    uint32_t anterior[ZFLUJO_VENTANA];  /* cadena de posiciones con el mismo hash */
    uint32_t pos;                       /* bytes ya comprimidos (con vuelta a 0 a los 4 GB) */
    bool     cabecera;                  /* ya se escribió la cabecera zlib */
} zflujo_t;
void zflujo_iniciar(zflujo_t *z);
int  zflujo_limite(int n);              /* tamaño de salida siempre suficiente para n bytes de entrada */
/* Comprime n bytes, los añade al flujo y devuelve los bytes escritos en sal, o COMP_ERRO_CHEIO. */
int  zflujo_comprimir(zflujo_t *z, const uint8_t *ent, int n, uint8_t *sal, int max);

/* Lo mismo para leer (mejora 097 fase 2, D254): el INFLATE de un flujo zlib que sigue
 * abierto entre llamadas y conserva los últimos 32 KB descomprimidos como diccionario.
 * Cada llamada recibe un trozo completo que acaba en un vaciado de sincronización (lo que
 * produce zflujo_comprimir o zlib con Z_SYNC_FLUSH); la cabecera zlib solo se espera en el
 * primero. Devuelve los bytes escritos en saida o un COMP_ERRO_*. Si falla, el flujo ya no
 * sirve: hay que reiniciarlo. zinflar_iniciar también lo reinicia (Tight pide reiniciar flujos). */
#define ZINFLAR_VENTANA 32768
typedef struct {
    uint8_t  ventana[ZINFLAR_VENTANA];
    uint32_t n;            /* bytes válidos de la ventana */
    bool     cabecera;     /* ya se leyó la cabecera zlib */
} zinflar_t;
void zinflar_iniciar(zinflar_t *z);
int  zinflar(zinflar_t *z, const uint8_t *ent, int n, uint8_t *saida, int max);

/* ---- gzip ---- */
/* Escreve cabeçalho (com nome, se não for NULL), fluxo e rodapé; devolve o tamanho. */
int  gzip_comprimir(const uint8_t *ent, int n, const char *nome, uint32_t mtime, int nivel,
                    uint8_t *saida, int max_saida);
/* Descomprime todos os membros seguidos; *saida é um buffer novo (liberar com libera).
 * nome (opcional, NOME_MAX) recebe o nome original do primeiro membro, se houver. */
int  gzip_descomprimir(const uint8_t *ent, int n, uint8_t **saida, int *tam, char *nome, int max_nome);
bool eh_gzip(const uint8_t *d, int n);

/* ---- buffer crescente ---- */
typedef struct { uint8_t *d; int n, cap; } buf_t;
int  comp_buf_somar(buf_t *b, const void *dados, int n);   /* 0 ou COMP_ERRO_MEMORIA */
void comp_buf_liberar(buf_t *b);

/* ---- tar ---- */
typedef struct {
    char     nome[1024];
    char     tipo;           /* '0' arquivo, '5' pasta, outro = ignorado ('2' symlink...) */
    uint32_t tamanho, mtime, modo;
    const uint8_t *dados;    /* dentro do buffer do tar */
} tar_entrada_t;
/* Escreve uma entrada (cabeçalho + dados + enchimento). nome em UTF-8. */
int  tar_adicionar(buf_t *b, const char *nome, const uint8_t *dados, uint32_t n, uint32_t mtime,
                   uint32_t modo, bool pasta);
int  tar_concluir(buf_t *b);   /* os dois blocos de zeros finais */
/* Lê a próxima entrada a partir de *pos: 1 = entrada, 0 = fim, < 0 = erro. */
int  tar_proximo(const uint8_t *buf, int n, int *pos, tar_entrada_t *e);

/* ---- zip ---- */
typedef struct {
    char    *nome;           /* UTF-8, como no arquivo */
    uint32_t crc, tc, tu, desloc, attr;
    uint16_t metodo, flags, hora, data;   /* hora e data no formato do DOS */
    bool     pasta;
    uint32_t mtime_ut;       /* D341: UTC do campo extra "UT" (0x5455) do Info-ZIP; 0 = não tem */
} zip_ent_t;

typedef struct { buf_t b; zip_ent_t *e; int n, cap; } zip_escritor_t;
/* nivel 0 = guardar; senão deflate (se não encolher, guarda). Devolve 0 ou erro. */
int  zip_adicionar(zip_escritor_t *z, const char *nome, const uint8_t *dados, uint32_t n,
                   uint32_t mtime, int nivel, bool pasta, uint32_t modo);
int  zip_concluir(zip_escritor_t *z);   /* grava o diretório central; o resultado fica em z->b */
void zip_escritor_liberar(zip_escritor_t *z);
typedef struct { const uint8_t *d; int n; zip_ent_t *e; int cnt; } zip_leitor_t;
int  zip_abrir(zip_leitor_t *z, const uint8_t *d, int n);   /* 0 ou erro */
int  zip_extrair(const zip_leitor_t *z, int i, uint8_t **saida);   /* buffer novo com tu bytes (+1 de folga) */
void zip_leitor_liberar(zip_leitor_t *z);

/* ---- utilidades de arquivos e nomes (para tar, zip e quem mais precisar) ---- */
uint8_t *carregar_arquivo(const char *caminho, int *n);   /* o arquivo inteiro (libera com libera) ou NULL */
uint8_t *carregar_descritor(int fd, int *n);              /* tudo até o fim (para stdin) */
int      salvar_arquivo(const char *caminho, const void *dados, int n);   /* 0 ou erro do sistema */
int      criar_pasta_p(const char *caminho);         /* como mkdir -p */
/* Transforma o nome de um arquivo empacotado em caminho relativo seguro dentro
 * de destino. false = perigoso (sai do destino por "..") ou vazio. */
bool     caminho_seguro(const char *nome, const char *destino, char *saida, int max);
int      nome_para_utf8(const char *src, char *dst, int max);
int      nome_de_utf8(const char *src, char *dst, int max);   /* o que não cabe vira '?' */
bool     eh_ascii(const char *s);
/* Percorre uma pasta recursivamente; para cada entrada chama cb(caminho_real, nome_no_arquivo,
 * tipo, tamanho, ctx); cb devolve < 0 para parar. */
typedef int (*percorrer_cb_t)(const char *real, const char *nome, unsigned tipo, unsigned tamanho, void *ctx);
int      percorrer(const char *real, const char *nome, bool recursivo, percorrer_cb_t cb, void *ctx);
/* Data e hora local formatadas (AAAA-MM-DD HH:MM) de um instante UTC. */
void     comp_data_curta(uint32_t utc, char *s, int max);
uint32_t comp_mtime(const char *caminho);   /* UTC da última modificação, ou a hora atual se o disco não guarda */

#endif
