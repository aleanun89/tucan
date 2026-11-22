// SPDX-License-Identifier: GPL-2.0-or-later
/* gui_barras.c - componentes de aplicaciones de escritorio con aspecto Windows 98 (mejora 116,
 * D291): bisel, barra de menús, barra de herramientas, lista desplegable, regla y barra de
 * estado. Los menús y las listas desplegables usan el menú contextual del sistema
 * (menu_contextual), así que se pintan encima de la ventana sin que el programa redibuje nada. */
#include "gui.h"

static bool dentro(int px, int py, int x, int y, int l, int a)
{
    return px >= x && py >= y && px < x + l && py < y + a;
}

static int iabs(int v) { return v < 0 ? -v : v; }

/* ------------------------------------------------------------ bisel y flechas */

void g_bisel(jan_t *j, int x, int y, int l, int a, bool af)
{
    uint32_t e1 = af ? W98_SOMBRA : W98_LUZ, e2 = af ? W98_LUZ : W98_OSCURO;
    uint32_t i1 = af ? W98_OSCURO : W98_FACE, i2 = af ? W98_FACE : W98_SOMBRA;
    g_retangulo(j, x, y, l - 1, 1, e1);  g_retangulo(j, x, y, 1, a - 1, e1);
    g_retangulo(j, x, y + a - 1, l, 1, e2); g_retangulo(j, x + l - 1, y, 1, a, e2);
    g_retangulo(j, x + 1, y + 1, l - 3, 1, i1); g_retangulo(j, x + 1, y + 1, 1, a - 3, i1);
    g_retangulo(j, x + 1, y + a - 2, l - 2, 1, i2); g_retangulo(j, x + l - 2, y + 1, 1, a - 2, i2);
}

void g_flecha(jan_t *j, int x, int y, int dir, uint32_t cor)
{
    for (int i = 0; i < 4; i++) {
        switch (dir) {
        case 0: g_retangulo(j, x + 3 - i, y + i, 1 + 2 * i, 1, cor); break;
        case 1: g_retangulo(j, x + i, y + i, 7 - 2 * i, 1, cor); break;
        case 2: g_retangulo(j, x + i, y + 3 - i, 1, 1 + 2 * i, cor); break;
        default: g_retangulo(j, x + 3 - i, y + i, 1, 7 - 2 * i, cor); break;
        }
    }
}

/* ------------------------------------------------------------ barra de menús */

#define MENU_COD 7000

void barra_menu_iniciar(barra_menu_t *b, int x, int y, int l)
{
    b->x = x; b->y = y; b->l = l; b->a = 20; b->aberto = -1;
    int xx = x + 2;
    for (int i = 0; i < b->n; i++) {
        b->x_item[i] = xx;
        b->l_item[i] = (int)tam_texto(b->nomes[i]) * LETRA_L + 14;
        xx += b->l_item[i];
    }
}

void barra_menu_desenhar(jan_t *j, barra_menu_t *b)
{
    g_retangulo(j, b->x, b->y, b->l, b->a, W98_FACE);
    g_retangulo(j, b->x, b->y + b->a - 1, b->l, 1, W98_SOMBRA);
    for (int i = 0; i < b->n; i++) {
        bool ab = b->aberto == i;
        if (ab) g_bisel(j, b->x_item[i], b->y + 1, b->l_item[i], b->a - 2, true);
        g_texto(j, b->x_item[i] + 7 + (ab ? 1 : 0), b->y + 2 + (ab ? 1 : 0), b->nomes[i], W98_NEGRO);
    }
    jan_mostrar_area(j, b->x, b->y, b->l, b->a);
    GR_REG(j, "barra_menu", b, b->x, b->y, b->l, b->a, "", 0, 0);
}

int barra_menu_tratar(jan_t *j, barra_menu_t *b, const evento_t *e)
{
    if (e->tipo == EV_CLIQUE && (e->botoes & 1) && e->y >= b->y && e->y < b->y + b->a) {
        for (int i = 0; i < b->n; i++)
            if (e->x >= b->x_item[i] && e->x < b->x_item[i] + b->l_item[i]) {
                b->aberto = i;
                barra_menu_desenhar(j, b);
                menu_contextual(j->id, b->x_item[i], b->y + b->a, b->itens[i], MENU_COD + i);
                return -1;
            }
        return -1;
    }
    if (e->tipo == EV_MENU && e->tecla >= (unsigned)MENU_COD && e->tecla < (unsigned)MENU_COD + BARRA_MENU_MAX) {
        int m = (int)e->tecla - MENU_COD;
        b->aberto = -1;
        barra_menu_desenhar(j, b);
        if (e->x < 0) return -1;
        return BARRA_MENU_ITEM(m, e->x);
    }
    return -1;
}

/* ------------------------------------------------------------ barra de herramientas */

#define BTN_L 23
#define BTN_A 22

static int item_larg(const barra_item_t *it)
{
    if (it->larg) return it->larg;
    return it->id == 0 ? 8 : BTN_L;
}

void barra_ferr_iniciar(barra_ferr_t *b, int x, int y, int l, barra_item_t *itens, int n)
{
    b->x = x; b->y = y; b->l = l; b->a = BTN_A + 6;
    b->itens = itens; b->n = n; b->apertado = -1; b->sobre = -1;
}

int barra_ferr_x(const barra_ferr_t *b, int indice)
{
    int x = b->x + 4;
    for (int i = 0; i < indice && i < b->n; i++) x += item_larg(&b->itens[i]);
    return x;
}

void barra_ferr_marcar(barra_ferr_t *b, int id, bool m)
{
    for (int i = 0; i < b->n; i++) if (b->itens[i].id == id) b->itens[i].marcado = m;
}

static void ferr_item(jan_t *j, barra_ferr_t *b, int i)
{
    barra_item_t *it = &b->itens[i];
    int x = barra_ferr_x(b, i), y = b->y + 3, l = item_larg(it);
    if (it->id < 0) return;
    if (it->id == 0) {
        g_retangulo(j, x + 3, y + 1, 1, BTN_A - 2, W98_SOMBRA);
        g_retangulo(j, x + 4, y + 1, 1, BTN_A - 2, W98_LUZ);
        return;
    }
    bool ap = b->apertado == i && b->sobre == i, ch = it->marcado;
    uint32_t fundo = (ch && !ap) ? W98_MARCADA : W98_FACE;
    g_retangulo(j, x, y, l, BTN_A, fundo);
    if (!it->desativado) {
        if (ap || ch) g_bisel(j, x, y, l, BTN_A, true);
        else if (b->sobre == i) g_bisel(j, x, y, l, BTN_A, false);
    }
    int d = (ap || ch) ? 1 : 0;
    if (it->icone) it->icone(j, x + (l - 16) / 2 + d, y + (BTN_A - 16) / 2 + d, !it->desativado);
    else if (it->texto) {
        int tl = g_largura_texto(it->texto), tx = x + (l - tl) / 2 + d, ty = y + (BTN_A - LETRA_A) / 2 + d;
        uint32_t cor = it->desativado ? W98_SOMBRA : W98_NEGRO;
        g_texto(j, tx, ty, it->texto, cor);
        if (it->negrita) g_texto(j, tx + 1, ty, it->texto, cor);
    }
}

void barra_ferr_desenhar(jan_t *j, barra_ferr_t *b)
{
    g_retangulo(j, b->x, b->y, b->l, b->a, W98_FACE);
    g_retangulo(j, b->x, b->y, b->l, 1, W98_LUZ);
    g_retangulo(j, b->x, b->y + b->a - 1, b->l, 1, W98_SOMBRA);
    for (int i = 0; i < b->n; i++) ferr_item(j, b, i);
    jan_mostrar_area(j, b->x, b->y, b->l, b->a);
    GR_REG(j, "barra_ferr", b, b->x, b->y, b->l, b->a, "", 0, 0);
}

static int ferr_em(const barra_ferr_t *b, int mx, int my)
{
    if (my < b->y + 3 || my >= b->y + 3 + BTN_A) return -1;
    for (int i = 0; i < b->n; i++) {
        const barra_item_t *it = &b->itens[i];
        if (it->id <= 0) continue;
        int x = barra_ferr_x(b, i);
        if (mx >= x && mx < x + item_larg(it)) return i;
    }
    return -1;
}

int barra_ferr_tratar(jan_t *j, barra_ferr_t *b, const evento_t *e)
{
    if (e->tipo == EV_MOVER && !(e->botoes & 1)) {
        int i = ferr_em(b, e->x, e->y);
        if (i >= 0 && b->itens[i].desativado) i = -1;
        if (i != b->sobre) {
            int antes = b->sobre;
            b->sobre = i;
            if (antes >= 0) ferr_item(j, b, antes);
            if (i >= 0) ferr_item(j, b, i);
            jan_mostrar_area(j, b->x, b->y, b->l, b->a);
        }
        return 0;
    }
    if (e->tipo == EV_CLIQUE && (e->botoes & 1)) {
        int i = ferr_em(b, e->x, e->y);
        if (i >= 0 && !b->itens[i].desativado) {
            b->apertado = i; b->sobre = i;
            ferr_item(j, b, i);
            jan_mostrar_area(j, b->x, b->y, b->l, b->a);
        }
        return 0;
    }
    if (e->tipo == EV_SOLTOU && b->apertado >= 0) {
        int i = b->apertado;
        b->apertado = -1;
        bool igual = ferr_em(b, e->x, e->y) == i;
        if (!igual) b->sobre = -1;
        if (igual && b->itens[i].conmutavel) b->itens[i].marcado = !b->itens[i].marcado;
        ferr_item(j, b, i);
        jan_mostrar_area(j, b->x, b->y, b->l, b->a);
        return igual ? b->itens[i].id : 0;
    }
    return 0;
}

/* ------------------------------------------------------------ lista desplegable */

void combo_iniciar(combo_t *c, int x, int y, int l, const char *const *itens, int qtd, bool editavel, int codigo)
{
    memset(c, 0, sizeof *c);
    c->x = x; c->y = y; c->l = l; c->a = 21;
    c->itens = itens; c->qtd = qtd; c->editavel = editavel; c->codigo = codigo; c->sel = -1;
}

void combo_definir(combo_t *c, const char *texto) { copia_texto(c->texto, texto, sizeof c->texto); c->cursor = (int)tam_texto(c->texto); }

void combo_desenhar(jan_t *j, combo_t *c)
{
    g_retangulo(j, c->x, c->y, c->l, c->a, COR_CAMPO);
    g_bisel(j, c->x, c->y, c->l, c->a, true);
    int cols = (c->l - 4 - 17 - 4) / LETRA_L;
    int n = (int)tam_texto(c->texto);
    if (c->foco && c->editavel) {
        g_retangulo(j, c->x + 2, c->y + 2, c->l - 4 - 16, c->a - 4, W98_AZUL);
        g_texto_n(j, c->x + 4, c->y + 2, c->texto, cols, COR_SEL_TEXTO);
    } else g_texto_n(j, c->x + 4, c->y + 2, c->texto, cols, W98_NEGRO);
    (void)n;
    int bx = c->x + c->l - 18;
    g_retangulo(j, bx, c->y + 2, 16, c->a - 4, W98_FACE);
    g_bisel(j, bx, c->y + 2, 16, c->a - 4, false);
    g_flecha(j, bx + 5, c->y + c->a / 2 - 2, 1, W98_NEGRO);
    jan_mostrar_area(j, c->x, c->y, c->l, c->a);
    GR_REG(j, "combo", c, c->x, c->y, c->l, c->a, c->texto, c->l - 4 - 17 - 4, GRF_CONTROL);
}

static void combo_abrir(jan_t *j, combo_t *c)
{
    char buf[768];
    int n = 0;
    for (int i = 0; i < c->qtd && n < (int)sizeof buf - 40; i++)
        n += formatar_texto(buf + n, sizeof buf - (size_t)n, "%s%s", i ? "\n" : "", c->itens[i]);
    menu_contextual(j->id, c->x, c->y + c->a, buf, c->codigo);
}

int combo_tratar(jan_t *j, combo_t *c, const evento_t *e)
{
    if (e->tipo == EV_MENU && e->tecla == (unsigned)c->codigo) {
        if (e->x >= 0 && e->x < c->qtd) {
            c->sel = e->x;
            combo_definir(c, c->itens[e->x]);
            combo_desenhar(j, c);
            return COMBO_SELECIONOU;
        }
        return COMBO_NADA;
    }
    if (e->tipo == EV_CLIQUE && (e->botoes & 1)) {
        bool dentro_c = dentro(e->x, e->y, c->x, c->y, c->l, c->a);
        bool estava = c->foco;
        c->foco = dentro_c && c->editavel;
        if (dentro_c) {
            bool seta = e->x >= c->x + c->l - 18 || !c->editavel;
            if (seta) { combo_desenhar(j, c); combo_abrir(j, c); return COMBO_NADA; }
            c->cursor = (int)tam_texto(c->texto);
        }
        if (dentro_c || estava) combo_desenhar(j, c);
        return COMBO_NADA;
    }
    if (e->tipo == EV_TECLA && c->foco && c->editavel) {
        int n = (int)tam_texto(c->texto);
        unsigned t = e->tecla;
        if (t == '\n') { c->foco = false; combo_desenhar(j, c); return COMBO_ENTROU; }
        if (t == '\b') { if (n > 0) c->texto[n - 1] = 0; }
        else if (t >= 0x20 && t < 0x7F && n < (int)sizeof c->texto - 1) { c->texto[n] = (char)t; c->texto[n + 1] = 0; }
        else return COMBO_NADA;
        combo_desenhar(j, c);
    }
    return COMBO_NADA;
}

/* ------------------------------------------------------------ regla */

#define TWIPS_CM 567

static int rpx(const regua_t *r, int tw)
{
    long p = (long)tw * r->zoom;
    return r->ox + (int)((p + (p >= 0 ? 750 : -750)) / 1500);
}

static void digitos_mini(jan_t *j, int x, int y, int v, uint32_t cor)
{
    static const unsigned short g[10] = {   /* 3x5, 15 bits (fila a fila) */
        0x7B6F, 0x2C97, 0x73E7, 0x73CF, 0x5BC9, 0x79CF, 0x79EF, 0x7249, 0x7BEF, 0x7BCF };
    char t[8]; int n = formatar_texto(t, sizeof t, "%d", v);
    for (int k = 0; k < n; k++) {
        if (t[k] == '-') { g_retangulo(j, x, y + 2, 3, 1, cor); x += 4; continue; }
        unsigned bits = g[t[k] - '0'];
        for (int f = 0; f < 5; f++)
            for (int c = 0; c < 3; c++)
                if (bits & (1u << (14 - (f * 3 + c)))) g_retangulo(j, x + c, y + f, 1, 1, cor);
        x += 4;
    }
}

void regua_iniciar(regua_t *r, int x, int y, int l)
{
    memset(r, 0, sizeof *r);
    r->x = x; r->y = y; r->l = l; r->a = 20; r->zoom = 100;
    r->pagina = 11906; r->m_esq = r->m_dir = 1417; r->ox = x + 20;
}

static void marcador(jan_t *j, int x, int y, bool abajo)
{
    for (int i = 0; i < 5; i++) {
        int w = abajo ? 9 - 2 * i : 1 + 2 * i;
        g_retangulo(j, x + 4 - w / 2, y + i, w, 1, W98_FACE);
        g_retangulo(j, x + 4 - w / 2, y + i, 1, 1, W98_NEGRO);
        g_retangulo(j, x + 4 + w / 2, y + i, 1, 1, W98_NEGRO);
    }
    if (abajo) g_retangulo(j, x + 1, y + 4, 7, 1, W98_NEGRO);
    else g_retangulo(j, x + 1, y, 7, 1, W98_NEGRO);
}

void regua_desenhar(jan_t *j, regua_t *r)
{
    g_retangulo(j, r->x, r->y, r->l, r->a, W98_FACE);
    int ty = r->y + 4, ta = r->a - 8;
    int x0 = rpx(r, 0), x1 = rpx(r, r->pagina);
    int xm0 = rpx(r, r->m_esq), xm1 = rpx(r, r->pagina - r->m_dir);
    /* zona de la hoja: gris en los márgenes, blanco en el área de texto */
    int a = x0 < r->x ? r->x : x0, b = x1 > r->x + r->l ? r->x + r->l : x1;
    if (b > a) g_retangulo(j, a, ty, b - a, ta, W98_SOMBRA);
    a = xm0 < r->x ? r->x : xm0; b = xm1 > r->x + r->l ? r->x + r->l : xm1;
    if (b > a) g_retangulo(j, a, ty, b - a, ta, COR_CAMPO);
    g_retangulo(j, r->x, ty - 1, r->l, 1, W98_SOMBRA);
    g_retangulo(j, r->x, ty + ta, r->l, 1, W98_LUZ);
    /* graduación en cm desde el margen izquierdo, en ambos sentidos */
    for (int k = -30; k <= 60; k++) {
        for (int medio = 0; medio < 2; medio++) {
            int tw = r->m_esq + k * TWIPS_CM + medio * (TWIPS_CM / 2);
            if (tw < 0 || tw > r->pagina) continue;
            int px = rpx(r, tw);
            if (px < xm0 || px > xm1 || px < r->x || px >= r->x + r->l) continue;
            if (medio) g_retangulo(j, px, ty + ta / 2 - 1, 1, 3, W98_SOMBRA);
            else if (k == 0) continue;
            else { digitos_mini(j, px - 1, ty + ta / 2 - 2, k < 0 ? -k : k, W98_NEGRO); }
        }
    }
    marcador(j, rpx(r, r->m_esq + r->sang_esq + r->sang_prim) - 4, ty - 3, false);
    marcador(j, rpx(r, r->m_esq + r->sang_esq) - 4, ty + ta - 3, true);
    marcador(j, rpx(r, r->pagina - r->m_dir - r->sang_dir) - 4, ty + ta - 3, true);
    jan_mostrar_area(j, r->x, r->y, r->l, r->a);
    GR_REG(j, "regua", r, r->x, r->y, r->l, r->a, "", 0, 0);
}

static int snap57(int v) { return (v + (v >= 0 ? 28 : -28)) / 57 * 57; }

int regua_tratar(jan_t *j, regua_t *r, const evento_t *e)
{
    int ty = r->y + 4, ta = r->a - 8;
    if (e->tipo == EV_CLIQUE && (e->botoes & 1) && dentro(e->x, e->y, r->x, r->y, r->l, r->a)) {
        int px_prim = rpx(r, r->m_esq + r->sang_esq + r->sang_prim), px_sang = rpx(r, r->m_esq + r->sang_esq);
        int px_dir = rpx(r, r->pagina - r->m_dir - r->sang_dir);
        bool arriba = e->y < ty + ta / 2;
        if (arriba && iabs(e->x - px_prim) <= 5) r->arrastando = REGUA_M_PRIM;
        else if (!arriba && iabs(e->x - px_sang) <= 5) r->arrastando = REGUA_M_SANG;
        else if (!arriba && iabs(e->x - px_dir) <= 5) r->arrastando = REGUA_M_SANG_D;
        else if (iabs(e->x - rpx(r, r->m_esq)) <= 3) r->arrastando = REGUA_M_ESQ;
        else if (iabs(e->x - rpx(r, r->pagina - r->m_dir)) <= 3) r->arrastando = REGUA_M_DIR;
        return REGUA_NADA;
    }
    if (r->arrastando && e->tipo == EV_MOVER && (e->botoes & 1)) {
        int tw = snap57((e->x - r->ox) * 1500 / r->zoom);
        int rel = tw - r->m_esq;
        int abs_prim = r->sang_esq + r->sang_prim;
        switch (r->arrastando) {
        case REGUA_M_ESQ: if (tw >= 0 && tw < r->pagina - r->m_dir - 1134) r->m_esq = tw; break;
        case REGUA_M_DIR: { int v = r->pagina - tw; if (v >= 0 && v < r->pagina - r->m_esq - 1134) r->m_dir = v; break; }
        case REGUA_M_PRIM: r->sang_prim = rel - r->sang_esq; break;
        case REGUA_M_SANG: if (rel >= 0) { r->sang_esq = rel; r->sang_prim = abs_prim - rel; } break;
        case REGUA_M_SANG_D: { int v = (r->pagina - r->m_dir) - tw; r->sang_dir = v < 0 ? 0 : v; break; }
        }
        regua_desenhar(j, r);
        return REGUA_ARRASTA;
    }
    if (e->tipo == EV_SOLTOU && r->arrastando) {
        r->marcador = r->arrastando;
        r->arrastando = 0;
        return REGUA_MUDOU;
    }
    return REGUA_NADA;
}

/* ------------------------------------------------------------ barra de estado */

void barra_estado_iniciar(barra_estado_t *b, int x, int y, int l, int n, const int *larg)
{
    memset(b, 0, sizeof *b);
    b->x = x; b->y = y; b->l = l; b->a = 22;
    b->n = n > BARRA_ESTADO_MAX ? BARRA_ESTADO_MAX : n;
    for (int i = 0; i < b->n; i++) b->larg[i] = larg ? larg[i] : 100;
}

void barra_estado_texto(barra_estado_t *b, int i, const char *t)
{
    if (i >= 0 && i < b->n) copia_texto(b->texto[i], t, sizeof b->texto[i]);
}

void barra_estado_desenhar(jan_t *j, barra_estado_t *b)
{
    g_retangulo(j, b->x, b->y, b->l, b->a, W98_FACE);
    int x = b->x + 2;
    for (int i = 0; i < b->n; i++) {
        int l = i == b->n - 1 ? b->x + b->l - 2 - x : b->larg[i];
        if (l < 4) break;
        g_retangulo(j, x, b->y + 2, l, b->a - 3, W98_FACE);
        g_relevo(j, x, b->y + 2, l, b->a - 3, true);
        g_retangulo(j, x, b->y + 2, l, 1, W98_SOMBRA); g_retangulo(j, x, b->y + 2, 1, b->a - 3, W98_SOMBRA);
        g_retangulo(j, x, b->y + b->a - 2, l, 1, W98_LUZ); g_retangulo(j, x + l - 1, b->y + 2, 1, b->a - 3, W98_LUZ);
        g_texto_n(j, x + 4, b->y + 3, b->texto[i], (l - 6) / LETRA_L, W98_NEGRO);
        GR_REG(j, "seccion", NULL, x, b->y + 2, l, b->a - 3, b->texto[i], l - 6, 0);
        x += l + 2;
    }
    jan_mostrar_area(j, b->x, b->y, b->l, b->a);
    GR_REG(j, "barra_estado", b, b->x, b->y, b->l, b->a, "", 0, 0);
}
