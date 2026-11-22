// SPDX-License-Identifier: GPL-2.0-or-later
/* presentacion.c - modelo, composición de texto y dibujo de las presentaciones de Tucán (D369).
 * Los ficheros están en presentacion_io.c (formato propio, PDF, PNG) y presentacion_pptx.c (PPTX). */
#include "presentacion.h"
#include "pdf_escribir.h"
#include "wordart.h"

extern const unsigned char fonte_sagui[256][16];

const char *pr_hints[3] = { "Clic para agregar un título", "Clic para agregar texto", "Clic para agregar un subtítulo" };

/* ------------------------------------------------------------ utilidades */

static unsigned contador_version;

static bool igual_i(const char *a, const char *b)
{
    for (; *a && *b; a++, b++) {
        char x = *a, y = *b;
        if (x >= 'A' && x <= 'Z') x = (char)(x + 32);
        if (y >= 'A' && y <= 'Z') y = (char)(y + 32);
        if (x != y) return false;
    }
    return *a == *b;
}

int pr_latin1_a_utf8(const char *s, char *d, int max)
{
    int k = 0;
    for (const unsigned char *p = (const unsigned char *)s; *p && k < max - 3; p++) {
        if (*p < 0x80) d[k++] = (char)*p;
        else { d[k++] = (char)(0xC0 | (*p >> 6)); d[k++] = (char)(0x80 | (*p & 0x3F)); }
    }
    d[k] = 0;
    return k;
}

/* UTF-8 -> Latin-1: lo que no existe se sustituye por algo parecido o por '?'. */
int pr_utf8_a_latin1(const char *s, int n, char *d, int max)
{
    int k = 0;
    for (int i = 0; i < n && k < max - 4; ) {
        unsigned char c = (unsigned char)s[i];
        unsigned cp; int len;
        if (c < 0x80) { cp = c; len = 1; }
        else if ((c & 0xE0) == 0xC0 && i + 1 < n) { cp = ((unsigned)(c & 0x1F) << 6) | ((unsigned char)s[i + 1] & 0x3F); len = 2; }
        else if ((c & 0xF0) == 0xE0 && i + 2 < n) { cp = ((unsigned)(c & 0x0F) << 12) | (((unsigned char)s[i + 1] & 0x3Fu) << 6) | ((unsigned char)s[i + 2] & 0x3F); len = 3; }
        else if ((c & 0xF8) == 0xF0 && i + 3 < n) { cp = 0x3F; len = 4; }
        else { cp = 0x3F; len = 1; }
        i += len;
        if (cp < 0x100) { d[k++] = (char)cp; continue; }
        switch (cp) {
        case 0x2018: case 0x2019: case 0x201A: d[k++] = '\''; break;
        case 0x201C: case 0x201D: case 0x201E: d[k++] = '"'; break;
        case 0x2013: case 0x2014: d[k++] = '-'; break;
        case 0x2022: case 0x25CF: d[k++] = (char)0xB7; break;
        case 0x2026: d[k++] = '.'; d[k++] = '.'; d[k++] = '.'; break;
        case 0x20AC: d[k++] = 'E'; d[k++] = 'U'; d[k++] = 'R'; break;
        case 0x2122: d[k++] = 'T'; d[k++] = 'M'; break;
        case 0x2192: d[k++] = '-'; d[k++] = '>'; break;
        default: d[k++] = '?'; break;
        }
    }
    d[k] = 0;
    return k;
}

static int isqrt_u(unsigned v)
{
    unsigned r = 0, b = 1u << 30;
    while (b > v) b >>= 2;
    while (b) {
        if (v >= r + b) { v -= r + b; r = (r >> 1) + b; } else r >>= 1;
        b >>= 2;
    }
    return (int)r;
}

/* seno de un ángulo en grados (0..360) multiplicado por 1000 (aproximación de Bhaskara) */
static int seno1000(int g)
{
    g %= 360; if (g < 0) g += 360;
    int neg = 0;
    if (g >= 180) { g -= 180; neg = 1; }
    int p = g * (180 - g);
    int v = 4 * p * 1000 / (40500 - p);
    return neg ? -v : v;
}
static int coseno1000(int g) { return seno1000(g + 90); }

/* ------------------------------------------------------------ temas */

const pr_tema_t pr_temas[] = {
    { "Oficina",       0xFFFFFF, 0xFFFFFF, 0x1F3864, 0x262626, 0x2E75B6, 0xED7D31, 1, "Arial", "Arial" },
    { "Tucán",         0x14110F, 0x2E241C, 0xFFA726, 0xF5F0E6, 0xFF8F00, 0xFFD54F, 3, "Arial", "Arial" },
    { "Selva",         0xF1F8E9, 0xDCEDC8, 0x1B5E20, 0x263238, 0x2E7D32, 0xFBC02D, 2, "Arial", "Arial" },
    { "Océano",        0x0D2B45, 0x1E6091, 0xFFFFFF, 0xE3F2FD, 0x4FC3F7, 0xFFEE58, 1, "Arial", "Arial" },
    { "Atardecer",     0xFF8A65, 0xAD1457, 0xFFFFFF, 0xFFF3E0, 0xFFD54F, 0xFFFFFF, 1, "Arial", "Arial" },
    { "Papel",         0xFBF5E6, 0xF1E6CA, 0x5D4037, 0x3E2723, 0x8D6E63, 0xBF360C, 2, "Times New Roman", "Times New Roman" },
    { "Noche",         0x10172B, 0x1F2A4D, 0xFFFFFF, 0xCFD8DC, 0x7C4DFF, 0x00E5FF, 3, "Arial", "Arial" },
    { "Menta",         0xE0F7F4, 0xFFFFFF, 0x00695C, 0x37474F, 0x26A69A, 0xEF5350, 1, "Arial", "Arial" },
    { "Minimal",       0xFAFAFA, 0xFAFAFA, 0x212121, 0x424242, 0x212121, 0xE53935, 0, "Arial", "Arial" },
    { "Alto contraste", 0x000000, 0x000000, 0xFFFF00, 0xFFFFFF, 0x00FF00, 0x00FFFF, 0, "Arial", "Arial" },
};
int pr_n_temas(void) { return (int)(sizeof pr_temas / sizeof pr_temas[0]); }

static const char *const nomes_forma[PF_N] = { "Rectángulo", "Rectángulo redondeado", "Elipse", "Triángulo", "Rombo", "Flecha", "Línea", "Estrella" };
static const char *const nomes_layout[PR_LY_N] = { "Diapositiva de título", "Título y contenido", "Dos columnas", "Solo el título", "En blanco" };
static const char *const nomes_trans[PR_TRANS_N] = { "Corte", "Fundido", "Desplazar", "Barrido" };
const char *pr_nombre_forma(int f) { return f >= 0 && f < PF_N ? nomes_forma[f] : ""; }
const char *pr_nombre_layout(int l) { return l >= 0 && l < PR_LY_N ? nomes_layout[l] : ""; }
const char *pr_nombre_trans(int t) { return t >= 0 && t < PR_TRANS_N ? nomes_trans[t] : ""; }

static const pr_tema_t *tema_de(const pr_deck_t *d)
{
    int t = d ? d->tema : 0;
    if (t < 0 || t >= pr_n_temas()) t = 0;
    return &pr_temas[t];
}

/* ------------------------------------------------------------ fuentes */

int pr_familia(const char *nombre)
{
    if (!nombre || !nombre[0]) return 0;
    const fuentes_prov_t *p = fuentes_prov_obter();
    if (p && p->resolver) { int f = p->resolver(nombre); if (f >= 0) return f; }
    for (int i = 0; i < 3; i++) if (igual_i(pdfw_nombre_familia(i), nombre)) return i;
    static const struct { const char *n; int f; } alias[] = {
        { "helvetica", 0 }, { "calibri", 0 }, { "segoe ui", 0 }, { "verdana", 0 }, { "tahoma", 0 }, { "dejavu sans", 0 },
        { "times", 1 }, { "cambria", 1 }, { "georgia", 1 }, { "garamond", 1 }, { "palatino linotype", 1 }, { "dejavu serif", 1 },
        { "courier", 2 }, { "consolas", 2 }, { "lucida console", 2 }, { "dejavu sans mono", 2 },
    };
    for (unsigned i = 0; i < sizeof alias / sizeof alias[0]; i++) if (igual_i(alias[i].n, nombre)) return alias[i].f;
    return 0;
}
const char *pr_nombre_fam(int fam) { return pdfw_nombre_familia(fam); }

const char *pr_fuente_obj(const pr_deck_t *d, const pr_obj_t *o)
{
    if (o->fuente[0]) return o->fuente;
    if (o->rol == PR_ROL_TITULO && d->f_titulo[0]) return d->f_titulo;
    if (d->f_cuerpo[0]) return d->f_cuerpo;
    return "Arial";
}

uint32_t pr_color_texto(const pr_deck_t *d, const pr_obj_t *o)
{
    if (!(o->col & PR_COL_TEMA)) return o->col & 0xFFFFFF;
    const pr_tema_t *t = tema_de(d);
    if (o->tipo == PR_FORMA && o->rellena) {   /* texto sobre una forma rellena: blanco o negro según el relleno */
        uint32_t r = pr_color_relleno(d, o);
        int lum = (int)(((r >> 16) & 255) * 30 + ((r >> 8) & 255) * 59 + (r & 255) * 11) / 100;
        return lum > 140 ? 0x202020 : 0xFFFFFF;
    }
    return (o->rol == PR_ROL_TITULO || o->rol == PR_ROL_SUBTITULO) ? t->titulo : t->texto;
}
uint32_t pr_color_relleno(const pr_deck_t *d, const pr_obj_t *o)
{
    return (o->relleno & PR_COL_TEMA) ? tema_de(d)->acento : (o->relleno & 0xFFFFFF);
}
uint32_t pr_color_borde(const pr_deck_t *d, const pr_obj_t *o)
{
    return (o->borde_col & PR_COL_TEMA) ? tema_de(d)->acento2 : (o->borde_col & 0xFFFFFF);
}

/* ------------------------------------------------------------ modelo */

void pr_tocar(pr_diap_t *s) { if (s) s->version = ++contador_version; }

void pr_obj_liberar_campos(pr_obj_t *o)
{
    libera(o->texto);
    o->texto = NULL;
}

bool pr_obj_poner_texto(pr_obj_t *o, const char *t)
{
    size_t n = t ? tam_texto(t) : 0;
    if (n > PR_TEXTO_MAX) n = PR_TEXTO_MAX;
    char *c = aloca(n + 1);
    if (!c) return false;
    if (n) memcpy(c, t, n);
    c[n] = 0;
    libera(o->texto);
    o->texto = c;
    return true;
}

bool pr_obj_copiar(pr_obj_t *dst, const pr_obj_t *src)
{
    *dst = *src;
    dst->texto = NULL;
    return pr_obj_poner_texto(dst, src->texto ? src->texto : "");
}

pr_obj_t *pr_obj_nuevo(pr_diap_t *s, int tipo)
{
    if (s->n >= PR_OBJ_MAX) return NULL;
    if (s->n >= s->cap) {
        int nc = s->cap ? s->cap * 2 : 8;
        pr_obj_t *nv = realoca(s->obj, (size_t)nc * sizeof *nv);
        if (!nv) return NULL;
        s->obj = nv; s->cap = nc;
    }
    pr_obj_t *o = &s->obj[s->n];
    memset(o, 0, sizeof *o);
    o->tipo = (uint8_t)tipo;
    o->tam = 24;
    o->col = PR_COL_TEMA; o->relleno = PR_COL_TEMA; o->borde_col = PR_COL_TEMA;
    o->medio = -1;
    o->x = 60; o->y = 60; o->w = 300; o->h = 100;
    if (!pr_obj_poner_texto(o, "")) return NULL;
    s->n++;
    pr_tocar(s);
    return o;
}

void pr_obj_borrar(pr_diap_t *s, int i)
{
    if (i < 0 || i >= s->n) return;
    pr_obj_liberar_campos(&s->obj[i]);
    memmove(&s->obj[i], &s->obj[i + 1], (size_t)(s->n - i - 1) * sizeof(pr_obj_t));
    s->n--;
    pr_tocar(s);
}

void pr_obj_orden(pr_diap_t *s, int i, int a)
{
    if (i < 0 || i >= s->n || a < 0 || a >= s->n || a == i) return;
    pr_obj_t t = s->obj[i];
    if (a > i) memmove(&s->obj[i], &s->obj[i + 1], (size_t)(a - i) * sizeof t);
    else memmove(&s->obj[a + 1], &s->obj[a], (size_t)(i - a) * sizeof t);
    s->obj[a] = t;
    pr_tocar(s);
}

static void diap_liberar(pr_diap_t *s)
{
    if (!s) return;
    for (int i = 0; i < s->n; i++) pr_obj_liberar_campos(&s->obj[i]);
    libera(s->obj);
    libera(s->notas);
    libera(s);
}

static pr_diap_t *diap_crear(void)
{
    pr_diap_t *s = aloca_zerado(sizeof *s);
    if (!s) return NULL;
    s->fondo_medio = -1;
    s->fondo1 = 0xFFFFFF; s->fondo2 = 0xCCCCCC;
    s->notas = aloca(1);
    if (!s->notas) { libera(s); return NULL; }
    s->notas[0] = 0;
    pr_tocar(s);
    return s;
}

static bool deck_meter(pr_deck_t *d, int pos, pr_diap_t *s)
{
    if (d->n >= PR_DIAP_MAX) return false;
    if (d->n >= d->cap) {
        int nc = d->cap ? d->cap * 2 : 16;
        pr_diap_t **nv = realoca(d->d, (size_t)nc * sizeof *nv);
        if (!nv) return false;
        d->d = nv; d->cap = nc;
    }
    if (pos < 0 || pos > d->n) pos = d->n;
    memmove(&d->d[pos + 1], &d->d[pos], (size_t)(d->n - pos) * sizeof *d->d);
    d->d[pos] = s;
    d->n++;
    return true;
}

/* Rectángulos de los marcadores de cada diseño (en puntos, para una diapositiva W x H). */
typedef struct { int rol, x, y, w, h, tam, alin, valign, lista; } plaza_t;
static int plazas(const pr_deck_t *d, int layout, plaza_t *p)
{
    int W = d->ancho, H = d->alto;
    int mx = W * 6 / 100, mw = W - 2 * mx;
    switch (layout) {
    case PR_LY_TITULO:
        p[0] = (plaza_t){ PR_ROL_TITULO, W * 8 / 100, H * 26 / 100, W * 84 / 100, H * 26 / 100, 48, 1, 2, 0 };
        p[1] = (plaza_t){ PR_ROL_SUBTITULO, W * 8 / 100, H * 55 / 100, W * 84 / 100, H * 18 / 100, 26, 1, 0, 0 };
        return 2;
    case PR_LY_TITULO_CONTENIDO:
        p[0] = (plaza_t){ PR_ROL_TITULO, mx, H * 5 / 100, mw, H * 18 / 100, 40, 0, 1, 0 };
        p[1] = (plaza_t){ PR_ROL_CUERPO, mx, H * 26 / 100, mw, H * 62 / 100, 28, 0, 0, PR_LISTA_VINETA };
        return 2;
    case PR_LY_DOS_COLUMNAS: {
        int cw = (mw - 20) / 2;
        p[0] = (plaza_t){ PR_ROL_TITULO, mx, H * 5 / 100, mw, H * 18 / 100, 40, 0, 1, 0 };
        p[1] = (plaza_t){ PR_ROL_CUERPO, mx, H * 26 / 100, cw, H * 62 / 100, 24, 0, 0, PR_LISTA_VINETA };
        p[2] = (plaza_t){ PR_ROL_CUERPO, mx + cw + 20, H * 26 / 100, cw, H * 62 / 100, 24, 0, 0, PR_LISTA_VINETA };
        return 3;
    }
    case PR_LY_SOLO_TITULO:
        p[0] = (plaza_t){ PR_ROL_TITULO, mx, H * 5 / 100, mw, H * 18 / 100, 40, 0, 1, 0 };
        return 1;
    default: return 0;
    }
}

void pr_aplicar_layout(pr_deck_t *d, pr_diap_t *s, int layout)
{
    if (layout < 0 || layout >= PR_LY_N) layout = PR_LY_BLANCO;
    plaza_t pl[4];
    int np = plazas(d, layout, pl);
    bool usado[PR_OBJ_MAX] = { false };
    s->layout = (uint8_t)layout;
    for (int k = 0; k < np; k++) {
        pr_obj_t *o = NULL;
        int rolk = pl[k].rol;
        for (int i = 0; i < s->n && !o; i++)   /* reutiliza un marcador con ese papel (o el cuerpo para el subtítulo y al revés) */
            if (!usado[i] && s->obj[i].rol == rolk) { o = &s->obj[i]; usado[i] = true; }
        if (!o && (rolk == PR_ROL_SUBTITULO || rolk == PR_ROL_CUERPO))
            for (int i = 0; i < s->n && !o; i++)
                if (!usado[i] && (s->obj[i].rol == PR_ROL_SUBTITULO || s->obj[i].rol == PR_ROL_CUERPO)) { o = &s->obj[i]; usado[i] = true; }
        if (!o) {
            o = pr_obj_nuevo(s, PR_TEXTO);
            if (!o) continue;
            usado[s->n - 1] = true;
        }
        o->rol = (uint8_t)rolk;
        o->x = (int16_t)pl[k].x; o->y = (int16_t)pl[k].y; o->w = (int16_t)pl[k].w; o->h = (int16_t)pl[k].h;
        o->tam = (uint8_t)pl[k].tam; o->alin = (uint8_t)pl[k].alin; o->valign = (uint8_t)pl[k].valign; o->lista = (uint8_t)pl[k].lista;
        o->est = rolk == PR_ROL_TITULO ? PR_NEGRITA : 0;
        o->col = PR_COL_TEMA; o->fuente[0] = 0;
    }
    /* los marcadores que sobran: si tienen texto pasan a ser cuadros libres; si no, se quitan */
    for (int i = s->n - 1; i >= 0; i--) {
        if (i < PR_OBJ_MAX && !usado[i] && s->obj[i].rol != PR_ROL_LIBRE && s->obj[i].tipo == PR_TEXTO) {
            if (s->obj[i].texto && s->obj[i].texto[0]) s->obj[i].rol = PR_ROL_LIBRE;
            else { pr_obj_borrar(s, i); memmove(&usado[i], &usado[i + 1], (size_t)(PR_OBJ_MAX - i - 1)); }
        }
    }
    pr_tocar(s);
}

pr_diap_t *pr_diap_nueva(pr_deck_t *d, int pos, int layout)
{
    pr_diap_t *s = diap_crear();
    if (!s) return NULL;
    if (!deck_meter(d, pos, s)) { diap_liberar(s); return NULL; }
    pr_aplicar_layout(d, s, layout);
    return s;
}

static pr_diap_t *diap_copiar(const pr_diap_t *o)
{
    pr_diap_t *s = diap_crear();
    if (!s) return NULL;
    libera(s->notas); s->notas = NULL;
    s->obj = NULL; s->n = s->cap = 0;
    s->layout = o->layout; s->trans = o->trans; s->oculta = o->oculta; s->fondo_tipo = o->fondo_tipo; s->dur_trans = o->dur_trans;
    s->fondo1 = o->fondo1; s->fondo2 = o->fondo2; s->fondo_medio = o->fondo_medio; s->auto_seg = o->auto_seg;
    size_t nn = o->notas ? tam_texto(o->notas) : 0;
    s->notas = aloca(nn + 1);
    if (!s->notas) { diap_liberar(s); return NULL; }
    if (nn) memcpy(s->notas, o->notas, nn);
    s->notas[nn] = 0;
    if (o->n) {
        s->obj = aloca_zerado((size_t)o->n * sizeof(pr_obj_t));
        if (!s->obj) { diap_liberar(s); return NULL; }
        s->cap = o->n;
        for (int i = 0; i < o->n; i++) {
            if (!pr_obj_copiar(&s->obj[i], &o->obj[i])) { s->n = i; diap_liberar(s); return NULL; }
            s->n = i + 1;
        }
    }
    pr_tocar(s);
    return s;
}

pr_diap_t *pr_diap_duplicar(pr_deck_t *d, int i)
{
    if (i < 0 || i >= d->n) return NULL;
    pr_diap_t *s = diap_copiar(d->d[i]);
    if (!s) return NULL;
    if (!deck_meter(d, i + 1, s)) { diap_liberar(s); return NULL; }
    return s;
}

void pr_diap_borrar(pr_deck_t *d, int i)
{
    if (i < 0 || i >= d->n || d->n <= 1) return;
    diap_liberar(d->d[i]);
    memmove(&d->d[i], &d->d[i + 1], (size_t)(d->n - i - 1) * sizeof *d->d);
    d->n--;
}

void pr_diap_mover(pr_deck_t *d, int de, int a)
{
    if (de < 0 || de >= d->n || a < 0 || a >= d->n || de == a) return;
    pr_diap_t *s = d->d[de];
    if (a > de) memmove(&d->d[de], &d->d[de + 1], (size_t)(a - de) * sizeof *d->d);
    else memmove(&d->d[a + 1], &d->d[a], (size_t)(de - a) * sizeof *d->d);
    d->d[a] = s;
}

pr_diap_t *pr_diap_vacia(pr_deck_t *d)
{
    pr_diap_t *s = diap_crear();
    if (!s) return NULL;
    if (!deck_meter(d, d->n, s)) { diap_liberar(s); return NULL; }
    return s;
}

pr_deck_t *pr_deck_vacio(bool ancha)
{
    pr_deck_t *d = aloca_zerado(sizeof *d);
    if (!d) return NULL;
    d->ancho = ancha ? 960 : 720; d->alto = 540;
    copia_texto(d->f_titulo, pr_temas[0].f_titulo, sizeof d->f_titulo);
    copia_texto(d->f_cuerpo, pr_temas[0].f_cuerpo, sizeof d->f_cuerpo);
    return d;
}

pr_deck_t *pr_nueva(bool ancha)
{
    pr_deck_t *d = aloca_zerado(sizeof *d);
    if (!d) return NULL;
    d->ancho = ancha ? 960 : 720; d->alto = 540;
    d->tema = 0;
    copia_texto(d->f_titulo, pr_temas[0].f_titulo, sizeof d->f_titulo);
    copia_texto(d->f_cuerpo, pr_temas[0].f_cuerpo, sizeof d->f_cuerpo);
    if (!pr_diap_nueva(d, 0, PR_LY_TITULO)) { pr_liberar(d); return NULL; }
    return d;
}

void pr_liberar(pr_deck_t *d)
{
    if (!d) return;
    for (int i = 0; i < d->n; i++) diap_liberar(d->d[i]);
    libera(d->d);
    for (int i = 0; i < d->n_medios; i++) { libera(d->medios[i].dados); imagem_liberar(&d->medios[i].img); }
    libera(d->medios);
    libera(d);
}

void pr_aplicar_tema(pr_deck_t *d, int tema)
{
    if (tema < 0 || tema >= pr_n_temas()) return;
    d->tema = tema;
    copia_texto(d->f_titulo, pr_temas[tema].f_titulo, sizeof d->f_titulo);
    copia_texto(d->f_cuerpo, pr_temas[tema].f_cuerpo, sizeof d->f_cuerpo);
    for (int i = 0; i < d->n; i++) pr_tocar(d->d[i]);
}

void pr_tamano(pr_deck_t *d, bool ancha)
{
    int nw = ancha ? 960 : 720;
    if (nw == d->ancho) return;
    int ow = d->ancho;
    for (int i = 0; i < d->n; i++) {
        pr_diap_t *s = d->d[i];
        for (int k = 0; k < s->n; k++) {
            pr_obj_t *o = &s->obj[k];
            o->x = (int16_t)(o->x * nw / ow); o->w = (int16_t)(o->w * nw / ow);
        }
        pr_tocar(s);
    }
    d->ancho = nw;
}

/* ------------------------------------------------------------ medios (imágenes) */

int pr_medio_anadir(pr_deck_t *d, const uint8_t *dados, int n)
{
    if (!dados || n < 8) return -1;
    for (int i = 0; i < d->n_medios; i++)   /* la misma imagen no se guarda dos veces */
        if (d->medios[i].n == n && memcmp(d->medios[i].dados, dados, (size_t)n) == 0) return i;
    if (d->n_medios >= PR_MEDIOS_MAX) return -1;
    bool png = dados[0] == 0x89 && dados[1] == 'P' && dados[2] == 'N' && dados[3] == 'G';
    bool jpg = dados[0] == 0xFF && dados[1] == 0xD8;
    uint8_t *copia = NULL; int cn = 0;
    const char *ext = "png";
    imagem_t im = { 0 };
    bool tengo_im = false;
    if (png || jpg) {
        copia = aloca((size_t)n);
        if (!copia) return -1;
        memcpy(copia, dados, (size_t)n);
        cn = n; ext = jpg ? "jpg" : "png";
    } else {   /* BMP, GIF, PPM...: se guarda como PNG */
        if (imagem_decodificar(dados, n, &im) != 0) return -1;
        tengo_im = true;
        if (imagem_codificar_png(&im, &copia, &cn) != 0) { imagem_liberar(&im); return -1; }
    }
    pr_medio_t *nv = realoca(d->medios, (size_t)(d->n_medios + 1) * sizeof *nv);
    if (!nv) { libera(copia); if (tengo_im) imagem_liberar(&im); return -1; }
    d->medios = nv;
    pr_medio_t *m = &d->medios[d->n_medios];
    memset(m, 0, sizeof *m);
    m->dados = copia; m->n = cn;
    copia_texto(m->ext, ext, sizeof m->ext);
    if (tengo_im) { m->img = im; m->dec = true; }
    return d->n_medios++;
}

const imagem_t *pr_medio_imagen(pr_deck_t *d, int medio)
{
    if (medio < 0 || medio >= d->n_medios) return NULL;
    pr_medio_t *m = &d->medios[medio];
    if (!m->dec) {
        m->dec = true;
        imagem_t im = { 0 };
        if (imagem_decodificar(m->dados, m->n, &im) == 0) {
            if (im.larg > 1600 || im.alt > 1200) {   /* en equipos con poca memoria no se guarda una foto de 12 Mpx entera */
                int l = im.larg, a = im.alt;
                if (l * 1200 > a * 1600) { a = a * 1600 / l; l = 1600; } else { l = l * 1200 / a; a = 1200; }
                if (l < 1) l = 1;
                if (a < 1) a = 1;
                imagem_t r = { 0 };
                if (imagem_escalar(&r, &im, l, a, true)) { imagem_liberar(&im); im = r; }
            }
            m->img = im;
        }
    }
    return m->img.px ? &m->img : NULL;
}

/* ------------------------------------------------------------ composición de texto */

int pr_ancho_texto(int fam, int est, int tam, const char *t, int n)
{
    int m = 0;
    for (int i = 0; i < n; i++) m += pdfw_largura_char(fam, est & 3, (unsigned char)t[i]);
    return m * tam / 10;
}

static int sumar_m(int fam, int est, const char *t, int a, int b)
{
    int m = 0;
    for (int i = a; i < b; i++) m += pdfw_largura_char(fam, est & 3, (unsigned char)t[i]);
    return m;
}

bool pr_componer(const pr_deck_t *d, const pr_obj_t *o, const char *texto, pr_comp_t *c)
{
    memset(c, 0, sizeof *c);
    int fam = pr_familia(pr_fuente_obj(d, o)), est = o->est, tam = o->tam < 1 ? 12 : o->tam;
    int W = (o->w - 2 * PR_INSET_X) * 100;
    if (W < 500) W = 500;
    int lh = tam * 120;
    int cap = 16;
    pr_lin_t *L = aloca((size_t)cap * sizeof *L);
    if (!L) return false;
    int n = 0, y = 0;
    int cuenta[6] = { 0 };
    int pos = 0, len = (int)tam_texto(texto);
    bool primero = true;
    for (;;) {
        int pe = pos;
        while (pe < len && texto[pe] != '\n') pe++;
        int s = pos, nivel = 0;
        while (s < pe && texto[s] == '\t' && nivel < 4) { s++; nivel++; }
        int vacia = (s == pe);
        int sangria = nivel * 2700 + (o->lista ? 1800 : 0);
        int disp = W - sangria;
        if (disp < 1000) disp = 1000;
        int marca = 0;
        if (o->lista && !vacia) {
            if (o->lista == PR_LISTA_NUMERO) { cuenta[nivel]++; for (int k = nivel + 1; k < 6; k++) cuenta[k] = 0; marca = cuenta[nivel] + 1; }
            else marca = 1;
        }
        if (!primero) y += tam * 20;
        bool prim = true;
        int ini = s;
        do {
            int fin = pe, lastsp = -1, m = 0, mlast = 0;
            for (int i = ini; i < pe; i++) {
                int cw = pdfw_largura_char(fam, est & 3, (unsigned char)texto[i]);
                if ((m + cw) * tam / 10 > disp && i > ini) {
                    if (lastsp >= ini) fin = lastsp + 1; else fin = i;
                    (void)mlast;
                    break;
                }
                if (texto[i] == ' ') { lastsp = i; mlast = m; }
                m += cw;
            }
            int fin_vis = fin;
            while (fin_vis > ini && texto[fin_vis - 1] == ' ') fin_vis--;
            int w = sumar_m(fam, est, texto, ini, fin_vis) * tam / 10;
            if (n >= cap) {
                cap *= 2;
                pr_lin_t *nl = realoca(L, (size_t)cap * sizeof *L);
                if (!nl) { libera(L); return false; }
                L = nl;
            }
            pr_lin_t *l = &L[n++];
            l->ini = ini; l->n = fin - ini; l->pini = prim ? pos : ini;
            l->ancho = w;
            int off = o->alin == 1 ? (disp - w) / 2 : o->alin == 2 ? disp - w : 0;
            if (off < 0) off = 0;
            l->x = sangria + off;
            l->y = y;
            l->base = y + tam * 92;
            l->nivel = nivel;
            l->marca = prim ? marca : 0;
            l->marca_x = nivel * 2700;
            l->primera = prim;
            y += lh;
            prim = false;
            ini = fin;
        } while (ini < pe);
        primero = false;
        if (pe >= len) break;
        pos = pe + 1;
    }
    int ih = (o->h - 2 * PR_INSET_Y) * 100;
    int y0 = 0;
    if (o->valign == 1) y0 = (ih - y) / 2; else if (o->valign == 2) y0 = ih - y;
    if (y0 != 0) for (int i = 0; i < n; i++) { L[i].y += y0; L[i].base += y0; }
    c->l = L; c->n = n; c->alto = y; c->ancho_int = W; c->y0 = y0;
    return true;
}

void pr_comp_liberar(pr_comp_t *c) { libera(c->l); c->l = NULL; c->n = 0; }

void pr_cursor_xy(const pr_deck_t *d, const pr_obj_t *o, const pr_comp_t *c, int pos, int *x, int *y)
{
    int fam = pr_familia(pr_fuente_obj(d, o)), tam = o->tam < 1 ? 12 : o->tam;
    const char *t = o->texto ? o->texto : "";
    int k = 0;
    for (int i = 0; i < c->n; i++) if (c->l[i].pini <= pos) k = i;
    const pr_lin_t *l = &c->l[k];
    int hasta = pos < l->ini ? l->ini : pos;
    if (hasta > l->ini + l->n) hasta = l->ini + l->n;
    *x = PR_INSET_X * 100 + l->x + pr_ancho_texto(fam, o->est, tam, t + l->ini, hasta - l->ini);
    *y = PR_INSET_Y * 100 + l->y;
}

int pr_pos_en(const pr_deck_t *d, const pr_obj_t *o, const pr_comp_t *c, int x, int y)
{
    int fam = pr_familia(pr_fuente_obj(d, o)), tam = o->tam < 1 ? 12 : o->tam;
    const char *t = o->texto ? o->texto : "";
    x -= PR_INSET_X * 100; y -= PR_INSET_Y * 100;
    int k = 0;
    for (int i = 0; i < c->n; i++) { if (c->l[i].y <= y) k = i; }
    const pr_lin_t *l = &c->l[k];
    int rx = x - l->x, m = 0, best = l->ini;
    for (int i = 0; i < l->n; i++) {
        int cw = pdfw_largura_char(fam, o->est & 3, (unsigned char)t[l->ini + i]);
        int mid = (m + cw / 2) * tam / 10;
        if (rx < mid) return l->ini + i;
        m += cw;
        best = l->ini + i + 1;
    }
    return best;
}

/* ------------------------------------------------------------ pintado: primitivas */

typedef struct { uint32_t *px; int w, h; int sc; int mini; } sup_t;   /* sc: píxeles por punto en 16.16 */

#define PX(s, v) (((v) * (s)->sc) >> 16)
/* centésimas de punto -> píxeles en 1/256 */
static int c100_fx(const sup_t *s, int cpt) { return ((cpt / 100) * s->sc >> 8) + (((cpt % 100) * s->sc / 100) >> 8); }

static inline uint32_t mezclar(uint32_t d, uint32_t c, int a)   /* a: 0..255 */
{
    if (a <= 0) return d;
    if (a >= 255) return c;
    int ia = 255 - a;
    uint32_t r = ((d >> 16 & 255) * ia + (c >> 16 & 255) * a + 127) / 255;
    uint32_t g = ((d >> 8 & 255) * ia + (c >> 8 & 255) * a + 127) / 255;
    uint32_t b = ((d & 255) * ia + (c & 255) * a + 127) / 255;
    return r << 16 | g << 8 | b;
}

static void rect_c(sup_t *s, int x0, int y0, int x1, int y1, uint32_t c, int a)
{
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > s->w) x1 = s->w;
    if (y1 > s->h) y1 = s->h;
    for (int y = y0; y < y1; y++) {
        uint32_t *p = s->px + y * s->w;
        if (a >= 255) for (int x = x0; x < x1; x++) p[x] = c;
        else for (int x = x0; x < x1; x++) p[x] = mezclar(p[x], c, a);
    }
}

#define POLI_MAX 160
/* Relleno de un polígono (coordenadas en 1/16 de píxel) con 4 subfilas por píxel: bordes suaves. */
static void poli_rellenar(sup_t *s, const int *pt, int n, uint32_t c, int alfa)
{
    static int acc[2304];
    if (n < 3 || s->w > 2300) return;
    int ymin = pt[1], ymax = pt[1];
    for (int i = 1; i < n; i++) { if (pt[2 * i + 1] < ymin) ymin = pt[2 * i + 1]; if (pt[2 * i + 1] > ymax) ymax = pt[2 * i + 1]; }
    int py0 = (ymin >> 4) - 1, py1 = (ymax >> 4) + 1;
    if (py0 < 0) py0 = 0;
    if (py1 >= s->h) py1 = s->h - 1;
    for (int py = py0; py <= py1; py++) {
        int xa = s->w, xb = -1;
        for (int sub = 0; sub < 4; sub++) {
            int sy = py * 16 + sub * 4 + 2;
            int xs[POLI_MAX], nx = 0;
            for (int i = 0; i < n; i++) {
                int j = i + 1 == n ? 0 : i + 1;
                int x0 = pt[2 * i], y0 = pt[2 * i + 1], x1 = pt[2 * j], y1 = pt[2 * j + 1];
                if (y0 == y1) continue;
                if ((sy < y0 && sy < y1) || (sy >= y0 && sy >= y1)) continue;
                if (nx < POLI_MAX) xs[nx++] = x0 + (sy - y0) * (x1 - x0) / (y1 - y0);
            }
            for (int i = 1; i < nx; i++) { int v = xs[i], k = i - 1; while (k >= 0 && xs[k] > v) { xs[k + 1] = xs[k]; k--; } xs[k + 1] = v; }
            for (int i = 0; i + 1 < nx; i += 2) {
                int a = xs[i], b = xs[i + 1];
                if (b <= a) continue;
                if (a < 0) a = 0;
                if (b > s->w * 16) b = s->w * 16;
                if (b <= a) continue;
                int pa = a >> 4, pb = (b - 1) >> 4;
                if (pa < xa) xa = pa;
                if (pb > xb) xb = pb;
                if (pa == pb) { acc[pa] += b - a; continue; }
                acc[pa] += 16 - (a & 15);
                for (int x = pa + 1; x < pb; x++) acc[x] += 16;
                acc[pb] += ((b - 1) & 15) + 1;
            }
        }
        if (xb < 0) continue;
        uint32_t *row = s->px + py * s->w;
        for (int x = xa; x <= xb && x < s->w; x++) {
            int v = acc[x];
            acc[x] = 0;
            if (v <= 0) continue;
            int al = v >= 64 ? 255 : v * 255 / 64;
            if (alfa < 255) al = al * alfa / 255;
            row[x] = mezclar(row[x], c, al);
        }
    }
}

/* Un trazo grueso entre dos puntos (1/16 px) con grosor gr (1/16 px); ext: alarga los extremos (esquinas limpias). */
static void trazo(sup_t *s, int x0, int y0, int x1, int y1, int gr, uint32_t c, bool ext)
{
    int dx = x1 - x0, dy = y1 - y0;
    int len = isqrt_u((unsigned)(dx * dx) + (unsigned)(dy * dy));
    if (len == 0) return;
    int hx = dx * gr / 2 / len, hy = dy * gr / 2 / len;     /* mitad del grosor a lo largo */
    int nx = -hy, ny = hx;                                  /* y perpendicular */
    if (ext) { x0 -= hx; y0 -= hy; x1 += hx; y1 += hy; }
    int p[8] = { x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny };
    poli_rellenar(s, p, 4, c, 255);
}

/* ------------------------------------------------------------ formas */

/* Puntos de la forma dentro del rectángulo (x, y, w, h) en 1/16 de píxel. Devuelve el número de puntos. */
static int forma_puntos(int forma, int x, int y, int w, int h, int *p, bool *esquinas)
{
    int n = 0;
    *esquinas = true;
#define P(a, b) do { p[n++] = (a); p[n++] = (b); } while (0)
    switch (forma) {
    case PF_TRIANGULO: P(x + w / 2, y); P(x + w, y + h); P(x, y + h); return 3;
    case PF_ROMBO: P(x + w / 2, y); P(x + w, y + h / 2); P(x + w / 2, y + h); P(x, y + h / 2); return 4;
    case PF_FLECHA: {
        int cu = w * 58 / 100, ay = h * 28 / 100;
        P(x, y + ay); P(x + cu, y + ay); P(x + cu, y); P(x + w, y + h / 2); P(x + cu, y + h); P(x + cu, y + h - ay); P(x, y + h - ay);
        return 7;
    }
    case PF_ESTRELLA: {
        int cx = x + w / 2, cy = y + h / 2;
        for (int i = 0; i < 10; i++) {
            int ang = -90 + i * 36;
            int rr = (i & 1) ? 40 : 100;
            P(cx + w / 2 * rr / 100 * coseno1000(ang) / 1000, cy + h / 2 * rr / 100 * seno1000(ang) / 1000);
        }
        return 10;
    }
    case PF_ELIPSE: {
        *esquinas = false;
        int cx = x + w / 2, cy = y + h / 2, N = 64;
        for (int i = 0; i < N; i++) {
            int ang = i * 360 / N;
            P(cx + w / 2 * coseno1000(ang) / 1000, cy + h / 2 * seno1000(ang) / 1000);
        }
        return N;
    }
    case PF_REDONDEADO: {
        *esquinas = false;
        int r = (w < h ? w : h) / 6;
        int cxs[4] = { x + w - r, x + r, x + r, x + w - r }, cys[4] = { y + r, y + r, y + h - r, y + h - r };
        for (int q = 0; q < 4; q++)
            for (int k = 0; k <= 8; k++) {
                int ang = -90 + q * 90 + k * 90 / 8;
                P(cxs[q] + r * coseno1000(ang) / 1000, cys[q] + r * seno1000(ang) / 1000);
            }
        return 36;
    }
    default: P(x, y); P(x + w, y); P(x + w, y + h); P(x, y + h); return 4;
    }
#undef P
}

/* ------------------------------------------------------------ glifos y texto */

static void glifo_dib(sup_t *s, int fam, int est, unsigned char ch, int em, int x, int base, uint32_t col)
{
    if (ch <= ' ' || ch == 0xA0 || em < 1) return;
    const fuentes_prov_t *p = fuentes_prov_obter();
    if (p && p->glifo) {
        int l, a, ox, oy;
        const uint8_t *g = p->glifo(fam, est & 3, ch, em, &l, &a, &ox, &oy);
        if (g) {
            for (int yy = 0; yy < a; yy++) {
                int Y = base + oy + yy;
                if (Y < 0 || Y >= s->h) continue;
                const uint8_t *lin = g + yy * l;
                uint32_t *row = s->px + Y * s->w;
                for (int xx = 0; xx < l; xx++) {
                    int X = x + ox + xx;
                    if (lin[xx] && X >= 0 && X < s->w) row[X] = mezclar(row[X], col, lin[xx]);
                }
            }
            return;
        }
    }
    int ch_h = em + em / 8, cw = em / 2 + 1;   /* sin proveedor: la 8x16 escalada */
    int top = base - ch_h * 13 / 16;
    for (int dy = 0; dy < ch_h; dy++) {
        int Y = top + dy;
        if (Y < 0 || Y >= s->h) continue;
        unsigned char fila = fonte_sagui[ch][dy * 16 / ch_h];
        for (int dx = 0; dx < cw; dx++) {
            int X = x + dx;
            if (X < 0 || X >= s->w) continue;
            if ((fila >> (7 - dx * 8 / cw)) & 1) s->px[Y * s->w + X] = col;
        }
    }
}

/* Dibuja un texto compuesto: o es el objeto (posición y formato), texto el contenido. ox, oy: origen del
 * objeto en puntos. sin_texto: solo las viñetas. */
static void texto_dib(sup_t *s, const pr_deck_t *d, const pr_obj_t *o, const char *texto, int ox, int oy, bool sin_texto, uint32_t col)
{
    pr_comp_t c;
    if (!pr_componer(d, o, texto, &c)) return;
    int fam = pr_familia(pr_fuente_obj(d, o)), tam = o->tam < 1 ? 12 : o->tam;
    int em_fx = tam * s->sc / 256;                         /* cuerpo en píxeles x256 */
    int em = (em_fx + 128) >> 8;
    int bx = PX(s, ox + PR_INSET_X), by = PX(s, oy + PR_INSET_Y);
    for (int i = 0; i < c.n; i++) {
        const pr_lin_t *l = &c.l[i];
        int base = by + (c100_fx(s, l->base) >> 8);
        int x_fx = (bx << 8) + c100_fx(s, l->x);
        if (l->marca) {
            int mx = bx + (c100_fx(s, l->marca_x) >> 8);
            if (l->marca == 1) {
                int d_ = em * 3 / 10 + 1, cy = base - em * 32 / 100;
                int cx = mx + em * 4 / 10;
                int p[8 * 2];
                for (int k = 0; k < 8; k++) { p[2 * k] = cx * 16 + d_ * 8 * coseno1000(k * 45) / 1000; p[2 * k + 1] = cy * 16 + d_ * 8 * seno1000(k * 45) / 1000; }
                poli_rellenar(s, p, 8, col, 255);
            } else if (!sin_texto) {
                char nb[12];
                int nn = formatar_texto(nb, sizeof nb, "%d.", l->marca - 1);
                int xf = mx << 8;
                for (int k = 0; k < nn; k++) { glifo_dib(s, fam, o->est, (unsigned char)nb[k], em, xf >> 8, base, col); xf += pdfw_largura_char(fam, o->est & 3, (unsigned char)nb[k]) * em_fx / 1000; }
            }
        }
        if (sin_texto) continue;
        if (s->mini && em < 7) {   /* miniatura: el texto como una raya */
            int w = c100_fx(s, l->ancho) >> 8;
            int gr = em / 3 < 1 ? 1 : em / 3;
            rect_c(s, x_fx >> 8, base - em / 2, (x_fx >> 8) + w, base - em / 2 + gr, col, 150);
            continue;
        }
        for (int k = 0; k < l->n; k++) {
            unsigned char ch = (unsigned char)texto[l->ini + k];
            glifo_dib(s, fam, o->est, ch, em, x_fx >> 8, base, col);
            x_fx += pdfw_largura_char(fam, o->est & 3, ch) * em_fx / 1000;
        }
        if (o->est & PR_SUBRAYADO) {
            int w = c100_fx(s, l->ancho) >> 8;
            int x0 = (bx << 8) + c100_fx(s, l->x);
            rect_c(s, x0 >> 8, base + em / 12 + 1, (x0 >> 8) + w, base + em / 12 + 1 + (em / 14 < 1 ? 1 : em / 14), col, 255);
        }
    }
    pr_comp_liberar(&c);
}

/* ------------------------------------------------------------ WordArt (con caché) */

#define WA_CACHE 8
static struct { unsigned h; imagem_t im; unsigned uso; } wa_cache[WA_CACHE];
static unsigned wa_uso;

static unsigned hash_wa(const pr_obj_t *o, int l, int a, int fam)
{
    unsigned h = 2166136261u;
    for (const char *p = o->texto; p && *p; p++) h = (h ^ (unsigned char)*p) * 16777619u;
    h = (h ^ (unsigned)o->wa_estilo) * 16777619u; h = (h ^ (unsigned)l) * 16777619u; h = (h ^ (unsigned)a) * 16777619u;
    h = (h ^ (unsigned)fam) * 16777619u; h = (h ^ o->est) * 16777619u;
    return h | 1;
}

static void wordart_dib(sup_t *s, const pr_obj_t *o, int x, int y, int w, int h)
{
    if (w < 4 || h < 4 || !o->texto || !o->texto[0]) return;
    int fam = 0, est = 0;
    wordart_padrao(o->wa_estilo, &fam, &est);
    unsigned hh = hash_wa(o, w, h, fam);
    int k = -1;
    for (int i = 0; i < WA_CACHE; i++) if (wa_cache[i].h == hh) { k = i; break; }
    if (k < 0) {
        int old = 0;
        for (int i = 1; i < WA_CACHE; i++) if (wa_cache[i].uso < wa_cache[old].uso) old = i;
        imagem_liberar(&wa_cache[old].im);
        wa_cache[old].h = 0;
        char tx[WORDART_TEXTO_MAX];
        copia_texto(tx, o->texto, sizeof tx);
        if (!wordart_renderizar(tx, o->wa_estilo, fam, est, w, h, &wa_cache[old].im)) return;
        wa_cache[old].h = hh;
        k = old;
    }
    wa_cache[k].uso = ++wa_uso;
    const imagem_t *im = &wa_cache[k].im;
    for (int yy = 0; yy < im->alt; yy++) {
        int Y = y + yy;
        if (Y < 0 || Y >= s->h) continue;
        for (int xx = 0; xx < im->larg; xx++) {
            int X = x + xx;
            if (X < 0 || X >= s->w) continue;
            uint32_t p = im->px[yy * im->larg + xx];
            int r = p >> 16 & 255, g = p >> 8 & 255, b = p & 255;
            int mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
            if (mn >= 250) continue;                       /* el fondo blanco del WordArt es transparente */
            int a = 255 - (mn > 200 ? (mn - 200) * 4 : 0);
            if (a < 0) a = 0;
            s->px[Y * s->w + X] = mezclar(s->px[Y * s->w + X], p, a > 255 ? 255 : a);
        }
    }
}

/* ------------------------------------------------------------ gráficos */

typedef struct { char et[24]; int v; } dato_t;
static int graf_datos(const char *t, dato_t *dt, int max)
{
    int n = 0;
    while (t && *t && n < max) {
        while (*t == ' ' || *t == ',' || *t == ';') t++;
        if (!*t) break;
        int k = 0;
        while (*t && *t != '=' && *t != ',' && k < 23) dt[n].et[k++] = *t++;
        dt[n].et[k] = 0;
        while (*t && *t != '=' && *t != ',') t++;
        int v = 0, neg = 0;
        if (*t == '=') { t++; if (*t == '-') { neg = 1; t++; } while (*t >= '0' && *t <= '9') { v = v * 10 + (*t - '0'); t++; } while (*t && *t != ',') t++; }
        dt[n].v = neg ? -v : v;
        n++;
    }
    return n;
}

static const uint32_t grafcol[6] = { 0, 0xED7D31, 0xA5A5A5, 0xFFC000, 0x5B9BD5, 0x70AD47 };

static void graf_dib(sup_t *s, const pr_deck_t *d, const pr_obj_t *o, int x, int y, int w, int h)
{
    dato_t dt[12];
    int n = graf_datos(o->texto, dt, 12);
    const pr_tema_t *t = tema_de(d);
    uint32_t cols[6];
    cols[0] = t->acento;
    for (int i = 1; i < 6; i++) cols[i] = grafcol[i];
    uint32_t txt = t->texto;
    if (n == 0) return;
    int fam = 0, em = PX(s, 12);
    if (em < 6) em = 6;
    int vmax = 1;
    for (int i = 0; i < n; i++) if (dt[i].v > vmax) vmax = dt[i].v;
    if (o->graf == PR_GRAF_SECTORES) {
        int tot = 0;
        for (int i = 0; i < n; i++) if (dt[i].v > 0) tot += dt[i].v;
        if (tot <= 0) return;
        int r = (w < h ? w : h) / 2 - 2, cx = x + w / 2, cy = y + h / 2, ang = -90;
        for (int i = 0; i < n; i++) {
            if (dt[i].v <= 0) continue;
            int da = dt[i].v * 360 / tot;
            if (i == n - 1) da = 270 - ang;
            if (da < 1) da = 1;
            int p[2 * 40], k = 0;
            p[k++] = cx * 16; p[k++] = cy * 16;
            int pasos = da / 6 + 1;
            for (int q = 0; q <= pasos && k < 78; q++) {
                int a2 = ang + da * q / pasos;
                p[k++] = (cx + r * coseno1000(a2) / 1000) * 16; p[k++] = (cy + r * seno1000(a2) / 1000) * 16;
            }
            poli_rellenar(s, p, k / 2, cols[i % 6], 255);
            ang += da;
        }
        return;
    }
    int ex = x + em * 3, ey = y + 4, ew = w - em * 3 - 4, eh = h - em * 2 - 8;
    if (ew < 10 || eh < 10) return;
    rect_c(s, ex, ey + eh, ex + ew, ey + eh + 1, txt, 255);
    rect_c(s, ex, ey, ex + 1, ey + eh, txt, 255);
    int paso = ew / n;
    int xant = 0, yant = 0;
    for (int i = 0; i < n; i++) {
        int v = dt[i].v < 0 ? 0 : dt[i].v;
        int bh = v * eh / vmax;
        int bx = ex + i * paso + paso / 6, bw = paso * 2 / 3;
        if (o->graf == PR_GRAF_LINEAS) {
            int cxp = ex + i * paso + paso / 2, cyp = ey + eh - bh;
            if (i) trazo(s, xant * 16, yant * 16, cxp * 16, cyp * 16, 3 * 16 / 2 + s->sc / 4096, cols[0], false);
            xant = cxp; yant = cyp;
            int pp[8] = { (cxp - 3) * 16, (cyp - 3) * 16, (cxp + 3) * 16, (cyp - 3) * 16, (cxp + 3) * 16, (cyp + 3) * 16, (cxp - 3) * 16, (cyp + 3) * 16 };
            poli_rellenar(s, pp, 4, cols[1], 255);
        } else rect_c(s, bx, ey + eh - bh, bx + bw, ey + eh, cols[i % 6], 255);
        int lw = 0, nl = (int)tam_texto(dt[i].et);
        for (int k = 0; k < nl; k++) lw += pdfw_largura_char(fam, 0, (unsigned char)dt[i].et[k]) * em / 1000;
        int lx = ex + i * paso + (paso - lw) / 2;
        if (lw <= paso + em)
            for (int k = 0; k < nl; k++) { glifo_dib(s, fam, 0, (unsigned char)dt[i].et[k], em, lx, ey + eh + em + 2, txt); lx += pdfw_largura_char(fam, 0, (unsigned char)dt[i].et[k]) * em / 1000; }
    }
}

/* ------------------------------------------------------------ tabla */

static const char *celda(const char *t, int idx, int *n)
{
    const char *p = t;
    for (int i = 0; i < idx; i++) { while (*p && *p != '\x1f') p++; if (*p) p++; else { *n = 0; return p; } }
    int k = 0;
    while (p[k] && p[k] != '\x1f') k++;
    *n = k;
    return p;
}

/* La celda (f, c) de una tabla como un objeto de texto suelto (tmp), con su texto en buf y su color. */
static void celda_obj(const pr_deck_t *d, const pr_obj_t *o, int f, int c, pr_obj_t *tmp, char *buf, uint32_t *col)
{
    const pr_tema_t *t = tema_de(d);
    int n; const char *tx = celda(o->texto ? o->texto : "", f * o->cols + c, &n);
    if (n > 199) n = 199;
    memcpy(buf, tx, (size_t)n); buf[n] = 0;
    *tmp = *o;
    tmp->w = (int16_t)(o->w / o->cols); tmp->h = (int16_t)(o->h / o->filas); tmp->valign = 1; tmp->lista = 0;
    if (f == 0) tmp->est |= PR_NEGRITA;
    if (!(o->col & PR_COL_TEMA)) *col = o->col & 0xFFFFFF;
    else if (f == 0) *col = ((t->acento >> 16 & 255) * 30 + (t->acento >> 8 & 255) * 59 + (t->acento & 255) * 11) / 100 > 140 ? 0x202020 : 0xFFFFFF;
    else *col = t->texto;
}

static void tabla_dib(sup_t *s, const pr_deck_t *d, const pr_obj_t *o, bool sin_texto)
{
    int F = o->filas, C = o->cols;
    if (F < 1 || C < 1) return;
    const pr_tema_t *t = tema_de(d);
    int cw = o->w / C, ch = o->h / F;
    for (int f = 0; f < F; f++)
        for (int c = 0; c < C; c++) {
            int x0 = PX(s, o->x + c * cw), y0 = PX(s, o->y + f * ch), x1 = PX(s, o->x + (c + 1) * cw), y1 = PX(s, o->y + (f + 1) * ch);
            uint32_t fondo = f == 0 ? t->acento : (f & 1 ? mezclar(t->fondo1, t->acento, 28) : mezclar(t->fondo1, t->acento, 8));
            rect_c(s, x0, y0, x1, y1, fondo, 255);
            rect_c(s, x0, y0, x1, y0 + 1, t->acento, 255);
            rect_c(s, x0, y0, x0 + 1, y1, t->acento, 255);
            rect_c(s, x1 - 1, y0, x1, y1, t->acento, 255);
            rect_c(s, x0, y1 - 1, x1, y1, t->acento, 255);
            if (sin_texto) continue;
            pr_obj_t tmp; char buf[200]; uint32_t col;
            celda_obj(d, o, f, c, &tmp, buf, &col);
            texto_dib(s, d, &tmp, buf, o->x + c * cw, o->y + f * ch, false, col);
        }
}

/* ------------------------------------------------------------ objetos y diapositivas */

static void objeto_dib(sup_t *s, pr_deck_t *d, pr_obj_t *o, unsigned flags)
{
    int x = PX(s, o->x), y = PX(s, o->y), w = PX(s, o->w), h = PX(s, o->h);
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    bool sin_texto = (flags & PRD_SIN_TEXTO) != 0;
    switch (o->tipo) {
    case PR_FORMA: case PR_TEXTO: {
        if (o->tipo == PR_FORMA || o->rellena || o->borde) {
            int p[2 * 80];
            bool esq;
            int np = forma_puntos(o->forma, x * 16, y * 16, w * 16, h * 16, p, &esq);
            if (o->forma == PF_LINEA) {
                int gr = (o->borde ? o->borde : 2) * s->sc / 4096;
                if (gr < 8) gr = 8;
                trazo(s, x * 16, (y + h / 2) * 16, (x + w) * 16, (y + h / 2) * 16, gr, pr_color_borde(d, o), true);
            } else {
                if (o->rellena) poli_rellenar(s, p, np, pr_color_relleno(d, o), 255);
                if (o->borde) {
                    int gr = o->borde * s->sc / 4096;
                    if (gr < 8) gr = 8;
                    for (int i = 0; i < np; i++) { int j = i + 1 == np ? 0 : i + 1; trazo(s, p[2 * i], p[2 * i + 1], p[2 * j], p[2 * j + 1], gr, pr_color_borde(d, o), esq); }
                }
            }
        }
        const char *t = o->texto ? o->texto : "";
        if (t[0]) texto_dib(s, d, o, t, o->x, o->y, sin_texto, pr_color_texto(d, o));
        else if ((flags & PRD_EDITOR) && o->rol != PR_ROL_LIBRE && !sin_texto) {
            pr_obj_t tmp = *o;
            const char *h_ = pr_hints[o->rol == PR_ROL_TITULO ? 0 : o->rol == PR_ROL_SUBTITULO ? 2 : 1];
            tmp.lista = 0;
            texto_dib(s, d, &tmp, h_, o->x, o->y, false, mezclar(tema_de(d)->fondo1, pr_color_texto(d, o), 90));
        }
        break;
    }
    case PR_IMAGEN: {
        const imagem_t *im = pr_medio_imagen(d, o->medio);
        if (im) {
            int cx0 = x < 0 ? 0 : x, cy0 = y < 0 ? 0 : y;
            imagem_desenhar(s->px, s->w, 0, 0, s->w, s->h, im, x, y, w, h, !s->mini);
            (void)cx0; (void)cy0;
        } else {
            rect_c(s, x, y, x + w, y + h, 0xD0D0D0, 255);
            rect_c(s, x, y, x + w, y + 1, 0x808080, 255); rect_c(s, x, y + h - 1, x + w, y + h, 0x808080, 255);
            rect_c(s, x, y, x + 1, y + h, 0x808080, 255); rect_c(s, x + w - 1, y, x + w, y + h, 0x808080, 255);
        }
        if (o->borde) {
            int g = PX(s, o->borde); if (g < 1) g = 1;
            uint32_t c = pr_color_borde(d, o);
            rect_c(s, x, y, x + w, y + g, c, 255); rect_c(s, x, y + h - g, x + w, y + h, c, 255);
            rect_c(s, x, y, x + g, y + h, c, 255); rect_c(s, x + w - g, y, x + w, y + h, c, 255);
        }
        break;
    }
    case PR_WORDART: wordart_dib(s, o, x, y, w, h); break;
    case PR_TABLA: tabla_dib(s, d, o, sin_texto); break;
    case PR_GRAFICO: graf_dib(s, d, o, x, y, w, h); break;
    }
}

void pr_recorrer_textos(pr_deck_t *d, int i, pr_texto_cb cb, void *ctx)
{
    if (i < 0 || i >= d->n) return;
    pr_diap_t *sl = d->d[i];
    for (int q = 0; q < sl->n; q++) {
        pr_obj_t *o = &sl->obj[q];
        if ((o->tipo == PR_TEXTO || o->tipo == PR_FORMA) && o->texto && o->texto[0])
            cb(ctx, o, o->texto, o->x, o->y, pr_color_texto(d, o));
        else if (o->tipo == PR_TABLA && o->filas > 0 && o->cols > 0)
            for (int f = 0; f < o->filas; f++)
                for (int c = 0; c < o->cols; c++) {
                    pr_obj_t tmp; char buf[200]; uint32_t col;
                    celda_obj(d, o, f, c, &tmp, buf, &col);
                    if (buf[0]) cb(ctx, &tmp, buf, o->x + c * (o->w / o->cols), o->y + f * (o->h / o->filas), col);
                }
    }
    pr_obj_t tmp;
    memset(&tmp, 0, sizeof tmp);
    tmp.tipo = PR_TEXTO; tmp.tam = 12; tmp.col = PR_COL_TEMA; tmp.medio = -1; tmp.h = 24; tmp.valign = 1; tmp.w = (int16_t)(d->ancho / 2);
    uint32_t col = tema_de(d)->deco == 1 ? tema_de(d)->texto : mezclar(tema_de(d)->fondo2, tema_de(d)->texto, 170);
    if (d->con_pie && d->pie[0]) cb(ctx, &tmp, d->pie, d->ancho / 20, d->alto - 40, col);
    if (d->num) {
        char nb[16];
        formatar_texto(nb, sizeof nb, "%d", i + 1);
        tmp.alin = 2; tmp.w = 80;
        cb(ctx, &tmp, nb, d->ancho - 80 - d->ancho / 20, d->alto - 40, col);
    }
}

int pr_pasos(const pr_diap_t *s)
{
    int k = 0;
    for (int i = 0; i < s->n; i++) if (s->obj[i].anim) k++;
    return k;
}

static void fondo_dib(sup_t *s, pr_deck_t *d, const pr_diap_t *sl)
{
    const pr_tema_t *t = tema_de(d);
    uint32_t c1 = t->fondo1, c2 = t->fondo2;
    int tipo = sl->fondo_tipo;
    if (tipo == PR_FONDO_COLOR) { c1 = c2 = sl->fondo1; }
    else if (tipo == PR_FONDO_DEGRADADO) { c1 = sl->fondo1; c2 = sl->fondo2; }
    else if (tipo == PR_FONDO_IMAGEN) {
        const imagem_t *im = pr_medio_imagen(d, sl->fondo_medio);
        if (im) {   /* cubre toda la diapositiva sin deformar la imagen */
            int l = s->w, a = s->h;
            if ((long)im->larg * a > (long)im->alt * l) l = im->larg * a / im->alt; else a = im->alt * l / im->larg;
            imagem_desenhar(s->px, s->w, 0, 0, s->w, s->h, im, (s->w - l) / 2, (s->h - a) / 2, l, a, !s->mini);
            return;
        }
    }
    for (int y = 0; y < s->h; y++) {
        uint32_t c = c1 == c2 ? c1 : mezclar(c1, c2, s->h > 1 ? y * 255 / (s->h - 1) : 0);
        uint32_t *row = s->px + y * s->w;
        for (int x = 0; x < s->w; x++) row[x] = c;
    }
}

static void deco_dib(sup_t *s, const pr_deck_t *d, int sw, int sh)
{
    const pr_tema_t *t = tema_de(d);
    switch (t->deco) {
    case 1:
        rect_c(s, 0, PX(s, sh - 12), s->w, s->h, t->acento, 255);
        rect_c(s, 0, PX(s, sh - 16), PX(s, sw * 30 / 100), PX(s, sh - 12), t->acento2, 255);
        break;
    case 2:
        rect_c(s, 0, 0, PX(s, 16), s->h, t->acento, 255);
        rect_c(s, PX(s, 16), 0, PX(s, 20), s->h, t->acento2, 255);
        break;
    case 3: {
        rect_c(s, 0, 0, s->w, PX(s, 8), t->acento, 255);
        int x0 = PX(s, sw - 70), x1 = s->w, y1 = PX(s, 70);
        int p[6] = { x0 * 16, 0, x1 * 16, 0, x1 * 16, y1 * 16 };   /* el pico del tucán en la esquina */
        poli_rellenar(s, p, 3, t->acento2, 255);
        int p2[6] = { PX(s, sw - 40) * 16, 0, x1 * 16, 0, x1 * 16, PX(s, 40) * 16 };
        poli_rellenar(s, p2, 3, t->acento, 255);
        break;
    }
    default: break;
    }
}

static void pie_dib(sup_t *s, pr_deck_t *d, int idx, unsigned flags)
{
    if (!(d->con_pie && d->pie[0]) && !d->num) return;
    pr_obj_t tmp;
    memset(&tmp, 0, sizeof tmp);
    tmp.tipo = PR_TEXTO; tmp.tam = 12; tmp.col = PR_COL_TEMA; tmp.medio = -1;
    tmp.w = (int16_t)(d->ancho / 2); tmp.h = 24; tmp.valign = 1;
    uint32_t col = mezclar(tema_de(d)->fondo2, tema_de(d)->texto, 170);
    if (tema_de(d)->deco == 1) col = tema_de(d)->texto;
    if (d->con_pie && d->pie[0]) { tmp.alin = 0; texto_dib(s, d, &tmp, d->pie, d->ancho / 20, d->alto - 40, (flags & PRD_SIN_TEXTO) != 0, col); }
    if (d->num) {
        char nb[16];
        formatar_texto(nb, sizeof nb, "%d", idx + 1);
        tmp.alin = 2; tmp.w = 80;
        texto_dib(s, d, &tmp, nb, d->ancho - 80 - d->ancho / 20, d->alto - 40, (flags & PRD_SIN_TEXTO) != 0, col);
    }
}

void pr_dibujar(pr_deck_t *d, int i, uint32_t *px, int pw, int ph, unsigned flags, int paso, int omitir)
{
    if (i < 0 || i >= d->n) return;
    pr_diap_t *sl = d->d[i];
    sup_t s = { px, pw, ph, pw * 65536 / d->ancho, pw < 400 };
    if (!(flags & PRD_SIN_FONDO)) { fondo_dib(&s, d, sl); deco_dib(&s, d, d->ancho, d->alto); }
    int k = 0;
    for (int q = 0; q < sl->n; q++) {
        pr_obj_t *o = &sl->obj[q];
        if (o->anim) { k++; if (paso >= 0 && k > paso) continue; }
        if (q == omitir) continue;
        objeto_dib(&s, d, o, flags);
    }
    pie_dib(&s, d, i, flags);
}

void pr_dibujar_obj(pr_deck_t *d, pr_obj_t *o, uint32_t *px, int pw, int ph, unsigned flags)
{
    sup_t s = { px, pw, ph, pw * 65536 / d->ancho, pw < 400 };
    objeto_dib(&s, d, o, flags);
}
