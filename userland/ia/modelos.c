// SPDX-License-Identifier: GPL-3.0-or-later
/* modelos.c - el descargador de modelos de la IA local (D380, fase 2). Ver ia_modelos.h.
 *
 * Por qué así:
 *  - Por trozos de 32 MB con «Range: bytes=a-b»: cada respuesta mide menos de 10^9 bytes (el
 *    Content-Length cabe en un int), una conexión cortada solo pierde su trozo y la descarga
 *    se puede seguir otro día desde el «.parte».
 *  - Hugging Face redirige «resolve/main/...» a su CDN con una URL firmada que caduca y que
 *    mide más de lo que cabe en url_t (512): la petición y la cabecera de la respuesta se
 *    leen aquí, con búferes grandes; si la URL firmada deja de valer (403), se vuelve a pedir
 *    la original.
 *  - La suma SHA-256 se calcula al final sobre el fichero entero (también lo que se bajó en
 *    otra sesión); si no coincide, lo bajado se borra y se dice. */
#include "ia_modelos.h"
#include "tls.h"
#include "cripto.h"

#define TROZO (32ull << 20)
#define INTENTOS 6
#define CAB_MAX 16384
#define URL_MAX 4096

/* ------------------------------------------------------------ números de 64 bits */

static int u64_texto(unsigned long long v, char *s)
{
    char t[24];
    int n = 0, k = 0;
    do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) s[k++] = t[--n];
    s[k] = 0;
    return k;
}

static bool texto_u64(const char *s, unsigned long long *v)
{
    unsigned long long x = 0;
    int d = 0;
    while (*s == ' ') s++;
    while (*s >= '0' && *s <= '9') {
        if (x > 1844674407370955161ull) return false;
        x = x * 10 + (unsigned)(*s++ - '0');
        d++;
    }
    *v = x;
    return d > 0;
}

/* ------------------------------------------------------------ la lista */

static void recortar(char *s)
{
    char *i = s;
    while (*i == ' ' || *i == '\t') i++;
    if (i != s) memmove(s, i, tam_texto(i) + 1);
    size_t n = tam_texto(s);
    while (n && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r')) s[--n] = 0;
}

static bool hex64(const char *s)
{
    if (tam_texto(s) != 64) return false;
    for (int i = 0; i < 64; i++) if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f'))) return false;
    return true;
}

/* Un nombre de fichero sin carpetas ni cosas raras (la lista viene de fuera del programa). */
static bool nome_ok(const char *s)
{
    if (!s[0] || s[0] == '.') return false;
    for (; *s; s++) if (*s == '/' || *s == '\\' || (unsigned char)*s < 0x20) return false;
    return true;
}

int ia_catalogo_ler(const char *caminho, ia_modelo_cat_t *v, int max)
{
    if (!caminho) { const char *e = obter_env("IA_CATALOGO"); caminho = e && *e ? e : IA_CATALOGO; }
    int fd = abrir(caminho, ABRIR_LER);
    if (fd < 0) return fd;
    static char buf[32768];
    int n = 0, k;
    while (n < (int)sizeof buf - 1 && (k = ler(fd, buf + n, sizeof buf - 1 - (unsigned)n)) > 0) n += k;
    fechar(fd);
    buf[n] = 0;
    int q = 0;
    for (char *l = buf; l && *l && q < max;) {
        char *fim = acha_char(l, '\n');
        if (fim) *fim = 0;
        char *prox = fim ? fim + 1 : NULL;
        recortar(l);
        if (l[0] && l[0] != '#') {
            /* id | nombre | fichero | bytes | sha256 | url | ram_mb | extra | nota */
            char *c[9] = { 0 };
            int nc = 0;
            for (char *p = l; nc < 9;) {
                c[nc++] = p;
                char *b = acha_char(p, '|');
                if (!b) break;
                *b = 0;
                p = b + 1;
            }
            ia_modelo_cat_t *m = &v[q];
            memset(m, 0, sizeof *m);
            if (nc >= 7) {
                for (int i = 0; i < nc; i++) recortar(c[i]);
                copia_texto(m->id, c[0], sizeof m->id);
                copia_texto(m->nome, c[1], sizeof m->nome);
                copia_texto(m->ficheiro, c[2], sizeof m->ficheiro);
                bool ok = texto_u64(c[3], &m->bytes) && m->bytes > 0;
                if (hex64(c[4])) copia_texto(m->sha256, c[4], sizeof m->sha256);
                else ok = ok && (!compara_texto(c[4], "pendiente") || !c[4][0]);
                copia_texto(m->url, c[5], sizeof m->url);
                m->ram_mb = (unsigned)texto_para_int(c[6]);
                if (nc > 7) copia_texto(m->extra, c[7], sizeof m->extra);
                if (nc > 8) copia_texto(m->nota, c[8], sizeof m->nota);
                if (ok && m->id[0] && nome_ok(m->ficheiro) && (comeca_com(m->url, "https://") || comeca_com(m->url, "http://")))
                    q++;
            }
        }
        l = prox;
    }
    return q;
}

const ia_modelo_cat_t *ia_catalogo_buscar(const ia_modelo_cat_t *v, int n, const char *id)
{
    for (int i = 0; i < n; i++) if (!compara_texto(v[i].id, id)) return &v[i];
    return NULL;
}

static void caminho_de(char *d, int max, const char *pasta, const char *f, const char *suf)
{
    formatar_texto(d, (size_t)max, "%s/%s%s", pasta, f, suf);
}

int ia_modelo_estado(const ia_modelo_cat_t *m, const char *pasta, unsigned long long *feito)
{
    char c[300];
    info_arquivo_t inf;
    if (!pasta) pasta = IA_PASTA_MODELOS;
    *feito = 0;
    caminho_de(c, sizeof c, pasta, m->ficheiro, "");
    if (info_arquivo(c, &inf) == 0 && (unsigned long long)inf.tamanho == m->bytes) { *feito = m->bytes; return 2; }
    caminho_de(c, sizeof c, pasta, m->ficheiro, ".parte");
    if (info_arquivo(c, &inf) == 0) { *feito = inf.tamanho; return 1; }
    return 0;
}

unsigned long long ia_espaco_livre_kb(const char *caminho)
{
    static info_montagem_t mt[16];
    int n = info_sistema(INFO_MONTAGENS, mt, sizeof mt / sizeof mt[0]);
    int melhor = -1;
    size_t mn = 0;
    for (int i = 0; i < n; i++) {
        size_t l = tam_texto(mt[i].ponto);
        bool pref = !compara_texto(mt[i].ponto, "/") ||
                    (!compara_texto_n(caminho, mt[i].ponto, l) && (caminho[l] == 0 || caminho[l] == '/'));
        if (pref && l >= mn) { melhor = i; mn = l; }
    }
    if (melhor >= 0) return mt[melhor].livre_kb;
    info_memoria_t mem;   /* la raíz en la RAM */
    if (info_sistema(INFO_MEMORIA, &mem, sizeof mem) == 0) return mem.livre_kb;
    return 0;
}

/* ------------------------------------------------------------ HTTP con URL largas */

typedef struct {
    conexao_t con;
    char buf[16384];
    int ini, fim;
} leitor_t;

static int encher(leitor_t *r)
{
    if (r->ini < r->fim) return r->fim - r->ini;
    r->ini = r->fim = 0;
    int k = conexao_ler(&r->con, r->buf, sizeof r->buf);
    if (k > 0) r->fim = k;
    return k;
}

/* Una línea de la cabecera (sin \r\n). <0 error o cierre. */
static int linha(leitor_t *r, char *s, int max)
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

static int ler_corpo(leitor_t *r, void *dst, int max)
{
    if (r->ini < r->fim) {
        int n = r->fim - r->ini;
        if (n > max) n = max;
        memcpy(dst, r->buf + r->ini, (size_t)n);
        r->ini += n;
        return n;
    }
    return conexao_ler(&r->con, dst, (unsigned)max);
}

/* URL -> url_t (servidor y puerto) y el camino aparte (puede ser largo). */
static bool separar(const char *url, url_t *u, char *caminho, int max)
{
    memset(u, 0, sizeof *u);
    const char *p;
    if (comeca_com(url, "https://")) { u->https = true; u->porta = 443; p = url + 8; }
    else if (comeca_com(url, "http://")) { u->porta = 80; p = url + 7; }
    else return false;
    int k = 0;
    while (*p && *p != '/' && *p != ':' && *p != '?' && k < (int)sizeof u->host - 1) u->host[k++] = *p++;
    u->host[k] = 0;
    if (!k || (*p && *p != '/' && *p != ':' && *p != '?')) return false;
    if (*p == ':') {
        int port = 0, d = 0;
        p++;
        while (*p >= '0' && *p <= '9' && d < 6) { port = port * 10 + (*p++ - '0'); d++; }
        if (port < 1 || port > 65535) return false;
        u->porta = port;
    }
    if (!*p) p = "/";
    if (*p == '?') { if (max < 3) return false; caminho[0] = '/'; copia_texto(caminho + 1, p, (size_t)max - 1); }
    else copia_texto(caminho, p, (size_t)max);
    if (tam_texto(p) >= (size_t)max - 1) return false;
    for (const char *q = caminho; *q; q++) if ((unsigned char)*q <= 0x20 || *q == 0x7F) return false;   /* sin inyectar cabeceras */
    copia_texto(u->caminho, "/", sizeof u->caminho);
    return true;
}

/* Location relativa o absoluta -> URL nueva. */
static bool resolver_url(const char *base, const char *loc, char *saida, int max)
{
    if (comeca_com(loc, "https://") || comeca_com(loc, "http://")) { if (tam_texto(loc) >= (size_t)max) return false; copia_texto(saida, loc, (size_t)max); return true; }
    if (loc[0] != '/') return false;
    const char *p = base + (comeca_com(base, "https://") ? 8 : 7);
    while (*p && *p != '/' && *p != '?') p++;
    int n = (int)(p - base);
    if (n + (int)tam_texto(loc) >= max) return false;
    memcpy(saida, base, (size_t)n);
    copia_texto(saida + n, loc, (size_t)(max - n));
    return true;
}

typedef struct {
    int codigo;
    bool tem_tam, tem_faixa, pedacos;
    unsigned long long tam, f_ini, f_fim, f_total;
    char local[URL_MAX];
} cab_t;

static bool campo(const char *l, const char *nome, const char **v)
{
    size_t n = tam_texto(nome);
    for (size_t i = 0; i < n; i++) {
        char a = l[i], b = nome[i];
        if (a >= 'A' && a <= 'Z') a = (char)(a + 32);
        if (b >= 'A' && b <= 'Z') b = (char)(b + 32);
        if (a != b) return false;
    }
    if (l[n] != ':') return false;
    l += n + 1;
    while (*l == ' ' || *l == '\t') l++;
    *v = l;
    return true;
}

static int ler_cabecalho(leitor_t *r, cab_t *c)
{
    static char l[URL_MAX + 64];
    memset(c, 0, sizeof *c);
    for (;;) {
        int k = linha(r, l, sizeof l);
        if (k < 0) return k;
        if (!comeca_com(l, "HTTP/")) return ERRO_FORMATO;
        const char *e = acha_char(l, ' ');
        c->codigo = e ? texto_para_int(e + 1) : 0;
        if (c->codigo < 100 || c->codigo > 999) return ERRO_FORMATO;
        int linhas = 0;
        for (;;) {
            k = linha(r, l, sizeof l);
            if (k < 0) return k;
            if (k == 0) break;
            if (++linhas > 200) return ERRO_FORMATO;
            const char *v;
            if (campo(l, "Content-Length", &v)) c->tem_tam = texto_u64(v, &c->tam);
            else if (campo(l, "Location", &v)) copia_texto(c->local, v, sizeof c->local);
            else if (campo(l, "Transfer-Encoding", &v)) c->pedacos = comeca_com(v, "chunked") || comeca_com(v, "Chunked");
            else if (campo(l, "Content-Range", &v)) {   /* bytes a-b/total */
                if (comeca_com(v, "bytes ")) {
                    const char *p = v + 6;
                    c->tem_faixa = texto_u64(p, &c->f_ini);
                    while (*p >= '0' && *p <= '9') p++;
                    if (*p++ != '-' || !texto_u64(p, &c->f_fim)) c->tem_faixa = false;
                    while (*p >= '0' && *p <= '9') p++;
                    if (*p++ != '/' || !texto_u64(p, &c->f_total)) c->tem_faixa = false;
                }
            }
        }
        if (c->codigo >= 200) return 0;   /* 100 Continue: viene otra */
    }
}

/* ------------------------------------------------------------ la descarga */

typedef struct {
    const ia_desc_cb_t *cb;
    unsigned long long total, feito;
    unsigned t0, ult;
    unsigned long long feito0;
} prog_t;

static void avisar(const ia_desc_cb_t *cb, const char *t) { if (cb && cb->aviso) cb->aviso(cb->ctx, t); }

static void progresso(prog_t *p, int fase, bool forcar)
{
    unsigned agora = tempo_ms();
    if (!forcar && agora - p->ult < 300) return;
    p->ult = agora;
    unsigned ms = agora - p->t0;
    unsigned kbs = ms > 500 && fase == 0 ? (unsigned)((p->feito - p->feito0) / ms * 1000 / 1024) : 0;
    if (p->cb && p->cb->progresso) p->cb->progresso(p->cb->ctx, fase, p->feito, p->total, kbs);
}

static bool parar(const ia_desc_cb_t *cb) { return cb && cb->parar && cb->parar(cb->ctx); }

/* Un trozo [ini, fim] a «saida» (ya en la posición ini). -> 0 hecho, 1 redirigido (url cambia),
 * 2 cancelado, <0 error (con erro). «perm» = error que no se arregla reintentando. */
static int trozo(char *url, const char *url_orig, tls_config_t *cfg, int saida, unsigned long long ini, unsigned long long fim,
                 prog_t *p, bool *perm, char *erro, int max)
{
    static leitor_t r;
    static char caminho[URL_MAX], pedido[URL_MAX + 512];
    static uint8_t bloco[16384];
    url_t u;
    *perm = false;
    if (!separar(url, &u, caminho, sizeof caminho)) { *perm = true; formatar_texto(erro, (size_t)max, "dirección no válida"); return -1; }
    memset(&r, 0, sizeof r);
    int rc = conexao_abrir(&r.con, &u, cfg);
    if (rc < 0) {
        formatar_texto(erro, (size_t)max, "%s: %s", u.host, r.con.erro[0] ? r.con.erro : nome_erro(rc));
        *perm = rc == TLS_ERRO_CERT;
        conexao_fechar(&r.con);
        return -1;
    }
    char a[24], b[24];
    u64_texto(ini, a);
    u64_texto(fim, b);
    char porta[16] = "";
    if ((u.https && u.porta != 443) || (!u.https && u.porta != 80)) formatar_texto(porta, sizeof porta, ":%d", u.porta);
    int n = formatar_texto(pedido, sizeof pedido, "GET %s HTTP/1.1\r\nHost: %s%s\r\nUser-Agent: SaguiOS-ia\r\n"
                           "Accept: */*\r\nAccept-Encoding: identity\r\nRange: bytes=%s-%s\r\nConnection: close\r\n\r\n",
                           caminho, u.host, porta, a, b);
    if (n >= (int)sizeof pedido - 1 || conexao_escrever(&r.con, pedido, (unsigned)n) < 0) {
        conexao_fechar(&r.con);
        formatar_texto(erro, (size_t)max, "no se pudo enviar la petición");
        return -1;
    }
    cab_t c;
    rc = ler_cabecalho(&r, &c);
    if (rc < 0) { conexao_fechar(&r.con); formatar_texto(erro, (size_t)max, "respuesta no válida del servidor"); return -1; }
    if (c.codigo == 301 || c.codigo == 302 || c.codigo == 303 || c.codigo == 307 || c.codigo == 308) {
        conexao_fechar(&r.con);
        static char nova[URL_MAX];
        if (!c.local[0] || !resolver_url(url, c.local, nova, sizeof nova)) { *perm = true; formatar_texto(erro, (size_t)max, "redirección no válida"); return -1; }
        if (u.https && comeca_com(nova, "http://")) { *perm = true; formatar_texto(erro, (size_t)max, "el servidor redirige de HTTPS a HTTP: no se sigue"); return -1; }
        copia_texto(url, nova, URL_MAX);
        return 1;
    }
    unsigned long long saltar = 0, quer = fim - ini + 1;
    if (c.codigo == 206) {
        if (!c.tem_faixa || c.f_ini != ini || c.f_fim > fim || c.f_fim < ini) {
            conexao_fechar(&r.con); *perm = true;
            formatar_texto(erro, (size_t)max, "el servidor devolvió otro trozo del fichero");
            return -1;
        }
        if (c.f_total != p->total) {
            conexao_fechar(&r.con); *perm = true;
            formatar_texto(erro, (size_t)max, "el fichero del servidor no mide lo que dice la lista de modelos");
            return -1;
        }
        quer = c.f_fim - ini + 1;
    } else if (c.codigo == 200) {
        /* sin «Range»: viene entero; se salta lo que ya está */
        if (c.tem_tam && c.tam != p->total) {
            conexao_fechar(&r.con); *perm = true;
            formatar_texto(erro, (size_t)max, "el fichero del servidor no mide lo que dice la lista de modelos");
            return -1;
        }
        saltar = ini;
    } else {
        conexao_fechar(&r.con);
        /* una URL firmada que caducó: se vuelve a la original */
        if ((c.codigo == 403 || c.codigo == 410) && compara_texto(url, url_orig)) { copia_texto(url, url_orig, URL_MAX); return 1; }
        *perm = c.codigo == 404 || c.codigo == 401 || c.codigo == 403 || c.codigo == 416;
        formatar_texto(erro, (size_t)max, "el servidor respondió %d", c.codigo);
        return -1;
    }
    if (c.pedacos) { conexao_fechar(&r.con); *perm = true; formatar_texto(erro, (size_t)max, "el servidor manda trozos (chunked): no admitido"); return -1; }
    while (quer > 0) {
        if (parar(p->cb)) { conexao_fechar(&r.con); return 2; }
        int m = (int)sizeof bloco;
        if (saltar && saltar < (unsigned long long)m) m = (int)saltar;
        else if (!saltar && quer < (unsigned long long)m) m = (int)quer;
        int k = ler_corpo(&r, bloco, m);
        if (k <= 0) {
            conexao_fechar(&r.con);
            formatar_texto(erro, (size_t)max, "la conexión se cortó");
            return -1;
        }
        if (saltar) { saltar -= (unsigned)k; continue; }
        if (escrever_tudo(saida, bloco, (unsigned)k) < 0) {
            conexao_fechar(&r.con); *perm = true;
            formatar_texto(erro, (size_t)max, "no se pudo escribir (¿disco lleno?)");
            return -1;
        }
        quer -= (unsigned)k;
        p->feito += (unsigned)k;
        progresso(p, 0, false);
    }
    conexao_fechar(&r.con);
    return 0;
}

static void hexa(const uint8_t *h, char *s)
{
    static const char x[] = "0123456789abcdef";
    for (int i = 0; i < 32; i++) { s[2 * i] = x[h[i] >> 4]; s[2 * i + 1] = x[h[i] & 15]; }
    s[64] = 0;
}

/* SHA-256 del fichero entero. 0 ok, 2 cancelado, <0 error */
static int somar(const char *c, prog_t *p, char *hex)
{
    static uint8_t b[65536];
    int fd = abrir(c, ABRIR_LER);
    if (fd < 0) return fd;
    hash_ctx_t h;
    HASH_SHA256.iniciar(&h);
    p->feito = 0;
    int k;
    while ((k = ler(fd, b, sizeof b)) > 0) {
        HASH_SHA256.somar(&h, b, (unsigned)k);
        p->feito += (unsigned)k;
        progresso(p, 1, false);
        if (parar(p->cb)) { fechar(fd); return 2; }
    }
    fechar(fd);
    if (k < 0) return k;
    uint8_t d[32];
    HASH_SHA256.fim(&h, d);
    hexa(d, hex);
    return 0;
}

/* Como «mkdir -p»: /home/compartido/modelos la crea root al arrancar (D380, con 1777), pero en un sistema
 * recién instalado o con otra carpeta puede faltar alguna de las de arriba. */
static void criar_pastas(const char *c)
{
    char p[300];
    copia_texto(p, c, sizeof p);
    for (char *q = p + 1; *q; q++)
        if (*q == '/') { *q = 0; criar_pasta(p); *q = '/'; }
    criar_pasta(p);
}

int ia_modelo_baixar(const ia_modelo_cat_t *m, const char *pasta, const ia_desc_cb_t *cb, char *erro, int max)
{
    if (!pasta) pasta = IA_PASTA_MODELOS;
    if (m->bytes >= 0x7FFFFFFFull) { formatar_texto(erro, (size_t)max, "el fichero mide más de 2 GB: este sistema aún no los guarda"); return -1; }
    char final_[300], parte[300];
    caminho_de(final_, sizeof final_, pasta, m->ficheiro, "");
    caminho_de(parte, sizeof parte, pasta, m->ficheiro, ".parte");
    criar_pastas(pasta);
    unsigned long long ja;
    if (ia_modelo_estado(m, pasta, &ja) == 2) { avisar(cb, "ya estaba bajado"); return 0; }
    if (ja > m->bytes) { int f = abrir(parte, ABRIR_ESCREVER); if (f >= 0) { truncar(f, 0); fechar(f); } ja = 0; }
    /* espacio: lo que falta y 1 MB de margen */
    unsigned long long livre = ia_espaco_livre_kb(pasta), falta_kb = (m->bytes - ja) / 1024 + 1024;
    if (livre && livre < falta_kb) {
        formatar_texto(erro, (size_t)max, "no hay sitio en %s: hacen falta %u MB y quedan %u MB", pasta,
                       (unsigned)(falta_kb / 1024), (unsigned)(livre / 1024));
        return -1;
    }
    /* memoria: un aviso, no un error (se puede bajar en un equipo para usar en otro) */
    info_memoria_t mem;
    if (m->ram_mb && info_sistema(INFO_MEMORIA, &mem, sizeof mem) == 0 && mem.ram_total_kb / 1024 < m->ram_mb) {
        char t[200];
        formatar_texto(t, sizeof t, "aviso: este modelo necesita unos %u MB de memoria y el equipo tiene %u MB", m->ram_mb,
                       mem.ram_total_kb / 1024);
        avisar(cb, t);
    }
    tls_config_t cfg;
    memset(&cfg, 0, sizeof cfg);
    char e[200];
    if (comeca_com(m->url, "https://")) {
        if (!tls_config_padrao(&cfg, NULL, e, sizeof e)) { formatar_texto(erro, (size_t)max, "%s", e); return -1; }
        cfg.inseguro = cb && cb->inseguro;
    }
    int saida = abrir(parte, ABRIR_ESCREVER | ABRIR_CRIAR);
    if (saida < 0) { formatar_texto(erro, (size_t)max, "no se puede escribir en %s: %s", pasta, nome_erro(saida)); return -1; }
    if (posicionar(saida, (int)ja, POS_INICIO) < 0) { fechar(saida); formatar_texto(erro, (size_t)max, "no se puede seguir el fichero a medias"); return -1; }
    if (ja) { char t[120]; formatar_texto(t, sizeof t, "se sigue la descarga a medias (%u MB ya bajados)", (unsigned)(ja >> 20)); avisar(cb, t); }
    static char url[URL_MAX];
    copia_texto(url, m->url, sizeof url);
    prog_t p = { cb, m->bytes, ja, tempo_ms(), 0, ja };
    int fallos = 0, redir = 0, rc = 0;
    unsigned long long trozo_b = TROZO;
    {   /* IA_DESCARGA_TROZO: trozos más pequeños (para las pruebas: así hay varios en un fichero pequeño) */
        const char *t = obter_env("IA_DESCARGA_TROZO");
        unsigned long long v;
        if (t && texto_u64(t, &v) && v >= 65536 && v <= TROZO) trozo_b = v;
    }
    while (p.feito < m->bytes) {
        unsigned long long ini = p.feito, fim = ini + trozo_b - 1;
        if (fim >= m->bytes) fim = m->bytes - 1;
        bool perm;
        unsigned long long antes = p.feito;
        rc = trozo(url, m->url, &cfg, saida, ini, fim, &p, &perm, erro, max);
        if (rc == 2) { fechar(saida); avisar(cb, "cancelado: lo bajado se guarda para seguir otro día"); return 1; }
        if (rc == 1) { if (++redir > 10) { fechar(saida); formatar_texto(erro, (size_t)max, "demasiadas redirecciones"); return -1; } continue; }
        redir = 0;
        if (rc < 0) {
            if (p.feito > antes) fallos = 0;
            if (perm || ++fallos >= INTENTOS) { fechar(saida); return -1; }
            sincronizar(saida);
            /* lo escrito de un trozo cortado vale: se sigue desde ahí tras una pausa */
            char t[260];
            formatar_texto(t, sizeof t, "%s; se reintenta (%d de %d)", erro, fallos, INTENTOS - 1);
            avisar(cb, t);
            dormir(1000u * (unsigned)fallos);
            if (posicionar(saida, (int)p.feito, POS_INICIO) < 0) { fechar(saida); return -1; }
            copia_texto(url, m->url, sizeof url);
            continue;
        }
        fallos = 0;
    }
    sincronizar(saida);
    fechar(saida);
    progresso(&p, 0, true);
    if (m->sha256[0]) {
        char h[65];
        p.t0 = tempo_ms();
        p.ult = 0;
        int r = somar(parte, &p, h);
        if (r == 2) { avisar(cb, "cancelado: lo bajado se guarda para seguir otro día"); return 1; }
        if (r < 0) { formatar_texto(erro, (size_t)max, "no se pudo leer lo bajado: %s", nome_erro(r)); return -1; }
        progresso(&p, 1, true);
        if (compara_texto(h, m->sha256)) {
            apagar(parte);
            char a[17], b[17];
            copia_texto(a, h, sizeof a);
            copia_texto(b, m->sha256, sizeof b);
            formatar_texto(erro, (size_t)max, "la suma SHA-256 no coincide (%s... en vez de %s...): se borra lo bajado", a, b);
            return -1;
        }
    } else avisar(cb, "aviso: la lista no trae la suma de este fichero; no se ha comprobado");
    apagar(final_);
    int r = renomear(parte, final_);
    if (r < 0) { formatar_texto(erro, (size_t)max, "no se pudo renombrar %s: %s", parte, nome_erro(r)); return -1; }
    return 0;
}
