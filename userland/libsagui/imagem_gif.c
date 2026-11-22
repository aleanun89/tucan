/* imagem_gif.c - lector de GIF de la libsagui (D185).
 *
 * Lee el primer cuadro de un GIF87a/89a (los GIF animados dan solo el primero): paleta global o
 * local, entrelazado, LZW de 2 a 12 bits y color transparente (que se mezcla con blanco, como
 * hace el lector de PNG). Todo en enteros. Es el formato más común en los proyectos JClic. */
#include "imagem.h"

#define GIF_LZW_MAX 4096

static int gif_subbloques(const uint8_t *d, int n, int *p)
{
    /* salta los subbloques de datos hasta el terminador; false si se acaba el archivo */
    while (*p < n) {
        int t = d[(*p)++];
        if (t == 0) return 0;
        *p += t;
    }
    return IMG_ERRO_CORROMPIDO;
}

int imagem_ler_gif(const uint8_t *d, int n, imagem_t *img)
{
    if (n < 13 || (memcmp(d, "GIF87a", 6) && memcmp(d, "GIF89a", 6))) return IMG_ERRO_FORMATO;
    int pl = d[6] | (d[7] << 8), pa = d[8] | (d[9] << 8);
    unsigned flg = d[10];
    int p = 13;
    const uint8_t *pal_g = NULL;
    int n_g = 0;
    if (flg & 0x80) {
        n_g = 2 << (flg & 7);
        if (p + 3 * n_g > n) return IMG_ERRO_CORROMPIDO;
        pal_g = d + p;
        p += 3 * n_g;
    }
    int transp = -1;
    while (p < n) {
        int b = d[p++];
        if (b == 0x3B) break;
        if (b == 0x21) {   /* extensión */
            if (p >= n) return IMG_ERRO_CORROMPIDO;
            int rot = d[p++];
            if (rot == 0xF9 && p + 5 < n && d[p] >= 4) {
                if (d[p + 1] & 1) transp = d[p + 4];
            }
            int r = gif_subbloques(d, n, &p);
            if (r) return r;
            continue;
        }
        if (b != 0x2C) return IMG_ERRO_CORROMPIDO;
        if (p + 9 > n) return IMG_ERRO_CORROMPIDO;
        int ix = d[p] | (d[p + 1] << 8), iy = d[p + 2] | (d[p + 3] << 8);
        int il = d[p + 4] | (d[p + 5] << 8), ia = d[p + 6] | (d[p + 7] << 8);
        unsigned f = d[p + 8];
        p += 9;
        const uint8_t *pal = pal_g;
        int np = n_g;
        if (f & 0x80) {
            np = 2 << (f & 7);
            if (p + 3 * np > n) return IMG_ERRO_CORROMPIDO;
            pal = d + p;
            p += 3 * np;
        }
        if (!pal) return IMG_ERRO_CORROMPIDO;
        if (il <= 0 || ia <= 0) return IMG_ERRO_CORROMPIDO;
        if (pl <= 0 || pa <= 0) { pl = ix + il; pa = iy + ia; }
        if (ix + il > pl) pl = ix + il;
        if (iy + ia > pa) pa = iy + ia;
        if (pl > IMAGEM_LADO_MAX || pa > IMAGEM_LADO_MAX || (long)pl * pa > IMAGEM_PIXELS_MAX) return IMG_ERRO_GRANDE;
        if (p >= n) return IMG_ERRO_CORROMPIDO;
        int tam_min = d[p++];
        if (tam_min < 2 || tam_min > 8) return IMG_ERRO_CORROMPIDO;
        if (!imagem_criar(img, pl, pa, 0xFFFFFF)) return IMG_ERRO_MEMORIA;

        /* LZW: la tabla guarda, por código, el prefijo y el último byte (se recorre al revés) */
        uint16_t *prefixo = aloca(GIF_LZW_MAX * sizeof(uint16_t));
        uint8_t *sufixo = aloca(GIF_LZW_MAX);
        uint8_t *pilha = aloca(GIF_LZW_MAX + 1);
        if (!prefixo || !sufixo || !pilha) {
            libera(prefixo); libera(sufixo); libera(pilha);
            imagem_liberar(img);
            return IMG_ERRO_MEMORIA;
        }
        int limpar = 1 << tam_min, fim = limpar + 1, proximo = limpar + 2;
        int bits = tam_min + 1, anterior = -1;
        uint32_t acum = 0;
        int nacum = 0, x = 0, y = 0, passo = (f & 0x40) ? 0 : -1;
        int linha_i = 0;   /* entrelazado: pasada actual */
        int bloco_fim = 0, bloco_pos = 0;   /* bytes que quedan del subbloque actual */
        bool acabou = false;
        for (int k = 0; k < limpar; k++) { prefixo[k] = 0xFFFF; sufixo[k] = (uint8_t)k; }
        while (!acabou) {
            while (nacum < bits) {
                if (bloco_pos >= bloco_fim) {
                    if (p >= n) { acabou = true; break; }
                    bloco_fim = d[p++];
                    bloco_pos = 0;
                    if (bloco_fim == 0) { acabou = true; break; }
                    if (p + bloco_fim > n) bloco_fim = n - p;
                }
                acum |= (uint32_t)d[p++] << nacum;
                bloco_pos++;
                nacum += 8;
            }
            if (acabou && nacum < bits) break;
            int cod = (int)(acum & ((1u << bits) - 1));
            acum >>= bits;
            nacum -= bits;
            if (cod == limpar) { proximo = limpar + 2; bits = tam_min + 1; anterior = -1; continue; }
            if (cod == fim) break;
            int topo = 0, atual = cod;
            if (anterior < 0) {
                if (cod >= limpar) break;
                pilha[topo++] = (uint8_t)cod;
            } else {
                if (cod > proximo || (cod == proximo && proximo >= GIF_LZW_MAX)) break;
                if (cod == proximo) {   /* el caso KwKwK: anterior + su primer byte */
                    int t = anterior;
                    while (prefixo[t] != 0xFFFF) t = prefixo[t];
                    pilha[topo++] = sufixo[t];
                    atual = anterior;
                }
                while (atual != 0xFFFF && topo < GIF_LZW_MAX) { pilha[topo++] = sufixo[atual]; atual = prefixo[atual]; }
                if (proximo < GIF_LZW_MAX) {
                    prefixo[proximo] = (uint16_t)anterior;
                    sufixo[proximo] = pilha[topo - 1];
                    proximo++;
                    if (proximo == (1 << bits) && bits < 12) bits++;
                }
            }
            anterior = cod;
            while (topo > 0) {
                int idx = pilha[--topo];
                if (y < ia) {
                    int yy = iy + y, xx = ix + x;
                    if (xx < pl && yy < pa && idx != transp && idx < np) {
                        const uint8_t *c = pal + 3 * idx;
                        img->px[yy * pl + xx] = ((uint32_t)c[0] << 16) | ((uint32_t)c[1] << 8) | c[2];
                    }
                }
                if (++x >= il) {
                    x = 0;
                    if (passo < 0) y++;
                    else {   /* entrelazado: 0 de 8 en 8, 4 de 8 en 8, 2 de 4 en 4, 1 de 2 en 2 */
                        static const uint8_t ini[4] = { 0, 4, 2, 1 }, inc[4] = { 8, 8, 4, 2 };
                        y += inc[linha_i];
                        while (y >= ia && linha_i < 3) { linha_i++; y = ini[linha_i]; }
                    }
                }
            }
        }
        libera(prefixo); libera(sufixo); libera(pilha);
        formatar_texto(img->formato, sizeof img->formato, "GIF %d bits", tam_min);
        return 0;
    }
    return IMG_ERRO_CORROMPIDO;
}
