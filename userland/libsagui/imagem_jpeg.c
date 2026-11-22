// SPDX-License-Identifier: GPL-2.0-or-later
/* imagem_jpeg.c - decodificador de JPEG (mejora 047, la parte de las fotos).
 *
 * Lee el JPEG *baseline* secuencial (Huffman), que es lo que hacen casi todas
 * las fotos y lo que guardan los moviles: YCbCr 4:4:4, 4:2:2 y 4:2:0, y en
 * escala de grises. El JPEG progresivo (SOF2) y el de 12 bits no se leen: se
 * avisa con IMG_ERRO_NAO_SUPORTA en vez de dar una imagen mal.
 *
 * Como esta hecho:
 *
 *  - Los datos comprimidos se leen con un lector de bits que deshace el
 *    "byte stuffing" (un 0xFF dentro de los datos siempre viene seguido de un
 *    0x00) y que se para en cuanto aparece un marcador.
 *  - Las tablas de Huffman se decodifican con el algoritmo canonico de la
 *    norma (ITU-T T.81, anexo F): con los recuentos por longitud se sacan los
 *    codigos minimo y maximo de cada longitud, y el simbolo sale con una
 *    comparacion por longitud. No hace falta tabla de busqueda.
 *  - La IDCT es entera y separable (una pasada por columnas y otra por
 *    filas), con el coseno en 11 bits. Nada de coma flotante: libsagui se
 *    compila con -mgeneral-regs-only.
 *
 * La escala esta pensada para que ningun intermedio se salga del int32: el
 * coseno va en 11 bits (S = 2048) y las sumas de la segunda pasada se hacen
 * termino a termino con un desplazamiento, de modo que el peor caso esta a
 * la mitad del limite.
 *
 * Los coeficientes AC se limitan a +-32767 (como hace el decodificador de
 * referencia): es el rango donde la IDCT entera no se desborda. El DC se
 * guarda aparte y se suma al final (su valor es DC/8 para todos los pixeles
 * del bloque), asi que un fondo muy claro no se ve recortado.
 *
 * El submuestreo se deshace con el vecino mas proximo: es lo que menos
 * codigo necesita, y la diferencia con el submuestreo "suave" de la
 * referencia se ve solo en los bordes de color. */
#include "imagem.h"

#define JPG_MAX_COMP   4
#define JPG_MAX_QUANT  4

/* ------------------------------------------------------------ huffman */

typedef struct {
    uint8_t bits[17];    /* quantos codigos de cada comprimento (1 a 16) */
    uint8_t vals[256];   /* os simbolos, na ordem do codigo */
    int32_t mincode[17];
    int32_t maxcode[17]; /* -1 se nao ha codigo desse comprimento */
    int32_t valptr[17];
} jpg_huff_t;

/* Prepara os codigos minimo e maximo de cada comprimento (la norma llama a
 * eso los punteros de valores y los codigos minimo y maximo). */
static void jpg_huff_preparar(jpg_huff_t *h)
{
    int32_t codigo = 0;
    int k = 0;
    for (int l = 1; l <= 16; l++) {
        h->valptr[l] = k;
        h->mincode[l] = codigo;
        k += h->bits[l];
        codigo += h->bits[l];
        h->maxcode[l] = h->bits[l] ? codigo - 1 : -1;
        codigo <<= 1;
    }
}

/* ------------------------------------------------------------ bits */

typedef struct {
    const uint8_t *d;
    int n;
    int pos;
    uint32_t buf;   /* os bits lidos, do mais antigo para o mais novo */
    int nbits;      /* bits validos em buf */
    bool fim;       /* deu de encontra um marcador */
} jpg_bits_t;

/* Um bit. Devolve false quando chega ao fim dos dados (ou aparece um
 * marcador, que para nos e o mesmo: nao ha mais nada que ler). */
static bool jpg_bit(jpg_bits_t *b)
{
    if (b->nbits == 0) {
        if (b->pos >= b->n) { b->fim = true; return false; }
        uint8_t v = b->d[b->pos++];
        if (v == 0xFF) {
            /* 0xFF 0x00 e um 0xFF de verdade; 0xFF seguido de outra coisa e
             * um marcador e acaba os dados. */
            if (b->pos < b->n && b->d[b->pos] == 0x00) b->pos++;
            else { b->fim = true; return false; }
        }
        b->buf = v;
        b->nbits = 8;
    }
    b->nbits--;
    return ((b->buf >> b->nbits) & 1) != 0;
}

/* n bits, o primeiro e o mais significativo. */
static int32_t jpg_bits(jpg_bits_t *b, int n)
{
    int32_t v = 0;
    for (int i = 0; i < n; i++) v = (v << 1) | (jpg_bit(b) ? 1 : 0);
    return v;
}

/* Um simbolo de Huffman. -1 se o codigo nao existe (arquivo corrompido). */
static int jpg_huff_simbolo(jpg_bits_t *b, const jpg_huff_t *h)
{
    int32_t codigo = 0;
    for (int l = 1; l <= 16; l++) {
        codigo = (codigo << 1) | (jpg_bit(b) ? 1 : 0);
        if (h->maxcode[l] >= 0 && codigo <= h->maxcode[l])
            return h->vals[h->valptr[l] + codigo - h->mincode[l]];
    }
    return -1;
}

/* Os "bits adicionais" da norma: um valor com sinal. */
static int32_t jpg_extender(jpg_bits_t *b, int s)
{
    if (s <= 0) return 0;
    int32_t v = jpg_bits(b, s);
    if (v < (1 << (s - 1))) v -= (1 << s) - 1;
    return v;
}


/* ------------------------------------------------------------ decodificador */

typedef struct {
    int id;          /* identificador do componente no arquivo */
    int h, v;        /* fatores de amostragem */
    int tq;          /* tabela de quantizacao */
    int tdc, tac;    /* tabelas de Huffman do componente */
    int32_t pred;    /* preditor do DC */
} jpg_comp_t;

typedef struct {
    jpg_bits_t bits;
    uint16_t qt[JPG_MAX_QUANT][64];   /* em ordem zigzag */
    jpg_huff_t hdc[4];
    jpg_huff_t hac[4];
    jpg_comp_t comp[JPG_MAX_COMP];
    int ordem[JPG_MAX_COMP];          /* ordem dos componentes no scan */
    int ncomp, nordem;
    int larg, alt;
    int maxh, maxv;
    int mcus_linha, mcus_col;
    int restart;      /* intervalo de reinicio em MCUs (0 = nenhum) */
    int adobe;        /* -1 = sin marcador APP14 de Adobe; si no, su "transform" */
} jpg_dec_t;

/* Um bloco: coeficientes en zigzag, desquantizados, en ordem natural. */
static bool jpg_bloco(jpg_dec_t *dec, jpg_comp_t *c, int32_t *coef, int32_t *dc)
{
    const uint16_t *q = dec->qt[c->tq];
    int s = jpg_huff_simbolo(&dec->bits, &dec->hdc[c->tdc]);
    if (s < 0) return false;
    c->pred += jpg_extender(&dec->bits, s);
    *dc = c->pred * (int32_t)q[0];
    /* El bloque empieza sin coeficientes: los que no salen en el bloque (los
     * que van detrás del fin) valen cero. Si no se limpian, se quedan los
     * del bloque anterior y aparecen rayas en la imagen (D129). */
    for (int i = 0; i < 64; i++) coef[i] = 0;

    int k = 1;
    while (k < 64) {
        int rs = jpg_huff_simbolo(&dec->bits, &dec->hac[c->tac]);
        if (rs < 0) return false;
        int tam = rs & 15, zeros = rs >> 4;
        if (tam == 0) {
            if (zeros != 15) break;         /* fim do bloco */
            k += 16;                        /* 16 zeros */
            continue;
        }
        k += zeros;
        if (k > 63) return false;
        int32_t v = jpg_extender(&dec->bits, tam) * (int32_t)q[k];
        if (v > 32767) v = 32767;
        else if (v < -32767) v = -32767;
        coef[imagem_zigzag[k]] = v;
        k++;
    }
    return true;
}

/* Procura o marcador de reinicio (RST0..RST7) e o consome. */
static bool jpg_esperar_restart(jpg_dec_t *dec)
{
    jpg_bits_t *b = &dec->bits;
    b->nbits = 0;
    while (b->pos + 1 < b->n) {
        if (b->d[b->pos] == 0xFF && b->d[b->pos + 1] >= 0xD0 && b->d[b->pos + 1] <= 0xD7) {
            b->pos += 2;
            return true;
        }
        b->pos++;
    }
    b->fim = true;
    return false;
}


/* Decodifica a parte comprimida e monta a imagen. */
static int jpg_decodificar(jpg_dec_t *dec, imagem_t *img)
{
    int32_t amostras[JPG_MAX_COMP][16 * 16];   /* amostras do MCU, por componente */
    int32_t coef[64];
    int n = 0;

    if (!imagem_criar(img, dec->larg, dec->alt, 0)) return IMG_ERRO_MEMORIA;

    for (int my = 0; my < dec->mcus_col; my++) {
        for (int mx = 0; mx < dec->mcus_linha; mx++) {
            /* Blocos de todos os componentes deste MCU. */
            for (int oi = 0; oi < dec->nordem; oi++) {
                jpg_comp_t *c = &dec->comp[dec->ordem[oi]];
                for (int by = 0; by < c->v; by++) {
                    for (int bx = 0; bx < c->h; bx++) {
                        int32_t dc = 0;
                        if (!jpg_bloco(dec, c, coef, &dc)) {
                            /* Datos rotos o cortados: si ya salio alguna
                             * fila de MCU, se da por buena la imagen (el
                             * resto queda en gris); si no, es un error. */
                            if (my == 0) {
                                imagem_liberar(img);
                                return IMG_ERRO_CORROMPIDO;
                            }
                            for (int i = my * dec->maxv * 8 * dec->larg; i < dec->larg * dec->alt; i++)
                                img->px[i] = 0x808080;
                            return 0;
                        }
                        int32_t *dst = amostras[oi] + (by * 8) * (c->h * 8) + bx * 8;
                        imagem_idct(coef, dst, c->h * 8);
                        /* O DC pesa o mesmo em todo o bloco, e o JPEG
                         * guarda as amostras com o nivel deslocado 128 (o
                         * que sobra para desfazer e somar 128 aqui). */
                        int32_t meio = dc / 8 + 128;
                        for (int y = 0; y < 8; y++) {
                            for (int x = 0; x < 8; x++) {
                                int32_t v = dst[y * (c->h * 8) + x] + meio;
                                if (v < 0) v = 0; else if (v > 255) v = 255;
                                dst[y * (c->h * 8) + x] = v;
                            }
                        }
                    }
                }
            }

            /* Escreve os pixels do MCU. */
            for (int py = 0; py < dec->maxv * 8; py++) {
                int y = my * dec->maxv * 8 + py;
                if (y >= dec->alt) break;
                for (int px = 0; px < dec->maxh * 8; px++) {
                    int x = mx * dec->maxh * 8 + px;
                    if (x >= dec->larg) break;
                    int32_t v[JPG_MAX_COMP] = { 0, 0, 0, 0 };
                    for (int oi = 0; oi < dec->nordem; oi++) {
                        jpg_comp_t *c = &dec->comp[dec->ordem[oi]];
                        /* O componente tem c->h x c->v amostras por bloco e o
                         * MCU sai em maxh*8 x maxv*8 pixels, entao se pega a
                         * amostra mais proxima. */
                        int sx = px * c->h / dec->maxh;
                        int sy = py * c->v / dec->maxv;
                        v[oi] = amostras[oi][sy * (c->h * 8) + sx];
                    }
                    uint32_t cor;
                    if (dec->nordem == 1) {
                        cor = (uint32_t)v[0] << 16 | (uint32_t)v[0] << 8 | (uint32_t)v[0];
                    } else if (dec->nordem == 4) {
                        /* CMYK (o YCCK): los de Adobe guardan los valores
                         * invertidos (255 = sin tinta), los demas no. */
                        int32_t c0 = v[0], c1 = v[1], c2 = v[2], k = v[3];
                        if (dec->adobe == 2) {
                            uint32_t rgb = imagem_ycbcr(c0, c1, c2);
                            c0 = (int32_t)(rgb >> 16) & 0xFF;
                            c1 = (int32_t)(rgb >> 8) & 0xFF;
                            c2 = (int32_t)rgb & 0xFF;
                        }
                        if (dec->adobe < 0) {
                            c0 = 255 - c0; c1 = 255 - c1; c2 = 255 - c2; k = 255 - k;
                        }
                        cor = (uint32_t)(c0 * k / 255) << 16 | (uint32_t)(c1 * k / 255) << 8 |
                              (uint32_t)(c2 * k / 255);
                    } else if (dec->adobe == 0) {
                        /* Adobe con transform 0: RGB tal cual. */
                        cor = (uint32_t)v[0] << 16 | (uint32_t)v[1] << 8 | (uint32_t)v[2];
                    } else {
                        cor = imagem_ycbcr(v[0], v[1], v[2]);
                    }
                    img->px[y * img->larg + x] = cor;
                }
            }

            n++;
            /* Tras el ultimo MCU no hay marcador de reinicio (viene el EOI). */
            if (dec->restart && n % dec->restart == 0 &&
                n < dec->mcus_linha * dec->mcus_col) {
                if (!jpg_esperar_restart(dec)) {
                    imagem_liberar(img);
                    return IMG_ERRO_CORROMPIDO;
                }
                for (int ci = 0; ci < dec->ncomp; ci++) dec->comp[ci].pred = 0;
            }
        }
    }
    return 0;
}

/* ------------------------------------------------------------ entrada */

int imagem_ler_jpeg(const uint8_t *d, int n, imagem_t *img)
{
    jpg_dec_t dec;
    int pos = 2;

    img->px = NULL;
    img->larg = img->alt = 0;
    img->formato[0] = 0;

    if (n < 4 || d[0] != 0xFF || d[1] != 0xD8) return IMG_ERRO_FORMATO;

    memset(&dec, 0, sizeof dec);
    dec.adobe = -1;
    dec.bits.d = d;
    dec.bits.n = n;

    while (pos + 1 < n) {
        if (d[pos] != 0xFF) { pos++; continue; }
        uint8_t marcador = d[pos + 1];
        pos += 2;
        if (marcador == 0xD8 || marcador == 0xD9) continue;   /* SOI, EOI */
        if (marcador >= 0xD0 && marcador <= 0xD7) continue;   /* RSTn */
        if (marcador == 0x01) continue;                       /* TEM */
        if (pos + 2 > n) break;
        int tam = ((int)d[pos] << 8) | d[pos + 1];
        if (tam < 2 || pos + tam > n) return IMG_ERRO_CORROMPIDO;
        const uint8_t *seg = d + pos + 2;
        int tamseg = tam - 2;

        switch (marcador) {
        case 0xC0:      /* SOF0: baseline sequencial */
        case 0xC1: {    /* SOF1: sequencial extendido (tambem e Huffman) */
            if (dec.ncomp) return IMG_ERRO_CORROMPIDO;
            if (tamseg < 6) return IMG_ERRO_CORROMPIDO;
            if (seg[0] != 8) return IMG_ERRO_NAO_SUPORTA;   /* so 8 bits */
            dec.alt = (seg[1] << 8) | seg[2];
            dec.larg = (seg[3] << 8) | seg[4];
            int nc = seg[5];
            if (nc < 1 || nc > JPG_MAX_COMP) return IMG_ERRO_NAO_SUPORTA;
            if (tamseg < 6 + nc * 3) return IMG_ERRO_CORROMPIDO;
            dec.ncomp = nc;
            dec.maxh = dec.maxv = 1;
            for (int i = 0; i < nc; i++) {
                dec.comp[i].id = seg[6 + i * 3];
                dec.comp[i].h = seg[7 + i * 3] >> 4;
                dec.comp[i].v = seg[7 + i * 3] & 15;
                dec.comp[i].tq = seg[8 + i * 3];
                if (dec.comp[i].h < 1 || dec.comp[i].h > 4 ||
                    dec.comp[i].v < 1 || dec.comp[i].v > 4) return IMG_ERRO_CORROMPIDO;
                if (dec.comp[i].tq >= JPG_MAX_QUANT) return IMG_ERRO_CORROMPIDO;
                if (dec.comp[i].h > dec.maxh) dec.maxh = dec.comp[i].h;
                if (dec.comp[i].v > dec.maxv) dec.maxv = dec.comp[i].v;
            }
            if (dec.larg <= 0 || dec.alt <= 0) return IMG_ERRO_CORROMPIDO;
            if (dec.larg > IMAGEM_LADO_MAX || dec.alt > IMAGEM_LADO_MAX) return IMG_ERRO_GRANDE;
            if (dec.larg * dec.alt > IMAGEM_PIXELS_MAX) return IMG_ERRO_GRANDE;
            dec.mcus_linha = (dec.larg + dec.maxh * 8 - 1) / (dec.maxh * 8);
            dec.mcus_col = (dec.alt + dec.maxv * 8 - 1) / (dec.maxv * 8);
            break;
        }
        case 0xC2:      /* SOF2: progresivo */
        case 0xC3: case 0xC5: case 0xC6: case 0xC7:
        case 0xC9: case 0xCA: case 0xCB:
        case 0xCD: case 0xCE: case 0xCF:
            return IMG_ERRO_NAO_SUPORTA;    /* ainda nao se le */
        case 0xC4: {    /* DHT: tabelas de Huffman */
            int p = 0;
            while (p + 17 <= tamseg) {
                int tc = seg[p] >> 4, th = seg[p] & 15;
                p++;
                if (th >= JPG_MAX_QUANT) return IMG_ERRO_CORROMPIDO;
                jpg_huff_t *h = tc ? &dec.hac[th] : &dec.hdc[th];
                int total = 0;
                for (int i = 1; i <= 16; i++) {
                    h->bits[i] = seg[p + i - 1];
                    total += h->bits[i];
                }
                p += 16;
                if (total > 256 || p + total > tamseg) return IMG_ERRO_CORROMPIDO;
                for (int i = 0; i < total; i++) h->vals[i] = seg[p + i];
                p += total;
                jpg_huff_preparar(h);
            }
            break;
        }
        case 0xDB: {    /* DQT: tabelas de quantizacao */
            int p = 0;
            while (p < tamseg) {
                int pq = seg[p] >> 4, tq = seg[p] & 15;
                p++;
                if (tq >= JPG_MAX_QUANT) return IMG_ERRO_CORROMPIDO;
                if (pq) {    /* 16 bits por coeficiente */
                    if (p + 128 > tamseg) return IMG_ERRO_CORROMPIDO;
                    for (int i = 0; i < 64; i++)
                        dec.qt[tq][i] = (uint16_t)((seg[p + i * 2] << 8) | seg[p + i * 2 + 1]);
                    p += 128;
                } else {
                    if (p + 64 > tamseg) return IMG_ERRO_CORROMPIDO;
                    for (int i = 0; i < 64; i++) dec.qt[tq][i] = seg[p + i];
                    p += 64;
                }
            }
            break;
        }
        case 0xDD:      /* DRI: intervalo de reinicio */
            if (tamseg < 2) return IMG_ERRO_CORROMPIDO;
            dec.restart = (seg[0] << 8) | seg[1];
            break;
        case 0xDA: {    /* SOS: comeca os dados comprimidos */
            if (!dec.ncomp) return IMG_ERRO_CORROMPIDO;
            if (tamseg < 1) return IMG_ERRO_CORROMPIDO;
            int ns = seg[0];
            if (ns < 1 || ns > JPG_MAX_COMP) return IMG_ERRO_NAO_SUPORTA;
            if (tamseg < 1 + ns * 2 + 3) return IMG_ERRO_CORROMPIDO;
            dec.nordem = 0;
            for (int i = 0; i < ns; i++) {
                int cs = seg[1 + i * 2], td = seg[2 + i * 2] >> 4, ta = seg[2 + i * 2] & 15;
                int achou = -1;
                for (int c = 0; c < dec.ncomp; c++)
                    if (dec.comp[c].id == cs) achou = c;
                if (achou < 0) return IMG_ERRO_CORROMPIDO;
                dec.comp[achou].tdc = td;
                dec.comp[achou].tac = ta;
                if (td >= JPG_MAX_QUANT || ta >= JPG_MAX_QUANT) return IMG_ERRO_CORROMPIDO;
                dec.ordem[dec.nordem++] = achou;
            }
            if (dec.nordem != dec.ncomp) return IMG_ERRO_NAO_SUPORTA;  /* scans separados */
            dec.bits.pos = pos + tam;
            int r = jpg_decodificar(&dec, img);
            if (r < 0) return r;
            /* O formato, para mostrar na janela do visor. */
            const char *sub = (dec.ncomp == 1) ? "grises"
                         : (dec.maxh * dec.maxv == 4) ? "4:4:4"
                         : (dec.maxh == 2 && dec.maxv == 1) ? "4:2:2" : "4:2:0";
            const char *partes[3] = { "JPEG ", sub, "" };
            int k = 0;
            for (int p = 0; p < 3; p++)
                for (const char *s = partes[p]; *s && k < 15; s++) img->formato[k++] = *s;
            img->formato[k] = 0;
            return 0;
        }
        case 0xEE:      /* APP14 de Adobe: dice como van los colores */
            if (tamseg >= 12 && memcmp(seg, "Adobe", 5) == 0) dec.adobe = seg[11];
            break;
        default:
            break;      /* APPn, COM, etc.: se ignoran */
        }
        pos += tam;
    }
    return IMG_ERRO_CORROMPIDO;
}
