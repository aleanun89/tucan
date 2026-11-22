// SPDX-License-Identifier: GPL-2.0-or-later
/* gui_pestanas.c - fila de pestañas para diálogos de varias páginas (mejora 117, D293).
 * Aspecto Windows 98: la pestaña activa sube dos píxeles y se funde con la página, que dibuja
 * quien la usa (un g_bisel debajo, en y + 22). */
#include "gui.h"

static int ancho_pestana(const pestanas_t *p, int i) { return g_largura_texto(p->nomes[i]) + 20; }

void pestanas_desenhar(jan_t *j, pestanas_t *p)
{
    g_retangulo(j, p->x, p->y, p->l, 22, W98_FACE);
    int x = p->x + 2;
    for (int i = 0; i < p->n; i++) {
        int l = ancho_pestana(p, i);
        bool a = i == p->sel;
        int y0 = p->y + (a ? 0 : 2);
        int h = 22 - (a ? 0 : 2);
        int xi = a ? x - 2 : x;
        int li = a ? l + 4 : l;
        g_retangulo(j, xi, y0, li, h, W98_FACE);
        g_linha(j, xi, y0 + 2, xi, p->y + 21, W98_LUZ);             /* luz izquierda */
        g_linha(j, xi + 1, y0 + 1, xi + 1, p->y + 21, W98_LUZ);
        g_linha(j, xi + 2, y0, xi + li - 3, y0, W98_LUZ);           /* luz de arriba */
        g_linha(j, xi + li - 2, y0 + 1, xi + li - 2, p->y + 21, W98_SOMBRA);
        g_linha(j, xi + li - 1, y0 + 2, xi + li - 1, p->y + 21, W98_NEGRO);
        g_texto(j, x + 10, y0 + (h - LETRA_A) / 2 + 1, p->nomes[i], W98_NEGRO);
        x += l;
    }
    /* La línea que cierra la fila por debajo, salvo bajo la pestaña activa. */
    int xa = p->x + 2;
    for (int i = 0; i < p->sel; i++) xa += ancho_pestana(p, i);
    int la = p->sel < p->n ? ancho_pestana(p, p->sel) : 0;
    g_linha(j, p->x, p->y + 21, xa - 3, p->y + 21, W98_LUZ);
    g_linha(j, xa + la + 2, p->y + 21, p->x + p->l - 1, p->y + 21, W98_LUZ);
    jan_mostrar_area(j, p->x, p->y, p->l, 22);
    GR_REG(j, "pestanas", p, p->x, p->y, p->l, 22, p->n > 0 ? p->nomes[p->sel] : "", 0, 0);
}

bool pestanas_tratar(jan_t *j, pestanas_t *p, const evento_t *e)
{
    if (e->tipo == EV_CLIQUE && (e->botoes & 1) && e->y >= p->y && e->y < p->y + 22) {
        int x = p->x + 2;
        for (int i = 0; i < p->n; i++) {
            int l = ancho_pestana(p, i);
            if (e->x >= x && e->x < x + l) {
                if (i == p->sel) return false;
                p->sel = i;
                pestanas_desenhar(j, p);
                return true;
            }
            x += l;
        }
    }
    return false;
}
