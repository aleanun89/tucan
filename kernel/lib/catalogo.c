// SPDX-License-Identifier: GPL-2.0-or-later
/* catalogo.c - lector de catálogos de traducción (D313). Ver catalogo.h.
 * No se fía del archivo: comprueba que la tabla y cada desplazamiento caen dentro. */
#include "catalogo.h"

#define CAB 16u
#define ENT 12u

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

uint32_t catalogo_hash(const char *s)
{
    uint32_t h = 2166136261u;
    for (; *s; s++) { h ^= (uint8_t)*s; h *= 16777619u; }
    return h;
}

int catalogo_abrir(catalogo_t *c, const void *d, uint32_t tam)
{
    const uint8_t *p = d;
    c->d = 0; c->tam = c->n = 0;
    if (!p || tam < CAB || le32(p) != CATALOGO_MAGIA || le32(p + 4) != 1) return -1;
    uint32_t n = le32(p + 8);
    if (n > (tam - CAB) / ENT) return -1;
    c->d = p; c->tam = tam; c->n = n;
    return 0;
}

/* La cadena que empieza en "off", o NULL si cae fuera o no termina dentro del archivo. */
static const char *cadena(const catalogo_t *c, uint32_t off)
{
    if (off < CAB + c->n * ENT || off >= c->tam) return 0;
    for (uint32_t i = off; i < c->tam; i++)
        if (!c->d[i]) return (const char *)c->d + off;
    return 0;
}

const char *catalogo_buscar(const catalogo_t *c, const char *clave)
{
    if (!c || !c->d || !c->n || !clave) return 0;
    uint32_t h = catalogo_hash(clave), lo = 0, hi = c->n;
    while (lo < hi) {   /* primer índice con hash >= h */
        uint32_t m = lo + (hi - lo) / 2;
        if (le32(c->d + CAB + m * ENT) < h) lo = m + 1; else hi = m;
    }
    for (uint32_t i = lo; i < c->n; i++) {
        const uint8_t *e = c->d + CAB + i * ENT;
        if (le32(e) != h) break;
        const char *k = cadena(c, le32(e + 4)), *a = clave;
        if (!k) continue;
        while (*k && *k == *a) { k++; a++; }
        if (*k || *a) continue;
        const char *t = cadena(c, le32(e + 8));
        return t && *t ? t : 0;
    }
    return 0;
}

void catalogo_idioma_norm(const char *in, char *out)
{
    char b[IDIOMA_MAX];
    int n = 0;
    for (; in && *in && *in != '.' && *in != '@' && n < IDIOMA_MAX - 1; in++) {
        char c = *in;
        if (c == '-') c = '_';
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')) { n = 0; break; }
        b[n++] = c;
    }
    b[n] = 0;
    /* idioma en minúsculas, región en mayúsculas: pt_br -> pt_BR */
    for (int i = 0; i < n; i++) {
        bool reg = false;
        for (int k = 0; k < i; k++) if (b[k] == '_') reg = true;
        if (b[i] != '_') {
            if (reg && b[i] >= 'a' && b[i] <= 'z') b[i] = (char)(b[i] - 32);
            if (!reg && b[i] >= 'A' && b[i] <= 'Z') b[i] = (char)(b[i] + 32);
        }
    }
    if (!n || (b[0] == 'c' && !b[1]) || (b[0] == 'p' && b[1] == 'o' && b[2] == 's')) { out[0] = 'e'; out[1] = 's'; out[2] = 0; return; }
    if (b[0] == 'p' && b[1] == 't') {   /* el portugués es siempre el de Brasil */
        const char *r = "pt_BR";
        int i = 0;
        for (; r[i]; i++) out[i] = r[i];
        out[i] = 0;
        return;
    }
    if (n > 2 && b[2] == '_') { b[2] = 0; n = 2; }   /* en_US -> en */
    for (int i = 0; i <= n; i++) out[i] = b[i];
}
