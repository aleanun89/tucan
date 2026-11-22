// SPDX-License-Identifier: GPL-2.0-or-later
/* imagem.h - imágenes de libsagui (D054): leer BMP, PPM/PGM/PBM, PNG y JPEG,
 * grabar BMP (y PPM) y dibujar una imagen escalada.
 *
 * En la memoria una imagen es siempre igual, sea cual sea el formato del
 * archivo: ancho x alto píxeles 0x00RRGGBB, línea a línea, de arriba a
 * abajo (el mismo formato de los píxeles de las ventanas). Quien lee convierte; quien
 * dibuja sólo copia. La transparencia (PNG) se mezcla con blanco al leer.
 *
 * Todo en enteros (punto fijo 16.16): libsagui se compila con
 * -mgeneral-regs-only y no puede usar float. */
#ifndef SAGUI_IMAGEM_H
#define SAGUI_IMAGEM_H
#include "sagui.h"

typedef struct {
    int       larg, alt;
    uint32_t *px;            /* ancho * alto píxeles 0x00RRGGBB */
    char      formato[16];   /* "BMP 24 bits", "PNG 8 bits RGB"... (lo que se leyó) */
} imagem_t;

#define IMAGEM_LADO_MAX 8192   /* lado mayor aceptado al leer */
#define IMAGEM_PIXELS_MAX (16 * 1024 * 1024)   /* 64 MB de píxeles */

/* Errores propios (los de archivo son los ERRO_* del sistema). */
#define IMG_ERRO_FORMATO     -200   /* no es BMP, PNM ni PNG */
#define IMG_ERRO_CORROMPIDO  -201   /* el archivo termina antes o tiene datos inválidos */
#define IMG_ERRO_NAO_SUPORTA -202   /* variante que no leemos (BMP comprimido, 16 bits...) */
#define IMG_ERRO_GRANDE      -203   /* mayor que IMAGEM_LADO_MAX / IMAGEM_PIXELS_MAX */
#define IMG_ERRO_MEMORIA     -204

bool imagem_criar(imagem_t *img, int larg, int alt, uint32_t cor);   /* false = sin memoria */
void imagem_liberar(imagem_t *img);
bool imagem_copiar(imagem_t *dst, const imagem_t *src);
/* Lee el archivo entero y reconoce el formato por el principio (no por la extensión). */
int  imagem_ler(const char *caminho, imagem_t *img);                  /* 0 = ok */
int  imagem_decodificar(const uint8_t *dados, int n, imagem_t *img);
/* JPEG baseline (mejora 047): lo implementa imagem_jpeg.c. */
int  imagem_ler_jpeg(const uint8_t *dados, int n, imagem_t *img);      /* 0 = ok */
/* GIF (D185): el primer cuadro; el color transparente se mezcla con blanco. */
int  imagem_ler_gif(const uint8_t *dados, int n, imagem_t *img);       /* 0 = ok */
int  imagem_gravar_bmp(const char *caminho, const imagem_t *img);     /* 24 bits; 0 = ok */
int  imagem_gravar_png(const char *caminho, const imagem_t *img);     /* RGB 8 bits, sin comprimir (D125) */
/* PNG en memoria (RGB 8 bits, filtro Sub y Flate): *dados se libera con libera(). 0 = ok (D296). */
int  imagem_codificar_png(const imagem_t *img, uint8_t **dados, int *n);
int  imagem_gravar_ppm(const char *caminho, const imagem_t *img);     /* P6 */
const char *imagem_erro(int erro);                                    /* texto en español */
bool imagem_extensao(const char *nome);   /* .bmp .ppm .pgm .pbm .pnm .png (sin distinguir mayúsculas) */

/* Dibuja img escalada a l x a con la esquina en (ox, oy) del destino
 * (larg_dst píxeles por línea), sólo dentro del recorte (rx, ry, rl, ra).
 * suave = bilineal; si no, vecino más próximo. */
void imagem_desenhar(uint32_t *dst, int larg_dst, int rx, int ry, int rl, int ra,
                     const imagem_t *img, int ox, int oy, int l, int a, bool suave);
/* Una imagen nueva con otro tamaño. */
bool imagem_escalar(imagem_t *dst, const imagem_t *src, int l, int a, bool suave);

/* El inflate del PNG, expuesto para quien necesite zlib/deflate:
 * descomprime un flujo deflate crudo en saida; devuelve los bytes escritos o <0. */
int  inflar(const uint8_t *ent, int n_ent, uint8_t *saida, int max_saida);

/* La IDCT entera de 8x8 (la misma para el JPEG y el MPEG-1): los coeficientes
 * van en orden natural (coef[v*8+u]) y la salida tiene signo (quien llama
 * suma el DC y el nivel 128). */
void imagem_idct(const int32_t *coef, int32_t *saida, int passo);
/* Convierte YCbCr en RGB, en punto fijo. */
uint32_t imagem_ycbcr(int32_t y, int32_t cb, int32_t cr);
/* El orden en zigzag (el mismo en el JPEG y en el MPEG-1). */
extern const uint8_t imagem_zigzag[64];

#endif
