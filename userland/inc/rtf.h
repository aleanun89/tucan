// SPDX-License-Identifier: GPL-2.0-or-later
/* rtf.h - lector y escritor de RTF para el control rte de la libsagui (mejora 116, D291).
 *
 * Subconjunto de la especificación pública RTF 1.x que cubre lo que escribe Word 97 y
 * LibreOffice en un texto normal: tablas de fuentes y colores, tamaño, negrita, cursiva,
 * subrayado, tachado, color, alineación, sangrías, interlineado, espaciado, viñetas simples,
 * saltos de página, tabuladores, caracteres \'hh y \uN, tamaño y márgenes de la hoja. Lo demás
 * (tablas, listas numeradas, estilos, cabeceras) se descarta; las imágenes PNG y JPEG se leen y se
 * escriben (\pict con \pngblip o \jpegblip, D296); las listas numeradas
 * pasan a viñetas. Sin macros ni campos. */
#ifndef SAGUI_RTF_H
#define SAGUI_RTF_H
#include "gui.h"

/* Carga el RTF en un documento vacío (lo limpia antes). 0 = bien, -1 = no es RTF. */
int rtf_ler(rte_t *r, const char *dados, int n);
/* Tras rtf_ler: cuántas fuentes del RTF no están en el sistema y se sustituyeron por la clásica más
 * parecida (D319), con sus nombres separados por comas en lista (si no es NULL). 0 = ninguna. */
int rtf_fuentes_sustituidas(char *lista, int n);
/* Escribe el documento como RTF; *saida se libera con libera(). 0 = bien. */
int rtf_escrever(rte_t *r, char **saida, int *n);

#endif
