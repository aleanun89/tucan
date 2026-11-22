// SPDX-License-Identifier: GPL-2.0-or-later
/* gui_revisar.c - detector de problemas de la GUI (D300), ayuda para depurar.
 *
 * Con SAGUI_GUI_REVISAR=1|2 en el entorno (o 1|2 en /etc/gui-revisar.modo, que sirve a las
 * aplicaciones lanzadas desde el escritorio) cada componente de la libsagui anota, al terminar
 * de dibujarse, su rectángulo, tipo y texto (GR_REG en gui.h). Un registro por ventana:
 *  - una entrada nueva sustituye a la anterior del mismo objeto (o del mismo tipo y posición);
 *  - un relleno posterior que cubre un rectángulo entero lo borra del registro (lo pintado
 *    después tapa lo anterior: así un cambio de pestaña no deja componentes fantasma);
 *  - al terminar un redibujo (jan_mostrar, o jan_mostrar_area como mucho cada 250 ms) se
 *    analizan las entradas y se escriben avisos "[GUI-REVISAR] programa ventana: ..." en el
 *    registro serie, sin repetir el mismo aviso.
 * No es un criterio de fallo: un marco con controles dentro, un menú desplegado o un tirador
 * sobre un objeto se solapan a propósito. Se marcan con gui_revisar_ignorar() o con
 * /etc/gui-revisar.ignorar (líneas "programa:tipo:texto", con * como comodín).
 * Apagado no cuesta nada más que un "if" por dibujo. */
#include "gui.h"

int gr_modo;                         /* 0 apagado, 1 avisos, 2 avisos + borde rojo */

#define GR_MAX     512
#define GR_JANELAS 16
#define GR_VISTOS  1024
#define GR_IGN     64
#define GR_PAT     32
#define GR_LIMITE  80                /* avisos por ventana como mucho */

typedef struct {
    int         jid;
    const char *tn;                  /* tipo: "botao", "caixa", ... */
    const void *obj;
    int         x, y, l, a, disp;
    unsigned    flags;
    bool        aviso;               /* tuvo algún aviso en el último análisis */
    char        texto[32];
} gr_ent_t;

typedef struct { int jid, larg, alt, avisos, cob; char titulo[40]; } gr_jan_t;
typedef struct { char prog[24], tipo[24], texto[32]; } gr_pat_t;

extern char **__argv_inicial;        /* lo guarda inicio.asm */

static gr_ent_t   *ent;
static int         nent;
static gr_jan_t    jan[GR_JANELAS];
static uint32_t   *vistos;
static int         nvistos;
static const void *ignorados[GR_IGN];
static int         nign;
static gr_pat_t    pats[GR_PAT];
static int         npats;
static char        prog[24];
static bool        sujo;
static bool        interno;          /* dibujando los bordes rojos: no contar como relleno */
static unsigned    t_ultimo;
static int         iniciado;

/* ------------------------------------------------------------ comodines */

static bool casa(const char *p, const char *t)
{
    if (!*p) return true;
    if (*p == '*') {
        for (;; t++) {
            if (casa(p + 1, t)) return true;
            if (!*t) return false;
        }
    }
    if (*p == *t) return *t ? casa(p + 1, t + 1) : true;
    return false;
}

static bool padrao_casa(const gr_ent_t *e)
{
    for (int i = 0; i < npats; i++)
        if (casa(pats[i].prog, prog) && casa(pats[i].tipo, e->tn) && casa(pats[i].texto, e->texto))
            return true;
    return false;
}

static bool ignorada(const gr_ent_t *e)
{
    for (int i = 0; i < nign; i++)
        if (ignorados[i] == e->obj && e->obj) return true;
    return padrao_casa(e);
}

/* ------------------------------------------------------------ inicio */

static void ler_padroes(void)
{
    int fd = abrir("/etc/gui-revisar.ignorar", ABRIR_LER);
    if (fd < 0) return;
    char b[1024];
    int n = ler(fd, b, sizeof b - 1);
    fechar(fd);
    if (n <= 0) return;
    b[n] = 0;
    for (char *p = b; *p && npats < GR_PAT;) {
        char *fim = p;
        while (*fim && *fim != '\n') fim++;
        char *proximo = *fim ? fim + 1 : fim;
        *fim = 0;
        if (*p && *p != '#') {
            char *c1 = p, *c2;
            while (*c1 && *c1 != ':') c1++;
            if (*c1 == ':') {
                *c1++ = 0;
                c2 = c1;
                while (*c2 && *c2 != ':') c2++;
                if (*c2 == ':') *c2++ = 0;
                gr_pat_t *g = &pats[npats++];
                copia_texto(g->prog, p, sizeof g->prog);
                copia_texto(g->tipo, c1, sizeof g->tipo);
                copia_texto(g->texto, c2, sizeof g->texto);
            }
        }
        p = proximo;
    }
}

/* Lo llama jan_abrir: lee el modo una vez y reserva las tablas solo si está activo. */
static void iniciar(void)
{
    if (iniciado) return;
    iniciado = 1;
    int m = 0;
    const char *v = obter_env("SAGUI_GUI_REVISAR");
    if (v && v[0] >= '0' && v[0] <= '9') m = v[0] - '0';
    if (!m) {
        int fd = abrir("/etc/gui-revisar.modo", ABRIR_LER);
        if (fd >= 0) {
            char c[4] = "";
            if (ler(fd, c, 1) == 1 && c[0] >= '0' && c[0] <= '9') m = c[0] - '0';
            fechar(fd);
        }
    }
    if (m <= 0) return;
    ent = aloca(sizeof(gr_ent_t) * GR_MAX);
    vistos = aloca(sizeof(uint32_t) * GR_VISTOS);
    if (!ent || !vistos) return;
    const char *a0 = __argv_inicial ? __argv_inicial[0] : NULL;
    if (a0) {
        const char *b = a0;
        for (const char *q = a0; *q; q++) if (*q == '/') b = q + 1;
        copia_texto(prog, b, sizeof prog);
    }
    if (!prog[0]) copia_texto(prog, "?", sizeof prog);
    ler_padroes();
    gr_modo = m > 2 ? 2 : m;
    registrar("[GUI-REVISAR] %s: detector activo (modo %d)\n", prog, gr_modo);
}


void gr_janela_aberta(jan_t *j, const char *titulo)
{
    iniciar();
    if (!gr_modo) return;
    for (int i = 0; i < GR_JANELAS; i++)
        if (!jan[i].jid) {
            jan[i].jid = j->id + 1;      /* 0 = hueco libre */
            jan[i].larg = j->larg;
            jan[i].alt = j->alt;
            jan[i].avisos = 0;
            jan[i].cob = 0;
            copia_texto(jan[i].titulo, titulo ? titulo : "?", sizeof jan[i].titulo);
            return;
        }
}

void gr_janela_fechada(jan_t *j)
{
    if (!gr_modo) return;
    int k = 0, mios = 0;
    for (int i = 0; i < nent; i++)
        if (ent[i].jid != j->id) ent[k++] = ent[i]; else mios++;
    (void)mios;
    nent = k;
    for (int i = 0; i < GR_JANELAS; i++)
        if (jan[i].jid == j->id + 1) jan[i].jid = 0;
}

static gr_jan_t *buscar_jan(jan_t *j)
{
    for (int i = 0; i < GR_JANELAS; i++)
        if (jan[i].jid == j->id + 1) return &jan[i];
    return NULL;
}

/* ------------------------------------------------------------ registro */

void gr_registrar(jan_t *j, const char *tipo, const void *obj, int x, int y, int l, int a,
                  const char *texto, int disp, unsigned flags)
{
    if (!ent || interno) return;
    int pos = -1;
    for (int i = 0; i < nent; i++) {
        gr_ent_t *e = &ent[i];
        if (e->jid != j->id || e->tn != tipo) continue;
        if (obj ? e->obj == obj : (!e->obj && e->x == x && e->y == y)) { pos = i; break; }
    }
    if (pos < 0) {
        if (nent >= GR_MAX) return;
        pos = nent++;
        sujo = true;
    }
    gr_ent_t *e = &ent[pos];
    char t[32];
    memset(t, 0, sizeof t);
    copia_texto(t, texto ? texto : "", sizeof t);
    if (e->jid != j->id || e->x != x || e->y != y || e->l != l || e->a != a || e->disp != disp ||
        memcmp(e->texto, t, sizeof t) != 0) sujo = true;
    e->jid = j->id; e->tn = tipo; e->obj = obj;
    e->x = x; e->y = y; e->l = l; e->a = a; e->disp = disp; e->flags = flags;
    memcpy(e->texto, t, sizeof t);
}

void gui_revisar_rect(jan_t *j, const char *tipo, int x, int y, int l, int a, const char *texto, int disp)
{
    GR_REG(j, tipo, NULL, x, y, l, a, texto, disp, 0);
}

void gui_revisar_ignorar(const void *componente)
{
    if (!gr_modo || !componente) return;
    for (int i = 0; i < nign; i++) if (ignorados[i] == componente) return;
    if (nign < GR_IGN) ignorados[nign++] = componente;
}

/* Un relleno que cubre un rectángulo entero lo tapa: fuera del registro. Lo llama g_retangulo. */
void gr_pintado(jan_t *j, int x, int y, int l, int a)
{
    if (!ent || interno || l < 12 || a < 12) return;
    int k = 0;
    for (int i = 0; i < nent; i++) {
        const gr_ent_t *e = &ent[i];
        bool tapado = e->jid == j->id && e->l > 0 && e->a > 0 &&
                      e->x >= x && e->y >= y && e->x + e->l <= x + l && e->y + e->a <= y + a;
        if (!tapado) ent[k++] = ent[i];
    }
    nent = k;
}

/* Texto escrito sin recorte que se sale del borde de la ventana (lo llama g_texto_n). */
static bool ya_visto(uint32_t h)
{
    for (int i = 0; i < nvistos; i++) if (vistos[i] == h) return true;
    if (nvistos < GR_VISTOS) vistos[nvistos++] = h;
    return false;
}

static uint32_t mezclar(uint32_t h, uint32_t v) { return (h ^ v) * 16777619u; }

static uint32_t hash_texto(uint32_t h, const char *t)
{
    for (; *t; t++) h = mezclar(h, (uint8_t)*t);
    return h;
}

void gr_texto_fuera(jan_t *j, int x, int y, const char *t, int n)
{
    if (!ent || interno) return;
    int len = (int)tam_texto(t);
    if (n < len) len = n;
    if (len <= 0 || x < 0 || x >= j->larg || y < 0 || y + LETRA_A > j->alt) return;
    if (x + len * LETRA_L <= j->larg) return;
    gr_jan_t *w = buscar_jan(j);
    uint32_t h = hash_texto(mezclar(mezclar(7, (uint32_t)x), (uint32_t)y), t);
    if (ya_visto(h) || (w && w->avisos >= GR_LIMITE)) return;
    if (w) w->avisos++;
    char c[28];
    copia_texto(c, t, sizeof c);   /* registrar no sabe de precisiones: se recorta antes */
    registrar("[GUI-REVISAR] %s %s: texto cortado por el borde de la ventana en (%d,%d): \"%s\" necesita %d px y quedan %d\n",
              prog, w ? w->titulo : "?", x, y, c, len * LETRA_L, j->larg - x);
}

/* ------------------------------------------------------------ análisis */

static bool contem(const gr_ent_t *g, const gr_ent_t *p)
{
    return p->x >= g->x - 1 && p->y >= g->y - 1 && p->x + p->l <= g->x + g->l + 1 &&
           p->y + p->a <= g->y + g->a + 1;
}

static void avisar(gr_jan_t *w, uint32_t h, const gr_ent_t *a, const gr_ent_t *b, const char *msg, int p1, int p2)
{
    if (ya_visto(h)) return;
    if (w->avisos >= GR_LIMITE) return;
    w->avisos++;
    if (b)
        registrar("[GUI-REVISAR] %s %s: %s %s(%d,%d %dx%d \"%s\") con %s(%d,%d %dx%d \"%s\"), zona %d px x %d px\n",
                  prog, w->titulo, msg, a->tn, a->x, a->y, a->l, a->a, a->texto,
                  b->tn, b->x, b->y, b->l, b->a, b->texto, p1, p2);
    else
        registrar("[GUI-REVISAR] %s %s: %s %s(%d,%d %dx%d \"%s\") [%d/%d]\n",
                  prog, w->titulo, msg, a->tn, a->x, a->y, a->l, a->a, a->texto, p1, p2);
}

void gr_analizar(jan_t *j, bool completo)
{
    if (!ent || interno) return;
    unsigned ahora = tempo_ms();
    if (!completo && ahora - t_ultimo < 250) return;
    if (!sujo && gr_modo < 2) return;
    t_ultimo = ahora;
    gr_jan_t *w = buscar_jan(j);
    if (!w) return;
    w->larg = j->larg;
    w->alt = j->alt;
    {   /* cobertura: cuántos componentes de la libsagui hay en la ventana (lo dibujado a mano no cuenta) */
        int n = 0;
        for (int i = 0; i < nent; i++) if (ent[i].jid == j->id) n++;
        if (n >= 2 * w->cob || (w->cob == 0 && n > 0)) {
            w->cob = n > 0 ? n : 1;
            registrar("[GUI-REVISAR] %s %s: cobertura, %d componentes registrados\n", prog, w->titulo, n);
        }
    }
    if (sujo) {
        for (int i = 0; i < nent; i++) ent[i].aviso = false;
        for (int i = 0; i < nent; i++) {
            gr_ent_t *a = &ent[i];
            if (a->jid != j->id || a->l <= 0 || a->a <= 0) continue;
            bool ign = ignorada(a);
            uint32_t ha = mezclar(mezclar(mezclar(mezclar(hash_texto(1, a->tn), (uint32_t)a->x), (uint32_t)a->y),
                                          (uint32_t)a->l), (uint32_t)a->a);
            /* solape con un hermano */
            for (int k = i + 1; k < nent; k++) {
                gr_ent_t *b = &ent[k];
                if (b->jid != j->id || b->l <= 0 || b->a <= 0) continue;
                int ox = (a->x + a->l < b->x + b->l ? a->x + a->l : b->x + b->l) - (a->x > b->x ? a->x : b->x);
                int oy = (a->y + a->a < b->y + b->a ? a->y + a->a : b->y + b->a) - (a->y > b->y ? a->y : b->y);
                if (ox <= 1 || oy <= 1) continue;          /* tolerancia: borde compartido de 1 px */
                if (contem(a, b) || contem(b, a)) continue; /* padre e hijo */
                if (ign || ignorada(b)) continue;
                a->aviso = b->aviso = true;
                uint32_t h = mezclar(mezclar(mezclar(mezclar(mezclar(hash_texto(2, b->tn), (uint32_t)b->x),
                             (uint32_t)b->y), (uint32_t)b->l), (uint32_t)b->a), ha);
                avisar(w, h, a, b, "solape", ox, oy);
            }
            if (ign) continue;
            /* fuera del área cliente */
            if (a->x < 0 || a->y < 0 || a->x + a->l > j->larg || a->y + a->a > j->alt) {
                a->aviso = true;
                int fx = a->x < 0 ? -a->x : a->x + a->l - j->larg;
                int fy = a->y < 0 ? -a->y : a->y + a->a - j->alt;
                avisar(w, mezclar(ha, 3), a, NULL, "fuera de la ventana", fx > 0 ? fx : 0, fy > 0 ? fy : 0);
            }
            /* texto que no cabe */
            if (a->disp > 0 && a->texto[0]) {
                int need = g_largura_texto(a->texto);
                if (need > a->disp) {
                    a->aviso = true;
                    avisar(w, mezclar(ha, 4), a, NULL, "texto cortado: ancho necesario/disponible", need, a->disp);
                }
            }
            /* demasiado pequeño */
            if ((a->flags & GRF_CONTROL) && (a->a < LETRA_A || a->l < LETRA_L)) {
                a->aviso = true;
                avisar(w, mezclar(ha, 5), a, NULL, "demasiado pequeño: tamaño/mínimo", a->a < a->l ? a->a : a->l, LETRA_A);
            }
        }
        sujo = false;
    }
    if (gr_modo >= 2) {   /* modo visual: borde rojo en lo que tiene aviso (D300) */
        interno = true;
        for (int i = 0; i < nent; i++) {
            const gr_ent_t *a = &ent[i];
            if (a->jid != j->id || !a->aviso) continue;
            g_moldura(j, a->x - 1, a->y - 1, a->l + 2, a->a + 2, 0xFF0000);
            retangulo_t r = { a->x - 1, a->y - 1, a->l + 2, a->a + 2 };
            atualizar_janela(j->id, &r);
        }
        interno = false;
    }
}
