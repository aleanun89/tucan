// SPDX-License-Identifier: GPL-3.0-only
/* gui.h - kit gráfico de libsagui (H22).
 *
 * Todo aquí corre en el propio programa, en el anillo 3: el kernel sólo entrega el área
 * de píxeles de la ventana y los eventos. Cada componente es una estructura con la
 * posición y el estado, una función que dibuja y otra que trata un evento. */
#ifndef SAGUI_GUI_H
#define SAGUI_GUI_H
#include "sagui.h"
#include "imagem.h"   /* imagem_t, para el lienzo (mejora 042) */

#include "esquemas.h"

/* Esquema de apariencia (D428): todos los colores del sistema en una estructura (kernel/inc/esquemas.h) que el kernel
 * guarda y la libsagui lee. `sagui_esq` se carga al abrir la primera ventana y se recarga solo cuando llega EV_TEMA (lo hace
 * proximo_evento / esperar_evento), así que COR_* y W98_* valen siempre lo del esquema vigente: con «SaguiOS»
 * (el de serie), los de antes. Los programas que no tratan EV_TEMA se repintan con los colores nuevos la próxima vez
 * que dibujan. */
extern esquema_t sagui_esq;
void esq_asegurar(void);          /* carga el esquema del kernel si cambió (la versión); lo llaman jan_abrir y el bucle de eventos */
int  esq_recargar(void);          /* lo lee del kernel y rehace mod_tema; 0 = bien */
int  esq_aplicar(const esquema_t *e, bool login);   /* lo pone en el sistema (redibuja todo y avisa a las ventanas); login = el de la pantalla de inicio (root) */
void esq_para_mod_tema(void);     /* rehace mod_tema desde sagui_esq */
/* Una variante clara de un color de trazo oscuro para fondos oscuros (iconos, D428): sube la luminosidad si sagui_esq.escuro. */
uint32_t esq_trazo(uint32_t c);
int  esq_para_texto(const esquema_t *e, char *buf, int max);   /* formato de /home/.config/estilo.conf (D428) */
bool esq_de_texto(const char *txt, esquema_t *e);

/* Colores del tema. */
#define COR_FUNDO      (sagui_esq.fundo)
#define COR_TEXTO      (sagui_esq.texto)
#define COR_TEXTO_FRACO (sagui_esq.texto_fraco)
#define COR_BORDA      (sagui_esq.borda)
#define COR_BOTAO      (sagui_esq.botao)
#define COR_BOTAO_APERTADO (sagui_esq.botao_apertado)
#define COR_DESTAQUE   (sagui_esq.destaque)   /* verde de SaguiOS */
#define COR_SELECAO    (sagui_esq.selecao)
#define COR_BRANCO     0xFFFFFF
#define COR_CAMPO      (sagui_esq.campo_fundo)   /* fondo de campos, listas y cuadros de texto (era COR_BRANCO) */
#define COR_SEL_TEXTO  (sagui_esq.selecao_texto) /* texto sobre la selección (era COR_BRANCO) */
#define COR_SEL_SUAVE  (sagui_esq.sel_suave)     /* fondo de la selección de texto dentro de un campo */
#define COR_LISTA_FUNDO (sagui_esq.lista_fundo)

typedef struct {
    int       id;
    uint32_t *px;
    int       larg, alt;
    unsigned  estilo;          /* lo que se pidió en jan_estilo (D044) */
    int       min_l, min_a;
} jan_t;
void esq_vista_previa(jan_t *j, int x, int y, int l, int a, const esquema_t *e);   /* miniatura 320x200: escritorio, ventanas, menú, botón, campo */

/* ---- ventana y dibujo ---- */
jan_t *jan_abrir(const char *titulo, int largura, int altura);
void   jan_fechar(jan_t *j);
void   jan_mostrar(jan_t *j);                                   /* toda la ventana */
void   jan_mostrar_area(jan_t *j, int x, int y, int l, int a);
/* Tamaño variable (D044): jan_estilo activa JAN_REDIMENSIONAVEL y/o
 * JAN_TELA_CHEIA_OK con el menor tamaño aceptado. Después, en el bucle de eventos:
 *     if (jan_redimensionar(j, &e)) { rehacer el layout con j->larg, j->alt y redibujar }
 * (el kernel ya copió el dibujo anterior a la esquina de arriba a la izquierda). */
int    jan_estilo(jan_t *j, unsigned estilo, int min_l, int min_a);
bool   jan_redimensionar(jan_t *j, const evento_t *e);    /* true = EV_REDIMENSIONAR aceptado */
bool   jan_tela_cheia(jan_t *j, bool ligar);              /* pide (o suelta) la pantalla completa */
bool   jan_cursor_visivel(jan_t *j, bool visivel);        /* D230: oculta el puntero del sistema sobre la ventana */
void   g_retangulo(jan_t *j, int x, int y, int l, int a, uint32_t cor);
void   g_moldura(jan_t *j, int x, int y, int l, int a, uint32_t cor);
void   g_linha(jan_t *j, int x0, int y0, int x1, int y1, uint32_t cor);
/* Copia un bloque l x a de (sx, sy) a (dx, dy) dentro de la propia ventana (D102). Acepta áreas que
 * se superponen y recorta lo que sale de la ventana. */
void   g_copiar_area(jan_t *j, int sx, int sy, int l, int a, int dx, int dy);
void   g_texto(jan_t *j, int x, int y, const char *t, uint32_t cor);
void   g_texto_n(jan_t *j, int x, int y, const char *t, int n, uint32_t cor);
void   g_desenho(jan_t *j, int x, int y, const char *const *linhas, int l, int a,
                 uint32_t (*paleta)(char), int escala);   /* paleta devuelve 0xFF000000 = transparente */
void   g_relevo(jan_t *j, int x, int y, int l, int a, bool afundado);
int    g_largura_texto(const char *t);
#define LETRA_L 8
#define LETRA_A 16

/* ---- etiqueta ---- */
void rotulo(jan_t *j, int x, int y, const char *texto, uint32_t cor);

/* ---- botón ---- */
typedef struct {
    int x, y, l, a;
    const char *texto;
    bool apertado;
} botao_t;
void botao_desenhar(jan_t *j, botao_t *b);
bool botao_tratar(jan_t *j, botao_t *b, const evento_t *e);   /* true = fue pulsado */

/* ---- cuadro de texto de una línea ---- */
typedef struct caixa_gui {
    int  x, y, l;
    char texto[128];
    int  cursor;
    bool foco;
    bool oculta;   /* D145: si es cierto, el texto se dibuja con asteriscos
                    * (para las claves de API). */
    /* D423: dibujo propio. Si no es NULL, caixa_tratar lo usa en lugar de caixa_desenhar al redibujar tras
     * escribir o hacer clic (así Archivos pone su campo moderno sin que el antiguo asome un instante).
     * Al final de la estructura: los inicializadores posicionales antiguos lo dejan en NULL. */
    void (*desenhar)(jan_t *j, struct caixa_gui *c);
} caixa_t;
void caixa_desenhar(jan_t *j, caixa_t *c);
bool caixa_tratar(jan_t *j, caixa_t *c, const evento_t *e);   /* true = Enter */
void caixa_definir(caixa_t *c, const char *texto);

/* ---- área de texto con varias líneas y desplazamiento ---- */
typedef struct {
    int   x, y, l, a;
    char *buf;            /* texto (Latin-1), terminado en cero */
    int   tam, cap;
    int   cursor;         /* posición en el búfer */
    int   topo;           /* primera línea visible */
    bool  foco;
    bool  modificado;
    bool  somente_leitura;
    int   ancora;         /* otro extremo de la selección (D074); -1 = sin selección */
    bool  arrastando;     /* botón apretado: mover el ratón selecciona */
} area_t;
bool area_iniciar(area_t *a, int x, int y, int l, int al);
void area_definir(area_t *a, const char *texto, int n);
void area_desenhar(jan_t *j, area_t *a);
bool area_tratar(jan_t *j, area_t *a, const evento_t *e);    /* true = cambió el texto; EV_RODA desplaza (D048) */
int  area_linhas_visiveis(area_t *a);
void area_mudar_tamanho(area_t *a, int l, int al);          /* después de redimensionar la ventana */
/* Selección con Shift o arrastrando; Ctrl+A/C/X/V en la propia area_tratar (D074).
 * Para los menús: */
bool area_copiar(area_t *a);              /* false = no hay selección */
bool area_cortar(jan_t *j, area_t *a);
bool area_colar(jan_t *j, area_t *a);     /* true = pegó algo */

/* ---- lista con selección ---- */
#define LISTA_NADA       0
#define LISTA_SELECIONOU 1
#define LISTA_ATIVOU     2   /* doble clic ou Enter */
typedef struct lista_gui {
    int    x, y, l, a;
    char (*itens)[64];
    int    qtd;
    int    sel;
    int    topo;
    /* Dibuja un icono pequeño (16x16) antes del texto del ítem, si no es NULL. */
    void (*icone)(jan_t *j, int x, int y, int item);
    /* Dibujo propio (D217): si no es NULL, lista_tratar lo usa en lugar de lista_desenhar al
     * redibujar tras la rueda, el teclado o un clic. Así un programa con una vista propia
     * (Archivos: iconos, detalles) no ve su lista cambiada por la genérica mientras rueda.
     * Al final de la estructura: los inicializadores posicionales antiguos lo dejan en NULL. */
    void (*desenhar)(jan_t *j, struct lista_gui *l);
} lista_t;
void lista_desenhar(jan_t *j, lista_t *l);
int  lista_tratar(jan_t *j, lista_t *l, const evento_t *e);   /* EV_RODA desplaza RODA_LINHAS ítems por muesca; RePág/AvPág/Inicio/Fin (D217) */
#define LISTA_ALTURA_ITEM 20

/* ---- diálogos (ventanas propias, esperan la respuesta) ---- */
bool confirmar(const char *titulo, const char *pergunta);      /* "¿Seguro?" Sí/No */
void mensagem(const char *titulo, const char *texto);           /* OK */
bool pedir_texto(const char *titulo, const char *pergunta, char *resposta, int max);
/* D340: lo mismo con asteriscos (contraseñas). */
bool pedir_texto_oculto(const char *titulo, const char *pergunta, char *resposta, int max);

/* ---- diálogos de archivos estilo Windows 95 (mejora 103, D272; gui_arquivos.c) ----
 * Modales; devuelven true y la ruta absoluta en "saida" si se aceptó. "filtros":
 * "Texto (*.txt)|*.txt|Todos|*" (NULL = todos); un «>» antes del nombre de un tipo lo deja elegido de entrada. "inicial": carpeta, o carpeta + nombre propuesto.
 * Guardar pide confirmación al sobrescribir y añade la extensión del tipo si falta. */
bool dialogo_abrir(const char *titulo, const char *filtros, const char *inicial, char *saida, int max);
bool dialogo_guardar(const char *titulo, const char *filtros, const char *inicial, char *saida, int max);
bool dialogo_carpeta(const char *titulo, const char *inicial, char *saida, int max);

/* ---- lienzo (mejora 042): superficie con elementos retenidos ----
 *
 * Los elementos se guardan en coordenadas del lienzo y se ven a través de una
 * ventana con desplazamiento y zoom. El zoom va en punto fijo 16.16
 * (0x10000 = 1.0), porque libsagui se compila sin coma flotante (D076). */
typedef struct lenzo lenzo_t;

lenzo_t *lenzo_criar(int x, int y, int l, int a);
void     lenzo_liberar(lenzo_t *c);
void     lenzo_limpar(lenzo_t *c);
/* Añadir elementos; devuelven el índice del elemento o -1 si no hay memoria.
 * Las coordenadas van en píxeles del lienzo. */
int      lenzo_linha(lenzo_t *c, int x0, int y0, int x1, int y1, uint32_t cor, int espessura);
int      lenzo_retangulo(lenzo_t *c, int x, int y, int l, int a, uint32_t cor,
                         uint32_t cor_linha, bool preenchido, int espessura);
int      lenzo_elipse(lenzo_t *c, int x, int y, int l, int a, uint32_t cor,
                      uint32_t cor_linha, bool preenchido, int espessura);
/* "pontos" son pares (x, y); n es el número de puntos (máx. 64). */
int      lenzo_poligono(lenzo_t *c, const int *pontos, int n, uint32_t cor,
                        uint32_t cor_linha, bool preenchido, int espessura);
int      lenzo_texto(lenzo_t *c, int x, int y, const char *texto, uint32_t cor);
/* El lienzo guarda una copia de la imagen; l/a = 0 para el tamaño natural. */
int      lenzo_imagem(lenzo_t *c, int x, int y, const imagem_t *img, int l, int a);

void     lenzo_desenhar(jan_t *j, lenzo_t *c);
bool     lenzo_tratar(jan_t *j, lenzo_t *c, const evento_t *e);   /* true = hay que redibujar */
void     lenzo_deslocar(lenzo_t *c, int dx, int dy);
void     lenzo_zoom(lenzo_t *c, int por_100);      /* 100 = normal; de 25 a 400 */
int      lenzo_elementos(lenzo_t *c);
int      lenzo_em_baixo_de(lenzo_t *c, int lx, int ly);   /* en coordenadas del lienzo */
int      lenzo_sobre_mouse(lenzo_t *c);      /* elemento bajo el ratón (-1 = ninguno) */
int      lenzo_selecionado(lenzo_t *c);      /* elemento elegido con un clic (-1 = ninguno) */

/* ---- barra de progreso (mejora 042) ---- */
typedef struct progresso progresso_t;

progresso_t *progresso_criar(int x, int y, int l, int a, bool vertical);
void         progresso_definir(progresso_t *p, int valor);      /* 0..100 */
void         progresso_indeterminado(progresso_t *p, bool ligar); /* banda que se mueve */
void         progresso_texto(progresso_t *p, bool mostrar);     /* escribe "42%" */
void         progresso_desenhar(jan_t *j, progresso_t *p);
bool         progresso_avancar(progresso_t *p);   /* mueve la banda; true = redibujar */
void         progresso_rect(progresso_t *p, retangulo_t *r);   /* sitio que ocupa */
void         progresso_liberar(progresso_t *p);   /* suelta la barra (al redimensionar) */

/* ---- lista en árbol (D367, mejora 124) ----
 * Los ítems van en orden de recorrido con su nivel (0 = raíz); tiene hijos el que va seguido de un
 * nivel más hondo. aberto[i] dice si la rama i está desplegada. */
#define ARVORE_NADA      0
#define ARVORE_ESCOLHEU  1   /* cambió el ítem elegido (clic en la fila o teclado) */
#define ARVORE_ALTERNOU  2   /* solo se abrió o cerró una rama */
typedef struct {
    int x, y, l, a;
    int n;
    const char *const *textos;
    const unsigned char *nivel;
    unsigned char *aberto;
    int sel;                 /* ítem elegido (índice en la tabla completa) */
    int topo;                /* primera fila visible desplazada */
} arvore_t;
void arvore_desenhar(jan_t *j, arvore_t *a);
int  arvore_tratar(jan_t *j, arvore_t *a, const evento_t *e);
bool arvore_tem_filhos(const arvore_t *a, int i);
bool arvore_visivel(const arvore_t *a, int i);
int  arvore_pai(const arvore_t *a, int i);          /* -1 = raíz */
void arvore_mostrar(arvore_t *a, int i);            /* abre los antepasados y lo deja a la vista */

/* ---- casilla de verificación (mejora 042) ---- */
typedef struct {
    int  x, y;
    bool marcada;
} casilla_t;
void casilla_desenhar(jan_t *j, casilla_t *c);
bool casilla_tratar(casilla_t *c, const evento_t *e);   /* true = cambió */

/* ---- visor de texto con estilos: Markdown por secciones (D239) ----
 *
 * Muestra texto con formato (negrita, cursiva, títulos, listas, citas, código
 * en línea y bloques de código con fondo y botón «Copiar» al portapapeles) en
 * un rectángulo de la ventana, con desplazamiento. El documento es una lista
 * de secciones (un mensaje del chat, por ejemplo), cada una con un rótulo
 * opcional; rico_juntar añade texto al final de la última y solo vuelve a
 * maquetar esa (vale para el texto que llega en streaming). Texto Latin-1. */
#define RICO_NEGRITA  1
#define RICO_CURSIVA  2
#define RICO_CODIGO   4      /* código en línea */
#define RICO_ENLACE   8
#define RICO_TITULO   16
#define RICO_MONO     32     /* bloque de código o tabla */
/* Lo que devuelve rico_tratar. */
#define RICO_NADA     0
#define RICO_ROLOU    1      /* se desplazó (ya está redibujado) */
#define RICO_COPIOU   2      /* se pulsó «Copiar»: rico_copiado() dice cuántos bytes */
typedef struct {             /* lo que encontró el análisis de una sección */
    int negrita, cursiva, codigo_linha, titulos, itens_lista, blocos_codigo;
} rico_contagem_t;
typedef struct rico rico_t;
rico_t *rico_criar(int x, int y, int l, int a);
void    rico_liberar(rico_t *r);
void    rico_limpar(rico_t *r);
void    rico_mover(rico_t *r, int x, int y, int l, int a);   /* al redimensionar la ventana */
int     rico_secao(rico_t *r, const char *rotulo, uint32_t cor, bool markdown);   /* -> índice */
void    rico_juntar(rico_t *r, const char *t, int n);   /* al final de la última sección */
void    rico_remover_ultima(rico_t *r);
int     rico_secoes(const rico_t *r);
const char *rico_texto(const rico_t *r, int i, int *n);   /* la fuente de la sección i */
void    rico_desenhar(jan_t *j, rico_t *r);
int     rico_tratar(jan_t *j, rico_t *r, const evento_t *e);   /* rueda, RePág/AvPág, barra, «Copiar» */
bool    rico_no_fim(const rico_t *r);
void    rico_ir_fim(rico_t *r);
int     rico_secao_em(const rico_t *r, int x, int y);   /* sección bajo el punto, o -1 */
bool    rico_botao_copiar(const rico_t *r, int i, int k, retangulo_t *ret);   /* el k-ésimo de la sección i, si se ve */
int     rico_copiado(const rico_t *r);   /* bytes del último «Copiar» */
void    rico_contar(const rico_t *r, int i, rico_contagem_t *c);

/* ======================================================================
 * Componentes de aplicaciones de escritorio (mejora 116, D291): barra de menús, barra de
 * herramientas con iconos, lista desplegable, regla, barra de estado y el control de edición
 * de texto con formato (el de «Pluma»). Aspecto Windows 98: gris, biseles. Todos siguen el
 * patrón de siempre: una estructura del programa, una función que dibuja y otra que trata
 * el evento. Las medidas del documento van en TWIPS (1/20 de punto; 1 cm = 567 twips).
 * ====================================================================== */
#define W98_FACE    (sagui_esq.cara)     /* gris de los botones y las barras */
#define W98_SOMBRA  (sagui_esq.sombra)
#define W98_OSCURO  (sagui_esq.oscuro)
#define W98_LUZ     (sagui_esq.luz)
#define W98_AZUL    (sagui_esq.azul)     /* barra de título y selección */
#define W98_NEGRO   (sagui_esq.negro)
#define W98_CLARO   esq_mix(sagui_esq.cara, sagui_esq.luz, 50)   /* DFDFDF con el gris de siempre */
#define W98_MARCADA (sagui_esq.cara == 0xC0C0C0 ? 0xE0E0E0 : esq_mix(sagui_esq.cara, sagui_esq.luz, 50))   /* botón de barra activado */
void g_bisel(jan_t *j, int x, int y, int l, int a, bool afundado);   /* borde de 2 px estilo Win98 */
void g_flecha(jan_t *j, int x, int y, int dir, uint32_t cor);        /* triángulo 7x4: 0 arriba, 1 abajo, 2 izq., 3 der. */

/* ---- barra de menús (con menús desplegables del sistema) ----
 * itens[i] es el texto de su menú (líneas separadas por '\n', "-" = separador, "~x" = desactivada,
 * "Texto\tAtajo"). barra_menu_tratar devuelve BARRA_MENU_ITEM(menu, línea) al elegir, o -1. */
#define BARRA_MENU_MAX 8
#define BARRA_MENU_ITEM(m, k) ((m) * 100 + (k))
typedef struct {
    int x, y, l, a;
    int n;
    const char *nomes[BARRA_MENU_MAX];
    const char *itens[BARRA_MENU_MAX];
    int aberto;          /* menú desplegado (-1 = ninguno) */
    int x_item[BARRA_MENU_MAX], l_item[BARRA_MENU_MAX];
} barra_menu_t;
void barra_menu_iniciar(barra_menu_t *b, int x, int y, int l);   /* rellenar nomes/itens y n antes */
void barra_menu_desenhar(jan_t *j, barra_menu_t *b);
int  barra_menu_tratar(jan_t *j, barra_menu_t *b, const evento_t *e);

/* ---- barra de herramientas con iconos y botones conmutables ----
 * id > 0: botón (barra_ferr_tratar devuelve su id); id = 0: separador; id < 0: hueco de "larg"
 * píxeles para colocar otro componente (una lista desplegable). El icono es una función que dibuja
 * 16x16 en (x, y) (o un texto corto en "texto"; "negrita" lo dibuja en negrita). */
typedef struct {
    int  id;
    void (*icone)(jan_t *j, int x, int y, bool ativo);
    const char *texto;
    bool negrita;
    bool conmutavel;     /* botón de dos estados (N, C, S, alineación) */
    bool marcado;
    bool desativado;
    int  larg;           /* 0 = 23 px (separador: 8 px) */
} barra_item_t;
typedef struct {
    int x, y, l, a;
    barra_item_t *itens;
    int n;
    int apertado;        /* índice pulsado (-1 = ninguno) */
    int sobre;           /* índice bajo el ratón */
} barra_ferr_t;
void barra_ferr_iniciar(barra_ferr_t *b, int x, int y, int l, barra_item_t *itens, int n);
void barra_ferr_desenhar(jan_t *j, barra_ferr_t *b);
int  barra_ferr_tratar(jan_t *j, barra_ferr_t *b, const evento_t *e);   /* id pulsado o 0 */
int  barra_ferr_x(const barra_ferr_t *b, int indice);                    /* x del ítem (para los huecos) */
void barra_ferr_marcar(barra_ferr_t *b, int id, bool marcado);           /* cambia el estado de un conmutable */

/* ---- lista desplegable (combo): texto editable opcional + flecha ---- */
#define COMBO_NADA        0
#define COMBO_SELECIONOU  1     /* eligió un ítem de la lista */
#define COMBO_ENTROU      2     /* Enter en el texto editable */
typedef struct {
    int x, y, l, a;                  /* a = 21 por defecto (combo_iniciar) */
    const char *const *itens;
    int qtd, sel;
    bool editavel;
    char texto[32];
    int  cursor;
    bool foco;
    int  codigo;                     /* identificador del menú desplegable (único por ventana) */
} combo_t;
void combo_iniciar(combo_t *c, int x, int y, int l, const char *const *itens, int qtd, bool editavel, int codigo);
void combo_definir(combo_t *c, const char *texto);
void combo_desenhar(jan_t *j, combo_t *c);
int  combo_tratar(jan_t *j, combo_t *c, const evento_t *e);

/* ---- regla horizontal con márgenes y sangrías (en twips; ox = x del borde izquierdo de la hoja) ---- */
#define REGUA_NADA     0
#define REGUA_ARRASTA  1       /* se está moviendo un marcador (ya redibujada) */
#define REGUA_MUDOU    2       /* soltó: hay un valor nuevo en regua_t */
#define REGUA_M_ESQ    1       /* qué se movió (regua_t.marcador) */
#define REGUA_M_DIR    2
#define REGUA_M_PRIM   3       /* sangría de primera línea */
#define REGUA_M_SANG   4       /* sangría izquierda (colgante) */
#define REGUA_M_SANG_D 5       /* sangría derecha */
typedef struct {
    int x, y, l, a;
    int ox, zoom;                 /* píxel de la hoja y zoom en % */
    int pagina;                   /* ancho de la hoja */
    int m_esq, m_dir;             /* márgenes */
    int sang_esq, sang_prim, sang_dir;   /* respecto al margen (prim: relativa al margen izquierdo, puede ser negativa) */
    int marcador;                 /* el último arrastrado */
    int arrastando;
} regua_t;
void regua_iniciar(regua_t *r, int x, int y, int l);
void regua_desenhar(jan_t *j, regua_t *r);
int  regua_tratar(jan_t *j, regua_t *r, const evento_t *e);

/* ---- barra de estado con secciones ----
 * Anchos fijos en larg[]; la última sección ocupa el resto. */
#define BARRA_ESTADO_MAX 8
typedef struct {
    int x, y, l, a;
    int n;
    int larg[BARRA_ESTADO_MAX];
    char texto[BARRA_ESTADO_MAX][64];
} barra_estado_t;
void barra_estado_iniciar(barra_estado_t *b, int x, int y, int l, int n, const int *larg);
void barra_estado_texto(barra_estado_t *b, int i, const char *texto);
void barra_estado_desenhar(jan_t *j, barra_estado_t *b);

/* ---- control de edición de texto con formato (mejora 116) ----
 * Vista de página (hoja blanca sobre fondo gris, con zoom), formato por caracteres (fuente, tamaño,
 * negrita/cursiva/subrayado/tachado, color) y por párrafos (alineación, sangrías, interlineado,
 * viñetas, espaciado), selección con ratón y teclado, portapapeles, deshacer/rehacer y buscar. Texto en
 * Latin-1; el párrafo termina en '\n' y '\f' es un salto de página. Las medidas de las fuentes son las
 * de Helvetica/Times/Courier (las mismas del PDF), de modo que la paginación en pantalla y en el PDF
 * coinciden. En pantalla se dibujan con la fuente de mapa de bits escalada. */
#define RTE_SANS   0
#define RTE_SERIF  1
#define RTE_MONO   2
#define RTE_NEGRITA    1
#define RTE_CURSIVA    2
#define RTE_SUBLINHADO 4
#define RTE_RISCADO    8
typedef struct { uint8_t fam, est, tam; uint32_t cor; } rte_fmt_t;     /* tam en puntos */
#define RTE_ESQ    0
#define RTE_CENTRO 1
#define RTE_DIR    2
#define RTE_JUST   3
typedef struct {
    uint8_t alin, marcador;          /* marcador: 1 = viñeta */
    int16_t esq, dir, prim;          /* sangrías en twips (prim relativa a esq: negativa = colgante) */
    uint16_t interlin;               /* % (100, 150, 200) */
    int16_t antes, depois;           /* espaciado en twips */
} rte_par_t;
/* campos de rte_fmt_aplicar / rte_par_aplicar */
#define RTE_C_FAM   1
#define RTE_C_TAM   2
#define RTE_C_COR   4
#define RTE_P_ALIN    1
#define RTE_P_SANG    2
#define RTE_P_INTER   4
#define RTE_P_MARC    8
#define RTE_P_ESPACO  16
/* lo que devuelve rte_tratar (máscara) */
#define RTE_R_MUDOU   1      /* cambió el documento */
#define RTE_R_CURSOR  2      /* se movió el cursor o la selección (actualizar barra y estado) */
#define RTE_R_OBJ     4      /* doble clic en una imagen o un WordArt: el programa abre su diálogo (D296) */
typedef struct rte rte_t;
rte_t *rte_criar(int x, int y, int l, int a);
void   rte_liberar(rte_t *r);
void   rte_mover(rte_t *r, int x, int y, int l, int a);
void   rte_limpar(rte_t *r);
void   rte_papel(rte_t *r, int largura, int altura, int m_esq, int m_dir, int m_sup, int m_inf);   /* twips */
void   rte_papel_obter(const rte_t *r, int *largura, int *altura, int *m_esq, int *m_dir, int *m_sup, int *m_inf);
void   rte_zoom(rte_t *r, int por_100);                /* 25..400 */
int    rte_zoom_obter(const rte_t *r);
void   rte_foco(rte_t *r, bool foco);
bool   rte_modificado(const rte_t *r);
void   rte_marcar_limpo(rte_t *r);
void   rte_desenhar(jan_t *j, rte_t *r);
int    rte_tratar(jan_t *j, rte_t *r, const evento_t *e);
int    rte_pagina_x(rte_t *r);                         /* x (en la ventana) del borde izquierdo de la hoja */
/* contenido */
int    rte_tamanho(const rte_t *r);
const char *rte_texto(const rte_t *r);
void   rte_doc_anexar(rte_t *r, const char *t, int n, const rte_fmt_t *f);   /* carga: añade al final */
void   rte_doc_par(rte_t *r, const rte_par_t *p);                            /* atributos del último párrafo */
void   rte_doc_fim(rte_t *r);                                                /* acabó la carga */
int    rte_n_par(const rte_t *r);
bool   rte_par_dados(const rte_t *r, int i, int *ini, int *fim, rte_par_t *p);
void   rte_fmt_em(const rte_t *r, int pos, rte_fmt_t *f);
/* cursor y selección */
int    rte_cursor(const rte_t *r);
bool   rte_selecao(const rte_t *r, int *a, int *b);
void   rte_selecionar(rte_t *r, int a, int b);
void   rte_linha_coluna(rte_t *r, int *lin, int *col);
int    rte_pagina_atual(rte_t *r);
int    rte_paginas(rte_t *r);
void   rte_ir_pagina(rte_t *r, int pagina);
/* formato (sobre la selección o, sin ella, sobre lo que se escriba después) */
void   rte_fmt_atual(const rte_t *r, rte_fmt_t *f);
void   rte_fmt_aplicar(rte_t *r, unsigned campos, const rte_fmt_t *f);
void   rte_estilo(rte_t *r, unsigned bits, bool ligar);
void   rte_par_atual(const rte_t *r, rte_par_t *p);
void   rte_par_aplicar(rte_t *r, unsigned campos, const rte_par_t *p);
/* edición */
void   rte_inserir(rte_t *r, const char *t, int n);        /* sustituye la selección */
void   rte_salto_pagina(rte_t *r);
bool   rte_copiar(rte_t *r);
bool   rte_cortar(jan_t *j, rte_t *r);
bool   rte_colar(jan_t *j, rte_t *r);
void   rte_selecionar_tudo(rte_t *r);
bool   rte_desfazer(rte_t *r);
bool   rte_refazer(rte_t *r);
void   rte_grupo_iniciar(rte_t *r);                  /* lo que se haga hasta rte_grupo_fim es un solo paso de deshacer */
void   rte_grupo_fim(rte_t *r);
bool   rte_pode_desfazer(const rte_t *r);
bool   rte_pode_refazer(const rte_t *r);
/* buscar y reemplazar */
bool   rte_buscar(rte_t *r, const char *t, bool maiusculas, bool adiante);   /* selecciona la siguiente coincidencia */
bool   rte_substituir(rte_t *r, const char *por);                            /* si la selección es la coincidencia */
int    rte_substituir_tudo(rte_t *r, const char *t, const char *por, bool maiusculas);
void   rte_contar(const rte_t *r, int *palavras, int *caracteres, int *paragrafos);
/* ---- objetos incrustados: imágenes y WordArt (mejora 122, D296) ----
 * Un objeto ocupa un carácter (RTE_OBJ_CHAR) del texto, con su propia entrada en la tabla de
 * formatos. Se selecciona con un clic (tiradores para cambiar el tamaño; con Mayús, libre), se mueve
 * arrastrándolo y su doble clic devuelve RTE_R_OBJ. Los datos pesados (píxeles, JPEG o PNG
 * originales) se comparten entre las versiones de un objeto, así que deshacer cambia también el
 * tamaño y las propiedades. */
#define RTE_OBJ_CHAR     1
#define RTE_OBJ_IMAGEM   1
#define RTE_OBJ_WORDART  2
#define RTE_AJ_LINHA     0        /* en línea con el texto */
#define RTE_AJ_QUADRADO  1        /* (reservado: el texto rodea la imagen; no está hecho) */
#define RTE_AJ_CIMA_BAIXO 2       /* arriba y abajo: la imagen ocupa su propia línea */
#define RTE_BLOB_NENHUM  0
#define RTE_BLOB_JPEG    1
#define RTE_BLOB_PNG     2
typedef struct rte_obj_dados rte_obj_dados_t;
typedef struct rte_obj {
    int  tipo;                    /* RTE_OBJ_IMAGEM o RTE_OBJ_WORDART */
    int  l_tw, a_tw;              /* tamaño de la imagen en twips (sin el pie) */
    int  ajuste;                  /* RTE_AJ_* */
    int  borde;                   /* 0 sin borde, 1 fino, 2 medio, 3 grueso */
    char pie[80];                 /* pie de imagen (Latin-1; vacío = sin pie) */
    int  wa_estilo, wa_fam, wa_est, wa_tam;   /* WordArt: estilo de la galería, fuente, N/C y cuerpo en pt */
    char wa_texto[96];
    rte_obj_dados_t *d;           /* datos compartidos (píxeles y originales) */
} rte_obj_t;
/* Una imagen: se queda con los píxeles de *img y copia "blob" (el JPEG o PNG original, o NULL). */
rte_obj_dados_t *rte_obj_dados_criar(imagem_t *img, const uint8_t *blob, int n_blob, int blob_tipo, int blob_l, int blob_a);
/* Rellena o (imagen) desde archivo o bytes: decodifica y fija el tamaño natural a 96 ppp.
 * false = formato no reconocido o sin memoria. */
bool   rte_obj_de_bytes(rte_obj_t *o, const uint8_t *dados, int n);
bool   rte_obj_de_arquivo(rte_obj_t *o, const char *caminho);
void   rte_obj_liberar(rte_obj_t *o);                 /* suelta los datos de un objeto que no se insertó */
const imagem_t *rte_obj_imagem(const rte_obj_t *o);   /* los píxeles (pueden estar reducidos) */
const uint8_t *rte_obj_blob(const rte_obj_t *o, int *n, int *tipo, int *larg, int *alt);
int    rte_obj_alto_total(const rte_obj_t *o);        /* imagen + pie, en twips */
/* En el documento. rte_obj_inserir se queda con o->d (no lo liberes). Un WordArt sin datos se dibuja aquí. */
bool   rte_obj_inserir(rte_t *r, rte_obj_t *o);
bool   rte_obj_selecionado(const rte_t *r, rte_obj_t *o, int *pos);   /* copia superficial */
bool   rte_obj_substituir(rte_t *r, const rte_obj_t *novo);           /* reemplaza el seleccionado */
const rte_obj_t *rte_obj_em(const rte_t *r, int pos);                 /* NULL si en pos no hay objeto */
void   rte_doc_obj(rte_t *r, rte_obj_t *o);                           /* carga (RTF): añade al final */
void   rte_area_util(const rte_t *r, int *l_tw, int *a_tw);           /* hueco máximo de un objeto */
bool   rte_obj_retangulo(rte_t *r, int *x, int *y, int *l, int *a);   /* del objeto seleccionado, en la ventana (px) */
bool   rte_obj_wordart_dibujar(rte_obj_t *o);                         /* (re)genera los píxeles de un WordArt */
/* Vista previa de un formato en un rectángulo (diálogo Fuente). */
void   rte_amostra(jan_t *j, int x, int y, int l, int a, const rte_fmt_t *f, const char *texto);
/* Exporta a PDF con la paginación de la pantalla; *dados se libera con libera(). 0 = bien. */
int    rte_exportar_pdf(rte_t *r, const char *titulo, uint8_t **dados, int *n);
/* Diálogos (ventanas modales): true = Aceptar. */
/* Diálogos de objetos (D296; dialogos_objeto.c): true = Aceptar. WordArt: la galería de 20 estilos con
 * texto, fuente y tamaño (si o->wa_texto está vacío es uno nuevo y se calcula su tamaño). Formato de
 * imagen: tamaño en cm y %, ajuste, alineación (0 izq, 1 centro, 2 der; la del párrafo), borde y pie. */
bool   dialogo_wordart(rte_obj_t *o);
bool   dialogo_formato_imagem(rte_obj_t *o, int *alinhamento);
bool   dialogo_fonte(rte_fmt_t *f);
bool   dialogo_familia(int *fam);       /* selector de familia con todas las fuentes, cada una con la suya (D319) */
bool   dialogo_paragrafo(rte_par_t *p);

/* ---- diálogos comunes de color y fuente, y el combo de fuentes (D327; dialogos_cf.c) ----
 * Modales, estilo Windows. Los usan Pluma, Macaco, Paint y, por sagui.dialogo(3|4), Python.
 *
 * dialogo_cor: el diálogo Color. 48 colores básicos, 16 personalizados (guardados en
 * /home/.config/colores.conf, uno por línea en hexadecimal), y el botón «Definir colores
 * personalizados >>» que despliega el espectro (tono x saturación, con cruz), la barra de luminosidad,
 * la muestra Color|Sólido, los cuadros Tono/Sat/Lum (0-240, como Windows) y Rojo/Verde/Azul
 * (0-255), sincronizados, y «Agregar a colores personalizados». *cor es 0xRRGGBB, de entrada y de
 * salida (solo cambia si se acepta). titulo NULL = «Color». */
bool   dialogo_cor(const char *titulo, uint32_t *cor);
/* Conversión HSL (0-240) <-> RGB, la de Windows, en enteros. */
void   cor_de_hsl(int h, int s, int l, uint32_t *rgb);
void   cor_para_hsl(uint32_t rgb, int *h, int *s, int *l);

/* dialogo_fonte_sistema: el diálogo Fuente completo. Lista de las fuentes del sistema con un cuadro de
 * texto encima que filtra mientras se escribe; estilos (los que tenga la familia); tamaño (lista con
 * cuadro, 6...72, o cualquier valor 1-127); efectos (tachado, subrayado y color, con dialogo_cor) y la
 * muestra «AaBbYyZz» con la fuente real. Entrada: fam (número de familia de rte/pdfw; si es < 0 se
 * busca por el nombre de familia), estilo, efectos, tam, cor. Salida: todo eso y familia (nombre). */
#define FONTE_ESTILO_NEGRITA 1     /* = RTE_NEGRITA */
#define FONTE_ESTILO_CURSIVA 2     /* = RTE_CURSIVA */
#define FONTE_EFEITO_SUBLINHADO 1
#define FONTE_EFEITO_RISCADO    2
typedef struct {
    char     familia[48];   /* nombre de la familia */
    int      fam;           /* número de familia (rte/pdfw); < 0 en la entrada = buscar por nombre */
    uint8_t  estilo;        /* FONTE_ESTILO_* */
    uint8_t  efeitos;       /* FONTE_EFEITO_* */
    int      tam;           /* puntos */
    uint32_t cor;           /* 0xRRGGBB */
} fonte_sel_t;
bool   dialogo_fonte_sistema(const char *titulo, fonte_sel_t *f);
/* Estilos que trae una familia: máscara con el bit (1 << (FONTE_ESTILO_*)). Siempre incluye el normal. */
int    fuente_estilos(int fam);
/* Número de familia por nombre (sin distinguir mayúsculas); -1 si no existe. Instala el proveedor si hace falta. */
int    fuente_familia_por_nombre(const char *nombre);

/* combo_fuentes: lista desplegable de fuentes con buscador (escribir filtra) y cada nombre dibujado con su
 * propia fuente. Pulsarlo abre la lista sobre la ventana (restaura lo de debajo al cerrarse). */
typedef struct {
    int  x, y, l, a;      /* a = 21 */
    int  fam;             /* familia mostrada y elegida */
    char texto[64];       /* su nombre */
} combo_fuentes_t;
void combo_fuentes_iniciar(combo_fuentes_t *c, int x, int y, int l);
void combo_fuentes_definir(combo_fuentes_t *c, int fam);
void combo_fuentes_desenhar(jan_t *j, combo_fuentes_t *c);
/* COMBO_SELECIONOU: eligió una familia (c->fam). Se abre con el clic izquierdo. */
int  combo_fuentes_tratar(jan_t *j, combo_fuentes_t *c, const evento_t *e);

/* ---- cinta (Ribbon) con el estilo de SaguiOS (mejora 132, D334) ----
 * Se describe con tablas de C: pestañas -> grupos -> controles. Lo que devuelve cinta_tratar es el id del
 * control elegido (como barra_ferr_tratar), CINTA_TRATADO si el evento era suyo pero no hay orden, o 0 si no
 * le interesaba (el programa lo sigue tratando). Los datos de estado (marcado, desactivado, color, valor de
 * los combos) viven en las propias tablas; cinta_buscar(c, id) da el control para leerlos.
 *  - Texto de ayuda (tip): "Título\nDescripción\nAtajo", se muestra en la fila de pestañas.
 *  - Teclado: Alt+letra de pestaña abre esa pestaña con sus KeyTips; Alt+. muestra los de las pestañas;
 *    Ctrl+F1 minimiza; con KeyTips activos, flechas izq./der. cambian de pestaña y Esc sale.
 *  - Doble clic en una pestaña minimiza la cinta; con ella minimizada, un clic en una pestaña la abre
 *    mientras se usa. Cuando cambia la altura, cinta_cambio() devuelve true: hay que recolocar el programa. */
#define CINTA_TRATADO   (-1)
#define CINTA_PEST_MAX  8
#define CINTA_GRUPOS_MAX 8
#define CINTA_TAB_A     22
#define CINTA_PANEL_A   94
enum { CC_BOTON = 1, CC_DESPLEG, CC_DIVIDIDO, CC_CONMUT, CC_EXCL, CC_COMBO, CC_FUENTES, CC_COR, CC_GALERIA };
#define CCF_GRANDE    1      /* botón grande: icono de 32 px y texto debajo */
#define CCF_SIGUE     2      /* en la misma fila que el anterior (a su derecha) */
#define CCF_SIN_TEXTO 4      /* solo el icono */
#define CCF_SALTO     8      /* empieza columna nueva */
#define CCF_EDITABLE  16     /* combo con texto editable */
typedef struct {
    int id;
    unsigned char tipo, flags;
    const char *texto;           /* "Pegar" (traducido con _() por el programa) */
    const char *tip;             /* "Título\nDescripción\nAtajo" */
    const char *kt;              /* KeyTip: 1 o 2 letras */
    void (*icone)(jan_t *j, int x, int y, bool ativo);   /* 16x16; el grande lo amplía al doble */
    const char *menu;            /* desplegable/dividido: líneas del menú (como menu_contextual) */
    const int *ids;              /* id de cada línea del menú (0 = separador) */
    const char *const *lista;    /* combo, galería */
    int n_lista, ancho;
    int excl;                    /* grupo de botones exclusivos (CC_EXCL): mismo número = mismo grupo */
    bool marcado, desativado;
    uint32_t cor;                /* CC_COR: el color actual */
    int sel;                     /* galería y combo: el índice elegido */
    combo_t cb;
    combo_fuentes_t cf;
    bool ini;
    int x, y, l, a;              /* solo lectura: posición calculada */
} cinta_ctrl_t;
typedef struct {
    const char *nome;
    cinta_ctrl_t *ctrl;
    int n;
    int lanzador;                /* id del botón de la esquina (0 = sin él) */
    int prio;                    /* menor = se reduce antes al estrechar */
    int nivel, x, l;             /* calculados: 0 completo, 1 pequeños con texto, 2 solo iconos, 3 un botón */
} cinta_grupo_t;
#define CINTA_GRUPO(nome, tabla, lanz, prio) { nome, tabla, (int)(sizeof(tabla) / sizeof((tabla)[0])), lanz, prio, 0, 0, 0 }
typedef struct {
    const char *nome, *kt;
    cinta_grupo_t *grupos;
    int n;
    bool contextual;             /* solo sale si cinta_pest_visible(c, i, true) */
    bool visible;
    int x, l;
} cinta_pest_t;
typedef struct {
    int x, y, l;
    cinta_pest_t *pest; int n_pest, sel;
    const char *archivo;         /* menú de la pestaña «Archivo» (NULL = no hay) */
    const int *archivo_ids;
    cinta_ctrl_t *acceso; int n_acceso;      /* barra de acceso rápido */
    bool minimizada, temporal;
    bool depurar;                /* registra posiciones y órdenes en el puerto serie (pruebas) */
    uint32_t acento, suave;
    cinta_ctrl_t *sobre, *apertado;
    int ap_zona, sobre_tab;
    int kt_modo;                 /* 0 no, 1 letras de pestañas, 2 letras de controles */
    char kt_buf[3];
    cinta_ctrl_t *menu_k;
    int colapso_ids[24], colapso_sel[24];
    bool cambio;
    int panel_alt;
    int arch_x, arch_l;
    int firma;                   /* para registrar el layout solo cuando cambia (depurar) */
} cinta_t;
void cinta_iniciar(cinta_t *c, int x, int y, int l, cinta_pest_t *pest, int n);
void cinta_largura(cinta_t *c, int x, int y, int l);        /* tras redimensionar */
int  cinta_altura(const cinta_t *c);
void cinta_desenhar(jan_t *j, cinta_t *c);
int  cinta_tratar(jan_t *j, cinta_t *c, const evento_t *e);
cinta_ctrl_t *cinta_buscar(cinta_t *c, int id);
void cinta_marcar(cinta_t *c, int id, bool marcado);        /* conmutable o exclusivo (apaga los demás del grupo) */
void cinta_activar(cinta_t *c, int id, bool activo);        /* false = desactivado */
void cinta_pestana(cinta_t *c, int indice);
void cinta_pest_visible(cinta_t *c, int indice, bool visible);   /* pestañas contextuales */
void cinta_minimizar(cinta_t *c, bool min);
void cinta_tam_janela(int *l, int *a);                      /* recorta el tamaño pedido a la pantalla principal */
void cinta_traducir(cinta_t *c);                            /* pasa los textos por _() */
bool cinta_cambio(cinta_t *c);                              /* la altura cambió: recolocar y redibujar */
/* Buscar/Reemplazar: devuelve DLG_BUSCAR_* (0 = cerrado). com_substituir muestra el campo y los botones de reemplazo. */
#define DLG_BUSCAR_NADA      0
#define DLG_BUSCAR_SEGUINTE  1
#define DLG_BUSCAR_SUBSTITUIR 2
#define DLG_BUSCAR_TUDO      3
int    dialogo_buscar(char *buscar, int max_b, char *por, int max_p, bool *maiusculas, bool com_substituir);
/* Cm <-> twips con texto ("1,5"): sin coma flotante. */
int    twips_de_cm_texto(const char *t);
char  *cm_texto_de_twips(char *d, int max, int twips);

/* ---- iconos de 16x16 propios para barras de herramientas (mejora 117, D293) ----
 * Tienen la forma de barra_item_t.icone. */
void ic98_novo(jan_t *j, int x, int y, bool ativo);
void ic98_abrir(jan_t *j, int x, int y, bool ativo);
void ic98_guardar(jan_t *j, int x, int y, bool ativo);
void ic98_pdf(jan_t *j, int x, int y, bool ativo);
void ic98_cortar(jan_t *j, int x, int y, bool ativo);
void ic98_copiar(jan_t *j, int x, int y, bool ativo);
void ic98_colar(jan_t *j, int x, int y, bool ativo);
void ic98_desfazer(jan_t *j, int x, int y, bool ativo);
void ic98_esq(jan_t *j, int x, int y, bool ativo);
void ic98_centro(jan_t *j, int x, int y, bool ativo);
void ic98_dir(jan_t *j, int x, int y, bool ativo);

/* ---- pestañas (mejora 117): fila de pestañas de un diálogo de varias páginas ----
 * pestanas_tratar devuelve true si cambió la pestaña activa (en "sel"). */
#define PESTANAS_MAX 8
typedef struct {
    int x, y, l;                 /* y: arriba de las pestañas; alto fijo de 22 px */
    int n, sel;
    const char *nomes[PESTANAS_MAX];
} pestanas_t;
void pestanas_desenhar(jan_t *j, pestanas_t *p);
bool pestanas_tratar(jan_t *j, pestanas_t *p, const evento_t *e);

/* ---- componentes de explorador de archivos (v8.5.5.2, D420; libsagui/gui_explorador.c) ---- */
/* abas_t: pestañas con cierre, «+» y orden arrastrable. Alto fijo ABAS_ALTURA. Los nombres los pone quien la usa. */
#define ABAS_MAX 12
#define ABAS_ALTURA 22
typedef struct {
    int x, y, l;
    int n, sel;
    char titulos[ABAS_MAX][32];
    int destaque;     /* pestaña resaltada como destino de un arrastre (-1 = ninguna) */
    int premida;      /* pestaña bajo el botón apretado (para moverla), -1 = ninguna */
    int hover;        /* D423: pestaña con el ratón encima + 1; -1 = el «+»; 0 = ninguna */
} abas_t;
enum { ABAS_NADA, ABAS_SELECIONOU, ABAS_FECHOU, ABAS_NOVA, ABAS_MOVEU };
void abas_desenhar(jan_t *j, abas_t *a);
/* Devuelve ABAS_*: SELECIONOU (nueva en *i), FECHOU (la *i), NOVA, MOVEU (de *i a *k; quien llama cambia sus datos). */
int  abas_tratar(jan_t *j, abas_t *a, const evento_t *e, int *i, int *k);
int  abas_em(const abas_t *a, int x, int y);   /* la pestaña que hay en el punto, o -1 */

/* migas_t: la ruta como migas de pan; tras cada parte hay una flecha que sirve para listar sus subcarpetas. */
#define MIGAS_MAX 16
typedef struct {
    int x, y, l, a;
    int n;
    struct { int x, l, xs; char cam[160]; char nome[40]; } p[MIGAS_MAX];
    int  hover;        /* D423: parte con el ratón encima + 1 (0 = ninguna) */
    bool hover_seta;   /* ...y si es su flecha */
    bool foco;         /* panel activo: borde de acento */
} migas_t;
void migas_desenhar(jan_t *j, migas_t *m, const char *caminho);
/* 0 = nada; 1 = una parte (en cam, su ruta); 2 = la flecha de una parte (en cam, la carpeta de cuyas subcarpetas se habla). */
int  migas_em(const migas_t *m, int x, int y, char *cam, int max);
/* La parte bajo el punto + 1 (0 = ninguna), y en *seta si es su flecha: para resaltarla con el ratón (D423). */
int  migas_hover_em(const migas_t *m, int x, int y, bool *seta);

/* colunas_t: encabezado de columnas. La columna 0 ocupa lo que sobra; las demás tienen ancho fijo
 * (larg[k]) y se ajustan arrastrando su borde izquierdo. */
#define COLUNAS_MAX 6
typedef struct {
    int x, y, l;
    int n;
    const char *titulos[COLUNAS_MAX];
    int larg[COLUNAS_MAX];
    bool direita[COLUNAS_MAX];
    int ord; bool desc;
    int arrasto, arrasto_x0, arrasto_l0;   /* columna que se está ajustando (0 = ninguna) */
} colunas_t;
enum { COLUNAS_NADA, COLUNAS_ORDENAR, COLUNAS_REDIM };
int  colunas_x(const colunas_t *c, int k);   /* x izquierda de la columna k */
void colunas_desenhar(jan_t *j, colunas_t *c);
int  colunas_tratar(jan_t *j, colunas_t *c, const evento_t *e, int *col);   /* ORDENAR (col pulsada) o REDIM (col ajustada) */
int  colunas_em(const colunas_t *c, int x, int y);   /* columna del encabezado bajo el punto, o -1 */
/* Barra de espacio usado (0-100 %), roja si queda poco. */
void barra_uso_desenhar(jan_t *j, int x, int y, int l, int a, int pct);

/* ---- componentes modernos (v8.5.5.2, D423; libsagui/gui_moderno.c) ----
 * Botones planos y con esquinas redondeadas, control segmentado, interruptor, chips, progreso, insignia,
 * campo de búsqueda, tarjeta, tooltip con retardo y toast. Todos beben de una sola paleta (mod_tema):
 * verde y turquesa de SaguiOS, superficie clara, bordes finos y radio 4. Cambiando mod_tema cambian a la vez
 * todos los componentes (un tema oscuro solo necesita otra paleta y variantes oscuras de los iconos). Las esquinas se suavizan
 * a mano (4x4 muestras por píxel de esquina) sobre lo que ya hubiera dibujado.
 * Convención: cada componente es una estructura con su posición y estado, *_desenhar y *_tratar
 * (que devuelve lo que pasó). Cada uno limpia su rectángulo con "fundo" (el color que hay detrás;
 * 0 = mod_tema.barra) antes de dibujar, así un botón plano vuelve a su aspecto sin dejar rastro del hover. */
typedef struct {
    uint32_t acento, acento_hover, acento_press, acento_suave, sobre_acento;   /* verde SaguiOS y lo que va encima */
    uint32_t foco;                          /* anillo del foco de teclado (turquesa) */
    uint32_t superficie, barra, campo;      /* tarjetas y menús / barras de órdenes / interior de los campos */
    uint32_t pista, borde, borde_forte;     /* fondo de barras y selectores / líneas finas / líneas con hover */
    uint32_t hover, press;                  /* botón plano con el ratón encima y pulsado */
    uint32_t texto, texto_fraco;
    uint32_t desab_fundo, desab_texto;
    uint32_t peligro, peligro_suave, aviso, ok;
    uint32_t dica_fundo, dica_texto;        /* tooltip y toast */
    int radio;                              /* esquinas de los botones y campos (px) */
    int radio_cartao;                       /* esquinas de las tarjetas */
    int altura;                             /* alto normal de un botón o campo */
} mod_tema_t;
extern mod_tema_t mod_tema;
void mod_tema_padrao(mod_tema_t *t);        /* la clara de SaguiOS (la de arranque) */
uint32_t mod_mezclar(uint32_t a, uint32_t b, unsigned t);   /* a*(255-t)/255 + b*t/255, por canal */

/* Primitivas con esquinas suaves: se mezclan con el píxel que ya hay. r se acota a la mitad del lado
 * (r = a/2 da una píldora; l == a con r = a/2 da un círculo). */
void g_retangulo_red(jan_t *j, int x, int y, int l, int a, int r, uint32_t cor);
void g_retangulo_red_a(jan_t *j, int x, int y, int l, int a, int r, uint32_t cor, unsigned alfa);   /* alfa 0..255 */
void g_caixa_red(jan_t *j, int x, int y, int l, int a, int r, uint32_t fundo, uint32_t borde);       /* relleno con borde de 1 px */
void g_aro_red(jan_t *j, int x, int y, int l, int a, int r, int esp, uint32_t cor);                  /* solo el contorno, de esp px */
void g_sombra_red(jan_t *j, int x, int y, int l, int a, int r, int fuerza);                          /* sombra suave (fuerza 1 o 2); ocupa 3 px alrededor */

/* Botón moderno. Variantes: primario (relleno de acento), secundario (borde), plano (sin borde hasta el
 * hover), peligro (plano con texto rojo), icono (cuadrado) e icono redondo. texto = NULL o "" -> solo icono. */
enum { MB_PRIMARIO, MB_SECUNDARIO, MB_PLANO, MB_PELIGRO, MB_ICONO, MB_ICONO_REDONDO };
typedef struct {
    int x, y, l, a;                 /* l = 0 o a = 0 -> mbotao_ajustar los calcula */
    int variante;
    const char *texto;              /* se traduce solo (idioma_traducir_sl) */
    const char *icone;              /* clave de icones.h, o NULL */
    const char *dica;               /* texto del tooltip para quien use dica_t; NULL = ninguno */
    uint32_t fundo;                 /* color que hay detrás (0 = mod_tema.barra) */
    bool ativo;                     /* conmutado: plano/icono con fondo de acento suave */
    bool desabilitado;
    bool foco;                      /* foco de teclado: anillo turquesa; Enter o espacio lo pulsan */
    bool hover, apertado;           /* los pone mbotao_tratar */
} mbotao_t;
#define MB_ICONO_L 16
void mbotao_ajustar(mbotao_t *b);                 /* a = mod_tema.altura si es 0; l según el contenido si es 0 */
void mbotao_desenhar(jan_t *j, mbotao_t *b);
bool mbotao_tratar(jan_t *j, mbotao_t *b, const evento_t *e);   /* true = pulsado (suelta dentro del botón o Enter con foco); gestiona el hover */

/* Control segmentado (vistas lista/detalles/iconos...): un solo seleccionado, resaltado con el acento. */
#define SEG_MAX 6
typedef struct {
    int x, y, l, a;                 /* l se calcula en segmentado_ajustar; a = 0 -> mod_tema.altura */
    int n, sel;
    const char *icones[SEG_MAX];    /* NULL = sin icono */
    const char *textos[SEG_MAX];    /* NULL = solo icono */
    const char *dicas[SEG_MAX];
    int larg[SEG_MAX];              /* ancho de cada segmento (lo pone segmentado_ajustar) */
    int hover;                      /* -1 = ninguno */
    uint32_t fundo;
    bool foco;
} segmentado_t;
void segmentado_ajustar(segmentado_t *s);
void segmentado_desenhar(jan_t *j, segmentado_t *s);
int  segmentado_tratar(jan_t *j, segmentado_t *s, const evento_t *e);   /* el segmento pulsado, o -1 (cambia sel) */
int  segmentado_em(const segmentado_t *s, int x, int y);                /* el segmento bajo el punto, o -1 */
void segmentado_rect(const segmentado_t *s, int k, int *x, int *y, int *l, int *a);

/* Interruptor de dos estados con su etiqueta a la derecha. Al pulsarlo la bolita se desliza (4 pasos). */
typedef struct {
    int x, y;                       /* el interruptor mide 36 x 20; la etiqueta va a su derecha */
    bool ligado, desabilitado, foco, hover;
    const char *texto;
    uint32_t fundo;
} interruptor_t;
#define INTERRUPTOR_L 36
#define INTERRUPTOR_A 20
int  interruptor_largura(const interruptor_t *s);   /* con la etiqueta */
void interruptor_desenhar(jan_t *j, interruptor_t *s);
bool interruptor_tratar(jan_t *j, interruptor_t *s, const evento_t *e);   /* true = cambió (ya dibujado, con animación) */

/* Chip o etiqueta: píldora con texto, un punto de color opcional y una «x» opcional. */
typedef struct {
    int x, y, l, a;                 /* l y a = 0 -> chip_ajustar */
    const char *texto;
    uint32_t cor;                   /* punto de color a la izquierda (0 = ninguno) */
    bool fechavel, sel, hover, hover_x;
    uint32_t fundo;
} chip_t;
enum { CHIP_NADA, CHIP_CLIQUE, CHIP_FECHAR };
void chip_ajustar(chip_t *c);
void chip_desenhar(jan_t *j, chip_t *c);
int  chip_tratar(jan_t *j, chip_t *c, const evento_t *e);

/* Barra de progreso redondeada. pct 0-100; con MP_USO cambia a ámbar y rojo cuando se llena (espacio de
 * discos). El indeterminado desliza un bloque según fase (0-99; quien llama la va subiendo). */
#define MP_USO 1
#define MOD_FUNDO_NENHUM 0xFF000000u   /* como fundo: no limpiar el rectángulo, solo mezclar las esquinas con lo que haya */
void progresso_mod_desenhar(jan_t *j, int x, int y, int l, int a, int pct, unsigned flags, uint32_t fundo);
void progresso_mod_indet(jan_t *j, int x, int y, int l, int a, int fase, uint32_t fundo);

/* Insignia (badge) con número; devuelve el ancho dibujado (alto fijo 18). Con cor = 0 usa el color de peligro. */
int  insignia_desenhar(jan_t *j, int x, int y, int n, uint32_t cor);

/* Campo de texto redondeado (el de Archivos para la ruta y la búsqueda; el campo de búsqueda con lupa y
 * botón de borrar). Edita sobre caixa_t: texto, cursor y foco son los de siempre. */
#define CAMPO_LUPA    1       /* icono de lupa a la izquierda */
#define CAMPO_LIMPAR  2       /* botón «x» a la derecha mientras haya texto */
#define CAMPO_HOVER   4
#define CAMPO_HOVER_X 8
#define CAMPO_RECTO   16      /* esquinas de radio normal en vez de píldora, y 5 px de margen (la caja de ruta de Archivos) */
void campo_desenhar(jan_t *j, caixa_t *c, int a, const char *dica, unsigned flags, uint32_t fundo);
bool campo_x_em(const caixa_t *c, int a, int x, int y);     /* el punto cae sobre su botón de borrar */
typedef struct {
    caixa_t c;                      /* c.x, c.y, c.l, y el texto */
    int a;                          /* 0 -> mod_tema.altura */
    const char *dica;               /* texto gris cuando está vacío */
    bool hover, hover_x;
    uint32_t fundo;
} campo_busca_t;
enum { BUSCA_NADA, BUSCA_MUDOU, BUSCA_ENTER, BUSCA_LIMPOU };
void campo_busca_desenhar(jan_t *j, campo_busca_t *b);
int  campo_busca_tratar(jan_t *j, campo_busca_t *b, const evento_t *e);   /* MUDOU: cambió el texto; ENTER; LIMPOU: pulsó la «x» o Esc */

/* Tarjeta con sombra suave, título y su icono. cartao_interior da el área libre bajo el título. */
typedef struct {
    int x, y, l, a;                 /* la sombra ocupa 3 px más alrededor: dejar ese margen */
    const char *titulo;             /* NULL = sin cabecera */
    const char *icone;
    bool hover, sel;
    uint32_t fundo;                 /* lo que hay detrás (para la sombra y las esquinas) */
} cartao_t;
void cartao_desenhar(jan_t *j, cartao_t *c);
void cartao_interior(const cartao_t *c, int *x, int *y, int *l, int *a);
bool cartao_tratar(jan_t *j, cartao_t *c, const evento_t *e);   /* gestiona el hover; true = clic dentro */

/* Separador vertical fino para las barras de órdenes. */
void mod_separador(jan_t *j, int x, int y, int a);

/* Tooltip con retardo (DICA_RETARDO ms). Se dibuja encima de lo que haya y al ocultarse restaura los
 * píxeles de debajo; por eso quien lo use lo oculta (dica_ocultar) antes de redibujar la zona.
 *     EV_MOVER: dica_mover(j, &d, id_del_elemento_o_-1, texto, x, y, l, a);   (cambiar de id reinicia el retardo)
 *     inactivo: dica_tic(j, &d);                                             (lo muestra cuando toca)
 *     cualquier otro evento: dica_ocultar(j, &d). */
#define DICA_RETARDO 450
#define DICA_MAX_L 320
#define DICA_ALT 24
typedef struct {
    int alvo;                       /* elemento bajo el ratón (-1 = ninguno) */
    char texto[96];
    int ex, ey, el, ea;             /* rectángulo del elemento, para colocar el tooltip */
    unsigned t0;
    bool visivel, gasta;            /* gasta: ya se enseñó (o se descartó) para este elemento */
    int rx, ry, rl, ra;             /* donde está dibujado */
    uint32_t salvo[DICA_MAX_L * DICA_ALT];
} dica_t;
void dica_iniciar(dica_t *d);
void dica_mover(jan_t *j, dica_t *d, int alvo, const char *texto, int ex, int ey, int el, int ea);
bool dica_tic(jan_t *j, dica_t *d);     /* true = acaba de aparecer */
void dica_ocultar(jan_t *j, dica_t *d);

/* Toast: aviso que aparece abajo y se va solo. */
enum { TOAST_INFO, TOAST_OK, TOAST_AVISO, TOAST_ERRO };
#define TOAST_MAX_L 400
#define TOAST_ALT 32
typedef struct {
    char texto[96];
    int tipo;
    unsigned t0, dur;
    bool visivel;
    int rx, ry, rl, ra;
    uint32_t salvo[TOAST_MAX_L * TOAST_ALT];
} toast_t;
void toast_iniciar(toast_t *t);
void toast_mostrar(jan_t *j, toast_t *t, const char *texto, int tipo, unsigned dur_ms);
bool toast_tic(jan_t *j, toast_t *t);   /* true = acaba de irse; llamarlo en los ratos libres */
void toast_ocultar(jan_t *j, toast_t *t);

/* ---- controles Win98 de diálogo, compartidos (D291/D293): botón, casilla y lista con marco ---- */
void boton98(jan_t *j, botao_t *b);
bool boton98_tratar(jan_t *j, botao_t *b, const evento_t *e);
void casilla98(jan_t *j, casilla_t *c, const char *texto);
bool casilla98_tratar(jan_t *j, casilla_t *c, const char *texto, const evento_t *e);
void lista_en_marco(jan_t *j, lista_t *l);

/* ---- detector de problemas de la GUI (D300; guía en docs/gui-revisar.md) ----
 * Ayuda para depurar, no criterio de fallo: con SAGUI_GUI_REVISAR=1 (o 2: además dibuja un borde rojo)
 * o con el fichero /etc/gui-revisar.modo, cada componente anota su rectángulo al dibujarse y, al
 * terminar un redibujo, se avisa por el registro serie de solapes entre hermanos, componentes fuera
 * de la ventana, textos que no caben y controles demasiado pequeños. Apagado, cada dibujo paga un
 * solo "if" sobre gr_modo. */
#define GRF_CONTROL 1      /* botón, casilla, caja...: se avisa si es más bajo que la fuente */
extern int gr_modo;        /* 0 apagado, 1 avisos, 2 avisos + borde rojo */
void gr_registrar(jan_t *j, const char *tipo, const void *obj, int x, int y, int l, int a,
                  const char *texto, int disp, unsigned flags);
#define GR_REG(...) do { if (__builtin_expect(gr_modo, 0)) gr_registrar(__VA_ARGS__); } while (0)
/* internos de la libsagui */
void gr_janela_aberta(jan_t *j, const char *titulo);
void gr_janela_fechada(jan_t *j);
void gr_pintado(jan_t *j, int x, int y, int l, int a);
void gr_texto_fuera(jan_t *j, int x, int y, const char *t, int n);
void gr_analizar(jan_t *j, bool completo);
void gui_revisar_ignorar(const void *componente);   /* este componente se solapa a propósito */
void gui_revisar_rect(jan_t *j, const char *tipo, int x, int y, int l, int a, const char *texto, int disp);

/* Componentes para editores (D454): paneles divisibles, inspector, selector de color, lienzo, paleta de casillas, código, deshacer, arrastrar y consola. */
#include "gui_editor.h"

#endif
