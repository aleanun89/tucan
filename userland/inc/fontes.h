// SPDX-License-Identifier: GPL-2.0-or-later
/* fontes.h - las fuentes vectoriales de SaguiOS (D196, libfontes).
 *
 * TrueType, OpenType con contornos CFF, WOFF y WOFF2, con antialias por cobertura
 * exacta y caché de glifos. El motor es el del navegador Cipó (D141), sacado a
 * userland/libfontes/; lo enlazan los programas con FONTES_OBJ (usa la FPU x87,
 * así que no está en la libsagui comun). La fuente del escritorio (mapa de bits
 * 8x16 del kernel) no cambia: esto es para programas que dibujan sus textos.
 *
 * Los textos son UTF-8 y se toleran los bytes sueltos como Latin-1 (los literales
 * de los programas salen en ISO-8859-1): un byte que no forma una secuencia UTF-8
 * válida vale por sí mismo. */
#ifndef FONTES_H
#define FONTES_H

#include "sagui.h"

typedef struct fonte fonte_t;

/* ---- cargar ---- */
fonte_t *fonte_abrir(const uint8_t *d, unsigned n);       /* desde memoria (copia los datos) */
fonte_t *fonte_abrir_archivo(const char *ruta);           /* NULL si no existe o no es una fuente */
void     fonte_fechar(fonte_t *f);

/* ---- metricas (px = tamaño de la fuente en píxeles) ---- */
int  fonte_ascendente(fonte_t *f, int px);                /* sobre la línea base */
int  fonte_descendente(fonte_t *f, int px);               /* bajo la línea base, positivo */
int  fonte_altura_normal(fonte_t *f, int px);             /* altura de línea (ascendente + descendente + hueco) */
int  fonte_avanco(fonte_t *f, unsigned cp, int px);       /* avance de un carácter en 1/64 de píxel */
bool fonte_tem(fonte_t *f, unsigned cp);
/* Nombre de familia y de estilo (de la tabla name) en Latin-1; devuelve el largo
 * de la familia, 0 si la fuente no lo trae. */
int  fonte_nombre(fonte_t *f, char *familia, int nf, char *estilo, int ne);

/* ---- medir y dibujar ---- */
/* Ancho en píxeles de un texto UTF-8 (o hasta el primer '\n'). */
int  fonte_medir(fonte_t *f, const char *texto, int px);
int  fonte_medir_n(fonte_t *f, const char *texto, int n, int px);
/* Dibuja el texto en un búfer 0xRRGGBB de larg x alt (lo que hay en jan_t->px),
 * mezclando con la cobertura de cada píxel. (x, y) es la esquina de arriba a la
 * izquierda de la línea (y = arriba del ascendente). Devuelve el ancho dibujado,
 * igual que fonte_medir. */
int  fonte_texto_buf(fonte_t *f, uint32_t *dst, int larg, int alt, int x, int y,
                     const char *texto, int px, uint32_t cor);
int  fonte_texto_buf_n(fonte_t *f, uint32_t *dst, int larg, int alt, int x, int y,
                       const char *texto, int n, int px, uint32_t cor);
/* Lo mismo sobre una ventana (jan_t de gui.h); falta el jan_mostrar. */
#define fonte_texto(f, jn_, x_, y_, texto_, tam_, cor_) \
    fonte_texto_buf((f), (jn_)->px, (jn_)->larg, (jn_)->alt, (x_), (y_), (texto_), (tam_), (cor_))

/* ---- fuentes variables, itálico sintético (las usa el navegador) ---- */
const uint8_t *fonte_glifo(fonte_t *f, unsigned cp, int px, int *l, int *a, int *ox, int *oy);
fonte_t *fonte_instancia(fonte_t *b, const uint32_t *tags, const int *valores100, int n);
bool     fonte_variavel(fonte_t *f);
fonte_t *fonte_obliqua(fonte_t *b);

/* ---- el catálogo del sistema ---- */
#define FUENTES_DIR_APPS  "/apps/fuentes"     /* las que trae el sistema (partición SAGUI-APPS) */
#define FUENTES_DIR_HOME  "/home/fuentes"     /* las que añade el usuario */
#define FUENTES_MAX       1024
typedef struct { char ruta[88]; char familia[48]; char estilo[24]; } fuente_info_t;
/* Busca *.otf y *.ttf en las dos carpetas y lee la familia de cada una (solo la
 * tabla name, sin cargar la fuente). Ordenado por familia. Devuelve cuántas. */
int fuentes_catalogo(fuente_info_t *v, int max);

/* ---- las familias del sistema para los programas de documentos (D319) ----
 * Pluma y los demás (Macaco, PlanoMini, GebraMini podrán usarla) agrupan las fuentes de /fontes
 * (DejaVu, en el ramdisk), /apps/fuentes y /home/fuentes por FAMILIA, con una ruta por estilo:
 * ruta[0] regular, ruta[1] negrita, ruta[2] cursiva, ruta[3] negrita cursiva (cadena vacía = no
 * hay). Los nombres salen de la tabla name del fichero (el nombre del fichero si no la trae); lo
 * que no es negrita ni cursiva en el estilo (Light, Condensed...) pasa a formar otra familia.
 * Si una familia está en dos carpetas gana la primera de la lista. Ordenadas por nombre. */
#define FUENTES_DIR_SISTEMA "/fontes"
typedef struct { char familia[48]; char ruta[4][88]; } familia_fuente_t;
int fuentes_sistema_listar(familia_fuente_t *v, int max);
/* Instala el proveedor de fuentes de documento (familias.c) en la libsagui: las familias >= 3 de
 * rte/pdfw/rtf pasan a ser estas fuentes (el 0..2 clásico se dibuja con DejaVu). Idempotente. */
void fuentes_doc_instalar(void);
/* Letras vectoriales (DejaVu) para los WordArt de wordart.h (D296; Tucán, D369). */
void wordart_fuentes_instalar(void);
/* Para pruebas: vuelve a leer las carpetas y devuelve cuántas familias quedan registradas. */
int fuentes_doc_actualizar(void);

/* ---- instalar y quitar fuentes (D356, libfontes/instalar.c) ---- */
enum { FUENTES_OK = 0, FUENTES_E_NOEXISTE = -1, FUENTES_E_EXT = -2, FUENTES_E_TAM = -3, FUENTES_E_DANADA = -4,
       FUENTES_E_VACIA = -5, FUENTES_E_COPIA = -6, FUENTES_E_PERMISO = -7 };
const char *fuentes_error_texto(int r);
/* La carpeta de fuentes de la cuenta: /home/fuentes, o $HOME/fuentes si HOME es /home/<cuenta>. */
void fuentes_dir_usuario(char *dst, int n);
/* Comprueba que el archivo es una fuente que la libfontes abre y dibuja; da su familia y estilo. */
int  fuentes_validar(const char *ruta, char *familia, int nf, char *estilo, int ne);
/* Valida y copia a dir (la crea si hace falta). Sin permiso en dir: FUENTES_E_PERMISO. */
int  fuentes_instalar(const char *origen, const char *dir, char *familia, int nf);
int  fuentes_instalar_carpeta(const char *carpeta, const char *dir, int *malas);   /* cuántas se instalaron */
/* Borra de dir los archivos de la familia (o «Familia Estilo»); devuelve cuántos. */
int  fuentes_quitar(const char *dir, const char *familia, int *errores);
/* El catálogo de una sola carpeta, sin ordenar. */
int  fuentes_catalogo_dir(const char *dir, fuente_info_t *v, int max);
/* Marca de cambios: instalar y quitar suben un contador en /tmp/fuentes.cambio; los programas
 * con la lista abierta lo comparan (fuentes_cambio) y vuelven a leer las carpetas. */
unsigned fuentes_cambio(void);
void     fuentes_cambio_marcar(void);

/* Fuente por defecto elegida en Configuración: uso 0 = documentos, 1 = títulos.
 * Copia la ruta; devuelve false si no hay ninguna elegida. */
bool fuentes_defecto(int uso, char *ruta, int n);
#define FUENTE_DOCUMENTOS 0
#define FUENTE_TITULOS    1
/* Abre la fuente por defecto de un uso (NULL si no hay o no se pudo cargar). */
fonte_t *fuentes_abrir_defecto(int uso);

/* Para el catálogo: la tabla name de un archivo ya abierto (fonte_leer_tabla_name)
 * y su nombre de familia. */
int fonte_leer_tabla_name(int fd, uint8_t *buf, int max);
int fonte_nombre_de_tabla(const uint8_t *tabla, int n, char *familia, int nf, char *estilo, int ne);

#endif
