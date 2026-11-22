// SPDX-License-Identifier: GPL-2.0-or-later
/* difer.h - diferencia por líneas entre dos textos (D240).
 *
 * Algoritmo de Myers (la secuencia común más larga de líneas, en O(N*D)). Sirve al
 * panel «Asistente» del Mico para enseñar lo que cambia una propuesta de la IA, y
 * es reutilizable: no sabe nada del Mico ni de la pantalla. */
#ifndef SAGUI_DIFER_H
#define SAGUI_DIFER_H

/* Una línea: puntero y largo (sin el '\n'). */
typedef struct { const char *p; int n; } difer_linha_t;

enum { DIFER_IGUAL = 0, DIFER_QUITA = 1, DIFER_ANADE = 2 };

/* Una operación del guion de edición, en orden:
 *   IGUAL: la línea ia de A es la línea ib de B;
 *   QUITA: la línea ia de A desaparece (ib = línea de B donde estaría);
 *   ANADE: la línea ib de B aparece (ia = línea de A antes de la que se inserta). */
typedef struct { unsigned char tipo; int ia, ib; } difer_op_t;

/* Calcula el guion. Devuelve el número de operaciones (>= 0) o -1 si no cabe en max
 * o si las diferencias son demasiadas (más de DIFER_D_MAX): en ese caso quien llama
 * puede tratar todo como «quitar A y añadir B». */
#define DIFER_D_MAX 1500
int difer_linhas(const difer_linha_t *a, int na, const difer_linha_t *b, int nb,
                 difer_op_t *ops, int max_ops);

/* Parte un texto en líneas (el '\n' final no crea una línea vacía). Devuelve cuántas
 * (como mucho max). */
int difer_partir(const char *s, int n, difer_linha_t *linhas, int max);

#endif
