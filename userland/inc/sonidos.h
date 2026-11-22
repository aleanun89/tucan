// SPDX-License-Identifier: GPL-3.0-only
/* sonidos.h - sonidos del sistema (D421): esquemas, eventos y la configuración.
 *
 * Los esquemas son carpetas con un esquema.conf (nombre, créditos y «evento=archivo»):
 * /sistema/sonidos/<id> (los que van en toda imagen: «saguios», generado por código, y «kenney», CC0)
 * y /apps/sonidos/<id> (los privados de quien compila con INPUT_SHARE; nunca en git ni en las imágenes públicas).
 * La configuración es de la persona: /home/.config/sonidos.conf. Todo está apagado por defecto.
 * Incluir después de sagui.h. */
#ifndef SAGUI_SONIDOS_H
#define SAGUI_SONIDOS_H

#define SON_EV_N 10
#define SON_ARQ_CONF  "/home/.config/sonidos.conf"
#define SON_DIR_SIS   "/sistema/sonidos"
#define SON_DIR_PRIV  "/apps/sonidos"
#define SON_ARRANQUE_MAX_MS 8000   /* tope de un sonido que suena sin esperar */
#define SON_APAGADO_MAX_MS  3000   /* tope del sonido de apagado/reinicio: el apagado no espera más */

typedef struct {
    int           activo;        /* 0 = nada suena (por defecto) */
    int           volumen;       /* 0-100, de los sonidos del sistema (por defecto 70) */
    char          esquema[32];   /* id de la carpeta; "" o "ninguno" = ninguno */
    unsigned char ev[SON_EV_N];  /* 1 = ese evento suena */
} sonidos_conf_t;

typedef struct {
    char id[32];
    char nombre[40];
    char creditos[96];
    bool privado;                /* de /apps/sonidos (imagen privada) */
} sonidos_esquema_t;

const char *sonidos_evento_id(int i);                /* "arranque", "apagar", "reiniciar", "aviso", "error", "pregunta", */
int         sonidos_evento_indice(const char *id);   /* "usb_conectar", "usb_desconectar", "bateria", "papelera"; -1 = no existe */
int         sonidos_evento_defecto(int i);           /* 1 = suena de serie al activar los sonidos (arranque y apagado: no) */
void sonidos_conf_defecto(sonidos_conf_t *c);
int  sonidos_conf_ler(sonidos_conf_t *c);            /* 0 = leída; 1 = no había (valores de serie) */
int  sonidos_conf_gravar(const sonidos_conf_t *c);   /* 0 o < 0 */
int  sonidos_esquemas(sonidos_esquema_t *v, int max);   /* los instalados (sin «Ninguno»); -> cuántos */
int  sonidos_ruta(const char *esquema, const char *evento, char *dst, int max);   /* 0 = archivo del evento; < 0 = no lo tiene */
/* Lanza /bin/sonidos para el evento si está activado (sin esperar): devuelve el pid, 0 si no suena. */
int  sonido_evento(const char *evento);
/* Igual, pero espera a que acabe hasta max_ms (y lo mata si se pasa): para el apagado y el reinicio. */
int  sonido_evento_esperar(const char *evento, unsigned max_ms);
#endif
