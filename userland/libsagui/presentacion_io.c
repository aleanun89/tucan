// SPDX-License-Identifier: GPL-2.0-or-later
/* presentacion_io.c - formato propio de Tucán (.tcn), exportación a PDF y PNG (D369).
 *
 * El formato .tcn es un ZIP con «tucan.txt» (texto Latin-1, una línea por elemento; ver docs/tucan.md) y las
 * imágenes en «media/mN.png|jpg». El mismo texto sirve para deshacer/rehacer en memoria. */
#include "presentacion.h"
#include "pdf_escribir.h"
#include "compressao.h"

char pr_avisos[240];

const char *pr_err_texto(int e)
{
    switch (e) {
    case PR_ERR_FORMATO: return "formato no reconocido";
    case PR_ERR_MEMORIA: return "sin memoria";
    case PR_ERR_ARCHIVO: return "no se pudo leer o escribir el archivo";
    default: return "";
    }
}

/* ------------------------------------------------------------ texto interno */

static int b_txt(buf_t *b, const char *s) { return comp_buf_somar(b, s, (int)tam_texto(s)); }

static int b_esc(buf_t *b, const char *s)
{
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        char t[8]; int n;
        if (*p == '\\') n = formatar_texto(t, sizeof t, "\\\\");
        else if (*p == ' ') n = formatar_texto(t, sizeof t, "\\s");
        else if (*p == '\n') n = formatar_texto(t, sizeof t, "\\n");
        else if (*p == '\t') n = formatar_texto(t, sizeof t, "\\t");
        else if (*p < 0x20) n = formatar_texto(t, sizeof t, "\\x%02x", *p);
        else { t[0] = (char)*p; n = 1; }
        if (comp_buf_somar(b, t, n)) return -1;
    }
    return 0;
}

static void desescapar(const char *s, int n, char *d, int max)
{
    int k = 0;
    for (int i = 0; i < n && k < max - 1; i++) {
        if (s[i] != '\\' || i + 1 >= n) { d[k++] = s[i]; continue; }
        char c = s[++i];
        if (c == 's') d[k++] = ' ';
        else if (c == 'n') d[k++] = '\n';
        else if (c == 't') d[k++] = '\t';
        else if (c == 'x' && i + 2 < n + 0) {
            int v = 0;
            for (int q = 1; q <= 2; q++) { char h = s[i + q]; v = v * 16 + (h >= 'a' ? h - 'a' + 10 : h >= 'A' ? h - 'A' + 10 : h - '0'); }
            d[k++] = (char)v; i += 2;
        } else d[k++] = c;
    }
    d[k] = 0;
}

static int b_fmt(buf_t *b, const char *fmt, ...)
{
    char t[512];
    va_list ap;
    va_start(ap, fmt);
    int n = formatar_lista(t, sizeof t, fmt, ap);
    va_end(ap);
    if (n < 0) return -1;
    if (n > (int)sizeof t - 1) n = (int)sizeof t - 1;
    return comp_buf_somar(b, t, n);
}

char *pr_a_texto(const pr_deck_t *d, int *nout)
{
    buf_t b = { 0 };
    int e = 0;
    e |= b_txt(&b, "TUCAN 1\n");
    e |= b_fmt(&b, "deck ancho=%d alto=%d tema=%d num=%d pie=%d auto=%d ftit=", d->ancho, d->alto, d->tema, d->num, d->con_pie, d->auto_avance);
    e |= b_esc(&b, d->f_titulo); e |= b_txt(&b, " fcue="); e |= b_esc(&b, d->f_cuerpo); e |= b_txt(&b, "\n");
    e |= b_txt(&b, "pietxt "); e |= b_esc(&b, d->pie); e |= b_txt(&b, "\n");
    for (int i = 0; i < d->n; i++) {
        const pr_diap_t *s = d->d[i];
        e |= b_fmt(&b, "diap layout=%d trans=%d oculta=%d fondo=%d f1=%06x f2=%06x fm=%d auto=%d dur=%d\n",
                   s->layout, s->trans, s->oculta, s->fondo_tipo, (unsigned)s->fondo1, (unsigned)s->fondo2, s->fondo_medio, s->auto_seg, s->dur_trans);
        if (s->notas && s->notas[0]) { e |= b_txt(&b, "notas "); e |= b_esc(&b, s->notas); e |= b_txt(&b, "\n"); }
        for (int k = 0; k < s->n; k++) {
            const pr_obj_t *o = &s->obj[k];
            e |= b_fmt(&b, "obj tipo=%d forma=%d rol=%d anim=%d alin=%d va=%d lista=%d est=%d tam=%d borde=%d rell=%d wa=%d graf=%d filas=%d cols=%d "
                       "x=%d y=%d w=%d h=%d g=%d medio=%d relleno=%x bcol=%x col=%x fuente=",
                       o->tipo, o->forma, o->rol, o->anim, o->alin, o->valign, o->lista, o->est, o->tam, o->borde, o->rellena, o->wa_estilo, o->graf, o->filas, o->cols,
                       o->x, o->y, o->w, o->h, o->grupo, o->medio, (unsigned)o->relleno, (unsigned)o->borde_col, (unsigned)o->col);
            e |= b_esc(&b, o->fuente);
            e |= b_txt(&b, "\n");
            if (o->texto && o->texto[0]) { e |= b_txt(&b, "txt "); e |= b_esc(&b, o->texto); e |= b_txt(&b, "\n"); }
        }
    }
    if (e) { comp_buf_liberar(&b); return NULL; }
    char *r = aloca((size_t)b.n + 1);
    if (!r) { comp_buf_liberar(&b); return NULL; }
    memcpy(r, b.d, (size_t)b.n);
    r[b.n] = 0;
    if (nout) *nout = b.n;
    comp_buf_liberar(&b);
    return r;
}

static unsigned hex_de(const char *s)
{
    unsigned v = 0;
    for (; (*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F'); s++)
        v = v * 16 + (unsigned)(*s <= '9' ? *s - '0' : (*s | 0x20) - 'a' + 10);
    return v;
}
static int dec_de(const char *s)
{
    int neg = 0, v = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return neg ? -v : v;
}

/* Busca «clave=» en una línea de tokens separados por espacios; devuelve el valor (sin escapar) y su largo. */
static const char *clave(const char *l, int n, const char *k, int *vn)
{
    int kl = (int)tam_texto(k);
    for (int i = 0; i < n; ) {
        int j = i;
        while (j < n && l[j] != ' ') j++;
        if (j - i > kl && l[i + kl] == '=' && memcmp(l + i, k, (size_t)kl) == 0) { *vn = j - i - kl - 1; return l + i + kl + 1; }
        i = j + 1;
    }
    *vn = 0;
    return NULL;
}
static int num_clave(const char *l, int n, const char *k, int defecto)
{
    int vn; const char *v = clave(l, n, k, &vn);
    if (!v) return defecto;
    char t[24]; if (vn > 23) vn = 23; memcpy(t, v, (size_t)vn); t[vn] = 0;
    return dec_de(t);
}
static unsigned hex_clave(const char *l, int n, const char *k, unsigned defecto)
{
    int vn; const char *v = clave(l, n, k, &vn);
    if (!v) return defecto;
    char t[24]; if (vn > 23) vn = 23; memcpy(t, v, (size_t)vn); t[vn] = 0;
    return hex_de(t);
}

pr_deck_t *pr_de_texto(const char *t, int n, pr_deck_t *base)
{
    pr_deck_t *d = pr_deck_vacio(true);
    if (!d) return NULL;
    if (base) {   /* se queda con las imágenes de la anterior */
        d->medios = base->medios; d->n_medios = base->n_medios;
        base->medios = NULL; base->n_medios = 0;
    }
    pr_diap_t *s = NULL;
    pr_obj_t *o = NULL;
    int pos = 0;
    bool cabecera = false;
    while (pos < n) {
        int fin = pos;
        while (fin < n && t[fin] != '\n') fin++;
        const char *l = t + pos;
        int ln = fin - pos;
        pos = fin + 1;
        if (ln <= 0) continue;
        if (ln >= 5 && memcmp(l, "TUCAN", 5) == 0) { cabecera = true; continue; }
        if (!cabecera) { pr_liberar(d); return NULL; }
        if (ln >= 5 && memcmp(l, "deck ", 5) == 0) {
            d->ancho = num_clave(l, ln, "ancho", 960); d->alto = num_clave(l, ln, "alto", 540);
            if (d->ancho != 720) d->ancho = 960;
            d->alto = 540;
            d->tema = num_clave(l, ln, "tema", 0);
            if (d->tema < 0 || d->tema >= pr_n_temas()) d->tema = 0;
            d->num = num_clave(l, ln, "num", 0) != 0; d->con_pie = num_clave(l, ln, "pie", 0) != 0; d->auto_avance = num_clave(l, ln, "auto", 0) != 0;
            int vn; const char *v = clave(l, ln, "ftit", &vn);
            if (v) desescapar(v, vn, d->f_titulo, sizeof d->f_titulo);
            v = clave(l, ln, "fcue", &vn);
            if (v) desescapar(v, vn, d->f_cuerpo, sizeof d->f_cuerpo);
        } else if (ln >= 6 && memcmp(l, "pietxt", 6) == 0) {
            desescapar(l + (ln > 7 ? 7 : ln), ln > 7 ? ln - 7 : 0, d->pie, sizeof d->pie);
        } else if (ln >= 5 && memcmp(l, "diap ", 5) == 0) {
            s = pr_diap_vacia(d);
            if (!s) { pr_liberar(d); return NULL; }
            o = NULL;
            s->layout = (uint8_t)num_clave(l, ln, "layout", 0); s->trans = (uint8_t)num_clave(l, ln, "trans", 0);
            s->oculta = (uint8_t)num_clave(l, ln, "oculta", 0); s->fondo_tipo = (uint8_t)num_clave(l, ln, "fondo", 0);
            s->fondo1 = hex_clave(l, ln, "f1", 0xFFFFFF); s->fondo2 = hex_clave(l, ln, "f2", 0xCCCCCC);
            s->fondo_medio = num_clave(l, ln, "fm", -1); s->auto_seg = num_clave(l, ln, "auto", 0);
            s->dur_trans = (uint8_t)num_clave(l, ln, "dur", 0);
            if (s->trans >= PR_TRANS_N) s->trans = 0;
        } else if (s && ln >= 5 && memcmp(l, "notas", 5) == 0) {
            char *tx = aloca((size_t)ln + 1);
            if (!tx) { pr_liberar(d); return NULL; }
            desescapar(l + (ln > 6 ? 6 : ln), ln > 6 ? ln - 6 : 0, tx, ln + 1);
            libera(s->notas); s->notas = tx;
        } else if (s && ln >= 4 && memcmp(l, "obj ", 4) == 0) {
            int tipo = num_clave(l, ln, "tipo", 0);
            if (tipo < PR_TEXTO || tipo > PR_GRAFICO) { o = NULL; continue; }
            o = pr_obj_nuevo(s, tipo);
            if (!o) continue;
            o->forma = (uint8_t)num_clave(l, ln, "forma", 0); o->rol = (uint8_t)num_clave(l, ln, "rol", 0); o->anim = (uint8_t)num_clave(l, ln, "anim", 0);
            o->alin = (uint8_t)num_clave(l, ln, "alin", 0); o->valign = (uint8_t)num_clave(l, ln, "va", 0); o->lista = (uint8_t)num_clave(l, ln, "lista", 0);
            o->est = (uint8_t)num_clave(l, ln, "est", 0); o->tam = (uint8_t)num_clave(l, ln, "tam", 18); o->borde = (uint8_t)num_clave(l, ln, "borde", 0);
            o->rellena = (uint8_t)num_clave(l, ln, "rell", 0); o->wa_estilo = (uint8_t)num_clave(l, ln, "wa", 0); o->graf = (uint8_t)num_clave(l, ln, "graf", 0);
            o->filas = (uint8_t)num_clave(l, ln, "filas", 0); o->cols = (uint8_t)num_clave(l, ln, "cols", 0);
            o->x = (int16_t)num_clave(l, ln, "x", 0); o->y = (int16_t)num_clave(l, ln, "y", 0);
            o->w = (int16_t)num_clave(l, ln, "w", 100); o->h = (int16_t)num_clave(l, ln, "h", 50);
            o->grupo = (int16_t)num_clave(l, ln, "g", 0); o->medio = num_clave(l, ln, "medio", -1);
            if (o->medio >= d->n_medios) o->medio = -1;
            o->relleno = hex_clave(l, ln, "relleno", PR_COL_TEMA); o->borde_col = hex_clave(l, ln, "bcol", PR_COL_TEMA); o->col = hex_clave(l, ln, "col", PR_COL_TEMA);
            int vn; const char *v = clave(l, ln, "fuente", &vn);
            if (v) desescapar(v, vn, o->fuente, sizeof o->fuente);
        } else if (o && ln >= 3 && memcmp(l, "txt", 3) == 0) {
            char *tx = aloca((size_t)ln + 1);
            if (!tx) { pr_liberar(d); return NULL; }
            desescapar(l + (ln > 4 ? 4 : ln), ln > 4 ? ln - 4 : 0, tx, ln + 1);
            pr_obj_poner_texto(o, tx);
            libera(tx);
        }
    }
    if (!cabecera) { pr_liberar(d); return NULL; }
    if (d->n == 0 && !pr_diap_nueva(d, 0, PR_LY_TITULO)) { pr_liberar(d); return NULL; }
    return d;
}

/* ------------------------------------------------------------ .tcn (ZIP) */

int pr_guardar(const pr_deck_t *d, const char *ruta)
{
    int nt = 0;
    char *txt = pr_a_texto(d, &nt);
    if (!txt) return PR_ERR_MEMORIA;
    zip_escritor_t z;
    memset(&z, 0, sizeof z);
    int e = zip_adicionar(&z, "tucan.txt", (const uint8_t *)txt, (uint32_t)nt, 1700000000u, 6, false, 0644);
    libera(txt);
    for (int i = 0; i < d->n_medios && !e; i++) {
        char nom[32];
        formatar_texto(nom, sizeof nom, "media/m%d.%s", i, d->medios[i].ext);
        e = zip_adicionar(&z, nom, d->medios[i].dados, (uint32_t)d->medios[i].n, 1700000000u, 0, false, 0644);
    }
    if (!e) e = zip_concluir(&z);
    int r = 0;
    if (e) r = PR_ERR_MEMORIA;
    else if (salvar_arquivo(ruta, z.b.d, z.b.n) != 0) r = PR_ERR_ARCHIVO;
    zip_escritor_liberar(&z);
    return r;
}

static pr_deck_t *abrir_tcn(const uint8_t *dat, int n, int *err)
{
    zip_leitor_t z;
    if (zip_abrir(&z, dat, n) != 0) { *err = PR_ERR_FORMATO; return NULL; }
    int it = -1;
    for (int i = 0; i < z.cnt; i++) if (compara_texto(z.e[i].nome, "tucan.txt") == 0) it = i;
    if (it < 0) { zip_leitor_liberar(&z); *err = PR_ERR_FORMATO; return NULL; }
    pr_deck_t *tmp = pr_deck_vacio(true);
    if (!tmp) { zip_leitor_liberar(&z); *err = PR_ERR_MEMORIA; return NULL; }
    /* las imágenes van en orden m0, m1... (el índice es el que usan los objetos) */
    for (int k = 0; k < PR_MEDIOS_MAX; k++) {
        int hit = -1;
        char a[32], b[32];
        formatar_texto(a, sizeof a, "media/m%d.png", k); formatar_texto(b, sizeof b, "media/m%d.jpg", k);
        for (int i = 0; i < z.cnt; i++) if (compara_texto(z.e[i].nome, a) == 0 || compara_texto(z.e[i].nome, b) == 0) hit = i;
        if (hit < 0) break;
        uint8_t *dd = NULL;
        int ln = zip_extrair(&z, hit, &dd);
        if (ln < 0 || !dd) break;
        pr_medio_t *nv = realoca(tmp->medios, (size_t)(tmp->n_medios + 1) * sizeof *nv);
        if (!nv) { libera(dd); break; }
        tmp->medios = nv;
        pr_medio_t *m = &tmp->medios[tmp->n_medios++];
        memset(m, 0, sizeof *m);
        m->dados = dd; m->n = ln;
        copia_texto(m->ext, z.e[hit].nome[tam_texto(z.e[hit].nome) - 1] == 'g' && z.e[hit].nome[tam_texto(z.e[hit].nome) - 2] == 'p' ? "jpg" : "png", sizeof m->ext);
    }
    uint8_t *tx = NULL;
    int tn = zip_extrair(&z, it, &tx);
    pr_deck_t *d = NULL;
    if (tn >= 0 && tx) d = pr_de_texto((const char *)tx, tn, tmp);
    libera(tx);
    pr_liberar(tmp);
    zip_leitor_liberar(&z);
    if (!d) *err = PR_ERR_FORMATO;
    return d;
}

pr_deck_t *pr_abrir(const char *ruta, int *err)
{
    int e0;
    if (!err) err = &e0;
    *err = 0;
    pr_avisos[0] = 0;
    int n = 0;
    uint8_t *dat = carregar_arquivo(ruta, &n);
    if (!dat) { *err = PR_ERR_ARCHIVO; return NULL; }
    pr_deck_t *d = NULL;
    if (n > 4 && dat[0] == 'P' && dat[1] == 'K') {
        zip_leitor_t z;
        bool pptx = false;
        if (zip_abrir(&z, dat, n) == 0) {
            for (int i = 0; i < z.cnt; i++) if (compara_texto(z.e[i].nome, "ppt/presentation.xml") == 0) pptx = true;
            zip_leitor_liberar(&z);
        }
        d = pptx ? pr_importar_pptx(dat, n, err, pr_avisos, sizeof pr_avisos) : abrir_tcn(dat, n, err);
    } else if (n > 6 && memcmp(dat, "TUCAN", 5) == 0) {
        d = pr_de_texto((const char *)dat, n, NULL);
        if (!d) *err = PR_ERR_FORMATO;
    } else *err = PR_ERR_FORMATO;
    libera(dat);
    return d;
}

/* ------------------------------------------------------------ PNG */

int pr_exportar_png(pr_deck_t *d, int i, const char *ruta, int ancho)
{
    if (i < 0 || i >= d->n) return PR_ERR_FORMATO;
    if (ancho < 64) ancho = 64;
    if (ancho > 2000) ancho = 2000;
    int ph = ancho * d->alto / d->ancho;
    imagem_t im;
    if (!imagem_criar(&im, ancho, ph, 0xFFFFFF)) return PR_ERR_MEMORIA;
    pr_dibujar(d, i, im.px, ancho, ph, 0, -1, -1);
    uint8_t *dat = NULL; int n = 0;
    int r = 0;
    if (imagem_codificar_png(&im, &dat, &n) != 0) r = PR_ERR_MEMORIA;
    else if (salvar_arquivo(ruta, dat, n) != 0) r = PR_ERR_ARCHIVO;
    libera(dat);
    imagem_liberar(&im);
    return r;
}

/* ------------------------------------------------------------ PDF */

typedef struct { pdfw_t *w; const pr_deck_t *d; int ox, oy, num, den; } pdfctx_t;

static void emitir(void *vc, const pr_obj_t *o, const char *texto, int ox, int oy, uint32_t col)
{
    pdfctx_t *c = vc;
    pr_comp_t cp;
    if (!pr_componer(c->d, o, texto, &cp)) return;
    int fam = pr_familia(pr_fuente_obj(c->d, o)), tam = o->tam < 1 ? 12 : o->tam;
    int tam_tw = tam * 20 * c->num / c->den;
    for (int i = 0; i < cp.n; i++) {
        const pr_lin_t *l = &cp.l[i];
        int base = c->oy + (((oy + PR_INSET_Y) * 20 + l->base / 5) * c->num / c->den);
        int x0 = c->ox + (((ox + PR_INSET_X) * 20 + l->x / 5) * c->num / c->den);
        if (l->marca == 1) {
            int sz = tam_tw * 3 / 10;
            pdfw_retangulo(c->w, c->ox + (((ox + PR_INSET_X) * 20 + l->marca_x / 5) * c->num / c->den) + tam_tw * 3 / 10, base - tam_tw * 4 / 10, sz, sz, col);
        } else if (l->marca > 1) {
            char nb[12];
            int nn = formatar_texto(nb, sizeof nb, "%d.", l->marca - 1);
            pdfw_texto(c->w, c->ox + (((ox + PR_INSET_X) * 20 + l->marca_x / 5) * c->num / c->den), base, fam, o->est & 3, tam_tw, col, nb, nn, 0);
        }
        if (l->n > 0) pdfw_texto(c->w, x0, base, fam, o->est & 3, tam_tw, col, texto + l->ini, l->n, 0);
        if ((o->est & PR_SUBRAYADO) && l->n > 0) {
            int w = l->ancho / 5 * c->num / c->den;
            pdfw_retangulo(c->w, x0, base + tam_tw / 12 + 10, w, tam_tw / 16 + 6, col);
        }
    }
    pr_comp_liberar(&cp);
}

int pr_exportar_pdf(pr_deck_t *d, const char *ruta, bool con_notas)
{
    pdfw_t *w = pdfw_criar(PDFW_A4);
    if (!w) return PR_ERR_MEMORIA;
    if (!con_notas) pdfw_tamanho(w, d->ancho * 20, d->alto * 20);
    pdfw_titulo(w, "Presentacion");
    int paginas = 0, r = 0;
    for (int i = 0; i < d->n && !r; i++) {
        if (d->d[i]->oculta) continue;
        int pw = con_notas ? 960 : d->ancho * 4 / 3, ph = pw * d->alto / d->ancho;
        uint32_t *px = aloca((size_t)pw * (size_t)ph * 4);
        if (!px) { r = PR_ERR_MEMORIA; break; }
        pr_dibujar(d, i, px, pw, ph, PRD_SIN_TEXTO, -1, -1);
        if (!pdfw_pagina(w)) { libera(px); r = PR_ERR_MEMORIA; break; }
        pdfctx_t c = { w, d, 0, 0, 1, 1 };
        if (!con_notas) {
            pdfw_imagem_rgb(w, 0, 0, d->ancho * 20, d->alto * 20, px, pw, ph);
        } else {
            int mw = 11906 - 2 * 1440;
            int mh = mw * d->alto / d->ancho;
            pdfw_imagem_rgb(w, 1440, 1440, mw, mh, px, pw, ph);
            pdfw_linha(w, 1440, 1440, 1440 + mw, 1440, 10, 0x808080); pdfw_linha(w, 1440, 1440 + mh, 1440 + mw, 1440 + mh, 10, 0x808080);
            pdfw_linha(w, 1440, 1440, 1440, 1440 + mh, 10, 0x808080); pdfw_linha(w, 1440 + mw, 1440, 1440 + mw, 1440 + mh, 10, 0x808080);
            c.ox = 1440; c.oy = 1440; c.num = mw; c.den = d->ancho * 20;
            /* las notas, debajo, con ajuste de línea */
            const char *nt = d->d[i]->notas ? d->d[i]->notas : "";
            int y = 1440 + mh + 700, tam = 240, anchomax = mw;
            int len = (int)tam_texto(nt), p = 0;
            while (p < len && y < 16838 - 1440) {
                int fin = p;
                while (fin < len && nt[fin] != '\n') fin++;
                int s = p;
                do {
                    int e = fin, lastsp = -1;
                    for (int q = s; q < fin; q++) {
                        if (pdfw_largura_texto(PDFW_SANS, 0, tam, nt + s, q + 1 - s) > anchomax && q > s) { e = lastsp > s ? lastsp + 1 : q; break; }
                        if (nt[q] == ' ') lastsp = q;
                    }
                    int ev = e;
                    while (ev > s && nt[ev - 1] == ' ') ev--;
                    if (ev > s) pdfw_texto(w, 1440, y, PDFW_SANS, 0, tam, 0x202020, nt + s, ev - s, 0);
                    y += tam * 13 / 10;
                    s = e;
                } while (s < fin);
                p = fin + 1;
            }
        }
        libera(px);
        pr_recorrer_textos(d, i, emitir, &c);
        paginas++;
    }
    if (!r && paginas == 0) r = PR_ERR_FORMATO;
    if (!r) {
        uint8_t *dat = NULL; int n = 0;
        if (!pdfw_finalizar(w, &dat, &n)) r = PR_ERR_MEMORIA;
        else if (salvar_arquivo(ruta, dat, n) != 0) r = PR_ERR_ARCHIVO;
        libera(dat);
    }
    pdfw_liberar(w);
    return r;
}
