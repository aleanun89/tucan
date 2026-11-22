// SPDX-License-Identifier: GPL-2.0-or-later
/* difer.c - diferencia por líneas con el algoritmo de Myers (D240).
 *
 * Se recorta el prefijo y el sufijo comunes y, con lo que queda en medio, se
 * busca el camino más corto en el grafo de edición guardando una copia de la
 * diagonal en cada paso (memoria O(D^2), D = líneas distintas). Con DIFER_PC se
 * compila en el anfitrión para la prueba de unidad (malloc en vez de aloca). */
#ifdef DIFER_PC
#include <stdlib.h>
#define D_ALOCA(n) malloc(n)
#define D_LIBERA(p) free(p)
#else
#include "sagui.h"
#define D_ALOCA(n) aloca(n)
#define D_LIBERA(p) libera(p)
#endif
#include "difer.h"

static int igual(const difer_linha_t *x, const difer_linha_t *y)
{
    if (x->n != y->n) return 0;
    for (int i = 0; i < x->n; i++) if (x->p[i] != y->p[i]) return 0;
    return 1;
}

int difer_partir(const char *s, int n, difer_linha_t *linhas, int max)
{
    int k = 0, ini = 0;
    for (int i = 0; i <= n; i++) {
        if (i == n || s[i] == '\n') {
            if (i == n && ini == n) break;   /* nada después del último '\n' */
            if (k < max) { linhas[k].p = s + ini; linhas[k].n = i - ini; k++; }
            ini = i + 1;
        }
    }
    return k;
}

int difer_linhas(const difer_linha_t *a, int na, const difer_linha_t *b, int nb,
                 difer_op_t *ops, int max_ops)
{
    int pre = 0;
    while (pre < na && pre < nb && igual(&a[pre], &b[pre])) pre++;
    int suf = 0;
    while (suf < na - pre && suf < nb - pre && igual(&a[na - 1 - suf], &b[nb - 1 - suf])) suf++;
    int n = na - pre - suf, m = nb - pre - suf;
    const difer_linha_t *A = a + pre, *B = b + pre;
    int total = 0;
    #define EMITIR(t, x, y) do { if (total >= max_ops) goto sem_lugar; ops[total].tipo = (t); ops[total].ia = (x); ops[total].ib = (y); total++; } while (0)
    int **trilha = NULL;
    int dmax = n + m;
    if (dmax > DIFER_D_MAX) dmax = DIFER_D_MAX;

    for (int i = 0; i < pre; i++) EMITIR(DIFER_IGUAL, i, i);

    if (n == 0 || m == 0) {
        for (int i = 0; i < n; i++) EMITIR(DIFER_QUITA, pre + i, pre);
        for (int j = 0; j < m; j++) EMITIR(DIFER_ANADE, pre, pre + j);
    } else {
        int off = dmax + 1, w = 2 * dmax + 3;
        int *v = D_ALOCA((size_t)w * sizeof(int));
        trilha = D_ALOCA((size_t)(dmax + 1) * sizeof(int *));
        if (!v || !trilha) { if (v) D_LIBERA(v); if (trilha) D_LIBERA(trilha); return -1; }
        int dfim = -1, nt = 0;
        for (int i = 0; i < w; i++) v[i] = 0;
        for (int d = 0; d <= dmax && dfim < 0; d++) {
            for (int k = -d; k <= d; k += 2) {
                int x;
                if (k == -d || (k != d && v[off + k - 1] < v[off + k + 1])) x = v[off + k + 1];
                else x = v[off + k - 1] + 1;
                int y = x - k;
                while (x < n && y < m && igual(&A[x], &B[y])) { x++; y++; }
                v[off + k] = x;
                if (x >= n && y >= m) { dfim = d; break; }
            }
            /* copia de la diagonal -d-1..d+1 al terminar el paso d */
            int *c = D_ALOCA((size_t)(2 * d + 3) * sizeof(int));
            if (!c) { dfim = -2; break; }
            for (int k = -d - 1; k <= d + 1; k++) c[k + d + 1] = v[off + k];
            trilha[nt++] = c;
        }
        if (dfim < 0) {
            for (int i = 0; i < nt; i++) D_LIBERA(trilha[i]);
            D_LIBERA(trilha); D_LIBERA(v);
            return -1;
        }
        /* camino atrás: del final al comienzo, y luego se invierte */
        int cap = n + m + 1;
        difer_op_t *rev = D_ALOCA((size_t)cap * sizeof *rev);
        if (!rev) {
            for (int i = 0; i < nt; i++) D_LIBERA(trilha[i]);
            D_LIBERA(trilha); D_LIBERA(v);
            return -1;
        }
        int nr = 0, x = n, y = m;
        for (int d = dfim; d > 0; d--) {
            int *p = trilha[d - 1];      /* diagonales del paso d-1 */
            int dp = d - 1;
            #define PV(k) p[(k) + dp + 1]
            int k = x - y, kp;
            if (k == -d || (k != d && PV(k - 1) < PV(k + 1))) kp = k + 1; else kp = k - 1;
            int xp = PV(kp);
            /* la serpiente va de (inicio de la arista) hasta (x,y) */
            int xs = kp == k + 1 ? xp : xp + 1;   /* x tras la arista */
            int ys = xs - k;
            while (x > xs && y > ys) { x--; y--; rev[nr].tipo = DIFER_IGUAL; rev[nr].ia = pre + x; rev[nr].ib = pre + y; nr++; }
            if (kp == k + 1) { y--; rev[nr].tipo = DIFER_ANADE; rev[nr].ia = pre + x; rev[nr].ib = pre + y; nr++; }
            else { x--; rev[nr].tipo = DIFER_QUITA; rev[nr].ia = pre + x; rev[nr].ib = pre + y; nr++; }
            #undef PV
        }
        while (x > 0 && y > 0) { x--; y--; rev[nr].tipo = DIFER_IGUAL; rev[nr].ia = pre + x; rev[nr].ib = pre + y; nr++; }
        for (int i = 0; i < nt; i++) D_LIBERA(trilha[i]);
        D_LIBERA(trilha); D_LIBERA(v);
        trilha = NULL;
        if (total + nr + suf > max_ops) { D_LIBERA(rev); return -1; }
        for (int i = nr - 1; i >= 0; i--) EMITIR(rev[i].tipo, rev[i].ia, rev[i].ib);
        D_LIBERA(rev);
    }
    for (int i = 0; i < suf; i++) EMITIR(DIFER_IGUAL, na - suf + i, nb - suf + i);
    return total;
sem_lugar:
    return -1;   /* no cabe en max_ops (aquí no hay memoria reservada pendiente) */
}
