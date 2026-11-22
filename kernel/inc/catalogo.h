// SPDX-License-Identifier: GPL-2.0-or-later
/* catalogo.h - catálogos de traducción (D313, mejora 118).
 *
 * El mismo catalogo.c entra en el kernel (la interfaz del escritorio) y en la
 * libsagui (los programas) y lo compila también la prueba del anfitrión. Es un
 * lector propio, sin código de GNU gettext: la cadena en español es la clave y
 * el catálogo guarda pares (clave, traducción) ordenados por un hash FNV-1a.
 *
 * Formato del .cat (todo little-endian, textos en Latin-1 con NUL final):
 *   0  "SGCT"            magia
 *   4  u32 versión (1)
 *   8  u32 n             número de entradas
 *  12  u32 reservado
 *  16  n x { u32 hash, u32 desp_clave, u32 desp_traduccion }   ordenadas por hash
 *      ...cadenas...    los desplazamientos cuentan desde el principio del archivo
 * Una traducción vacía significa «sin traducir»: se devuelve NULL. Lo genera
 * tools/extraer_textos.py a partir de los .po. */
#if !defined(SAGUI_KERNEL) && !defined(CATALOGO_ANFITRION)
#include "sagui.h"
#endif
#ifndef SAGUI_CATALOGO_H
#define SAGUI_CATALOGO_H

#if defined(SAGUI_KERNEL)
#include "tipos.h"
#elif defined(CATALOGO_ANFITRION)
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#endif

#define CATALOGO_MAGIA 0x54434753u   /* "SGCT" */

typedef struct {
    const uint8_t *d;    /* el archivo entero, en memoria */
    uint32_t tam, n;
} catalogo_t;

uint32_t catalogo_hash(const char *s);
/* Valida la cabecera y la tabla. 0 = bien, <0 = no es un catálogo. No copia: "d" debe seguir vivo. */
int catalogo_abrir(catalogo_t *c, const void *d, uint32_t tam);
/* La traducción de "clave", o NULL si no hay (o está vacía). */
const char *catalogo_buscar(const catalogo_t *c, const char *clave);

/* Normaliza el nombre de un idioma ("pt", "pt_BR.ISO-8859-1", "EN_us" -> "pt_BR", "pt_BR", "en"):
 * "es" si viene vacío. "out" de IDIOMA_MAX bytes como mínimo. */
#define IDIOMA_MAX 12
void catalogo_idioma_norm(const char *in, char *out);

#endif
