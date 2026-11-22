// SPDX-License-Identifier: GPL-2.0-or-later
/* presentacion.h - el modelo de presentaciones de Tucán (D369): diapositivas, objetos, temas, dibujo
 * a un mapa de píxeles, formato propio (.tcn), exportación a PDF y a PNG, e importación/exportación
 * de PPTX (OOXML). Sin interfaz: lo usan Tucán (la aplicación) y sus modos de línea de órdenes.
 *
 * Medidas: puntos tipográficos (1/72 de pulgada), el mismo sistema que PPTX (12700 EMU por punto).
 * Una diapositiva 16:9 mide 960 x 540 y una 4:3, 720 x 540. Todo es entero (la libsagui no usa
 * coma flotante, D076). Los textos van en Latin-1, como el resto del sistema.
 *
 * Qué guarda cada objeto: el texto de un cuadro, una forma o una celda tiene un solo formato por
 * objeto (fuente, cuerpo, negrita/cursiva/subrayado, color, alineación, lista). No hay formato por
 * letra: es una decisión de diseño para mantener la aplicación ligera en equipos antiguos. */
#ifndef SAGUI_PRESENTACION_H
#define SAGUI_PRESENTACION_H
#include "sagui.h"
#include "imagem.h"

#define PR_DIAP_MAX   300
#define PR_OBJ_MAX    80
#define PR_MEDIOS_MAX 120
#define PR_TEXTO_MAX  8000
#define PR_FUENTE_MAX 40
#define PR_COL_TEMA   0x01000000u   /* «el color del tema»: no es un color fijo */

/* tipos de objeto */
enum { PR_TEXTO = 1, PR_FORMA, PR_IMAGEN, PR_WORDART, PR_TABLA, PR_GRAFICO };
/* formas */
enum { PF_RECT = 0, PF_REDONDEADO, PF_ELIPSE, PF_TRIANGULO, PF_ROMBO, PF_FLECHA, PF_LINEA, PF_ESTRELLA, PF_N };
/* papeles de un marcador de posición (rol) */
enum { PR_ROL_LIBRE = 0, PR_ROL_TITULO, PR_ROL_CUERPO, PR_ROL_SUBTITULO };
/* animaciones de entrada */
enum { PR_ANIM_NADA = 0, PR_ANIM_APARECER, PR_ANIM_FUNDIDO, PR_ANIM_N };
/* transiciones */
enum { PR_TRANS_CORTE = 0, PR_TRANS_FUNDIDO, PR_TRANS_DESPLAZAR, PR_TRANS_BARRIDO, PR_TRANS_N };
/* diseños */
enum { PR_LY_TITULO = 0, PR_LY_TITULO_CONTENIDO, PR_LY_DOS_COLUMNAS, PR_LY_SOLO_TITULO, PR_LY_BLANCO, PR_LY_N };
/* listas */
enum { PR_LISTA_NADA = 0, PR_LISTA_VINETA, PR_LISTA_NUMERO };
/* gráficos */
enum { PR_GRAF_BARRAS = 0, PR_GRAF_LINEAS, PR_GRAF_SECTORES, PR_GRAF_N };
/* fondos */
enum { PR_FONDO_TEMA = 0, PR_FONDO_COLOR, PR_FONDO_DEGRADADO, PR_FONDO_IMAGEN };
/* bits de estilo de texto (los mismos de RTE_*) */
#define PR_NEGRITA 1
#define PR_CURSIVA 2
#define PR_SUBRAYADO 4

typedef struct {
    uint8_t  tipo, forma, rol, anim;
    uint8_t  alin;               /* 0 izquierda, 1 centro, 2 derecha */
    uint8_t  valign;             /* 0 arriba, 1 centro, 2 abajo */
    uint8_t  lista;              /* PR_LISTA_* */
    uint8_t  est;                /* PR_NEGRITA | PR_CURSIVA | PR_SUBRAYADO */
    uint8_t  tam;                /* cuerpo en puntos */
    uint8_t  borde;              /* grosor del contorno en puntos (0 = sin) */
    uint8_t  rellena;            /* 1 = con relleno */
    uint8_t  wa_estilo;          /* WordArt: estilo de la galería */
    uint8_t  graf;               /* gráfico: PR_GRAF_* */
    uint8_t  filas, cols;        /* tabla */
    int16_t  x, y, w, h;         /* en puntos, respecto a la diapositiva */
    int16_t  grupo;              /* 0 = suelto; los del mismo número se mueven juntos */
    int      medio;              /* imagen: índice en d->medios (-1 = ninguno) */
    uint32_t relleno, borde_col, col;
    char     fuente[PR_FUENTE_MAX];   /* vacía = la del tema (títulos o cuerpo según el rol) */
    char    *texto;              /* texto Latin-1 (reservado con aloca); tabla: celdas por filas separadas por '\x1f';
                                  * gráfico: «etiqueta=valor» separados por comas */
} pr_obj_t;

typedef struct {
    pr_obj_t *obj;
    int       n, cap;
    char     *notas;             /* notas del orador */
    uint8_t   layout, trans, oculta, fondo_tipo;
    uint8_t   dur_trans;         /* décimas de segundo; 0 = la de siempre */
    uint32_t  fondo1, fondo2;
    int       fondo_medio;
    int       auto_seg;          /* avanzar solo tras N segundos (0 = con un clic) */
    unsigned  version;           /* sube con cada cambio: para las cachés de miniaturas */
} pr_diap_t;

typedef struct {
    uint8_t *dados;              /* el archivo original (PNG o JPEG) */
    int      n;
    char     ext[5];             /* "png" o "jpg" */
    imagem_t img;                /* píxeles ya descodificados, reducidos si eran enormes */
    bool     dec;                /* intentado descodificar */
} pr_medio_t;

typedef struct {
    pr_diap_t **d;
    int        n, cap;
    int        ancho, alto;      /* 960x540 o 720x540 */
    int        tema;
    char       f_titulo[PR_FUENTE_MAX], f_cuerpo[PR_FUENTE_MAX];
    char       pie[80];
    bool       num, con_pie, auto_avance;
    pr_medio_t *medios;
    int        n_medios;
} pr_deck_t;

/* ---- temas ---- */
typedef struct {
    const char *nombre;
    uint32_t    fondo1, fondo2;  /* el degradado vertical va de fondo1 (arriba) a fondo2 (abajo) */
    uint32_t    titulo, texto, acento, acento2;
    int         deco;            /* 0 nada, 1 franja inferior, 2 franja izquierda, 3 franja superior con pico */
    const char *f_titulo, *f_cuerpo;
} pr_tema_t;
extern const pr_tema_t pr_temas[];
int pr_n_temas(void);
const char *pr_nombre_forma(int f);
const char *pr_nombre_layout(int l);
const char *pr_nombre_trans(int t);

/* ---- modelo ---- */
pr_deck_t *pr_deck_vacio(bool ancha);            /* sin diapositivas (para cargar) */
pr_diap_t *pr_diap_vacia(pr_deck_t *d);          /* añade al final una sin objetos */
pr_deck_t *pr_nueva(bool ancha);                 /* con una diapositiva de título */
void       pr_liberar(pr_deck_t *d);
pr_diap_t *pr_diap_nueva(pr_deck_t *d, int pos, int layout);   /* inserta con ese diseño; NULL = sin memoria o lleno */
pr_diap_t *pr_diap_duplicar(pr_deck_t *d, int i);              /* copia i justo detrás */
void       pr_diap_borrar(pr_deck_t *d, int i);                /* siempre queda una */
void       pr_diap_mover(pr_deck_t *d, int de, int a);
pr_obj_t  *pr_obj_nuevo(pr_diap_t *s, int tipo);               /* zerado con los valores por defecto; NULL = lleno */
bool       pr_obj_copiar(pr_obj_t *dst, const pr_obj_t *src);  /* copia profunda del texto */
void       pr_obj_borrar(pr_diap_t *s, int i);
void       pr_obj_liberar_campos(pr_obj_t *o);
bool       pr_obj_poner_texto(pr_obj_t *o, const char *t);
void       pr_obj_orden(pr_diap_t *s, int i, int a);           /* mueve el objeto i a la posición a (Z) */
void       pr_aplicar_layout(pr_deck_t *d, pr_diap_t *s, int layout);   /* rehace los marcadores de posición */
void       pr_aplicar_tema(pr_deck_t *d, int tema);            /* también las fuentes y colores de los marcadores */
void       pr_tamano(pr_deck_t *d, bool ancha);                /* 16:9 o 4:3, reescalando los objetos */
uint32_t   pr_color_texto(const pr_deck_t *d, const pr_obj_t *o);   /* resuelve PR_COL_TEMA */
uint32_t   pr_color_relleno(const pr_deck_t *d, const pr_obj_t *o);
uint32_t   pr_color_borde(const pr_deck_t *d, const pr_obj_t *o);
const char *pr_fuente_obj(const pr_deck_t *d, const pr_obj_t *o);   /* nombre efectivo de la familia */
extern const char *pr_hints[3];                                      /* ayuda de los marcadores vacíos: título, cuerpo, subtítulo */
int        pr_medio_anadir(pr_deck_t *d, const uint8_t *dados, int n);   /* -1 = no es una imagen que sepamos leer */
const imagem_t *pr_medio_imagen(pr_deck_t *d, int medio);      /* NULL = no se pudo */
void       pr_tocar(pr_diap_t *s);                              /* version++ */
int        pr_familia(const char *nombre);                      /* número de familia de rte/pdfw (0 = Arial si no existe) */
const char *pr_nombre_fam(int fam);

/* ---- texto: composición en líneas ---- */
typedef struct {
    int ini, n;                  /* trozo del texto del objeto (sin los tabuladores iniciales ni el '\n') */
    int pini;                    /* dónde empieza su párrafo (con los tabuladores); en las demás líneas, igual que ini */
    int x, y;                    /* esquina de la línea, en centésimas de punto, respecto al borde interior del cuadro */
    int ancho;                   /* ancho del texto de la línea, en centésimas de punto */
    int base;                    /* línea base, en centésimas de punto desde y */
    int nivel;                   /* 0..4 (tabuladores iniciales) */
    int marca;                   /* 0 sin; 1 viñeta; n>=2: número n-1 */
    int marca_x;                 /* x de la viñeta o el número */
    bool primera;                /* primera línea de su párrafo */
} pr_lin_t;
typedef struct { pr_lin_t *l; int n; int alto; int ancho_int; int y0; } pr_comp_t;   /* alto del bloque; y0: desplazamiento vertical (alineación) */
#define PR_INSET_X 7
#define PR_INSET_Y 4
/* Compone el texto del objeto (en el cuadro w x h del propio objeto). false = sin memoria. */
bool pr_componer(const pr_deck_t *d, const pr_obj_t *o, const char *texto, pr_comp_t *c);
void pr_comp_liberar(pr_comp_t *c);
/* Posición del cursor "pos" (índice en el texto) respecto al origen del objeto, en centésimas de punto. */
void pr_cursor_xy(const pr_deck_t *d, const pr_obj_t *o, const pr_comp_t *c, int pos, int *x, int *y);
/* Índice del texto más cercano al punto (x, y) (centésimas de punto respecto al origen del objeto). */
int  pr_pos_en(const pr_deck_t *d, const pr_obj_t *o, const pr_comp_t *c, int x, int y);
int  pr_ancho_texto(int fam, int est, int tam, const char *t, int n);   /* centésimas de punto */

/* ---- dibujo ---- */
#define PRD_SIN_TEXTO   1        /* sin los textos (para el PDF, que los pone como texto real) */
#define PRD_EDITOR      2        /* los marcadores vacíos muestran su ayuda */
#define PRD_SIN_FONDO   4
#define PRD_SOLO_TEXTO_MINI 8    /* miniatura: el texto como rayas */
/* Dibuja la diapositiva i en pw x ph píxeles (stride = pw). paso: -1 = todo visible; >= 0 oculta los
 * objetos animados cuyo orden sea mayor que paso. omitir: objeto que no se dibuja (-1 = ninguno). */
void pr_dibujar(pr_deck_t *d, int i, uint32_t *px, int pw, int ph, unsigned flags, int paso, int omitir);
void pr_dibujar_obj(pr_deck_t *d, pr_obj_t *o, uint32_t *px, int pw, int ph, unsigned flags);
/* Recorre todo el texto que se dibuja en la diapositiva i (cuadros, formas, celdas de tablas, pie y número): para
 * cada uno llama a cb con el objeto (puede ser uno temporal), su texto, su origen en puntos y su color. */
typedef void (*pr_texto_cb)(void *ctx, const pr_obj_t *o, const char *texto, int ox, int oy, uint32_t col);
void pr_recorrer_textos(pr_deck_t *d, int i, pr_texto_cb cb, void *ctx);
int  pr_pasos(const pr_diap_t *s);   /* cuántos clics de animación tiene */

/* ---- ficheros ---- */
char    *pr_a_texto(const pr_deck_t *d, int *n);              /* formato interno de texto (también para deshacer) */
pr_deck_t *pr_de_texto(const char *t, int n, pr_deck_t *base);   /* base != NULL: reutiliza sus medios */
int      pr_guardar(const pr_deck_t *d, const char *ruta);     /* .tcn (ZIP); 0 = ok */
pr_deck_t *pr_abrir(const char *ruta, int *err);               /* .tcn, .pptx; err < 0 si falla */
int      pr_exportar_pdf(pr_deck_t *d, const char *ruta, bool con_notas);
int      pr_exportar_png(pr_deck_t *d, int i, const char *ruta, int ancho);
int      pr_exportar_pptx(const pr_deck_t *d, const char *ruta);
pr_deck_t *pr_importar_pptx(const uint8_t *z, int n, int *err, char *avisos, int max_avisos);
extern char pr_avisos[240];       /* lo que no se pudo importar (vacío = todo) */
#define PR_ERR_FORMATO  -1
#define PR_ERR_MEMORIA  -2
#define PR_ERR_ARCHIVO  -3
const char *pr_err_texto(int e);

/* ---- utilidades ---- */
int  pr_latin1_a_utf8(const char *s, char *d, int max);
int  pr_utf8_a_latin1(const char *s, int n, char *d, int max);

#endif
