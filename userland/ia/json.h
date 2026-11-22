// SPDX-License-Identifier: GPL-2.0-or-later
/* json.h - lector de JSON pequeño para los agentes de IA (D295).
 *
 * Analiza un texto JSON completo en una tabla plana de «tokens» (sin memoria dinámica ni
 * recursión sin límite) y deja consultar objetos, listas y cadenas con escapes. Sirve para
 * las respuestas de la API (streaming y no) y para los argumentos de las herramientas, que
 * pueden ir anidados y con escapes (\", \\, \n, \uXXXX con pares sustitutos).
 *
 * Es independiente del sistema: con -DAG_ANFITRION se compila en el PC (tests/json_unidad.c). */
#ifndef SAGUI_IA_JSON_H
#define SAGUI_IA_JSON_H
#ifdef AG_ANFITRION
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#else
#include "sagui.h"
#endif

enum { JSON_OBJ = 1, JSON_ARR, JSON_STR, JSON_NUM, JSON_LIT };

/* Un token. «ini» y «fim» delimitan el texto crudo en el original: para una cadena, sin las
 * comillas (con los escapes aún dentro); para un objeto o lista, de la llave a su cierre.
 * «n» = hijos directos (un objeto cuenta pares: la clave es un token STR y el valor, el
 * siguiente). Los hijos van detrás del padre, en orden (como en jsmn). */
typedef struct { unsigned char tipo; int ini, fim, n; } json_tok_t;

#define JSON_PROFUNDIDAD_MAX 24

/* Analiza «t» (n bytes). Devuelve cuántos tokens escribió (>= 1), o un negativo: -1 sintaxis,
 * -2 no caben en «max», -3 demasiado anidado. El texto debe tener un solo valor (detrás solo
 * se admiten espacios). */
int json_analisar(const char *t, int n, json_tok_t *tk, int max);

/* El índice del token que sigue al subárbol de «i». */
int json_depois(const json_tok_t *tk, int i);
/* El valor del campo «nome» del objeto «i» (índice del token valor), o -1. */
int json_campo(const char *t, const json_tok_t *tk, int i, const char *nome);
/* El k-ésimo elemento de la lista «i» (índice), o -1. */
int json_elem(const json_tok_t *tk, int i, int k);
/* Cadena: escribe en «dst» el valor sin escapes, en UTF-8, siempre terminado en cero; devuelve
 * los bytes escritos (si no cabe, corta en un carácter entero) o -1 si el token no es una cadena. */
int json_texto(const char *t, const json_tok_t *tk, int i, char *dst, int max);
/* Número entero (con signo); «por_omision» si no es un número. */
int json_int(const char *t, const json_tok_t *tk, int i, int por_omision);
/* ¿El literal es «true»? */
bool json_verdadero(const char *t, const json_tok_t *tk, int i);
/* Largo del texto crudo de un token. */
static inline int json_largo(const json_tok_t *tk, int i) { return tk[i].fim - tk[i].ini; }

#endif
