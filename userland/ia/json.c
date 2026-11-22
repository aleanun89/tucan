// SPDX-License-Identifier: GPL-2.0-or-later
/* json.c - lector de JSON para los agentes de IA (D295). Ver json.h. */
#include "json.h"

static int espacio(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

static int nombre_igual(const char *t, const json_tok_t *k, const char *nome)
{
    int n = 0;
    while (nome[n]) n++;
    /* La clave se compara cruda: los nombres que buscamos no llevan escapes. */
    if (k->fim - k->ini != n) return 0;
    return memcmp(t + k->ini, nome, (size_t)n) == 0;
}

typedef struct { const char *t; int n, p; json_tok_t *tk; int max, qtd; } ana_t;

static int valor(ana_t *a, int prof);

static int novo(ana_t *a, int tipo, int ini, int fim)
{
    if (a->qtd >= a->max) return -2;
    a->tk[a->qtd].tipo = (unsigned char)tipo;
    a->tk[a->qtd].ini = ini;
    a->tk[a->qtd].fim = fim;
    a->tk[a->qtd].n = 0;
    return a->qtd++;
}

static void saltar_espacios(ana_t *a) { while (a->p < a->n && espacio(a->t[a->p])) a->p++; }

/* Una cadena: desde la comilla de apertura. */
static int cadena(ana_t *a)
{
    int ini = ++a->p;
    while (a->p < a->n && a->t[a->p] != '"') {
        unsigned char c = (unsigned char)a->t[a->p];
        if (c < 0x20) return -1;               /* los controles van escapados */
        if (c == '\\') {
            a->p++;
            if (a->p >= a->n) return -1;
            char e = a->t[a->p];
            if (e == 'u') {
                for (int i = 1; i <= 4; i++) {
                    if (a->p + i >= a->n) return -1;
                    char h = a->t[a->p + i];
                    if (!((h >= '0' && h <= '9') || (h >= 'a' && h <= 'f') || (h >= 'A' && h <= 'F'))) return -1;
                }
                a->p += 4;
            } else if (!(e == '"' || e == '\\' || e == '/' || e == 'b' || e == 'f' || e == 'n' || e == 'r' || e == 't')) return -1;
        }
        a->p++;
    }
    if (a->p >= a->n) return -1;
    int idx = novo(a, JSON_STR, ini, a->p);
    a->p++;
    return idx;
}

static int literal(ana_t *a)
{
    int ini = a->p;
    int tipo = JSON_LIT;
    if (a->t[a->p] == '-' || (a->t[a->p] >= '0' && a->t[a->p] <= '9')) {
        tipo = JSON_NUM;
        while (a->p < a->n && (a->t[a->p] == '-' || a->t[a->p] == '+' || a->t[a->p] == '.' || a->t[a->p] == 'e' ||
                               a->t[a->p] == 'E' || (a->t[a->p] >= '0' && a->t[a->p] <= '9'))) a->p++;
    } else {
        static const char *lits[] = { "true", "false", "null" };
        int ok = 0;
        for (int i = 0; i < 3; i++) {
            int l = 0;
            while (lits[i][l]) l++;
            if (a->p + l <= a->n && memcmp(a->t + a->p, lits[i], (size_t)l) == 0) { a->p += l; ok = 1; break; }
        }
        if (!ok) return -1;
    }
    if (a->p == ini) return -1;
    return novo(a, tipo, ini, a->p);
}

static int valor(ana_t *a, int prof)
{
    if (prof > JSON_PROFUNDIDAD_MAX) return -3;
    saltar_espacios(a);
    if (a->p >= a->n) return -1;
    char c = a->t[a->p];
    if (c == '"') return cadena(a);
    if (c == '{' || c == '[') {
        int obj = c == '{';
        int idx = novo(a, obj ? JSON_OBJ : JSON_ARR, a->p, 0);
        if (idx < 0) return idx;
        a->p++;
        saltar_espacios(a);
        int cuenta = 0;
        if (a->p < a->n && a->t[a->p] == (obj ? '}' : ']')) { a->p++; a->tk[idx].fim = a->p; return idx; }
        for (;;) {
            saltar_espacios(a);
            if (obj) {
                if (a->p >= a->n || a->t[a->p] != '"') return -1;
                int r = cadena(a);
                if (r < 0) return r;
                saltar_espacios(a);
                if (a->p >= a->n || a->t[a->p] != ':') return -1;
                a->p++;
            }
            int r = valor(a, prof + 1);
            if (r < 0) return r;
            cuenta++;
            saltar_espacios(a);
            if (a->p >= a->n) return -1;
            if (a->t[a->p] == ',') { a->p++; continue; }
            if (a->t[a->p] == (obj ? '}' : ']')) { a->p++; break; }
            return -1;
        }
        a->tk[idx].n = cuenta;
        a->tk[idx].fim = a->p;
        return idx;
    }
    return literal(a);
}

int json_analisar(const char *t, int n, json_tok_t *tk, int max)
{
    ana_t a = { t, n, 0, tk, max, 0 };
    int r = valor(&a, 0);
    if (r < 0) return r;
    saltar_espacios(&a);
    if (a.p != a.n) return -1;
    return a.qtd;
}

int json_depois(const json_tok_t *tk, int i)
{
    int pend = 1;      /* valores que faltan por saltar */
    int j = i;
    while (pend > 0) {
        pend--;
        if (tk[j].tipo == JSON_OBJ) pend += tk[j].n * 2;
        else if (tk[j].tipo == JSON_ARR) pend += tk[j].n;
        j++;
    }
    return j;
}

int json_campo(const char *t, const json_tok_t *tk, int i, const char *nome)
{
    if (i < 0 || tk[i].tipo != JSON_OBJ) return -1;
    int j = i + 1;
    for (int k = 0; k < tk[i].n; k++) {
        int v = j + 1;
        if (nombre_igual(t, &tk[j], nome)) return v;
        j = json_depois(tk, v);
    }
    return -1;
}

int json_elem(const json_tok_t *tk, int i, int k)
{
    if (i < 0 || tk[i].tipo != JSON_ARR || k < 0 || k >= tk[i].n) return -1;
    int j = i + 1;
    while (k-- > 0) j = json_depois(tk, j);
    return j;
}

static int hex(char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c - 'A' + 10; }

static int poner_utf8(char *dst, int k, unsigned cp)
{
    if (cp < 0x80) { dst[k++] = (char)cp; }
    else if (cp < 0x800) { dst[k++] = (char)(0xC0 | (cp >> 6)); dst[k++] = (char)(0x80 | (cp & 0x3F)); }
    else if (cp < 0x10000) { dst[k++] = (char)(0xE0 | (cp >> 12)); dst[k++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                             dst[k++] = (char)(0x80 | (cp & 0x3F)); }
    else { dst[k++] = (char)(0xF0 | (cp >> 18)); dst[k++] = (char)(0x80 | ((cp >> 12) & 0x3F));
           dst[k++] = (char)(0x80 | ((cp >> 6) & 0x3F)); dst[k++] = (char)(0x80 | (cp & 0x3F)); }
    return k;
}

int json_texto(const char *t, const json_tok_t *tk, int i, char *dst, int max)
{
    if (i < 0 || tk[i].tipo != JSON_STR || max < 1) { if (max > 0) dst[0] = 0; return -1; }
    int k = 0;
    int p = tk[i].ini, fim = tk[i].fim;
    while (p < fim) {
        if (k >= max - 5) break;               /* sitio para un carácter de hasta 4 bytes */
        char c = t[p];
        if (c != '\\') { dst[k++] = c; p++; continue; }
        p++;
        char e = t[p++];
        switch (e) {
        case 'n': dst[k++] = '\n'; break;
        case 't': dst[k++] = '\t'; break;
        case 'r': dst[k++] = '\r'; break;
        case 'b': dst[k++] = '\b'; break;
        case 'f': dst[k++] = '\f'; break;
        case 'u': {
            unsigned cp = 0;
            for (int q = 0; q < 4; q++) cp = cp * 16 + (unsigned)hex(t[p + q]);
            p += 4;
            /* Par sustituto: 😀 es un único carácter fuera del plano básico. */
            if (cp >= 0xD800 && cp < 0xDC00 && p + 6 <= fim && t[p] == '\\' && t[p + 1] == 'u') {
                unsigned lo = 0;
                int ok = 1;
                for (int q = 0; q < 4; q++) {
                    char h = t[p + 2 + q];
                    if (!((h >= '0' && h <= '9') || (h >= 'a' && h <= 'f') || (h >= 'A' && h <= 'F'))) ok = 0;
                    else lo = lo * 16 + (unsigned)hex(h);
                }
                if (ok && lo >= 0xDC00 && lo < 0xE000) { cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00); p += 6; }
            }
            if (cp >= 0xD800 && cp < 0xE000) cp = 0xFFFD;   /* sustituto suelto */
            if (cp == 0) cp = 0xFFFD;                      /* \u0000 cortaría el texto en C: lo de detrás no se vería */
            k = poner_utf8(dst, k, cp);
            break;
        }
        default: dst[k++] = e; break;           /* \" \\ \/ */
        }
    }
    dst[k] = 0;
    return k;
}

int json_int(const char *t, const json_tok_t *tk, int i, int por_omision)
{
    if (i < 0 || tk[i].tipo != JSON_NUM) return por_omision;
    int p = tk[i].ini, neg = 0, v = 0;
    if (t[p] == '-') { neg = 1; p++; }
    /* Revisión de seguridad: satura en vez de desbordar (un entero con signo que desborda es indefinido). */
    while (p < tk[i].fim && t[p] >= '0' && t[p] <= '9') {
        int d = t[p] - '0';
        v = v > (2147483647 - d) / 10 ? 2147483647 : v * 10 + d;
        p++;
    }
    return neg ? -v : v;
}

bool json_verdadero(const char *t, const json_tok_t *tk, int i)
{
    return i >= 0 && tk[i].tipo == JSON_LIT && tk[i].fim - tk[i].ini == 4 && memcmp(t + tk[i].ini, "true", 4) == 0;
}
