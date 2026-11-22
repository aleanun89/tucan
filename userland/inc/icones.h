// SPDX-License-Identifier: GPL-2.0-or-later
/* icones.h - biblioteca de iconos del SaguiOS (D135).
 *
 * Los iconos son dibujos de 16x16 píxeles en pixel art: cada línea tiene 16
 * caracteres y cada carácter es un tono de la paleta compartida (icones_cor).
 * '.' es transparente.
 *
 * La biblioteca la genera tools/gerar-icones.py a partir de los "Fluent UI
 * System Icons" de Microsoft (licencia MIT); el aviso de copyright y el texto
 * de la licencia están en userland/terceros/fluentui/ (vea docs/terceros.md).
 *
 * Uso desde un programa:
 *     icone_desenhar(j, 10, 10, "pasta", 2);      16x16 a doble
 *     icone_desenhar_indice(j, 40, 10, 0, 2);      el primero de la tabla
 */
#ifndef SAGUI_ICONES_H
#define SAGUI_ICONES_H
#include "gui.h"

/* El lado del pixel art (16x16). */
#define ICONES_LADO 16

typedef struct {
    const char *chave;          /* el nombre del icono ("pasta") */
    const char *const *linhas;  /* ICONES_LADO líneas de ICONES_LADO caracteres */
} icone_t;

/* Cuántos iconos hay en la biblioteca. */
int icones_total(void);
/* La tabla entera, para hacer una rejilla. */
const icone_t *icones_todos(void);
/* El icono con ese nombre, o NULL si no existe. */
const icone_t *icones_procurar(const char *chave);
/* El color de un carácter de la paleta (0xFF000000 = transparente). */
uint32_t icones_cor(char c);
/* Dibuja un icono por su nombre o por su posición en la tabla. */
void icone_desenhar(jan_t *j, int x, int y, const char *chave, int escala);
void icone_desenhar_indice(jan_t *j, int x, int y, int indice, int escala);

#endif
