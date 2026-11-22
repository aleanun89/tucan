// SPDX-License-Identifier: GPL-2.0-or-later
/* ia.c - cliente de chat con IA para la API compatible con la de OpenAI (D134).
 *
 * Habla con cualquier servidor que acepte "POST /v1/chat/completions":
 * un llama.cpp server, el Ollama, el LM Studio, el OpenRouter u otro. La
 * respuesta puede venir en streaming (SSE, "data: {...}") o toda de una vez.
 *
 * No hay una biblioteca JSON completa: lo que interesa aquí son dos cosas,
 * el texto que viene en "choices[0].delta.content" (streaming) o
 * "choices[0].message.content" (todo de una vez), y la lista de modelos de
 * "GET /v1/models". El lector de JSON de abajo busca esos campos y dibuja
 * las secuencias de escape; el resto del objeto se ignora. */
#include "ia.h"
#include "tls.h"
#include "json.h"

#define IA_PRAZO_PADRAO 30000

/* ------------------------------------------------------------ configuracion */

void ia_config_inicial(ia_config_t *c)
{
    memset(c, 0, sizeof *c);
    c->prazo_ms = IA_PRAZO_PADRAO;
}

/* Lee "clave=valor" de un fichero de texto (una línea por par). */
static void ler_pares(const char *caminho, ia_config_t *c)
{
    int fd = abrir(caminho, ABRIR_LER);
    if (fd < 0) return;
    static char texto[8192];
    int n = 0, k;
    while (n < (int)sizeof texto - 1 && (k = ler(fd, texto + n, (unsigned)(sizeof texto - 1 - n))) > 0) n += k;
    fechar(fd);
    texto[n] = 0;
    char *linha = texto;
    while (linha && *linha) {
        char *fim = linha;
        while (*fim && *fim != '\n') fim++;
        char *prox = *fim ? fim + 1 : NULL;
        if (fim > linha) *fim = 0;
        char *igual = acha_char(linha, '=');
        if (igual) {
            *igual = 0;
            const char *nome = linha, *valor = igual + 1;
            if (compara_texto(nome, "url") == 0) copia_texto(c->url, valor, sizeof c->url);
            else if (compara_texto(nome, "modelo") == 0) copia_texto(c->modelo, valor, sizeof c->modelo);
            else if (compara_texto(nome, "sistema") == 0) copia_texto(c->sistema, valor, sizeof c->sistema);
            /* D145: los modelos marcados y el que usa cada programa. */
            else if (compara_texto(nome, "modelos") == 0) copia_texto(c->modelos, valor, sizeof c->modelos);
            else if (compara_texto(nome, "modelo_terminal") == 0) copia_texto(c->modelo_uso[IA_USO_TERMINAL], valor, IA_MODELO_MAX);
            else if (compara_texto(nome, "modelo_chatia") == 0) copia_texto(c->modelo_uso[IA_USO_CHATIA], valor, IA_MODELO_MAX);
            else if (compara_texto(nome, "modelo_mico") == 0) copia_texto(c->modelo_uso[IA_USO_MICO], valor, IA_MODELO_MAX);
        }
        linha = prox;
    }
}

/* La carpeta donde va la configuración de la IA. /home si es el disco
 * persistente; si no existe (o no deja escribir), /tmp, que vale solo para la
 * sesión (como las capturas de pantalla, D125). */
static char pasta_conf_ia[32];
static char caminho_provedor[48];
static char caminho_chave[48];

static void decidir_pasta(void)
{
    if (pasta_conf_ia[0]) return;
    int r = criar_pasta("/home/.config");
    if (r == 0 || r == ERRO_JA_EXISTE) {
        criar_pasta("/home/.config/ia");
        copia_texto(pasta_conf_ia, "/home/.config/ia", sizeof pasta_conf_ia);
    } else {
        criar_pasta("/tmp/ia");
        copia_texto(pasta_conf_ia, "/tmp/ia", sizeof pasta_conf_ia);
    }
    formatar_texto(caminho_provedor, sizeof caminho_provedor, "%s/provedor", pasta_conf_ia);
    formatar_texto(caminho_chave, sizeof caminho_chave, "%s/chave", pasta_conf_ia);
}

/* El nombre del fichero con los pares (lo usan la ventana de Configuración y
 * este módulo). */
const char *ia_caminho_provedor(void) { decidir_pasta(); return caminho_provedor; }

/* El nombre del fichero con la clave (idem). */
const char *ia_caminho_chave(void) { decidir_pasta(); return caminho_chave; }

/* D239: la carpeta (para las conversaciones del chat) y si es la persistente. */
const char *ia_pasta(void) { decidir_pasta(); return pasta_conf_ia; }

bool ia_pasta_persistente(void) { decidir_pasta(); return comeca_com(pasta_conf_ia, "/home/"); }

/* Si el entorno puso el modelo: entonces gana sobre el de cada programa
 * (es lo explícito de la sesión). */
static bool modelo_do_ambiente;

/* La clave, de su fichero (sin el salto de línea del final). */
static void ler_chave(ia_config_t *c)
{
    int fd = abrir(ia_caminho_chave(), ABRIR_LER);
    if (fd < 0) return;
    static char texto[512];
    int n = 0, k;
    while (n < (int)sizeof texto - 1 && (k = ler(fd, texto + n, (unsigned)(sizeof texto - 1 - n))) > 0) n += k;
    fechar(fd);
    texto[n] = 0;
    while (n > 0 && (texto[n - 1] == '\n' || texto[n - 1] == '\r' || texto[n - 1] == ' ')) texto[--n] = 0;
    copia_texto(c->chave, texto, sizeof c->chave);
}

void ia_config_carregar_arquivo(ia_config_t *c)
{
    ia_config_inicial(c);
    ler_pares(ia_caminho_provedor(), c);
    ler_chave(c);
}

void ia_config_carregar(ia_config_t *c)
{
    ia_config_inicial(c);
    modelo_do_ambiente = false;
    /* El fichero primero; el entorno gana después. */
    ler_pares(ia_caminho_provedor(), c);
    /* La clave vive sola en su fichero (y sólo se lee; nunca se escribe
     * en el repositorio ni en la imagen). */
    int fd = abrir(ia_caminho_chave(), ABRIR_LER);
    if (fd >= 0) {
        static char texto[512];
        int n = 0, k;
        while (n < (int)sizeof texto - 1 && (k = ler(fd, texto + n, (unsigned)(sizeof texto - 1 - n))) > 0) n += k;
        fechar(fd);
        texto[n] = 0;
        while (n > 0 && (texto[n - 1] == '\n' || texto[n - 1] == '\r' || texto[n - 1] == ' ')) texto[--n] = 0;
        copia_texto(c->chave, texto, sizeof c->chave);
    }
    const char *v;
    if ((v = obter_env("IA_URL")) && *v) copia_texto(c->url, v, sizeof c->url);
    if ((v = obter_env("IA_MODELO")) && *v) { copia_texto(c->modelo, v, sizeof c->modelo); modelo_do_ambiente = true; }
    if ((v = obter_env("IA_CHAVE")) && *v) copia_texto(c->chave, v, sizeof c->chave);
    if ((v = obter_env("IA_SISTEMA")) && *v) copia_texto(c->sistema, v, sizeof c->sistema);
}

void ia_config_escolher(ia_config_t *c, int uso)
{
    if (uso < 0 || uso >= IA_USO_QTD) return;
    if (modelo_do_ambiente) return;          /* el entorno manda */
    const char *m = c->modelo_uso[uso];
    if (m && *m) copia_texto(c->modelo, m, sizeof c->modelo);
}

/* Los proveedores con nombre (D145). Los tres primeros son los más usados:
 * la OpenAI de verdad, el OpenRouter (que tiene muchos modelos) y un
 * llama.cpp server en la red local. El último es el "personalizado": la
 * dirección la pone el usuario. */
static const ia_provedor_t g_provedores[] = {
    { "OpenAI",     "https://api.openai.com/v1",       "gpt-4o-mini",           true  },
    { "OpenRouter", "https://openrouter.ai/api/v1",    "openai/gpt-4o-mini",    true  },
    { "llama.cpp",  "http://127.0.0.1:8080/v1",        "local",                 false },
    { "Ollama",     "http://127.0.0.1:11434/v1",       "llama3.2",              false },
    { "LM Studio",  "http://127.0.0.1:1234/v1",        "local-model",           false },
    { "Groq",       "https://api.groq.com/openai/v1",  "llama-3.3-70b-versatile", true },
    { "DeepSeek",   "https://api.deepseek.com/v1",     "deepseek-chat",         true  },
    { "Personalizado", "",                             "",                      false },
};

int ia_provedores(ia_provedor_t *destino, int max)
{
    int n = (int)(sizeof g_provedores / sizeof g_provedores[0]);
    if (max < n) n = max;
    for (int i = 0; i < n; i++) destino[i] = g_provedores[i];
    return n;
}

/* Escribe un fichero de texto (para el provedor y para la clave). */
static int gravar_texto(const char *caminho, const char *texto)
{
    int fd = abrir(caminho, ABRIR_ESCREVER | ABRIR_CRIAR | ABRIR_TRUNCAR);
    if (fd < 0) return fd;
    int r = escrever_tudo(fd, texto, (unsigned)tam_texto(texto));
    fechar(fd);
    return r < 0 ? r : 0;
}

int ia_config_gravar(const ia_config_t *c, int gravar_chave)
{
    /* La carpeta decide dónde queda: /home si es persistente, /tmp si no. */
    const char *provedor = ia_caminho_provedor();
    char t[2048];
    formatar_texto(t, sizeof t,
                   "# Configuración de la IA (la escribe la ventana «Configuración»; D145).\n"
                   "url=%s\nmodelo=%s\nsistema=%s\nmodelos=%s\n"
                   "modelo_terminal=%s\nmodelo_chatia=%s\nmodelo_mico=%s\n",
                   c->url, c->modelo, c->sistema, c->modelos,
                   c->modelo_uso[IA_USO_TERMINAL], c->modelo_uso[IA_USO_CHATIA],
                   c->modelo_uso[IA_USO_MICO]);
    int r = gravar_texto(provedor, t);
    if (r < 0) return r;
    if (gravar_chave) {
        char k[IA_CHAVE_MAX + 2];
        formatar_texto(k, sizeof k, "%s\n", c->chave);
        r = gravar_texto(ia_caminho_chave(), k);
        if (r < 0) return r;
    }
    return 0;
}

/* --------------------------------------------------------------- JSON */

/* Latin-1 -> UTF-8 (el texto de la pantalla es Latin-1; la API espera UTF-8). */
static int latin1_para_utf8(const char *s, int n, char *dst, int max)
{
    int k = 0;
    for (int i = 0; i < n && k < max - 2; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) dst[k++] = (char)c;
        else { dst[k++] = (char)(0xC0 | (c >> 6)); dst[k++] = (char)(0x80 | (c & 0x3F)); }
    }
    dst[k] = 0;
    return k;
}

/* Escribe un texto como string JSON (con comillas y escapes). */
static int json_cadeia(char *dst, int max, const char *s, int n)
{
    int k = 0;
    if (k < max - 1) dst[k++] = '"';
    for (int i = 0; i < n && k < max - 3; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '"' || c == '\\') { dst[k++] = '\\'; dst[k++] = (char)c; }
        else if (c == '\n') { dst[k++] = '\\'; dst[k++] = 'n'; }
        else if (c == '\r') { dst[k++] = '\\'; dst[k++] = 'r'; }
        else if (c == '\t') { dst[k++] = '\\'; dst[k++] = 't'; }
        else if (c < 0x20) { k += formatar_texto(dst + k, (size_t)(max - k), "\\u%04x", c); }
        else dst[k++] = (char)c;
    }
    if (k < max - 1) dst[k++] = '"';
    dst[k] = 0;
    return k;
}

/* Busca "nombre": devuelve el valor (apuntando a lo siguiente útil:
 * la comilla de una string, el '{', el '[' o un número). NULL si no hay. */
static const char *achar_campo(const char *t, const char *nome)
{
    size_t n = tam_texto(nome);
    const char *p = t;
    while (p && *p) {
        if (*p != '"') { p++; continue; }
        p++;
        if (compara_texto_n(p, nome, n) == 0 && p[n] == '"') {
            const char *q = p + n + 1;
            while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') q++;
            if (*q == ':') {
                q++;
                while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') q++;
                return q;
            }
        }
        /* salta a la string actual */
        while (*p && *p != '"') { if (*p == '\\' && p[1]) p++; p++; }
        if (*p) p++;
    }
    return NULL;
}

/* Lee una string JSON desde la comilla; escribe en "dst" (ya en UTF-8,
 * pasado a Latin-1 al final). Devuelve los bytes escritos, o -1 si no hay
 * string. */
static int ler_string(const char *v, char *dst, int max)
{
    if (!v || *v != '"') return -1;
    v++;
    int k = 0;
    while (*v && *v != '"') {
        if (k >= max - 6) break;
        if (*v == '\\') {
            v++;
            switch (*v) {
            case 'n': dst[k++] = '\n'; v++; break;
            case 't': dst[k++] = '\t'; v++; break;
            case 'r': dst[k++] = '\r'; v++; break;
            case 'b': dst[k++] = '\b'; v++; break;
            case 'f': dst[k++] = '\f'; v++; break;
            case '"': dst[k++] = '"'; v++; break;
            case '\\': dst[k++] = '\\'; v++; break;
            case '/': dst[k++] = '/'; v++; break;
            case 'u': {
                int cp = 0;
                for (int i = 1; i <= 4 && v[i]; i++) {
                    char c = v[i];
                    int d = (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'f') ? c - 'a' + 10 :
                            (c >= 'A' && c <= 'F') ? c - 'A' + 10 : -1;
                    if (d < 0) break;
                    cp = cp * 16 + d;
                }
                v += 5;
                if (cp < 0x80) dst[k++] = (char)cp;
                else if (cp < 0x800) { dst[k++] = (char)(0xC0 | (cp >> 6)); dst[k++] = (char)(0x80 | (cp & 0x3F)); }
                else { dst[k++] = (char)(0xE0 | (cp >> 12)); dst[k++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                       dst[k++] = (char)(0x80 | (cp & 0x3F)); }
                break;
            }
            default: dst[k++] = *v ? *v++ : 0; break;
            }
        } else dst[k++] = *v++;
    }
    dst[k] = 0;
    return k;
}

/* El texto de un chunk: "choices[0].delta.content" (streaming) o
 * "choices[0].message.content" (respuesta entera). */
static const char *texto_do_chunk(const char *json)
{
    const char *delta = achar_campo(json, "delta");
    if (delta) {
        const char *c = achar_campo(delta, "content");
        if (c && *c == '"') return c;
    }
    const char *msg = achar_campo(json, "message");
    if (msg) {
        const char *c = achar_campo(msg, "content");
        if (c && *c == '"') return c;
    }
    return NULL;
}

/* --------------------------------------------------------------- HTTP */

/* Conecta con el servidor y envia la peticion; devuelve la conexion (tls) o el error. */
static int ligar(const ia_config_t *c, const char *caminho, const char *metodo,
                 const char *cabecalho_extra, const char *corpo, int ncorpo,
                 conexao_t *con, url_t *u, char *erro, int max_erro)
{
    char url[512];
    copia_texto(url, c->url, sizeof url);
    size_t n = tam_texto(url);
    while (n > 1 && url[n - 1] == '/') url[--n] = 0;
    junta_texto(url, caminho, sizeof url);
    if (!url_separar(url, u)) {
        formatar_texto(erro, (size_t)max_erro, "endereco invalido: %s", c->url);
        return -1;
    }
    tls_config_t cfg;
    memset(&cfg, 0, sizeof cfg);
    if (u->https) {
        char e[200];
        if (!tls_config_padrao(&cfg, NULL, e, sizeof e)) {
            formatar_texto(erro, (size_t)max_erro, "%s", e);
            return -1;
        }
        cfg.inseguro = c->inseguro;
    }
    int r = conexao_abrir(con, u, &cfg);
    if (r < 0) {
        formatar_texto(erro, (size_t)max_erro, "%s", con->erro[0] ? con->erro : nome_erro(r));
        return r;
    }
    char pedido[2048];
    int np = http_montar_pedido(u, metodo, cabecalho_extra, pedido, sizeof pedido);
    if (np < 0) { conexao_fechar(con); formatar_texto(erro, (size_t)max_erro, "pedido grande demais"); return -1; }
    if (conexao_escrever(con, pedido, (unsigned)np) < 0 ||
        (ncorpo > 0 && conexao_escrever(con, corpo, (unsigned)ncorpo) < 0)) {
        conexao_fechar(con);
        formatar_texto(erro, (size_t)max_erro, "nao se conseguiu enviar o pedido");
        return -1;
    }
    return 0;
}

/* Monta el cuerpo JSON de la conversacion: el mensaje de sistema (si hay) y
 * despues los mensajes, por orden. El texto de cada uno pasa a UTF-8 antes de ir. */
static int corpo_conversa(const ia_config_t *c, const ia_mensagem_t *msgs, int nmsgs,
                          char *dst, int max)
{
    static char utf8[IA_MSG_MAX * 3];
    int n = 0;
    n += formatar_texto(dst + n, (size_t)(max - n), "{\"model\":");
    json_cadeia(dst + n, max - n, c->modelo[0] ? c->modelo : "gpt-3.5-turbo",
                (int)tam_texto(c->modelo[0] ? c->modelo : "gpt-3.5-turbo"));
    n += (int)tam_texto(dst + n);
    n += formatar_texto(dst + n, (size_t)(max - n), ",\"messages\":[");
    bool primeira = true;
    if (c->sistema[0]) {
        latin1_para_utf8(c->sistema, (int)tam_texto(c->sistema), utf8, sizeof utf8);
        n += formatar_texto(dst + n, (size_t)(max - n), "{\"role\":\"system\",\"content\":");
        json_cadeia(dst + n, max - n, utf8, (int)tam_texto(utf8));
        n += (int)tam_texto(dst + n);
        n += formatar_texto(dst + n, (size_t)(max - n), "}");
        primeira = false;
    }
    for (int i = 0; i < nmsgs; i++) {
        if (!msgs[i].texto[0]) continue;
        const char *papel = msgs[i].papel[0] ? msgs[i].papel : "user";
        latin1_para_utf8(msgs[i].texto, (int)tam_texto(msgs[i].texto), utf8, sizeof utf8);
        if (!primeira) n += formatar_texto(dst + n, (size_t)(max - n), ",");
        n += formatar_texto(dst + n, (size_t)(max - n), "{\"role\":\"");
        n += formatar_texto(dst + n, (size_t)(max - n), "%s", papel);
        n += formatar_texto(dst + n, (size_t)(max - n), "\",\"content\":");
        json_cadeia(dst + n, max - n, utf8, (int)tam_texto(utf8));
        n += (int)tam_texto(dst + n);
        n += formatar_texto(dst + n, (size_t)(max - n), "}");
        primeira = false;
    }
    n += formatar_texto(dst + n, (size_t)(max - n), "],\"stream\":true}");
    return n;
}

/* Las cabeceras de la peticion (clave, tipo del contenido y el tamano). */
static void cabecalhos(const ia_config_t *c, const char *tipo, int ncorpo, char *dst, int max)
{
    int n = formatar_texto(dst, (size_t)max, "Content-Type: %s\r\nContent-Length: %d\r\n", tipo, ncorpo);
    if (c->chave[0]) n += formatar_texto(dst + n, (size_t)(max - n), "Authorization: Bearer %s\r\n", c->chave);
    dst[n] = 0;
}

/* --------------------------------------------------------------- modelos */

int ia_modelos(const ia_config_t *c, char (*nomes)[64], int max, char *erro, int max_erro)
{
    if (!c->url[0]) { formatar_texto(erro, (size_t)max_erro, "sem endereco: defina IA_URL"); return -1; }
    conexao_t con;
    url_t u;
    char cab[512];
    cabecalhos(c, "application/json", 0, cab, sizeof cab);
    if (ligar(c, "/models", "GET", cab, NULL, 0, &con, &u, erro, max_erro) < 0) return -1;
    http_resposta_t resp;
    int r = http_ler_cabecalho_de(&resp, conexao_ler, &con);
    if (r < 0) {
        conexao_fechar(&con);
        formatar_texto(erro, (size_t)max_erro, "resposta invalida do servidor");
        return -1;
    }
    static char corpo[16384];
    int n = 0;
    while (n < (int)sizeof corpo - 1) {
        int k = http_ler_corpo(&resp, corpo + n, (int)sizeof corpo - 1 - n);
        if (k <= 0) break;
        n += k;
    }
    corpo[n] = 0;
    conexao_fechar(&con);
    if (resp.codigo < 200 || resp.codigo > 299) {
        const char *e = achar_campo(corpo, "message");
        char msg[200];
        if (e) ler_string(e, msg, sizeof msg);
        else formatar_texto(msg, sizeof msg, "HTTP %d", resp.codigo);
        formatar_texto(erro, (size_t)max_erro, "%s", msg);
        return -1;
    }
    /* "data": [ { "id": "modelo", ... }, ... ] */
    const char *dados = achar_campo(corpo, "data");
    if (!dados) { formatar_texto(erro, (size_t)max_erro, "resposta sem lista de modelos"); return -1; }
    int qtd = 0;
    const char *p = dados;
    while (qtd < max) {
        const char *id = achar_campo(p, "id");
        if (!id) break;
        char nome[64];
        if (ler_string(id, nome, sizeof nome) >= 0) { copia_texto(nomes[qtd], nome, 64); qtd++; }
        const char *fim = id;
        while (*fim && *fim != '}') fim++;
        if (!*fim) break;
        p = fim + 1;
    }
    return qtd;
}

/* --------------------------------------------------------------- chat */

/* Lee la respuesta linea a linea: cada "data: {...}" es un pedazo. Usa
 * http_ler_corpo (que descodifica el "chunked" si el servidor lo usa) y junta
 * las lineas que lleguen partidas. */
static int chat_corpo(const ia_config_t *c, const char *corpo, int ncorpo,
                      void (*ao_texto)(const char *t, int n, void *ctx), void *ctx,
                      void (*ao_fim)(int completa, void *ctx), void *ctx_fim,
                      int (*deve_parar)(void *ctx), void *ctx_parar,
                      char *erro, int max_erro);

static int chat_impl(const ia_config_t *c, const ia_mensagem_t *msgs, int nmsgs,
                     void (*ao_texto)(const char *t, int n, void *ctx), void *ctx,
                     void (*ao_fim)(int completa, void *ctx), void *ctx_fim,
                     int (*deve_parar)(void *ctx), void *ctx_parar,
                     char *erro, int max_erro)
{
    if (!c->url[0]) { formatar_texto(erro, (size_t)max_erro, "sem endereco: defina IA_URL"); return -1; }
    static char corpo[65536];
    int ncorpo = corpo_conversa(c, msgs, nmsgs, corpo, sizeof corpo);
    if (ncorpo <= 0 || ncorpo >= (int)sizeof corpo) {
        formatar_texto(erro, (size_t)max_erro, "conversa demasiado longa");
        return -1;
    }
    return chat_corpo(c, corpo, ncorpo, ao_texto, ctx, ao_fim, ctx_fim, deve_parar, ctx_parar, erro, max_erro);
}

/* La parte de red: manda el cuerpo ya armado y lee la respuesta. */
static int chat_corpo(const ia_config_t *c, const char *corpo, int ncorpo,
                      void (*ao_texto)(const char *t, int n, void *ctx), void *ctx,
                      void (*ao_fim)(int completa, void *ctx), void *ctx_fim,
                      int (*deve_parar)(void *ctx), void *ctx_parar,
                      char *erro, int max_erro)
{
    conexao_t con;
    url_t u;
    char cab[512];
    cabecalhos(c, "application/json", ncorpo, cab, sizeof cab);
    if (ligar(c, "/chat/completions", "POST", cab, corpo, ncorpo, &con, &u, erro, max_erro) < 0) return -1;
    http_resposta_t resp;
    int r = http_ler_cabecalho_de(&resp, conexao_ler, &con);
    if (r < 0) {
        conexao_fechar(&con);
        formatar_texto(erro, (size_t)max_erro, "resposta invalida do servidor");
        return -1;
    }
    if (resp.codigo < 200 || resp.codigo > 299) {
        static char txt[4096];
        int n = 0;
        while (n < (int)sizeof txt - 1) {
            int k = http_ler_corpo(&resp, txt + n, (int)sizeof txt - 1 - n);
            if (k <= 0) break;
            n += k;
        }
        txt[n] = 0;
        conexao_fechar(&con);
        const char *e = achar_campo(txt, "message");
        char msg[200];
        if (e) ler_string(e, msg, sizeof msg);
        else formatar_texto(msg, sizeof msg, "HTTP %d", resp.codigo);
        formatar_texto(erro, (size_t)max_erro, "%s", msg);
        return -1;
    }
    /* Lee la respuesta linea a linea: cada "data: {...}" es un pedazo. */
    static char pendente[8192];
    static char txt[4096];
    int npend = 0;
    int completa = 1;
    for (;;) {
        /* Antes de leer, ve si el usuario pidió parar (el botón
         * "Detener" o el Ctrl+C del terminal). */
        if (deve_parar && deve_parar(ctx_parar)) { completa = 0; break; }
        int k = http_ler_corpo(&resp, pendente + npend, (int)sizeof pendente - npend - 1);
        if (k < 0) { completa = 0; break; }
        if (k == 0) break;
        npend += k;
        pendente[npend] = 0;
        /* Una linea de cada vez: el SSE manda "data: {...}" y una linea
         * vazia entre eventos. */
        int ini = 0;
        for (;;) {
            char *nl = acha_char(pendente + ini, '\n');
            if (!nl) break;
            int nlinha = (int)(nl - (pendente + ini));
            char ln[4096];
            int cop = nlinha < (int)sizeof ln - 1 ? nlinha : (int)sizeof ln - 1;
            memcpy(ln, pendente + ini, (size_t)cop);
            ln[cop] = 0;
            if (cop && ln[cop - 1] == '\r') ln[cop - 1] = 0;
            ini += nlinha + 1;
            if (cop == 0) continue;                      /* linha vazia: separator */
            if (!comeca_com(ln, "data:")) continue;
            const char *d = ln + 5;
            while (*d == ' ') d++;
            if (compara_texto(d, "[DONE]") == 0) { npend = 0; goto fim; }
            const char *t = texto_do_chunk(d);
            if (!t) continue;
            int nt = ler_string(t, txt, sizeof txt);
            if (nt > 0 && ao_texto) ao_texto(txt, nt, ctx);
        }
        /* Lo que sobro (linea a medias) queda para la proxima lectura. */
        if (ini > 0) {
            memmove(pendente, pendente + ini, (size_t)(npend - ini));
            npend -= ini;
            pendente[npend] = 0;
        }
        if (npend >= (int)sizeof pendente - 1) npend = 0;   /* linha demasiado longa */
    }
fim:
    conexao_fechar(&con);
    if (ao_fim) ao_fim(completa, ctx_fim);
    return 0;
}

int ia_chat_conversa(const ia_config_t *c, const ia_mensagem_t *msgs, int nmsgs,
                     void (*ao_texto)(const char *t, int n, void *ctx), void *ctx,
                     void (*ao_fim)(int completa, void *ctx), void *ctx_fim,
                     int (*deve_parar)(void *ctx), void *ctx_parar,
                     char *erro, int max_erro)
{
    return chat_impl(c, msgs, nmsgs, ao_texto, ctx, ao_fim, ctx_fim, deve_parar, ctx_parar, erro, max_erro);
}

int ia_chat(const ia_config_t *c, const char *pergunta,
            void (*ao_texto)(const char *t, int n, void *ctx), void *ctx,
            void (*ao_fim)(int completa, void *ctx), void *ctx_fim,
            int (*deve_parar)(void *ctx), void *ctx_parar,
            char *erro, int max_erro)
{
    ia_mensagem_t m;
    memset(&m, 0, sizeof m);
    copia_texto(m.papel, "user", sizeof m.papel);
    copia_texto(m.texto, pergunta, sizeof m.texto);
    return chat_impl(c, &m, 1, ao_texto, ctx, ao_fim, ctx_fim, deve_parar, ctx_parar, erro, max_erro);
}

/* --------------------------------------------------------------- D239 */

/* Un texto Latin-1 como string JSON en UTF-8, directamente (sin el búfer
 * intermedio de IA_MSG_MAX). Cada byte ocupa como mucho 6 ("\u00XX"). */
static int json_latin1(char *dst, const char *s)
{
    int k = 0;
    dst[k++] = '"';
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') { dst[k++] = '\\'; dst[k++] = (char)c; }
        else if (c == '\n') { dst[k++] = '\\'; dst[k++] = 'n'; }
        else if (c == '\r') { dst[k++] = '\\'; dst[k++] = 'r'; }
        else if (c == '\t') { dst[k++] = '\\'; dst[k++] = 't'; }
        else if (c < 0x20) k += formatar_texto(dst + k, 8, "\\u%04x", c);
        else if (c >= 0x80) { dst[k++] = (char)(0xC0 | (c >> 6)); dst[k++] = (char)(0x80 | (c & 0x3F)); }
        else dst[k++] = (char)c;
    }
    dst[k++] = '"';
    dst[k] = 0;
    return k;
}

int ia_chat_conversa_ref(const ia_config_t *c, const ia_mensagem_ref_t *msgs, int nmsgs,
                         void (*ao_texto)(const char *t, int n, void *ctx), void *ctx,
                         void (*ao_fim)(int completa, void *ctx), void *ctx_fim,
                         int (*deve_parar)(void *ctx), void *ctx_parar,
                         char *erro, int max_erro)
{
    if (!c->url[0]) { formatar_texto(erro, (size_t)max_erro, "sem endereco: defina IA_URL"); return -1; }
    const char *modelo = c->modelo[0] ? c->modelo : "gpt-3.5-turbo";
    /* El tamaño del peor caso: 6 bytes por carácter y lo fijo de cada mensaje. */
    unsigned max = 96 + 6 * (unsigned)tam_texto(modelo) + 6 * (unsigned)tam_texto(c->sistema) + 48;
    for (int i = 0; i < nmsgs; i++)
        max += 48 + 6 * (unsigned)tam_texto(msgs[i].texto ? msgs[i].texto : "") + 16;
    if (max > IA_CORPO_MAX) {
        formatar_texto(erro, (size_t)max_erro, "conversa demasiado longa");
        return -1;
    }
    char *corpo = aloca(max);
    if (!corpo) { formatar_texto(erro, (size_t)max_erro, "sem memoria para o pedido"); return -1; }
    int n = formatar_texto(corpo, max, "{\"model\":");
    n += json_latin1(corpo + n, modelo);
    n += formatar_texto(corpo + n, max - (unsigned)n, ",\"messages\":[");
    bool primeira = true;
    if (c->sistema[0]) {
        n += formatar_texto(corpo + n, max - (unsigned)n, "{\"role\":\"system\",\"content\":");
        n += json_latin1(corpo + n, c->sistema);
        corpo[n++] = '}';
        primeira = false;
    }
    for (int i = 0; i < nmsgs; i++) {
        const char *t = msgs[i].texto;
        if (!t || !t[0]) continue;
        const char *papel = msgs[i].papel && msgs[i].papel[0] ? msgs[i].papel : "user";
        if (compara_texto(papel, "user") && compara_texto(papel, "assistant") && compara_texto(papel, "system"))
            papel = "user";
        if (!primeira) corpo[n++] = ',';
        n += formatar_texto(corpo + n, max - (unsigned)n, "{\"role\":\"%s\",\"content\":", papel);
        n += json_latin1(corpo + n, t);
        corpo[n++] = '}';
        primeira = false;
    }
    n += formatar_texto(corpo + n, max - (unsigned)n, "],\"stream\":true}");
    int r = chat_corpo(c, corpo, n, ao_texto, ctx, ao_fim, ctx_fim, deve_parar, ctx_parar, erro, max_erro);
    libera(corpo);
    return r;
}

/* ============================================================ D295: herramientas */

int ia_json_latin1(char *dst, const char *s) { return json_latin1(dst, s); }

/* UTF-8 -> Latin-1 (lo que no cabe, «?»). Devuelve los bytes escritos. */
int ia_utf8_latin1(const char *s, int n, char *dst, int max)
{
    int k = 0;
    for (int i = 0; i < n && k < max - 1; ) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) { dst[k++] = (char)c; i++; }
        else if ((c & 0xE0) == 0xC0 && i + 1 < n) {
            unsigned cp = ((c & 0x1Fu) << 6) | ((unsigned char)s[i + 1] & 0x3Fu);
            /* Revisión de seguridad (D295): C0 80 y demás formas largas de ASCII no valen (C0 80 sería un NUL). */
            dst[k++] = cp >= 0x80 && cp < 0x100 ? (char)cp : '?';
            i += 2;
        } else if ((c & 0xF0) == 0xE0) { dst[k++] = '?'; i += 3; }
        else if ((c & 0xF8) == 0xF0) { dst[k++] = '?'; i += 4; }
        else { dst[k++] = (char)c; i++; }      /* ya era Latin-1 suelto */
    }
    dst[k] = 0;
    return k;
}

void ia_turno_liberar(ia_turno_t *t)
{
    if (t->texto) libera(t->texto);
    for (int i = 0; i < IA_CHAMADAS_MAX; i++) if (t->ch[i].args) libera(t->ch[i].args);
    memset(t, 0, sizeof *t);
}

/* Añade bytes a un búfer dinámico terminado en cero. */
static bool juntar(char **b, int *n, int *cap, const char *d, int k)
{
    if (k <= 0) return true;
    if (*n + k + 1 > *cap) {
        int nc = *cap ? *cap : 256;
        while (nc < *n + k + 1) nc *= 2;
        char *nb = realoca(*b, (size_t)nc);
        if (!nb) return false;
        *b = nb;
        *cap = nc;
    }
    memcpy(*b + *n, d, (size_t)k);
    *n += k;
    (*b)[*n] = 0;
    return true;
}

/* La cadena JSON del token i, descodificada (UTF-8), en memoria dinámica. */
static char *cadeia_dyn(const char *t, const json_tok_t *tk, int i, int *n)
{
    char *d = aloca((size_t)(tk[i].fim - tk[i].ini) + 8);
    if (!d) return NULL;
    *n = json_texto(t, tk, i, d, tk[i].fim - tk[i].ini + 8);
    return d;
}

int ia_turno_linha(ia_turno_t *res, const char *json, int n,
                   void (*ao_texto)(const char *t, int n, void *ctx), void *ctx)
{
    while (n > 0 && (*json == ' ' || *json == '\n' || *json == '\r' || *json == '\t')) { json++; n--; }
    if (n >= 6 && memcmp(json, "[DONE]", 6) == 0) return 1;
    static json_tok_t tk[640];
    int nt = json_analisar(json, n, tk, 640);
    if (nt < 1) return 0;
    int ch = json_campo(json, tk, 0, "choices");
    int c0 = json_elem(tk, ch, 0);
    if (c0 < 0) return 0;
    int obj = json_campo(json, tk, c0, "delta");
    if (obj < 0) obj = json_campo(json, tk, c0, "message");
    int fr = json_campo(json, tk, c0, "finish_reason");
    if (fr >= 0 && tk[fr].tipo == JSON_STR) json_texto(json, tk, fr, res->fim, sizeof res->fim);
    if (obj < 0) return 0;
    int ct = json_campo(json, tk, obj, "content");
    if (ct >= 0 && tk[ct].tipo == JSON_STR) {
        int nn;
        char *d = cadeia_dyn(json, tk, ct, &nn);
        if (d) {
            /* Revisión de seguridad (D295): un servidor que no para no agota la memoria. */
            if (nn > 0 && res->ntexto + nn <= IA_TURNO_TEXTO_MAX) {
                juntar(&res->texto, &res->ntexto, &res->captexto, d, nn);
                if (ao_texto) ao_texto(d, nn, ctx);
            }
            libera(d);
        }
    }
    int tc = json_campo(json, tk, obj, "tool_calls");
    if (tc >= 0 && tk[tc].tipo == JSON_ARR) {
        for (int k = 0; k < tk[tc].n; k++) {
            int e = json_elem(tk, tc, k);
            if (e < 0 || tk[e].tipo != JSON_OBJ) continue;
            int idx = json_int(json, tk, json_campo(json, tk, e, "index"), -1);
            if (idx < 0) idx = k;
            char id[64] = "";
            int ii = json_campo(json, tk, e, "id");
            if (ii >= 0 && tk[ii].tipo == JSON_STR) json_texto(json, tk, ii, id, sizeof id);
            /* Algunos servidores repiten el índice 0 en cada llamada: una «id» nueva abre otra. */
            if (idx < IA_CHAMADAS_MAX && id[0] && res->ch[idx].id[0] && compara_texto(res->ch[idx].id, id) != 0 && res->n < IA_CHAMADAS_MAX)
                idx = res->n;
            if (idx >= IA_CHAMADAS_MAX) continue;
            ia_chamada_t *c = &res->ch[idx];
            if (idx + 1 > res->n) res->n = idx + 1;
            if (id[0] && !c->id[0]) copia_texto(c->id, id, sizeof c->id);
            int fn = json_campo(json, tk, e, "function");
            if (fn < 0) continue;
            int nm = json_campo(json, tk, fn, "name");
            if (nm >= 0 && tk[nm].tipo == JSON_STR && !c->nome[0]) json_texto(json, tk, nm, c->nome, sizeof c->nome);
            int ar = json_campo(json, tk, fn, "arguments");
            if (ar >= 0 && tk[ar].tipo == JSON_STR) {
                int nn;
                char *d = cadeia_dyn(json, tk, ar, &nn);
                /* Unos argumentos que pasan del tope se quedan cortados: no son JSON válido y se rechazan. */
                if (d) { if (c->nargs + nn <= IA_ARGS_MAX) juntar(&c->args, &c->nargs, &c->cap, d, nn); else c->cortados = true; libera(d); }
            } else if (ar >= 0 && (tk[ar].tipo == JSON_OBJ || tk[ar].tipo == JSON_ARR)) {
                /* Ollama nativo manda el objeto ya hecho. */
                if (c->nargs + (tk[ar].fim - tk[ar].ini) <= IA_ARGS_MAX) juntar(&c->args, &c->nargs, &c->cap, json + tk[ar].ini, tk[ar].fim - tk[ar].ini);
                else c->cortados = true;
            }
        }
    }
    return 0;
}

static int ultimo_http;

/* Una petición de turno. Devuelve 0, o <0 con el mensaje en «erro» (y ultimo_http con el código). */
static int turno_http(const ia_config_t *c, const char *corpo, int ncorpo,
                      void (*ao_texto)(const char *t, int n, void *ctx), void *ctx,
                      int (*deve_parar)(void *ctx), void *ctx_parar,
                      ia_turno_t *res, char *erro, int max_erro)
{
    conexao_t con;
    url_t u;
    char cab[512];
    ultimo_http = 0;
    cabecalhos(c, "application/json", ncorpo, cab, sizeof cab);
    if (ligar(c, "/chat/completions", "POST", cab, corpo, ncorpo, &con, &u, erro, max_erro) < 0) return -1;
    http_resposta_t resp;
    if (http_ler_cabecalho_de(&resp, conexao_ler, &con) < 0) {
        conexao_fechar(&con);
        formatar_texto(erro, (size_t)max_erro, "resposta invalida do servidor");
        return -1;
    }
    ultimo_http = resp.codigo;
    if (resp.codigo < 200 || resp.codigo > 299) {
        char *txt = aloca(4096);
        int n = 0;
        while (txt && n < 4095) {
            int k = http_ler_corpo(&resp, txt + n, 4095 - n);
            if (k <= 0) break;
            n += k;
        }
        if (txt) txt[n] = 0;
        conexao_fechar(&con);
        char msg[200];
        json_tok_t tk[64];
        msg[0] = 0;
        if (txt && json_analisar(txt, n, tk, 64) > 0) {
            int e = json_campo(txt, tk, 0, "error");
            int m = e >= 0 && tk[e].tipo == JSON_OBJ ? json_campo(txt, tk, e, "message") : e;
            if (m < 0) m = json_campo(txt, tk, 0, "message");
            if (m >= 0 && tk[m].tipo == JSON_STR) json_texto(txt, tk, m, msg, sizeof msg);
        }
        if (!msg[0]) formatar_texto(msg, sizeof msg, "HTTP %d", resp.codigo);
        if (txt) libera(txt);
        formatar_texto(erro, (size_t)max_erro, "%s", msg);
        return -1;
    }
    enum { LINHA_MAX = 256 * 1024 };
    char rd[2048];
    char *linha = NULL, *cru = NULL;
    int nl = 0, cl = 0, ncru = 0, ccru = 0;
    bool descarta = false, feito = false;
    res->completa = true;
    while (!feito) {
        if (deve_parar && deve_parar(ctx_parar)) { res->completa = false; break; }
        int k = http_ler_corpo(&resp, rd, sizeof rd);
        if (k < 0) { res->completa = false; break; }
        bool fim = k == 0;
        for (int i = 0; i <= k && !feito; i++) {
            char ch = i < k ? rd[i] : (fim ? '\n' : 0);
            if (i == k && !fim) break;
            if (ch != '\n') {
                if (nl < LINHA_MAX) juntar(&linha, &nl, &cl, &ch, 1); else descarta = true;
                continue;
            }
            /* Línea completa. */
            if (nl && linha[nl - 1] == '\r') linha[--nl] = 0;
            if (!descarta && nl > 0) {
                if (comeca_com(linha, "data:")) {
                    const char *d = linha + 5;
                    int nd = nl - 5;
                    while (nd > 0 && *d == ' ') { d++; nd--; }
                    if (ia_turno_linha(res, d, nd, ao_texto, ctx)) feito = true;
                } else if (!comeca_com(linha, ":") && !comeca_com(linha, "event:") && !comeca_com(linha, "id:") && !comeca_com(linha, "retry:")) {
                    if (ncru < 512 * 1024) juntar(&cru, &ncru, &ccru, linha, nl);   /* sin streaming: cuerpo JSON */
                }
            }
            nl = 0;
            descarta = false;
        }
        if (fim) break;
    }
    conexao_fechar(&con);
    if (cru && ncru) ia_turno_linha(res, cru, ncru, ao_texto, ctx);
    if (linha) libera(linha);
    if (cru) libera(cru);
    return 0;
}

static bool erro_de_ferramentas(const char *e)
{
    static const char *const claves[] = { "tool", "jinja", "function", "Tool", "TOOL", 0 };
    for (int i = 0; claves[i]; i++)
        for (const char *p = e; *p; p++) if (comeca_com(p, claves[i])) return true;
    return false;
}

int ia_turno(const ia_config_t *c, const char *sistema, const char *const *msgs_json, int nmsgs,
             const char *tools_json,
             void (*ao_texto)(const char *t, int n, void *ctx), void *ctx,
             int (*deve_parar)(void *ctx), void *ctx_parar,
             ia_turno_t *res, char *erro, int max_erro)
{
    if (!c->url[0]) { formatar_texto(erro, (size_t)max_erro, "sem endereco: defina IA_URL"); return -1; }
    const char *modelo = c->modelo[0] ? c->modelo : "gpt-3.5-turbo";
    unsigned max = 256 + 6 * (unsigned)tam_texto(modelo) + (sistema ? 6 * (unsigned)tam_texto(sistema) + 48 : 0) +
                   (tools_json ? (unsigned)tam_texto(tools_json) : 0);
    for (int i = 0; i < nmsgs; i++) max += (unsigned)tam_texto(msgs_json[i]) + 2;
    if (max > IA_CORPO_MAX) { formatar_texto(erro, (size_t)max_erro, "conversa demasiado longa"); return -1; }
    char *corpo = aloca(max);
    if (!corpo) { formatar_texto(erro, (size_t)max_erro, "sem memoria para o pedido"); return -1; }
    int r = -1;
    for (int intento = 0; intento < 2; intento++) {
        const char *tools = intento == 0 ? tools_json : NULL;
        int n = formatar_texto(corpo, max, "{\"model\":");
        n += json_latin1(corpo + n, modelo);
        n += formatar_texto(corpo + n, max - (unsigned)n, ",\"messages\":[");
        bool primeira = true;
        if (sistema && sistema[0]) {
            n += formatar_texto(corpo + n, max - (unsigned)n, "{\"role\":\"system\",\"content\":");
            n += json_latin1(corpo + n, sistema);
            corpo[n++] = '}';
            primeira = false;
        }
        for (int i = 0; i < nmsgs; i++) {
            if (!primeira) corpo[n++] = ',';
            int l = (int)tam_texto(msgs_json[i]);
            memcpy(corpo + n, msgs_json[i], (size_t)l);
            n += l;
            primeira = false;
        }
        corpo[n++] = ']';
        if (tools && tools[0]) n += formatar_texto(corpo + n, max - (unsigned)n, ",\"tools\":%s,\"tool_choice\":\"auto\"", tools);
        n += formatar_texto(corpo + n, max - (unsigned)n, ",\"stream\":true}");
        r = turno_http(c, corpo, n, ao_texto, ctx, deve_parar, ctx_parar, res, erro, max_erro);
        if (r == 0) break;
        if (intento == 0 && tools && tools[0] && ultimo_http >= 400 && erro_de_ferramentas(erro)) { res->sem_ferramentas = true; continue; }
        break;
    }
    libera(corpo);
    return r;
}

int ia_http_obter(const char *url, char *dst, int max, char *erro, int max_erro)
{
    ia_config_t c;
    ia_config_inicial(&c);
    if (tam_texto(url) >= sizeof c.url) { formatar_texto(erro, (size_t)max_erro, "endereco demasiado longo"); return -1; }
    copia_texto(c.url, url, sizeof c.url);
    conexao_t con;
    url_t u;
    char cab[128];
    formatar_texto(cab, sizeof cab, "User-Agent: SaguiOS-agente\r\nAccept: text/html,*/*\r\n");
    if (ligar(&c, "", "GET", cab, NULL, 0, &con, &u, erro, max_erro) < 0) return -1;
    http_resposta_t resp;
    if (http_ler_cabecalho_de(&resp, conexao_ler, &con) < 0) {
        conexao_fechar(&con);
        formatar_texto(erro, (size_t)max_erro, "resposta invalida do servidor");
        return -1;
    }
    int n = 0;
    while (n < max - 1) {
        int k = http_ler_corpo(&resp, dst + n, max - 1 - n);
        if (k <= 0) break;
        n += k;
    }
    dst[n] = 0;
    conexao_fechar(&con);
    if (resp.codigo < 200 || resp.codigo > 299) { formatar_texto(erro, (size_t)max_erro, "HTTP %d", resp.codigo); return -1; }
    return n;
}
