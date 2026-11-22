// SPDX-License-Identifier: GPL-2.0-or-later
/* empacotar.c - tar, zip e utilidades de arquivos da mejora 056 (D103).
 * tar: ustar (POSIX) na escrita, com a extensão "@LongLink" do GNU para
 * nomes que não cabem; na leitura também aceita pax (cabeçalhos x e g) e
 * GNU (L). zip: APPNOTE da PKWARE, métodos 0 e 8, sem cifrado nem ZIP64. */
#include "compressao.h"

/* ------------------------------------------------------------ buffer crescente */

int comp_buf_somar(buf_t *b, const void *dados, int n)
{
    if (b->n + n > b->cap) {
        int nova = b->cap ? b->cap : 4096;
        while (nova < b->n + n) nova *= 2;
        uint8_t *t = realoca(b->d, (size_t)nova);
        if (!t) return COMP_ERRO_MEMORIA;
        b->d = t;
        b->cap = nova;
    }
    if (n) memcpy(b->d + b->n, dados, (size_t)n);
    b->n += n;
    return 0;
}

void comp_buf_liberar(buf_t *b)
{
    if (b->d) libera(b->d);
    b->d = NULL;
    b->n = b->cap = 0;
}

/* ------------------------------------------------------------ arquivos e nomes */

uint8_t *carregar_descritor(int fd, int *n)
{
    int cap = 65536, t = 0;
    uint8_t *d = aloca((size_t)cap + 1);
    if (!d) return NULL;
    for (;;) {
        if (t == cap) {
            if ((uint32_t)cap >= COMP_LIMITE_MAX) { libera(d); return NULL; }
            uint8_t *x = realoca(d, (size_t)cap * 2 + 1);
            if (!x) { libera(d); return NULL; }
            d = x; cap *= 2;
        }
        int r = ler(fd, d + t, (unsigned)(cap - t) > 65536u ? 65536u : (unsigned)(cap - t));
        if (r < 0) { libera(d); return NULL; }
        if (r == 0) break;
        t += r;
    }
    *n = t;
    return d;
}

uint8_t *carregar_arquivo(const char *caminho, int *n)
{
    int fd = abrir(caminho, ABRIR_LER);
    if (fd < 0) return NULL;
    uint8_t *d = carregar_descritor(fd, n);
    fechar(fd);
    return d;
}

int salvar_arquivo(const char *caminho, const void *dados, int n)
{
    int fd = abrir(caminho, ABRIR_ESCREVER | ABRIR_CRIAR | ABRIR_TRUNCAR);
    if (fd < 0) return fd;
    int r = n ? escrever_tudo(fd, dados, (unsigned)n) : 0;
    fechar(fd);
    return r < 0 ? r : 0;
}

int criar_pasta_p(const char *caminho)
{
    char t[512];
    size_t n = tam_texto(caminho);
    if (n >= sizeof t) return ERRO_INVALIDO;
    copia_texto(t, caminho, sizeof t);
    int r = 0;
    for (size_t i = 1; i <= n; i++) {
        if (t[i] != '/' && t[i] != 0) continue;
        char c = t[i];
        t[i] = 0;
        if (tam_texto(t) && compara_texto(t, ".") != 0) {
            r = criar_pasta(t);
            if (r == ERRO_JA_EXISTE) r = 0;
            if (r < 0) return r;
        }
        t[i] = c;
    }
    return 0;
}

bool caminho_seguro(const char *nome, const char *destino, char *saida, int max)
{
    char rel[512];
    int k = 0;
    const char *p = nome;
    while (*p) {
        while (*p == '/') p++;
        const char *ini = p;
        while (*p && *p != '/') p++;
        int len = (int)(p - ini);
        if (!len || (len == 1 && ini[0] == '.')) continue;
        if (len == 2 && ini[0] == '.' && ini[1] == '.') return false;   /* sai do destino */
        if (k + len + 2 >= (int)sizeof rel) return false;
        if (k) rel[k++] = '/';
        memcpy(rel + k, ini, (size_t)len);
        k += len;
    }
    if (!k) return false;
    rel[k] = 0;
    size_t d = destino ? tam_texto(destino) : 0;
    if (!d || compara_texto(destino, ".") == 0) {
        if (k >= max) return false;
        copia_texto(saida, rel, (size_t)max);
        return true;
    }
    if (d + (size_t)k + 2 >= (size_t)max) return false;
    copia_texto(saida, destino, (size_t)max);
    if (destino[d - 1] != '/') junta_texto(saida, "/", (size_t)max);
    junta_texto(saida, rel, (size_t)max);
    return true;
}

bool eh_ascii(const char *s)
{
    for (; *s; s++) if ((uint8_t)*s >= 0x80) return false;
    return true;
}

int nome_para_utf8(const char *src, char *dst, int max)
{
    int k = 0;
    for (const uint8_t *p = (const uint8_t *)src; *p; p++) {
        if (*p < 0x80) { if (k + 1 >= max) return -1; dst[k++] = (char)*p; }
        else { if (k + 2 >= max) return -1; dst[k++] = (char)(0xC0 | *p >> 6); dst[k++] = (char)(0x80 | (*p & 0x3F)); }
    }
    dst[k] = 0;
    return k;
}

int nome_de_utf8(const char *src, char *dst, int max)
{
    const uint8_t *p = (const uint8_t *)src;
    /* só converte se tudo for UTF-8 válido; senão assume que já é Latin-1 */
    bool valido = true;
    for (const uint8_t *q = p; *q && valido;) {
        if (*q < 0x80) q++;
        else if ((*q & 0xE0) == 0xC0 && (q[1] & 0xC0) == 0x80 && *q >= 0xC2) q += 2;
        else if ((*q & 0xF0) == 0xE0 && (q[1] & 0xC0) == 0x80 && (q[2] & 0xC0) == 0x80) q += 3;
        else if ((*q & 0xF8) == 0xF0 && (q[1] & 0xC0) == 0x80 && (q[2] & 0xC0) == 0x80 && (q[3] & 0xC0) == 0x80) q += 4;
        else valido = false;
    }
    int k = 0;
    while (*p) {
        if (k + 1 >= max) return -1;
        if (!valido || *p < 0x80) dst[k++] = (char)*p++;
        else if ((*p & 0xE0) == 0xC0) { unsigned c = (unsigned)(*p & 0x1F) << 6 | (p[1] & 0x3F); dst[k++] = c < 0x100 ? (char)c : '?'; p += 2; }
        else { dst[k++] = '?'; p += (*p & 0xF0) == 0xE0 ? 3 : 4; }
    }
    dst[k] = 0;
    return k;
}

int percorrer(const char *real, const char *nome, bool recursivo, percorrer_cb_t cb, void *ctx)
{
    static int fundo;
    info_arquivo_t inf;
    int r = info_arquivo(real, &inf);
    if (r < 0) return r;
    r = cb(real, nome, inf.tipo, inf.tamanho, ctx);
    if (r < 0 || inf.tipo != TIPO_PASTA || !recursivo) return r < 0 ? r : 0;
    if (fundo > 64) return ERRO_LIMITE;
    fundo++;
    size_t lr = tam_texto(real), ln = tam_texto(nome);
    char *cr = aloca(lr + NOME_MAX + 2), *cn = aloca(ln + NOME_MAX + 2);
    entrada_pasta_t *e = aloca(sizeof *e);
    r = (cr && cn && e) ? 0 : COMP_ERRO_MEMORIA;
    for (int i = 0; r >= 0; i++) {
        int q = listar(real, i, e);
        if (q < 0) { r = q; break; }
        if (q == 0) break;
        if (compara_texto(e->nome, ".") == 0 || compara_texto(e->nome, "..") == 0) continue;
        copia_texto(cr, real, lr + 2);
        if (lr && real[lr - 1] != '/') junta_texto(cr, "/", lr + NOME_MAX + 2);
        junta_texto(cr, e->nome, lr + NOME_MAX + 2);
        copia_texto(cn, nome, ln + 2);
        if (ln && nome[ln - 1] != '/') junta_texto(cn, "/", ln + NOME_MAX + 2);
        junta_texto(cn, e->nome, ln + NOME_MAX + 2);
        r = percorrer(cr, cn, true, cb, ctx);
    }
    if (cr) libera(cr);
    if (cn) libera(cn);
    if (e) libera(e);
    fundo--;
    return r < 0 ? r : 0;
}

void comp_data_curta(uint32_t utc, char *s, int max)
{
    fuso_data_t d;
    hora_local(utc, &d);
    formatar_data(s, max, "%Y-%m-%d %H:%M", &d);
}

uint32_t comp_mtime(const char *caminho)
{
    datas_arquivo_t d;
    if (datas_arquivo(caminho, &d) == 0 && d.modificado) return d.modificado;
    return hora_utc(NULL);
}

/* ------------------------------------------------------------ tar */

static void oct(char *dst, int len, uint32_t v)
{
    for (int i = len - 2; i >= 0; i--) { dst[i] = (char)('0' + (v & 7)); v >>= 3; }
    dst[len - 1] = 0;
}

static void tar_bloco(buf_t *b, const char *nome, const char *prefixo, uint32_t modo, uint32_t tam,
                      uint32_t mtime, char tipo, int *erro)
{
    uint8_t h[512];
    memset(h, 0, sizeof h);
    copia_texto((char *)h, nome, 100 + 1 > (int)tam_texto(nome) + 1 ? tam_texto(nome) + 1 : 100);
    if (tam_texto(nome) >= 100) memcpy(h, nome, 100);   /* sem terminador se ocupar os 100 */
    oct((char *)h + 100, 8, modo & 07777);
    oct((char *)h + 108, 8, 0);
    oct((char *)h + 116, 8, 0);
    oct((char *)h + 124, 12, tam);
    oct((char *)h + 136, 12, mtime);
    memset(h + 148, ' ', 8);
    h[156] = (uint8_t)tipo;
    memcpy(h + 257, "ustar\0" "00", 8);
    memcpy(h + 265, "sagui", 6);
    memcpy(h + 297, "sagui", 6);
    if (prefixo) copia_texto((char *)h + 345, prefixo, 156);
    uint32_t soma = 0;
    for (int i = 0; i < 512; i++) soma += h[i];
    oct((char *)h + 148, 7, soma);
    h[155] = ' ';
    int r = comp_buf_somar(b, h, 512);
    if (r < 0) *erro = r;
}

int tar_adicionar(buf_t *b, const char *nome, const uint8_t *dados, uint32_t n, uint32_t mtime,
                  uint32_t modo, bool pasta)
{
    char nm[1024];
    int erro = 0;
    size_t len = tam_texto(nome);
    if (len + 2 >= sizeof nm) return COMP_ERRO_GRANDE;
    copia_texto(nm, nome, sizeof nm);
    if (pasta && (!len || nm[len - 1] != '/')) { nm[len++] = '/'; nm[len] = 0; }
    if (pasta) n = 0;
    const char *curto = nm, *prefixo = NULL;
    char pre[160];
    if (len > 100) {
        /* ustar: parte no último '/' que deixe o resto em 100 e o prefixo em 155 */
        int corte = -1;
        for (int i = 0; i < (int)len - 1; i++)
            if (nm[i] == '/' && i <= 155 && (int)len - i - 1 <= 100) corte = i;
        if (corte > 0) {
            memcpy(pre, nm, (size_t)corte);
            pre[corte] = 0;
            prefixo = pre;
            curto = nm + corte + 1;
        } else {   /* extensão do GNU: entrada L com o nome inteiro */
            tar_bloco(b, "././@LongLink", NULL, 0, (uint32_t)len + 1, 0, 'L', &erro);
            int r = comp_buf_somar(b, nm, (int)len + 1);
            if (r < 0) return r;
            static const uint8_t zeros[512];
            int resto = 512 - ((int)len + 1) % 512;
            if (resto < 512) r = comp_buf_somar(b, zeros, resto);
            if (r < 0) return r;
            curto = nm;   /* o cabeçalho leva os 100 primeiros */
        }
    }
    tar_bloco(b, curto, prefixo, modo ? modo : (pasta ? 0755 : 0644), n, mtime, pasta ? '5' : '0', &erro);
    if (erro) return erro;
    if (n) {
        int r = comp_buf_somar(b, dados, (int)n);
        if (r < 0) return r;
        static const uint8_t zeros[512];
        int resto = 512 - (int)(n % 512);
        if (resto < 512) r = comp_buf_somar(b, zeros, resto);
        if (r < 0) return r;
    }
    return 0;
}

int tar_concluir(buf_t *b)
{
    static const uint8_t zeros[1024];
    return comp_buf_somar(b, zeros, 1024);
}

static int64_t numero_tar(const uint8_t *p, int len)
{
    if (p[0] & 0x80) {   /* base 256 do GNU */
        int64_t v = p[0] & 0x7F;
        for (int i = 1; i < len; i++) { if (v > 0x7FFFFFFF) return -1; v = v << 8 | p[i]; }
        return v;
    }
    int64_t v = 0;
    int i = 0;
    while (i < len && (p[i] == ' ' || p[i] == 0)) i++;
    for (; i < len && p[i] >= '0' && p[i] <= '7'; i++) v = v * 8 + (p[i] - '0');
    return v;
}

int tar_proximo(const uint8_t *buf, int n, int *pos, tar_entrada_t *e)
{
    char longo[1024];
    bool tem_longo = false;
    uint32_t tam_pax = 0;
    bool tem_tam_pax = false;
    for (;;) {
        int p = *pos;
        if (p + 512 > n) {
            if (p >= n) return 0;   /* acabou sem os blocos de zeros: tar de outras ferramentas */
            return COMP_ERRO_TRUNCADO;
        }
        const uint8_t *h = buf + p;
        bool zeros = true;
        for (int i = 0; i < 512 && zeros; i++) if (h[i]) zeros = false;
        if (zeros) return 0;
        uint32_t soma = 0;
        for (int i = 0; i < 512; i++) soma += (i >= 148 && i < 156) ? ' ' : h[i];
        if ((int64_t)soma != numero_tar(h + 148, 8)) return COMP_ERRO_FORMATO;
        int64_t tam = numero_tar(h + 124, 12);
        if (tam < 0 || tam > 0x7FFFFFFF) return COMP_ERRO_FORMATO;
        char tipo = (char)h[156];
        int dados = p + 512;
        int64_t ocupa = (tam + 511) / 512 * 512;
        if ((int64_t)dados + tam > n) return COMP_ERRO_TRUNCADO;
        *pos = (int)(dados + (dados + ocupa > n ? n - dados : ocupa));
        if (tipo == 'L') {
            if (tam >= (int64_t)sizeof longo) return COMP_ERRO_FORMATO;
            memcpy(longo, buf + dados, (size_t)tam);
            longo[tam] = 0;
            tem_longo = true;
            continue;
        }
        if (tipo == 'x' || tipo == 'g') {
            if (tipo == 'g') continue;
            const uint8_t *q = buf + dados, *fim = q + tam;
            while (q < fim) {   /* registros "tamanho chave=valor\n" */
                int len = 0;
                const uint8_t *ini = q;
                while (q < fim && *q >= '0' && *q <= '9') len = len * 10 + (*q++ - '0');
                if (q >= fim || *q != ' ' || len <= 0 || ini + len > fim) break;
                q++;
                const uint8_t *fr = ini + len - 1;   /* o '\n' */
                if (fr - q > 5 && !memcmp(q, "path=", 5)) {
                    int l = (int)(fr - q - 5);
                    if (l >= (int)sizeof longo) return COMP_ERRO_FORMATO;
                    memcpy(longo, q + 5, (size_t)l);
                    longo[l] = 0;
                    tem_longo = true;
                } else if (fr - q > 5 && !memcmp(q, "size=", 5)) {
                    uint32_t v = 0;
                    for (const uint8_t *d = q + 5; d < fr && *d >= '0' && *d <= '9'; d++) v = v * 10 + (uint32_t)(*d - '0');
                    tam_pax = v;
                    tem_tam_pax = true;
                }
                q = ini + len;
            }
            continue;
        }
        if (tem_tam_pax && tam_pax != (uint32_t)tam) {   /* tamanho grande declarado no pax: refaz o avanço */
            tam = tam_pax;
            if ((int64_t)dados + tam > n) return COMP_ERRO_TRUNCADO;
            *pos = (int)(dados + (tam + 511) / 512 * 512);
            if (*pos > n) *pos = n;
        }
        /* entrada de verdade */
        if (tem_longo) copia_texto(e->nome, longo, sizeof e->nome);
        else {
            char nm[101], pre[156];
            memcpy(nm, h, 100); nm[100] = 0;
            e->nome[0] = 0;
            if (!memcmp(h + 257, "ustar", 6) && h[345]) {
                memcpy(pre, h + 345, 155); pre[155] = 0;
                copia_texto(e->nome, pre, sizeof e->nome);
                junta_texto(e->nome, "/", sizeof e->nome);
            }
            junta_texto(e->nome, nm, sizeof e->nome);
        }
        if (tipo == 0 || tipo == '7') tipo = '0';
        size_t l = tam_texto(e->nome);
        if (tipo == '0' && l && e->nome[l - 1] == '/') tipo = '5';
        e->tipo = tipo;
        e->tamanho = (uint32_t)tam;
        e->mtime = (uint32_t)numero_tar(h + 136, 12);
        e->modo = (uint32_t)numero_tar(h + 100, 8);
        e->dados = buf + dados;
        return 1;
    }
}

/* ------------------------------------------------------------ zip */

static void p16(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void p32(uint8_t *p, uint32_t v) { p16(p, v); p16(p + 2, v >> 16); }
static uint32_t l16(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8; }
static uint32_t l32(const uint8_t *p) { return l16(p) | l16(p + 2) << 16; }

static void dos_data(uint32_t utc, uint16_t *hora, uint16_t *data)
{
    fuso_data_t d;
    if (!utc) { *hora = 0; *data = 0x21; return; }
    hora_local(utc, &d);
    if (d.ano < 1980) { *hora = 0; *data = 0x21; return; }
    *hora = (uint16_t)(d.hora << 11 | d.minuto << 5 | d.segundo / 2);
    *data = (uint16_t)((d.ano - 1980) << 9 | d.mes << 5 | d.dia);
}

int zip_adicionar(zip_escritor_t *z, const char *nome, const uint8_t *dados, uint32_t n, uint32_t mtime,
                  int nivel, bool pasta, uint32_t modo)
{
    if (z->n >= 65535) return COMP_ERRO_NAO_SUPORTADO;
    if (z->n == z->cap) {
        int nc = z->cap ? z->cap * 2 : 64;
        zip_ent_t *t = realoca(z->e, sizeof(zip_ent_t) * (size_t)nc);
        if (!t) return COMP_ERRO_MEMORIA;
        z->e = t; z->cap = nc;
    }
    size_t len = tam_texto(nome);
    if (len > 60000) return COMP_ERRO_GRANDE;
    char *nm = aloca(len + 2);
    if (!nm) return COMP_ERRO_MEMORIA;
    copia_texto(nm, nome, len + 2);
    if (pasta && (!len || nm[len - 1] != '/')) { nm[len++] = '/'; nm[len] = 0; }
    if (pasta) n = 0;
    zip_ent_t *e = &z->e[z->n];
    memset(e, 0, sizeof *e);
    e->nome = nm;
    e->pasta = pasta;
    e->tu = n;
    e->crc = pasta ? 0 : crc32_somar(0, dados, n);
    e->flags = eh_ascii(nm) ? 0 : 0x800;
    dos_data(mtime, &e->hora, &e->data);
    e->attr = ((pasta ? 040000u | (modo ? modo : 0755) : 0100000u | (modo ? modo : 0644)) << 16) | (pasta ? 0x10u : 0);
    const uint8_t *cont = dados;
    uint8_t *tmp = NULL;
    e->metodo = 0;
    e->tc = n;
    if (!pasta && nivel > 0 && n > 0) {
        tmp = aloca((size_t)deflar_limite((int)n));
        if (!tmp) { libera(nm); return COMP_ERRO_MEMORIA; }
        int r = deflar(dados, (int)n, tmp, deflar_limite((int)n), nivel);
        if (r >= 0 && (uint32_t)r < n) { e->metodo = 8; e->tc = (uint32_t)r; cont = tmp; }
    }
    e->desloc = (uint32_t)z->b.n;
    uint8_t h[30];
    memset(h, 0, sizeof h);
    p32(h, 0x04034B50);
    p16(h + 4, 20);
    p16(h + 6, e->flags);
    p16(h + 8, e->metodo);
    p16(h + 10, e->hora);
    p16(h + 12, e->data);
    p32(h + 14, e->crc);
    p32(h + 18, e->tc);
    p32(h + 22, e->tu);
    p16(h + 26, (uint32_t)len);
    int r = comp_buf_somar(&z->b, h, 30);
    if (r >= 0) r = comp_buf_somar(&z->b, nm, (int)len);
    if (r >= 0 && e->tc) r = comp_buf_somar(&z->b, cont, (int)e->tc);
    if (tmp) libera(tmp);
    if (r < 0) { libera(nm); return r; }
    z->n++;
    return 0;
}

int zip_concluir(zip_escritor_t *z)
{
    int ini = z->b.n, r = 0;
    for (int i = 0; i < z->n && r >= 0; i++) {
        zip_ent_t *e = &z->e[i];
        uint8_t h[46];
        memset(h, 0, sizeof h);
        p32(h, 0x02014B50);
        p16(h + 4, 3 << 8 | 20);   /* feito no Unix: os atributos têm o modo */
        p16(h + 6, 20);
        p16(h + 8, e->flags);
        p16(h + 10, e->metodo);
        p16(h + 12, e->hora);
        p16(h + 14, e->data);
        p32(h + 16, e->crc);
        p32(h + 20, e->tc);
        p32(h + 24, e->tu);
        p16(h + 28, (uint32_t)tam_texto(e->nome));
        p32(h + 38, e->attr);
        p32(h + 42, e->desloc);
        r = comp_buf_somar(&z->b, h, 46);
        if (r >= 0) r = comp_buf_somar(&z->b, e->nome, (int)tam_texto(e->nome));
    }
    if (r < 0) return r;
    uint8_t f[22];
    memset(f, 0, sizeof f);
    p32(f, 0x06054B50);
    p16(f + 8, (uint32_t)z->n);
    p16(f + 10, (uint32_t)z->n);
    p32(f + 12, (uint32_t)(z->b.n - ini));
    p32(f + 16, (uint32_t)ini);
    return comp_buf_somar(&z->b, f, 22);
}

void zip_escritor_liberar(zip_escritor_t *z)
{
    for (int i = 0; i < z->n; i++) libera(z->e[i].nome);
    if (z->e) libera(z->e);
    comp_buf_liberar(&z->b);
    memset(z, 0, sizeof *z);
}

int zip_abrir(zip_leitor_t *z, const uint8_t *d, int n)
{
    memset(z, 0, sizeof *z);
    z->d = d; z->n = n;
    if (n < 22) return n >= 4 && l32(d) == 0x04034B50 ? COMP_ERRO_TRUNCADO : COMP_ERRO_FORMATO;
    int p = n - 22, limite = n - 22 - 65535 < 0 ? 0 : n - 22 - 65535;
    while (p >= limite && l32(d + p) != 0x06054B50) p--;
    if (p < limite) return COMP_ERRO_FORMATO;
    int cnt = (int)l16(d + p + 10);
    uint32_t tcd = l32(d + p + 12), ocd = l32(d + p + 16);
    if (cnt == 0xFFFF || tcd == 0xFFFFFFFFu || ocd == 0xFFFFFFFFu) return COMP_ERRO_NAO_SUPORTADO;   /* ZIP64 */
    if ((uint64_t)ocd + tcd > (uint64_t)p) return COMP_ERRO_TRUNCADO;
    z->e = aloca(sizeof(zip_ent_t) * (size_t)(cnt ? cnt : 1));
    if (!z->e) return COMP_ERRO_MEMORIA;
    memset(z->e, 0, sizeof(zip_ent_t) * (size_t)(cnt ? cnt : 1));
    int q = (int)ocd;
    for (int i = 0; i < cnt; i++) {
        if (q + 46 > p || l32(d + q) != 0x02014B50) { zip_leitor_liberar(z); return COMP_ERRO_FORMATO; }
        zip_ent_t *e = &z->e[i];
        int nl = (int)l16(d + q + 28), xl = (int)l16(d + q + 30), cl = (int)l16(d + q + 32);
        if (q + 46 + nl + xl + cl > p) { zip_leitor_liberar(z); return COMP_ERRO_FORMATO; }
        e->flags = (uint16_t)l16(d + q + 8);
        e->metodo = (uint16_t)l16(d + q + 10);
        e->hora = (uint16_t)l16(d + q + 12);
        e->data = (uint16_t)l16(d + q + 14);
        e->crc = l32(d + q + 16);
        e->tc = l32(d + q + 20);
        e->tu = l32(d + q + 24);
        e->attr = l32(d + q + 38);
        e->desloc = l32(d + q + 42);
        e->nome = aloca((size_t)nl + 1);
        if (!e->nome) { z->cnt = i; zip_leitor_liberar(z); return COMP_ERRO_MEMORIA; }
        memcpy(e->nome, d + q + 46, (size_t)nl);
        e->nome[nl] = 0;
        e->pasta = nl > 0 && e->nome[nl - 1] == '/';
        /* D341: el campo extra "UT" (0x5455) lleva la fecha de modificación en UTC (la del DOS es local) */
        for (int x = q + 46 + nl, fim = x + xl; x + 4 <= fim;) {
            int id = (int)l16(d + x), tam = (int)l16(d + x + 2);
            if (x + 4 + tam > fim) break;
            if (id == 0x5455 && tam >= 5 && (d[x + 4] & 1)) e->mtime_ut = l32(d + x + 5);
            x += 4 + tam;
        }
        z->cnt = i + 1;

        q += 46 + nl + xl + cl;
    }
    return 0;
}

int zip_extrair(const zip_leitor_t *z, int i, uint8_t **saida)
{
    const zip_ent_t *e = &z->e[i];
    if (e->flags & 1) return COMP_ERRO_NAO_SUPORTADO;   /* cifrado */
    if (e->tu == 0xFFFFFFFFu || e->tc == 0xFFFFFFFFu || e->desloc == 0xFFFFFFFFu) return COMP_ERRO_NAO_SUPORTADO;
    if (e->metodo != 0 && e->metodo != 8) return COMP_ERRO_NAO_SUPORTADO;
    if (e->tu > COMP_LIMITE_MAX) return COMP_ERRO_GRANDE;
    if ((uint64_t)e->desloc + 30 > (uint64_t)z->n || l32(z->d + e->desloc) != 0x04034B50) return COMP_ERRO_FORMATO;
    uint64_t ini = (uint64_t)e->desloc + 30 + l16(z->d + e->desloc + 26) + l16(z->d + e->desloc + 28);
    if (ini + e->tc > (uint64_t)z->n) return COMP_ERRO_TRUNCADO;
    uint8_t *o = aloca((size_t)e->tu + 1);
    if (!o) return COMP_ERRO_MEMORIA;
    int r;
    if (e->metodo == 0) {
        if (e->tc != e->tu) { libera(o); return COMP_ERRO_FORMATO; }
        memcpy(o, z->d + ini, e->tu);
        r = (int)e->tu;
    } else {
        r = inflar(z->d + ini, (int)e->tc, o, (int)e->tu);
        if (r == COMP_ERRO_CHEIO) r = COMP_ERRO_FORMATO;   /* dá mais do que o diretório diz */
    }
    if (r < 0) { libera(o); return r; }
    if ((uint32_t)r != e->tu) { libera(o); return COMP_ERRO_TRUNCADO; }
    if (crc32_somar(0, o, e->tu) != e->crc) { libera(o); return COMP_ERRO_CRC; }
    *saida = o;
    return r;
}

void zip_leitor_liberar(zip_leitor_t *z)
{
    for (int i = 0; i < z->cnt; i++) if (z->e[i].nome) libera(z->e[i].nome);
    if (z->e) libera(z->e);
    z->e = NULL;
    z->cnt = 0;
}
