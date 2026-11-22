// SPDX-License-Identifier: GPL-3.0-only
/* gui.c - kit gráfico de libsagui (H22): dibujo y componentes en el anillo 3.
 *
 * El programa dibuja en los píxeles de la propia ventana (memoria que el kernel
 * comparte con él) y llama a jan_mostrar para que el kernel la muestre. Los
 * componentes no guardan nada oculto: cada uno es una estructura del
 * programa, y el programa decide cuándo dibujar y a quién enviar cada
 * evento. Es simple de entender y de probar. */
#include "gui.h"
#include "sonidos.h"

extern const unsigned char fonte_sagui[256][16];

/* ------------------------------------------------------------ ventana y dibujo */

jan_t *jan_abrir(const char *titulo, int largura, int altura)
{
    jan_t *j = aloca(sizeof(jan_t));
    if (!j) return NULL;
    esq_asegurar();   /* D428: la ventana nace con el esquema vigente */
    j->id = criar_janela(titulo, largura, altura, &j->px);
    if (j->id < 0) { libera(j); return NULL; }
    j->larg = largura;
    j->alt = altura;
    j->estilo = 0;
    j->min_l = j->min_a = 0;
    gr_janela_aberta(j, titulo);
    g_retangulo(j, 0, 0, largura, altura, COR_FUNDO);
    return j;
}

void jan_fechar(jan_t *j)
{
    if (!j) return;
    if (gr_modo) gr_janela_fechada(j);
    fechar_janela(j->id);
    libera(j);
}

void jan_mostrar(jan_t *j)
{
    atualizar_janela(j->id, NULL);
    if (__builtin_expect(gr_modo, 0)) gr_analizar(j, true);
}

void jan_mostrar_area(jan_t *j, int x, int y, int l, int a)
{
    retangulo_t r = { x, y, l, a };
    atualizar_janela(j->id, &r);
    if (__builtin_expect(gr_modo, 0)) gr_analizar(j, false);
}

/* ------------------------------------------------------------ tamaño variable (D044) */

int jan_estilo(jan_t *j, unsigned estilo, int min_l, int min_a)
{
    j->estilo = estilo & ~(unsigned)JAN_TELA_CHEIA;
    j->min_l = min_l;
    j->min_a = min_a;
    return estilo_janela(j->id, estilo, min_l, min_a);
}

bool jan_redimensionar(jan_t *j, const evento_t *e)
{
    if (e->tipo != EV_REDIMENSIONAR) return false;
    uint32_t *px;
    if (redimensionar_janela(j->id, e->x, e->y, &px) < 0) return false;
    j->px = px;
    j->larg = e->x;
    j->alt = e->y;
    return true;
}

bool jan_cursor_visivel(jan_t *j, bool visivel)
{
    if (visivel) j->estilo &= ~(unsigned)JAN_SEM_CURSOR;
    else j->estilo |= JAN_SEM_CURSOR;
    return cursor_janela(j->id, visivel) == 0;
}

bool jan_tela_cheia(jan_t *j, bool ligar)
{
    unsigned e = j->estilo | (ligar ? JAN_TELA_CHEIA | JAN_TELA_CHEIA_OK : 0);
    return estilo_janela(j->id, e, j->min_l, j->min_a) == 0;
}

void g_retangulo(jan_t *j, int x, int y, int l, int a, uint32_t cor)
{
    if (__builtin_expect(gr_modo, 0)) gr_pintado(j, x, y, l, a);
    int x1 = x + l, y1 = y + a;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > j->larg) x1 = j->larg;
    if (y1 > j->alt) y1 = j->alt;
    if (x >= x1) return;
    for (int yy = y; yy < y1; yy++)
        preencher32(j->px + yy * j->larg + x, cor, (size_t)(x1 - x));   /* rep stosd o SSE2 (D042) */
}

void g_moldura(jan_t *j, int x, int y, int l, int a, uint32_t cor)
{
    g_retangulo(j, x, y, l, 1, cor);
    g_retangulo(j, x, y + a - 1, l, 1, cor);
    g_retangulo(j, x, y, 1, a, cor);
    g_retangulo(j, x + l - 1, y, 1, a, cor);
}

static inline void ponto(jan_t *j, int x, int y, uint32_t cor)
{
    if ((unsigned)x < (unsigned)j->larg && (unsigned)y < (unsigned)j->alt)
        j->px[y * j->larg + x] = cor;
}

/* Líneas (D249). Horizontales: una fila rellenada de una vez; verticales: un paso de columna
 * sin cálculos por píxel; diagonales: Bresenham entero (sin divisiones), con el bucle sin
 * comprobaciones cuando la caja de la línea cabe entera en la ventana. Mismos píxeles que antes. */
void g_linha(jan_t *j, int x0, int y0, int x1, int y1, uint32_t cor)
{
    int W = j->larg, H = j->alt;
    int xmin = x0 < x1 ? x0 : x1, xmax = x0 < x1 ? x1 : x0;
    int ymin = y0 < y1 ? y0 : y1, ymax = y0 < y1 ? y1 : y0;
    if (xmax < 0 || ymax < 0 || xmin >= W || ymin >= H) return;
    if (y0 == y1) {                         /* horizontal */
        if (xmin < 0) xmin = 0;
        if (xmax >= W) xmax = W - 1;
        preencher32(j->px + y0 * W + xmin, cor, (size_t)(xmax - xmin + 1));
        return;
    }
    if (x0 == x1) {                         /* vertical */
        if (ymin < 0) ymin = 0;
        if (ymax >= H) ymax = H - 1;
        uint32_t *p = j->px + ymin * W + x0;
        for (int n = ymax - ymin + 1; n > 0; n--, p += W) *p = cor;
        return;
    }
    int dx = xmax - xmin, dy = ymax - ymin, sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int passos = dx > dy ? dx : dy, acum = 0;
    bool dentro = xmin >= 0 && ymin >= 0 && xmax < W && ymax < H;
    if (dentro) {
        uint32_t *p = j->px + y0 * W + x0;
        int pa = sy * W;
        if (dx >= dy) {
            for (int k = 0; k <= passos; k++, p += sx) {
                *p = cor;
                acum += dy;
                if (2 * acum >= dx) { p += pa; acum -= dx; }
            }
        } else {
            for (int k = 0; k <= passos; k++, p += pa) {
                *p = cor;
                acum += dx;
                if (2 * acum >= dy) { p += sx; acum -= dy; }
            }
        }
        return;
    }
    for (int k = 0; k <= passos; k++) {
        ponto(j, x0, y0, cor);
        if (dx >= dy) { x0 += sx; acum += dy; if (2 * acum >= dx) { y0 += sy; acum -= dx; } }
        else          { y0 += sy; acum += dx; if (2 * acum >= dy) { x0 += sx; acum -= dy; } }
    }
}

void g_copiar_area(jan_t *j, int sx, int sy, int l, int a, int dx, int dy)
{
    /* Recorta el destino y el origen; al recortar a la izquierda/arriba, el origen va junto. */
    if (dx < 0) { l += dx; sx -= dx; dx = 0; }
    if (dy < 0) { a += dy; sy -= dy; dy = 0; }
    if (sx < 0) { l += sx; dx -= sx; sx = 0; }
    if (sy < 0) { a += sy; dy -= sy; sy = 0; }
    if (dx + l > j->larg) l = j->larg - dx;
    if (sx + l > j->larg) l = j->larg - sx;
    if (dy + a > j->alt) a = j->alt - dy;
    if (sy + a > j->alt) a = j->alt - sy;
    if (l <= 0 || a <= 0) return;
    /* Si el destino queda debajo del origen, copia de abajo arriba (como memmove, por líneas). */
    if (dy > sy) {
        for (int y = a - 1; y >= 0; y--)
            memmove(j->px + (dy + y) * j->larg + dx, j->px + (sy + y) * j->larg + sx, (size_t)l * 4);
    } else {
        for (int y = 0; y < a; y++)
            memmove(j->px + (dy + y) * j->larg + dx, j->px + (sy + y) * j->larg + sx, (size_t)l * 4);
    }
}

void g_texto_n(jan_t *j, int x, int y, const char *t, int n, uint32_t cor)
{
    if (__builtin_expect(gr_modo, 0)) gr_texto_fuera(j, x, y, t, n);
    for (int k = 0; k < n && t[k]; k++, x += LETRA_L) {
        if (x >= j->larg) break;
        const unsigned char *g = fonte_sagui[(uint8_t)t[k]];
        for (int lin = 0; lin < 16; lin++)
            for (int col = 0; col < 8; col++)
                if (g[lin] & (0x80 >> col)) ponto(j, x + col, y + lin, cor);
    }
}

void g_texto(jan_t *j, int x, int y, const char *t, uint32_t cor)
{
    g_texto_n(j, x, y, t, 1 << 30, cor);
}

int g_largura_texto(const char *t) { return (int)tam_texto(t) * LETRA_L; }

void g_desenho(jan_t *j, int x, int y, const char *const *linhas, int l, int a,
               uint32_t (*paleta)(char), int escala)
{
    for (int lin = 0; lin < a; lin++)
        for (int col = 0; col < l; col++) {
            uint32_t c = paleta(linhas[lin][col]);
            if (c != 0xFF000000)
                g_retangulo(j, x + col * escala, y + lin * escala, escala, escala, c);
        }
}

/* Marco en relieve: claro arriba y a la izquierda, oscuro abajo y a la derecha
 * (o al revés, para parecer hundido). */
void g_relevo(jan_t *j, int x, int y, int l, int a, bool afundado)
{
    uint32_t claro = W98_LUZ, escuro = COR_BORDA;
    if (afundado) { uint32_t t = claro; claro = escuro; escuro = t; }
    g_retangulo(j, x, y, l, 1, claro);
    g_retangulo(j, x, y, 1, a, claro);
    g_retangulo(j, x, y + a - 1, l, 1, escuro);
    g_retangulo(j, x + l - 1, y, 1, a, escuro);
}

static bool dentro(int px, int py, int x, int y, int l, int a)
{
    return px >= x && py >= y && px < x + l && py < y + a;
}

/* ------------------------------------------------------------ rótulo */

void rotulo(jan_t *j, int x, int y, const char *texto, uint32_t cor)
{
    g_texto(j, x, y, texto, cor);
    GR_REG(j, "rotulo", NULL, x, y, g_largura_texto(texto), LETRA_A, texto, j->larg - x, 0);
}

/* ------------------------------------------------------------ botón */

void botao_desenhar(jan_t *j, botao_t *b)
{
    g_retangulo(j, b->x, b->y, b->l, b->a, b->apertado ? COR_BOTAO_APERTADO : COR_BOTAO);
    g_relevo(j, b->x, b->y, b->l, b->a, b->apertado);
    g_moldura(j, b->x - 1, b->y - 1, b->l + 2, b->a + 2, sagui_esq.borda_forte);
    const char *rot = idioma_traducir_sl(b->texto);   /* D313 */
    int tx = b->x + (b->l - g_largura_texto(rot)) / 2 + (b->apertado ? 1 : 0);
    int ty = b->y + (b->a - LETRA_A) / 2 + (b->apertado ? 1 : 0);
    g_texto(j, tx, ty, rot, COR_TEXTO);
    jan_mostrar_area(j, b->x - 1, b->y - 1, b->l + 2, b->a + 2);
    GR_REG(j, "botao", b, b->x, b->y, b->l, b->a, b->texto, b->l - 2, GRF_CONTROL);
}

/* Clic = apretó y soltó dentro del botón (soltar fuera cancela). */
bool botao_tratar(jan_t *j, botao_t *b, const evento_t *e)
{
    if (e->tipo == EV_CLIQUE && (e->botoes & 1) && dentro(e->x, e->y, b->x, b->y, b->l, b->a)) {
        b->apertado = true;
        botao_desenhar(j, b);
        return false;
    }
    if (e->tipo == EV_SOLTOU && b->apertado) {
        b->apertado = false;
        botao_desenhar(j, b);
        return dentro(e->x, e->y, b->x, b->y, b->l, b->a);
    }
    return false;
}

/* ------------------------------------------------------------ cuadro de texto */

void caixa_definir(caixa_t *c, const char *texto)
{
    copia_texto(c->texto, texto, sizeof c->texto);
    c->cursor = (int)tam_texto(c->texto);
}

void caixa_desenhar(jan_t *j, caixa_t *c)
{
    int a = LETRA_A + 8;
    g_retangulo(j, c->x, c->y, c->l, a, COR_CAMPO);
    g_moldura(j, c->x, c->y, c->l, a, c->foco ? COR_DESTAQUE : COR_BORDA);
    if (c->foco) g_moldura(j, c->x + 1, c->y + 1, c->l - 2, a - 2, COR_DESTAQUE);
    int cabem = (c->l - 10) / LETRA_L;
    if (cabem < 1) cabem = 1;
    int nt = (int)tam_texto(c->texto);
    if (c->cursor > nt) c->cursor = nt;   /* D217: un cursor fuera del texto no puede dibujar fuera de la caja */
    if (c->cursor < 0) c->cursor = 0;
    int ini = c->cursor > cabem ? c->cursor - cabem : 0;
    if (c->oculta) {
        /* D145: el texto escondido se ve como una fila de asteriscos (la clave
         * de API no se enseña, pero se ve cuánto ocupa). */
        char masc[sizeof c->texto + 64];
        int n = (int)tam_texto(c->texto);
        int total = n > (int)sizeof masc - 1 ? (int)sizeof masc - 1 : n;
        for (int i = 0; i < total; i++) masc[i] = '*';
        if (ini > total) ini = total;
        int quantos = cabem;
        if (ini + quantos > total) quantos = total - ini;
        if (quantos > 0) g_texto_n(j, c->x + 5, c->y + 4, masc + ini, quantos, COR_TEXTO);
    } else {
        g_texto_n(j, c->x + 5, c->y + 4, c->texto + ini, cabem, COR_TEXTO);
    }
    if (c->foco)
        g_retangulo(j, c->x + 5 + (c->cursor - ini) * LETRA_L, c->y + 4, 2, LETRA_A, COR_TEXTO);
    jan_mostrar_area(j, c->x, c->y, c->l, a);
    GR_REG(j, "caixa", c, c->x, c->y, c->l, a, "", 0, GRF_CONTROL);
}

bool caixa_tratar(jan_t *j, caixa_t *c, const evento_t *e)
{
    if (e->tipo == EV_CLIQUE) {
        bool estava = c->foco;
        c->foco = dentro(e->x, e->y, c->x, c->y, c->l, LETRA_A + 8);
        if (c->foco) {
            /* D217: con el texto desplazado (cursor mas alla de lo que cabe) la columna pulsada
             * cuenta desde el primer caracter visible, el mismo que usa caixa_desenhar. */
            int cabem = (c->l - 10) / LETRA_L;
            if (cabem < 1) cabem = 1;
            int ini = c->cursor > cabem ? c->cursor - cabem : 0;
            int col = ini + (e->x - c->x - 5 + LETRA_L / 2) / LETRA_L;
            int n = (int)tam_texto(c->texto);
            c->cursor = col < 0 ? 0 : col > n ? n : col;
        }
        if (c->foco || estava) (c->desenhar ? c->desenhar : caixa_desenhar)(j, c);
        return false;
    }
    if (e->tipo != EV_TECLA || !c->foco) return false;
    int n = (int)tam_texto(c->texto);
    unsigned t = e->tecla;
    if (t == '\n') return true;
    if (t == '\b') {
        if (c->cursor > 0) { memmove(c->texto + c->cursor - 1, c->texto + c->cursor, (size_t)(n - c->cursor + 1)); c->cursor--; }
    } else if (t == TECLA_DELETE) {
        if (c->cursor < n) memmove(c->texto + c->cursor, c->texto + c->cursor + 1, (size_t)(n - c->cursor));
    } else if (t == TECLA_ESQUERDA) { if (c->cursor > 0) c->cursor--; }
    else if (t == TECLA_DIREITA) { if (c->cursor < n) c->cursor++; }
    else if (t == TECLA_HOME) c->cursor = 0;
    else if (t == TECLA_END) c->cursor = n;
    else if (t == 22) {   /* Ctrl+V (D074): solo la primera línea de lo que se copió */
        char b[sizeof c->texto];
        colar_texto(b, sizeof b);
        int k = 0;
        while (b[k] && b[k] != '\n' && b[k] != '\r' && n + k < (int)sizeof c->texto - 1) k++;
        memmove(c->texto + c->cursor + k, c->texto + c->cursor, (size_t)(n - c->cursor + 1));
        memcpy(c->texto + c->cursor, b, (size_t)k);
        c->cursor += k;
    }
    else if (t >= 0x20 && t < 0x100 && t != 0x7F && n < (int)sizeof c->texto - 1) {
        memmove(c->texto + c->cursor + 1, c->texto + c->cursor, (size_t)(n - c->cursor + 1));
        c->texto[c->cursor++] = (char)t;
    } else return false;
    (c->desenhar ? c->desenhar : caixa_desenhar)(j, c);
    return false;
}

/* ------------------------------------------------------------ área de texto */

#define BARRA_ROLAGEM 12

int area_linhas_visiveis(area_t *a) { return (a->a - 8) / LETRA_A; }

bool area_iniciar(area_t *a, int x, int y, int l, int al)
{
    memset(a, 0, sizeof *a);
    a->x = x; a->y = y; a->l = l; a->a = al;
    a->cap = 4096;
    a->buf = aloca((size_t)a->cap);
    if (!a->buf) return false;
    a->buf[0] = 0;
    a->ancora = -1;
    return true;
}

static bool garantir(area_t *a, int precisa)
{
    if (precisa < a->cap) return true;
    int nova = a->cap;
    while (nova <= precisa) nova *= 2;
    char *b = realoca(a->buf, (size_t)nova);
    if (!b) return false;
    a->buf = b;
    a->cap = nova;
    return true;
}

void area_definir(area_t *a, const char *texto, int n)
{
    if (!garantir(a, n + 1)) n = a->cap - 1;
    memcpy(a->buf, texto, (size_t)n);
    a->buf[n] = 0;
    a->tam = n;
    a->cursor = 0;
    a->topo = 0;
    a->ancora = -1;
    a->modificado = false;
}

/* Selección (D074): de "ancora" hasta el cursor, en cualquier orden. */
static bool selecao(area_t *a, int *s0, int *s1)
{
    if (a->ancora < 0 || a->ancora == a->cursor) return false;
    *s0 = a->ancora < a->cursor ? a->ancora : a->cursor;
    *s1 = a->ancora < a->cursor ? a->cursor : a->ancora;
    if (*s1 > a->tam) *s1 = a->tam;
    return *s0 < *s1;
}

/* Línea y columna de una posición del buffer. */
static void linha_coluna(area_t *a, int pos, int *lin, int *col)
{
    int l = 0, ini = 0;
    for (int i = 0; i < pos; i++)
        if (a->buf[i] == '\n') { l++; ini = i + 1; }
    *lin = l;
    *col = pos - ini;
}

static int inicio_da_linha(area_t *a, int lin)
{
    int pos = 0;
    for (int l = 0; l < lin && pos < a->tam; pos++)
        if (a->buf[pos] == '\n') l++;
    return pos;
}

static int tamanho_da_linha(area_t *a, int ini)
{
    int n = 0;
    while (ini + n < a->tam && a->buf[ini + n] != '\n') n++;
    return n;
}

static int total_linhas(area_t *a)
{
    int n = 1;
    for (int i = 0; i < a->tam; i++) if (a->buf[i] == '\n') n++;
    return n;
}

static void cursor_visivel(area_t *a)
{
    int lin, col, vis = area_linhas_visiveis(a);
    linha_coluna(a, a->cursor, &lin, &col);
    if (lin < a->topo) a->topo = lin;
    if (lin >= a->topo + vis) a->topo = lin - vis + 1;
}

void area_mudar_tamanho(area_t *a, int l, int al)
{
    a->l = l;
    a->a = al;
    cursor_visivel(a);
}

void area_desenhar(jan_t *j, area_t *a)
{
    int texto_l = a->l - BARRA_ROLAGEM;
    g_retangulo(j, a->x, a->y, a->l, a->a, COR_CAMPO);
    g_moldura(j, a->x, a->y, a->l, a->a, a->foco ? COR_DESTAQUE : COR_BORDA);
    int vis = area_linhas_visiveis(a), cabem = (texto_l - 10) / LETRA_L;
    int lin_cur, col_cur;
    linha_coluna(a, a->cursor, &lin_cur, &col_cur);
    int pos = inicio_da_linha(a, a->topo), s0 = 0, s1 = 0;
    bool tem_sel = selecao(a, &s0, &s1);
    for (int k = 0; k < vis && pos <= a->tam; k++) {
        int n = tamanho_da_linha(a, pos);
        if (tem_sel && s0 <= pos + n && s1 > pos) {   /* fondo de la selección (el fin de línea seleccionado vale media letra) */
            int c0 = s0 > pos ? s0 - pos : 0, c1 = s1 - pos > n ? n + 1 : s1 - pos;
            if (c1 > cabem) c1 = cabem;
            if (c1 > c0) g_retangulo(j, a->x + 5 + c0 * LETRA_L, a->y + 4 + k * LETRA_A, (c1 - c0) * LETRA_L, LETRA_A, COR_SEL_SUAVE);
        }
        g_texto_n(j, a->x + 5, a->y + 4 + k * LETRA_A, a->buf + pos, n < cabem ? n : cabem, COR_TEXTO);
        if (a->foco && !a->somente_leitura && a->topo + k == lin_cur && col_cur <= cabem)
            g_retangulo(j, a->x + 5 + col_cur * LETRA_L, a->y + 4 + k * LETRA_A, 2, LETRA_A, COR_TEXTO);
        pos += n + 1;
        if (pos > a->tam) break;
    }
    /* Barra de desplazamiento: el "pulgar" muestra qué parte del texto está a la vista. */
    int bx = a->x + a->l - BARRA_ROLAGEM, total = total_linhas(a);
    g_retangulo(j, bx, a->y + 1, BARRA_ROLAGEM - 1, a->a - 2, sagui_esq.scroll_pista);
    int trilho = a->a - 4;
    int pol_a = total <= vis ? trilho : trilho * vis / total;
    if (pol_a < 12) pol_a = 12;
    int pol_y = total <= vis ? 0 : (trilho - pol_a) * a->topo / (total - vis > 0 ? total - vis : 1);
    if (pol_a > trilho) pol_a = trilho;
    if (pol_y > trilho - pol_a) pol_y = trilho - pol_a;   /* D217: el pulgar no se sale del carril */
    if (pol_y < 0) pol_y = 0;
    g_retangulo(j, bx + 2, a->y + 2 + pol_y, BARRA_ROLAGEM - 5, pol_a, sagui_esq.scroll_polegar);
    jan_mostrar_area(j, a->x, a->y, a->l, a->a);
    GR_REG(j, "area", a, a->x, a->y, a->l, a->a, "", 0, 0);
}

static void inserir(area_t *a, const char *t, int n)
{
    if (!garantir(a, a->tam + n + 1)) return;
    memmove(a->buf + a->cursor + n, a->buf + a->cursor, (size_t)(a->tam - a->cursor + 1));
    memcpy(a->buf + a->cursor, t, (size_t)n);
    a->tam += n;
    a->cursor += n;
    a->modificado = true;
}

static void remover(area_t *a, int pos)
{
    if (pos < 0 || pos >= a->tam) return;
    memmove(a->buf + pos, a->buf + pos + 1, (size_t)(a->tam - pos));
    a->tam--;
    a->modificado = true;
}

/* Borra la selección (si la hay) y deja el cursor en su lugar. */
static bool apagar_selecao(area_t *a)
{
    int s0, s1;
    if (!selecao(a, &s0, &s1)) { a->ancora = -1; return false; }
    memmove(a->buf + s0, a->buf + s1, (size_t)(a->tam - s1 + 1));
    a->tam -= s1 - s0;
    a->cursor = s0;
    a->ancora = -1;
    a->modificado = true;
    return true;
}

bool area_copiar(area_t *a)
{
    int s0, s1;
    if (!selecao(a, &s0, &s1)) return false;
    copiar_texto(a->buf + s0, (unsigned)(s1 - s0));
    registrar("gui: area copiou %d bytes", s1 - s0);
    return true;
}

bool area_cortar(jan_t *j, area_t *a)
{
    if (a->somente_leitura || !area_copiar(a)) return false;
    apagar_selecao(a);
    cursor_visivel(a);
    area_desenhar(j, a);
    return true;
}

bool area_colar(jan_t *j, area_t *a)
{
    if (a->somente_leitura) return false;
    int n = colar_texto(NULL, 0);
    if (n <= 0) return false;
    char *b = aloca((size_t)n + 1);
    if (!b) return false;
    n = colar_texto(b, (unsigned)n + 1);
    int k = 0;
    for (int i = 0; i < n; i++) if (b[i] != '\r') b[k++] = b[i];   /* fines de línea de fuera */
    apagar_selecao(a);
    inserir(a, b, k);
    libera(b);
    registrar("gui: area colou %d bytes", k);
    cursor_visivel(a);
    area_desenhar(j, a);
    return true;
}

/* Posición del buffer debajo de (x, y) de la ventana. */
static int posicao_em(area_t *a, int x, int y)
{
    int lin = a->topo + (y - a->y - 4) / LETRA_A, col = (x - a->x - 5 + LETRA_L / 2) / LETRA_L;
    int total = total_linhas(a);
    if (y < a->y + 4) lin = a->topo - 1;
    if (lin >= total) lin = total - 1;
    if (lin < 0) lin = 0;
    int ini = inicio_da_linha(a, lin), n = tamanho_da_linha(a, ini);
    return ini + (col < 0 ? 0 : col > n ? n : col);
}

static void mover_vertical(area_t *a, int delta)
{
    int lin, col;
    linha_coluna(a, a->cursor, &lin, &col);
    int nova = lin + delta, total = total_linhas(a);
    if (nova < 0) nova = 0;
    if (nova >= total) nova = total - 1;
    int ini = inicio_da_linha(a, nova), n = tamanho_da_linha(a, ini);
    a->cursor = ini + (col < n ? col : n);
}

/* Rueda del ratón (D048): RODA_LINHAS líneas por muesca, sin mover el
 * cursor ni el foco, solo si el cursor del ratón está sobre el área. */
static void area_rodar(jan_t *j, area_t *a, int entalhes)
{
    int vis = area_linhas_visiveis(a), total = total_linhas(a), antes = a->topo;
    a->topo += entalhes * RODA_LINHAS;
    if (a->topo > total - vis) a->topo = total - vis;
    if (a->topo < 0) a->topo = 0;
    if (a->topo == antes) return;
    area_desenhar(j, a);
    registrar("gui: area rolada pela roda (%d) para a linha %d de %d", entalhes, a->topo, total);
}

bool area_tratar(jan_t *j, area_t *a, const evento_t *e)
{
    bool mudou = false;
    if (e->tipo == EV_RODA) {
        if (dentro(e->x, e->y, a->x, a->y, a->l, a->a)) area_rodar(j, a, (int)e->cliques);
        return false;
    }
    if (e->tipo == EV_CLIQUE) {
        bool estava = a->foco;
        a->foco = dentro(e->x, e->y, a->x, a->y, a->l, a->a);
        if (a->foco) {
            if (e->x >= a->x + a->l - BARRA_ROLAGEM) {
                /* Clic en la barra de desplazamiento: una página arriba o abajo. */
                int vis = area_linhas_visiveis(a), total = total_linhas(a);
                a->topo += e->y < a->y + a->a / 2 ? -vis : vis;
                if (a->topo > total - vis) a->topo = total - vis;
                if (a->topo < 0) a->topo = 0;
            } else if (e->botoes & 1) {
                /* Clic: cursor; con Shift, extiende la selección; arrastrar selecciona (D074). */
                int p = posicao_em(a, e->x, e->y);
                if ((e->modificadores & 1) && a->ancora < 0) a->ancora = a->cursor;
                else if (!(e->modificadores & 1)) a->ancora = p;
                a->cursor = p;
                a->arrastando = true;
            }
        }
        if (a->foco || estava) area_desenhar(j, a);
        return false;
    }
    if (e->tipo == EV_MOVER && a->arrastando && (e->botoes & 1)) {
        int p = posicao_em(a, e->x, e->y);
        if (p != a->cursor) { a->cursor = p; cursor_visivel(a); area_desenhar(j, a); }
        return false;
    }
    if (e->tipo == EV_SOLTOU && a->arrastando) {
        a->arrastando = false;
        if (a->ancora == a->cursor) a->ancora = -1;
        return false;
    }
    if (e->tipo != EV_TECLA || !a->foco) return false;
    unsigned t = e->tecla;
    int vis = area_linhas_visiveis(a);
    bool mover = t == TECLA_ESQUERDA || t == TECLA_DIREITA || t == TECLA_CIMA || t == TECLA_BAIXO ||
                 t == TECLA_PGUP || t == TECLA_PGDN || t == TECLA_HOME || t == TECLA_END;
    if (mover) {   /* Shift + movimiento extiende la selección; sin Shift, se pierde */
        if (e->modificadores & 1) { if (a->ancora < 0) a->ancora = a->cursor; }
        else a->ancora = -1;
    }
    if (t == 1) { a->ancora = 0; a->cursor = a->tam; cursor_visivel(a); area_desenhar(j, a); return false; }   /* Ctrl+A */
    if (t == 3) { area_copiar(a); return false; }                                                            /* Ctrl+C */
    if (t == 24) return area_cortar(j, a);                                                                   /* Ctrl+X */
    if (t == 22) return area_colar(j, a);                                                                    /* Ctrl+V */
    if (!mover && !a->somente_leitura && (t == '\b' || t == TECLA_DELETE || t == '\n' || t == '\t' ||
                                          (t >= 0x20 && t < 0x100 && t != 0x7F))) {
        if (apagar_selecao(a) && (t == '\b' || t == TECLA_DELETE)) {   /* borrar la selección ya fue la tecla */
            cursor_visivel(a);
            area_desenhar(j, a);
            return true;
        }
    }
    if (t == TECLA_ESQUERDA) { if (a->cursor > 0) a->cursor--; }
    else if (t == TECLA_DIREITA) { if (a->cursor < a->tam) a->cursor++; }
    else if (t == TECLA_CIMA) mover_vertical(a, -1);
    else if (t == TECLA_BAIXO) mover_vertical(a, 1);
    else if (t == TECLA_PGUP) { mover_vertical(a, -vis); a->topo -= vis; if (a->topo < 0) a->topo = 0; }
    else if (t == TECLA_PGDN) { mover_vertical(a, vis); a->topo += vis; }
    else if (t == TECLA_HOME) { int l, c; linha_coluna(a, a->cursor, &l, &c); a->cursor -= c; }
    else if (t == TECLA_END) { int l, c; linha_coluna(a, a->cursor, &l, &c); a->cursor += tamanho_da_linha(a, a->cursor - c) - c; }
    else if (a->somente_leitura) return false;
    else if (t == '\b') { if (a->cursor > 0) { remover(a, a->cursor - 1); a->cursor--; mudou = true; } }
    else if (t == TECLA_DELETE) { if (a->cursor < a->tam) { remover(a, a->cursor); mudou = true; } }
    else if (t == '\n') { inserir(a, "\n", 1); mudou = true; }
    else if (t == '\t') { inserir(a, "    ", 4); mudou = true; }
    else if (t >= 0x20 && t < 0x100 && t != 0x7F) { char c = (char)t; inserir(a, &c, 1); mudou = true; }
    else return false;
    int total = total_linhas(a);
    if (a->topo > total - 1) a->topo = total - 1 > 0 ? total - 1 : 0;
    cursor_visivel(a);
    area_desenhar(j, a);
    return mudou;
}

/* ------------------------------------------------------------ lista */

/* Ítems que caben en la lista (D217: nunca menos de 1, así no hay divisiones por cero ni topes negativos). */
static int lista_visiveis(const lista_t *l)
{
    int vis = (l->a - 4) / LISTA_ALTURA_ITEM;
    return vis < 1 ? 1 : vis;
}

/* Mantiene topo dentro de [0, qtd - vis]. */
static void lista_limitar_topo(lista_t *l)
{
    int max = l->qtd - lista_visiveis(l);
    if (l->topo > max) l->topo = max;
    if (l->topo < 0) l->topo = 0;
}

void lista_desenhar(jan_t *j, lista_t *l)
{
    int vis = lista_visiveis(l);
    lista_limitar_topo(l);
    g_retangulo(j, l->x, l->y, l->l, l->a, COR_CAMPO);
    g_moldura(j, l->x, l->y, l->l, l->a, COR_BORDA);
    for (int k = 0; k < vis && l->topo + k < l->qtd; k++) {
        int i = l->topo + k, y = l->y + 2 + k * LISTA_ALTURA_ITEM;
        bool sel = i == l->sel;
        if (sel) g_retangulo(j, l->x + 2, y, l->l - 4, LISTA_ALTURA_ITEM, COR_SELECAO);
        int tx = l->x + 6;
        if (l->icone) { l->icone(j, tx, y + 2, i); tx += 22; }
        int cols = (l->x + l->l - tx - 4) / LETRA_L;
        if (cols > 0) g_texto_n(j, tx, y + 2, l->itens[i], cols, sel ? COR_SEL_TEXTO : COR_TEXTO);
    }
    if (l->qtd > vis) {   /* indicador simple de que hay más ítems */
        int total = l->qtd, trilho = l->a - 4;
        int pol_a = trilho * vis / total;
        if (pol_a < 8) pol_a = 8;
        if (pol_a > trilho) pol_a = trilho;
        int pol_y = (trilho - pol_a) * l->topo / (total - vis);   /* total > vis: sin división por cero */
        g_retangulo(j, l->x + l->l - 6, l->y + 2 + pol_y, 4, pol_a, sagui_esq.scroll_polegar);
    }
    jan_mostrar_area(j, l->x, l->y, l->l, l->a);
    GR_REG(j, "lista", l, l->x, l->y, l->l, l->a, "", 0, 0);
}

/* Redibuja con el dibujo propio de la lista si lo tiene (D217). */
static void lista_pintar(jan_t *j, lista_t *l)
{
    if (l->desenhar) l->desenhar(j, l);
    else lista_desenhar(j, l);
}

static void lista_garantir_visivel(lista_t *l)
{
    int vis = lista_visiveis(l);
    if (l->sel < l->topo) l->topo = l->sel;
    if (l->sel >= l->topo + vis) l->topo = l->sel - vis + 1;
    lista_limitar_topo(l);
}

int lista_tratar(jan_t *j, lista_t *l, const evento_t *e)
{
    if (e->tipo == EV_RODA) {   /* rueda (D048): desplaza sin cambiar la selección */
        if (!dentro(e->x, e->y, l->x, l->y, l->l, l->a)) return LISTA_NADA;
        int antes = l->topo;
        l->topo += (int)e->cliques * RODA_LINHAS;
        lista_limitar_topo(l);
        if (l->topo != antes) {
            lista_pintar(j, l);
            registrar("gui: lista rolada pela roda (%d) para o item %d de %d", (int)e->cliques, l->topo, l->qtd);
        }
        return LISTA_NADA;
    }
    if (e->tipo == EV_CLIQUE && dentro(e->x, e->y, l->x, l->y, l->l, l->a)) {
        int k = (e->y - l->y - 2) / LISTA_ALTURA_ITEM;
        int i = l->topo + k;
        if (e->y - l->y - 2 < 0 || k >= lista_visiveis(l) || i < 0 || i >= l->qtd) return LISTA_NADA;
        l->sel = i;
        lista_pintar(j, l);
        return e->cliques >= 2 ? LISTA_ATIVOU : LISTA_SELECIONOU;
    }
    if (e->tipo != EV_TECLA || l->qtd == 0) return LISTA_NADA;
    if (e->tecla == '\n' && l->sel >= 0) return LISTA_ATIVOU;
    int antes = l->sel, vis = lista_visiveis(l);
    if (e->tecla == TECLA_BAIXO && l->sel < l->qtd - 1) l->sel++;
    else if (e->tecla == TECLA_CIMA && l->sel > 0) l->sel--;
    else if (e->tecla == TECLA_HOME) l->sel = 0;
    else if (e->tecla == TECLA_END) l->sel = l->qtd - 1;
    else if (e->tecla == TECLA_PGDN) { l->sel += vis; if (l->sel > l->qtd - 1) l->sel = l->qtd - 1; }
    else if (e->tecla == TECLA_PGUP) { l->sel -= vis; if (l->sel < 0) l->sel = 0; }
    if (l->sel == antes) return LISTA_NADA;
    lista_garantir_visivel(l);
    lista_pintar(j, l);
    return LISTA_SELECIONOU;
}

/* ------------------------------------------------------------ diálogos */

/* Issue #16: los diálogos parten el texto por palabras según el ancho real (la fuente es de 8 px
 * por carácter) y la ventana crece, hasta DLG_MAX_L, para que nada se salga por la derecha. */
#define DLG_MIN_L 380
#define DLG_MAX_L 640

/* Longitud de la línea que empieza en t cuando caben cols letras: corta en '\n' o en el último
 * espacio que quepa (si una palabra es más larga que la línea, en seco). *salto = cuántos
 * caracteres se saltan después (el '\n' o los espacios del corte). */
static int linha_ate_quebra(const char *t, int cols, int *salto)
{
    int n = 0;
    while (t[n] && t[n] != '\n' && n < cols) n++;
    *salto = 0;
    if (!t[n]) return n;
    if (t[n] == '\n') { *salto = 1; return n; }
    int k = n;   /* se llenó la línea: buscar el último espacio */
    while (k > 0 && t[k] != ' ') k--;
    if (k == 0) return n;   /* palabra enorme: corte en seco */
    n = k;
    while (n > 0 && t[n - 1] == ' ') n--;
    *salto = k - n;
    while (t[n + *salto] == ' ') (*salto)++;
    return n;
}

static int linhas_quebradas(const char *t, int cols)
{
    int n = 0;
    while (*t) {
        int salto, c = linha_ate_quebra(t, cols, &salto);
        n++;
        t += c + salto;
        if (!c && !salto) break;
    }
    return n ? n : 1;
}

static void texto_multilinha(jan_t *j, int x, int y, const char *t, int cols)
{
    while (*t) {
        int salto, n = linha_ate_quebra(t, cols, &salto);
        g_texto_n(j, x, y, t, n, COR_TEXTO);
        y += LETRA_A + 2;
        t += n + salto;
        if (!n && !salto) break;
    }
}

/* Ancho que pide el texto: el de su párrafo más largo, entre DLG_MIN_L y DLG_MAX_L. */
static int largura_dialogo(const char *t)
{
    int max = 0, n = 0;
    for (;; t++) {
        if (*t == '\n' || !*t) { if (n > max) max = n; n = 0; if (!*t) break; } else n++;
    }
    int l = max * LETRA_L + 32;
    return l < DLG_MIN_L ? DLG_MIN_L : l > DLG_MAX_L ? DLG_MAX_L : l;
}

static int dialogo(const char *titulo, const char *texto, const char *b1, const char *b2, caixa_t *caixa)
{
    titulo = idioma_traducir_sl(titulo);   /* D313: los textos que llegan literales de los programas */
    texto = idioma_traducir_sl(texto);
    int larg = largura_dialogo(texto);
    int cols = (larg - 32) / LETRA_L;
    int alt = 70 + linhas_quebradas(texto, cols) * (LETRA_A + 2) + (caixa ? 34 : 0);
    jan_t *j = jan_abrir(titulo, larg, alt);
    if (!j) return 0;
    jan_estilo(j, JAN_DIALOGO, 0, 0);   /* fuera de la barra de tareas y sin minimizar (D065) */
    texto_multilinha(j, 16, 16, texto, cols);
    botao_t bs[2] = { { larg - 16 - 90, alt - 40, 90, 26, b1, false }, { larg - 16 - 190, alt - 40, 90, 26, b2, false } };
    int qtd = b2 ? 2 : 1;
    if (caixa) {
        caixa->x = 16; caixa->y = alt - 40 - 36; caixa->l = larg - 32; caixa->foco = true;
        caixa_desenhar(j, caixa);
    }
    for (int i = 0; i < qtd; i++) botao_desenhar(j, &bs[i]);
    jan_mostrar(j);
    int resposta = 0;
    evento_t e;
    while (!resposta) {
        if (proximo_evento(j->id, &e, true) != 1) continue;
        if (e.tipo == EV_FECHAR) { resposta = 2; break; }
        if (e.tipo == EV_TECLA && e.tecla == TECLA_ESC) { resposta = qtd == 2 ? 2 : 1; break; }
        if (caixa && caixa_tratar(j, caixa, &e)) { resposta = 1; break; }
        if (!caixa && e.tipo == EV_TECLA && e.tecla == '\n') { resposta = 1; break; }
        for (int i = 0; i < qtd; i++)
            if (botao_tratar(j, &bs[i], &e)) resposta = i + 1;
    }
    jan_fechar(j);
    return resposta;
}

bool confirmar(const char *titulo, const char *pergunta)
{
    sonido_evento("pregunta");   /* D421: suena solo si la persona lo activó en Configuración > Sonido */
    return dialogo(titulo, pergunta, _("Sí"), _("No"), NULL) == 1;
}

/* ¿El título habla de un error? (los programas no tienen un diálogo de error aparte: D421) */
static bool titulo_de_error(const char *t)
{
    for (; *t; t++)
        if ((t[0] == 'E' || t[0] == 'e') && t[1] == 'r' && t[2] == 'r' && t[3] == 'o' && t[4] == 'r') return true;
        else if ((t[0] == 'F' || t[0] == 'f') && t[1] == 'a' && t[2] == 'l' && t[3] == 'l' && t[4] == 'o') return true;
    return false;
}

void mensagem(const char *titulo, const char *texto)
{
    sonido_evento(titulo_de_error(titulo) ? "error" : "aviso");   /* D421 */
    dialogo(titulo, texto, _("OK"), NULL, NULL);
}

bool pedir_texto_oculto(const char *titulo, const char *pergunta, char *resposta, int max)
{
    caixa_t c;
    memset(&c, 0, sizeof c);
    c.oculta = true;
    caixa_definir(&c, resposta);
    bool ok = dialogo(titulo, pergunta, _("OK"), _("Cancelar"), &c) == 1;
    if (ok) copia_texto(resposta, c.texto, (size_t)max);
    memset(&c, 0, sizeof c);   /* no dejar la contraseña en la pila */
    return ok;
}

bool pedir_texto(const char *titulo, const char *pergunta, char *resposta, int max)
{
    caixa_t c;
    memset(&c, 0, sizeof c);
    caixa_definir(&c, resposta);
    if (dialogo(titulo, pergunta, _("OK"), _("Cancelar"), &c) != 1) return false;
    copia_texto(resposta, c.texto, (size_t)max);
    return true;
}
