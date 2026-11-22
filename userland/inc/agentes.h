// SPDX-License-Identifier: GPL-2.0-or-later
/* agentes.h - agentes de IA con herramientas (D295, mejora 121).
 *
 * Capas, de abajo arriba:
 *   json.h              lector de JSON.
 *   agente_politica.c   LA SEGURIDAD (sin llamadas al sistema: se prueba en el PC): rutas prohibidas,
 *                       rechazo de «admin», clasificación de herramientas, decisión de confirmar.
 *   herramientas.c      las herramientas del sistema (leer, listar, buscar, info, órdenes, escribir,
 *                       abrir, web, correo) y el REGISTRO donde cada aplicación añade las suyas.
 *   agente.c            definiciones de agente (ficheros .agente), registro y el BUCLE del agente.
 *   agente_panel.c      el panel gráfico reutilizable (conversación, pasos, confirmaciones).
 *
 * Para integrar un agente en una aplicación: docs/agentes-ia.md, «Integrar agentes en una aplicación». */
#ifndef SAGUI_AGENTES_H
#define SAGUI_AGENTES_H
#ifdef AG_ANFITRION
#include "../ia/json.h"
#else
#include "sagui.h"
#include "ia.h"
#include "contas.h"
#include "../ia/json.h"
#endif

/* ------------------------------------------------------------------ política */

/* Clases de herramienta. LEER: solo lee (sin preguntar); su resultado es contenido externo (dato,
 * no órdenes) y «mancha» la sesión. INFO: solo lee y el resultado lo genera el sistema (no mancha).
 * ESCRIBIR, EJECUTAR y RED: piden confirmación en cada acción. */
enum { AG_CL_INFO = 0, AG_CL_LEER, AG_CL_ESCRIBIR, AG_CL_EJECUTAR, AG_CL_RED };

/* Respuestas de una confirmación. */
enum { AG_NEGAR = 0, AG_UNA_VEZ = 1, AG_SIEMPRE = 2 };

/* Ruta absoluta y limpia (sin «.», «..» ni «//»). «cwd» para las relativas. 0 si ok. */
int  ag_ruta_normalizar(const char *ruta, const char *cwd, char *dst, int max);
/* ¿Prohibida para leer aunque el usuario pueda leerla? (sombra, clave del sshd, .env, clave de la IA...) */
bool ag_ruta_prohibida_leer(const char *ruta_normal);
/* ¿Permitida para escribir? Solo /home y /tmp, salvo las carpetas de configuración sensibles. */
bool ag_ruta_permitida_escribir(const char *ruta_normal);
/* ¿La orden de shell pide ser administrador (admin, sudo, su, elevar...) o lanza otro agente? Si sí,
 * escribe en «palabra» cuál (para el mensaje). Mejor un falso positivo que dejar pasar uno. */
bool ag_orden_pide_admin(const char *orden, char *palabra, int max);
/* ¿La orden menciona una ruta prohibida (mejor esfuerzo: un shell se puede ofuscar; lo último que
 * decide es la persona al ver la orden exacta)? */
bool ag_orden_toca_prohibido(const char *orden, char *cual, int max);
/* ¿La orden parece destructiva (rm, dd, mkfs, formatear...)? Solo para el aviso rojo del diálogo. */
bool ag_orden_destructiva(const char *orden);

/* El estado de seguridad de una ejecución del agente. */
typedef struct {
    bool     manchada;                 /* ya entró contenido externo (fichero, web, correo) en la conversación */
    char     siempre[16][32];          /* herramientas con «Siempre en esta sesión» */
    int      n_siempre;
} ag_sesion_t;

/* ¿Hay que preguntar? clase y nombre de la herramienta. Las de solo lectura nunca; las demás siempre,
 * salvo «Siempre» concedido Y la sesión sin manchar (con contenido externo, se pregunta siempre).
 * La red (AG_CL_RED: buscar_web) nunca admite «Siempre»: es el canal por el que saldrían datos.
 * «puede_siempre» dice si el diálogo debe ofrecer «Siempre». */
bool ag_debe_preguntar(const ag_sesion_t *s, int clase, const char *nome, bool *puede_siempre);
void ag_conceder_siempre(ag_sesion_t *s, const char *nome);

/* ---- Revisión de seguridad (D295): lo que ve la persona es lo que se hace ---- */

/* Texto para un diálogo o un terminal: los caracteres de control (ESC, CR, BS, NUL...), DEL y los
 * C1 de Latin-1 (0x80-0x9F) pasan a «?» (no pueden borrar ni tapar parte de lo que se muestra), el
 * tabulador a espacio, CR LF a LF, y las líneas de más de «cols» columnas se parten (con «  ... »
 * delante de la continuación) para que nada quede fuera de la vista. Siempre termina en cero; si
 * no cabe entero, lo dice al final y en v->cortado. Devuelve los bytes escritos. */
typedef struct {
    int  lineas;                       /* líneas que ocupa, ya partidas */
    int  controles;                    /* caracteres sustituidos por «?» */
    int  largo_max;                    /* la línea original más larga */
    bool cortado;                      /* no cupo entero en «dst» */
} ag_vista_t;
#define AG_VISTA_COLS 68               /* cabe en el diálogo gráfico (620 px) y en un terminal de 80 */
int  ag_texto_visible(const char *s, int n, char *dst, int max, int cols, ag_vista_t *v);

/* Una orden de ejecutar_orden: una sola línea de caracteres visibles. 0 si vale; <0 con el motivo. */
#define AG_ORDEN_MAX 480
int  ag_orden_validar(const char *orden, char *motivo, int max);
/* Avisos para la persona sobre la forma de la orden (encadena órdenes, sustituciones, variables,
 * redirecciones, largo). Escribe en «dst» (vacío si nada) y devuelve los bytes. */
int  ag_orden_avisos(const char *orden, char *dst, int max);

/* Marca un resultado como dato externo con un código propio de esta llamada: «[DATOS EXTERNOS de X,
 * código C: ...]» ... «[FIN DE DATOS EXTERNOS C]». Dentro, cualquier «DATOS EXTERNOS» (sin distinguir
 * mayúsculas ni el separador) pasa a «DATOS_EXTERNOS»: el contenido no puede abrir ni cerrar un
 * bloque. Devuelve los bytes escritos en «dst» (que debe tener n + 400). */
int  ag_marcar_externo(const char *herr, const char *res, int n, const char *codigo, char *dst, int cap);

/* ¿Es una variable de entorno con un secreto (clave de la IA, tokens, contraseñas)? «entrada» es
 * «NOMBRE=valor». Las órdenes del agente nacen sin ellas. */
bool ag_env_secreta(const char *entrada);

/* ¿Puede abrir_aplicacion lanzar este programa de /bin? Solo aplicaciones con ventana que abren
 * documentos; nunca shells, intérpretes, compiladores, terminales ni lo que toca el sistema. */
bool ag_app_abrible(const char *nome);

#ifndef AG_ANFITRION   /* lo que sigue necesita el sistema; la política de arriba se prueba en el PC */
/* ------------------------------------------------------------------ herramientas */

#define AG_ARGS_TOKENS 96
typedef struct {
    char        *t;                    /* el JSON de los argumentos (UTF-8) */
    int          n;
    json_tok_t   tk[AG_ARGS_TOKENS];
    int          ntk;
} ag_args_t;

/* Los argumentos se leen de aquí ya en Latin-1 (lo que usa el sistema). 0 si falta o no es texto. */
int ag_arg_texto(const ag_args_t *a, const char *nome, char *dst, int max);   /* -1 falta; -2 no cabe o lleva NUL */
int ag_arg_int(const ag_args_t *a, const char *nome, int por_omision);
/* Texto opcional (vacío si falta): 0, o <0 con el motivo en «erro» si no cabe en «dst» (nunca se recorta en silencio). D298. */
static inline int ag_arg_opc(const ag_args_t *a, const char *nome, char *dst, int max, char *erro, int me)
{
    if (ag_arg_texto(a, nome, dst, max) == -2) {
        formatar_texto(erro, (size_t)me, "el argumento «%s» es demasiado largo o no es valido", nome);
        return -1;
    }
    return 0;
}
/* FNV-1a para las huellas de lo que se enseñó en la confirmación (c->huella). */
static inline unsigned ag_hash(unsigned h, const void *p, int n)
{
    const unsigned char *s = p;
    for (int i = 0; i < n; i++) { h ^= s[i]; h *= 16777619u; }
    return h;
}
/* Booleano: 1 = true (o «true»/«si»/«sí» como cadena), 0 = false, -1 = ausente. Para las herramientas de las aplicaciones (D298). */
static inline int ag_arg_bool(const ag_args_t *a, const char *nome)
{
    if (a->ntk < 1) return -1;
    int i = json_campo(a->t, a->tk, 0, nome);
    if (i < 0) return -1;
    if (a->tk[i].tipo == JSON_STR) {
        char t[8];
        json_texto(a->t, a->tk, i, t, sizeof t);
        return (t[0] == 't' || t[0] == 'T' || t[0] == 's' || t[0] == 'S' || t[0] == '1') ? 1 : 0;
    }
    return json_verdadero(a->t, a->tk, i) ? 1 : 0;
}
/* Lo mismo con memoria dinámica y sin límite (contenido de ficheros): libera(); *n = bytes. NULL si falta. */
char *ag_arg_dyn(const ag_args_t *a, const char *nome, int *n);
/* Analiza el JSON de los argumentos (texto UTF-8 que debe seguir vivo). 0 o <0 (JSON no válido). */
int ag_args_analisar(ag_args_t *a, char *texto, int n);
/* Ejecuta un programa con la salida recogida (entrada vacía, tope de tiempo; SAGUI_AGENTE=1 en el entorno).
 * Devuelve los bytes de salida o <0; *codigo = código de salida (o -1 si se mató por tiempo). */
int ag_capturar(const char *programa, char *const argv[], int prazo_ms, char *saida, int max, int *codigo);
/* Lanza un programa por cuenta del agente: entorno sin secretos (ag_env_secreta) más SAGUI_AGENTE=1 y, en
 * una sesión de administrador, como «sagui» (USU_COMO): nada de lo que lanza un agente es root. pid o <0. */
int ag_lanzar(const char *programa, char *const argv[]);

/* Contexto que reciben las herramientas. */
typedef struct {
    unsigned uid;
    char     cwd[128];
    int      max_salida;               /* tope de bytes del resultado */
    void    *app;                      /* dato de la aplicación (herramientas registradas) */
    /* Revisión de seguridad: «preparar» anota aquí la huella de lo que enseñó (p. ej. el fichero que
     * se va a pisar) y «executar» comprueba que no cambió mientras la persona decidía. */
    unsigned huella;
    bool     con_huella;
} ag_ctx_t;

typedef struct ag_herr {
    const char *nome;                  /* identificador (a-z, 0-9, _) */
    const char *descricao;             /* para el modelo (UTF-8 o ASCII) */
    const char *parametros;            /* JSON Schema de los argumentos (objeto) */
    int         clase;                 /* AG_CL_* */
    /* Valida los argumentos y escribe en «desc» lo que se le enseña a la persona (la orden exacta, el
     * diff...). Devuelve 0 si se puede intentar; <0 con el motivo en «erro» (ruta prohibida, admin...). */
    int (*preparar)(ag_ctx_t *c, const ag_args_t *a, char *desc, int max_desc, char *erro, int max_erro);
    /* Hace el trabajo y escribe el resultado (Latin-1) en «saida». Devuelve los bytes o <0. Solo se
     * llama si «preparar» dio 0 y, si hacía falta, la persona lo permitió. */
    int (*executar)(ag_ctx_t *c, const ag_args_t *a, char *saida, int max_saida);
} ag_herr_t;

/* Las herramientas de serie y las que registran las aplicaciones. */
int  ag_herr_registrar(const ag_herr_t *h);        /* copia el descriptor; 0 o <0 (llena, nombre repetido) */
void ag_herr_desregistrar(const char *nome);
const ag_herr_t *ag_herr_buscar(const char *nome);
int  ag_herr_contar(void);
const ag_herr_t *ag_herr_n(int i);
/* La lista «tools» de la API para las herramientas permitidas («permitidas»: nombres separados por coma;
 * vacío = todas). Devuelve memoria dinámica (libera) o NULL. UTF-8. */
char *ag_herr_json(const char *permitidas);
bool  ag_herr_permitida(const char *lista, const char *nome);   /* ¿está en la lista de nombres separados por coma? (vacía = todas) */

/* ------------------------------------------------------------------ agentes */

#define AG_NOME_MAX 48
typedef struct {
    char nome[AG_NOME_MAX];
    char descricao[160];
    char modelo[IA_MODELO_MAX];        /* vacío = el del uso */
    char herramientas[512];            /* separadas por coma; vacío = todas las del sistema */
    int  passos;                       /* límite de pasos (rondas de herramientas) */
    char sistema[1536];                /* instrucciones */
    char arquivo[96];                  /* de dónde se leyó */
    bool de_serie;                     /* viene de /etc/agentes (no se edita, se copia) */
} ag_def_t;

#define AG_PASSOS_PADRAO 8
#define AG_PASSOS_MAX    24
#define AG_DIR_SERIE   "/etc/agentes"
#define AG_DIR_USUARIO "/home/.config/agentes"
int  ag_def_ler(const char *caminho, ag_def_t *d);                   /* 0 o <0 */
int  ag_def_gravar(const char *caminho, const ag_def_t *d);
int  ag_def_listar(ag_def_t *v, int max);                            /* de serie y del usuario (este gana) */
int  ag_def_buscar(const char *nome, ag_def_t *d);                   /* 0 o <0 */
/* La carpeta del usuario y el registro. */
const char *ag_pasta(void);
const char *ag_caminho_registro(void);
void ag_registrar(const char *agente, const char *fmt, ...);        /* una línea en el registro */

/* ------------------------------------------------------------------ bucle */

/* Lo que la interfaz hace por el agente. Todas opcionales salvo «confirmar» (sin ella se deniega todo
 * lo que exige confirmación). */
typedef struct {
    void (*texto)(const char *t, int n, void *ctx);                  /* respuesta del modelo (UTF-8, en streaming) */
    void (*passo)(int num, const char *herramienta, const char *args, void *ctx);    /* va a usar una herramienta (Latin-1) */
    void (*resultado)(int num, const char *herramienta, const char *res, int estado, void *ctx);  /* estado: 0 ok, 1 error, 2 denegada */
    /* Pide permiso. «desc» es la orden exacta o el diff; «aviso» puede estar vacío (contenido externo
     * leído, orden destructiva, sesión de administrador). Devuelve AG_NEGAR, AG_UNA_VEZ o AG_SIEMPRE
     * (solo si «puede_siempre»). */
    int  (*confirmar)(const char *herramienta, const char *desc, const char *aviso, bool puede_siempre, void *ctx);
    int  (*parar)(void *ctx);                                        /* 1 = cancelar */
    void *ctx;
} ag_ui_t;

#define AG_TIEMPO_PADRAO_MS (5 * 60 * 1000)
#define AG_SALIDA_PADRAO    8192
typedef struct {
    int      passos_usados;
    int      herramientas_usadas;
    int      denegadas;
    bool     sin_herramientas;         /* el modelo no admite herramientas: se contestó como chat */
    bool     limite_passos;
    bool     limite_tiempo;
    bool     cancelado;
} ag_resultado_t;

/* Ejecuta el agente con un mensaje del usuario. «historia» (puede ser NULL, n=0) son mensajes anteriores
 * como JSON de la API (los que devolvió esta misma función en «hist_io»), para conversar: si «hist_io»
 * no es NULL, se añaden a la conversación los mensajes nuevos (user, assistant, tool) para el turno siguiente.
 * Devuelve 0, o <0 con el mensaje en «erro». Rechaza correr si el proceso es de un servicio. */
typedef struct { char **v; int n, cap; } ag_hist_t;
void ag_hist_liberar(ag_hist_t *h);
int  ag_correr(const ia_config_t *cfg, const ag_def_t *def, ag_hist_t *hist, ag_sesion_t *sesion,
               const char *tarea, const ag_ui_t *ui, void *app, ag_resultado_t *res, char *erro, int max_erro);
/* ¿Puede este proceso usar agentes? (no un servicio; no dentro de otro agente). */
bool ag_permitido(char *motivo, int max);
/* Quién está detrás (para los diálogos): «sagui (1000)» o aviso si es root. */
bool ag_es_root(void);

/* ------------------------------------------------------------------ panel gráfico */
/* agente_panel.c (se enlaza solo en programas con ventana: IA_GUI_OBJ). */
struct jan;
typedef struct ag_panel ag_panel_t;
/* Crea un panel en el rectángulo (x, y, l, a) de la ventana «j». «app» se pasa a las herramientas
 * registradas. */
ag_panel_t *ag_panel_criar(void *j, int x, int y, int l, int a, void *app);
void        ag_panel_liberar(ag_panel_t *p);
void        ag_panel_mudar_tamanho(ag_panel_t *p, int x, int y, int l, int a);
void        ag_panel_desenhar(ag_panel_t *p);
/* Procesa un evento; true si el panel lo consumió. Mientras el agente trabaja, el panel atiende
 * los eventos de la ventana él mismo (Detener, plegar pasos, confirmaciones). */
bool        ag_panel_tratar(ag_panel_t *p, const void *evento);
void        ag_panel_escolher(ag_panel_t *p, const char *nome_agente);
/* Por omisión el panel lee la configuración de la IA (ia_config_carregar + el uso Chat IA); una aplicación
 * con su propia línea de órdenes (--url, --modelo) se la pasa aquí. */
void        ag_panel_config(ag_panel_t *p, const ia_config_t *cfg);
/* Diálogo modal de confirmación (también usable suelto por las aplicaciones). */
int         ag_dialogo_confirmar(const char *herramienta, const char *desc, const char *aviso, bool puede_siempre);

/* Ayudas para integrar el panel en una aplicación (D298). */
/* Quita de argv las opciones --url y --modelo (para pruebas) y las aplica a todo panel que se cree después. */
void        ag_app_opciones(int *argc, char **argv);
/* Limita los agentes que ofrece el panel (NULL = todos). Útil cuando el contenido que se maneja es hostil
 * (correo): solo se ofrecen agentes sin herramientas peligrosas. */
void        ag_panel_filtro(ag_panel_t *p, bool (*ok)(const ag_def_t *d));
/* Lanza el agente «agente» (NULL = el elegido) con una tarea predefinida, como una acción rápida de menú.
 * false si el panel está ocupado o no hay agentes. */
bool        ag_panel_tarea(ag_panel_t *p, const char *agente, const char *tarea);
bool        ag_panel_ocupado(const ag_panel_t *p);

#endif /* AG_ANFITRION */

#endif
