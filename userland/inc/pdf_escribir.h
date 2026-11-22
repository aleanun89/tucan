// SPDX-License-Identifier: GPL-2.0-or-later
/* pdf_escribir.h - escritor de PDF mínimo de la libsagui (mejora 116, D291).
 *
 * Crea PDF 1.4 con texto, rectángulos y líneas usando las 14 fuentes estándar
 * (Helvetica, Times y Courier con sus variantes, codificación WinAnsi: los acentos
 * de Latin-1 salen bien) y el contenido comprimido con Flate (deflar + cabecera
 * zlib). Incrusta las fuentes del sistema que use el texto (D319); las 14 estándar no.
 *
 * Todas las medidas van en TWIPS (1/20 de punto, como RTF), con el origen arriba a
 * la izquierda de la hoja y la "y" creciendo hacia abajo: el escritor lo invierte.
 * No usa coma flotante (la libsagui se compila sin ella, D076).
 *
 *   pdfw_t *w = pdfw_criar(PDFW_A4);
 *   pdfw_pagina(w);
 *   pdfw_texto(w, 1440, 1440 + 240, PDFW_SANS, PDFW_NEGRITA, 240, 0x000000, "Hola", 4, 0);
 *   uint8_t *dados; int n;
 *   pdfw_finalizar(w, &dados, &n);   // y a escribir; libera(dados) y pdfw_liberar(w)
 */
#ifndef SAGUI_PDF_ESCRIBIR_H
#define SAGUI_PDF_ESCRIBIR_H
#include "sagui.h"
#include "fontes_pdf.h"

#define PDFW_A4     0
#define PDFW_CARTA  1
/* Familias y estilos (el estilo es una máscara). */
#define PDFW_SANS   0       /* Helvetica */
#define PDFW_SERIF  1       /* Times */
#define PDFW_MONO   2       /* Courier */
#define PDFW_NEGRITA  1
#define PDFW_CURSIVA  2

typedef struct pdfw pdfw_t;

/* ---- fuentes del sistema (D319) ----
 * Las familias 0..2 son las clásicas (Helvetica, Times y Courier estándar). Las de numero >= 3
 * son fuentes TrueType/OpenType instaladas (/fontes, /apps/fuentes, /home/fuentes): las pone un
 * proveedor (userland/libfontes/familias.c, que usa la FPU y por eso no está en la libsagui; solo
 * lo instalan los programas que enlazan FONTES_OBJ, como Pluma). Sin proveedor, o si la fuente ya
 * no se puede abrir, todo se comporta como antes: una familia >= 3 mide, dibuja y exporta como
 * Helvetica. El PDF incrusta las fuentes del sistema usadas (subconjunto TrueType con FontFile2;
 * OpenType CFF entera con FontFile3). */
#define PDFW_FAM_SIS 3
#define PDFW_FAM_MAX 249
typedef struct {
    int  (*ancho)(int fam, int est, unsigned char c);        /* milésimas de em; -1 = esa fuente no está */
    bool (*info)(int fam, int est, pdfw_finfo_t *i);          /* para incrustar; false = no se puede */
    uint8_t *(*archivo)(int fam, int est, const uint8_t *usadas, int *n, int *original);  /* aloca; false... NULL = no */
    /* Un glifo de un carácter Latin-1 como mapa de cobertura 0..255 (l x a, con el desplazamiento ox, oy
     * respecto a la línea base); NULL = dibujar con la fuente 8x16. Vale para las clásicas y las del sistema. */
    const uint8_t *(*glifo)(int fam, int est, unsigned char c, int px, int *l, int *a, int *ox, int *oy);
    int  (*cantidad)(void);                                  /* mayor número de familia + 1 (>= 3) */
    const char *(*nombre)(int fam);                          /* nombre de familia; NULL si no existe */
    int  (*resolver)(const char *nombre);                    /* familia del sistema por nombre (sin distinguir mayúsculas), -1 = no hay */
    int  (*clase)(int fam);                                  /* 0 sans, 1 serif, 2 mono: la clásica más parecida */
    int  (*actualizar)(void);                                /* vuelve a mirar las carpetas; devuelve cuántas familias nuevas */
    int  (*estilos)(int fam);                                /* D327: estilos que trae la familia, máscara con el bit (1 << est),
                                                              * est = RTE_NEGRITA | RTE_CURSIVA (0 normal ... 3 negrita cursiva); NULL = los cuatro */
} fuentes_prov_t;
void fuentes_prov_instalar(const fuentes_prov_t *p);
const fuentes_prov_t *fuentes_prov_obter(void);
/* Nombre de la familia clásica o del sistema ("Arial", "Times New Roman", "Courier New" o la del sistema). */
const char *pdfw_nombre_familia(int fam);

/* Tamaño de la hoja en twips (A4 = 11906 x 16838, Carta = 12240 x 15840). */
void    pdfw_papel_tamanho(int papel, int *largura, int *altura);
/* Ancho de un carácter Latin-1 en milésimas de em (0 si no se dibuja). */
int     pdfw_largura_char(int familia, int estilo, unsigned char c);
/* Ancho de un texto en twips para un cuerpo de "tam" twips. */
int     pdfw_largura_texto(int familia, int estilo, int tam, const char *t, int n);

pdfw_t *pdfw_criar(int papel);
void    pdfw_tamanho(pdfw_t *w, int largura, int altura);       /* hoja de tamaño propio (twips) */
void    pdfw_liberar(pdfw_t *w);
void    pdfw_titulo(pdfw_t *w, const char *titulo);          /* /Info /Title (Latin-1) */
bool    pdfw_pagina(pdfw_t *w);                              /* abre una página nueva */
/* Texto con la línea base en (x, y). "extra" son twips que se suman a cada espacio
 * (para justificar). */
void    pdfw_texto(pdfw_t *w, int x, int y, int familia, int estilo, int tam, uint32_t cor,
                   const char *t, int n, int extra);
void    pdfw_retangulo(pdfw_t *w, int x, int y, int l, int a, uint32_t cor);   /* relleno */
void    pdfw_linha(pdfw_t *w, int x0, int y0, int x1, int y1, int espessura, uint32_t cor);
/* Imágenes (D296): la esquina de arriba a la izquierda en (x, y) y el tamaño l x a, en twips.
 * pdfw_imagem_rgb: píxeles 0x00RRGGBB (pw x ph), guardados como RGB de 8 bits con Flate.
 * pdfw_imagem_jpeg: un JPEG baseline tal cual (DCTDecode), con sus píxeles y componentes (1 o 3).
 * false = sin memoria, sin página abierta o demasiadas imágenes. */
bool    pdfw_imagem_rgb(pdfw_t *w, int x, int y, int l, int a, const uint32_t *px, int pw, int ph);
bool    pdfw_imagem_jpeg(pdfw_t *w, int x, int y, int l, int a, const uint8_t *jpg, int n, int pw, int ph, int ncomp);
/* Cierra el documento; *dados se reserva con aloca (libera(*dados)). false = sin memoria
 * o sin páginas. */
bool    pdfw_finalizar(pdfw_t *w, uint8_t **dados, int *n);
int     pdfw_paginas(const pdfw_t *w);

#endif
