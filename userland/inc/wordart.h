// SPDX-License-Identifier: GPL-2.0-or-later
/* wordart.h - títulos con efectos tipo WordArt (mejora 122, D296).
 *
 * Genera una imagen RGB (fondo blanco) con un texto transformado y coloreado según uno de los
 * 20 estilos de la galería: relleno sólido, degradado, arco iris o rayas, contorno, sombra,
 * extrusión 3D y formas (arco arriba y abajo, onda, subida, cursiva, perspectiva). Los diseños son
 * propios. Todo en enteros (la libsagui no usa coma flotante).
 *
 * Las letras salen de un «proveedor de máscaras»: una función que dibuja el texto como cobertura de
 * 8 bits. Por defecto es la fuente 8x16 ampliada con suavizado; un programa que enlace la libfontes
 * (Pluma) instala otro con contornos vectoriales (DejaVu) con wordart_provedor(). */
#ifndef SAGUI_WORDART_H
#define SAGUI_WORDART_H
#include "imagem.h"

#define WORDART_ESTILOS 20
#define WORDART_TEXTO_MAX 96

/* Dibuja "texto" con la familia (0 sans, 1 serif, 2 mono) y el estilo (1 negrita, 2 cursiva) a "px"
 * píxeles de cuerpo. *mascara se reserva con aloca (largo x alto bytes de cobertura, 0 = nada). */
typedef bool (*wordart_mascara_fn)(const char *texto, int fam, int est, int px,
                                   uint8_t **mascara, int *largo, int *alto);
void wordart_provedor(wordart_mascara_fn f);      /* NULL = el de la fuente 8x16 */

const char *wordart_nome(int estilo);
/* Familia y estilo propios de un estilo de la galería (p. ej. negrita con serif). */
void wordart_padrao(int estilo, int *fam, int *est);
/* Tamaño natural en twips de un WordArt de ese texto con cuerpo "tam" puntos. */
void wordart_tamanho_natural(const char *texto, int estilo, int fam, int est, int tam, int *l_tw, int *a_tw);
/* Genera la imagen de l x a píxeles. false = sin memoria. */
bool wordart_renderizar(const char *texto, int estilo, int fam, int est, int l, int a, imagem_t *saida);

#endif
