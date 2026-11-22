// SPDX-License-Identifier: GPL-2.0-or-later
/* gui_cinta.c - la cinta (Ribbon) con el estilo de SaguiOS (mejora 132, D334).
 *
 * Pestañas arriba (la de «Archivo» con el color de acento del sistema, D112) y debajo el panel de
 * grupos con su rótulo y, si hace falta, el lanzador. Se describe con tablas de C (gui.h). Si la
 * ventana estrecha, los grupos se reducen por prioridad: botones grandes -> pequeños con texto ->
 * solo iconos -> un único botón que abre el grupo como menú. La información sobre la herramienta
 * sale en la propia fila de pestañas (sin tapar el documento). Los menús y las listas usan el menú
 * contextual del sistema, como el resto de componentes de gui_barras.c. */
#include "gui.h"

#define TAB_A    CINTA_TAB_A
#define PANEL_A  CINTA_PANEL_A
#define CONT_A   70           /* alto del contenido de un grupo */
#define FILA_A   22
#define ROT_A    16
#define COD_ARCHIVO 7600
#define COD_MENU    7601
#define COD_COLAPSO 7602
#define COD_COMBO   7610      /* + id % 90 para las listas de los combos */

static uint32_t mezcla(uint32_t a, uint32_t b, int pb)   /* pb 0..256: parte de b */
{
    uint32_t r = 0;
    for (int s = 0; s <= 16; s += 8) {
        int ca = (int)((a >> s) & 255), cb = (int)((b >> s) & 255);
        r |= (uint32_t)((ca * (256 - pb) + cb * pb) >> 8) << s;
    }
    return r;
}

static int luma(uint32_t c) { return (int)((((c >> 16) & 255) * 299 + ((c >> 8) & 255) * 587 + (c & 255) * 114) / 1000); }

static char minus(unsigned t) { return (char)((t >= 'A' && t <= 'Z') ? t + 32 : t); }

static bool dentro(int px, int py, int x, int y, int l, int a)
{
    return px >= x && py >= y && px < x + l && py < y + a;
}

#define PAN() mezcla(W98_FACE, W98_LUZ, 90)

/* ------------------------------------------------------------ metricas */

static bool es_grande(const cinta_ctrl_t *k, int nivel)
{
    return k->tipo == CC_GALERIA || ((k->flags & CCF_GRANDE) && nivel == 0);
}

static bool tiene_flecha(const cinta_ctrl_t *k) { return k->tipo == CC_DESPLEG || k->tipo == CC_DIVIDIDO || k->tipo == CC_COR || k->tipo == CC_GALERIA; }

/* Reparte el texto de un botón grande en dos líneas (en el espacio más cercano al medio). */
static void lineas(const char *t, char *l1, char *l2)
{
    int n = (int)tam_texto(t), corte = -1, mejor = 99;
    for (int i = 1; i < n - 1; i++)
        if (t[i] == ' ') { int d = i > n / 2 ? i - n / 2 : n / 2 - i; if (d < mejor) { mejor = d; corte = i; } }
    if (n <= 8 || corte < 0) { copia_texto(l1, t, 24); l2[0] = 0; return; }
    int i;
    for (i = 0; i < corte && i < 23; i++) l1[i] = t[i];
    l1[i] = 0;
    copia_texto(l2, t + corte + 1, 24);
}

static int ancho_ctrl(const cinta_ctrl_t *k, int nivel)
{
    bool solo_icono = nivel >= 2 || (k->flags & CCF_SIN_TEXTO) || !k->texto;
    int fl = tiene_flecha(k) ? 12 : 0;
    switch (k->tipo) {
    case CC_COMBO: return k->ancho ? k->ancho : 80;
    case CC_FUENTES: return k->ancho ? k->ancho : 150;
    case CC_GALERIA: { int v = k->n_lista < 3 ? k->n_lista : 3; return v * 62 + 14 + 4; }
    case CC_COR: return 36;
    }
    if (es_grande(k, nivel)) {
        char a[24], b[24];
        lineas(k->texto ? k->texto : "", a, b);
        int w = (int)tam_texto(a) * LETRA_L, w2 = (int)tam_texto(b) * LETRA_L + (tiene_flecha(k) ? 12 : 0);
        if (w2 > w) w = w2;
        return w + 12 < 44 ? 44 : w + 12;
    }
    int w = 6 + (k->icone || solo_icono ? 16 : 0);
    if (!solo_icono) w += (k->icone ? 4 : 0) + g_largura_texto(k->texto);
    return w + fl + 2;
}

/* Coloca los controles de un grupo para un nivel; devuelve su ancho. */
static int grupo_layout(cinta_grupo_t *g, int nivel, int ox, int oy)
{
    if (nivel >= 3) return 52;
    int colx = ox + 4, fila = -1, colw = 0, rowx[3] = { 0, 0, 0 };
    for (int i = 0; i < g->n; i++) {
        cinta_ctrl_t *k = &g->ctrl[i];
        int w = ancho_ctrl(k, nivel);
        if (es_grande(k, nivel)) {
            if (fila >= 0) { colx += colw + 2; fila = -1; colw = 0; }
            k->x = colx; k->y = oy; k->l = w; k->a = CONT_A;
            colx += w + 2;
            continue;
        }
        if ((k->flags & CCF_SIGUE) && fila >= 0) {
            /* a la derecha del anterior, en la misma fila */
        } else {
            if (fila < 0) { fila = 0; colw = 0; }
            else if (fila >= 2 || (k->flags & CCF_SALTO)) { colx += colw + 2; fila = 0; colw = 0; }
            else fila++;
            rowx[fila] = colx;
        }
        k->x = rowx[fila]; k->y = oy + 2 + fila * FILA_A; k->l = w;
        k->a = (k->tipo == CC_COMBO || k->tipo == CC_FUENTES) ? 21 : FILA_A - 1;
        rowx[fila] += w + 1;
        if (rowx[fila] - colx > colw) colw = rowx[fila] - colx;
    }
    if (fila >= 0) colx += colw;
    return colx - ox + 2;
}

static int ancho_rotulo(const cinta_grupo_t *g)
{
    return g_largura_texto(g->nome) + 8 + (g->lanzador ? 12 : 0);
}

static int ancho_grupo(cinta_grupo_t *g, int nivel)
{
    int w = grupo_layout(g, nivel, 0, 0), r = ancho_rotulo(g);
    return w > r ? w : r;
}

int cinta_altura(const cinta_t *c) { return TAB_A + (c->minimizada && !c->temporal ? 1 : PANEL_A); }

static bool panel_visible(const cinta_t *c) { return !c->minimizada || c->temporal; }

static cinta_pest_t *pest_sel(cinta_t *c) { return c->sel >= 0 && c->sel < c->n_pest ? &c->pest[c->sel] : NULL; }

static bool pest_vis(const cinta_pest_t *p) { return !p->contextual || p->visible; }

static void ctrl_iniciar(cinta_ctrl_t *k)
{
    if (k->ini) return;
    k->ini = true;
    if (k->tipo == CC_COMBO)
        combo_iniciar(&k->cb, 0, 0, k->ancho ? k->ancho : 80, k->lista, k->n_lista, (k->flags & CCF_EDITABLE) != 0, COD_COMBO + k->id % 90);
    else if (k->tipo == CC_FUENTES) combo_fuentes_iniciar(&k->cf, 0, 0, k->ancho ? k->ancho : 150);
}

/* Calcula el layout de la pestaña activa: reduce grupos hasta que quepa. */
static void layout(cinta_t *c)
{
    cinta_pest_t *p = pest_sel(c);
    if (!p) return;
    int dispo = c->l - 4;
    for (int i = 0; i < p->n; i++) {
        p->grupos[i].nivel = 0;
        for (int k = 0; k < p->grupos[i].n; k++) ctrl_iniciar(&p->grupos[i].ctrl[k]);
    }
    for (;;) {
        int total = 2;
        for (int i = 0; i < p->n; i++) total += ancho_grupo(&p->grupos[i], p->grupos[i].nivel) + 3;
        if (total <= dispo) break;
        int mejor = -1;
        for (int i = 0; i < p->n; i++) {
            if (p->grupos[i].nivel >= 3) continue;
            if (mejor < 0 || p->grupos[i].prio <= p->grupos[mejor].prio) mejor = i;
        }
        if (mejor < 0) break;
        p->grupos[mejor].nivel++;
        if (c->depurar) registrar("cinta: nivel %s %d\n", p->grupos[mejor].nome, p->grupos[mejor].nivel);
    }
    int x = c->x + 2, oy = c->y + TAB_A + 3;
    for (int i = 0; i < p->n; i++) {
        cinta_grupo_t *g = &p->grupos[i];
        int w = grupo_layout(g, g->nivel, x, oy), r = ancho_rotulo(g);
        g->x = x; g->l = w > r ? w : r;
        x += g->l + 3;
    }
}

static void tabs_layout(cinta_t *c)
{
    int qat = c->n_acceso ? c->n_acceso * 22 + 8 : 0;
    int x = c->x + 2;
    for (int i = 0; i < c->n_acceso; i++) {
        cinta_ctrl_t *k = &c->acceso[i];
        k->x = x + i * 22; k->y = c->y + 1; k->l = 22; k->a = 20;
    }
    x += qat;
    c->arch_x = x; c->arch_l = c->archivo ? 8 * (int)tam_texto(_("Archivo")) + 20 : 0;
    x += c->arch_l ? c->arch_l + 2 : 0;
    for (int i = 0; i < c->n_pest; i++) {
        cinta_pest_t *p = &c->pest[i];
        p->x = x; p->l = 0;
        if (!pest_vis(p)) continue;
        p->l = g_largura_texto(p->nome) + 24;
        x += p->l + 1;
    }
}

/* ------------------------------------------------------------ dibujo */

static void icono_grande(jan_t *j, int x, int y, void (*ic)(jan_t *, int, int, bool), bool ativo)
{
    static uint32_t buf[256];
    jan_t t = { -1, buf, 16, 16, 0, 0, 0 };
    for (int i = 0; i < 256; i++) buf[i] = 0xFF00FF;
    int modo = gr_modo;
    gr_modo = 0;
    ic(&t, 0, 0, ativo);
    gr_modo = modo;
    for (int yy = 0; yy < 16; yy++)
        for (int xx = 0; xx < 16; xx++)
            if (buf[yy * 16 + xx] != 0xFF00FF) g_retangulo(j, x + xx * 2, y + yy * 2, 2, 2, buf[yy * 16 + xx]);
}

static void icono_generico(jan_t *j, int x, int y, int tam, const char *texto, uint32_t cor)
{
    g_retangulo(j, x + 1, y + 1, tam - 2, tam - 2, cor);
    g_moldura(j, x, y, tam, tam, W98_OSCURO);
    if (texto && texto[0]) {
        char t[2] = { texto[0], 0 };
        g_texto(j, x + (tam - 8) / 2, y + (tam - 16) / 2, t, COR_BRANCO);
    }
}

static void esquinas(jan_t *j, int x, int y, int l, int a, uint32_t fondo)
{
    g_retangulo(j, x, y, 1, 1, fondo); g_retangulo(j, x + l - 1, y, 1, 1, fondo);
    g_retangulo(j, x, y + a - 1, 1, 1, fondo); g_retangulo(j, x + l - 1, y + a - 1, 1, 1, fondo);
}

/* Fondo de un botón según su estado. */
static void fondo_boton(jan_t *j, cinta_t *c, int x, int y, int l, int a, bool sobre, bool pulsado, bool marcado)
{
    uint32_t pan = PAN();
    if (pulsado || marcado) {
        g_retangulo(j, x, y, l, a, pulsado ? mezcla(c->acento, W98_LUZ, 150) : c->suave);
        g_moldura(j, x, y, l, a, c->acento);
        esquinas(j, x, y, l, a, pan);
    } else if (sobre) {
        g_retangulo(j, x, y, l, a, mezcla(pan, W98_LUZ, 160));
        g_moldura(j, x, y, l, a, mezcla(c->acento, pan, 120));
        esquinas(j, x, y, l, a, pan);
    }
}

static void dibujar_ctrl(jan_t *j, cinta_t *c, cinta_ctrl_t *k, int nivel)
{
    bool sobre = c->sobre == k && !k->desativado, ap = c->apertado == k;
    uint32_t tc = k->desativado ? W98_SOMBRA : W98_NEGRO;
    int x = k->x, y = k->y, l = k->l, a = k->a;
    bool ativo = !k->desativado;
    switch (k->tipo) {
    case CC_COMBO:
        k->cb.x = x; k->cb.y = y; k->cb.l = l;
        combo_desenhar(j, &k->cb);
        return;
    case CC_FUENTES:
        k->cf.x = x; k->cf.y = y; k->cf.l = l;
        combo_fuentes_desenhar(j, &k->cf);
        return;
    case CC_GALERIA: {
        int v = k->n_lista < 3 ? k->n_lista : 3;
        for (int i = 0; i < v; i++) {
            int cx = x + i * 62;
            bool s = k->sel == i;
            g_retangulo(j, cx, y, 60, a, COR_CAMPO);
            g_moldura(j, cx, y, 60, a, s ? c->acento : W98_SOMBRA);
            if (s) g_moldura(j, cx + 1, y + 1, 58, a - 2, c->acento);
            g_texto_n(j, cx + (60 - 8 * (int)tam_texto(k->lista[i])) / 2 + 1, y + (a - 16) / 2, k->lista[i], 7, W98_NEGRO);
        }
        int ax = x + v * 62;
        fondo_boton(j, c, ax, y, 14, a, sobre && c->ap_zona == 1, ap, false);
        g_retangulo(j, ax, y, 14, a, PAN());
        g_moldura(j, ax, y, 14, a, W98_SOMBRA);
        g_flecha(j, ax + 3, y + a / 2 - 2, 1, tc);
        return;
    }
    }
    bool grande = es_grande(k, nivel);
    bool marcado = (k->tipo == CC_CONMUT || k->tipo == CC_EXCL) && k->marcado;
    bool solo_icono = nivel >= 2 || (k->flags & CCF_SIN_TEXTO) || !k->texto;
    int fl = tiene_flecha(k) ? 12 : 0;
    if (grande) {
        bool dividido = k->tipo == CC_DIVIDIDO;
        int hi = dividido ? 48 : a;     /* en el dividido, el icono va por un lado y el texto por otro */
        fondo_boton(j, c, x, y, l, a, sobre, ap, marcado);
        if (dividido && sobre) {
            g_retangulo(j, x + 1, y + 49, l - 2, 1, mezcla(c->acento, PAN(), 120));
        }
        (void)hi;
        int d = ap ? 1 : 0;
        if (k->icone) icono_grande(j, x + (l - 32) / 2 + d, y + 4 + d, k->icone, ativo);
        else icono_generico(j, x + (l - 32) / 2 + d, y + 4 + d, 32, k->texto, ativo ? c->acento : W98_SOMBRA);
        char l1[24], l2[24];
        lineas(k->texto ? k->texto : "", l1, l2);
        int ty = y + 4 + 32 + 2 + d;
        g_texto(j, x + (l - g_largura_texto(l1)) / 2 + d, ty, l1, tc);
        int w2 = g_largura_texto(l2) + (fl ? 12 : 0), tx = x + (l - w2) / 2 + d;
        if (l2[0]) g_texto(j, tx, ty + 16, l2, tc);
        if (fl) g_flecha(j, tx + g_largura_texto(l2) + 4, ty + 16 + 6, 1, tc);
        return;
    }
    fondo_boton(j, c, x, y, l, a, sobre, ap, marcado);
    int d = ap ? 1 : 0, ix = x + 3 + d;
    int iy = y + (a - 16) / 2 + d;
    if (k->tipo == CC_COR) {
        if (k->icone) k->icone(j, ix + 2, iy - 1, ativo);
        else icono_generico(j, ix + 2, iy - 1, 14, "A", c->acento);
        g_retangulo(j, ix + 1, iy + 13, 16, 3, k->cor);
        g_flecha(j, x + l - 11, y + a / 2 - 2, 1, tc);
        if (sobre) g_retangulo(j, x + l - 14, y + 2, 1, a - 4, mezcla(c->acento, PAN(), 120));
        return;
    }
    if (k->icone) { k->icone(j, ix, iy, ativo); ix += 20; }
    else if (solo_icono) { icono_generico(j, ix, iy, 16, k->texto, ativo ? c->acento : W98_SOMBRA); ix += 20; }
    if (!solo_icono) g_texto(j, ix + (k->icone ? 0 : 2), y + (a - LETRA_A) / 2 + d, k->texto, tc);
    if (fl) {
        g_flecha(j, x + l - 11, y + a / 2 - 2, 1, tc);
        if (k->tipo == CC_DIVIDIDO && (sobre || marcado)) g_retangulo(j, x + l - 14, y + 2, 1, a - 4, mezcla(c->acento, PAN(), 120));
    }
}

static void badge(jan_t *j, cinta_t *c, int cx, int y, const char *kt)
{
    int w = 8 * (int)tam_texto(kt) + 6;
    int x = cx - w / 2;
    g_retangulo(j, x, y, w, 17, c->acento);
    g_moldura(j, x, y, w, 17, W98_NEGRO);
    g_texto(j, x + 3, y + 1, kt, luma(c->acento) > 140 ? W98_NEGRO : COR_BRANCO);
}

static void tab_texto_info(jan_t *j, cinta_t *c)
{
    /* información sobre la herramienta: título, descripción y atajo, en el hueco de la fila de pestañas */
    cinta_ctrl_t *k = c->sobre;
    if (!k) return;
    const char *tip = k->tip ? k->tip : k->texto;
    if (!tip) return;
    char tit[48] = "", des[96] = "", ata[24] = "";
    char *dest[3] = { tit, des, ata };
    int m[3] = { 48, 96, 24 }, ix = 0, n = 0;
    for (const char *p = tip; *p; p++) {
        if (*p == '\n' && ix < 2) { dest[ix][n] = 0; ix++; n = 0; continue; }
        if (n < m[ix] - 1) dest[ix][n++] = *p;
    }
    dest[ix][n] = 0;
    char t[200];
    formatar_texto(t, sizeof t, "%s%s%s%s%s%s", tit, des[0] ? " - " : "", des, ata[0] ? "  (" : "", ata, ata[0] ? ")" : "");
    int x0 = c->x + c->l;
    for (int i = c->n_pest - 1; i >= 0; i--) if (c->pest[i].l) { x0 = c->pest[i].x + c->pest[i].l + 14; break; }
    int cols = (c->x + c->l - x0 - 4) / LETRA_L;
    if (cols < 6) return;
    g_texto_n(j, x0, c->y + 3, t, cols, W98_OSCURO);
}

static void log_posiciones(cinta_t *c);

void cinta_desenhar(jan_t *j, cinta_t *c)
{
    uint32_t pan = PAN();
    tabs_layout(c);
    int alt = cinta_altura(c);
    g_retangulo(j, c->x, c->y, c->l, TAB_A, W98_FACE);
    /* barra de acceso rápido */
    for (int i = 0; i < c->n_acceso; i++) {
        cinta_ctrl_t *k = &c->acceso[i];
        ctrl_iniciar(k);
        dibujar_ctrl(j, c, k, 2);
    }
    if (c->n_acceso) {
        int sx = c->x + 2 + c->n_acceso * 22 + 2;
        g_retangulo(j, sx, c->y + 4, 1, TAB_A - 8, W98_SOMBRA);
        g_retangulo(j, sx + 1, c->y + 4, 1, TAB_A - 8, W98_LUZ);
    }
    /* pestaña Archivo: rellena con el acento */
    if (c->archivo) {
        bool s = c->sobre_tab == -1;
        uint32_t f = s ? mezcla(c->acento, W98_LUZ, 40) : c->acento;
        g_retangulo(j, c->arch_x, c->y + 2, c->arch_l, TAB_A - 2, f);
        g_texto(j, c->arch_x + 10, c->y + 4, _("Archivo"), luma(c->acento) > 150 ? W98_NEGRO : COR_BRANCO);
    }
    /* línea que cierra la fila de pestañas */
    if (panel_visible(c)) g_retangulo(j, c->x, c->y + TAB_A - 1, c->l, 1, W98_SOMBRA);
    for (int i = 0; i < c->n_pest; i++) {
        cinta_pest_t *p = &c->pest[i];
        if (!p->l) continue;
        bool act = i == c->sel && panel_visible(c);
        bool nota = i == c->sel && !panel_visible(c);
        if (act) {
            g_retangulo(j, p->x, c->y + 1, p->l, TAB_A, pan);
            g_retangulo(j, p->x, c->y + 1, p->l, 3, c->acento);
            g_retangulo(j, p->x, c->y + 4, 1, TAB_A - 4, W98_SOMBRA);
            g_retangulo(j, p->x + p->l - 1, c->y + 4, 1, TAB_A - 4, W98_SOMBRA);
        } else if (p->contextual) {
            g_retangulo(j, p->x, c->y + 2, p->l, TAB_A - 2, c->suave);
            g_retangulo(j, p->x, c->y + 2, p->l, 2, c->acento);
        } else if (c->sobre_tab == i) {
            g_retangulo(j, p->x, c->y + 3, p->l, TAB_A - 4, mezcla(pan, W98_FACE, 128));
        }
        if (nota) g_retangulo(j, p->x + 4, c->y + TAB_A - 3, p->l - 8, 2, c->acento);
        g_texto(j, p->x + 12, c->y + (act ? 4 : 3), p->nome, W98_NEGRO);
    }
    if (c->kt_modo == 1) {
        if (c->archivo) badge(j, c, c->arch_x + c->arch_l / 2, c->y + 4, "A");
        for (int i = 0; i < c->n_pest; i++)
            if (c->pest[i].l && c->pest[i].kt) badge(j, c, c->pest[i].x + c->pest[i].l / 2, c->y + 4, c->pest[i].kt);
    }
    if (!panel_visible(c)) {
        g_retangulo(j, c->x, c->y + TAB_A, c->l, 1, W98_SOMBRA);
        if (c->kt_modo == 0) tab_texto_info(j, c);
        jan_mostrar_area(j, c->x, c->y, c->l, alt);
        GR_REG(j, "cinta", c, c->x, c->y, c->l, alt, "", 0, 0);
        return;
    }
    /* panel */
    int py = c->y + TAB_A;
    g_retangulo(j, c->x, py, c->l, PANEL_A - 1, pan);
    g_retangulo(j, c->x, py + PANEL_A - 1, c->l, 1, W98_SOMBRA);
    g_retangulo(j, c->x, py, 1, PANEL_A - 1, pan);
    cinta_pest_t *p = pest_sel(c);
    if (p) {
        /* el hueco de la línea bajo la pestaña activa */
        g_retangulo(j, p->x + 1, py - 1, p->l - 2, 1, pan);
        layout(c);
        int firma = c->sel * 100000 + c->l * 4 + (c->minimizada ? 1 : 0) + (c->temporal ? 2 : 0);
        if (c->depurar && firma != c->firma) { c->firma = firma; log_posiciones(c); }
        for (int i = 0; i < p->n; i++) {
            cinta_grupo_t *g = &p->grupos[i];
            if (g->nivel >= 3) {
                bool so = c->sobre == &g->ctrl[0];
                fondo_boton(j, c, g->x, py + 3, g->l, CONT_A, so, c->apertado == &g->ctrl[0], false);
                cinta_ctrl_t *k0 = &g->ctrl[0];
                if (k0->icone) icono_grande(j, g->x + (g->l - 32) / 2, py + 7, k0->icone, true);
                else icono_generico(j, g->x + (g->l - 32) / 2, py + 7, 32, g->nome, c->acento);
                g_texto(j, g->x + (g->l - g_largura_texto(_("Más"))) / 2 - 4, py + 43, _("Más"), W98_NEGRO);
                g_flecha(j, g->x + g->l / 2 + 6, py + 49, 1, W98_NEGRO);
            } else {
                for (int k = 0; k < g->n; k++) dibujar_ctrl(j, c, &g->ctrl[k], g->nivel);
            }
            int ly = py + 3 + CONT_A + 2;
            int lw = g_largura_texto(g->nome);
            g_texto(j, g->x + (g->l - (g->lanzador ? 12 : 0) - lw) / 2, ly, g->nome, mezcla(W98_OSCURO, pan, 40));
            if (g->lanzador) {
                int lx = g->x + g->l - 13, lyy = ly + 3;
                bool so = c->sobre_tab == 100 + i;
                if (so) fondo_boton(j, c, lx - 2, lyy - 2, 14, 14, true, false, false);
                g_retangulo(j, lx, lyy, 8, 1, W98_OSCURO); g_retangulo(j, lx, lyy, 1, 8, W98_OSCURO);
                g_linha(j, lx + 2, lyy + 2, lx + 7, lyy + 7, W98_OSCURO);
                g_retangulo(j, lx + 7, lyy + 4, 1, 4, W98_OSCURO); g_retangulo(j, lx + 4, lyy + 7, 4, 1, W98_OSCURO);
            }
            int sx = g->x + g->l + 1;
            if (i < p->n - 1) {
                g_retangulo(j, sx, py + 6, 1, PANEL_A - 14, W98_SOMBRA);
                g_retangulo(j, sx + 1, py + 6, 1, PANEL_A - 14, W98_LUZ);
            }
        }
    }
    if (c->kt_modo == 2 && p) {
        for (int i = 0; i < p->n; i++)
            for (int k = 0; k < p->grupos[i].n; k++) {
                cinta_ctrl_t *q = &p->grupos[i].ctrl[k];
                if (!q->kt || p->grupos[i].nivel >= 3) continue;
                if (c->kt_buf[0] && q->kt[0] != c->kt_buf[0]) continue;
                badge(j, c, q->x + (q->a > 40 ? q->l / 2 : 10), q->y + (q->a > 40 ? 20 : q->a - 8), q->kt);
            }
    }
    if (c->kt_modo == 0) tab_texto_info(j, c);
    jan_mostrar_area(j, c->x, c->y, c->l, alt);
    GR_REG(j, "cinta", c, c->x, c->y, c->l, alt, "", 0, 0);
}

/* ------------------------------------------------------------ API de estado */

void cinta_largura(cinta_t *c, int x, int y, int l) { c->x = x; c->y = y; c->l = l; }

static void ctrl_iniciar(cinta_ctrl_t *k);

void cinta_iniciar(cinta_t *c, int x, int y, int l, cinta_pest_t *pest, int n)
{
    int ac = acento_cor(-1);
    c->x = x; c->y = y; c->l = l; c->pest = pest; c->n_pest = n;
    c->sel = 0; c->minimizada = c->temporal = false; c->kt_modo = 0; c->kt_buf[0] = 0;
    c->sobre = c->apertado = NULL; c->sobre_tab = -2; c->cambio = false;
    c->acento = (ac >= 0 && ac <= 0xFFFFFF) ? (uint32_t)ac : COR_DESTAQUE;
    c->suave = mezcla(c->acento, W98_LUZ, 190);
    for (int i = 0; i < c->n_acceso; i++) ctrl_iniciar(&c->acceso[i]);
    for (int i = 0; i < n; i++)
        for (int g = 0; g < pest[i].n; g++)
            for (int k = 0; k < pest[i].grupos[g].n; k++) ctrl_iniciar(&pest[i].grupos[g].ctrl[k]);
}

cinta_ctrl_t *cinta_buscar(cinta_t *c, int id)
{
    for (int i = 0; i < c->n_acceso; i++) if (c->acceso[i].id == id) return &c->acceso[i];
    for (int i = 0; i < c->n_pest; i++)
        for (int g = 0; g < c->pest[i].n; g++)
            for (int k = 0; k < c->pest[i].grupos[g].n; k++)
                if (c->pest[i].grupos[g].ctrl[k].id == id) return &c->pest[i].grupos[g].ctrl[k];
    return NULL;
}

void cinta_marcar(cinta_t *c, int id, bool m)
{
    /* puede haber el mismo id en varias pestañas (p. ej. «Mostrar fórmulas»): se marcan todos */
    cinta_ctrl_t *k = cinta_buscar(c, id);
    if (!k) return;
    if (k->tipo == CC_EXCL && m && k->excl) {
        for (int i = 0; i < c->n_pest; i++)
            for (int g = 0; g < c->pest[i].n; g++)
                for (int q = 0; q < c->pest[i].grupos[g].n; q++) {
                    cinta_ctrl_t *o = &c->pest[i].grupos[g].ctrl[q];
                    if (o->tipo == CC_EXCL && o->excl == k->excl) o->marcado = false;
                }
    }
    for (int i = 0; i < c->n_pest; i++)
        for (int g = 0; g < c->pest[i].n; g++)
            for (int q = 0; q < c->pest[i].grupos[g].n; q++)
                if (c->pest[i].grupos[g].ctrl[q].id == id) c->pest[i].grupos[g].ctrl[q].marcado = m;
    for (int i = 0; i < c->n_acceso; i++) if (c->acceso[i].id == id) c->acceso[i].marcado = m;
}

void cinta_activar(cinta_t *c, int id, bool activo)
{
    cinta_ctrl_t *k = cinta_buscar(c, id);
    if (k) k->desativado = !activo;
}

void cinta_pestana(cinta_t *c, int i) { if (i >= 0 && i < c->n_pest && pest_vis(&c->pest[i])) c->sel = i; }

void cinta_pest_visible(cinta_t *c, int i, bool v)
{
    if (i < 0 || i >= c->n_pest) return;
    c->pest[i].visible = v;
    if (!v && c->sel == i) c->sel = 0;
}

void cinta_minimizar(cinta_t *c, bool min)
{
    if (c->minimizada == min && !c->temporal) return;
    c->minimizada = min; c->temporal = false; c->cambio = true;
    if (c->depurar) registrar("cinta: %s\n", min ? "minimizada" : "restaurada");
}

/* Pasa por _() los textos de las tablas (nombres, textos y ayudas); se llama una vez tras cinta_iniciar. */
void cinta_traducir(cinta_t *c)
{
    for (int i = 0; i < c->n_acceso; i++) { cinta_ctrl_t *k = &c->acceso[i]; if (k->tip) k->tip = _(k->tip); if (k->texto) k->texto = _(k->texto); }
    for (int i = 0; i < c->n_pest; i++) {
        cinta_pest_t *p = &c->pest[i];
        p->nome = _(p->nome);
        for (int g = 0; g < p->n; g++) {
            p->grupos[g].nome = _(p->grupos[g].nome);
            for (int k = 0; k < p->grupos[g].n; k++) {
                cinta_ctrl_t *q = &p->grupos[g].ctrl[k];
                if (q->texto) q->texto = _(q->texto);
                if (q->tip) q->tip = _(q->tip);
                for (int z = 0; q->tipo == CC_GALERIA && z < q->n_lista; z++) ((const char **)q->lista)[z] = (const char *)_(q->lista[z]);
            }
        }
    }
}

bool cinta_cambio(cinta_t *c) { bool r = c->cambio; c->cambio = false; return r; }

/* ------------------------------------------------------------ eventos */

typedef struct { cinta_ctrl_t *k; int g; int zona; } impacto_t;   /* zona: 0 principal, 1 flecha, 10+i celda de galería */

static void anotar_orden(cinta_t *c, int id)
{
    if (!c->depurar) return;
    cinta_ctrl_t *k = cinta_buscar(c, id);
    registrar("cinta: orden %d %s\n", id, k && k->texto ? k->texto : "?");
}

static void log_posiciones(cinta_t *c)
{
    if (!c->depurar) return;
    cinta_pest_t *p = pest_sel(c);
    if (!p) return;
    registrar("cinta: pestana %s y=%d a=%d\n", p->nome, c->y, cinta_altura(c));
    if (c->archivo) registrar("cinta: archivo %d %d %d\n", c->arch_x, c->y, c->arch_l);
    for (int i = 0; i < c->n_pest; i++) if (c->pest[i].l) registrar("cinta: tab %d %s %d %d %d\n", i, c->pest[i].nome, c->pest[i].x, c->y, c->pest[i].l);
    if (panel_visible(c)) {
        for (int i = 0; i < p->n; i++) {
            registrar("cinta: grupo %s nivel=%d x=%d l=%d\n", p->grupos[i].nome, p->grupos[i].nivel, p->grupos[i].x, p->grupos[i].l);
            if (p->grupos[i].nivel < 3)
                for (int k = 0; k < p->grupos[i].n; k++) {
                    cinta_ctrl_t *q = &p->grupos[i].ctrl[k];
                    registrar("cinta: ctrl %d %d %d %d %d %s\n", q->id, q->x, q->y, q->l, q->a, q->texto ? q->texto : "-");
                }
        }
    }
    for (int i = 0; i < c->n_acceso; i++) registrar("cinta: ctrl %d %d %d %d %d %s\n", c->acceso[i].id, c->acceso[i].x, c->acceso[i].y, c->acceso[i].l, c->acceso[i].a, c->acceso[i].texto ? c->acceso[i].texto : "-");
}

static bool buscar_impacto(cinta_t *c, int mx, int my, impacto_t *h)
{
    h->k = NULL; h->g = -1; h->zona = 0;
    for (int i = 0; i < c->n_acceso; i++) {
        cinta_ctrl_t *k = &c->acceso[i];
        if (dentro(mx, my, k->x, k->y, k->l, k->a)) { h->k = k; return true; }
    }
    cinta_pest_t *p = pest_sel(c);
    if (!p || !panel_visible(c) || my < c->y + TAB_A) return false;
    for (int i = 0; i < p->n; i++) {
        cinta_grupo_t *g = &p->grupos[i];
        if (mx < g->x || mx >= g->x + g->l) continue;
        if (g->nivel >= 3) {
            if (my >= c->y + TAB_A + 3 && my < c->y + TAB_A + 3 + CONT_A) { h->k = &g->ctrl[0]; h->g = i; h->zona = 3; return true; }
            return false;
        }
        if (g->lanzador && dentro(mx, my, g->x + g->l - 15, c->y + TAB_A + 3 + CONT_A + 3, 15, 14)) { h->g = i; h->zona = 2; return true; }
        for (int k = 0; k < g->n; k++) {
            cinta_ctrl_t *q = &g->ctrl[k];
            if (!dentro(mx, my, q->x, q->y, q->l, q->a)) continue;
            h->k = q; h->g = i;
            if (q->tipo == CC_GALERIA) {
                int v = q->n_lista < 3 ? q->n_lista : 3, cel = (mx - q->x) / 62;
                h->zona = cel < v ? 10 + cel : 1;
            } else if (es_grande(q, g->nivel)) {
                if (q->tipo == CC_DESPLEG || (q->tipo == CC_DIVIDIDO && my >= q->y + 48)) h->zona = 1;
            } else if (q->tipo == CC_DESPLEG || ((q->tipo == CC_DIVIDIDO || q->tipo == CC_COR) && mx >= q->x + q->l - 14)) h->zona = 1;
            return true;
        }
    }
    return false;
}

static void abrir_menu(jan_t *j, cinta_t *c, cinta_ctrl_t *k)
{
    if (!k->menu) return;
    c->menu_k = k;
    menu_contextual(j->id, k->x, k->y + k->a, k->menu, COD_MENU);
}

static void abrir_colapso(jan_t *j, cinta_t *c, int gi)
{
    cinta_grupo_t *g = &pest_sel(c)->grupos[gi];
    char buf[512];
    int n = 0, m = 0;
    for (int i = 0; i < g->n && m < 24; i++) {
        cinta_ctrl_t *k = &g->ctrl[i];
        if (k->tipo == CC_COMBO || k->tipo == CC_FUENTES || n > 440) continue;
        if (k->tipo == CC_GALERIA) {
            for (int q = 0; q < k->n_lista && m < 24; q++) {
                n += formatar_texto(buf + n, sizeof buf - (size_t)n, "%s%s", m ? "\n" : "", k->lista[q]);
                c->colapso_ids[m] = k->id; c->colapso_sel[m++] = q;
            }
            continue;
        }
        if (!k->texto) continue;
        n += formatar_texto(buf + n, sizeof buf - (size_t)n, "%s%s%s", m ? "\n" : "", k->desativado ? "~" : "", k->texto);
        c->colapso_ids[m] = k->id; c->colapso_sel[m++] = -1;
    }
    if (m) menu_contextual(j->id, g->x, c->y + TAB_A + 3 + CONT_A, buf, COD_COLAPSO);
}

static int activar_ctrl(jan_t *j, cinta_t *c, cinta_ctrl_t *k, int zona)
{
    if (k->desativado) return 0;
    switch (k->tipo) {
    case CC_DESPLEG: abrir_menu(j, c, k); return CINTA_TRATADO;
    case CC_DIVIDIDO:
        if (zona == 1) { abrir_menu(j, c, k); return CINTA_TRATADO; }
        return k->id;
    case CC_CONMUT: k->marcado = !k->marcado; return k->id;
    case CC_EXCL: cinta_marcar(c, k->id, true); return k->id;
    case CC_COR:
        if (zona == 1) {
            uint32_t cor = k->cor;
            if (!dialogo_cor(k->texto ? k->texto : "Color", &cor)) return CINTA_TRATADO;
            k->cor = cor;
        }
        return k->id;
    case CC_GALERIA:
        if (zona >= 10) { k->sel = zona - 10; return k->id; }
        if (k->lista && k->n_lista > 0) {
            char buf[512]; int n = 0;
            for (int i = 0; i < k->n_lista && n < 460; i++) n += formatar_texto(buf + n, sizeof buf - (size_t)n, "%s%s", i ? "\n" : "", k->lista[i]);
            c->menu_k = k;
            menu_contextual(j->id, k->x, k->y + k->a, buf, COD_MENU + 100);
        }
        return CINTA_TRATADO;
    default: return k->id;
    }
}

/* Cierra el panel temporal (cinta minimizada) tras usarlo. */
static void cerrar_temporal(cinta_t *c)
{
    if (c->minimizada && c->temporal) { c->temporal = false; c->cambio = true; }
}

static int devolver(jan_t *j, cinta_t *c, int id)
{
    if (id > 0) { anotar_orden(c, id); cerrar_temporal(c); }
    cinta_desenhar(j, c);
    return id;
}

static void seleccionar_pest(jan_t *j, cinta_t *c, int i)
{
    c->sel = i;
    if (c->depurar && c->kt_modo == 2) registrar("cinta: keytips 2 %s\n", c->pest[i].nome);
    if (c->minimizada) { c->temporal = true; c->cambio = true; }
    cinta_desenhar(j, c);
}

static int tab_en(cinta_t *c, int mx, int my)
{
    if (my < c->y || my >= c->y + TAB_A) return -2;
    if (c->archivo && mx >= c->arch_x && mx < c->arch_x + c->arch_l) return -1;
    for (int i = 0; i < c->n_pest; i++)
        if (c->pest[i].l && mx >= c->pest[i].x && mx < c->pest[i].x + c->pest[i].l) return i;
    return -2;
}

static void abrir_archivo(jan_t *j, cinta_t *c)
{
    if (c->archivo) menu_contextual(j->id, c->arch_x, c->y + TAB_A, c->archivo, COD_ARCHIVO);
}

/* KeyTips: busca el control cuyo KeyTip coincide con lo escrito. */
static int teclas_kt(jan_t *j, cinta_t *c, char ch)
{
    cinta_pest_t *p = pest_sel(c);
    if (c->kt_modo == 1) {
        if (c->archivo && ch == 'a') { c->kt_modo = 0; cinta_desenhar(j, c); abrir_archivo(j, c); return CINTA_TRATADO; }
        for (int i = 0; i < c->n_pest; i++)
            if (c->pest[i].l && c->pest[i].kt && minus((unsigned char)c->pest[i].kt[0]) == ch) {
                c->kt_modo = 2; c->kt_buf[0] = 0;
                seleccionar_pest(j, c, i);
                return CINTA_TRATADO;
            }
        return CINTA_TRATADO;
    }
    if (!p) return CINTA_TRATADO;
    int n = (int)tam_texto(c->kt_buf);
    c->kt_buf[n] = ch; c->kt_buf[n + 1] = 0;
    int cand = 0;
    for (int i = 0; i < p->n; i++) {
        if (p->grupos[i].nivel >= 3) continue;
        for (int k = 0; k < p->grupos[i].n; k++) {
            cinta_ctrl_t *q = &p->grupos[i].ctrl[k];
            if (!q->kt) continue;
            int m = (int)tam_texto(c->kt_buf), ok = 1;
            for (int z = 0; z < m; z++) if (minus((unsigned char)q->kt[z]) != c->kt_buf[z]) ok = 0;
            if (!ok) continue;
            if ((int)tam_texto(q->kt) == m) {
                c->kt_modo = 0; c->kt_buf[0] = 0;
                int id;
                if (q->tipo == CC_COMBO || q->tipo == CC_FUENTES) {
                    evento_t e2 = { .tipo = EV_CLIQUE, .x = q->x + q->l - 4, .y = q->y + 4, .botoes = 1 };
                    cinta_desenhar(j, c);
                    if (q->tipo == CC_COMBO) { q->cb.foco = (q->flags & CCF_EDITABLE) != 0; combo_tratar(j, &q->cb, &e2); }
                    else if (combo_fuentes_tratar(j, &q->cf, &e2) == COMBO_SELECIONOU) return devolver(j, c, q->id);
                    return CINTA_TRATADO;
                }
                id = activar_ctrl(j, c, q, q->tipo == CC_DESPLEG ? 1 : 0);
                return devolver(j, c, id);
            }
            cand++;
        }
    }
    if (!cand) c->kt_buf[0] = 0;
    cinta_desenhar(j, c);
    return CINTA_TRATADO;
}

static cinta_ctrl_t *ctrl_por_codigo_combo(cinta_t *c, unsigned cod)
{
    cinta_pest_t *p = pest_sel(c);
    if (!p) return NULL;
    for (int i = 0; i < p->n; i++)
        for (int k = 0; k < p->grupos[i].n; k++) {
            cinta_ctrl_t *q = &p->grupos[i].ctrl[k];
            if (q->tipo == CC_COMBO && (unsigned)(COD_COMBO + q->id % 90) == cod) return q;
        }
    return NULL;
}

int cinta_tratar(jan_t *j, cinta_t *c, const evento_t *e)
{
    /* ---- respuestas de los menús */
    if (e->tipo == EV_MENU) {
        if (e->tecla == COD_ARCHIVO) {
            if (e->x >= 0 && c->archivo_ids && c->archivo_ids[e->x]) return devolver(j, c, c->archivo_ids[e->x]);
            return CINTA_TRATADO;
        }
        if (e->tecla == COD_MENU) {
            cinta_ctrl_t *k = c->menu_k;
            if (k && k->ids && e->x >= 0 && k->ids[e->x]) return devolver(j, c, k->ids[e->x]);
            return CINTA_TRATADO;
        }
        if (e->tecla == COD_MENU + 100) {
            cinta_ctrl_t *k = c->menu_k;
            if (k && e->x >= 0 && e->x < k->n_lista) { k->sel = e->x; return devolver(j, c, k->id); }
            return CINTA_TRATADO;
        }
        if (e->tecla == COD_COLAPSO) {
            if (e->x >= 0 && e->x < 24 && c->colapso_ids[e->x]) {
                if (c->colapso_sel[e->x] >= 0) { cinta_ctrl_t *g = cinta_buscar(c, c->colapso_ids[e->x]); if (g) g->sel = c->colapso_sel[e->x]; }
                return devolver(j, c, c->colapso_ids[e->x]);
            }
            return CINTA_TRATADO;
        }
        cinta_ctrl_t *q = ctrl_por_codigo_combo(c, e->tecla);
        if (q) {
            int r = combo_tratar(j, &q->cb, e);
            if (r == COMBO_SELECIONOU || r == COMBO_ENTROU) { q->sel = q->cb.sel; return devolver(j, c, q->id); }
            return CINTA_TRATADO;
        }
        return 0;
    }
    /* ---- teclado */
    if (e->tipo == EV_TECLA) {
        unsigned t = e->tecla;
        if (t == 0x10A && (e->modificadores & 2)) { cinta_minimizar(c, !c->minimizada); cinta_desenhar(j, c); return CINTA_TRATADO; }
        if (c->kt_modo) {
            if (t == TECLA_ESC) { c->kt_modo = 0; c->kt_buf[0] = 0; cerrar_temporal(c); cinta_desenhar(j, c); return CINTA_TRATADO; }
            if (t == TECLA_ESQUERDA || t == TECLA_DIREITA) {
                int d = t == TECLA_DIREITA ? 1 : -1, i = c->sel;
                for (int z = 0; z < c->n_pest; z++) {
                    i = (i + d + c->n_pest) % c->n_pest;
                    if (c->pest[i].l) break;
                }
                c->kt_modo = 2; c->kt_buf[0] = 0;
                seleccionar_pest(j, c, i);
                return CINTA_TRATADO;
            }
            if (t < 0x100 && t > 32) return teclas_kt(j, c, minus(t));
            return CINTA_TRATADO;
        }
        if ((e->modificadores & 4) && t < 0x100) {
            char ch = minus(t);
            if (ch == '.') { c->kt_modo = 1; c->kt_buf[0] = 0; if (c->depurar) registrar("cinta: keytips 1\n"); cinta_desenhar(j, c); return CINTA_TRATADO; }
            if (c->archivo && ch == 'a') { abrir_archivo(j, c); return CINTA_TRATADO; }
            for (int i = 0; i < c->n_pest; i++)
                if (c->pest[i].l && c->pest[i].kt && minus((unsigned char)c->pest[i].kt[0]) == ch) {
                    c->kt_modo = 2; c->kt_buf[0] = 0;
                    seleccionar_pest(j, c, i);
                    return CINTA_TRATADO;
                }
            return 0;
        }
        /* un combo editable de la cinta con el foco recibe el teclado */
        cinta_pest_t *p = pest_sel(c);
        if (p)
            for (int i = 0; i < p->n; i++)
                for (int k = 0; k < p->grupos[i].n; k++) {
                    cinta_ctrl_t *q = &p->grupos[i].ctrl[k];
                    if (q->tipo == CC_COMBO && q->cb.foco) {
                        int r = combo_tratar(j, &q->cb, e);
                        if (r == COMBO_ENTROU) { q->sel = -1; return devolver(j, c, q->id); }
                        return CINTA_TRATADO;
                    }
                }
        return 0;
    }
    int alt = cinta_altura(c);
    /* ---- ratón */
    if (e->tipo == EV_MOVER && !(e->botoes & 1)) {
        impacto_t h;
        bool en = e->y >= c->y && e->y < c->y + alt && e->x >= c->x && e->x < c->x + c->l;
        cinta_ctrl_t *nuevo = NULL;
        int tab = -2;
        if (en) {
            tab = tab_en(c, e->x, e->y);
            if (tab == -2 && buscar_impacto(c, e->x, e->y, &h)) {
                if (h.zona == 2) tab = 100 + h.g;
                else if (!h.k->desativado && h.k->tipo != CC_COMBO && h.k->tipo != CC_FUENTES) nuevo = h.k;
            }
        }
        if (nuevo != c->sobre || tab != c->sobre_tab) {
            c->sobre = nuevo; c->sobre_tab = tab;
            cinta_desenhar(j, c);
        }
        return 0;
    }
    if (e->tipo == EV_CLIQUE && (e->botoes & 1)) {
        bool en = e->y >= c->y && e->y < c->y + alt && e->x >= c->x && e->x < c->x + c->l;
        if (!en) {
            if (c->kt_modo) { c->kt_modo = 0; c->kt_buf[0] = 0; cinta_desenhar(j, c); }
            if (c->minimizada && c->temporal) { cerrar_temporal(c); }
            return 0;
        }
        if (c->kt_modo) { c->kt_modo = 0; c->kt_buf[0] = 0; }
        int tab = tab_en(c, e->x, e->y);
        if (tab == -1) { cinta_desenhar(j, c); abrir_archivo(j, c); return CINTA_TRATADO; }
        if (tab >= 0) {
            if (e->cliques == 2) { cinta_minimizar(c, !c->minimizada); cinta_desenhar(j, c); return CINTA_TRATADO; }
            if (c->minimizada && c->temporal && c->sel == tab) { cerrar_temporal(c); cinta_desenhar(j, c); return CINTA_TRATADO; }
            seleccionar_pest(j, c, tab);
            return CINTA_TRATADO;
        }
        impacto_t h;
        if (!buscar_impacto(c, e->x, e->y, &h)) { cinta_desenhar(j, c); return CINTA_TRATADO; }
        if (h.zona == 2) return devolver(j, c, pest_sel(c)->grupos[h.g].lanzador);
        if (h.zona == 3) { abrir_colapso(j, c, h.g); return CINTA_TRATADO; }
        cinta_ctrl_t *k = h.k;
        if (k->desativado) return CINTA_TRATADO;
        if (k->tipo == CC_COMBO) {
            int r = combo_tratar(j, &k->cb, e);
            if (r == COMBO_SELECIONOU || r == COMBO_ENTROU) { k->sel = k->cb.sel; return devolver(j, c, k->id); }
            return CINTA_TRATADO;
        }
        if (k->tipo == CC_FUENTES) {
            if (combo_fuentes_tratar(j, &k->cf, e) == COMBO_SELECIONOU) return devolver(j, c, k->id);
            cinta_desenhar(j, c);
            return CINTA_TRATADO;
        }
        if (k->tipo == CC_DESPLEG || ((k->tipo == CC_DIVIDIDO || k->tipo == CC_COR) && h.zona == 1) || (k->tipo == CC_GALERIA && h.zona == 1)) {
            /* se abre al pulsar */
            if (k->tipo == CC_COR) { c->apertado = k; c->ap_zona = 1; cinta_desenhar(j, c); return CINTA_TRATADO; }
            int r = activar_ctrl(j, c, k, h.zona);
            cinta_desenhar(j, c);
            return r ? r : CINTA_TRATADO;
        }
        c->apertado = k; c->ap_zona = h.zona;
        cinta_desenhar(j, c);
        return CINTA_TRATADO;
    }
    if (e->tipo == EV_SOLTOU && c->apertado) {
        cinta_ctrl_t *k = c->apertado;
        int zona = c->ap_zona;
        c->apertado = NULL;
        impacto_t h;
        bool igual = buscar_impacto(c, e->x, e->y, &h) && h.k == k;
        if (igual) {
            int id = activar_ctrl(j, c, k, zona);
            return devolver(j, c, id > 0 ? id : CINTA_TRATADO);
        }
        cinta_desenhar(j, c);
        return CINTA_TRATADO;
    }
    return 0;
}

/* Recorta el tamaño que quiere abrir un programa a la pantalla principal (a 800x600 no cabe una ventana de
 * 820 ni de 900 de ancho). Con la cinta hace falta ver la fila de grupos entera. */
void cinta_tam_janela(int *l, int *a)
{
    info_telas_t t;
    if (telas_sis(TELAS_OP_INFO, (unsigned)&t, sizeof t) < 1) return;
    unsigned p = t.principal < t.qtd ? t.principal : 0;
    int pl = (int)t.t[p].largura, pa = (int)t.t[p].altura;
    if (pl >= 320 && *l > pl - 12) *l = pl - 12;
    if (pa >= 240 && *a > pa - 70) *a = pa - 70;
}
