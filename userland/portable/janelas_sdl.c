// SPDX-License-Identifier: GPL-2.0-or-later
/* janelas_sdl.c - ventanas, eventos, menús contextuales y portapapeles de la capa «libsagui sobre SDL 2» (D459).
 *
 * Implementa en el anfitrión lo que en SaguiOS son llamadas al kernel (criar_janela, proximo_evento,
 * menu_contextual, copiar_texto...). Los programas dibujan en un búfer de píxeles 0x00RRGGBB, igual que
 * en SaguiOS; aquí ese búfer se sube a una textura SDL. Los eventos salen con la misma codificación que
 * el kernel (Latin-1, TECLA_* desde 0x100, Ctrl+letra = código de control, botones 1/2/4).
 *
 * Modo de prueba: con SAGUI_ROTEIRO=archivo, los eventos salen de un guion (texto, teclas, clics, capturas)
 * en vez del teclado y el ratón; con SDL_VIDEODRIVER=dummy no hace falta pantalla. Ver docs/apps-portables.md. */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui.h"
#include "portavel.h"

extern const unsigned char fonte_sagui[256][16];   /* tipo del guion de la fuente: unsigned char */

#define JAN_MAX 16
#define FILA_MAX 512

typedef struct {
    char *texto[MENU_MAX_ITENS + 2];   /* texto de cada línea (sin marcas) */
    unsigned char marca[MENU_MAX_ITENS + 2];   /* 0 normal, '-' separador, '~' desactivada, '*' marcada, '>' submenú, '<' fin */
    char *atajo[MENU_MAX_ITENS + 2];
    int  nivel[MENU_MAX_ITENS + 2];   /* profundidad de submenú de cada línea */
    int  n;
} menu_dados_t;

typedef struct {
    int         ativa;
    int         x, y, codigo;
    menu_dados_t d;
    char        *copia;
    /* niveles abiertos: cada uno es un panel; abertos[k] = índice de la línea ">" que lo abrió (k >= 1) */
    int         niveis, abertos[8], sel[8];
    int         px[8], py[8], pl[8], pa[8];
} menu_t;

typedef struct {
    int       usada;
    SDL_Window   *w;
    SDL_Renderer *r;
    SDL_Texture  *t;
    Uint32    wid;
    uint32_t *px;
    int       larg, alt;
    uint32_t *vis;      /* lo que se ve: px con el menú encima */
    evento_t  fila[FILA_MAX];
    int       fi, ff;
    unsigned  botoes;
    unsigned  estilo;
    int       min_l, min_a;
    int       estado;   /* JAN_ESTADO_* */
    menu_t    menu;
} jan_sis_t;

static jan_sis_t J[JAN_MAX];
static int sdl_ok;
static int escala = 1;
static int fim_pedido;

/* ---------------------------------------------------------------- texto Latin-1 <-> UTF-8 */

size_t portavel_latin1_para_utf8(char *dst, size_t max, const char *s)
{
    size_t n = 0;
    for (; *s && n + 3 < max; s++) {
        unsigned char c = (unsigned char)*s;
        if (c < 0x80) dst[n++] = (char)c;
        else { dst[n++] = (char)(0xC0 | (c >> 6)); dst[n++] = (char)(0x80 | (c & 0x3F)); }
    }
    if (max) dst[n] = 0;
    return n;
}

/* UTF-8 -> Latin-1; lo que no cabe en 8 bits sale como '?'. */
size_t portavel_utf8_para_latin1(char *dst, size_t max, const char *s)
{
    size_t n = 0;
    while (*s && n + 1 < max) {
        unsigned char c = (unsigned char)*s;
        unsigned cp = c;
        int extra = 0;
        if (c >= 0xF0) { cp = c & 7; extra = 3; }
        else if (c >= 0xE0) { cp = c & 15; extra = 2; }
        else if (c >= 0xC0) { cp = c & 31; extra = 1; }
        s++;
        while (extra-- > 0 && ((unsigned char)*s & 0xC0) == 0x80) cp = (cp << 6) | ((unsigned char)*s++ & 0x3F);
        dst[n++] = cp < 256 ? (char)cp : '?';
    }
    if (max) dst[n] = 0;
    return n;
}

/* ---------------------------------------------------------------- guion de pruebas */

typedef struct { char *linha; } passo_t;
static passo_t *roteiro;
static int roteiro_n, roteiro_i;
static unsigned roteiro_espera;       /* ms hasta el siguiente paso */
static unsigned roteiro_t0;
static char roteiro_texto[512];       /* texto pendiente de teclear */
static int roteiro_tpos;
static int roteiro_ativo;
#define PEND_MAX 16
static evento_t roteiro_pend[PEND_MAX];   /* eventos del guion ya generados (soltar tras clic, pasos de arrastre) */
static int roteiro_pend_n, roteiro_pend_i;

static void roteiro_carregar(void)
{
    const char *f = getenv("SAGUI_ROTEIRO");
    if (!f || !*f) return;
    FILE *fp = fopen(f, "rb");
    if (!fp) { fprintf(stderr, "sagui-portavel: no se puede abrir el guion %s\n", f); return; }
    char l[1024];
    while (fgets(l, sizeof l, fp)) {
        size_t n = strlen(l);
        while (n && (l[n - 1] == '\n' || l[n - 1] == '\r')) l[--n] = 0;
        if (!n || l[0] == '#') continue;
        roteiro = realloc(roteiro, sizeof *roteiro * (size_t)(roteiro_n + 1));
        roteiro[roteiro_n++].linha = strdup(l);
    }
    fclose(fp);
    roteiro_ativo = 1;
}

/* ---------------------------------------------------------------- utilidades */

static jan_sis_t *por_id(int id)
{
    if (id < 1 || id > JAN_MAX || !J[id - 1].usada) return NULL;
    return &J[id - 1];
}

static jan_sis_t *por_wid(Uint32 wid)
{
    for (int i = 0; i < JAN_MAX; i++) if (J[i].usada && J[i].wid == wid) return &J[i];
    return NULL;
}

static void empurrar(jan_sis_t *j, evento_t *e)
{
    int prox = (j->ff + 1) % FILA_MAX;
    if (prox == j->fi) return;   /* llena: se pierde el más nuevo */
    e->janela = (int)(j - J) + 1;
    j->fila[j->ff] = *e;
    j->ff = prox;
}

static int sdl_iniciar(void)
{
    if (sdl_ok) return 0;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "sagui-portavel: SDL no arranca: %s\n", SDL_GetError());
        return -1;
    }
    const char *e = getenv("SAGUI_ESCALA");
    if (e && atoi(e) > 0 && atoi(e) <= 4) escala = atoi(e);
    SDL_StartTextInput();
    roteiro_carregar();
    sdl_ok = 1;
    return 0;
}

static unsigned mods_atuais(void)
{
    SDL_Keymod m = SDL_GetModState();
    return ((m & KMOD_SHIFT) ? 1u : 0) | ((m & KMOD_CTRL) ? 2u : 0) | ((m & KMOD_ALT) ? 4u : 0);
}

static void textura_nova(jan_sis_t *j)
{
    if (j->t) SDL_DestroyTexture(j->t);
    j->t = SDL_CreateTexture(j->r, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, j->larg, j->alt);
}

/* ---------------------------------------------------------------- menú contextual (dibujado aquí) */

#define MENU_FILA 20
#define MENU_PAD  6

static void menu_liberar(menu_t *m)
{
    free(m->copia);
    m->copia = NULL;
    m->ativa = 0;
    m->d.n = 0;
}

/* Parte el texto de las líneas (separadas por \n); un ">" abre submenú hasta la línea "<". */
static void menu_analisar(menu_t *m, const char *itens)
{
    free(m->copia);
    m->copia = strdup(itens);
    m->d.n = 0;
    int niv = 0;
    char *p = m->copia;
    while (p && m->d.n < MENU_MAX_ITENS) {
        char *fim = strchr(p, '\n');
        if (fim) *fim = 0;
        int i = m->d.n++;
        unsigned char marca = 0;
        char *t = p;
        if (!strcmp(t, "-")) marca = '-';
        else if (!strcmp(t, "<")) marca = '<';
        else if (*t == '~' || *t == '*' || *t == '>') { marca = (unsigned char)*t; t++; }
        char *tab = strchr(t, '\t');
        m->d.atajo[i] = NULL;
        if (tab) { *tab = 0; m->d.atajo[i] = tab + 1; }
        if (marca == '<' && niv > 0) niv--;
        m->d.nivel[i] = niv;
        m->d.marca[i] = marca;
        m->d.texto[i] = t;
        if (marca == '>') niv++;
        p = fim ? fim + 1 : NULL;
    }
}

/* Líneas de un panel: las de nivel k que cuelgan de la línea «pai» (-1 = raíz). Devuelve cuántas y rellena idx. */
static int menu_linhas(const menu_t *m, int k, int idx[MENU_MAX_ITENS])
{
    int ini = k == 0 ? 0 : m->abertos[k] + 1, n = 0;
    for (int i = ini; i < m->d.n; i++) {
        if (m->d.nivel[i] < k) break;
        if (m->d.nivel[i] == k && m->d.marca[i] != '<') idx[n++] = i;
        if (m->d.nivel[i] == k - 1 && k > 0) break;
    }
    return n;
}

static int texto_larg(const char *s) { return (int)strlen(s) * 8; }

static void pix(uint32_t *d, int L, int A, int x, int y, uint32_t c)
{
    if (x >= 0 && y >= 0 && x < L && y < A) d[y * L + x] = c;
}

static void desenhar_texto(uint32_t *d, int L, int A, int x, int y, const char *s, uint32_t cor)
{
    for (; *s; s++, x += 8) {
        const unsigned char *g = fonte_sagui[(unsigned char)*s];
        for (int yy = 0; yy < 16; yy++)
            for (int xx = 0; xx < 8; xx++)
                if (g[yy] & (0x80 >> xx)) pix(d, L, A, x + xx, y + yy, cor);
    }
}

static void caixa(uint32_t *d, int L, int A, int x, int y, int l, int a, uint32_t cor)
{
    for (int yy = y; yy < y + a; yy++)
        for (int xx = x; xx < x + l; xx++) pix(d, L, A, xx, yy, cor);
}

/* Calcula la geometría de todos los paneles abiertos (el raíz en x, y; cada submenú a la derecha de su línea). */
static void menu_geometria(jan_sis_t *j)
{
    menu_t *m = &j->menu;
    for (int k = 0; k < m->niveis; k++) {
        int idx[MENU_MAX_ITENS], n = menu_linhas(m, k, idx);
        int lmax = 0, amax = 0;
        for (int i = 0; i < n; i++) {
            int w = texto_larg(m->d.texto[idx[i]]) + (m->d.atajo[idx[i]] ? 24 + texto_larg(m->d.atajo[idx[i]]) : 0) + 4 * MENU_PAD + 12;
            if (w > lmax) lmax = w;
            amax += m->d.marca[idx[i]] == '-' ? 7 : MENU_FILA;
        }
        amax += 4;
        int x, y;
        if (k == 0) { x = m->x; y = m->y; }
        else {
            int pi = m->abertos[k], linha_y = m->py[k - 1] + 2;
            int idx2[MENU_MAX_ITENS], n2 = menu_linhas(m, k - 1, idx2);
            for (int i = 0; i < n2 && idx2[i] != pi; i++) linha_y += m->d.marca[idx2[i]] == '-' ? 7 : MENU_FILA;
            x = m->px[k - 1] + m->pl[k - 1] - 2;
            y = linha_y - 2;
        }
        if (x + lmax > j->larg) x = k == 0 ? j->larg - lmax : m->px[k - 1] - lmax + 2;
        if (y + amax > j->alt) y = j->alt - amax;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        m->px[k] = x; m->py[k] = y; m->pl[k] = lmax; m->pa[k] = amax;
    }
}

static void menu_pintar(jan_sis_t *j)
{
    menu_t *m = &j->menu;
    uint32_t bg = sagui_esq.campo_fundo, borda = sagui_esq.borda, tx = sagui_esq.texto, fr = sagui_esq.texto_fraco;
    uint32_t sel = sagui_esq.selecao, st = sagui_esq.selecao_texto;
    menu_geometria(j);
    for (int k = 0; k < m->niveis; k++) {
        caixa(j->vis, j->larg, j->alt, m->px[k], m->py[k], m->pl[k], m->pa[k], borda);
        caixa(j->vis, j->larg, j->alt, m->px[k] + 1, m->py[k] + 1, m->pl[k] - 2, m->pa[k] - 2, bg);
        int idx[MENU_MAX_ITENS], n = menu_linhas(m, k, idx);
        int y = m->py[k] + 2;
        for (int i = 0; i < n; i++) {
            int li = idx[i];
            if (m->d.marca[li] == '-') {
                caixa(j->vis, j->larg, j->alt, m->px[k] + 4, y + 3, m->pl[k] - 8, 1, borda);
                y += 7;
                continue;
            }
            int ativa = m->sel[k] == li;
            if (ativa && m->d.marca[li] != '~') caixa(j->vis, j->larg, j->alt, m->px[k] + 2, y, m->pl[k] - 4, MENU_FILA, sel);
            uint32_t cor = m->d.marca[li] == '~' ? fr : ativa ? st : tx;
            if (m->d.marca[li] == '*') desenhar_texto(j->vis, j->larg, j->alt, m->px[k] + MENU_PAD, y + 2, "\xB7", cor);
            desenhar_texto(j->vis, j->larg, j->alt, m->px[k] + 2 * MENU_PAD + 6, y + 2, m->d.texto[li], cor);
            if (m->d.atajo[li]) desenhar_texto(j->vis, j->larg, j->alt, m->px[k] + m->pl[k] - MENU_PAD - texto_larg(m->d.atajo[li]) - 6, y + 2, m->d.atajo[li], cor);
            if (m->d.marca[li] == '>') desenhar_texto(j->vis, j->larg, j->alt, m->px[k] + m->pl[k] - MENU_PAD - 8, y + 2, ">", cor);
            y += MENU_FILA;
        }
    }
}

/* Línea del panel k bajo (x, y), o -1. */
static int menu_acertar(jan_sis_t *j, int k, int x, int y)
{
    menu_t *m = &j->menu;
    if (x < m->px[k] || x >= m->px[k] + m->pl[k] || y < m->py[k] || y >= m->py[k] + m->pa[k]) return -2;   /* fuera del panel */
    int idx[MENU_MAX_ITENS], n = menu_linhas(m, k, idx);
    int yy = m->py[k] + 2;
    for (int i = 0; i < n; i++) {
        int h = m->d.marca[idx[i]] == '-' ? 7 : MENU_FILA;
        if (y >= yy && y < yy + h) return m->d.marca[idx[i]] == '-' ? -1 : idx[i];
        yy += h;
    }
    return -1;
}

static void menu_responder(jan_sis_t *j, int indice)
{
    evento_t e = { .tipo = EV_MENU, .x = indice, .tecla = (unsigned)j->menu.codigo };
    menu_liberar(&j->menu);
    empurrar(j, &e);
}

/* Trata un evento de ratón o teclado con el menú abierto; devuelve 1 si lo consumió (siempre, mientras está abierto). */
static void menu_mouse(jan_sis_t *j, int tipo, int x, int y)
{
    menu_t *m = &j->menu;
    int dentro = -2, k;
    for (k = m->niveis - 1; k >= 0; k--) {
        dentro = menu_acertar(j, k, x, y);
        if (dentro != -2) break;
    }
    if (tipo == EV_MOVER && k >= 0) {
        m->niveis = k + 1;
        if (dentro >= 0) {
            m->sel[k] = dentro;
            if (m->d.marca[dentro] == '>') { m->niveis = k + 2; m->abertos[k + 1] = dentro; m->sel[k + 1] = -1; }
        }
    } else if (tipo == EV_CLIQUE) {
        if (k < 0) menu_responder(j, -1);
        else if (dentro >= 0 && m->d.marca[dentro] != '~' && m->d.marca[dentro] != '>') menu_responder(j, dentro);
    }
}

static void menu_tecla(jan_sis_t *j, unsigned t)
{
    menu_t *m = &j->menu;
    int k = m->niveis - 1, idx[MENU_MAX_ITENS], n = menu_linhas(m, k, idx), p = -1;
    for (int i = 0; i < n; i++) if (idx[i] == m->sel[k]) p = i;
    if (t == TECLA_ESC) { if (k > 0) m->niveis--; else menu_responder(j, -1); return; }
    if (t == TECLA_BAIXO || t == TECLA_CIMA) {
        for (int passo = 0; passo < n; passo++) {
            p = (p + (t == TECLA_BAIXO ? 1 : -1) + n + (p < 0 && t == TECLA_CIMA ? n : 0)) % n;
            if (m->d.marca[idx[p]] != '-' && m->d.marca[idx[p]] != '~') { m->sel[k] = idx[p]; break; }
        }
    } else if (t == TECLA_DIREITA && p >= 0 && m->d.marca[idx[p]] == '>') {
        m->niveis = k + 2; m->abertos[k + 1] = idx[p]; m->sel[k + 1] = -1;
        int i2[MENU_MAX_ITENS]; if (menu_linhas(m, k + 1, i2) > 0) m->sel[k + 1] = i2[0];
    } else if (t == TECLA_ESQUERDA && k > 0) m->niveis--;
    else if ((t == '\n' || t == ' ') && p >= 0) {
        int li = idx[p];
        if (m->d.marca[li] == '>') { m->niveis = k + 2; m->abertos[k + 1] = li; int i2[MENU_MAX_ITENS]; m->sel[k + 1] = menu_linhas(m, k + 1, i2) > 0 ? i2[0] : -1; }
        else if (m->d.marca[li] != '~') menu_responder(j, li);
    }
}

int menu_contextual(int id, int x, int y, const char *itens, int codigo)
{
    jan_sis_t *j = por_id(id);
    if (!j) return ERRO_INVALIDO;
    menu_liberar(&j->menu);
    menu_analisar(&j->menu, itens ? itens : "");
    j->menu.ativa = 1;
    j->menu.x = x; j->menu.y = y; j->menu.codigo = codigo;
    j->menu.niveis = 1;
    j->menu.sel[0] = -1;
    atualizar_janela(id, NULL);
    return 0;
}

/* ---------------------------------------------------------------- ventanas */

int criar_janela(const char *titulo, int largura, int altura, uint32_t **pixels)
{
    if (sdl_iniciar() < 0) return ERRO_NAO_SUPORTADO;
    int i;
    for (i = 0; i < JAN_MAX && J[i].usada; i++) ;
    if (i == JAN_MAX) return ERRO_LIMITE;
    jan_sis_t *j = &J[i];
    memset(j, 0, sizeof *j);
    char t8[256];
    portavel_latin1_para_utf8(t8, sizeof t8, titulo ? titulo : "");
    j->w = SDL_CreateWindow(t8, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, largura * escala, altura * escala, SDL_WINDOW_SHOWN);
    if (!j->w) { fprintf(stderr, "sagui-portavel: SDL_CreateWindow: %s\n", SDL_GetError()); return ERRO_SEM_MEMORIA; }
    j->r = SDL_CreateRenderer(j->w, -1, 0);
    if (!j->r) j->r = SDL_CreateRenderer(j->w, -1, SDL_RENDERER_SOFTWARE);
    if (!j->r) { SDL_DestroyWindow(j->w); fprintf(stderr, "sagui-portavel: SDL_CreateRenderer: %s\n", SDL_GetError()); return ERRO_SEM_MEMORIA; }
    j->wid = SDL_GetWindowID(j->w);
    j->larg = largura; j->alt = altura;
    j->px = aloca_zerado((size_t)largura * (size_t)altura * 4);
    j->vis = aloca_zerado((size_t)largura * (size_t)altura * 4);
    if (!j->px || !j->vis) { libera(j->px); libera(j->vis); SDL_DestroyRenderer(j->r); SDL_DestroyWindow(j->w); return ERRO_SEM_MEMORIA; }
    textura_nova(j);
    j->usada = 1;
    if (pixels) *pixels = j->px;
    return i + 1;
}

int fechar_janela(int id)
{
    jan_sis_t *j = por_id(id);
    if (!j) return ERRO_INVALIDO;
    menu_liberar(&j->menu);
    if (j->t) SDL_DestroyTexture(j->t);
    SDL_DestroyRenderer(j->r);
    SDL_DestroyWindow(j->w);
    libera(j->px);
    libera(j->vis);
    j->usada = 0;
    return 0;
}

static void apresentar(jan_sis_t *j)
{
    memcpy(j->vis, j->px, (size_t)j->larg * (size_t)j->alt * 4);
    if (j->menu.ativa) menu_pintar(j);
    SDL_UpdateTexture(j->t, NULL, j->vis, j->larg * 4);
    SDL_RenderClear(j->r);
    SDL_RenderCopy(j->r, j->t, NULL, NULL);
    SDL_RenderPresent(j->r);
}

int atualizar_janela(int id, const retangulo_t *r)
{
    (void)r;   /* se sube siempre la ventana entera: es simple y las ventanas son pequeñas */
    jan_sis_t *j = por_id(id);
    if (!j) return ERRO_INVALIDO;
    apresentar(j);
    return 0;
}

int titulo_janela(int id, const char *titulo)
{
    jan_sis_t *j = por_id(id);
    if (!j) return ERRO_INVALIDO;
    char t8[256];
    portavel_latin1_para_utf8(t8, sizeof t8, titulo ? titulo : "");
    SDL_SetWindowTitle(j->w, t8);
    return 0;
}

int mover_janela(int id, int x, int y)
{
    jan_sis_t *j = por_id(id);
    if (!j) return ERRO_INVALIDO;
    SDL_SetWindowPosition(j->w, x, y);
    return 0;
}

int estilo_janela(int id, unsigned estilo, int min_l, int min_a)
{
    jan_sis_t *j = por_id(id);
    if (!j) return ERRO_INVALIDO;
    j->estilo = estilo; j->min_l = min_l; j->min_a = min_a;
    SDL_SetWindowResizable(j->w, (estilo & (JAN_REDIMENSIONAVEL | JAN_TELA_CHEIA_OK)) ? SDL_TRUE : SDL_FALSE);
    if (min_l > 0 && min_a > 0) SDL_SetWindowMinimumSize(j->w, min_l * escala, min_a * escala);
    if (estilo & JAN_TELA_CHEIA) SDL_SetWindowFullscreen(j->w, SDL_WINDOW_FULLSCREEN_DESKTOP);
    else if (!(estilo & JAN_SO_CURSOR) && !(estilo & JAN_SO_CAPTURA)) SDL_SetWindowFullscreen(j->w, 0);
    return 0;
}

int cursor_janela(int id, bool visivel)
{
    (void)visivel;
    return por_id(id) ? 0 : ERRO_INVALIDO;
}

int redimensionar_janela(int id, int largura, int altura, uint32_t **pixels)
{
    jan_sis_t *j = por_id(id);
    if (!j || largura < 1 || altura < 1) return ERRO_INVALIDO;
    uint32_t *np = aloca_zerado((size_t)largura * (size_t)altura * 4), *nv = aloca_zerado((size_t)largura * (size_t)altura * 4);
    if (!np || !nv) { libera(np); libera(nv); return ERRO_SEM_MEMORIA; }
    libera(j->px); libera(j->vis);
    j->px = np; j->vis = nv; j->larg = largura; j->alt = altura;
    textura_nova(j);
    if (pixels) *pixels = np;
    return 0;
}

int pedir_fechar_janela(int id)
{
    jan_sis_t *j = por_id(id);
    if (!j) return ERRO_INVALIDO;
    evento_t e = { .tipo = EV_FECHAR };
    empurrar(j, &e);
    return 0;
}

/* ---------------------------------------------------------------- eventos de SDL -> evento_t */

static unsigned tecla_especial(SDL_Keycode k)
{
    switch (k) {
    case SDLK_RETURN: case SDLK_KP_ENTER: return '\n';
    case SDLK_BACKSPACE: return '\b';
    case SDLK_TAB: return '\t';
    case SDLK_ESCAPE: return TECLA_ESC;
    case SDLK_UP: return TECLA_CIMA;
    case SDLK_DOWN: return TECLA_BAIXO;
    case SDLK_LEFT: return TECLA_ESQUERDA;
    case SDLK_RIGHT: return TECLA_DIREITA;
    case SDLK_HOME: return TECLA_HOME;
    case SDLK_END: return TECLA_END;
    case SDLK_PAGEUP: return TECLA_PGUP;
    case SDLK_PAGEDOWN: return TECLA_PGDN;
    case SDLK_INSERT: return TECLA_INSERT;
    case SDLK_DELETE: return TECLA_DELETE;
    case SDLK_LGUI: case SDLK_RGUI: return TECLA_SUPER;
    default: break;
    }
    if (k >= SDLK_F1 && k <= SDLK_F12) return 0x10A + (unsigned)(k - SDLK_F1);
    return 0;
}

static void tratar_sdl(SDL_Event *ev)
{
    jan_sis_t *j;
    evento_t e;
    memset(&e, 0, sizeof e);
    switch (ev->type) {
    case SDL_QUIT:
        fim_pedido = 1;
        for (int i = 0; i < JAN_MAX; i++) if (J[i].usada) { e.tipo = EV_FECHAR; empurrar(&J[i], &e); break; }
        break;
    case SDL_WINDOWEVENT:
        j = por_wid(ev->window.windowID);
        if (!j) break;
        if (ev->window.event == SDL_WINDOWEVENT_CLOSE) { e.tipo = EV_FECHAR; empurrar(j, &e); }
        else if (ev->window.event == SDL_WINDOWEVENT_SIZE_CHANGED || ev->window.event == SDL_WINDOWEVENT_RESIZED) {
            int w = ev->window.data1 / escala, h = ev->window.data2 / escala;
            if (w > 0 && h > 0 && (w != j->larg || h != j->alt) && (j->estilo & (JAN_REDIMENSIONAVEL | JAN_TELA_CHEIA_OK))) {
                e.tipo = EV_REDIMENSIONAR; e.x = w; e.y = h;
                Uint32 f = SDL_GetWindowFlags(j->w);
                e.tecla = (f & SDL_WINDOW_FULLSCREEN_DESKTOP) == SDL_WINDOW_FULLSCREEN_DESKTOP ? JAN_ESTADO_TELA_CHEIA
                        : (f & SDL_WINDOW_MAXIMIZED) ? JAN_ESTADO_MAXIMIZADA : JAN_ESTADO_NORMAL;
                empurrar(j, &e);
            }
        } else if (ev->window.event == SDL_WINDOWEVENT_EXPOSED) apresentar(j);
        break;
    case SDL_MOUSEMOTION:
        j = por_wid(ev->motion.windowID);
        if (!j) break;
        if (j->menu.ativa) { menu_mouse(j, EV_MOVER, ev->motion.x / escala, ev->motion.y / escala); apresentar(j); break; }
        e.tipo = EV_MOVER; e.x = ev->motion.x / escala; e.y = ev->motion.y / escala; e.botoes = j->botoes;
        empurrar(j, &e);
        break;
    case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP: {
        j = por_wid(ev->button.windowID);
        if (!j) break;
        unsigned b = ev->button.button == SDL_BUTTON_LEFT ? 1u : ev->button.button == SDL_BUTTON_RIGHT ? 2u : ev->button.button == SDL_BUTTON_MIDDLE ? 4u : 0;
        if (!b) break;
        int x = ev->button.x / escala, y = ev->button.y / escala;
        if (ev->type == SDL_MOUSEBUTTONDOWN) {
            j->botoes |= b;
            if (j->menu.ativa) { menu_mouse(j, EV_CLIQUE, x, y); apresentar(j); break; }
            e.tipo = EV_CLIQUE; e.cliques = ev->button.clicks >= 2 ? 2 : 1;
        } else {
            j->botoes &= ~b;
            if (j->menu.ativa) break;
            e.tipo = EV_SOLTOU;
        }
        e.x = x; e.y = y; e.botoes = b; e.modificadores = mods_atuais() & 3;
        empurrar(j, &e);
        break;
    }
    case SDL_MOUSEWHEEL: {
        j = por_wid(ev->wheel.windowID);
        if (!j) break;
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        if (j->menu.ativa) break;
        if (ev->wheel.y) {
            e.tipo = EV_RODA; e.cliques = (unsigned)(int)(ev->wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? ev->wheel.y : -ev->wheel.y);
            e.x = mx / escala; e.y = my / escala; empurrar(j, &e);
        }
        if (ev->wheel.x) {
            evento_t h = { .tipo = EV_RODA_H, .cliques = (unsigned)(int)ev->wheel.x, .x = mx / escala, .y = my / escala };
            empurrar(j, &h);
        }
        break;
    }
    case SDL_KEYDOWN: {
        j = por_wid(ev->key.windowID);
        if (!j) break;
        SDL_Keycode k = ev->key.keysym.sym;
        unsigned mods = mods_atuais(), t = tecla_especial(k);
        int altgr = (SDL_GetModState() & KMOD_MODE) != 0;
        if (!t && (mods & 6) && !altgr && k >= 0x20 && k < 0x7F) {
            /* Ctrl+letra da el código de control (como el teclado de SaguiOS); el resto va con el modificador */
            t = (mods & 2) && k >= 'a' && k <= 'z' ? (unsigned)(k & 0x1F) : (unsigned)k;
            if ((mods & 1) && k >= 'a' && k <= 'z' && !(mods & 2)) t = (unsigned)(k - 32);
        }
        if (!t) break;
        if (j->menu.ativa) { menu_tecla(j, t); apresentar(j); break; }
        e.tipo = EV_TECLA; e.tecla = t; e.modificadores = mods & 7;
        empurrar(j, &e);
        break;
    }
    case SDL_TEXTINPUT: {
        j = NULL;
        for (int i = 0; i < JAN_MAX && !j; i++) if (J[i].usada && (SDL_GetWindowFlags(J[i].w) & SDL_WINDOW_INPUT_FOCUS)) j = &J[i];
        if (!j) for (int i = 0; i < JAN_MAX && !j; i++) if (J[i].usada) j = &J[i];
        if (!j || j->menu.ativa) break;
        unsigned mods = mods_atuais();
        if ((mods & 6) && !(SDL_GetModState() & KMOD_MODE)) break;   /* Ctrl/Alt: ya salió por KEYDOWN */
        char l[16];
        portavel_utf8_para_latin1(l, sizeof l, ev->text.text);
        for (char *p = l; *p; p++) {
            e.tipo = EV_TECLA; e.tecla = (unsigned char)*p; e.modificadores = mods & 7;
            empurrar(j, &e);
        }
        break;
    }
    default: break;
    }
}

static void bombear(void)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) tratar_sdl(&ev);
}

/* ---------------------------------------------------------------- guion: eventos sintéticos */

static int captura_ventana(jan_sis_t *j, const char *arq)
{
    apresentar(j);   /* lo que se ve: la ventana con el menú abierto encima, si lo hay */
    imagem_t img = { j->larg, j->alt, j->vis, "" };
    return imagem_gravar_png(arq, &img);
}

/* Entrega un evento del guion a la ventana que lo pide. Devuelve 1 si hay evento. */
static int roteiro_proximo(jan_sis_t *j, evento_t *e)
{
    memset(e, 0, sizeof *e);
    if (!roteiro_ativo) return 0;
    if (roteiro_pend_i < roteiro_pend_n) { *e = roteiro_pend[roteiro_pend_i++]; return 1; }
    roteiro_pend_i = roteiro_pend_n = 0;
    if (roteiro_tpos < (int)strlen(roteiro_texto)) {
        e->tipo = EV_TECLA; e->tecla = (unsigned char)roteiro_texto[roteiro_tpos++];
        return 1;
    }
    if (roteiro_espera) {
        if (SDL_GetTicks() - roteiro_t0 < roteiro_espera) return 0;
        roteiro_espera = 0;
    }
    if (roteiro_i >= roteiro_n) return 0;
    char *l = roteiro[roteiro_i++].linha;
    char arg[1000];
    int a, b, c;
    if (!strncmp(l, "texto ", 6)) {
        /* \n y \t escapados */
        size_t o = 0;
        for (const char *p = l + 6; *p && o + 1 < sizeof roteiro_texto; p++) {
            if (p[0] == '\\' && p[1] == 'n') { roteiro_texto[o++] = '\n'; p++; }
            else if (p[0] == '\\' && p[1] == 't') { roteiro_texto[o++] = '\t'; p++; }
            else roteiro_texto[o++] = *p;
        }
        roteiro_texto[o] = 0; roteiro_tpos = 0;
        char l1[sizeof roteiro_texto];
        portavel_utf8_para_latin1(l1, sizeof l1, roteiro_texto);   /* el guion está en UTF-8 */
        strcpy(roteiro_texto, l1);
        return roteiro_proximo(j, e);
    }
    if (sscanf(l, "ctrl %c", arg) == 1) { e->tipo = EV_TECLA; e->tecla = (unsigned)(arg[0] & 0x1F); e->modificadores = 2; return 1; }
    if (sscanf(l, "tecla %i %i", &a, &b) >= 1) { e->tipo = EV_TECLA; e->tecla = (unsigned)a; e->modificadores = sscanf(l, "tecla %i %i", &a, &b) == 2 ? (unsigned)b : 0; return 1; }
    if (sscanf(l, "clique %d %d %d", &a, &b, &c) >= 2) {
        unsigned bt = sscanf(l, "clique %d %d %d", &a, &b, &c) == 3 ? (unsigned)c : 1u;
        e->tipo = EV_CLIQUE; e->x = a; e->y = b; e->cliques = 1; e->botoes = bt;
        roteiro_pend[roteiro_pend_n++] = (evento_t){ .tipo = EV_SOLTOU, .x = a, .y = b, .botoes = bt };   /* el soltar sale justo después */
        return 1;
    }
    {   /* arrastar x1 y1 x2 y2 [boton]: pulsar, 8 movimientos con el botón apretado y soltar */
        int x1, y1, x2, y2, bt = 1;
        if (sscanf(l, "arrastar %d %d %d %d %d", &x1, &y1, &x2, &y2, &bt) >= 4) {
            e->tipo = EV_CLIQUE; e->x = x1; e->y = y1; e->cliques = 1; e->botoes = (unsigned)bt;
            for (int k = 1; k <= 8; k++)
                roteiro_pend[roteiro_pend_n++] = (evento_t){ .tipo = EV_MOVER, .x = x1 + (x2 - x1) * k / 8, .y = y1 + (y2 - y1) * k / 8, .botoes = (unsigned)bt };
            roteiro_pend[roteiro_pend_n++] = (evento_t){ .tipo = EV_SOLTOU, .x = x2, .y = y2, .botoes = (unsigned)bt };
            return 1;
        }
    }
    if (sscanf(l, "mover %d %d", &a, &b) == 2) { e->tipo = EV_MOVER; e->x = a; e->y = b; return 1; }
    if (sscanf(l, "esperar %d", &a) == 1) { roteiro_espera = (unsigned)a; roteiro_t0 = SDL_GetTicks(); return 0; }
    if (sscanf(l, "captura %999s", arg) == 1) {
        if (captura_ventana(j, arg) != 0) fprintf(stderr, "sagui-portavel: no se pudo guardar la captura %s\n", arg);
        return roteiro_proximo(j, e);
    }
    if (!strcmp(l, "sair")) { e->tipo = EV_FECHAR; return 1; }
    fprintf(stderr, "sagui-portavel: paso de guion desconocido: %s\n", l);
    return roteiro_proximo(j, e);
}

/* ---------------------------------------------------------------- bucle de eventos de los programas */

static int sacar(jan_sis_t *j, evento_t *e)
{
    if (j->fi == j->ff) return 0;
    *e = j->fila[j->fi];
    j->fi = (j->fi + 1) % FILA_MAX;
    return 1;
}

static int algum_evento(int id, evento_t *e)
{
    jan_sis_t *j = por_id(id);
    if (!j) return 0;
    bombear();
    if (sacar(j, e)) return 1;
    /* ¿otra ventana con eventos? (id 0 en la ABI; aquí los programas siempre piden la suya) */
    int r = roteiro_proximo(j, e);
    while (r == 1 && j->menu.ativa && (e->tipo == EV_MOVER || e->tipo == EV_CLIQUE || e->tipo == EV_SOLTOU || e->tipo == EV_TECLA)) {
        /* con un menú abierto, el guion también habla con el menú (como el ratón y el teclado reales) */
        if (e->tipo == EV_TECLA) menu_tecla(j, e->tecla);
        else if (e->tipo != EV_SOLTOU) menu_mouse(j, e->tipo, e->x, e->y);
        if (sacar(j, e)) { r = 1; break; }   /* la respuesta del menú (EV_MENU) */
        apresentar(j);
        r = roteiro_proximo(j, e);
    }
    if (r == 1) { e->janela = id; return 1; }
    return 0;
}

int proximo_evento(int id, evento_t *e, bool esperar)
{
    if (algum_evento(id, e)) return 1;
    if (!esperar) return 0;
    for (int i = 0; i < 20; i++) {   /* hasta 100 ms: quien llama vuelve a preguntar */
        SDL_Event ev;
        if (SDL_WaitEventTimeout(&ev, roteiro_ativo ? 5 : 5)) tratar_sdl(&ev);
        if (algum_evento(id, e)) return 1;
    }
    return 0;
}

int esperar_evento(int id, evento_t *e, int ms)
{
    unsigned t0 = SDL_GetTicks();
    for (;;) {
        if (algum_evento(id, e)) return 1;
        if (ms >= 0 && (int)(SDL_GetTicks() - t0) >= ms) return 0;
        SDL_Event ev;
        int w = ms < 0 ? 20 : ms - (int)(SDL_GetTicks() - t0);
        if (w > 20) w = 20;
        if (w < 1) w = 1;
        if (SDL_WaitEventTimeout(&ev, w)) tratar_sdl(&ev);
    }
}

/* ---------------------------------------------------------------- portapapeles (el del sistema; texto Latin-1 aquí, UTF-8 en SDL) */

static char *clip_interno;

int copiar_texto(const char *t, unsigned n)
{
    if (n > PORTAPAPEIS_MAX) n = PORTAPAPEIS_MAX;
    char *l = malloc(n + 1), *u = malloc((size_t)n * 2 + 1);
    if (!l || !u) { free(l); free(u); return ERRO_SEM_MEMORIA; }
    memcpy(l, t, n); l[n] = 0;
    portavel_latin1_para_utf8(u, (size_t)n * 2 + 1, l);
    free(clip_interno);
    clip_interno = strdup(u);
    if (sdl_ok) SDL_SetClipboardText(u);
    free(l); free(u);
    return (int)n;
}

int colar_texto(char *buf, unsigned max)
{
    char *u = sdl_ok ? SDL_GetClipboardText() : NULL;
    const char *fonte = (u && *u) ? u : clip_interno;
    int r = 0;
    if (fonte) {
        size_t cap = strlen(fonte) + 1;
        char *l = malloc(cap);   /* en Latin-1 cabe en menos bytes que en UTF-8 */
        if (l) {
            size_t n = portavel_utf8_para_latin1(l, cap, fonte);
            if (n > PORTAPAPEIS_MAX) n = PORTAPAPEIS_MAX;
            r = (int)n;
            if (buf && max) { size_t c = n < max - 1 ? n : max - 1; memcpy(buf, l, c); buf[c] = 0; }
            free(l);
        }
    } else if (buf && max) buf[0] = 0;
    if (u) SDL_free(u);
    return r;
}

static uint32_t *img_clip;
static int img_l, img_a;

int copiar_imagem(int larg, int alt, const uint32_t *px)
{
    free(img_clip);
    img_clip = malloc((size_t)larg * (size_t)alt * 4);
    if (!img_clip) return ERRO_SEM_MEMORIA;
    memcpy(img_clip, px, (size_t)larg * (size_t)alt * 4);
    img_l = larg; img_a = alt;
    return 0;
}

int colar_imagem(uint32_t *destino, unsigned max_px)
{
    if (!img_clip) return 0;
    if (destino) {
        if ((unsigned)(img_l * img_a) > max_px) return 0;
        memcpy(destino, img_clip, (size_t)img_l * (size_t)img_a * 4);
    }
    return img_l | (img_a << 16);
}

int portavel_fim_pedido(void) { return fim_pedido; }
