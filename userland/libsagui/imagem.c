// SPDX-License-Identifier: GPL-2.0-or-later
/* imagem.c - lectura y grabación de imágenes y escala (D054).
 *
 * Formatos leídos: BMP (1, 4, 8 bits con paleta; 24 y 32 bits; BITFIELDS de
 * 16/32 bits), PNM (P1 a P6, máximo hasta 65535) y PNG (todos los tipos de
 * color, profundidades 1 a 16, con y sin entrelazado Adam7). Grabados: BMP
 * de 24 bits y PPM P6.
 *
 * El PNG necesita un descompresor deflate (RFC 1951). El de aquí sigue el
 * modo más corto de escribir uno: cada código de Huffman se decodifica bit
 * a bit a partir de los recuentos por longitud (códigos canónicos), sin
 * tabla de búsqueda. Es más lento que el de zlib, pero cabe en ~150 líneas y es
 * rápido lo bastante para las imágenes de un visor (una imagen de 640x480 sale
 * en unas pocas decenas de ms en QEMU). Los CRC de los trozos y el Adler-32 no se
 * comprueban: un archivo corrupto suele fallar en el propio deflate o en los
 * filtros, y la imagen queda como esté.
 *
 * Solo enteros: libsagui se compila con -mgeneral-regs-only. La escala usa
 * punto fijo 16.16 y el bilineal pesa los cuatro vecinos con 8 bits. No hay
 * división de 64 bits (sin libgcc). */
#include "imagem.h"
#include "compressao.h"

/* ------------------------------------------------------------ básico */

bool imagem_criar(imagem_t *img, int larg, int alt, uint32_t cor)
{
    img->larg = img->alt = 0;
    img->px = NULL;
    img->formato[0] = 0;
    if (larg <= 0 || alt <= 0 || larg > IMAGEM_LADO_MAX || alt > IMAGEM_LADO_MAX ||
        larg * alt > IMAGEM_PIXELS_MAX) return false;
    img->px = aloca((size_t)larg * (size_t)alt * 4);
    if (!img->px) return false;
    img->larg = larg;
    img->alt = alt;
    preencher32(img->px, cor, (size_t)larg * (size_t)alt);
    return true;
}

void imagem_liberar(imagem_t *img)
{
    libera(img->px);
    img->px = NULL;
    img->larg = img->alt = 0;
}

bool imagem_copiar(imagem_t *dst, const imagem_t *src)
{
    if (!imagem_criar(dst, src->larg, src->alt, 0)) return false;
    memcpy(dst->px, src->px, (size_t)src->larg * (size_t)src->alt * 4);
    copia_texto(dst->formato, src->formato, sizeof dst->formato);
    return true;
}

const char *imagem_erro(int erro)
{
    switch (erro) {
    case IMG_ERRO_FORMATO:     return "formato desconocido (se leen BMP, PPM/PGM/PBM y PNG)";
    case IMG_ERRO_CORROMPIDO:  return "archivo dañado o incompleto";
    case IMG_ERRO_NAO_SUPORTA: return "variante del formato no soportada";
    case IMG_ERRO_GRANDE:      return "imagen demasiado grande";
    case IMG_ERRO_MEMORIA:     return "no hay memoria suficiente";
    default:                   return nome_erro(erro);
    }
}

static int minuscula(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }

bool imagem_extensao(const char *nome)
{
    static const char *const ext[] = { ".bmp", ".ppm", ".pgm", ".pbm", ".pnm", ".png", ".jpg", ".jpeg" };
    int n = (int)tam_texto(nome);
    for (int i = 0; i < 8; i++) {
        int le = (int)tam_texto(ext[i]);
        if (n <= le) continue;
        int k = 0;
        while (k < le && minuscula((uint8_t)nome[n - le + k]) == ext[i][k]) k++;
        if (k == le) return true;
    }
    return false;
}

static uint32_t le16(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8; }
static uint32_t le32(const uint8_t *p) { return le16(p) | le16(p + 2) << 16; }
static uint32_t be32(const uint8_t *p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }
static void poe16(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void poe32(uint8_t *p, uint32_t v) { poe16(p, v); poe16(p + 2, v >> 16); }

static uint32_t rgb(uint32_t r, uint32_t g, uint32_t b) { return r << 16 | g << 8 | b; }

/* Mezcla con fondo blanco (la ventana no tiene transparencia). */
static uint32_t sobre_branco(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
    if (a == 255) return rgb(r, g, b);
    uint32_t f = 255 - a;
    return rgb((r * a + 255 * f + 127) / 255, (g * a + 255 * f + 127) / 255, (b * a + 255 * f + 127) / 255);
}

static int alocar(imagem_t *img, int l, int a)
{
    if (l <= 0 || a <= 0) return IMG_ERRO_CORROMPIDO;
    if (l > IMAGEM_LADO_MAX || a > IMAGEM_LADO_MAX || l * a > IMAGEM_PIXELS_MAX) return IMG_ERRO_GRANDE;
    return imagem_criar(img, l, a, 0xFFFFFF) ? 0 : IMG_ERRO_MEMORIA;
}

/* ------------------------------------------------------------ BMP */

/* Una máscara de BITFIELDS: dónde empieza y cuántos bits tiene. */
static void mascara(uint32_t m, int *desloc, int *bits)
{
    *desloc = *bits = 0;
    if (!m) return;
    while (!(m & 1)) { m >>= 1; (*desloc)++; }
    while (m & 1) { m >>= 1; (*bits)++; }
}

static uint32_t canal(uint32_t v, int desloc, int bits)
{
    if (!bits) return 0;
    v = (v >> desloc) & ((1u << bits) - 1);
    if (bits >= 8) return v >> (bits - 8);
    return v * 255 / ((1u << bits) - 1);
}

static int ler_bmp(const uint8_t *d, int n, imagem_t *img)
{
    if (n < 26) return IMG_ERRO_CORROMPIDO;
    uint32_t ini = le32(d + 10), cab = le32(d + 14);
    int l, a, bpp, comp = 0, cores = 0;
    if (cab == 12) {   /* OS/2 1.x: campos de 16 bits, paleta de 3 bytes */
        l = (int)le16(d + 18); a = (int16_t)le16(d + 20); bpp = (int)le16(d + 24);
    } else if (cab >= 40 && n >= 54) {
        l = (int)le32(d + 18); a = (int)le32(d + 22); bpp = (int)le16(d + 28);
        comp = (int)le32(d + 30); cores = (int)le32(d + 46);
    } else return IMG_ERRO_NAO_SUPORTA;
    bool de_cima = a < 0;   /* altura negativa: las líneas vienen de arriba abajo */
    if (de_cima) a = -a;
    if (comp != 0 && comp != 3) return IMG_ERRO_NAO_SUPORTA;   /* RLE, JPEG/PNG incrustados */
    if (bpp != 1 && bpp != 4 && bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32) return IMG_ERRO_NAO_SUPORTA;
    if (comp == 3 && bpp != 16 && bpp != 32) return IMG_ERRO_NAO_SUPORTA;
    /* Máscaras: las de BITFIELDS (justo después de la cabecera de 40 bytes, o
     * dentro de la cabecera V4/V5), si no las predeterminadas. */
    uint32_t mr = 0xFF0000, mg = 0xFF00, mb = 0xFF;
    if (bpp == 16) { mr = 0x7C00; mg = 0x03E0; mb = 0x001F; }
    if (comp == 3) {
        if (n < 14 + 40 + 12) return IMG_ERRO_CORROMPIDO;
        mr = le32(d + 54); mg = le32(d + 58); mb = le32(d + 62);
    }
    int dr, br, dg, bg, db, bb;
    mascara(mr, &dr, &br); mascara(mg, &dg, &bg); mascara(mb, &db, &bb);
    /* Paleta: justo después de la cabecera (y de las máscaras, si las hay). */
    uint32_t paleta[256];
    int tam_ent = cab == 12 ? 3 : 4;
    if (bpp <= 8) {
        int qtd = cores > 0 && cores <= (1 << bpp) ? cores : 1 << bpp;
        uint32_t p = 14 + cab + (comp == 3 && cab == 40 ? 12 : 0);
        for (int i = 0; i < 256; i++) paleta[i] = 0;
        for (int i = 0; i < qtd; i++) {
            if (p + (uint32_t)(i * tam_ent) + 3 > (uint32_t)n) return IMG_ERRO_CORROMPIDO;
            const uint8_t *e = d + p + i * tam_ent;
            paleta[i] = rgb(e[2], e[1], e[0]);
        }
    }
    int r = alocar(img, l, a);
    if (r) return r;
    uint32_t passo = (uint32_t)(((l * bpp + 31) / 32) * 4);
    if (ini > (uint32_t)n) { imagem_liberar(img); return IMG_ERRO_CORROMPIDO; }
    for (int y = 0; y < a; y++) {
        uint32_t off = ini + (uint32_t)y * passo;
        uint32_t *dst = img->px + (de_cima ? y : a - 1 - y) * l;
        if (off + (uint32_t)((l * bpp + 7) / 8) > (uint32_t)n) {
            /* Archivo cortado: el resto queda blanco, pero avisa. */
            copia_texto(img->formato, "BMP (cortado)", sizeof img->formato);
            return 0;
        }
        const uint8_t *s = d + off;
        for (int x = 0; x < l; x++) {
            switch (bpp) {
            case 1:  dst[x] = paleta[(s[x >> 3] >> (7 - (x & 7))) & 1]; break;
            case 4:  dst[x] = paleta[(s[x >> 1] >> (x & 1 ? 0 : 4)) & 15]; break;
            case 8:  dst[x] = paleta[s[x]]; break;
            case 24: dst[x] = rgb(s[3 * x + 2], s[3 * x + 1], s[3 * x]); break;
            case 16: {
                uint32_t v = le16(s + 2 * x);
                dst[x] = rgb(canal(v, dr, br), canal(v, dg, bg), canal(v, db, bb));
                break;
            }
            default: {
                uint32_t v = le32(s + 4 * x);
                dst[x] = rgb(canal(v, dr, br), canal(v, dg, bg), canal(v, db, bb));
            }
            }
        }
    }
    formatar_texto(img->formato, sizeof img->formato, "BMP %d bits", bpp);
    return 0;
}

int imagem_gravar_bmp(const char *caminho, const imagem_t *img)
{
    int l = img->larg, a = img->alt;
    uint32_t passo = (uint32_t)((l * 3 + 3) & ~3);
    uint32_t tam = 54 + passo * (uint32_t)a;
    uint8_t *b = aloca_zerado(tam);
    if (!b) return IMG_ERRO_MEMORIA;
    b[0] = 'B'; b[1] = 'M';
    poe32(b + 2, tam);
    poe32(b + 10, 54);
    poe32(b + 14, 40);
    poe32(b + 18, (uint32_t)l);
    poe32(b + 22, (uint32_t)a);   /* positiva: de abajo arriba, como casi todos graban */
    poe16(b + 26, 1);
    poe16(b + 28, 24);
    poe32(b + 34, passo * (uint32_t)a);
    poe32(b + 38, 2835);          /* 72 puntos por pulgada */
    poe32(b + 42, 2835);
    for (int y = 0; y < a; y++) {
        uint8_t *d = b + 54 + (uint32_t)(a - 1 - y) * passo;
        const uint32_t *s = img->px + y * l;
        for (int x = 0; x < l; x++) {
            d[3 * x] = (uint8_t)s[x];
            d[3 * x + 1] = (uint8_t)(s[x] >> 8);
            d[3 * x + 2] = (uint8_t)(s[x] >> 16);
        }
    }
    int fd = abrir(caminho, ABRIR_ESCREVER | ABRIR_CRIAR | ABRIR_TRUNCAR);
    int r = fd;
    if (fd >= 0) {
        r = escrever_tudo(fd, b, tam) == (int)tam ? 0 : ERRO_LIMITE;
        fechar(fd);
    }
    libera(b);
    return r;
}

int imagem_gravar_ppm(const char *caminho, const imagem_t *img)
{
    char cab[32];
    int nc = formatar_texto(cab, sizeof cab, "P6\n%d %d\n255\n", img->larg, img->alt);
    uint32_t tam = (uint32_t)nc + (uint32_t)(img->larg * img->alt * 3);
    uint8_t *b = aloca(tam);
    if (!b) return IMG_ERRO_MEMORIA;
    memcpy(b, cab, (size_t)nc);
    uint8_t *d = b + nc;
    for (int i = 0; i < img->larg * img->alt; i++) {
        *d++ = (uint8_t)(img->px[i] >> 16);
        *d++ = (uint8_t)(img->px[i] >> 8);
        *d++ = (uint8_t)img->px[i];
    }
    int fd = abrir(caminho, ABRIR_ESCREVER | ABRIR_CRIAR | ABRIR_TRUNCAR);
    int r = fd;
    if (fd >= 0) {
        r = escrever_tudo(fd, b, tam) == (int)tam ? 0 : ERRO_LIMITE;
        fechar(fd);
    }
    libera(b);
    return r;
}

/* ------------------------------------------------------------ PNG (grabar, D125) */

static uint32_t png_crc(uint32_t crc, const uint8_t *p, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        crc ^= p[i];
        for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1)));
    }
    return crc;
}

static void png_be32(uint8_t *d, uint32_t v) { d[0] = (uint8_t)(v >> 24); d[1] = (uint8_t)(v >> 16); d[2] = (uint8_t)(v >> 8); d[3] = (uint8_t)v; }

static int png_trozo(int fd, const char *tipo, const uint8_t *dados, uint32_t n)
{
    uint8_t cab[8], cola[4];
    png_be32(cab, n);
    memcpy(cab + 4, tipo, 4);
    uint32_t crc = png_crc(0xFFFFFFFFu, (const uint8_t *)tipo, 4);
    crc = png_crc(crc, dados, n) ^ 0xFFFFFFFFu;
    png_be32(cola, crc);
    if (escrever_tudo(fd, cab, 8) != 8) return -1;
    if (n && escrever_tudo(fd, dados, n) != (int)n) return -1;
    return escrever_tudo(fd, cola, 4) == 4 ? 0 : -1;
}

/* ---- compresor deflate propio (D125): LZ77 con tabla hash y cadenas cortas, Huffman fijo.
 * No hay otro compresor en el sistema (la mejora 056 sigue en propuesta). */
typedef struct { uint8_t *d; uint32_t n, max, acc; int nb; bool erro; } sbits_t;

static void bits_poe(sbits_t *b, uint32_t v, int n)   /* LSB primero */
{
    b->acc |= v << b->nb;
    b->nb += n;
    while (b->nb >= 8) {
        if (b->n < b->max) b->d[b->n++] = (uint8_t)b->acc; else b->erro = true;
        b->acc >>= 8; b->nb -= 8;
    }
}

static void bits_huff(sbits_t *b, uint32_t cod, int n)   /* los códigos Huffman van con el bit alto primero */
{
    uint32_t r = 0;
    for (int i = 0; i < n; i++) r |= ((cod >> i) & 1u) << (n - 1 - i);
    bits_poe(b, r, n);
}

static void fijo_literal(sbits_t *b, int v)
{
    if (v < 144) bits_huff(b, 0x30u + (uint32_t)v, 8);
    else if (v < 256) bits_huff(b, 0x190u + (uint32_t)(v - 144), 9);
    else if (v < 280) bits_huff(b, (uint32_t)(v - 256), 7);
    else bits_huff(b, 0xC0u + (uint32_t)(v - 280), 8);
}

static const uint16_t len_base[29] = { 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258 };
static const uint8_t  len_ext[29]  = { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0 };
static const uint16_t dis_base[30] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577 };
static const uint8_t  dis_ext[30]  = { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13 };

#define DEF_JANELA 32768
#define DEF_HASH   (1 << 15)
#define DEF_CADEIA 24

/* Comprime ent[0..n) en un flujo deflate crudo; devuelve los bytes o <0. */
static int deflar_png(const uint8_t *ent, uint32_t n, uint8_t *saida, uint32_t max)
{
    int32_t *head = aloca(sizeof(int32_t) * (DEF_HASH + DEF_JANELA));
    if (!head) return IMG_ERRO_MEMORIA;
    int32_t *prev = head + DEF_HASH;
    for (int i = 0; i < DEF_HASH; i++) head[i] = -1;
    sbits_t b = { saida, 0, max, 0, 0, false };
    bits_poe(&b, 1, 1);   /* último bloque */
    bits_poe(&b, 1, 2);   /* Huffman fijo */
    uint32_t i = 0;
    while (i < n) {
        int mejor = 0; uint32_t mdis = 0;
        if (i + 3 <= n) {
            uint32_t h = (((uint32_t)ent[i] << 10) ^ ((uint32_t)ent[i + 1] << 5) ^ ent[i + 2]) & (DEF_HASH - 1);
            int32_t c = head[h];
            uint32_t maxl = n - i < 258 ? n - i : 258;
            for (int k = 0; k < DEF_CADEIA && c >= 0 && i - (uint32_t)c <= DEF_JANELA; k++) {
                uint32_t l = 0;
                while (l < maxl && ent[(uint32_t)c + l] == ent[i + l]) l++;
                if ((int)l > mejor) { mejor = (int)l; mdis = i - (uint32_t)c; if (l == maxl) break; }
                c = prev[(uint32_t)c & (DEF_JANELA - 1)];
            }
        }
        uint32_t avanza = 1;
        if (mejor >= 3) {
            int lc = 28;
            while (len_base[lc] > mejor) lc--;
            fijo_literal(&b, 257 + lc);
            if (len_ext[lc]) bits_poe(&b, (uint32_t)mejor - len_base[lc], len_ext[lc]);
            int dc = 29;
            while (dis_base[dc] > mdis) dc--;
            bits_huff(&b, (uint32_t)dc, 5);
            if (dis_ext[dc]) bits_poe(&b, mdis - dis_base[dc], dis_ext[dc]);
            avanza = (uint32_t)mejor;
        } else fijo_literal(&b, ent[i]);
        for (uint32_t k = 0; k < avanza; k++, i++) {   /* registra las posiciones en la tabla */
            if (i + 3 > n) continue;
            uint32_t h = (((uint32_t)ent[i] << 10) ^ ((uint32_t)ent[i + 1] << 5) ^ ent[i + 2]) & (DEF_HASH - 1);
            prev[i & (DEF_JANELA - 1)] = head[h];
            head[h] = (int32_t)i;
        }
    }
    fijo_literal(&b, 256);
    if (b.nb) bits_poe(&b, 0, 8 - b.nb);
    libera(head);
    return b.erro ? ERRO_LIMITE : (int)b.n;
}

/* PNG RGB de 8 bits: filtro por línea (ninguno, Sub o Up, el de menor suma de
 * valores absolutos) y deflate propio. Una captura del escritorio queda en
 * decenas de KB. */
int imagem_gravar_png(const char *caminho, const imagem_t *img)
{
    uint32_t lin = 1 + (uint32_t)img->larg * 3, n = lin * (uint32_t)img->alt;
    uint8_t *f = aloca(n);
    uint8_t *prev_l = aloca((uint32_t)img->larg * 3 + 3);   /* la línea anterior, sin filtrar */
    uint8_t *cur = aloca((uint32_t)img->larg * 3 + 3);
    uint32_t zmax = n + n / 8 + 1024;
    uint8_t *z = aloca(zmax + 6);
    if (!f || !prev_l || !cur || !z) { libera(f); libera(prev_l); libera(cur); libera(z); return IMG_ERRO_MEMORIA; }
    uint32_t w = (uint32_t)img->larg * 3;
    memset(prev_l, 0, w);
    for (int y = 0; y < img->alt; y++) {
        for (int x = 0; x < img->larg; x++) {
            uint32_t px = img->px[y * img->larg + x];
            cur[x * 3] = (uint8_t)(px >> 16); cur[x * 3 + 1] = (uint8_t)(px >> 8); cur[x * 3 + 2] = (uint8_t)px;
        }
        uint32_t s0 = 0, s1 = 0, s2 = 0;
        for (uint32_t i = 0; i < w; i++) {
            int8_t v0 = (int8_t)cur[i];
            int8_t v1 = (int8_t)(uint8_t)(cur[i] - (i >= 3 ? cur[i - 3] : 0));
            int8_t v2 = (int8_t)(uint8_t)(cur[i] - prev_l[i]);
            s0 += (uint32_t)(v0 < 0 ? -v0 : v0); s1 += (uint32_t)(v1 < 0 ? -v1 : v1); s2 += (uint32_t)(v2 < 0 ? -v2 : v2);
        }
        uint8_t *o = f + (uint32_t)y * lin;
        int t = (s1 < s0 && s1 <= s2) ? 1 : (s2 < s0 ? 2 : 0);
        *o++ = (uint8_t)t;
        for (uint32_t i = 0; i < w; i++)
            o[i] = t == 0 ? cur[i] : t == 1 ? (uint8_t)(cur[i] - (i >= 3 ? cur[i - 3] : 0)) : (uint8_t)(cur[i] - prev_l[i]);
        memcpy(prev_l, cur, w);
    }
    z[0] = 0x78; z[1] = 0x01;
    int zn = deflar_png(f, n, z + 2, zmax);
    int r = zn;
    if (zn >= 0) {
        uint32_t a = 1, b = 0;
        for (uint32_t i = 0; i < n; i++) { a += f[i]; if (a >= 65521) a -= 65521; b += a; if (b >= 65521) b -= 65521; }
        uint8_t *o = z + 2 + zn;
        png_be32(o, (b << 16) | a);
        uint32_t ztam = (uint32_t)zn + 6;
        int fd = abrir(caminho, ABRIR_ESCREVER | ABRIR_CRIAR | ABRIR_TRUNCAR);
        r = fd;
        if (fd >= 0) {
            static const uint8_t firma[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
            uint8_t ihdr[13];
            png_be32(ihdr, (uint32_t)img->larg); png_be32(ihdr + 4, (uint32_t)img->alt);
            ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = ihdr[11] = ihdr[12] = 0;
            r = 0;
            if (escrever_tudo(fd, firma, 8) != 8 || png_trozo(fd, "IHDR", ihdr, 13) ||
                png_trozo(fd, "IDAT", z, ztam) || png_trozo(fd, "IEND", NULL, 0))
                r = ERRO_LIMITE;
            fechar(fd);
        }
    }
    libera(f); libera(prev_l); libera(cur); libera(z);
    return r;
}

/* ------------------------------------------------------------ PNM (PBM, PGM, PPM) */

/* Siguiente número de la cabecera (o de un P1-P3), saltando espacios y
 * comentarios "#...". -1 = se acabó el archivo. */
static int pnm_numero(const uint8_t *d, int n, int *p)
{
    for (;;) {
        while (*p < n && eh_espaco(d[*p])) (*p)++;
        if (*p < n && d[*p] == '#') { while (*p < n && d[*p] != '\n') (*p)++; continue; }
        break;
    }
    if (*p >= n || d[*p] < '0' || d[*p] > '9') return -1;
    int v = 0;
    while (*p < n && d[*p] >= '0' && d[*p] <= '9') {
        if (v < 100000000) v = v * 10 + (d[*p] - '0');
        (*p)++;
    }
    return v;
}

static int ler_pnm(const uint8_t *d, int n, imagem_t *img)
{
    int tipo = d[1] - '0', p = 2;
    int l = pnm_numero(d, n, &p), a = pnm_numero(d, n, &p);
    int max = tipo == 1 || tipo == 4 ? 1 : pnm_numero(d, n, &p);
    if (l <= 0 || a <= 0 || max <= 0 || max > 65535) return IMG_ERRO_CORROMPIDO;
    int r = alocar(img, l, a);
    if (r) return r;
    int canais = tipo == 3 || tipo == 6 ? 3 : 1;
    bool binario = tipo >= 4;
    if (binario) p++;   /* exactamente un espacio separa la cabecera de los datos */
    int bytes = max > 255 ? 2 : 1;
    int passo_pbm = (l + 7) / 8;
    for (int y = 0; y < a; y++)
        for (int x = 0; x < l; x++) {
            uint32_t v[3];
            for (int c = 0; c < canais; c++) {
                int s;
                if (!binario) s = pnm_numero(d, n, &p);
                else if (tipo == 4) {
                    int o = p + y * passo_pbm + x / 8;
                    s = o < n ? (d[o] >> (7 - (x & 7))) & 1 : -1;
                } else if (p + bytes > n) s = -1;
                else { s = bytes == 2 ? d[p] << 8 | d[p + 1] : d[p]; p += bytes; }
                if (s < 0) {   /* faltó dato: lo que ya llegó queda, el resto blanco */
                    formatar_texto(img->formato, sizeof img->formato, "P%d (cortado)", tipo);
                    return 0;
                }
                if (s > max) s = max;
                /* PBM: 1 = negro */
                v[c] = tipo == 1 || tipo == 4 ? (s ? 0 : 255) : (uint32_t)s * 255 / (uint32_t)max;
            }
            img->px[y * l + x] = canais == 3 ? rgb(v[0], v[1], v[2]) : rgb(v[0], v[0], v[0]);
        }
    static const char *const nome[] = { "", "PBM texto", "PGM texto", "PPM texto", "PBM", "PGM", "PPM" };
    copia_texto(img->formato, nome[tipo], sizeof img->formato);
    return 0;
}

/* O inflate (RFC 1951) mora em compressao.c, na libsagui (mejora 056, D103). */

/* ------------------------------------------------------------ PNG */

static int paeth(int a, int b, int c)
{
    int p = a + b - c;
    int pa = p > a ? p - a : a - p, pb = p > b ? p - b : b - p, pc = p > c ? p - c : c - p;
    if (pa <= pb && pa <= pc) return a;
    return pb <= pc ? b : c;
}

/* Deshace los filtros de una (sub)imagen de "a" líneas con "nb" bytes cada
 * (más el byte del filtro). bpp = bytes por píxel (mínimo 1). */
static bool desfiltrar(uint8_t *d, int nb, int a, int bpp)
{
    for (int y = 0; y < a; y++) {
        uint8_t *lin = d + y * (nb + 1) + 1, *ant = y ? lin - (nb + 1) : NULL;
        int f = lin[-1];
        for (int x = 0; x < nb; x++) {
            int esq = x >= bpp ? lin[x - bpp] : 0, cima = ant ? ant[x] : 0;
            int diag = ant && x >= bpp ? ant[x - bpp] : 0;
            switch (f) {
            case 0: break;
            case 1: lin[x] = (uint8_t)(lin[x] + esq); break;
            case 2: lin[x] = (uint8_t)(lin[x] + cima); break;
            case 3: lin[x] = (uint8_t)(lin[x] + ((esq + cima) >> 1)); break;
            case 4: lin[x] = (uint8_t)(lin[x] + paeth(esq, cima, diag)); break;
            default: return false;
            }
        }
    }
    return true;
}

typedef struct {
    int tipo, prof, canais;
    uint32_t paleta[256];
    uint8_t alfa[256];
} png_t;

/* Muestra "c" del píxel x de una línea ya desfiltrada, em 0..255. */
static uint32_t amostra(const png_t *p, const uint8_t *lin, int x, int c)
{
    int i = x * p->canais + c;
    switch (p->prof) {
    case 16: return lin[2 * i];
    case 8:  return lin[i];
    default: {
        int bit = i * p->prof;
        uint32_t v = (uint32_t)(lin[bit >> 3] >> (8 - p->prof - (bit & 7))) & ((1u << p->prof) - 1);
        return p->tipo == 3 ? v : v * 255 / ((1u << p->prof) - 1);   /* paleta: índice crudo */
    }
    }
}

static uint32_t pixel_png(const png_t *p, const uint8_t *lin, int x)
{
    switch (p->tipo) {
    case 0: { uint32_t g = amostra(p, lin, x, 0); return rgb(g, g, g); }
    case 2: return rgb(amostra(p, lin, x, 0), amostra(p, lin, x, 1), amostra(p, lin, x, 2));
    case 3: {
        uint32_t i = p->prof == 8 ? lin[x] : amostra(p, lin, x, 0);
        uint32_t c = p->paleta[i & 255];
        return sobre_branco(c >> 16, (c >> 8) & 255, c & 255, p->alfa[i & 255]);
    }
    case 4: { uint32_t g = amostra(p, lin, x, 0); return sobre_branco(g, g, g, amostra(p, lin, x, 1)); }
    default: return sobre_branco(amostra(p, lin, x, 0), amostra(p, lin, x, 1), amostra(p, lin, x, 2), amostra(p, lin, x, 3));
    }
}

static int ler_png(const uint8_t *d, int n, imagem_t *img)
{
    static png_t p;
    int l = 0, a = 0, entrel = 0, pos = 8, tam_dados = 0;
    memset(&p, 0, sizeof p);
    for (int i = 0; i < 256; i++) p.alfa[i] = 255;
    /* 1.ª pasada: cabecera, paleta y cuánto hay de IDAT. */
    while (pos + 12 <= n) {
        uint32_t len = be32(d + pos);
        const uint8_t *t = d + pos + 4, *c = d + pos + 8;
        if (len > (uint32_t)(n - pos - 12)) return IMG_ERRO_CORROMPIDO;
        if (!memcmp(t, "IHDR", 4) && len >= 13) {
            l = (int)be32(c); a = (int)be32(c + 4);
            p.prof = c[8]; p.tipo = c[9]; entrel = c[12];
            if (c[10] || c[11] || entrel > 1) return IMG_ERRO_NAO_SUPORTA;
        } else if (!memcmp(t, "PLTE", 4)) {
            for (uint32_t i = 0; i < len / 3 && i < 256; i++) p.paleta[i] = rgb(c[3 * i], c[3 * i + 1], c[3 * i + 2]);
        } else if (!memcmp(t, "tRNS", 4) && p.tipo == 3) {
            for (uint32_t i = 0; i < len && i < 256; i++) p.alfa[i] = c[i];
        } else if (!memcmp(t, "IDAT", 4)) tam_dados += (int)len;
        else if (!memcmp(t, "IEND", 4)) break;
        pos += 12 + (int)len;
    }
    static const uint8_t canais[7] = { 1, 0, 3, 1, 2, 0, 4 };
    if (p.tipo > 6 || !canais[p.tipo]) return IMG_ERRO_NAO_SUPORTA;
    p.canais = canais[p.tipo];
    int pr = p.prof;
    bool prof_ok = pr == 8 || (pr == 16 && p.tipo != 3) || ((pr == 1 || pr == 2 || pr == 4) && (p.tipo == 0 || p.tipo == 3));
    if (!prof_ok) return IMG_ERRO_NAO_SUPORTA;
    if (l <= 0 || a <= 0 || !tam_dados) return IMG_ERRO_CORROMPIDO;
    if (l > IMAGEM_LADO_MAX || a > IMAGEM_LADO_MAX || l * a > IMAGEM_PIXELS_MAX) return IMG_ERRO_GRANDE;
    /* Junta los IDAT (el flujo zlib puede estar dividido en varios). */
    uint8_t *z = aloca((size_t)tam_dados);
    if (!z) return IMG_ERRO_MEMORIA;
    int k = 0;
    for (pos = 8; pos + 12 <= n;) {
        uint32_t len = be32(d + pos);
        if (!memcmp(d + pos + 4, "IDAT", 4)) { memcpy(z + k, d + pos + 8, len); k += (int)len; }
        if (!memcmp(d + pos + 4, "IEND", 4)) break;
        pos += 12 + (int)len;
    }
    /* Los 7 pasos de Adam7 (sin entrelazado: un solo paso, la imagen entera). */
    static const uint8_t x0[7] = { 0, 4, 0, 2, 0, 1, 0 }, y0[7] = { 0, 0, 4, 0, 2, 0, 1 };
    static const uint8_t dx[7] = { 8, 8, 4, 4, 2, 2, 1 }, dy[7] = { 8, 8, 8, 4, 4, 2, 2 };
    int passos = entrel ? 7 : 1, bits_px = p.canais * pr, bpp = (bits_px + 7) / 8;
    int total = 0;
    for (int s = 0; s < passos; s++) {
        int pl = entrel ? (l - x0[s] + dx[s] - 1) / dx[s] : l, pa = entrel ? (a - y0[s] + dy[s] - 1) / dy[s] : a;
        if (pl > 0 && pa > 0) total += pa * ((pl * bits_px + 7) / 8 + 1);
    }
    uint8_t *cru = aloca((size_t)total);
    int r = cru ? 0 : IMG_ERRO_MEMORIA;
    /* zlib: 2 bytes de cabecera (método 8 = deflate, sin diccionario). */
    if (!r && (k < 2 || (z[0] & 15) != 8 || (z[1] & 0x20) || ((z[0] << 8) | z[1]) % 31)) r = IMG_ERRO_CORROMPIDO;
    if (!r && inflar(z + 2, k - 2, cru, total) != total) r = IMG_ERRO_CORROMPIDO;
    libera(z);
    if (!r) r = alocar(img, l, a);
    if (r) { libera(cru); return r; }
    uint8_t *q = cru;
    for (int s = 0; s < passos && !r; s++) {
        int pl = entrel ? (l - x0[s] + dx[s] - 1) / dx[s] : l, pa = entrel ? (a - y0[s] + dy[s] - 1) / dy[s] : a;
        if (pl <= 0 || pa <= 0) continue;
        int nb = (pl * bits_px + 7) / 8;
        if (!desfiltrar(q, nb, pa, bpp)) { r = IMG_ERRO_CORROMPIDO; break; }
        for (int y = 0; y < pa; y++) {
            const uint8_t *lin = q + y * (nb + 1) + 1;
            int yy = entrel ? y0[s] + y * dy[s] : y;
            for (int x = 0; x < pl; x++) {
                int xx = entrel ? x0[s] + x * dx[s] : x;
                img->px[yy * l + xx] = pixel_png(&p, lin, x);
            }
        }
        q += pa * (nb + 1);
    }
    libera(cru);
    if (r) { imagem_liberar(img); return r; }
    static const char *const nome_tipo[7] = { "gris", "", "RGB", "paleta", "gris+alfa", "", "RGBA" };
    formatar_texto(img->formato, sizeof img->formato, "PNG %s%s", nome_tipo[p.tipo], entrel ? " entr." : "");
    return 0;
}

/* ------------------------------------------------------------ entrada */

int imagem_decodificar(const uint8_t *d, int n, imagem_t *img)
{
    img->px = NULL;
    img->larg = img->alt = 0;
    img->formato[0] = 0;
    if (n >= 2 && d[0] == 'B' && d[1] == 'M') return ler_bmp(d, n, img);
    if (n >= 3 && d[0] == 'P' && d[1] >= '1' && d[1] <= '6' && eh_espaco(d[2])) return ler_pnm(d, n, img);
    if (n >= 8 && !memcmp(d, "\x89PNG\r\n\x1a\n", 8)) return ler_png(d, n, img);
    if (n >= 3 && d[0] == 0xFF && d[1] == 0xD8 && d[2] == 0xFF) return imagem_ler_jpeg(d, n, img);
    if (n >= 6 && !memcmp(d, "GIF8", 4)) return imagem_ler_gif(d, n, img);   /* D185 */
    return IMG_ERRO_FORMATO;
}

int imagem_ler(const char *caminho, imagem_t *img)
{
    img->px = NULL;
    img->larg = img->alt = 0;
    info_arquivo_t inf;
    int r = info_arquivo(caminho, &inf);
    if (r < 0) return r;
    if (inf.tipo == TIPO_PASTA) return ERRO_E_PASTA;
    if (inf.tamanho > 64u * 1024 * 1024) return IMG_ERRO_GRANDE;
    int fd = abrir(caminho, ABRIR_LER);
    if (fd < 0) return fd;
    int cap = (int)inf.tamanho + 1, n = 0;
    uint8_t *buf = aloca((size_t)cap);
    if (!buf) { fechar(fd); return IMG_ERRO_MEMORIA; }
    while (n < cap && (r = ler(fd, buf + n, (unsigned)(cap - n))) > 0) n += r;
    fechar(fd);
    r = imagem_decodificar(buf, n, img);
    libera(buf);
    return r;
}

/* ------------------------------------------------------------ escala */

void imagem_desenhar(uint32_t *dst, int larg_dst, int rx, int ry, int rl, int ra,
                     const imagem_t *img, int ox, int oy, int l, int a, bool suave)
{
    if (l <= 0 || a <= 0 || !img->px) return;
    /* recorte = intersección del rectángulo pedido con la imagen escalada */
    int x0 = rx > ox ? rx : ox, y0 = ry > oy ? ry : oy;
    int x1 = rx + rl < ox + l ? rx + rl : ox + l, y1 = ry + ra < oy + a ? ry + ra : oy + a;
    if (x0 >= x1 || y0 >= y1) return;
    int L = img->larg, A = img->alt;
    if (l == L && a == A) {   /* 1:1: copia de líneas */
        for (int y = y0; y < y1; y++)
            memcpy(dst + y * larg_dst + x0, img->px + (y - oy) * L + (x0 - ox), (size_t)(x1 - x0) * 4);
        return;
    }
    /* Paso en 16.16: cuántos píxeles de la imagen por píxel de la pantalla. Cabe en
     * 32 bits: (x - ox) * paso ≈ L << 16 <= 2^29. */
    int px = (L << 16) / l, py = (A << 16) / a;
    if (!suave) {
        for (int y = y0; y < y1; y++) {
            int sy = ((y - oy) * py + py / 2) >> 16;
            if (sy >= A) sy = A - 1;
            const uint32_t *s = img->px + sy * L;
            uint32_t *d = dst + y * larg_dst;
            int fx = (x0 - ox) * px + px / 2;
            for (int x = x0; x < x1; x++, fx += px) {
                int sx = fx >> 16;
                d[x] = s[sx < L ? sx : L - 1];
            }
        }
        return;
    }
    /* Bilineal: el centro del píxel de la pantalla cae entre cuatro de la imagen; los
     * pesos son las fracciones (8 bits) de la posición. */
    for (int y = y0; y < y1; y++) {
        int fy = (y - oy) * py + py / 2 - 0x8000;
        if (fy < 0) fy = 0;
        int sy = fy >> 16, wy = (fy >> 8) & 255;
        if (sy >= A - 1) { sy = A - 1; wy = 0; }
        const uint32_t *s0 = img->px + sy * L, *s1 = sy + 1 < A ? s0 + L : s0;
        uint32_t *d = dst + y * larg_dst;
        int fx = (x0 - ox) * px + px / 2 - 0x8000;
        for (int x = x0; x < x1; x++, fx += px) {
            int f = fx < 0 ? 0 : fx;
            int sx = f >> 16, wx = (f >> 8) & 255;
            if (sx >= L - 1) { sx = L - 1; wx = 0; }
            int sx1 = sx + 1 < L ? sx + 1 : sx;
            uint32_t a00 = s0[sx], a01 = s0[sx1], a10 = s1[sx], a11 = s1[sx1], c = 0;
            for (int k = 0; k <= 16; k += 8) {
                int c00 = (a00 >> k) & 255, c01 = (a01 >> k) & 255, c10 = (a10 >> k) & 255, c11 = (a11 >> k) & 255;
                int cima = c00 * 256 + (c01 - c00) * wx, baixo = c10 * 256 + (c11 - c10) * wx;
                int v = (cima * 256 + (baixo - cima) * wy + 32768) >> 16;
                c |= (uint32_t)(v < 0 ? 0 : v > 255 ? 255 : v) << k;
            }
            d[x] = c;
        }
    }
}

bool imagem_escalar(imagem_t *dst, const imagem_t *src, int l, int a, bool suave)
{
    if (!imagem_criar(dst, l, a, 0xFFFFFF)) return false;
    imagem_desenhar(dst->px, l, 0, 0, l, a, src, 0, 0, l, a, suave);
    copia_texto(dst->formato, src->formato, sizeof dst->formato);
    return true;
}
