// SPDX-License-Identifier: GPL-2.0-or-later
/* wordart_fuentes.c - las letras vectoriales (DejaVu) de los WordArt (D296, extraído de Pluma para Tucán, D369).
 * Instala el proveedor de máscaras de wordart.h con los contornos de la libfontes. Sin FPU en la libsagui:
 * esto está aquí porque la libfontes sí usa la x87. */
#include "fontes.h"
#include "wordart.h"

static fonte_t *fonte_wa(int fam, int est)
{
    static fonte_t *cache[3][4];
    static bool tentou[3][4];
    if (fam < 0 || fam > 2) fam = 0;
    int k = est & 3;
    if (tentou[fam][k]) return cache[fam][k];
    tentou[fam][k] = true;
    static const char *const base[3] = { "DejaVuSans", "DejaVuSerif", "DejaVuSansMono" };
    static const char *const dirs[2] = { "/fontes", FUENTES_DIR_APPS };
    bool neg = (est & 1) != 0, cur = (est & 2) != 0, real = cur && fam == 1 && !neg;
    fonte_t *f = NULL;
    for (int d = 0; d < 2 && !f; d++) {
        char ruta[120];
        formatar_texto(ruta, sizeof ruta, "%s/%s%s.ttf", dirs[d], base[fam], neg ? "-Bold" : real ? "-Italic" : "");
        f = fonte_abrir_archivo(ruta);
    }
    if (f && cur && !real) { fonte_t *o = fonte_obliqua(f); if (o) f = o; }
    cache[fam][k] = f;
    return f;
}

static bool mascara_fonte(const char *t, int fam, int est, int px, uint8_t **m, int *ml, int *ma)
{
    fonte_t *f = fonte_wa(fam, est);
    if (!f) return false;
    if (px < 8) px = 8;
    if (px > 500) px = 500;
    int asc = fonte_ascendente(f, px), desc = fonte_descendente(f, px);
    int pad = px / 6 + 2;
    int L = fonte_medir(f, t, px) + 2 * pad, A = asc + desc + 8;
    if (L < 4 || L > 6000) return false;
    uint32_t *buf = aloca_zerado((size_t)L * (size_t)A * 4);
    uint8_t *mk = aloca((size_t)L * (size_t)A);
    if (!buf || !mk) { libera(buf); libera(mk); return false; }
    fonte_texto_buf(f, buf, L, A, pad, 4, t, px, 0xFFFFFF);
    for (int i = 0; i < L * A; i++) mk[i] = (uint8_t)(buf[i] & 255);
    libera(buf);
    *m = mk; *ml = L; *ma = A;
    return true;
}

void wordart_fuentes_instalar(void) { wordart_provedor(mascara_fonte); }
