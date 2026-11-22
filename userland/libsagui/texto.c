// SPDX-License-Identifier: GPL-3.0-only
/* texto.c - memoria y texto para los programas. */
#include "sagui.h"

/* memcpy, memmove e memset están en rapido.c (rep movsd o SSE2, D042). */

int memcmp(const void *x, const void *y, size_t n)
{
    const uint8_t *a = x, *b = y;
    for (; n; n--, a++, b++)
        if (*a != *b) return *a - *b;
    return 0;
}

size_t tam_texto(const char *s)
{
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

int compara_texto(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

int compara_texto_n(const char *a, const char *b, size_t n)
{
    for (; n; n--, a++, b++) {
        if (*a != *b) return (uint8_t)*a - (uint8_t)*b;
        if (!*a) return 0;
    }
    return 0;
}

char *copia_texto(char *dst, const char *src, size_t max)
{
    size_t i = 0;
    if (!max) return dst;
    for (; i + 1 < max && src[i]; i++) dst[i] = src[i];
    dst[i] = 0;
    return dst;
}

char *junta_texto(char *dst, const char *src, size_t max)
{
    size_t n = tam_texto(dst);
    if (n < max) copia_texto(dst + n, src, max - n);
    return dst;
}

char *acha_char(const char *s, int c)
{
    for (; *s; s++)
        if (*s == (char)c) return (char *)s;
    return c == 0 ? (char *)s : NULL;
}

char *acha_ultimo_char(const char *s, int c)
{
    const char *achou = NULL;
    for (; *s; s++)
        if (*s == (char)c) achou = s;
    return (char *)achou;
}

bool comeca_com(const char *s, const char *p)
{
    while (*p)
        if (*s++ != *p++) return false;
    return true;
}

bool eh_espaco(int c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/* Convierte, en su lugar, secuencias UTF-8 válidas de caracteres de Latin-1
 * (U+0080 a U+00FF, dos bytes: C2/C3 + continuación) a un solo byte. El
 * resto queda como está. Devuelve el nuevo tamaño. Si el texto termina en medio
 * de una secuencia, *sobra recibe cuántos bytes del final quedaron pendientes
 * (para juntar con el siguiente bloque); con sobra = NULL, quedan como están. */
size_t utf8_para_latin1(char *s, size_t n, size_t *sobra)
{
    size_t i = 0, k = 0;
    if (sobra) *sobra = 0;
    while (i < n) {
        uint8_t c = (uint8_t)s[i];
        if ((c == 0xC2 || c == 0xC3) && i + 1 >= n && sobra) { *sobra = 1; break; }
        if ((c == 0xC2 || c == 0xC3) && i + 1 < n && ((uint8_t)s[i + 1] & 0xC0) == 0x80) {
            s[k++] = (char)(((c & 0x03) << 6) | ((uint8_t)s[i + 1] & 0x3F));
            i += 2;
            continue;
        }
        s[k++] = s[i++];
    }
    return k;
}

int texto_para_int(const char *s)
{
    int sinal = 1, v = 0;
    while (eh_espaco(*s)) s++;
    if (*s == '-') { sinal = -1; s++; } else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return v * sinal;
}
