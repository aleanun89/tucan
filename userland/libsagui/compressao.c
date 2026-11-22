// SPDX-License-Identifier: GPL-2.0-or-later
/* compressao.c - CRC-32, Adler-32, DEFLATE e gzip (mejora 056, D103).
 * Escrito a partir do RFC 1951 (deflate) e do RFC 1952 (gzip). O inflate é o
 * que já servia ao PNG (D054), movido para cá, agora com a contagem de bytes
 * usados e erros distintos. A compressão é própria: LZ77 com tabela hash e
 * cadeias (o nível dá o tamanho da busca), bloco com Huffman dinâmico, fixo ou
 * guardado, o que sair menor. */
#include "compressao.h"

const char *comp_nome_erro(int e)
{
    switch (e) {
    case COMP_ERRO_FORMATO:       return "datos corruptos o formato no válido";
    case COMP_ERRO_CHEIO:         return "el resultado no cabe";
    case COMP_ERRO_CRC:           return "la suma de comprobación (CRC) no coincide";
    case COMP_ERRO_TRUNCADO:      return "archivo truncado";
    case COMP_ERRO_NAO_SUPORTADO: return "no soportado (cifrado, ZIP64 o método desconocido)";
    case COMP_ERRO_MEMORIA:       return "sin memoria";
    case COMP_ERRO_GRANDE:        return "demasiado grande";
    }
    return nome_erro(e);
}

/* ------------------------------------------------------------ somas */

static uint32_t tab_crc[256];
static bool tab_crc_pronta;

uint32_t crc32_somar(uint32_t crc, const void *dados, size_t n)
{
    if (!tab_crc_pronta) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            tab_crc[i] = c;
        }
        tab_crc_pronta = true;
    }
    const uint8_t *p = dados;
    crc = ~crc;
    while (n--) crc = tab_crc[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

uint32_t adler32_somar(uint32_t adler, const void *dados, size_t n)
{
    const uint8_t *p = dados;
    uint32_t a = adler & 0xFFFF, b = adler >> 16;
    while (n) {
        size_t m = n < 5552 ? n : 5552;   /* o máximo sem estourar 32 bits */
        n -= m;
        while (m--) { a += *p++; b += a; }
        a %= 65521; b %= 65521;
    }
    return b << 16 | a;
}

/* ------------------------------------------------------------ tabelas comuns */

static const uint16_t base_comp[29] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
                                        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
static const uint8_t extra_comp[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
                                        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
static const uint16_t base_dist[30] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
                                        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
                                        8193, 12289, 16385, 24577 };
static const uint8_t extra_dist[30] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
                                        7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };
static const uint8_t ordem_cl[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

/* ------------------------------------------------------------ inflate (RFC 1951) */

#define RAPIDO 9   /* bits da tabela de busca direta */

typedef struct {
    const uint8_t *ent;
    int n, pos;
    uint32_t bits;   /* bits já lidos e ainda não usados (os de baixo primeiro) */
    int qtd;
    bool estourou;
} bits_t;

typedef struct {
    uint16_t contagem[16];   /* quantos códigos de cada comprimento */
    uint16_t simbolo[288];   /* símbolos em ordem de código */
    uint16_t rapida[1 << RAPIDO];   /* (comprimento << 12 | símbolo) pelos próximos RAPIDO bits; 0 = código longo */
} huff_t;

static inline void encher(bits_t *b, int n)
{
    while (b->qtd < n) {
        if (b->pos >= b->n) return;
        b->bits |= (uint32_t)b->ent[b->pos++] << b->qtd;
        b->qtd += 8;
    }
}

static int ler_bits(bits_t *b, int n)
{
    if (n == 0) return 0;
    encher(b, n);
    if (b->qtd < n) { b->estourou = true; return 0; }
    int v = (int)(b->bits & ((1u << n) - 1));
    b->bits >>= n;
    b->qtd -= n;
    return v;
}

static uint32_t inverter(uint32_t v, int n)
{
    uint32_t r = 0;
    while (n--) { r = r << 1 | (v & 1); v >>= 1; }
    return r;
}

/* Monta a tabela a partir dos comprimentos. Códigos canônicos: os de mesmo
 * comprimento são consecutivos, na ordem dos símbolos. A tabela rápida cobre
 * os códigos de até RAPIDO bits (os outros caem na decodificação bit a bit). */
static int construir(huff_t *h, const uint8_t *comp, int n)
{
    uint16_t desloc[16];
    uint32_t prox[16];
    for (int i = 0; i < 16; i++) h->contagem[i] = 0;
    for (int s = 0; s < n; s++) h->contagem[comp[s]]++;
    h->contagem[0] = 0;
    int sobra = 1;
    for (int len = 1; len < 16; len++) {
        sobra <<= 1;
        sobra -= h->contagem[len];
        if (sobra < 0) return -1;   /* códigos demais para o comprimento */
    }
    desloc[1] = 0;
    for (int len = 1; len < 15; len++) desloc[len + 1] = (uint16_t)(desloc[len] + h->contagem[len]);
    for (int s = 0; s < n; s++)
        if (comp[s]) h->simbolo[desloc[comp[s]]++] = (uint16_t)s;
    memset(h->rapida, 0, sizeof h->rapida);
    uint32_t cod = 0;
    for (int len = 1; len < 16; len++) { cod = (cod + h->contagem[len - 1]) << 1; prox[len] = cod; }
    for (int s = 0; s < n; s++) {
        int len = comp[s];
        if (!len || len > RAPIDO) continue;
        uint32_t c = prox[len]++;
        uint32_t r = inverter(c, len);
        for (uint32_t k = r; k < (1u << RAPIDO); k += 1u << len)
            h->rapida[k] = (uint16_t)(len << 12 | s);
    }
    return 0;
}

/* Decodifica um símbolo: tabela rápida primeiro; senão bit a bit, a cada
 * comprimento os códigos dele ocupam o intervalo [primeiro, primeiro + contagem). */
static int decodificar(bits_t *b, const huff_t *h)
{
    encher(b, RAPIDO);
    uint16_t r = h->rapida[b->bits & ((1u << RAPIDO) - 1)];
    if (r) {
        int len = r >> 12;
        if (len > b->qtd) { b->estourou = true; return -1; }
        b->bits >>= len;
        b->qtd -= len;
        return r & 0xFFF;
    }
    int codigo = 0, primeiro = 0, indice = 0;
    for (int len = 1; len < 16; len++) {
        codigo |= ler_bits(b, 1);
        if (b->estourou) return -1;
        int c = h->contagem[len];
        if (codigo - primeiro < c) return h->simbolo[indice + codigo - primeiro];
        indice += c;
        primeiro = (primeiro + c) << 1;
        codigo <<= 1;
    }
    return -1;
}

static int bloco(bits_t *b, const huff_t *hc, const huff_t *hd, uint8_t *s, int *k, int max)
{
    for (;;) {
        int sim = decodificar(b, hc);
        if (sim < 0) return b->estourou ? COMP_ERRO_TRUNCADO : COMP_ERRO_FORMATO;
        if (sim < 256) {
            if (*k >= max) return COMP_ERRO_CHEIO;
            s[(*k)++] = (uint8_t)sim;
        } else if (sim == 256) return 0;
        else {
            sim -= 257;
            if (sim >= 29) return COMP_ERRO_FORMATO;
            int len = base_comp[sim] + ler_bits(b, extra_comp[sim]);
            int ds = decodificar(b, hd);
            if (ds < 0 || ds >= 30) return b->estourou ? COMP_ERRO_TRUNCADO : COMP_ERRO_FORMATO;
            int dist = base_dist[ds] + ler_bits(b, extra_dist[ds]);
            if (b->estourou) return COMP_ERRO_TRUNCADO;
            if (dist > *k) return COMP_ERRO_FORMATO;
            if (*k + len > max) return COMP_ERRO_CHEIO;
            if (dist >= len) { memcpy(s + *k, s + *k - dist, (size_t)len); *k += len; }
            else for (int i = 0; i < len; i++, (*k)++) s[*k] = s[*k - dist];
        }
    }
}

/* El nucleo: *produzidos (opcional) recibe los bytes ya escritos aunque haya
 * error, para quien prefiera un flujo cortado a nada (el visor de PDF). */
static int inflar_nucleo(const uint8_t *ent, int n_ent, uint8_t *saida, int max_saida, int *usados,
                         int *produzidos)
{
    static huff_t hc, hd;   /* estáticas: a pilha do programa é pequena (64 KB) */
    bits_t b = { ent, n_ent, 0, 0, 0, false };
    uint8_t comp[320];
    int k = 0, ultimo, r;
#define FALHA(e) do { if (produzidos) *produzidos = k; return (e); } while (0)
    do {
        ultimo = ler_bits(&b, 1);
        int tipo = ler_bits(&b, 2);
        if (b.estourou) FALHA(COMP_ERRO_TRUNCADO);
        if (tipo == 0) {   /* guardado sem compressão */
            /* v8.5.4.2: los bytes enteros que ya estaban en el acumulador se devuelven a la entrada (antes se
             * perdían y un bloque guardado tras otro con bits de sobra no se leía bien); solo se tiran los bits
             * sueltos hasta el siguiente byte. */
            b.pos -= b.qtd / 8; b.bits = 0; b.qtd = 0;
            if (b.pos + 4 > b.n) FALHA(COMP_ERRO_TRUNCADO);
            int len = ent[b.pos] | ent[b.pos + 1] << 8, nlen = ent[b.pos + 2] | ent[b.pos + 3] << 8;
            b.pos += 4;
            if ((len ^ 0xFFFF) != nlen) FALHA(COMP_ERRO_FORMATO);
            if (b.pos + len > b.n) FALHA(COMP_ERRO_TRUNCADO);
            if (k + len > max_saida) FALHA(COMP_ERRO_CHEIO);
            memcpy(saida + k, ent + b.pos, (size_t)len);
            b.pos += len;
            k += len;
            continue;
        }
        if (tipo == 1) {   /* códigos fixos */
            int s = 0;
            for (; s < 144; s++) comp[s] = 8;
            for (; s < 256; s++) comp[s] = 9;
            for (; s < 280; s++) comp[s] = 7;
            for (; s < 288; s++) comp[s] = 8;
            construir(&hc, comp, 288);
            for (s = 0; s < 30; s++) comp[s] = 5;
            construir(&hd, comp, 30);
        } else if (tipo == 2) {   /* códigos dinâmicos, descritos no próprio bloco */
            int nlit = ler_bits(&b, 5) + 257, ndist = ler_bits(&b, 5) + 1, ncod = ler_bits(&b, 4) + 4;
            if (nlit > 286 || ndist > 30) FALHA(COMP_ERRO_FORMATO);
            for (int i = 0; i < 19; i++) comp[ordem_cl[i]] = 0;
            for (int i = 0; i < ncod; i++) comp[ordem_cl[i]] = (uint8_t)ler_bits(&b, 3);
            if (b.estourou) FALHA(COMP_ERRO_TRUNCADO);
            if (construir(&hc, comp, 19) < 0) FALHA(COMP_ERRO_FORMATO);
            int i = 0;
            while (i < nlit + ndist) {
                int sim = decodificar(&b, &hc), rep, val = 0;
                if (sim < 0) FALHA(b.estourou ? COMP_ERRO_TRUNCADO : COMP_ERRO_FORMATO);
                if (sim < 16) { comp[i++] = (uint8_t)sim; continue; }
                if (sim == 16) { if (!i) FALHA(COMP_ERRO_FORMATO); val = comp[i - 1]; rep = 3 + ler_bits(&b, 2); }
                else if (sim == 17) rep = 3 + ler_bits(&b, 3);
                else rep = 11 + ler_bits(&b, 7);
                if (i + rep > nlit + ndist) FALHA(COMP_ERRO_FORMATO);
                while (rep--) comp[i++] = (uint8_t)val;
            }
            if (b.estourou) FALHA(COMP_ERRO_TRUNCADO);
            if (comp[256] == 0) FALHA(COMP_ERRO_FORMATO);   /* sem o código de fim de bloco */
            if (construir(&hc, comp, nlit) < 0 || construir(&hd, comp + nlit, ndist) < 0) FALHA(COMP_ERRO_FORMATO);
        } else FALHA(COMP_ERRO_FORMATO);
        r = bloco(&b, &hc, &hd, saida, &k, max_saida);
        if (r < 0) FALHA(r);
    } while (!ultimo);
    if (usados) *usados = b.pos;
    if (produzidos) *produzidos = k;
    return k;
#undef FALHA
}

int inflar_usados(const uint8_t *ent, int n_ent, uint8_t *saida, int max_saida, int *usados)
{
    return inflar_nucleo(ent, n_ent, saida, max_saida, usados, NULL);
}

int inflar_parcial(const uint8_t *ent, int n_ent, uint8_t *saida, int max_saida, int *produzidos)
{
    return inflar_nucleo(ent, n_ent, saida, max_saida, NULL, produzidos);
}

int inflar(const uint8_t *ent, int n_ent, uint8_t *saida, int max_saida)
{
    return inflar_usados(ent, n_ent, saida, max_saida, NULL);
}

/* ------------------------------------------------------------ deflate (compressão) */

typedef struct {
    uint8_t *s;
    int pos, max;
    uint32_t acc;
    int nb;
    bool cheio;
} esc_t;

static void por_bits(esc_t *w, uint32_t v, int n)
{
    w->acc |= v << w->nb;
    w->nb += n;
    while (w->nb >= 8) {
        if (w->pos >= w->max) { w->cheio = true; w->nb = 0; w->acc = 0; return; }
        w->s[w->pos++] = (uint8_t)w->acc;
        w->acc >>= 8;
        w->nb -= 8;
    }
}

static void alinhar(esc_t *w) { if (w->nb > 0) por_bits(w, 0, 8 - w->nb); }

#define JANELA 32768
#define MAX_SIM 32767   /* símbolos por bloco */

/* D378: el estado del compresor (384 KB) se pide al malloc la primera vez que
 * se comprime, en vez de estar en la BSS: así no lo paga cada programa que
 * cargue la libsagui compartida solo para descomprimir. */
static struct {
    uint32_t cab[1 << 15];     /* última posição (+1) de cada hash */
    uint32_t ant[JANELA];      /* posição anterior (+1) com o mesmo hash */
    uint16_t sim_l[MAX_SIM + 1];   /* literal (dist 0) ou comprimento do par */
    uint16_t sim_d[MAX_SIM + 1];
} *deflar_estado;
#define cab   (deflar_estado->cab)
#define ant   (deflar_estado->ant)
#define sim_l (deflar_estado->sim_l)
#define sim_d (deflar_estado->sim_d)
static uint8_t  tab_len[259];
static bool     tab_len_pronta;

static int cod_dist(int d)
{
    int c = 29;
    while (base_dist[c] > d) c--;
    return c;
}

static void preparar_tabelas(void)
{
    if (tab_len_pronta) return;
    for (int l = 3; l <= 258; l++) {
        int c = 28;
        while (base_comp[c] > l) c--;
        tab_len[l] = (uint8_t)c;
    }
    tab_len_pronta = true;
}

/* Comprimentos de código com limite (Huffman por mesclagem dos dois menores;
 * se passar do limite, achata os pesos e repete). */
static void comprimentos(const uint32_t *freq, int n, int limite, uint8_t *comp)
{
    uint32_t f[288];
    uint32_t peso[576];
    int pai[576], vivo[288];
    int usados = 0;
    for (int i = 0; i < n; i++) { f[i] = freq[i]; if (f[i]) usados++; }
    for (int i = 0; usados < 2 && i < 2; i++) if (!f[i]) { f[i] = 1; usados++; }   /* deflate quer pelo menos dois códigos */
    for (;;) {
        int nv = 0, nn = n;
        for (int i = 0; i < n; i++) { peso[i] = f[i]; pai[i] = -1; comp[i] = 0; if (f[i]) vivo[nv++] = i; }
        while (nv > 1) {
            int a = 0, b = 1;
            if (peso[vivo[b]] < peso[vivo[a]]) { a = 1; b = 0; }
            for (int i = 2; i < nv; i++) {
                if (peso[vivo[i]] < peso[vivo[a]]) { b = a; a = i; }
                else if (peso[vivo[i]] < peso[vivo[b]]) b = i;
            }
            int na = vivo[a], nb2 = vivo[b];
            peso[nn] = peso[na] + peso[nb2];
            pai[nn] = -1;
            pai[na] = pai[nb2] = nn;
            vivo[a] = nn++;
            vivo[b] = vivo[--nv];
        }
        int pior = 0;
        for (int i = 0; i < n; i++) {
            if (!f[i]) continue;
            int d = 0;
            for (int p = pai[i]; p >= 0; p = pai[p]) d++;
            comp[i] = (uint8_t)d;
            if (d > pior) pior = d;
        }
        if (pior <= limite) return;
        for (int i = 0; i < n; i++) if (f[i]) f[i] = (f[i] + 1) >> 1;
    }
}

static void codigos(const uint8_t *comp, int n, uint16_t *cod)
{
    uint16_t cont[16] = { 0 };
    uint32_t prox[16];
    for (int i = 0; i < n; i++) cont[comp[i]]++;
    cont[0] = 0;
    uint32_t c = 0;
    for (int len = 1; len < 16; len++) { c = (c + cont[len - 1]) << 1; prox[len] = c; }
    for (int i = 0; i < n; i++) cod[i] = comp[i] ? (uint16_t)inverter(prox[comp[i]]++, comp[i]) : 0;
}

/* Descrição dos comprimentos (símbolos 16, 17, 18 repetem): escreve em w, ou só
 * conta os bits se w == NULL. rl[] recebe os comprimentos do código dos comprimentos. */
static int descrever(esc_t *w, const uint8_t *tot, int n, const uint8_t *cl_comp, const uint16_t *cl_cod,
                     uint32_t *cl_freq)
{
    int bits = 0;
    for (int i = 0; i < n;) {
        int v = tot[i], rep = 1;
        while (i + rep < n && tot[i + rep] == v) rep++;
        int usado = rep;
        if (v == 0 && rep >= 3) {
            if (rep > 138) rep = 138;
            usado = rep;
            int s = rep >= 11 ? 18 : 17, ex = rep >= 11 ? 7 : 3, val = rep >= 11 ? rep - 11 : rep - 3;
            if (cl_freq) cl_freq[s]++;
            if (w) { por_bits(w, cl_cod[s], cl_comp[s]); por_bits(w, (uint32_t)val, ex); }
            bits += (cl_comp ? cl_comp[s] : 0) + ex;
        } else {
            /* primeiro o valor, depois repetições (16) de 3 a 6 */
            if (cl_freq) cl_freq[v]++;
            if (w) por_bits(w, cl_cod[v], cl_comp[v]);
            bits += cl_comp ? cl_comp[v] : 0;
            int r = rep - 1;
            usado = 1;
            while (r >= 3) {
                int m = r > 6 ? 6 : r;
                if (cl_freq) cl_freq[16]++;
                if (w) { por_bits(w, cl_cod[16], cl_comp[16]); por_bits(w, (uint32_t)(m - 3), 2); }
                bits += (cl_comp ? cl_comp[16] : 0) + 2;
                r -= m;
                usado += m;
            }
            /* sobras de 1 ou 2: valores soltos */
            while (r > 0) {
                if (cl_freq) cl_freq[v]++;
                if (w) por_bits(w, cl_cod[v], cl_comp[v]);
                bits += cl_comp ? cl_comp[v] : 0;
                r--;
                usado++;
            }
        }
        i += usado;
    }
    return bits;
}

static void emitir_bloco(esc_t *w, const uint8_t *cru, int ini, int fim, int nsim, bool ultimo)
{
    uint32_t fl[288] = { 0 }, fd[30] = { 0 };
    uint8_t cl[288 + 30], cd[30], fixo_l[288];
    uint16_t codl[288], codd[30];
    int extras = 0;
    fl[256] = 1;
    for (int i = 0; i < nsim; i++) {
        if (!sim_d[i]) fl[sim_l[i]]++;
        else {
            int c = tab_len[sim_l[i]], d = cod_dist(sim_d[i]);
            fl[257 + c]++; fd[d]++;
            extras += extra_comp[c] + extra_dist[d];
        }
    }
    comprimentos(fl, 286, 15, cl);
    comprimentos(fd, 30, 15, cd);
    int nlit = 286, ndist = 30;
    while (nlit > 257 && !cl[nlit - 1]) nlit--;
    while (ndist > 1 && !cd[ndist - 1]) ndist--;
    uint8_t tot[288 + 30];
    memcpy(tot, cl, (size_t)nlit);
    memcpy(tot + nlit, cd, (size_t)ndist);
    uint32_t cf[19] = { 0 };
    descrever(NULL, tot, nlit + ndist, NULL, NULL, cf);
    uint8_t cc[19];
    uint16_t ccod[19];
    comprimentos(cf, 19, 7, cc);
    codigos(cc, 19, ccod);
    int ncod = 19;
    while (ncod > 4 && !cc[ordem_cl[ncod - 1]]) ncod--;
    int cab_bits = 14 + 3 * ncod + descrever(NULL, tot, nlit + ndist, cc, ccod, NULL);
    long din = 3 + cab_bits + extras, fix = 3 + extras;
    for (int i = 0; i < 288; i++) fixo_l[i] = i < 144 ? 8 : i < 256 ? 9 : i < 280 ? 7 : 8;
    for (int i = 0; i < 286; i++) { din += (long)fl[i] * cl[i]; fix += (long)fl[i] * fixo_l[i]; }
    for (int i = 0; i < 30; i++) { din += (long)fd[i] * cd[i]; fix += (long)fd[i] * 5; }
    int raw = fim - ini;
    long guard = (long)raw * 8 + 40L * ((raw + 65534) / 65535 ? (raw + 65534) / 65535 : 1) + 7;
    if (guard <= din && guard <= fix) {   /* guardado */
        int resta = raw;
        const uint8_t *p = cru + ini;
        do {
            int m = resta > 65535 ? 65535 : resta;
            resta -= m;
            por_bits(w, (ultimo && !resta) ? 1 : 0, 1);
            por_bits(w, 0, 2);
            alinhar(w);
            por_bits(w, (uint32_t)m, 16);
            por_bits(w, (uint32_t)m ^ 0xFFFF, 16);
            for (int i = 0; i < m; i++) por_bits(w, p[i], 8);
            p += m;
        } while (resta > 0);
        return;
    }
    if (fix <= din) {
        por_bits(w, ultimo ? 1 : 0, 1);
        por_bits(w, 1, 2);
        memcpy(cl, fixo_l, 288);
        for (int i = 0; i < 30; i++) cd[i] = 5;
        codigos(cl, 288, codl);
        codigos(cd, 30, codd);
    } else {
        por_bits(w, ultimo ? 1 : 0, 1);
        por_bits(w, 2, 2);
        por_bits(w, (uint32_t)(nlit - 257), 5);
        por_bits(w, (uint32_t)(ndist - 1), 5);
        por_bits(w, (uint32_t)(ncod - 4), 4);
        for (int i = 0; i < ncod; i++) por_bits(w, cc[ordem_cl[i]], 3);
        descrever(w, tot, nlit + ndist, cc, ccod, NULL);
        codigos(cl, 286, codl);
        codigos(cd, 30, codd);
    }
    for (int i = 0; i < nsim; i++) {
        if (!sim_d[i]) { por_bits(w, codl[sim_l[i]], cl[sim_l[i]]); continue; }
        int c = tab_len[sim_l[i]], d = cod_dist(sim_d[i]);
        por_bits(w, codl[257 + c], cl[257 + c]);
        por_bits(w, (uint32_t)(sim_l[i] - base_comp[c]), extra_comp[c]);
        por_bits(w, codd[d], cd[d]);
        por_bits(w, (uint32_t)(sim_d[i] - base_dist[d]), extra_dist[d]);
    }
    por_bits(w, codl[256], cl[256]);
}

static uint32_t hash3(const uint8_t *p)
{
    return (((uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16) * 2654435761u) >> 17;
}

static void inserir(const uint8_t *e, int n, int p)
{
    if (p + 3 > n) return;
    uint32_t h = hash3(e + p);
    ant[p & (JANELA - 1)] = cab[h];
    cab[h] = (uint32_t)p + 1;
}

static int mais_longa(const uint8_t *e, int n, int pos, int cadeia, int bom, int *dist)
{
    int max = n - pos > 258 ? 258 : n - pos;
    if (max < 3) return 0;
    int melhor = 2;
    uint32_t cand = cab[hash3(e + pos)];
    while (cand && cadeia-- > 0) {
        int c = (int)cand - 1, d = pos - c;
        if (d > JANELA - 1) break;
        if (melhor < max && e[c + melhor] == e[pos + melhor] && e[c] == e[pos]) {
            int l = 0;
            while (l < max && e[c + l] == e[pos + l]) l++;
            if (l > melhor && !(l == 3 && d > 4096)) {
                melhor = l; *dist = d;
                if (l >= bom || l == max) break;
            }
        }
        cand = ant[c & (JANELA - 1)];
    }
    return melhor >= 3 ? melhor : 0;
}

int deflar_limite(int n) { return n + (n >> 10) + 256; }

int deflar(const uint8_t *e, int n, uint8_t *saida, int max_saida, int nivel)
{
    static const uint16_t cadeias[10] = { 0, 4, 8, 32, 16, 32, 128, 256, 1024, 4096 };
    esc_t w = { saida, 0, max_saida, 0, 0, false };
    if (nivel < 0) nivel = 6;
    if (nivel > 9) nivel = 9;
    preparar_tabelas();
    if (nivel == 0 || n == 0) {   /* só guarda (ou o fluxo vazio) */
        if (n == 0) { por_bits(&w, 1, 1); por_bits(&w, 1, 2); por_bits(&w, 0, 7); }   /* bloco fixo só com o fim */
        else for (int p = 0; p < n;) {
            int m = n - p > 65535 ? 65535 : n - p;
            por_bits(&w, p + m >= n, 1);
            por_bits(&w, 0, 2);
            alinhar(&w);
            por_bits(&w, (uint32_t)m, 16);
            por_bits(&w, (uint32_t)m ^ 0xFFFF, 16);
            for (int i = 0; i < m; i++) por_bits(&w, e[p + i], 8);
            p += m;
        }
        alinhar(&w);
        return w.cheio ? COMP_ERRO_CHEIO : w.pos;
    }
    if (!deflar_estado && !(deflar_estado = aloca(sizeof *deflar_estado))) return COMP_ERRO_MEMORIA;
    memset(cab, 0, sizeof cab);
    int cadeia = cadeias[nivel];
    int bom = nivel < 4 ? 32 : nivel < 7 ? 128 : 258;
    int preguicoso = nivel >= 4 ? (nivel < 7 ? 16 : nivel < 9 ? 64 : 258) : 0;
    int pos = 0, ini = 0, nsim = 0;
    while (pos < n) {
        int d1 = 0, l1 = mais_longa(e, n, pos, cadeia, bom, &d1);
        inserir(e, n, pos);
        if (l1 && preguicoso && l1 < preguicoso && pos + 1 < n) {
            int d2 = 0, l2 = mais_longa(e, n, pos + 1, cadeia, bom, &d2);
            if (l2 > l1) l1 = 0;   /* melhor esperar: este vira literal */
        }
        if (l1) {
            sim_l[nsim] = (uint16_t)l1; sim_d[nsim++] = (uint16_t)d1;
            for (int i = 1; i < l1; i++) inserir(e, n, pos + i);
            pos += l1;
        } else {
            sim_l[nsim] = e[pos]; sim_d[nsim++] = 0;
            pos++;
        }
        if (nsim >= MAX_SIM) {
            emitir_bloco(&w, e, ini, pos, nsim, pos >= n);
            ini = pos; nsim = 0;
        }
    }
    if (nsim || ini < n || n == 0) emitir_bloco(&w, e, ini, pos, nsim, true);
    alinhar(&w);
    return w.cheio ? COMP_ERRO_CHEIO : w.pos;
}

/* ------------------------------------------------------------ gzip (RFC 1952) */

static void p32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static uint32_t l32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }

bool eh_gzip(const uint8_t *d, int n) { return n >= 18 && d[0] == 0x1F && d[1] == 0x8B && d[2] == 8; }

int gzip_comprimir(const uint8_t *ent, int n, const char *nome, uint32_t mtime, int nivel,
                   uint8_t *saida, int max_saida)
{
    int tn = nome ? (int)tam_texto(nome) + 1 : 0;
    if (max_saida < 10 + tn + 8) return COMP_ERRO_CHEIO;
    uint8_t *s = saida;
    s[0] = 0x1F; s[1] = 0x8B; s[2] = 8; s[3] = nome ? 8 : 0;
    p32(s + 4, mtime);
    s[8] = nivel == 9 ? 2 : nivel == 1 ? 4 : 0;
    s[9] = 3;   /* Unix */
    if (nome) memcpy(s + 10, nome, (size_t)tn);
    int k = 10 + tn;
    int r = deflar(ent, n, s + k, max_saida - k - 8, nivel);
    if (r < 0) return r;
    k += r;
    p32(s + k, crc32_somar(0, ent, (size_t)n));
    p32(s + k + 4, (uint32_t)n);
    return k + 8;
}

int gzip_descomprimir(const uint8_t *ent, int n, uint8_t **saida, int *tam, char *nome, int max_nome)
{
    int cap = n * 4 + 4096, total = 0, pos = 0, membros = 0;
    if (n >= 8) { uint32_t hint = l32(ent + n - 4); if (hint < COMP_LIMITE_MAX && (int)hint + 64 > cap) cap = (int)hint + 64; }
    uint8_t *out = aloca((size_t)cap);
    if (!out) return COMP_ERRO_MEMORIA;
    if (nome && max_nome) nome[0] = 0;
    while (pos < n) {
        const uint8_t *h = ent + pos;
        int resto = n - pos;
        if (resto >= 2 && h[0] == 0 && h[1] == 0 && membros) break;   /* enchimento de zeros depois do último */
        if (resto < 18 || h[0] != 0x1F || h[1] != 0x8B) { libera(out); return membros ? COMP_ERRO_FORMATO : (resto < 10 ? COMP_ERRO_TRUNCADO : COMP_ERRO_FORMATO); }
        if (h[2] != 8) { libera(out); return COMP_ERRO_NAO_SUPORTADO; }
        int fl = h[3], k = 10;
        if (fl & 4) { if (k + 2 > resto) { libera(out); return COMP_ERRO_TRUNCADO; } k += 2 + (h[k] | h[k + 1] << 8); }
        if (fl & 8) {
            int ini = k;
            while (k < resto && h[k]) k++;
            if (k >= resto) { libera(out); return COMP_ERRO_TRUNCADO; }
            if (!membros && nome && max_nome) { copia_texto(nome, (const char *)h + ini, (size_t)max_nome); }
            k++;
        }
        if (fl & 16) { while (k < resto && h[k]) k++; k++; }
        if (fl & 2) k += 2;
        if (k + 8 > resto) { libera(out); return COMP_ERRO_TRUNCADO; }
        int usados = 0, r;
        for (;;) {
            r = inflar_usados(h + k, resto - k, out + total, cap - total, &usados);
            if (r != COMP_ERRO_CHEIO) break;
            if ((uint32_t)cap >= COMP_LIMITE_MAX) { libera(out); return COMP_ERRO_GRANDE; }
            int novo = cap * 2 > (int)COMP_LIMITE_MAX ? (int)COMP_LIMITE_MAX : cap * 2;
            uint8_t *t = realoca(out, (size_t)novo);
            if (!t) { libera(out); return COMP_ERRO_MEMORIA; }
            out = t; cap = novo;
        }
        if (r < 0) { libera(out); return r; }
        k += usados;
        if (k + 8 > resto) { libera(out); return COMP_ERRO_TRUNCADO; }
        if (l32(h + k) != crc32_somar(0, out + total, (size_t)r) || l32(h + k + 4) != (uint32_t)r) { libera(out); return COMP_ERRO_CRC; }
        total += r;
        pos += k + 8;
        membros++;
    }
    *saida = out;
    *tam = total;
    return 0;
}
