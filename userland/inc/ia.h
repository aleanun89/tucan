// SPDX-License-Identifier: GPL-2.0-or-later
/* ia.h - cliente de chat con IA, para la API compatible con la de OpenAI (D134).
 *
 * Sirve para el comando `ia` del terminal, para la aplicación de chat con
 * ventana (chatia) y para el asistente del Mico: el mismo código habla con un
 * llama.cpp server, con el Ollama, con el LM Studio o con un proveedor en
 * Internet (OpenRouter y otros), cambiando solo el endereço y la clave.
 *
 * La dirección base y el modelo vienen de la configuración (IA_URL, IA_MODELO,
 * IA_CHAVE en el entorno, o el fichero /home/.config/ia/provedor y la clave en
 * /home/.config/ia/chave). La clave nunca se escribe en el repositorio ni en la
 * imagen: solo se lee del entorno o de un fichero del disco /home.
 *
 * D145: la ventana de Configuración escribe esa misma configuración, así que
 * los tres programas (terminal, chat con ventana y Mico) la leen de aquí. Cada
 * uno puede además usar un modelo distinto (IA_USO_*), y el usuario puede
 * marcar varios modelos de los que ofrece el servidor. */
#ifndef SAGUI_IA_H
#define SAGUI_IA_H
#include "sagui.h"

#define IA_URL_MAX     256
#define IA_MODELO_MAX  64
#define IA_CHAVE_MAX   192
#define IA_SISTEMA_MAX 1024
#define IA_NOMES_MAX   64
#define IA_PAPEL_MAX   12
#define IA_MSG_MAX     4096
/* Los modelos marcados (separados por comas) caben en esta lista. */
#define IA_MODELOS_MAX 1024

/* Para qué se usa la IA. Cada programa pide el suyo con ia_config_escolher(),
 * así que con un solo servidor configurado el terminal puede hablar con un
 * modelo y el chat con ventana con otro. */
#define IA_USO_TERMINAL 0
#define IA_USO_CHATIA   1
#define IA_USO_MICO     2
#define IA_USO_QTD      3

typedef struct {
    char url[IA_URL_MAX];        /* base: http://host:porta/v1 */
    char modelo[IA_MODELO_MAX];  /* modelo por omisión */
    char chave[IA_CHAVE_MAX];    /* clave de API (vacía si no hay) */
    char sistema[IA_SISTEMA_MAX];/* mensaje de sistema (vacía si no hay) */
    /* D145: los modelos que el usuario marcó y el que usa cada programa. */
    char modelos[IA_MODELOS_MAX];                    /* marcados, separados por comas */
    char modelo_uso[IA_USO_QTD][IA_MODELO_MAX];     /* el modelo de cada uso */
    unsigned prazo_ms;           /* límite de espera de la conexión (0 = 30 s) */
    bool inseguro;               /* HTTPS sin validar el certificado (-k) */
} ia_config_t;

/* Un proveedor con nombre (los preajustes de la ventana de Configuración).
 * "modelo" es el modelo por omisión que sugiere el proveedor; el usuario
 * puede cambiarlo o elegir otro de la lista que da el servidor. */
typedef struct {
    const char *nome;            /* "OpenAI", "llama.cpp"... */
    const char *url;             /* la dirección base */
    const char *modelo;          /* el modelo por omisión */
    bool        precisa_chave;   /* si hace falta clave de API */
} ia_provedor_t;

/* Una mensaje de la conversación. "papel" es "system", "user" o "assistant";
 * el texto es Latin-1 (el cliente lo pasa a UTF-8, que es lo que la API espera). */
typedef struct {
    char papel[IA_PAPEL_MAX];
    char texto[IA_MSG_MAX];
} ia_mensagem_t;

/* Valores de omisión (sin URL ninguna). */
void ia_config_inicial(ia_config_t *c);

/* Lee la configuración: entorno primero (IA_URL, IA_MODELO, IA_CHAVE,
 * IA_SISTEMA), después el fichero /home/.config/ia/provedor y la clave de
 * /home/.config/ia/chave. Lo que esté en el entorno gana. */
void ia_config_carregar(ia_config_t *c);

/* Elige el modelo del uso pedido (IA_USO_*): si ese uso tiene un modelo
 * asignado, lo pone en c->modelo. Si el entorno trae IA_MODELO, el entorno
 * gana (es lo explícito de la sesión). */
void ia_config_escolher(ia_config_t *c, int uso);

/* Los proveedores con nombre (para los botones de la ventana de
 * Configuración). Devuelve cuántos hay; el que tiene la URL vacía es el
 * "personalizado". */
int ia_provedores(ia_provedor_t *destino, int max);

/* Dónde se guardan las cosas (los usa la ventana de Configuración). Si /home
 * es un disco persistente, en /home/.config/ia; si no, en /tmp/ia (vale solo
 * para la sesión). */
const char *ia_caminho_provedor(void);
const char *ia_caminho_chave(void);

/* Guarda la configuración: /home/.config/ia/provedor con los pares y, si
 * gravar_chave no es cero, la clave en /home/.config/ia/chave. Devuelve 0 si
 * se guardó o un error negativo. Si /home no es persistente, el aviso lo pone
 * quien llama (el programa sabe si /home es un disco). */
int ia_config_gravar(const ia_config_t *c, int gravar_chave);

/* GET /models: escribe los nombres de los modelos en "nomes" (64 bytes cada) y
 * devuelve cuántos escribe. Negativo si dio error (el mensaje va a "erro"). */
int ia_modelos(const ia_config_t *c, char (*nomes)[64], int max, char *erro, int max_erro);

/* POST /chat/completions con stream:true. Cada pedazo de texto llama
 * "ao_texto" (puede venir en varias llamadas). Al acabar, "ao_fim" dice si la
 * respuesta llegó completa (1) o si fue interrumpida (0).
 * "deve_parar" se llama entre pedazos: si devuelve 1, el cliente para de
 * leer, cierra la conexión y avisa que la respuesta quedó incompleta (y el botón
 * "Detener" del Mico y el Ctrl+C del terminal usan esto).
 * Devuelve 0 si todo corrió bien; negativo si hubo error (mensaje en "erro"). */
int ia_chat(const ia_config_t *c, const char *pergunta,
            void (*ao_texto)(const char *t, int n, void *ctx), void *ctx,
            void (*ao_fim)(int completa, void *ctx), void *ctx_fim,
            int (*deve_parar)(void *ctx), void *ctx_parar,
            char *erro, int max_erro);

/* Lo mismo, pero con la conversación entera: las mensajes van por orden (la última
 * es la pregunta de ahora) y el servidor recibe el histórico, para que el modelo
 * sepa de qué se habla. El mensaje de sistema de la configuración ("sistema")
 * va delante, si hay. El texto de cada mensaje es Latin-1 y pasa a
 * UTF-8 antes de ir en la petición. */
int ia_chat_conversa(const ia_config_t *c, const ia_mensagem_t *msgs, int nmsgs,
                     void (*ao_texto)(const char *t, int n, void *ctx), void *ctx,
                     void (*ao_fim)(int completa, void *ctx), void *ctx_fim,
                     int (*deve_parar)(void *ctx), void *ctx_parar,
                     char *erro, int max_erro);

/* ---- D239: lo que añade el chat con ventana (aditivo; lo de arriba no cambia) ---- */

/* La carpeta de la configuración de la IA ("/home/.config/ia" o, si /home no
 * es persistente, "/tmp/ia") y si es la persistente. El chat guarda sus
 * conversaciones en la subcarpeta "conversas". */
const char *ia_pasta(void);
bool        ia_pasta_persistente(void);

/* Lee solo el fichero de configuración y la clave, sin el entorno ni el
 * modelo de ningún programa: lo que hay que modificar y volver a guardar con
 * ia_config_gravar() (así una variable IA_* de la sesión no acaba en el disco). */
void ia_config_carregar_arquivo(ia_config_t *c);

/* Una mensaje que no se copia: papel y texto (Latin-1, de cualquier tamaño)
 * son del que llama. */
typedef struct {
    const char *papel;
    const char *texto;
} ia_mensagem_ref_t;

/* ia_chat_conversa() con mensajes de cualquier tamaño (un fichero adjunto,
 * una respuesta larga). La petición se arma en memoria dinámica; si pasa de
 * IA_CORPO_MAX bytes, da error ("conversa demasiado longa") sin enviar nada. */
#define IA_CORPO_MAX (512 * 1024)
int ia_chat_conversa_ref(const ia_config_t *c, const ia_mensagem_ref_t *msgs, int nmsgs,
                         void (*ao_texto)(const char *t, int n, void *ctx), void *ctx,
                         void (*ao_fim)(int completa, void *ctx), void *ctx_fim,
                         int (*deve_parar)(void *ctx), void *ctx_parar,
                         char *erro, int max_erro);

/* ---- D295: llamada a herramientas (function calling) ---- */

/* Una herramienta que el modelo pidió. «args» es el JSON de los argumentos tal como llegó (UTF-8), ya
 * completo (en streaming llega en trozos y se junta aquí). */
#define IA_CHAMADAS_MAX 8
typedef struct {
    char  id[64];
    char  nome[48];
    char *args;                  /* memoria dinámica; siempre terminada en cero */
    int   nargs, cap;
    bool  cortados;              /* pasaron de IA_ARGS_MAX: no se usan */
} ia_chamada_t;
#define IA_ARGS_MAX        (1024 * 1024)   /* tope de los argumentos de una llamada */
#define IA_TURNO_TEXTO_MAX (1024 * 1024)   /* tope del texto de un turno */

/* El resultado de un turno: el texto del asistente (UTF-8) y las herramientas pedidas. */
typedef struct {
    char        *texto;          /* memoria dinámica ("" si no habló) */
    int          ntexto, captexto;
    ia_chamada_t ch[IA_CHAMADAS_MAX];
    int          n;
    bool         completa;       /* false = cortado por «parar» o por el servidor */
    bool         sem_ferramentas;/* el servidor rechazó «tools» y se repitió el turno como chat normal */
    char         fim[24];        /* finish_reason ("stop", "tool_calls"...) */
} ia_turno_t;
void ia_turno_liberar(ia_turno_t *t);

/* Un turno: manda los mensajes (cada uno ya es un objeto JSON de la API, en UTF-8: role, content,
 * tool_calls, tool_call_id) más el mensaje de sistema de «sistema» (Latin-1, puede ser NULL) y la
 * lista «tools» (un array JSON, o NULL para chat normal). Lee la respuesta en streaming: el texto
 * llama a «ao_texto» (UTF-8) y las llamadas a herramientas, que llegan partidas en trozos, se juntan
 * en «res». Si el servidor no admite herramientas, repite el turno sin ellas (res->sem_ferramentas). */
int ia_turno(const ia_config_t *c, const char *sistema, const char *const *msgs_json, int nmsgs,
             const char *tools_json,
             void (*ao_texto)(const char *t, int n, void *ctx), void *ctx,
             int (*deve_parar)(void *ctx), void *ctx_parar,
             ia_turno_t *res, char *erro, int max_erro);

/* Procesa UNA línea «data: ...» del SSE (o un cuerpo entero sin streaming) y la suma a «res».
 * Es la parte que se prueba con trozos partidos. Devuelve 1 si era [DONE]. */
int ia_turno_linha(ia_turno_t *res, const char *json, int n,
                   void (*ao_texto)(const char *t, int n, void *ctx), void *ctx);

/* GET sencillo (HTTP o HTTPS) de una URL completa: el cuerpo (hasta «max»-1 bytes) en «dst». Devuelve los bytes o <0. */
int ia_http_obter(const char *url, char *dst, int max, char *erro, int max_erro);

/* Utilidades de codificación para quien arma mensajes JSON: Latin-1 <-> UTF-8 y cadena JSON. */
int ia_json_latin1(char *dst, const char *s);               /* "\"texto\"" desde Latin-1; dst debe tener 6*n+3 */
int ia_utf8_latin1(const char *s, int n, char *dst, int max);

#endif
