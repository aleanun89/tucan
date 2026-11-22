// SPDX-License-Identifier: GPL-2.0-or-later
/* rapido_c.c - las rutas «rápidas» de píxeles y memoria de la libsagui (rapido.c) en C llano (D459).
 *
 * En SaguiOS rapido.c elige entre rep movsd y SSE por CPUID y reemplaza memcpy/memset; en la capa portable
 * el compilador y la libc del PC ya lo hacen bien, así que aquí quedan solo las funciones de píxeles. */
#include <string.h>
#include "sagui.h"

bool cpu_tem_sse(void)   { return false; }
bool cpu_tem_sse2(void)  { return false; }
bool cpu_tem_sse3(void)  { return false; }
bool cpu_tem_ssse3(void) { return false; }
bool cpu_tem_avx(void)   { return false; }
bool cpu_tem_avx2(void)  { return false; }
bool cpu_tem_fma(void)   { return false; }

int mem_metodo(int m) { (void)m; return MEM_SIMPLES; }
int mem_escolhas(char *s, size_t max) { if (max) s[0] = 0; return 0; }
int mem_descrever(char *s, size_t max) { copia_texto(s, "libc del anfitrion", max); return 0; }

void preencher32(uint32_t *d, uint32_t cor, size_t n) { for (size_t i = 0; i < n; i++) d[i] = cor; }

void ampliar2x32(uint32_t *d, uint32_t *d2, const uint32_t *o, size_t n)
{
    for (size_t i = 0; i < n; i++) { d[2 * i] = d[2 * i + 1] = o[i]; }
    if (d2) memcpy(d2, d, n * 8);
}

void escurecer32(uint32_t *d, size_t n, int desloc, uint32_t mascara)
{
    for (size_t i = 0; i < n; i++) d[i] = (d[i] >> desloc) & mascara;
}

void trocar_rb32(uint32_t *d, const uint32_t *o, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        uint32_t p = o[i];
        d[i] = (p & 0xFF00FF00u) | ((p & 0xFF) << 16) | ((p >> 16) & 0xFF);
    }
}

void misturar32(uint32_t *d, const uint32_t *o, size_t n, unsigned a)
{
    for (size_t i = 0; i < n; i++) {
        uint32_t p = o[i], q = d[i];
        uint32_t rb = (((p & 0xFF00FFu) * a + (q & 0xFF00FFu) * (256 - a)) >> 8) & 0xFF00FFu;
        uint32_t g = (((p & 0x00FF00u) * a + (q & 0x00FF00u) * (256 - a)) >> 8) & 0x00FF00u;
        d[i] = rb | g;
    }
}
