// SPDX-License-Identifier: GPL-3.0-only
/* gui_explorador.c - componentes de un explorador de archivos moderno (v8.5.5.2, D420; D423: con el aspecto moderno).
 *
 * Ideas de diseño tomadas de «Files» (explorador de Windows, licencia MIT: solo ideas, ningún código),
 * con el aspecto de SaguiOS (bisel Windows 98 de D293 y los colores del tema):
 *   - abas_t      pestañas con botón de cerrar, botón «+» y orden que se cambia arrastrando;
 *   - migas_t     migas de pan con una flecha tras cada parte (la flecha lista las subcarpetas);
 *   - colunas_t   encabezado de columnas ordenable y con columnas de ancho ajustable;
 *   - barra_uso_desenhar  barra de espacio usado de una unidad.
 * Valen para cualquier programa con listas de ficheros. */
#include "gui.h"

/* ------------------------------------------------------------ abas */

#define ABA_MAX_L 150
#define ABA_MIN_L 44
#define ABA_BOTAO 20      /* ancho del «+» */

static int aba_largura(const abas_t *a)
{
    if (a->n <= 0) return ABA_MAX_L;
    int l = (a->l - ABA_BOTAO - 4) / a->n;
    if (l > ABA_MAX_L) l = ABA_MAX_L;
    if (l < ABA_MIN_L) l = ABA_MIN_L;
    return l;
}

/* La pestaña que hay en el punto (x, y), o -1. */
int abas_em(const abas_t *a, int x, int y)
{
    if (y < a->y || y >= a->y + ABAS_ALTURA) return -1;
    int l = aba_largura(a), px = a->x + 2;
    for (int i = 0; i < a->n; i++, px += l)
        if (x >= px && x < px + l) return i;
    return -1;
}

void abas_desenhar(jan_t *j, abas_t *a)
{
    const mod_tema_t *t = &mod_tema;
    g_retangulo(j, a->x, a->y, a->l, ABAS_ALTURA, t->barra);
    int l = aba_largura(a), x = a->x + 2;
    for (int i = 0; i < a->n; i++, x += l) {
        bool act = i == a->sel, hov = a->hover == i + 1 || i == a->destaque;
        int y0 = a->y + 2, h = ABAS_ALTURA - 3, w = l - 2;
        if (act) g_caixa_red(j, x, y0, w, h, t->radio, t->superficie, t->borde);
        else if (hov) g_retangulo_red(j, x, y0, w, h, t->radio, i == a->destaque ? t->acento_suave : t->hover);
        if (act) g_retangulo_red(j, x + 6, y0 + h - 2, w - 12, 2, 1, t->acento);   /* el subrayado de la pestaña activa */
        int util = w - 28;   /* hueco del texto: el cierre ocupa 18 px a la derecha */
        int n = util / LETRA_L;
        if (n < 1) n = 1;
        g_texto_n(j, x + 8, y0 + (h - LETRA_A) / 2, a->titulos[i], n, act ? t->texto : t->texto_fraco);
        /* la «x» de cerrar, con un círculo al pasar el ratón por la pestaña */
        int cx = x + w - 12, cy = y0 + h / 2;
        if (act || hov) {
            uint32_t cor = act ? t->texto_fraco : t->desab_texto;
            for (int k = -3; k <= 3; k++) {
                g_retangulo(j, cx + k, cy + k, 1, 1, cor);
                g_retangulo(j, cx + k, cy - k, 1, 1, cor);
            }
        }
    }
    /* el botón «+» */
    int bx = a->x + 2 + l * a->n + 2;
    if (bx + ABA_BOTAO <= a->x + a->l) {
        int cx = bx + (ABA_BOTAO - 2) / 2, cy = a->y + ABAS_ALTURA / 2;
        if (a->hover == -1) g_retangulo_red(j, bx, a->y + 3, ABA_BOTAO - 2, ABAS_ALTURA - 6, t->radio, t->hover);
        g_retangulo(j, cx - 4, cy - 1, 9, 2, t->texto_fraco);
        g_retangulo(j, cx - 1, cy - 4, 2, 9, t->texto_fraco);
    }
    jan_mostrar_area(j, a->x, a->y, a->l, ABAS_ALTURA);
    GR_REG(j, "abas", a, a->x, a->y, a->l, ABAS_ALTURA, a->n > 0 ? a->titulos[a->sel] : "", 0, 0);
}

int abas_tratar(jan_t *j, abas_t *a, const evento_t *e, int *i, int *k)
{
    int l = aba_largura(a);
    if (e->tipo == EV_MOVER) {   /* el resaltado de la pestaña (o del «+») bajo el ratón */
        int t = abas_em(a, e->x, e->y), bx = a->x + 2 + l * a->n + 2;
        int h = t >= 0 ? t + 1 : (e->y >= a->y && e->y < a->y + ABAS_ALTURA && e->x >= bx && e->x < bx + ABA_BOTAO) ? -1 : 0;
        if (h != a->hover) { a->hover = h; abas_desenhar(j, a); }
    }
    if (e->tipo == EV_SOLTOU) { a->premida = -1; return ABAS_NADA; }
    if (e->tipo == EV_CLIQUE && (e->botoes & 5) && e->y >= a->y && e->y < a->y + ABAS_ALTURA) {
        int t = abas_em(a, e->x, e->y);
        if (t >= 0) {
            int px = a->x + 2 + t * l;
            bool cierre = e->x >= px + l - 18 && (e->botoes & 1);
            if (cierre || (e->botoes & 4)) { *i = t; return ABAS_FECHOU; }   /* la x o el botón central */
            a->premida = t;
            if (t != a->sel) { a->sel = t; *i = t; abas_desenhar(j, a); return ABAS_SELECIONOU; }
            return ABAS_NADA;
        }
        int bx = a->x + 2 + l * a->n + 2;
        if (e->x >= bx && e->x < bx + ABA_BOTAO && a->n < ABAS_MAX) return ABAS_NOVA;
    }
    if (e->tipo == EV_MOVER && a->premida >= 0 && (e->botoes & 1)) {
        int t = abas_em(a, e->x, a->y);
        if (t >= 0 && t != a->premida) {   /* la pestaña cambia de sitio con la que tiene al lado */
            *i = a->premida; *k = t;
            char tmp[32];
            copia_texto(tmp, a->titulos[*i], sizeof tmp);
            copia_texto(a->titulos[*i], a->titulos[t], sizeof a->titulos[0]);
            copia_texto(a->titulos[t], tmp, sizeof a->titulos[0]);
            if (a->sel == *i) a->sel = t; else if (a->sel == t) a->sel = *i;
            a->premida = t;
            abas_desenhar(j, a);
            return ABAS_MOVEU;
        }
    }
    return ABAS_NADA;
}

/* ------------------------------------------------------------ migas */

#define MIGA_FLECHA 14

static void miga_nome(const char *p, int ini, int fim, char *saida, int max)
{
    int t = fim - ini;
    if (t > max - 1) t = max - 1;
    if (t < 0) t = 0;
    memcpy(saida, p + ini, (size_t)t);
    saida[t] = 0;
}

/* Parte la ruta y calcula dónde cae cada parte; si no cabe, esconde las primeras tras un «...». */
static void migas_partir(migas_t *m, const char *caminho)
{
    char nomes[MIGAS_MAX][40], cams[MIGAS_MAX][160];
    int n = 0, ini = 1, total = (int)tam_texto(caminho);
    copia_texto(nomes[0], "/", sizeof nomes[0]);
    copia_texto(cams[0], "/", sizeof cams[0]);
    n = 1;
    while (ini < total && n < MIGAS_MAX) {
        int fim = ini;
        while (caminho[fim] && caminho[fim] != '/') fim++;
        if (fim > ini) {
            miga_nome(caminho, ini, fim, nomes[n], 40);
            miga_nome(caminho, 0, fim, cams[n], 160);
            n++;
        }
        ini = fim + 1;
    }
    int ancho[MIGAS_MAX], soma = 0;
    for (int i = 0; i < n; i++) { ancho[i] = g_largura_texto(nomes[i]) + 8; soma += ancho[i] + MIGA_FLECHA; }
    int primeira = 0, disp = m->l - 8;
    while (primeira < n - 1 && soma + (primeira ? 24 : 0) > disp) { soma -= ancho[primeira] + MIGA_FLECHA; primeira++; }
    m->n = 0;
    int x = m->x + 4;
    if (primeira > 0) {   /* «...»: lleva a la carpeta anterior a la primera visible */
        m->p[m->n].x = x; m->p[m->n].l = 20; m->p[m->n].xs = x + 20;
        copia_texto(m->p[m->n].cam, cams[primeira - 1], sizeof m->p[0].cam);
        copia_texto(m->p[m->n].nome, "...", sizeof m->p[0].nome);
        m->n++;
        x += 24;
    }
    for (int i = primeira; i < n && m->n < MIGAS_MAX; i++) {
        m->p[m->n].x = x; m->p[m->n].l = ancho[i]; m->p[m->n].xs = x + ancho[i];
        copia_texto(m->p[m->n].cam, cams[i], sizeof m->p[0].cam);
        copia_texto(m->p[m->n].nome, nomes[i], sizeof m->p[0].nome);
        m->n++;
        x += ancho[i] + MIGA_FLECHA;
    }
}

void migas_desenhar(jan_t *j, migas_t *m, const char *caminho)
{
    const mod_tema_t *t = &mod_tema;
    /* el campo: píldora de esquinas suaves; con el foco del panel activo lleva el borde de acento */
    g_caixa_red(j, m->x, m->y - 3, m->l, m->a + 6, t->radio, t->campo, m->foco ? t->acento : t->borde);
    if (m->foco) g_aro_red(j, m->x + 1, m->y - 2, m->l - 2, m->a + 4, t->radio - 1, 1, t->acento);
    migas_partir(m, caminho);
    int ty = m->y + (m->a - LETRA_A) / 2;
    for (int i = 0; i < m->n; i++) {
        bool hp = m->hover == i + 1 && !m->hover_seta, hs = m->hover == i + 1 && m->hover_seta;
        if (hp) g_retangulo_red(j, m->p[i].x, m->y, m->p[i].xs - m->p[i].x, m->a, 3, t->hover);
        if (hs) g_retangulo_red(j, m->p[i].xs, m->y, MIGA_FLECHA, m->a, 3, t->hover);
        g_texto(j, m->p[i].x + 4, ty, m->p[i].nome, t->texto);
        /* la flecha: un chevron de 4x7 */
        int fx = m->p[i].xs + 5, fy = m->y + m->a / 2 - 3;
        for (int k = 0; k < 4; k++) {
            g_retangulo(j, fx + k, fy + k, 1, 1, hs ? t->texto : t->texto_fraco);
            g_retangulo(j, fx + k, fy + 6 - k, 1, 1, hs ? t->texto : t->texto_fraco);
        }
    }
    jan_mostrar_area(j, m->x, m->y - 3, m->l, m->a + 6);
}

int migas_hover_em(const migas_t *m, int x, int y, bool *seta)
{
    *seta = false;
    if (y < m->y || y >= m->y + m->a) return 0;
    for (int i = 0; i < m->n; i++) {
        if (x >= m->p[i].x && x < m->p[i].xs) return i + 1;
        if (x >= m->p[i].xs && x < m->p[i].xs + MIGA_FLECHA) { *seta = true; return i + 1; }
    }
    return 0;
}

int migas_em(const migas_t *m, int x, int y, char *cam, int max)
{
    if (y < m->y || y >= m->y + m->a) return 0;
    for (int i = 0; i < m->n; i++) {
        if (x >= m->p[i].x && x < m->p[i].xs) { copia_texto(cam, m->p[i].cam, (size_t)max); return 1; }
        if (x >= m->p[i].xs && x < m->p[i].xs + MIGA_FLECHA) { copia_texto(cam, m->p[i].cam, (size_t)max); return 2; }
    }
    return 0;
}

/* ------------------------------------------------------------ colunas */

#define COL_BORDA 3   /* píxeles a cada lado del borde que se agarran para ajustar */

int colunas_x(const colunas_t *c, int k)
{
    int x = c->x + c->l;
    for (int i = c->n - 1; i >= k && i >= 1; i--) x -= c->larg[i];
    return k == 0 ? c->x : x;
}

void colunas_desenhar(jan_t *j, colunas_t *c)
{
    g_retangulo(j, c->x, c->y, c->l, 18, sagui_esq.cab_fundo);
    g_retangulo(j, c->x, c->y + 17, c->l, 1, COR_BORDA);
    for (int k = 0; k < c->n; k++) {
        int x0 = colunas_x(c, k), x1 = k + 1 < c->n ? colunas_x(c, k + 1) : c->x + c->l;
        uint32_t cor = k == c->ord ? COR_DESTAQUE : COR_TEXTO;
        char t[24];
        copia_texto(t, c->titulos[k], sizeof t);
        int tl = g_largura_texto(t);
        /* nombre alineado a la izquierda o a la derecha (con 10 px de margen, como las filas) */
        int tx = c->direita[k] ? x1 - 10 - tl : x0 + 6;
        if (c->direita[k] && tx < x0 + 2) tx = x0 + 2;
        g_texto(j, tx, c->y + 2, t, cor);
        if (k == c->ord) g_texto(j, c->direita[k] ? x1 - 8 : tx + tl + 4, c->y + 2, c->desc ? "v" : "^", COR_DESTAQUE);
        if (k > 0) g_retangulo(j, x0, c->y + 2, 1, 14, sagui_esq.cab_linha);   /* el borde que se arrastra */
    }
    jan_mostrar_area(j, c->x, c->y, c->l, 18);
}

/* El borde de la columna k (k >= 1) bajo el punto, o -1. */
static int colunas_borda(const colunas_t *c, int x, int y)
{
    if (y < c->y || y >= c->y + 18) return -1;
    for (int k = 1; k < c->n; k++) {
        int bx = colunas_x(c, k);
        if (x >= bx - COL_BORDA && x <= bx + COL_BORDA) return k;
    }
    return -1;
}

int colunas_em(const colunas_t *c, int x, int y)
{
    if (y < c->y || y >= c->y + 18 || x < c->x || x >= c->x + c->l) return -1;
    for (int k = c->n - 1; k >= 0; k--)
        if (x >= colunas_x(c, k)) return k;
    return 0;
}

int colunas_tratar(jan_t *j, colunas_t *c, const evento_t *e, int *col)
{
    (void)j;
    if (e->tipo == EV_CLIQUE && (e->botoes & 1) && e->y >= c->y && e->y < c->y + 18) {
        int b = colunas_borda(c, e->x, e->y);
        if (b >= 0) { c->arrasto = b; c->arrasto_x0 = e->x; c->arrasto_l0 = c->larg[b]; return COLUNAS_NADA; }
        int k = colunas_em(c, e->x, e->y);
        if (k >= 0) { *col = k; return COLUNAS_ORDENAR; }
    }
    if (e->tipo == EV_MOVER && c->arrasto > 0 && (e->botoes & 1)) {
        int l = c->arrasto_l0 + (c->arrasto_x0 - e->x);   /* arrastrar el borde a la izquierda ensancha la columna */
        if (l < 40) l = 40;
        if (l > 320) l = 320;
        if (l == c->larg[c->arrasto]) return COLUNAS_NADA;
        c->larg[c->arrasto] = l;
        *col = c->arrasto;
        return COLUNAS_REDIM;
    }
    if (e->tipo == EV_SOLTOU && c->arrasto > 0) { c->arrasto = 0; return COLUNAS_NADA; }
    return COLUNAS_NADA;
}

/* ------------------------------------------------------------ barra de uso */

void barra_uso_desenhar(jan_t *j, int x, int y, int l, int a, int pct)
{
    progresso_mod_desenhar(j, x, y, l, a, pct, MP_USO, MOD_FUNDO_NENHUM);   /* D423: el progreso redondeado de la libsagui */
}
