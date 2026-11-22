// SPDX-License-Identifier: GPL-2.0-or-later
/* conf_maquina.h - la configuración de la máquina, separada de la de la persona (D371, fase 1).
 *
 * Con SAGUI-CONF, lo que es de la máquina (aceleración, caché del disco, sonido, mando, teclado del sistema,
 * zona, frecuencia, pantallas, bloqueo DNS, ajustes del Wi-Fi) vive en /etc (en SAGUI-CONF) y solo lo cambia
 * root o un programa elevado con el diálogo de permisos. Antes estaba en la carpeta de sagui
 * (/home/.config/sagui.conf, /home/.zona...), así que sagui lo cambiaba sin ser administrador y las demás
 * cuentas no lo podían cambiar.
 *
 * /etc/sistema.conf hace además de marca: si existe, la separación está hecha (migrada). Sin SAGUI-CONF («en
 * vivo») no hay nada que separar: todo sigue en /home como antes. */
#ifndef SAGUI_CONF_MAQUINA_H
#define SAGUI_CONF_MAQUINA_H
/* Incluir después de sagui.h. */

#define MAQ_CONF        "sistema.conf"          /* /etc/sistema.conf: claves de la máquina (clave=valor) */
#define MAQ_ARQ_CONF    "/etc/sistema.conf"
#define MAQ_BLOQ_TABLA  "/sistema/conf/bloqueo/tabla.bin"   /* la tabla del bloqueo DNS (puede pesar MB: fuera de etc/) */

/* ¿La máquina tiene su configuración en /etc (SAGUI-CONF con /etc/sistema.conf)? */
bool maquina_separada(void);
/* ¿"clave" (de sagui.conf) es de la máquina? El teclado lo es y también de la persona (la de la persona manda). */
bool maquina_clave(const char *clave);
/* El valor de "clave" en /etc/sistema.conf; 0 = no está, 1 = está. */
int  maquina_ler(const char *clave, char *valor, int max);
/* Pone en /etc/sistema.conf las líneas "clave=valor\n" de "linhas" (solo claves de la máquina; "clave=" vacía la
 * quita), conservando las demás. Si no cambia nada no escribe. Sin ser root pide el diálogo de permisos (motivo) y lo suelta al acabar.
 * 0 = guardado o sin cambios; ERRO_ACESSO si no se concedió. */
int  maquina_gravar_linhas(const char *linhas, const char *motivo);
/* Escribe un archivo de la máquina (/etc/rel en SAGUI-CONF), pidiendo permisos si hace falta. */
int  maquina_gravar_arquivo(const char *rel, const void *dados, int n, const char *motivo);
/* La migración (root, al arrancar): pasa a /etc lo que había en /home y crea /etc/sistema.conf. No borra nada
 * de /home (una versión anterior lo seguiría leyendo). 1 = migrada ahora; 0 = no hacía falta; < 0 = error. */
int  maquina_migrar(void);
/* D371 (fase 4): la cuenta principal (sagui) pasa de /home entero a /home/sagui, una vez y reanudable
 * (casa_migrar.c). 1 = migrada ahora; 0 = no hacía falta o no se puede (en vivo, /home en la RAM o FAT32);
 * < 0 = error (se reintenta en el siguiente arranque). */
int  casa_migrar_sagui(void);
int  casa_pasta_modelos(void);   /* D380: /home/compartido/modelos de root con 1777 */
/* La carpeta real de una cuenta (de /etc/usuarios; "/home" o "/root" si no tiene). false = no está la cuenta. */
bool casa_real_de_uid(unsigned uid, char *casa, int max);
#endif
