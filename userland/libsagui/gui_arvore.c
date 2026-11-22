// SPDX-License-Identifier: GPL-2.0-or-later
/* gui_arvore.c - lista en árbol con ramas desplegables (D367, mejora 124).
 *
 * Los ítems vienen en orden de recorrido (padre antes que sus hijos) con su nivel de sangría; un
 * ítem tiene hijos si el siguiente es de un nivel más hondo. Una rama cerrada oculta todo lo que
 * cuelga de ella. Aspecto de SaguiOS: fondo blanco, marco gris, caja con +/- a la izquierda de
 * las ramas y selección azul. Teclado: arriba/abajo, izquierda cierra o sube al padre, derecha
 * abre o baja al primer hijo. */
#include "gui.h"

#define ARV_FILA  20
#define ARV_SANGRIA 14

static bool dentro(int px, int py, int x, int y, int l, int a)
{
    return px >= x && py >= y && px < x + l && py < y + a;
}

bool arvore_tem_filhos(const arvore_t *a, int i)
{
    return i >= 0 && i + 1 < a->n && a->nivel[i + 1] > a->nivel[i];
}

/* Un ítem se ve si todos sus antepasados están abiertos. */
bool arvore_visivel(const arvore_t *a, int i)
{
    if (i < 0 || i >= a->n) return false;
    int nivel = a->nivel[i];
    for (int k = i - 1; k >= 0 && nivel > 0; k--)
        if (a->nivel[k] < nivel) {
            if (!a->aberto[k]) return false;
            nivel = a->nivel[k];
        }
    return true;
}

int arvore_pai(const arvore_t *a, int i)
{
    for (int k = i - 1; k >= 0; k--) if (a->nivel[k] < a->nivel[i]) return k;
    return -1;
}

/* El ítem de la fila visible f (o -1), y cuántas filas visibles hay. */
static int fila_item(const arvore_t *a, int f)
{
    int v = 0;
    for (int i = 0; i < a->n; i++) {
        if (!arvore_visivel(a, i)) continue;
        if (v == f) return i;
        v++;
    }
    return -1;
}

static int fila_de(const arvore_t *a, int item)
{
    int v = 0;
    for (int i = 0; i < a->n; i++) {
        if (!arvore_visivel(a, i)) continue;
        if (i == item) return v;
        v++;
    }
    return -1;
}

static int filas_visiveis(const arvore_t *a)
{
    int v = 0;
    for (int i = 0; i < a->n; i++) if (arvore_visivel(a, i)) v++;
    return v;
}

static int filas_caben(const arvore_t *a) { int v = (a->a - 4) / ARV_FILA; return v < 1 ? 1 : v; }

static void limitar_topo(arvore_t *a)
{
    int max = filas_visiveis(a) - filas_caben(a);
    if (max < 0) max = 0;
    if (a->topo > max) a->topo = max;
    if (a->topo < 0) a->topo = 0;
}

void arvore_mostrar(arvore_t *a, int i)
{
    if (i < 0 || i >= a->n) return;
    for (int p = arvore_pai(a, i); p >= 0; p = arvore_pai(a, p)) a->aberto[p] = 1;
    int f = fila_de(a, i);
    if (f < a->topo) a->topo = f;
    if (f >= a->topo + filas_caben(a)) a->topo = f - filas_caben(a) + 1;
    limitar_topo(a);
}

void arvore_desenhar(jan_t *j, arvore_t *a)
{
    limitar_topo(a);
    g_retangulo(j, a->x, a->y, a->l, a->a, COR_CAMPO);
    g_moldura(j, a->x, a->y, a->l, a->a, COR_BORDA);
    int cab = filas_caben(a), f = 0;
    for (int i = 0; i < a->n; i++) {
        if (!arvore_visivel(a, i)) continue;
        int k = f++ - a->topo;
        if (k < 0) continue;
        if (k >= cab) break;
        int y = a->y + 2 + k * ARV_FILA, sx = a->x + 6 + a->nivel[i] * ARV_SANGRIA;
        bool sel = i == a->sel;
        if (sel) g_retangulo(j, a->x + 2, y, a->l - 4, ARV_FILA, COR_SELECAO);
        if (arvore_tem_filhos(a, i)) {
            g_retangulo(j, sx, y + 5, 9, 9, COR_CAMPO);
            g_moldura(j, sx, y + 5, 9, 9, sel ? COR_SEL_TEXTO : COR_BORDA);
            g_retangulo(j, sx + 2, y + 9, 5, 1, sel ? COR_SEL_TEXTO : COR_TEXTO);
            if (!a->aberto[i]) g_retangulo(j, sx + 4, y + 7, 1, 5, sel ? COR_SEL_TEXTO : COR_TEXTO);
        }
        int tx = sx + (arvore_tem_filhos(a, i) ? 14 : 4), cols = (a->x + a->l - tx - 8) / LETRA_L;
        if (cols > 0) g_texto_n(j, tx, y + 2, a->textos[i], cols, sel ? COR_SEL_TEXTO : COR_TEXTO);
        if (a->nivel[i] == 0) g_texto_n(j, tx + 1, y + 2, a->textos[i], cols > 0 ? cols : 0, sel ? COR_SEL_TEXTO : COR_TEXTO);   /* las ramas, en negrita */
    }
    int total = filas_visiveis(a);
    if (total > cab) {
        int trilho = a->a - 4, pol_a = trilho * cab / total;
        if (pol_a < 8) pol_a = 8;
        int pol_y = (trilho - pol_a) * a->topo / (total - cab);
        g_retangulo(j, a->x + a->l - 6, a->y + 2 + pol_y, 4, pol_a, sagui_esq.scroll_polegar);
    }
    jan_mostrar_area(j, a->x, a->y, a->l, a->a);
    GR_REG(j, "arvore", a, a->x, a->y, a->l, a->a, "", 0, 0);
}

static int mover(arvore_t *a, int f)
{
    int total = filas_visiveis(a);
    if (f < 0) f = 0;
    if (f >= total) f = total - 1;
    int i = fila_item(a, f);
    if (i >= 0) { a->sel = i; arvore_mostrar(a, i); }
    return i;
}

int arvore_tratar(jan_t *j, arvore_t *a, const evento_t *e)
{
    if (e->tipo == EV_RODA) {
        if (!dentro(e->x, e->y, a->x, a->y, a->l, a->a)) return ARVORE_NADA;
        int antes = a->topo;
        a->topo += (int)e->cliques * RODA_LINHAS;
        limitar_topo(a);
        if (a->topo != antes) arvore_desenhar(j, a);
        return ARVORE_NADA;
    }
    if (e->tipo == EV_CLIQUE && (e->botoes & 1) && dentro(e->x, e->y, a->x, a->y, a->l, a->a)) {
        int k = (e->y - a->y - 2) / ARV_FILA;
        if (e->y - a->y - 2 < 0) return ARVORE_NADA;
        int i = fila_item(a, a->topo + k);
        if (i < 0) return ARVORE_NADA;
        int sx = a->x + 6 + a->nivel[i] * ARV_SANGRIA;
        if (arvore_tem_filhos(a, i) && e->x >= sx - 2 && e->x < sx + 12) {   /* la caja +/-: solo abre o cierra */
            a->aberto[i] = !a->aberto[i];
            limitar_topo(a);
            arvore_desenhar(j, a);
            return ARVORE_ALTERNOU;
        }
        a->sel = i;
        arvore_desenhar(j, a);
        return ARVORE_ESCOLHEU;
    }
    if (e->tipo != EV_TECLA || a->n == 0) return ARVORE_NADA;
    int f = fila_de(a, a->sel), i = a->sel, antes = a->sel;
    if (e->tecla == TECLA_BAIXO) mover(a, f + 1);
    else if (e->tecla == TECLA_CIMA) mover(a, f - 1);
    else if (e->tecla == TECLA_HOME) mover(a, 0);
    else if (e->tecla == TECLA_END) mover(a, filas_visiveis(a) - 1);
    else if (e->tecla == TECLA_DIREITA) {
        if (arvore_tem_filhos(a, i) && !a->aberto[i]) { a->aberto[i] = 1; arvore_desenhar(j, a); return ARVORE_ALTERNOU; }
        if (arvore_tem_filhos(a, i)) mover(a, f + 1);
    } else if (e->tecla == TECLA_ESQUERDA) {
        if (arvore_tem_filhos(a, i) && a->aberto[i]) { a->aberto[i] = 0; arvore_desenhar(j, a); return ARVORE_ALTERNOU; }
        int p = arvore_pai(a, i);
        if (p >= 0) { a->sel = p; arvore_mostrar(a, p); }
    } else return ARVORE_NADA;
    if (a->sel == antes) return ARVORE_NADA;
    arvore_desenhar(j, a);
    return ARVORE_ESCOLHEU;
}
