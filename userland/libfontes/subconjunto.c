// SPDX-License-Identifier: GPL-2.0-or-later
/* subconjunto.c - subconjunto de fuentes TrueType para incrustar en PDF (D319).
 *
 * Entero puro (sin FPU) y sin depender de fonte.c: lee la tabla de tablas del sfnt, busca el
 * glifo de cada carácter Latin-1 usado (WinAnsi) en la cmap y escribe otra fuente TrueType con
 * lo imprescindible: head, hhea, maxp, hmtx, loca, glyf, cmap (3,1), post v3 y OS/2. Los
 * identificadores de glifo se conservan hasta el mayor usado (los que no se usan quedan vacíos),
 * así que los glifos compuestos siguen apuntando bien. Sin hinting (se descartan cvt, fpgm, prep).
 * También extrae lo que pide el FontDescriptor de PDF (nombre PostScript, caja, ascendente...).
 * Con SUBCONJ_ANFITRION se compila en el PC para la prueba de unidad (tests/h319_subconjunto.py). */
#ifdef SUBCONJ_ANFITRION
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#define aloca malloc
#define aloca_zerado(n) calloc(1, (n))
#define libera free
#else
#include "sagui.h"
#endif
#include "fontes_pdf.h"

static unsigned u16(const uint8_t *p) { return (unsigned)p[0] << 8 | p[1]; }
static int s16(const uint8_t *p) { return (int16_t)(p[0] << 8 | p[1]); }
static unsigned u32(const uint8_t *p) { return (unsigned)p[0] << 24 | (unsigned)p[1] << 16 | (unsigned)p[2] << 8 | p[3]; }
static void p16(uint8_t *p, unsigned v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static void p32(uint8_t *p, unsigned v) { p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v; }
#define TAG(a, b, c, d) ((uint32_t)(a) << 24 | (uint32_t)(b) << 16 | (uint32_t)(c) << 8 | (uint32_t)(d))

typedef struct { uint32_t tag; const uint8_t *p; unsigned n; } tabla_t;
#define NT_MAX 48

static int leer_tablas(const uint8_t *d, unsigned n, tabla_t *t)
{
    if (n < 12) return 0;
    uint32_t sig = u32(d);
    if (sig != 0x00010000 && sig != TAG('t', 'r', 'u', 'e') && sig != TAG('O', 'T', 'T', 'O')) return 0;
    unsigned k = u16(d + 4);
    int nt = 0;
    for (unsigned i = 0; i < k && nt < NT_MAX && 12 + i * 16 + 16 <= n; i++) {
        const uint8_t *e = d + 12 + i * 16;
        unsigned off = u32(e + 8), len = u32(e + 12);
        if (off > n || len > n - off) continue;
        t[nt].tag = u32(e); t[nt].p = d + off; t[nt].n = len; nt++;
    }
    return nt;
}

static const tabla_t *achar(const tabla_t *t, int nt, uint32_t tag)
{
    for (int i = 0; i < nt; i++) if (t[i].tag == tag) return &t[i];
    return NULL;
}

/* Glifo de un carácter Unicode en la cmap (formatos 4 y 12; 0 = no está). */
static unsigned cmap_gid(const tabla_t *cm, unsigned cp)
{
    if (!cm || cm->n < 4) return 0;
    const uint8_t *p = cm->p;
    unsigned ns = u16(p + 2), mejor = 0, mpri = 0;
    for (unsigned i = 0; i < ns && 4 + (i + 1) * 8 <= cm->n; i++) {
        unsigned plat = u16(p + 4 + i * 8), enc = u16(p + 6 + i * 8), off = u32(p + 8 + i * 8);
        unsigned pri = (plat == 3 && enc == 10) ? 4 : (plat == 3 && enc == 1) ? 3 : plat == 0 ? 2 : 0;
        if (pri > mpri && off + 4 <= cm->n) { mpri = pri; mejor = off; }
    }
    if (!mpri) return 0;
    const uint8_t *s = p + mejor;
    unsigned fmt = u16(s), ln = cm->n - mejor;
    if (fmt == 4 && ln >= 14) {
        unsigned seg = u16(s + 6) / 2;
        if (16 + seg * 8 > ln) return 0;
        const uint8_t *fin = s + 14, *ini = fin + seg * 2 + 2, *dl = ini + seg * 2, *ro = dl + seg * 2;
        for (unsigned i = 0; i < seg; i++) {
            if (cp > u16(fin + i * 2)) continue;
            if (cp < u16(ini + i * 2)) return 0;
            unsigned r = u16(ro + i * 2);
            if (!r) return (cp + u16(dl + i * 2)) & 0xFFFF;
            const uint8_t *g = ro + i * 2 + r + (cp - u16(ini + i * 2)) * 2;
            if (g + 2 > s + ln) return 0;
            unsigned v = u16(g);
            return v ? (v + u16(dl + i * 2)) & 0xFFFF : 0;
        }
        return 0;
    }
    if (fmt == 12 && ln >= 16) {
        unsigned ng = u32(s + 12);
        for (unsigned i = 0; i < ng && 16 + (i + 1) * 12 <= ln; i++) {
            unsigned a = u32(s + 16 + i * 12), b = u32(s + 20 + i * 12), g0 = u32(s + 24 + i * 12);
            if (cp >= a && cp <= b) return g0 + (cp - a);
        }
    }
    return 0;
}

/* WinAnsi (cp1252): el Unicode de cada byte 0x80..0x9F; el resto vale por sí mismo. */
static unsigned winansi_cp(unsigned b)
{
    static const uint16_t t[32] = {
        0x20AC, 0, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017D, 0,
        0, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0, 0x017E, 0x0178 };
    return b >= 0x80 && b < 0xA0 ? t[b - 0x80] : b;
}

static unsigned suma_tabla(const uint8_t *p, unsigned n)
{
    unsigned s = 0, i = 0;
    for (; i + 4 <= n; i += 4) s += u32(p + i);
    if (i < n) { uint8_t r[4] = { 0, 0, 0, 0 }; for (unsigned k = 0; i + k < n; k++) r[k] = p[i + k]; s += u32(r); }
    return s;
}

/* Contenido de la tabla name con el id dado (Windows UTF-16 o Mac), en ASCII; 0 = no hay. */
static int nombre_ascii(const tabla_t *nm, int id, char *d, int max)
{
    if (!nm || nm->n < 6 || max < 2) return 0;
    const uint8_t *p = nm->p;
    unsigned cnt = u16(p + 2), so = u16(p + 4);
    for (unsigned i = 0; i < cnt && 6 + (i + 1) * 12 <= nm->n; i++) {
        const uint8_t *r = p + 6 + i * 12;
        if ((int)u16(r + 6) != id) continue;
        unsigned plat = u16(r), len = u16(r + 8), off = u16(r + 10);
        if (so + off + len > nm->n) continue;
        const uint8_t *t = p + so + off;
        int k = 0;
        if (plat == 3) { for (unsigned q = 0; q + 1 < len && k < max - 1; q += 2) { unsigned c = u16(t + q); if (c > 32 && c < 127) d[k++] = (char)c; } }
        else if (plat == 1) { for (unsigned q = 0; q < len && k < max - 1; q++) if (t[q] > 32 && t[q] < 127) d[k++] = (char)t[q]; }
        else continue;
        d[k] = 0;
        if (k) return k;
    }
    return 0;
}

bool fonte_info_pdf(const uint8_t *d, unsigned n, const char *ruta, pdfw_finfo_t *o)
{
    tabla_t t[NT_MAX];
    int nt = leer_tablas(d, n, t);
    const tabla_t *head = achar(t, nt, TAG('h', 'e', 'a', 'd')), *hhea = achar(t, nt, TAG('h', 'h', 'e', 'a'));
    const tabla_t *os2 = achar(t, nt, TAG('O', 'S', '/', '2')), *post = achar(t, nt, TAG('p', 'o', 's', 't'));
    if (!head || head->n < 54 || !hhea || hhea->n < 8) return false;
    memset(o, 0, sizeof *o);
    int upem = (int)u16(head->p + 18);
    if (upem < 16) return false;
    o->cff = u32(d) == TAG('O', 'T', 'T', 'O');
    if (!nombre_ascii(achar(t, nt, TAG('n', 'a', 'm', 'e')), 6, o->psname, sizeof o->psname)) {
        int k = 0;
        const char *b = ruta;
        for (const char *q = ruta; *q; q++) if (*q == '/') b = q + 1;
        for (; *b && *b != '.' && k < 60; b++) if (*b > 32 && *b < 127 && *b != '(' && *b != ')' && *b != '/' && *b != '[' && *b != ']' && *b != '<' && *b != '>' && *b != '%') o->psname[k++] = *b;
        o->psname[k] = 0;
        if (!k) { o->psname[0] = 'F'; o->psname[1] = 0; }
    }
    for (char *q = o->psname; *q; q++) if (*q == '(' || *q == ')' || *q == '/' || *q == '[' || *q == ']' || *q == '<' || *q == '>' || *q == '%' || *q == '{' || *q == '}' || *q == '#') *q = '-';
    o->bbox[0] = s16(head->p + 36) * 1000 / upem; o->bbox[1] = s16(head->p + 38) * 1000 / upem;
    o->bbox[2] = s16(head->p + 40) * 1000 / upem; o->bbox[3] = s16(head->p + 42) * 1000 / upem;
    o->ascent = s16(hhea->p + 4) * 1000 / upem;
    o->descent = s16(hhea->p + 6) * 1000 / upem;
    o->capheight = o->ascent * 7 / 10;
    if (os2 && os2->n >= 90 && u16(os2->p) >= 2) { int c = s16(os2->p + 88); if (c > 0) o->capheight = c * 1000 / upem; }
    int mac = (int)u16(head->p + 44);
    o->flags = 32;
    if (post && post->n >= 16) {
        int ent = s16(post->p + 4);           /* parte entera del ángulo (Fixed 16.16) */
        o->angulo = ent;
        if (u32(post->p + 12)) o->flags |= 1;   /* isFixedPitch */
    }
    if ((mac & 2) || o->angulo) o->flags |= 64;
    o->stemv = (mac & 1) ? 140 : 80;
    return true;
}

/* Añade a "keep" los glifos que usa uno compuesto. */
static void cerrar_compuestos(const uint8_t *glyf, unsigned nglyf, const uint8_t *loca, unsigned nloca, bool largo, unsigned ng, uint8_t *keep)
{
    bool cambio = true;
    for (int vuelta = 0; cambio && vuelta < 8; vuelta++) {
        cambio = false;
        for (unsigned g = 0; g < ng; g++) {
            if (!keep[g]) continue;
            unsigned a, b;
            if (largo) { if ((g + 2) * 4 > nloca) continue; a = u32(loca + g * 4); b = u32(loca + g * 4 + 4); }
            else { if ((g + 2) * 2 > nloca) continue; a = u16(loca + g * 2) * 2; b = u16(loca + g * 2 + 2) * 2; }
            if (b <= a || b > nglyf || b - a < 10 || s16(glyf + a) >= 0) continue;
            const uint8_t *p = glyf + a + 10, *fin = glyf + b;
            for (;;) {
                if (p + 4 > fin) break;
                unsigned fl = u16(p), gi = u16(p + 2);
                if (gi < ng && !keep[gi]) { keep[gi] = 1; cambio = true; }
                p += 4 + ((fl & 1) ? 4 : 2);
                if (fl & 0x8) p += 2; else if (fl & 0x40) p += 4; else if (fl & 0x80) p += 8;
                if (!(fl & 0x20)) break;
            }
        }
    }
}

typedef struct { uint32_t tag; uint8_t *p; unsigned n; } salida_t;

bool fonte_subconjunto_ttf(const uint8_t *d, unsigned n, const uint8_t *usadas, uint8_t **out, unsigned *nout)
{
    tabla_t t[NT_MAX];
    int nt = leer_tablas(d, n, t);
    if (u32(d) == TAG('O', 'T', 'T', 'O')) return false;
    const tabla_t *head = achar(t, nt, TAG('h', 'e', 'a', 'd')), *hhea = achar(t, nt, TAG('h', 'h', 'e', 'a')), *maxp = achar(t, nt, TAG('m', 'a', 'x', 'p'));
    const tabla_t *hmtx = achar(t, nt, TAG('h', 'm', 't', 'x')), *cmap = achar(t, nt, TAG('c', 'm', 'a', 'p')), *glyf = achar(t, nt, TAG('g', 'l', 'y', 'f'));
    const tabla_t *loca = achar(t, nt, TAG('l', 'o', 'c', 'a')), *os2 = achar(t, nt, TAG('O', 'S', '/', '2')), *post = achar(t, nt, TAG('p', 'o', 's', 't'));
    if (!head || head->n < 54 || !hhea || hhea->n < 36 || !maxp || maxp->n < 6 || !hmtx || !cmap || !glyf || !loca) return false;
    unsigned ng = u16(maxp->p + 4), nhm = u16(hhea->p + 34);
    bool largo = s16(head->p + 50) != 0;
    if (!ng || nhm < 1 || nhm > ng || hmtx->n < nhm * 4) return false;
    if (loca->n < (ng + 1) * (largo ? 4u : 2u)) return false;
    uint8_t *keep = aloca_zerado(ng + 1);
    unsigned *gids = aloca_zerado(257 * sizeof(unsigned));
    if (!keep || !gids) { libera(keep); libera(gids); return false; }
    keep[0] = 1;
    unsigned maxg = 0;
    for (unsigned b = 32; b < 256; b++) {
        gids[b] = 0;
        if (!(usadas[b >> 3] & (1u << (b & 7)))) continue;
        unsigned g = cmap_gid(cmap, winansi_cp(b));
        gids[b] = g < ng ? g : 0;
        if (gids[b]) keep[gids[b]] = 1;
    }
    cerrar_compuestos(glyf->p, glyf->n, loca->p, loca->n, largo, ng, keep);
    for (unsigned g = 0; g < ng; g++) if (keep[g]) maxg = g;
    unsigned nng = maxg + 1;
    /* glyf y loca nuevos */
    unsigned tot = 0;
    for (unsigned g = 0; g < nng; g++) {
        if (!keep[g]) continue;
        unsigned a = largo ? u32(loca->p + g * 4) : u16(loca->p + g * 2) * 2u, b = largo ? u32(loca->p + g * 4 + 4) : u16(loca->p + g * 2 + 2) * 2u;
        if (b > a && b <= glyf->n) tot += (b - a + 3) & ~3u;
    }
    uint8_t *ngl = aloca_zerado(tot + 4), *nlo = aloca_zerado((nng + 1) * 4);
    uint8_t *nhmtx = aloca_zerado(nng * 4);
    /* cmap: formato 4, un segmento por carácter usado (los ordena por código) */
    unsigned segs = 0, cpcod[224], cpgid[224];
    for (unsigned b = 32; b < 256; b++) if (gids[b]) { cpcod[segs] = winansi_cp(b); cpgid[segs] = gids[b]; segs++; }
    for (unsigned i = 1; i < segs; i++) {
        unsigned c = cpcod[i], g = cpgid[i], j = i;
        while (j > 0 && cpcod[j - 1] > c) { cpcod[j] = cpcod[j - 1]; cpgid[j] = cpgid[j - 1]; j--; }
        cpcod[j] = c; cpgid[j] = g;
    }
    unsigned nseg = segs + 1, ncm = 12 + 16 + nseg * 8;      /* cabecera + subtabla (14 + 2 + 8 por segmento) */
    uint8_t *ncmap = aloca_zerado(ncm + 4);
    if (!ngl || !nlo || !nhmtx || !ncmap) { libera(keep); libera(gids); libera(ngl); libera(nlo); libera(nhmtx); libera(ncmap); return false; }
    unsigned pos = 0;
    for (unsigned g = 0; g < nng; g++) {
        p32(nlo + g * 4, pos);
        if (keep[g]) {
            unsigned a = largo ? u32(loca->p + g * 4) : u16(loca->p + g * 2) * 2u, b = largo ? u32(loca->p + g * 4 + 4) : u16(loca->p + g * 2 + 2) * 2u;
            if (b > a && b <= glyf->n) { memcpy(ngl + pos, glyf->p + a, b - a); pos += (b - a + 3) & ~3u; }
            /* métricas */
            unsigned i = g < nhm ? g : nhm - 1;
            unsigned aw = u16(hmtx->p + i * 4);
            int lsb = g < nhm ? s16(hmtx->p + g * 4 + 2) : ((hmtx->n >= nhm * 4 + (g - nhm + 1) * 2) ? s16(hmtx->p + nhm * 4 + (g - nhm) * 2) : 0);
            p16(nhmtx + g * 4, aw); p16(nhmtx + g * 4 + 2, (unsigned)lsb);
        } else if (g < nhm) {
            p16(nhmtx + g * 4, u16(hmtx->p + g * 4));          /* conserva el avance */
        } else p16(nhmtx + g * 4, u16(hmtx->p + (nhm - 1) * 4));
    }
    p32(nlo + nng * 4, pos);
    unsigned nglen = pos;
    /* cmap */
    p16(ncmap, 0); p16(ncmap + 2, 1); p16(ncmap + 4, 3); p16(ncmap + 6, 1); p32(ncmap + 8, 12);
    uint8_t *s = ncmap + 12;
    p16(s, 4); p16(s + 2, 16 + nseg * 8); p16(s + 4, 0); p16(s + 6, nseg * 2);
    { unsigned sr = 1, es = 0; while (sr * 2 <= nseg) { sr *= 2; es++; } p16(s + 8, sr * 2); p16(s + 10, es); p16(s + 12, nseg * 2 - sr * 2); }
    uint8_t *fin = s + 14, *ini = fin + nseg * 2 + 2, *dl = ini + nseg * 2, *ro = dl + nseg * 2;
    for (unsigned i = 0; i < segs; i++) { p16(fin + i * 2, cpcod[i]); p16(ini + i * 2, cpcod[i]); p16(dl + i * 2, (cpgid[i] - cpcod[i]) & 0xFFFF); p16(ro + i * 2, 0); }
    p16(fin + segs * 2, 0xFFFF); p16(ini + segs * 2, 0xFFFF); p16(dl + segs * 2, 1); p16(ro + segs * 2, 0);
    /* head, hhea, maxp, post */
    uint8_t *nhead = aloca(54), *nhhea = aloca(36), *nmaxp = aloca(maxp->n), *npost = aloca(32);
    uint8_t *nos2 = os2 ? aloca(os2->n) : NULL;
    if (!nhead || !nhhea || !nmaxp || !npost || (os2 && !nos2)) { libera(keep); libera(gids); libera(ngl); libera(nlo); libera(nhmtx); libera(ncmap); libera(nhead); libera(nhhea); libera(nmaxp); libera(npost); libera(nos2); return false; }
    memcpy(nhead, head->p, 54); p32(nhead + 8, 0); p16(nhead + 50, 1);
    memcpy(nhhea, hhea->p, 36); p16(nhhea + 34, nng);
    memcpy(nmaxp, maxp->p, maxp->n); p16(nmaxp + 4, nng);
    memset(npost, 0, 32); p32(npost, 0x00030000);
    if (post && post->n >= 16) memcpy(npost + 4, post->p + 4, 12);
    if (os2) memcpy(nos2, os2->p, os2->n);
    salida_t tb[9]; int k = 0;
    tb[k++] = (salida_t){ TAG('O', 'S', '/', '2'), nos2, os2 ? os2->n : 0 };
    tb[k++] = (salida_t){ TAG('c', 'm', 'a', 'p'), ncmap, ncm };
    tb[k++] = (salida_t){ TAG('g', 'l', 'y', 'f'), ngl, nglen };
    tb[k++] = (salida_t){ TAG('h', 'e', 'a', 'd'), nhead, 54 };
    tb[k++] = (salida_t){ TAG('h', 'h', 'e', 'a'), nhhea, 36 };
    tb[k++] = (salida_t){ TAG('h', 'm', 't', 'x'), nhmtx, nng * 4 };
    tb[k++] = (salida_t){ TAG('l', 'o', 'c', 'a'), nlo, (nng + 1) * 4 };
    tb[k++] = (salida_t){ TAG('m', 'a', 'x', 'p'), nmaxp, maxp->n };
    tb[k++] = (salida_t){ TAG('p', 'o', 's', 't'), npost, 32 };
    int ntab = 0;
    for (int i = 0; i < k; i++) if (tb[i].p) tb[ntab++] = tb[i];
    unsigned tam = 12 + 16 * (unsigned)ntab;
    for (int i = 0; i < ntab; i++) tam += (tb[i].n + 3) & ~3u;
    uint8_t *o = aloca_zerado(tam + 4);
    if (o) {
        p32(o, 0x00010000); p16(o + 4, (unsigned)ntab);
        unsigned sr = 1, es = 0; while (sr * 2 <= (unsigned)ntab) { sr *= 2; es++; }
        p16(o + 6, sr * 16); p16(o + 8, es); p16(o + 10, (unsigned)ntab * 16 - sr * 16);
        unsigned off = 12 + 16 * (unsigned)ntab, off_head = 0;
        for (int i = 0; i < ntab; i++) {
            memcpy(o + off, tb[i].p, tb[i].n);
            uint8_t *e = o + 12 + 16 * i;
            p32(e, tb[i].tag); p32(e + 4, suma_tabla(o + off, tb[i].n)); p32(e + 8, off); p32(e + 12, tb[i].n);
            if (tb[i].tag == TAG('h', 'e', 'a', 'd')) off_head = off;
            off += (tb[i].n + 3) & ~3u;
        }
        p32(o + off_head + 8, 0xB1B0AFBAu - suma_tabla(o, tam));
        *out = o; *nout = tam;
    }
    libera(keep); libera(gids); libera(ngl); libera(nlo); libera(nhmtx); libera(ncmap);
    libera(nhead); libera(nhhea); libera(nmaxp); libera(npost); libera(nos2);
    return o != NULL;
}
