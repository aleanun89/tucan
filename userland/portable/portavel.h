// SPDX-License-Identifier: GPL-2.0-or-later
/* portavel.h - lo que la capa «libsagui sobre SDL 2» (D459) ofrece además de la ABI de sagui.h. */
#ifndef SAGUI_PORTAVEL_H
#define SAGUI_PORTAVEL_H
#include <stddef.h>

size_t portavel_latin1_para_utf8(char *dst, size_t max, const char *s);
size_t portavel_utf8_para_latin1(char *dst, size_t max, const char *s);
/* Convierte una ruta de SaguiOS (/home/..., /sistema/..., /fontes/...) a la del anfitrión. */
const char *portavel_ruta(const char *sagui, char *buf, size_t max);
/* Carpeta de los datos que acompañan al programa (fuentes, catálogos de idioma). */
const char *portavel_datos(void);
int portavel_fim_pedido(void);
/* Operaciones de SIS_IDIOMA que idioma.c pide por la capa (IDIOMA_LER / IDIOMA_FIXAR). */
int portavel_idioma(unsigned op, const void *p, unsigned n);
#endif
