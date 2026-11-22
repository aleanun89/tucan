// SPDX-License-Identifier: GPL-2.0-or-later
/* http.c - el cliente HTTP/1.1 de curl y de Cipó (D046).
 *
 * Base: el http.c del PR #3 del repositorio original (M4R4G0N), que hablaba
 * HTTP/1.0. Aquí la petición es HTTP/1.1 con "Connection: close" (el servidor
 * cierra al final), y la respuesta puede venir con Content-Length, en trozos
 * (Transfer-Encoding: chunked) o hasta que se cierre la conexión. */
#include "sagui.h"

static char minuscula(char c) { return c >= 'A' && c <= 'Z' ? (char)(c + 32) : c; }

bool url_separar(const char *url, url_t *u)
{
    memset(u, 0, sizeof *u);
    u->porta = 80;
    if (comeca_com(url, "http://")) url += 7;
    else if (comeca_com(url, "https://")) { url += 8; u->https = true; u->porta = 443; }
    else
        for (const char *p = url; *p && *p != '/'; p++)
            if (p[0] == ':' && p[1] == '/' && p[2] == '/') return false;   /* "ftp://" y otros esquemas */
    int i = 0;
    const char *arroba = NULL;   /* "usuario@host": ignora el usuario */
    for (const char *p = url; *p && *p != '/' && *p != '?' && *p != '#'; p++) if (*p == '@') arroba = p;
    if (arroba) url = arroba + 1;
    while (*url && *url != '/' && *url != ':' && *url != '?' && *url != '#') {
        if (i >= (int)sizeof u->host - 1) return false;
        u->host[i++] = *url++;
    }
    u->host[i] = 0;
    if (*url == ':') {
        url++;
        int p = 0, dig = 0;
        while (*url >= '0' && *url <= '9') { p = p * 10 + (*url++ - '0'); if (++dig > 5) return false; }
        if (!dig) return false;
        u->porta = p;
    }
    if (*url == '?') { u->caminho[0] = '/'; copia_texto(u->caminho + 1, url, sizeof u->caminho - 1); }
    else copia_texto(u->caminho, *url == '/' ? url : "/", sizeof u->caminho);
    char *frag = acha_char(u->caminho, '#');   /* el "#trecho" no va al servidor */
    if (frag) *frag = 0;
    /* Codifica en %XX lo que no puede ir crudo en la petición (espacios y bytes
     * altos): "/foto uno.png" -> "/foto%20uno.png". Antes esto daba por inválida
     * la dirección y el recurso nunca se pedía. */
    {
        static const char hex[] = "0123456789ABCDEF";
        char cod[sizeof u->caminho * 3];
        int j = 0;
        for (int i = 0; u->caminho[i]; i++) {
            unsigned char c = (unsigned char)u->caminho[i];
            if (c <= 0x20 || c >= 0x7f) {
                if (j + 3 >= (int)sizeof cod) return false;
                cod[j++] = '%';
                cod[j++] = hex[c >> 4];
                cod[j++] = hex[c & 15];
            } else {
                if (j >= (int)sizeof cod) return false;
                cod[j++] = (char)c;
            }
        }
        cod[j] = 0;
        if (j >= (int)sizeof u->caminho) return false;   /* ya no cabe codificado */
        copia_texto(u->caminho, cod, sizeof u->caminho);
    }
    return u->host[0] && u->porta > 0 && u->porta <= 65535;
}

/* Quita "." y ".." de una ruta absoluta (lo que viene después de ? o # queda). */
static void normalizar(char *c)
{
    char *fim = c;
    while (*fim && *fim != '?' && *fim != '#') fim++;
    char resto[512];
    copia_texto(resto, fim, sizeof resto);
    *fim = 0;
    char saida[512];
    int k = 0;
    saida[0] = 0;
    for (char *p = c; *p;) {
        while (*p == '/') p++;
        char *ini = p;
        while (*p && *p != '/') p++;
        int n = (int)(p - ini);
        bool barra_final = *p == '/' || !*p;
        if (n == 0) break;
        if (n == 1 && ini[0] == '.') { if (!*p && k) { saida[k++] = '/'; saida[k] = 0; } continue; }
        if (n == 2 && ini[0] == '.' && ini[1] == '.') {
            while (k > 0 && saida[k - 1] == '/') k--;
            while (k > 0 && saida[k - 1] != '/') k--;
            saida[k] = 0;
            continue;
        }
        if (k > 0 && saida[k - 1] == '/') k--;
        if (k + n + 2 >= (int)sizeof saida) break;
        saida[k++] = '/';
        memcpy(saida + k, ini, (size_t)n);
        k += n;
        if (*p == '/' && !p[1]) saida[k++] = '/';
        saida[k] = 0;
        (void)barra_final;
    }
    if (!k) { saida[0] = '/'; saida[1] = 0; }
    copia_texto(c, saida, 512);
    junta_texto(c, resto, 512);
}

bool url_juntar(const char *base, const char *rel, char *saida, int max)
{
    while (*rel == ' ') rel++;
    if (comeca_com(rel, "http://") || comeca_com(rel, "https://")) { copia_texto(saida, rel, (size_t)max); return true; }
    url_t b;
    if (!url_separar(base, &b)) return false;
    const char *esquema = b.https ? "https" : "http";
    char porta[8] = "";
    if (b.porta != (b.https ? 443 : 80)) formatar_texto(porta, sizeof porta, ":%d", b.porta);
    if (comeca_com(rel, "//")) { formatar_texto(saida, (size_t)max, "%s:%s", esquema, rel); return true; }
    char caminho[512];
    if (rel[0] == '/') copia_texto(caminho, rel, sizeof caminho);
    else if (rel[0] == '?' || rel[0] == '#' || !rel[0]) {
        copia_texto(caminho, b.caminho, sizeof caminho);
        char *corte = acha_char(caminho, rel[0] == '#' ? '#' : '?');
        if (corte) *corte = 0;
        if (rel[0] == '#') { corte = acha_char(caminho, '#'); if (corte) *corte = 0; }
        junta_texto(caminho, rel, sizeof caminho);
    } else {
        copia_texto(caminho, b.caminho, sizeof caminho);
        char *q = acha_char(caminho, '?');
        if (q) *q = 0;
        char *barra = acha_ultimo_char(caminho, '/');
        if (barra) barra[1] = 0; else copia_texto(caminho, "/", sizeof caminho);
        junta_texto(caminho, rel, sizeof caminho);
    }
    normalizar(caminho);
    formatar_texto(saida, (size_t)max, "%s://%s%s%s", esquema, b.host, porta, caminho);
    return true;
}

int http_montar_pedido(const url_t *u, const char *metodo, const char *extra, char *buf, int max)
{
    char porta[8] = "";
    if (u->porta != (u->https ? 443 : 80)) formatar_texto(porta, sizeof porta, ":%d", u->porta);
    return formatar_texto(buf, (size_t)max,
                          "%s %s HTTP/1.1\r\nHost: %s%s\r\nUser-Agent: SaguiOS/1.0\r\nAccept: */*\r\n"
                          "Connection: close\r\n%s\r\n", metodo, u->caminho, u->host, porta, extra ? extra : "");
}

int http_pedir(const url_t *u, const char *metodo, const char *extra, char *pedido_enviado, int max)
{
    int fd = conectar_tcp(u->host, u->porta, 20000);
    if (fd < 0) return fd;
    char pedido[1024];
    int n = http_montar_pedido(u, metodo, extra, pedido, sizeof pedido);
    if (pedido_enviado) copia_texto(pedido_enviado, pedido, (size_t)max);
    int r = escrever_tudo(fd, pedido, (unsigned)n);
    if (r < 0) { fechar(fd); return r; }
    return fd;
}

bool http_campo(const char *cab, const char *nome, char *valor, int max)
{
    int t = (int)tam_texto(nome);
    for (const char *l = cab; l && *l;) {
        int i = 0;
        while (i < t && l[i] && minuscula(l[i]) == minuscula(nome[i])) i++;
        if (i == t && l[i] == ':') {
            l += t + 1;
            while (*l == ' ' || *l == '\t') l++;
            int k = 0;
            while (*l && *l != '\r' && *l != '\n' && k < max - 1) valor[k++] = *l++;
            valor[k] = 0;
            return true;
        }
        l = acha_char(l, '\n');
        if (l) l++;
    }
    return false;
}

static int ler_de(http_resposta_t *r, void *buf, unsigned n)
{
    return r->ler_fn ? r->ler_fn(r->ctx, buf, n) : ler(r->fd, buf, n);
}

static int encher(http_resposta_t *r)
{
    if (r->ini < r->fim) return r->fim - r->ini;
    r->ini = r->fim = 0;
    int n = ler_de(r, r->buf, sizeof r->buf);
    if (n > 0) r->fim = n;
    return n;
}

int http_ler_cabecalho(http_resposta_t *r, int fd) { return http_ler_cabecalho_de(r, NULL, (void *)fd); }

int http_ler_cabecalho_de(http_resposta_t *r, http_ler_t f, void *ctx)
{
    memset(r, 0, sizeof *r);
    r->fd = f ? -1 : (int)ctx;
    r->ler_fn = f;
    r->ctx = ctx;
    for (;;) {
        int cab_tam = 0;          /* bytes escritos na cabecalho */
        bool descartar = false;   /* a linha actual e demasiado comprida */
        for (;;) {
            /* procura uma linha completa no que ja esta lido */
            int nl = -1;
            for (int i = r->ini; i < r->fim; i++)
                if (r->buf[i] == '\n') { nl = i; break; }
            if (nl < 0) {
                if (r->fim == (int)sizeof r->buf) {
                    /* a linha e mais comprida que o buffer (o GitHub manda
                     * cabecalheiros de varios quilobytes): deita fora o que ja
                     * foi lido e continua a ler ate ao fim da linha. */
                    descartar = true;
                    r->ini = r->fim = 0;
                }
                int k = ler_de(r, r->buf + r->fim, (unsigned)((int)sizeof r->buf - r->fim));
                if (k < 0) return k;
                if (k == 0) return ERRO_FORMATO;   /* fechou antes de terminar a cabecera */
                r->fim += k;
                continue;
            }
            int len = nl - r->ini;
            const char *lin = r->buf + r->ini;
            r->ini = nl + 1;
            if (len > 0 && lin[len - 1] == '\r') len--;
            bool longa = descartar;
            descartar = false;
            if (len == 0) break;        /* linha vazia: a cabecera acabou */
            if (longa) continue;        /* linha demasiado comprida: deita fora */
            if (cab_tam + len + 2 > (int)sizeof r->cabecalho) continue;  /* nao cabe: deita fora */
            memcpy(r->cabecalho + cab_tam, lin, (size_t)len);
            cab_tam += len;
            r->cabecalho[cab_tam++] = '\n';
            r->cabecalho[cab_tam] = 0;
        }
        r->tam_cabecalho = cab_tam;
        /* "ICY 200 OK" (D225): las emisoras viejas de Shoutcast contestan así en lugar de "HTTP/1.x". */
        if (!comeca_com(r->cabecalho, "HTTP/") && !comeca_com(r->cabecalho, "ICY ")) return ERRO_FORMATO;
        const char *esp = acha_char(r->cabecalho, ' ');
        r->codigo = esp ? texto_para_int(esp + 1) : 0;
        if (r->codigo < 100 || r->codigo > 999) return ERRO_FORMATO;
        if (r->codigo < 200) {   /* "100 Continue": viene otra cabecera después */
            continue;
        }
        char v[64];
        r->restante = -1;
        if (http_campo(r->cabecalho, "Transfer-Encoding", v, sizeof v)) {
            for (char *p = v; *p; p++) *p = minuscula(*p);
            for (char *p = v; *p && !r->pedacos; p++) r->pedacos = comeca_com(p, "chunked");
        }
        if (!r->pedacos && http_campo(r->cabecalho, "Content-Length", v, sizeof v)) {
            if (v[0] < '0' || v[0] > '9' || tam_texto(v) > 9) return ERRO_FORMATO;
            r->restante = texto_para_int(v);
        }
        if (r->codigo == 204 || r->codigo == 304) r->acabou = true;
        return 0;
    }
}

/* Una línea del cuerpo (tamaño de los trozos), sin el fin de línea. */
static int linha(http_resposta_t *r, char *s, int max)
{
    int k = 0;
    for (;;) {
        int n = encher(r);
        if (n <= 0) return n < 0 ? n : ERRO_CONEXAO_CAIU;
        char c = r->buf[r->ini++];
        if (c == '\n') break;
        if (c != '\r' && k < max - 1) s[k++] = c;
    }
    s[k] = 0;
    return k;
}

static int copiar(http_resposta_t *r, char *dst, int max)
{
    if (r->ini < r->fim) {
        int n = r->fim - r->ini;
        if (n > max) n = max;
        memcpy(dst, r->buf + r->ini, (size_t)n);
        r->ini += n;
        return n;
    }
    return ler_de(r, dst, (unsigned)max);   /* directo al destino: sin copia de más */
}

int http_ler_corpo(http_resposta_t *r, void *destino, int max)
{
    char *dst = destino;
    if (r->acabou || max <= 0) return 0;
    if (!r->pedacos) {
        if (r->restante == 0) { r->acabou = true; return 0; }
        if (r->restante > 0 && max > r->restante) max = r->restante;
        int n = copiar(r, dst, max);
        if (n < 0) return n;
        if (n == 0) {
            r->acabou = true;
            return r->restante > 0 ? ERRO_CONEXAO_CAIU : 0;   /* cerró antes del Content-Length */
        }
        if (r->restante > 0) r->restante -= n;
        return n;
    }
    /* chunked: "tamaño en hexa\r\n" + datos + "\r\n" ... "0\r\n\r\n" */
    if (r->pedaco <= 0) {
        char s[80];
        int k;
        if (r->pedaco < 0 && (k = linha(r, s, sizeof s)) != 0) return k < 0 ? k : ERRO_FORMATO;   /* el \r\n después del trozo */
        k = linha(r, s, sizeof s);
        if (k < 0) return k;
        int t = 0, dig = 0;
        for (char *p = s; *p && *p != ';' && *p != ' '; p++, dig++) {
            int v = (*p >= '0' && *p <= '9') ? *p - '0' : (*p >= 'a' && *p <= 'f') ? *p - 'a' + 10 :
                    (*p >= 'A' && *p <= 'F') ? *p - 'A' + 10 : -1;
            if (v < 0 || t > 0x7FFFFFF) return ERRO_FORMATO;
            t = t * 16 + v;
        }
        if (!dig) return ERRO_FORMATO;
        if (t == 0) {   /* último: salta los campos finales hasta la línea vacía */
            for (int i = 0; i < 64 && linha(r, s, sizeof s) > 0; i++) ;
            r->acabou = true;
            return 0;
        }
        r->pedaco = t;
    }
    if (max > r->pedaco) max = r->pedaco;
    int n = copiar(r, dst, max);
    if (n <= 0) { r->acabou = true; return n < 0 ? n : ERRO_CONEXAO_CAIU; }
    r->pedaco -= n;
    if (r->pedaco == 0) r->pedaco = -1;
    return n;
}
