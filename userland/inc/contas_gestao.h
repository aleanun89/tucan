// SPDX-License-Identifier: GPL-2.0-or-later
/* contas_gestao.h - crear, borrar y renombrar cuentas; el grupo de administradores (D340, mejora 098,
 * fase 3). Implementación en libsagui/contas.c. No incluye contas.h para que lo pueda usar quien tiene
 * su propio conf_gravar (Configuración).
 *
 * Todo lo que cambia algo es solo para root (los programas con ventana piden antes el diálogo de
 * permisos, D269). Las cuentas nuevas son de persona: uid desde 1001 (sin reutilizar uno que aún
 * sea dueño de una carpeta de /home), su propio grupo, carpeta /home/<nombre> (0700, suya) y el
 * Terminal. root (0), sagui (1000) y las cuentas de servicio no se borran ni se renombran.
 *
 * /etc/grupos, como /etc/group: «nombre:gid:miembro,miembro». Solo se usa la línea «admin»: sus
 * miembros autorizan la elevación a root con su propia contraseña (además de la de root). Sin el
 * archivo, nadie (como antes: solo la contraseña de root). Lo pueden leer todos. */
#ifndef SAGUI_CONTAS_GESTAO_H
#define SAGUI_CONTAS_GESTAO_H
#include "sagui.h"

#define GRUPO_ADMIN     "admin"
#define GID_ADMIN       10
#define CONTA_UID_PRIMEIRO 1001   /* las cuentas que se crean */
#define CONTA_UID_ULTIMO   59999

/* Las cuentas de persona (uid de 1000 en adelante y con shell), para listarlas. */
typedef struct {
    char     nome[32];
    unsigned uid;
    char     descricao[64];
    bool     admin;
} conta_resumo_t;
int  contas_resumo(conta_resumo_t *v, int max);

bool conta_nome_valido(const char *nome);            /* [a-z0-9_-], 1 a 31 */
bool conta_admin(const char *nome);                   /* ¿está en el grupo admin? */
int  conta_admin_fixar(const char *nome, bool sim);   /* root */
/* ¿Guarda /home dueños y permisos? (no en FAT32): sin ellos no se crean cuentas nuevas. */
bool conta_home_com_donos(void);
/* root. senha "" = sin contraseña (entra sin preguntar). uid_saida puede ser NULL. */
int  conta_criar(const char *nome, const char *descricao, const char *senha, bool admin, unsigned *uid_saida);
/* root. con_arquivos: borra también /home/<nombre> (solo si es de esa cuenta). */
int  conta_apagar(const char *nome, bool con_arquivos);
int  conta_renomear(const char *velho, const char *novo);
/* root: la contraseña de otra cuenta ("" = quitarla). */
int  conta_senha(const char *nome, const char *senha);

#endif
