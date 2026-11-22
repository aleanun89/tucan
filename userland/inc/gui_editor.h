// SPDX-License-Identifier: GPL-3.0-only
/* gui_editor.h - componentes para editores de la libsagui (D454, v8.5.6).
 *
 * Los necesita el editor del motor de juegos Capibara, pero son generales: paneles divisibles, inspector de
 * propiedades, selector de color, lienzo con zoom, paleta de casillas, editor de código con resaltado,
 * deshacer/rehacer, arrastrar y soltar y consola de salida. Siguen el patrón de siempre (una estructura con
 * la posición y el estado, *_desenhar y *_tratar) y beben de mod_tema (D423) y del esquema (D428), así que
 * cambian con el estilo del sistema. Sin coma flotante: enteros y punto fijo. Documentación: docs/componentes-editor.md. */
#ifndef SAGUI_GUI_EDITOR_H
#define SAGUI_GUI_EDITOR_H
#include "gui.h"

/* ======================================================================== paneles divisibles */
#define PAINEIS_MAX 24
#define PAINEIS_SEP 7                       /* grosor de los separadores */
enum { PD_FOLHA, PD_LADO, PD_PILHA };       /* PD_LADO: a | b (separador vertical); PD_PILHA: a sobre b */
enum { PAINEIS_NADA, PAINEIS_ARRASTA, PAINEIS_SOLTOU, PAINEIS_PLEGOU };
typedef struct {
    int tipo, a, b;                         /* hijos de una división (índices en no[]) */
    int pos;                                /* milésimas del espacio disponible que ocupa a */
    int min_a, min_b;                       /* tamaños mínimos de cada lado, en píxeles */
    int plegavel;                           /* 0 = no se pliega, 1 = se pliega el lado a, 2 = el lado b */
    bool plegado;
    int id;                                 /* hoja: identificador de la aplicación */
    int x, y, l, h;                         /* área calculada */
    int sx, sy, sl, sh;                     /* separador calculado */
} pd_no_t;
typedef struct {
    int x, y, l, a;
    int n, raiz;
    pd_no_t no[PAINEIS_MAX];
    int arrasto;                            /* división cuyo separador se arrastra (-1 = ninguna) */
    int hover;                              /* división cuyo separador está bajo el ratón (-1 = ninguna) */
} paineis_t;
void paineis_iniciar(paineis_t *p, int x, int y, int l, int a);
int  paineis_folha(paineis_t *p, int id);                                   /* -> índice del nodo */
/* Divide: tipo PD_LADO o PD_PILHA, hijos a y b (nodos ya creados), pos en milésimas, mínimos en píxeles. El último
 * nodo que se crea pasa a ser la raíz (o paineis_raiz). Devuelve el índice de la división. */
int  paineis_dividir(paineis_t *p, int tipo, int a, int b, int pos, int min_a, int min_b, int plegavel);
void paineis_raiz(paineis_t *p, int no);
void paineis_mover(paineis_t *p, int x, int y, int l, int a);               /* al redimensionar la ventana; recalcula */
void paineis_calcular(paineis_t *p);
bool paineis_rect(const paineis_t *p, int id, int *x, int *y, int *l, int *a);   /* área de una hoja; false = plegada (tamaño 0) */
void paineis_desenhar(jan_t *j, const paineis_t *p);                        /* solo los separadores: el contenido lo pinta la aplicación */
int  paineis_tratar(jan_t *j, paineis_t *p, const evento_t *e);             /* PAINEIS_*: ARRASTA y SOLTOU piden repintar los contenidos */
void paineis_plegar(paineis_t *p, int no, bool plegar);
int  paineis_guardar(const paineis_t *p, char *buf, int max);               /* «pos,plegado;...» de cada división, en orden */
bool paineis_restaurar(paineis_t *p, const char *txt);

/* ======================================================================== selector de color */
#define SELCOR_L 260
#define SELCOR_A 208
enum { SELCOR_NADA, SELCOR_MUDOU };
typedef struct {
    int x, y;                               /* esquina; mide SELCOR_L x SELCOR_A */
    bool com_alfa;
    int r, g, b, alfa;                      /* 0..255 */
    int h, s, v;                            /* h 0..359, s y v 0..255 */
    int arrasto;                            /* qué se arrastra (0 = nada) */
} seletorcor_t;
void     seletorcor_iniciar(seletorcor_t *c, int x, int y, uint32_t argb, bool com_alfa);
void     seletorcor_definir(seletorcor_t *c, uint32_t argb);
uint32_t seletorcor_cor(const seletorcor_t *c);                             /* 0xAARRGGBB */
void     seletorcor_desenhar(jan_t *j, seletorcor_t *c);
int      seletorcor_tratar(jan_t *j, seletorcor_t *c, const evento_t *e);
bool     seletor_cor_dialogo(const char *titulo, uint32_t *argb, bool com_alfa);   /* ventana modal con Aceptar/Cancelar */
void     cor_rgb_para_hsv(int r, int g, int b, int *h, int *s, int *v);
void     cor_hsv_para_rgb(int h, int s, int v, int *r, int *g, int *b);

/* ======================================================================== inspector de propiedades */
#define INS_FILA 22
enum { INS_SECAO, INS_INT, INS_FIXO, INS_BOOL, INS_ENUM, INS_DESLIZ, INS_TEXTO, INS_COR, INS_ARQUIVO };
typedef struct {
    int  tipo;
    char nome[32];
    int  valor;               /* INT; FIXO (escalado por 10^decimais); BOOL 0/1; ENUM índice; DESLIZ; COR 0xAARRGGBB */
    int  min, max, passo;     /* límites y paso (en las mismas unidades que valor) */
    int  decimais;            /* FIXO: cifras decimales (valor 1250 con decimais 2 = 12,50) */
    char texto[96];           /* TEXTO y ARQUIVO */
    const char *const *opcoes; int nopcoes;   /* ENUM */
    const char *filtro;       /* ARQUIVO: filtros de dialogo_abrir */
    bool aberta;              /* SECAO: desplegada */
    bool desab;
    bool com_alfa;            /* COR */
} ins_prop_t;
enum { INSPETOR_NADA, INSPETOR_MUDOU, INSPETOR_SECAO };
typedef struct inspetor {
    int x, y, l, a;
    ins_prop_t *p; int n;
    int topo;                 /* primera fila visible (filas, no propiedades) */
    int sel;                  /* propiedad elegida */
    int col_nome;             /* ancho de la columna de nombres */
    int ultimo;               /* última propiedad cambiada */
    int editando;             /* propiedad con el texto en edición (-1 = ninguna) */
    char buf[96]; int cursor;
    int arrasto, arr_x0, arr_v0;   /* propiedad que se arrastra (-1) */
    bool foco, sel_tudo, moveu;    /* teclado: tiene el foco; el texto en edición está todo seleccionado; el arrastre ya se movió */
    int menu_prop;            /* propiedad cuyo desplegable está abierto (-1) */
    int codigo;               /* código de menu_contextual para los desplegables (único en la ventana) */
    void (*mudou)(void *ctx, struct inspetor *ins, int indice);   /* aviso de cambio (valor ya actualizado) */
    void *ctx;
} inspetor_t;
void inspetor_iniciar(inspetor_t *s, int x, int y, int l, int a, ins_prop_t *p, int n);
int  inspetor_secao(ins_prop_t *p, const char *nome, bool aberta);          /* rellena una propiedad de sección; devuelve 0 */
void inspetor_int(ins_prop_t *p, const char *nome, int valor, int min, int max, int passo);
void inspetor_fixo(ins_prop_t *p, const char *nome, int valor, int decimais, int min, int max, int passo);
void inspetor_bool(ins_prop_t *p, const char *nome, bool valor);
void inspetor_enum(ins_prop_t *p, const char *nome, int sel, const char *const *opcoes, int n);
void inspetor_deslizador(ins_prop_t *p, const char *nome, int valor, int min, int max);
void inspetor_texto(ins_prop_t *p, const char *nome, const char *texto);
void inspetor_cor(ins_prop_t *p, const char *nome, uint32_t argb, bool com_alfa);
void inspetor_arquivo(ins_prop_t *p, const char *nome, const char *caminho, const char *filtro);
void inspetor_desenhar(jan_t *j, inspetor_t *s);
int  inspetor_tratar(jan_t *j, inspetor_t *s, const evento_t *e);
int  inspetor_linhas(const inspetor_t *s);                                  /* filas visibles (secciones plegadas ocultan las suyas) */
void inspetor_valor_texto(const ins_prop_t *p, char *dst, int max);         /* «12,50», «Sí»... */
void inspetor_definir_int(inspetor_t *s, int i, int valor);                 /* cambia un valor y avisa */

/* ======================================================================== lienzo con zoom y desplazamiento */
/* Coordenadas del mundo enteras; zoom en milésimas (1000 = 100 %). Pantalla = origen + mundo * zoom / 1000. */
#define EDVISTA_ZOOM_MIN 62
#define EDVISTA_ZOOM_MAX 16000
enum { EDVISTA_E_CLIQUE, EDVISTA_E_RET, EDVISTA_E_MOVER, EDVISTA_E_ESCALAR, EDVISTA_E_GIRAR, EDVISTA_E_ZOOM, EDVISTA_E_DESLOCAR };
enum { EDVISTA_NADA, EDVISTA_MUDOU };
typedef struct vista {
    int x, y, l, a;
    int zoom;                 /* milésimas */
    int ox, oy;               /* píxel (relativo a x, y) donde cae el origen del mundo */
    int grade;                /* paso de la rejilla en unidades del mundo (0 = sin rejilla) */
    bool iman;                /* imán: lo que se arrastra salta a la rejilla */
    bool hover;               /* el ratón está sobre la vista (Espacio solo arma la mano si es así) */
    bool mao;                 /* modo mano armado con Espacio: el siguiente arrastre con el izquierdo desplaza */
    /* rectángulo seleccionado, con asas */
    bool tem_sel; int sx, sy, sl, sa, srot;    /* mundo; srot en grados (giro alrededor del centro) */
    int modo, asa;            /* gesto en curso: 0 nada, 1 desplazar vista, 2 rectángulo, 3 mover, 4 escalar, 5 girar */
    int ax, ay, ax0, ay0;     /* ancla del gesto (pantalla) y estado de partida */
    int rx0, ry0, rx1, ry1;   /* rectángulo de selección (mundo) mientras se arrastra */
    int s0x, s0y, s0l, s0a, s0rot;
    int sobre_x, sobre_y;     /* último punto del ratón en el mundo */
    void *ctx;
    /* La aplicación dibuja su contenido con edvista_ret / edvista_linha / edvista_texto... (ya recortado a la vista). */
    void (*desenhar)(void *ctx, jan_t *j, struct vista *v);
    int  (*acertar)(void *ctx, int wx, int wy);                     /* objeto bajo el punto del mundo, o -1 */
    /* aviso: ev = EDVISTA_E_*; a..d según el evento (ver docs). fim = true al soltar. */
    void (*aviso)(void *ctx, int ev, int a, int b, int c, int d, bool fim);
} edvista_t;
void edvista_iniciar(edvista_t *v, int x, int y, int l, int a);
void edvista_mover(edvista_t *v, int x, int y, int l, int a);
void edvista_desenhar(jan_t *j, edvista_t *v);
int  edvista_tratar(jan_t *j, edvista_t *v, const evento_t *e);
void edvista_para_tela(const edvista_t *v, int wx, int wy, int *sx, int *sy);   /* mundo -> ventana */
void edvista_para_mundo(const edvista_t *v, int sx, int sy, int *wx, int *wy);  /* ventana -> mundo */
void edvista_zoom(edvista_t *v, int zoom, int cx, int cy);                      /* zoom en milésimas alrededor del punto de pantalla (cx, cy) */
void edvista_centrar(edvista_t *v, int wx, int wy);
int  edvista_iman(const edvista_t *v, int w);                                   /* w salta a la rejilla si iman */
void edvista_selecionar(edvista_t *v, int x, int y, int l, int a, int rot);     /* rectángulo con asas; l = 0 y a = 0 lo quita */
/* Primitivas de dibujo en coordenadas del mundo, recortadas a la vista. */
void edvista_ret(jan_t *j, const edvista_t *v, int wx, int wy, int wl, int wa, uint32_t cor);
void edvista_moldura(jan_t *j, const edvista_t *v, int wx, int wy, int wl, int wa, uint32_t cor);
void edvista_linha(jan_t *j, const edvista_t *v, int x0, int y0, int x1, int y1, uint32_t cor);
void edvista_texto(jan_t *j, const edvista_t *v, int wx, int wy, const char *t, uint32_t cor);   /* el texto no cambia de tamaño con el zoom */
int  edvista_seno(int graus);                                                 /* seno x 1000 */

/* ======================================================================== paleta de casillas */
enum { EDPALETA_NADA, EDPALETA_MUDOU, EDPALETA_ESCOLHEU };
typedef struct paleta {
    int x, y, l, a;
    const imagem_t *img;      /* la imagen dividida en celdas (puede ser NULL si hay desenhar_celda) */
    int cel_l, cel_a;         /* tamaño de cada celda en la imagen */
    int cols, filas;
    int escala;               /* 1..4: tamaño en pantalla de cada celda = cel * escala */
    int c0, f0, c1, f1;       /* selección: esquina de partida y esquina actual (cualquier orden) */
    int cx, cy;               /* desplazamiento en píxeles de pantalla */
    bool arrastando, foco;
    void *ctx;
    void (*desenhar_celda)(void *ctx, jan_t *j, int indice, int x, int y, int l, int a);   /* celdas sin imagen */
} edpaleta_t;
void edpaleta_iniciar(edpaleta_t *p, int x, int y, int l, int a, const imagem_t *img, int cel_l, int cel_a);
void edpaleta_dimensoes(edpaleta_t *p, int cols, int filas);                    /* sin imagen: número de celdas */
void edpaleta_desenhar(jan_t *j, edpaleta_t *p);
int  edpaleta_tratar(jan_t *j, edpaleta_t *p, const evento_t *e);
void edpaleta_selecao(const edpaleta_t *p, int *c0, int *f0, int *c1, int *f1); /* normalizada: c0 <= c1 y f0 <= f1 */
int  edpaleta_indice(const edpaleta_t *p, int col, int fila);
int  edpaleta_selecao_indices(const edpaleta_t *p, int *saida, int max);        /* índices fila a fila; devuelve cuántos */
void edpaleta_definir_selecao(edpaleta_t *p, int c0, int f0, int c1, int f1);

/* ======================================================================== deshacer / rehacer */
typedef struct {
    void (*aplicar)(void *dados, bool refazer);   /* refazer = false: deshace; true: rehace */
    void (*liberar)(void *dados);
    void *dados;
    char nome[32];
    int  grupo;
} ordem_t;
typedef struct {
    ordem_t *o;
    int n, cap, pos;          /* pos: cuántas están aplicadas (las demás son el rehacer) */
    int max;                  /* máximo de órdenes que se recuerdan (las más viejas se sueltan) */
    int grupo, prox;          /* grupo abierto (0 = ninguno) y siguiente número */
    int prof;                 /* anidamiento de agrupar_inicio */
    char nome_grupo[32];
} pilha_desfazer_t;
void ed_desfazer_iniciar(pilha_desfazer_t *p, int max);
void ed_desfazer_liberar(pilha_desfazer_t *p);
/* Registra una orden que YA se ejecutó (borra el rehacer). Devuelve false si no hay memoria. */
bool ed_desfazer_registrar(pilha_desfazer_t *p, const char *nome, void (*aplicar)(void *, bool), void (*liberar)(void *), void *dados);
void ed_desfazer_agrupar_inicio(pilha_desfazer_t *p, const char *nome);        /* lo que se registre hasta _fim se deshace de una vez */
void ed_desfazer_agrupar_fim(pilha_desfazer_t *p);
bool ed_desfazer(pilha_desfazer_t *p);                                         /* false = nada que deshacer */
bool ed_refazer(pilha_desfazer_t *p);
bool ed_desfazer_pode(const pilha_desfazer_t *p);
bool ed_refazer_pode(const pilha_desfazer_t *p);
const char *ed_desfazer_nome(const pilha_desfazer_t *p);                       /* de la próxima orden a deshacer (NULL = ninguna) */
const char *ed_refazer_nome(const pilha_desfazer_t *p);
void *ed_desfazer_topo(const pilha_desfazer_t *p);                             /* datos de la última orden aplicada (para fundir tecleos) */
void ed_desfazer_limpar(pilha_desfazer_t *p);

/* ======================================================================== arrastrar y soltar */
#define ARRSOL_MAX_DADOS 64
#define ARRSOL_ALVOS_MAX 16
enum { ARRSOL_ENTRA, ARRSOL_MOVE, ARRSOL_SAI, ARRSOL_SOLTA };             /* fases del aviso a un destino */
enum { ARRSOL_NADA, ARRSOL_ATIVO, ARRSOL_FIM, ARRSOL_CANCELOU };          /* lo que devuelve arrsol_tratar */
typedef struct {
    int tipo;                                                 /* lo define la aplicación (> 0) */
    unsigned char dados[ARRSOL_MAX_DADOS];
    int n;
    char rotulo[32];                                          /* lo que se ve junto al puntero */
    int origem;                                               /* quien lo empezó (libre) */
} arrsol_item_t;
/* El destino devuelve true si acepta el elemento (ENTRA y MOVE: se resalta; SOLTA: lo consumió). */
typedef bool (*arrsol_aviso_t)(void *ctx, int fase, const arrsol_item_t *it, int x, int y);
void arrsol_alvos_limpar(void);
int  arrsol_alvo(int x, int y, int l, int a, arrsol_aviso_t cb, void *ctx);   /* registra un destino; -1 si no cabe */
void arrsol_preparar(const arrsol_item_t *it, int x, int y);        /* en EV_CLIQUE: se activa al moverse más de 4 px con el botón */
int  arrsol_tratar(jan_t *j, const evento_t *e);                 /* llamarlo primero con cada evento; si no devuelve ARRSOL_NADA el evento es suyo */
bool arrsol_ativo(void);
const arrsol_item_t *arrsol_item(void);
int  arrsol_alvo_atual(void);                                    /* destino bajo el puntero que acepta (-1 = ninguno) */

/* ======================================================================== consola de salida */
#define CONSOLA_MAX 256
#define CONSOLA_TXT 120
enum { CON_MSG, CON_AVISO, CON_ERRO };
enum { CONSOLA_NADA, CONSOLA_MUDOU, CONSOLA_IR };             /* IR: doble clic en una línea con posición (consola_ir_linha) */
typedef struct {
    char texto[CONSOLA_TXT];
    unsigned char nivel;
    int  linha;                                               /* línea de código a la que remite (0 = ninguna) */
} con_linha_t;
typedef struct {
    int x, y, l, a;
    con_linha_t lin[CONSOLA_MAX];
    int n;                                                    /* líneas guardadas (anillo: las más viejas se pierden) */
    int cont[3];                                              /* cuántas hay de cada nivel */
    unsigned mascara;                                         /* niveles visibles: bit CON_MSG, CON_AVISO, CON_ERRO */
    int topo;                                                 /* primera fila visible del filtrado */
    int sel;                                                  /* fila elegida del filtrado (-1) */
    bool seguir;                                              /* pegado al final */
    int ir_linha;                                             /* línea de la última orden CONSOLA_IR */
    int copiado;                                              /* bytes de la última copia */
} consola_t;
void consola_iniciar(consola_t *c, int x, int y, int l, int a);
void consola_escrever(consola_t *c, int nivel, const char *texto);
void consola_escrever_linha(consola_t *c, int nivel, int linha, const char *texto);
void consola_limpar(consola_t *c);
int  consola_visiveis(const consola_t *c);                    /* filas que pasan el filtro */
bool consola_boton(const consola_t *c, int k, int *x, int *y, int *l, int *a);   /* los botones de la cabecera: 0 mensajes, 1 avisos, 2 errores, 3 copiar, 4 limpiar */
int  consola_copiar(consola_t *c);                            /* al portapapeles las visibles; devuelve los bytes */
void consola_desenhar(jan_t *j, consola_t *c);
int  consola_tratar(jan_t *j, consola_t *c, const evento_t *e);

/* ======================================================================== editor de código con resaltado */
enum { RC_NORMAL, RC_CHAVE, RC_TIPO, RC_CADEIA, RC_COMENTARIO, RC_NUMERO, RC_FUNCAO, RC_OPERADOR, RC_PREPROC, RC_CONSTANTE, RC_QTD };
typedef struct {
    const char *nome;
    const char *linha_com;                                    /* comentario de línea: «--» o «//» */
    const char *bloco_ini, *bloco_fim;                        /* comentario de bloque: «--[[» «]]» o barra-asterisco y asterisco-barra (C) */
    const char *cadeia_ini, *cadeia_fim;                      /* cadena larga (Lua [[ ]]); NULL = no hay */
    const char *aspas;                                        /* delimitadores de cadena: «"'» */
    bool escapes;
    bool sem_caixa;                                           /* palabras sin distinguir mayúsculas */
    char preproc;                                             /* «#» al comienzo de la línea (C) o 0 */
    const char *const *chaves, *const *tipos, *const *funcoes, *const *constantes;   /* tablas terminadas en NULL */
} edrealce_regras_t;
extern const edrealce_regras_t edrealce_lua, edrealce_c;
/* Clasifica una línea: classes[i] = RC_* de cada letra; estado 0 = normal, 1 = comentario de bloque, 2 = cadena larga.
 * Devuelve el estado con el que empieza la línea siguiente. classes puede ser NULL (solo sigue el estado). */
int  edrealce_linha(const edrealce_regras_t *r, const char *s, int n, int estado, unsigned char *classes);
uint32_t edrealce_cor(int classe);                              /* color de una clase con el esquema vigente */

#define CODIGO_NADA   0
#define CODIGO_MUDOU  1                                       /* cambió el texto */
#define CODIGO_MOVEU  2                                       /* solo el cursor o el desplazamiento */
typedef struct codigo codigo_t;
codigo_t *codigo_criar(int x, int y, int l, int a);
void codigo_liberar(codigo_t *c);
void codigo_mover(codigo_t *c, int x, int y, int l, int a);
void codigo_regras(codigo_t *c, const edrealce_regras_t *r);    /* NULL = sin resaltado */
void codigo_definir(codigo_t *c, const char *texto, int n);   /* sustituye todo; limpia deshacer y error */
const char *codigo_texto(const codigo_t *c, int *n);
void codigo_desenhar(jan_t *j, codigo_t *c);
int  codigo_tratar(jan_t *j, codigo_t *c, const evento_t *e);
void codigo_marcar_erro(codigo_t *c, int linha, const char *msg);   /* linha 1..n; 0 la quita */
bool codigo_buscar(codigo_t *c, const char *txt, bool adiante, bool caso);   /* selecciona la siguiente coincidencia desde el cursor */
void codigo_ir_linha(codigo_t *c, int linha);                 /* 1..n */
int  codigo_linha(const codigo_t *c);                         /* línea del cursor, desde 1 */
int  codigo_coluna(const codigo_t *c);                        /* columna del cursor, desde 1 */
int  codigo_linhas(const codigo_t *c);
bool codigo_desfazer(codigo_t *c);
bool codigo_refazer(codigo_t *c);
bool codigo_modificado(const codigo_t *c);
void codigo_foco(codigo_t *c, bool foco);
void codigo_sangria(codigo_t *c, int espacos);                /* ancho del sangrado automático (por defecto 2) */
void codigo_inserir(codigo_t *c, const char *t, int n);       /* en el cursor, como si se tecleara (deshacer lo cubre) */
const char *codigo_erro(const codigo_t *c, int *linha);       /* mensaje marcado o NULL */
int  codigo_selecao(const codigo_t *c, char *dst, int max);   /* texto seleccionado; devuelve el tamaño */

#endif
