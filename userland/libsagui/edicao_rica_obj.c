// SPDX-License-Identifier: GPL-2.0-or-later
/* edicao_rica_obj.c - datos de las imágenes y los WordArt del control de texto (mejora 122, D296).
 *
 * Los datos pesados (píxeles de pantalla y el JPEG o PNG original) van aparte, con cuenta de
 * referencias: cada cambio de tamaño o de propiedades de un objeto crea una versión nueva del
 * descriptor (pequeña) que apunta a los mismos datos, y así deshacer lo recupera todo. */
#include "edicao_rica_obj.h"
#include "wordart.h"

#define LADO_PANTALLA 1600

rte_obj_dados_t *obj_dados_ref(rte_obj_dados_t *d) { if (d) d->ref++; return d; }

void obj_dados_unref(rte_obj_dados_t *d)
{
    if (!d || --d->ref > 0) return;
    imagem_liberar(&d->img);
    imagem_liberar(&d->cache);
    libera(d->blob);
    libera(d);
}

rte_obj_t *obj_clonar(const rte_obj_t *o)
{
    rte_obj_t *c = aloca(sizeof *c);
    if (!c) return NULL;
    *c = *o;
    obj_dados_ref(c->d);
    return c;
}

void obj_liberar(rte_obj_t *o) { if (o) { obj_dados_unref(o->d); libera(o); } }
void rte_obj_liberar(rte_obj_t *o) { if (o) { obj_dados_unref(o->d); o->d = NULL; } }

rte_obj_dados_t *rte_obj_dados_criar(imagem_t *img, const uint8_t *blob, int n_blob, int blob_tipo, int blob_l, int blob_a)
{
    rte_obj_dados_t *d = aloca_zerado(sizeof *d);
    if (!d) return NULL;
    d->ref = 1;
    d->img = *img;
    memset(img, 0, sizeof *img);
    if (blob && n_blob > 0 && blob_tipo != RTE_BLOB_NENHUM) {
        d->blob = aloca((size_t)n_blob);
        if (d->blob) {
            memcpy(d->blob, blob, (size_t)n_blob);
            d->n_blob = n_blob; d->blob_tipo = blob_tipo; d->blob_l = blob_l; d->blob_a = blob_a;
        }
    }
    return d;
}

const imagem_t *rte_obj_imagem(const rte_obj_t *o) { return o && o->d && o->d->img.px ? &o->d->img : NULL; }

const uint8_t *rte_obj_blob(const rte_obj_t *o, int *n, int *tipo, int *larg, int *alt)
{
    if (!o || !o->d || !o->d->blob) return NULL;
    if (n) *n = o->d->n_blob;
    if (tipo) *tipo = o->d->blob_tipo;
    if (larg) *larg = o->d->blob_l;
    if (alt) *alt = o->d->blob_a;
    return o->d->blob;
}

int rte_obj_alto_total(const rte_obj_t *o) { return o->a_tw + (o->pie[0] ? 320 : 0); }

/* ------------------------------------------------------------ JPEG */

bool obj_jpeg_info(const uint8_t *d, int n, int *l, int *a, int *ncomp)
{
    if (n < 4 || d[0] != 0xFF || d[1] != 0xD8) return false;
    int i = 2;
    while (i + 4 <= n) {
        if (d[i] != 0xFF) { i++; continue; }
        int m = d[i + 1];
        if (m == 0xFF) { i++; continue; }
        if (m == 0xD8 || (m >= 0xD0 && m <= 0xD7) || m == 0x01) { i += 2; continue; }
        int len = (d[i + 2] << 8) | d[i + 3];
        if (m >= 0xC0 && m <= 0xCF && m != 0xC4 && m != 0xC8 && m != 0xCC) {
            if (i + 10 > n) return false;
            if (m != 0xC0 && m != 0xC1) return false;      /* solo baseline (el que decodificamos) */
            int nc = d[i + 9];
            if (l) *l = (d[i + 7] << 8) | d[i + 8];
            if (a) *a = (d[i + 5] << 8) | d[i + 6];
            if (ncomp) *ncomp = nc;
            return nc == 1 || nc == 3;
        }
        if (m == 0xDA) return false;
        i += 2 + len;
    }
    return false;
}

/* ------------------------------------------------------------ crear desde bytes o archivo */

bool rte_obj_de_bytes(rte_obj_t *o, const uint8_t *dados, int n)
{
    imagem_t img;
    memset(&img, 0, sizeof img);
    if (imagem_decodificar(dados, n, &img) != 0 || !img.px) return false;
    int bt = RTE_BLOB_NENHUM, bl = img.larg, ba = img.alt;
    int jl, ja, jc;
    if (n > 8 && dados[0] == 0x89 && dados[1] == 'P' && dados[2] == 'N' && dados[3] == 'G') bt = RTE_BLOB_PNG;
    else if (obj_jpeg_info(dados, n, &jl, &ja, &jc)) { bt = RTE_BLOB_JPEG; bl = jl; ba = ja; }
    int ol = img.larg, oa = img.alt;
    /* los píxeles de pantalla no pasan de LADO_PANTALLA */
    int m = ol > oa ? ol : oa;
    if (m > LADO_PANTALLA) {
        imagem_t red;
        memset(&red, 0, sizeof red);
        int nl = (int)((long)ol * LADO_PANTALLA / m), na = (int)((long)oa * LADO_PANTALLA / m);
        if (nl < 1) nl = 1;
        if (na < 1) na = 1;
        if (imagem_escalar(&red, &img, nl, na, true)) { imagem_liberar(&img); img = red; }
    }
    rte_obj_dados_t *d = rte_obj_dados_criar(&img, dados, n, bt, bl, ba);
    if (!d) { imagem_liberar(&img); return false; }
    memset(o, 0, sizeof *o);
    o->tipo = RTE_OBJ_IMAGEM;
    o->d = d;
    o->l_tw = ol * 15;
    o->a_tw = oa * 15;
    return true;
}

bool rte_obj_de_arquivo(rte_obj_t *o, const char *caminho)
{
    int fd = abrir(caminho, ABRIR_LER);
    if (fd < 0) return false;
    int cap = 1 << 18, tam = 0, r;
    uint8_t *b = aloca((size_t)cap);
    while (b && (r = ler(fd, b + tam, (unsigned)(cap - tam))) > 0) {
        tam += r;
        if (tam >= cap) {
            if (cap >= (96 << 20)) break;
            uint8_t *nb = realoca(b, (size_t)cap * 2);
            if (!nb) break;
            b = nb; cap *= 2;
        }
    }
    fechar(fd);
    bool ok = b && tam > 0 && rte_obj_de_bytes(o, b, tam);
    libera(b);
    return ok;
}

/* ------------------------------------------------------------ WordArt */

bool rte_obj_wordart_dibujar(rte_obj_t *o)
{
    int l = o->l_tw / 15 * 2, a = o->a_tw / 15 * 2;     /* a doble resolución: se reduce al dibujar */
    if (l < 16) l = 16;
    if (a < 16) a = 16;
    if (l > 2400) { a = a * 2400 / l; l = 2400; }
    if (a > 1600) { l = l * 1600 / a; a = 1600; }
    imagem_t img;
    memset(&img, 0, sizeof img);
    if (!wordart_renderizar(o->wa_texto, o->wa_estilo, o->wa_fam, o->wa_est, l, a, &img)) return false;
    rte_obj_dados_t *d = rte_obj_dados_criar(&img, NULL, 0, RTE_BLOB_NENHUM, 0, 0);
    if (!d) { imagem_liberar(&img); return false; }
    obj_dados_unref(o->d);
    o->d = d;
    o->tipo = RTE_OBJ_WORDART;
    return true;
}

/* ------------------------------------------------------------ dibujo */

void obj_blit(jan_t *j, rte_obj_dados_t *d, int x, int y, int l, int a, int cx0, int cy0, int cx1, int cy1)
{
    if (!d || !d->img.px || l <= 0 || a <= 0) return;
    if (x >= cx1 || y >= cy1 || x + l <= cx0 || y + a <= cy0) return;
    if (d->cache_l != l || d->cache_a != a || !d->cache.px) {
        imagem_liberar(&d->cache);
        memset(&d->cache, 0, sizeof d->cache);
        d->cache_l = d->cache_a = 0;
        if (l == d->img.larg && a == d->img.alt) { /* mismo tamaño: se dibuja directo */ }
        else if (imagem_escalar(&d->cache, &d->img, l, a, true)) { d->cache_l = l; d->cache_a = a; }
    }
    const imagem_t *src = d->cache.px ? &d->cache : &d->img;
    int rx = cx0 < 0 ? 0 : cx0, ry = cy0 < 0 ? 0 : cy0;
    int rx1 = cx1 > j->larg ? j->larg : cx1, ry1 = cy1 > j->alt ? j->alt : cy1;
    imagem_desenhar(j->px, j->larg, rx, ry, rx1 - rx, ry1 - ry, src, x, y, l, a, false);
}
