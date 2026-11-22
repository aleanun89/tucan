// SPDX-License-Identifier: GPL-2.0-or-later
/* iconos98.c - iconos de 16x16 propios para las barras de herramientas estilo Office clásico
 * (mejora 117, D293). Dibujados con rectángulos y líneas, sin copiar ningún icono de terceros.
 * Cada función tiene la forma de barra_item_t.icone: dibuja en (x, y); con ativo = false se
 * pinta todo en gris (botón desactivado). Los usan Macaco y los programas que las necesiten. */
#include "gui.h"

#define R(x_, y_, w_, h_, c_) g_retangulo(j, x + (x_), y + (y_), (w_), (h_), (c_))
#define COR(a, c) ((a) ? (c) : 0x808080)

static void folha(jan_t *j, int x, int y, bool a, int dx, int dy, uint32_t fundo)
{
    R(dx, dy, 9, 11, COR(a, W98_NEGRO));
    R(dx + 1, dy + 1, 7, 9, COR(a, fundo));
    R(dx + 6, dy, 3, 3, COR(a, W98_FACE));
    R(dx + 6, dy + 2, 3, 1, COR(a, W98_NEGRO));
    R(dx + 6, dy, 1, 3, COR(a, W98_NEGRO));
}

void ic98_novo(jan_t *j, int x, int y, bool a) { folha(j, x, y, a, 3, 2, W98_LUZ); }

void ic98_abrir(jan_t *j, int x, int y, bool a)
{
    R(1, 4, 7, 2, COR(a, 0x808000)); R(1, 5, 13, 9, COR(a, 0xFFFF00)); R(1, 5, 13, 1, COR(a, 0x808000));
    R(1, 13, 13, 1, COR(a, 0x808000)); R(1, 5, 1, 9, COR(a, 0x808000)); R(13, 5, 1, 9, COR(a, 0x808000));
    R(4, 8, 11, 6, COR(a, 0xC0C000)); R(4, 8, 11, 1, COR(a, 0x808000)); R(14, 8, 1, 6, COR(a, 0x808000));
}

void ic98_guardar(jan_t *j, int x, int y, bool a)
{
    R(1, 1, 14, 14, COR(a, W98_NEGRO)); R(2, 2, 12, 12, COR(a, 0x0000A0));
    R(4, 2, 8, 5, COR(a, W98_LUZ)); R(5, 3, 3, 3, COR(a, W98_SOMBRA)); R(4, 9, 8, 5, COR(a, W98_FACE));
    R(6, 10, 4, 3, COR(a, W98_OSCURO));
}

void ic98_pdf(jan_t *j, int x, int y, bool a)
{
    folha(j, x, y, a, 3, 1, W98_LUZ);
    R(1, 8, 14, 7, COR(a, 0xC00000));
    R(2, 9, 1, 5, COR(a, W98_LUZ)); R(2, 9, 3, 1, COR(a, W98_LUZ)); R(4, 9, 1, 3, COR(a, W98_LUZ)); R(2, 11, 3, 1, COR(a, W98_LUZ));
    R(6, 9, 1, 5, COR(a, W98_LUZ)); R(6, 9, 3, 1, COR(a, W98_LUZ)); R(8, 9, 1, 5, COR(a, W98_LUZ)); R(6, 13, 3, 1, COR(a, W98_LUZ));
    R(10, 9, 1, 5, COR(a, W98_LUZ)); R(10, 9, 3, 1, COR(a, W98_LUZ)); R(10, 11, 3, 1, COR(a, W98_LUZ));
}

void ic98_cortar(jan_t *j, int x, int y, bool a)
{
    g_linha(j, x + 4, y + 1, x + 10, y + 10, COR(a, W98_NEGRO)); g_linha(j, x + 11, y + 1, x + 5, y + 10, COR(a, W98_NEGRO));
    R(2, 10, 4, 4, COR(a, 0xC00000)); R(3, 11, 2, 2, COR(a, W98_FACE)); R(9, 10, 4, 4, COR(a, 0xC00000)); R(10, 11, 2, 2, COR(a, W98_FACE));
}

void ic98_copiar(jan_t *j, int x, int y, bool a)
{
    folha(j, x, y, a, 1, 1, W98_LUZ); folha(j, x, y, a, 6, 4, W98_LUZ);
}

void ic98_colar(jan_t *j, int x, int y, bool a)
{
    R(1, 2, 11, 13, COR(a, 0x804000)); R(2, 3, 9, 11, COR(a, 0xC08040)); R(4, 1, 5, 3, COR(a, W98_OSCURO));
    folha(j, x, y, a, 6, 5, W98_LUZ);
}

void ic98_desfazer(jan_t *j, int x, int y, bool a)
{
    R(3, 3, 1, 1, COR(a, 0x000080)); R(2, 4, 3, 1, COR(a, 0x000080)); R(1, 5, 5, 1, COR(a, 0x000080));
    R(3, 6, 8, 2, COR(a, 0x000080)); R(10, 7, 3, 3, COR(a, 0x000080)); R(11, 10, 2, 2, COR(a, 0x000080)); R(3, 10, 8, 2, COR(a, 0x000080));
    R(4, 8, 6, 1, COR(a, 0x000080));
}

static void linhas_alin(jan_t *j, int x, int y, bool a, const int *xs, const int *ls)
{
    for (int i = 0; i < 5; i++) R(xs[i], 2 + i * 3, ls[i], 2, COR(a, W98_NEGRO));
}
void ic98_esq(jan_t *j, int x, int y, bool a) { static const int xs[5] = { 2, 2, 2, 2, 2 }, ls[5] = { 12, 8, 12, 8, 12 }; linhas_alin(j, x, y, a, xs, ls); }
void ic98_centro(jan_t *j, int x, int y, bool a) { static const int xs[5] = { 2, 4, 2, 4, 2 }, ls[5] = { 12, 8, 12, 8, 12 }; linhas_alin(j, x, y, a, xs, ls); }
void ic98_dir(jan_t *j, int x, int y, bool a) { static const int xs[5] = { 2, 6, 2, 6, 2 }, ls[5] = { 12, 8, 12, 8, 12 }; linhas_alin(j, x, y, a, xs, ls); }
