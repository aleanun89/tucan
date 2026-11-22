// SPDX-License-Identifier: GPL-2.0-or-later
/* pdf_escribir.c - escritor de PDF mínimo (mejora 116, D291). Ver pdf_escribir.h. */
#include "pdf_escribir.h"
#include "compressao.h"
#include "pdf_metricas.h"

#define MAX_PAGINAS 2000
#define MAX_IMAGENES 256

/* Una imagen del documento (D296): RGB comprimido con Flate o un JPEG tal cual (DCTDecode). */
typedef struct { uint8_t *d; int n, pw, ph, ncomp; bool jpeg; int pag; } imgpdf_t;

typedef struct { char *d; int n, cap; } bufer_t;

#define MAX_EMB 32      /* fuentes del sistema incrustadas por documento (familia x estilo) */
typedef struct { int fam, est; bool oblicua; uint8_t usadas[32]; } emb_t;

static const fuentes_prov_t *prov;
void fuentes_prov_instalar(const fuentes_prov_t *p) { prov = p; }
const fuentes_prov_t *fuentes_prov_obter(void) { return prov; }
const char *pdfw_nombre_familia(int fam)
{
    static const char *const cl[3] = { "Arial", "Times New Roman", "Courier New" };
    if (fam >= 0 && fam < 3) return cl[fam];
    const char *n = prov && prov->nombre ? prov->nombre(fam) : NULL;
    return n ? n : cl[0];
}

struct pdfw {
    int      papel, larg, alt;
    char     titulo[128];
    bufer_t  pag[MAX_PAGINAS];
    int      npag;
    bool     fuente_usada[12];
    bool     fallo;
    imgpdf_t img[MAX_IMAGENES];
    int      nimg;
    emb_t    emb[MAX_EMB];
    int      nemb;
    /* Estado gráfico de la página en curso: evita repetir fuente y color. */
    int      f_actual, tam_actual;
    uint32_t cor_actual;
};

static void b_poner(bufer_t *b, const char *s, int n, bool *fallo)
{
    if (b->n + n + 1 > b->cap) {
        int nc = b->cap ? b->cap * 2 : 4096;
        while (nc < b->n + n + 1) nc *= 2;
        char *p = realoca(b->d, (size_t)nc);
        if (!p) { *fallo = true; return; }
        b->d = p; b->cap = nc;
    }
    memcpy(b->d + b->n, s, (size_t)n);
    b->n += n;
    b->d[b->n] = 0;
}

static void b_texto(bufer_t *b, const char *s, bool *fallo) { b_poner(b, s, (int)tam_texto(s), fallo); }

/* Número con 2 decimales a partir de centésimas. */
static int fmt_cent(char *d, long v)
{
    int n = 0;
    if (v < 0) { d[n++] = '-'; v = -v; }
    n += formatar_texto(d + n, 20, "%d.%02d", (int)(v / 100), (int)(v % 100));
    return n;
}

/* twips -> puntos con 2 decimales (twips / 20 = twips * 5 centésimas). */
static int fmt_tw(char *d, long tw) { return fmt_cent(d, tw * 5); }

static int fmt_cor(char *d, uint32_t c)
{
    int r = (int)((c >> 16) & 255) * 1000 / 255, g = (int)((c >> 8) & 255) * 1000 / 255, b = (int)(c & 255) * 1000 / 255;
    return formatar_texto(d, 40, "%d.%03d %d.%03d %d.%03d", r / 1000, r % 1000, g / 1000, g % 1000, b / 1000, b % 1000);
}

void pdfw_papel_tamanho(int papel, int *l, int *a)
{
    if (papel == PDFW_CARTA) { *l = 12240; *a = 15840; }
    else { *l = 11906; *a = 16838; }
}

/* Índice de fuente: familia*4 + estilo (sans: 0..3, serif: 4..7, mono: 8..11). */
static int idx_fonte(int fam, int est) { return (fam < 0 || fam > 2 ? 0 : fam) * 4 + (est & 3); }

static const char *const nomes_fonte[12] = {
    "Helvetica", "Helvetica-Bold", "Helvetica-Oblique", "Helvetica-BoldOblique",
    "Times-Roman", "Times-Bold", "Times-Italic", "Times-BoldItalic",
    "Courier", "Courier-Bold", "Courier-Oblique", "Courier-BoldOblique",
};

int pdfw_largura_char(int fam, int est, unsigned char c)
{
    if (c < 32) return 0;
    if (fam >= PDFW_FAM_SIS) {
        int v = prov && prov->ancho ? prov->ancho(fam, est & 3, c) : -1;
        if (v >= 0) return v;
        fam = PDFW_SANS;
    }
    int tab;
    if (fam == PDFW_MONO) tab = 6;
    else if (fam == PDFW_SERIF) tab = 2 + (est & 3);
    else tab = (est & PDFW_NEGRITA) ? 1 : 0;
    return pdf_larguras[tab][c - 32];
}

int pdfw_largura_texto(int fam, int est, int tam, const char *t, int n)
{
    long s = 0;
    for (int i = 0; i < n; i++) s += pdfw_largura_char(fam, est, (unsigned char)t[i]);
    return (int)((s * tam + 500) / 1000);
}

pdfw_t *pdfw_criar(int papel)
{
    pdfw_t *w = aloca_zerado(sizeof *w);
    if (!w) return NULL;
    w->papel = papel;
    pdfw_papel_tamanho(papel, &w->larg, &w->alt);
    return w;
}

void pdfw_tamanho(pdfw_t *w, int l, int a) { if (l > 1000 && a > 1000) { w->larg = l; w->alt = a; } }

void pdfw_liberar(pdfw_t *w)
{
    if (!w) return;
    for (int i = 0; i < w->npag; i++) libera(w->pag[i].d);
    for (int i = 0; i < w->nimg; i++) libera(w->img[i].d);
    libera(w);
}

void pdfw_titulo(pdfw_t *w, const char *t) { copia_texto(w->titulo, t ? t : "", sizeof w->titulo); }
int  pdfw_paginas(const pdfw_t *w) { return w->npag; }

bool pdfw_pagina(pdfw_t *w)
{
    if (w->npag >= MAX_PAGINAS) return false;
    w->npag++;
    w->f_actual = -1; w->tam_actual = -1; w->cor_actual = 0xFFFFFFFFu;
    return true;
}

static void cor_rellenar(pdfw_t *w, bufer_t *b, uint32_t cor, bool trazo)
{
    char t[64];
    int n = fmt_cor(t, cor);
    b_poner(b, t, n, &w->fallo);
    b_texto(b, trazo ? " RG\n" : " rg\n", &w->fallo);
}

/* Casilla de la fuente del sistema (fam, est) o -1 si no se puede incrustar (se usa la clásica parecida). */
static int emb_slot(pdfw_t *w, int fam, int est)
{
    for (int i = 0; i < w->nemb; i++) if (w->emb[i].fam == fam && w->emb[i].est == est) return i;
    pdfw_finfo_t inf;
    if (!prov || !prov->info || !prov->archivo || w->nemb >= MAX_EMB || !prov->info(fam, est, &inf)) return -1;
    emb_t *e = &w->emb[w->nemb];
    memset(e, 0, sizeof *e);
    e->fam = fam; e->est = est; e->oblicua = inf.oblicua;
    return w->nemb++;
}

void pdfw_texto(pdfw_t *w, int x, int y, int fam, int est, int tam, uint32_t cor,
                const char *t, int n, int extra)
{
    if (w->npag == 0 || n <= 0) return;
    bufer_t *b = &w->pag[w->npag - 1];
    int f, e = -1;
    if (fam >= PDFW_FAM_SIS) {
        e = emb_slot(w, fam, est & 3);
        if (e < 0) { fam = prov && prov->clase ? prov->clase(fam) : 0; if (fam < 0 || fam > 2) fam = 0; }
    }
    if (e >= 0) {
        f = 12 + e;
        for (int i = 0; i < n; i++) { unsigned char c = (unsigned char)t[i]; w->emb[e].usadas[c >> 3] |= (uint8_t)(1u << (c & 7)); }
    } else {
        f = idx_fonte(fam, est);
        w->fuente_usada[f] = true;
    }
    char num[48];
    b_texto(b, "BT\n", &w->fallo);
    if (cor != w->cor_actual) { cor_rellenar(w, b, cor, false); w->cor_actual = cor; }
    int k = formatar_texto(num, sizeof num, "/F%d ", f + 1);
    b_poner(b, num, k, &w->fallo);
    k = fmt_tw(num, tam); b_poner(b, num, k, &w->fallo);
    b_texto(b, " Tf\n", &w->fallo);
    k = fmt_tw(num, extra); b_poner(b, num, k, &w->fallo);
    b_texto(b, e >= 0 && w->emb[e].oblicua ? " Tw\n1 0 0.2126 1 " : " Tw\n1 0 0 1 ", &w->fallo);
    k = fmt_tw(num, x); b_poner(b, num, k, &w->fallo);
    b_poner(b, " ", 1, &w->fallo);
    k = fmt_tw(num, w->alt - y); b_poner(b, num, k, &w->fallo);
    b_texto(b, " Tm\n(", &w->fallo);
    for (int i = 0; i < n; i++) {
        unsigned char c = (unsigned char)t[i];
        if (c == '(' || c == ')' || c == '\\') { char e[2] = { '\\', (char)c }; b_poner(b, e, 2, &w->fallo); }
        else if (c < 32 || c >= 127) {
            if (c < 32) continue;
            char e[4] = { '\\', (char)('0' + (c >> 6)), (char)('0' + ((c >> 3) & 7)), (char)('0' + (c & 7)) };
            b_poner(b, e, 4, &w->fallo);
        } else b_poner(b, (const char *)&c, 1, &w->fallo);
    }
    b_texto(b, ") Tj\nET\n", &w->fallo);
}

void pdfw_retangulo(pdfw_t *w, int x, int y, int l, int a, uint32_t cor)
{
    if (w->npag == 0) return;
    bufer_t *b = &w->pag[w->npag - 1];
    char num[48];
    if (cor != w->cor_actual) { cor_rellenar(w, b, cor, false); w->cor_actual = cor; }
    int k = fmt_tw(num, x); b_poner(b, num, k, &w->fallo); b_poner(b, " ", 1, &w->fallo);
    k = fmt_tw(num, w->alt - y - a); b_poner(b, num, k, &w->fallo); b_poner(b, " ", 1, &w->fallo);
    k = fmt_tw(num, l); b_poner(b, num, k, &w->fallo); b_poner(b, " ", 1, &w->fallo);
    k = fmt_tw(num, a); b_poner(b, num, k, &w->fallo);
    b_texto(b, " re f\n", &w->fallo);
}

void pdfw_linha(pdfw_t *w, int x0, int y0, int x1, int y1, int esp, uint32_t cor)
{
    if (w->npag == 0) return;
    bufer_t *b = &w->pag[w->npag - 1];
    char num[48];
    cor_rellenar(w, b, cor, true);
    int k = fmt_tw(num, esp); b_poner(b, num, k, &w->fallo); b_texto(b, " w\n", &w->fallo);
    int v[4] = { x0, w->alt - y0, x1, w->alt - y1 };
    static const char *const sep[4] = { " ", " m ", " ", " l\nS\n" };
    for (int i = 0; i < 4; i++) {
        k = fmt_tw(num, v[i]); b_poner(b, num, k, &w->fallo);
        b_texto(b, sep[i], &w->fallo);
    }
}


/* Coloca la imagen n en la página actual: x, y (arriba a la izquierda), l, a en twips. */
static bool img_colocar(pdfw_t *w, int x, int y, int l, int a, int k)
{
    bufer_t *b = &w->pag[w->npag - 1];
    char num[48];
    b_texto(b, "q\n", &w->fallo);
    /* matriz: l 0 0 a x y cm (el origen del PDF está abajo a la izquierda) */
    int n = fmt_tw(num, l); b_poner(b, num, n, &w->fallo); b_texto(b, " 0 0 ", &w->fallo);
    n = fmt_tw(num, a); b_poner(b, num, n, &w->fallo); b_texto(b, " ", &w->fallo);
    n = fmt_tw(num, x); b_poner(b, num, n, &w->fallo); b_texto(b, " ", &w->fallo);
    n = fmt_tw(num, w->alt - y - a); b_poner(b, num, n, &w->fallo); b_texto(b, " cm\n", &w->fallo);
    n = formatar_texto(num, sizeof num, "/Im%d Do\nQ\n", k + 1);
    b_poner(b, num, n, &w->fallo);
    return !w->fallo;
}

bool pdfw_imagem_rgb(pdfw_t *w, int x, int y, int l, int a, const uint32_t *px, int pw, int ph)
{
    if (w->npag == 0 || w->nimg >= MAX_IMAGENES || pw <= 0 || ph <= 0 || !px || l <= 0 || a <= 0) return false;
    long bruto = 3L * pw * ph;
    if (bruto > 0x3FFFFFFF) return false;
    uint8_t *raw = aloca((size_t)bruto);
    if (!raw) return false;
    for (long i = 0, k = 0; i < (long)pw * ph; i++) {
        raw[k++] = (uint8_t)(px[i] >> 16); raw[k++] = (uint8_t)(px[i] >> 8); raw[k++] = (uint8_t)px[i];
    }
    int lim = deflar_limite((int)bruto) + 16;
    uint8_t *z = aloca((size_t)lim);
    if (!z) { libera(raw); return false; }
    z[0] = 0x78; z[1] = 0x9C;
    int r = deflar(raw, (int)bruto, z + 2, lim - 6, 6);
    if (r < 0) { libera(raw); libera(z); return false; }
    int nz = 2 + r;
    uint32_t ad = adler32_somar(1, raw, (size_t)bruto);
    libera(raw);
    z[nz++] = (uint8_t)(ad >> 24); z[nz++] = (uint8_t)(ad >> 16); z[nz++] = (uint8_t)(ad >> 8); z[nz++] = (uint8_t)ad;
    imgpdf_t *im = &w->img[w->nimg];
    im->d = z; im->n = nz; im->pw = pw; im->ph = ph; im->ncomp = 3; im->jpeg = false; im->pag = w->npag - 1;
    return img_colocar(w, x, y, l, a, w->nimg++);
}

bool pdfw_imagem_jpeg(pdfw_t *w, int x, int y, int l, int a, const uint8_t *jpg, int n, int pw, int ph, int ncomp)
{
    if (w->npag == 0 || w->nimg >= MAX_IMAGENES || n <= 0 || !jpg || l <= 0 || a <= 0) return false;
    if (ncomp != 1 && ncomp != 3) return false;
    uint8_t *c = aloca((size_t)n);
    if (!c) return false;
    memcpy(c, jpg, (size_t)n);
    imgpdf_t *im = &w->img[w->nimg];
    im->d = c; im->n = n; im->pw = pw; im->ph = ph; im->ncomp = ncomp; im->jpeg = true; im->pag = w->npag - 1;
    return img_colocar(w, x, y, l, a, w->nimg++);
}

/* Escribe un objeto "N 0 obj ... endobj" y apunta su posición. */
static void obj_ini(bufer_t *o, int *pos, int num, bool *fallo)
{
    char t[40];
    pos[num] = o->n;
    int k = formatar_texto(t, sizeof t, "%d 0 obj\n", num);
    b_poner(o, t, k, fallo);
}

/* Un flujo comprimido con Flate: "N 0 obj << extra /Length n /Filter /FlateDecode >> stream ...". */
static void flujo_flate(bufer_t *o, int *pos, int num, const char *extra, const uint8_t *d, int n, bool *f)
{
    int lim = deflar_limite(n) + 16;
    uint8_t *z = aloca((size_t)lim);
    if (!z) { *f = true; return; }
    z[0] = 0x78; z[1] = 0x9C;
    int r = deflar(d, n, z + 2, lim - 6, 6);
    if (r < 0) { libera(z); *f = true; return; }
    int nz = 2 + r;
    uint32_t ad = adler32_somar(1, d, (size_t)n);
    z[nz++] = (uint8_t)(ad >> 24); z[nz++] = (uint8_t)(ad >> 16); z[nz++] = (uint8_t)(ad >> 8); z[nz++] = (uint8_t)ad;
    char t[160];
    obj_ini(o, pos, num, f);
    int k = formatar_texto(t, sizeof t, "<< %s/Length %d /Filter /FlateDecode >>\nstream\n", extra, nz);
    b_poner(o, t, k, f);
    b_poner(o, (const char *)z, nz, f);
    b_texto(o, "\nendstream\nendobj\n", f);
    libera(z);
}

/* La fuente incrustada n (D319): diccionario (num), descriptor (num+1) y archivo (num+2). TrueType:
 * subconjunto en FontFile2; OpenType CFF: la fuente entera en FontFile3 /OpenType. WinAnsi y Widths de 32 a 255. */
static void emb_escribir(pdfw_t *w, bufer_t *o, int *pos, int num, const emb_t *e)
{
    bool *f = &w->fallo;
    pdfw_finfo_t inf;
    int n = 0, original = 0;
    uint8_t *arq = NULL;
    if (prov && prov->info && prov->info(e->fam, e->est, &inf)) arq = prov->archivo(e->fam, e->est, e->usadas, &n, &original);
    if (!arq) { *f = true; return; }
    char t[400], base[80];
    /* nombre con prefijo de subconjunto (seis letras fijas por familia/estilo) en TrueType */
    if (!inf.cff) {
        unsigned h = (unsigned)(e->fam * 2654435761u + (unsigned)e->est * 40503u);
        for (int i = 0; i < 6; i++) { base[i] = (char)('A' + (h % 26)); h = h / 26 + (unsigned)i * 7919u; }
        base[6] = '+';
        copia_texto(base + 7, inf.psname, sizeof base - 7);
    } else copia_texto(base, inf.psname, sizeof base);
    obj_ini(o, pos, num, f);
    int k = formatar_texto(t, sizeof t, "<< /Type /Font /Subtype /%s /BaseFont /%s /FirstChar 32 /LastChar 255 /Encoding /WinAnsiEncoding /FontDescriptor %d 0 R /Widths [",
                           inf.cff ? "Type1" : "TrueType", base, num + 1);
    b_poner(o, t, k, f);
    for (int c = 32; c < 256; c++) {
        int a = prov->ancho ? prov->ancho(e->fam, e->est, (unsigned char)c) : 0;
        k = formatar_texto(t, sizeof t, "%d ", a < 0 ? 0 : a);
        b_poner(o, t, k, f);
    }
    b_texto(o, "] >>\nendobj\n", f);
    obj_ini(o, pos, num + 1, f);
    k = formatar_texto(t, sizeof t, "<< /Type /FontDescriptor /FontName /%s /Flags %d /FontBBox [%d %d %d %d] /ItalicAngle %d /Ascent %d /Descent %d /CapHeight %d /StemV %d /%s %d 0 R >>\nendobj\n",
                       base, inf.flags, inf.bbox[0], inf.bbox[1], inf.bbox[2], inf.bbox[3], inf.angulo, inf.ascent, inf.descent, inf.capheight, inf.stemv,
                       inf.cff ? "FontFile3" : "FontFile2", num + 2);
    b_poner(o, t, k, f);
    char ex[64];
    if (inf.cff) copia_texto(ex, "/Subtype /OpenType ", sizeof ex);
    else formatar_texto(ex, sizeof ex, "/Length1 %d ", original ? original : n);
    flujo_flate(o, pos, num + 2, ex, arq, n, f);
    libera(arq);
}

bool pdfw_finalizar(pdfw_t *w, uint8_t **dados, int *tam)
{
    if (w->npag == 0 || w->fallo) return false;
    int img_base = 16 + 2 * w->npag;       /* las imágenes van después de las páginas */
    int emb_base = img_base + w->nimg;       /* luego, 3 objetos por fuente incrustada: diccionario, descriptor y archivo */
    int nobj = 3 + 12 + 2 * w->npag + w->nimg + 3 * w->nemb;   /* 1 catálogo, 2 páginas, 3 info, 4..15 fuentes, luego contenido+página */
    int *pos = aloca_zerado(sizeof(int) * (size_t)(nobj + 1));
    if (!pos) return false;
    bufer_t o = { 0, 0, 0 };
    bool *f = &w->fallo;
    char t[256];
    b_texto(&o, "%PDF-1.4\n%\xE2\xE3\xCF\xD3\n", f);
    obj_ini(&o, pos, 1, f);
    b_texto(&o, "<< /Type /Catalog /Pages 2 0 R >>\nendobj\n", f);
    obj_ini(&o, pos, 2, f);
    b_texto(&o, "<< /Type /Pages /Kids [", f);
    for (int i = 0; i < w->npag; i++) {
        int k = formatar_texto(t, sizeof t, "%d 0 R ", 17 + 2 * i);
        b_poner(&o, t, k, f);
    }
    int k = formatar_texto(t, sizeof t, "] /Count %d >>\nendobj\n", w->npag);
    b_poner(&o, t, k, f);
    obj_ini(&o, pos, 3, f);
    b_texto(&o, "<< /Producer (SaguiOS Pluma) /Title (", f);
    for (const char *p = w->titulo; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c == '(' || c == ')' || c == '\\') b_poner(&o, "\\", 1, f);
        if (c >= 32 && c < 127) b_poner(&o, (const char *)&c, 1, f);
        else if (c >= 127) { char e[4] = { '\\', (char)('0' + (c >> 6)), (char)('0' + ((c >> 3) & 7)), (char)('0' + (c & 7)) }; b_poner(&o, e, 4, f); }
    }
    b_texto(&o, ") >>\nendobj\n", f);
    for (int i = 0; i < 12; i++) {
        obj_ini(&o, pos, 4 + i, f);
        k = formatar_texto(t, sizeof t, "<< /Type /Font /Subtype /Type1 /BaseFont /%s /Encoding /WinAnsiEncoding >>\nendobj\n", nomes_fonte[i]);
        b_poner(&o, t, k, f);
    }
    for (int p = 0; p < w->npag && !*f; p++) {
        bufer_t *c = &w->pag[p];
        int lim = deflar_limite(c->n) + 16;
        uint8_t *z = aloca((size_t)lim);
        if (!z) { *f = true; break; }
        int nz = 2;
        z[0] = 0x78; z[1] = 0x9C;
        int r = deflar((const uint8_t *)c->d, c->n, z + 2, lim - 6, 6);
        if (r < 0) { libera(z); *f = true; break; }
        nz += r;
        uint32_t ad = adler32_somar(1, c->d, (size_t)c->n);
        z[nz++] = (uint8_t)(ad >> 24); z[nz++] = (uint8_t)(ad >> 16); z[nz++] = (uint8_t)(ad >> 8); z[nz++] = (uint8_t)ad;
        obj_ini(&o, pos, 16 + 2 * p, f);
        k = formatar_texto(t, sizeof t, "<< /Length %d /Filter /FlateDecode >>\nstream\n", nz);
        b_poner(&o, t, k, f);
        b_poner(&o, (const char *)z, nz, f);
        b_texto(&o, "\nendstream\nendobj\n", f);
        libera(z);
        obj_ini(&o, pos, 17 + 2 * p, f);
        b_texto(&o, "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 ", f);
        char a[24];
        k = fmt_tw(a, w->larg); b_poner(&o, a, k, f); b_poner(&o, " ", 1, f);
        k = fmt_tw(a, w->alt); b_poner(&o, a, k, f);
        b_texto(&o, "] /Resources << /Font << ", f);
        for (int i = 0; i < 12; i++) {
            k = formatar_texto(t, sizeof t, "/F%d %d 0 R ", i + 1, 4 + i);
            b_poner(&o, t, k, f);
        }
        for (int i = 0; i < w->nemb; i++) {
            k = formatar_texto(t, sizeof t, "/F%d %d 0 R ", 13 + i, emb_base + 3 * i);
            b_poner(&o, t, k, f);
        }
        b_texto(&o, ">> ", f);
        bool hay_img = false;
        for (int q = 0; q < w->nimg; q++) if (w->img[q].pag == p) hay_img = true;
        if (hay_img) {
            b_texto(&o, "/XObject << ", f);
            for (int q = 0; q < w->nimg; q++) if (w->img[q].pag == p) {
                k = formatar_texto(t, sizeof t, "/Im%d %d 0 R ", q + 1, img_base + q);
                b_poner(&o, t, k, f);
            }
            b_texto(&o, ">> ", f);
        }
        k = formatar_texto(t, sizeof t, ">> /Contents %d 0 R >>\nendobj\n", 16 + 2 * p);
        b_poner(&o, t, k, f);
    }
    for (int q = 0; q < w->nimg && !*f; q++) {
        const imgpdf_t *im = &w->img[q];
        obj_ini(&o, pos, img_base + q, f);
        k = formatar_texto(t, sizeof t, "<< /Type /XObject /Subtype /Image /Width %d /Height %d /ColorSpace /%s /BitsPerComponent 8 /Filter /%s /Length %d >>\nstream\n",
                           im->pw, im->ph, im->ncomp == 1 ? "DeviceGray" : "DeviceRGB", im->jpeg ? "DCTDecode" : "FlateDecode", im->n);
        b_poner(&o, t, k, f);
        b_poner(&o, (const char *)im->d, im->n, f);
        b_texto(&o, "\nendstream\nendobj\n", f);
    }
    for (int q = 0; q < w->nemb && !*f; q++) emb_escribir(w, &o, pos, emb_base + 3 * q, &w->emb[q]);
    int xref = o.n;
    k = formatar_texto(t, sizeof t, "xref\n0 %d\n0000000000 65535 f \n", nobj + 1);
    b_poner(&o, t, k, f);
    for (int i = 1; i <= nobj; i++) {
        k = formatar_texto(t, sizeof t, "%010d 00000 n \n", pos[i]);
        b_poner(&o, t, k, f);
    }
    k = formatar_texto(t, sizeof t, "trailer\n<< /Size %d /Root 1 0 R /Info 3 0 R >>\nstartxref\n%d\n%%%%EOF\n", nobj + 1, xref);
    b_poner(&o, t, k, f);
    libera(pos);
    if (*f) { libera(o.d); return false; }
    *dados = (uint8_t *)o.d;
    *tam = o.n;
    return true;
}
