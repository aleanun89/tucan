// SPDX-License-Identifier: GPL-3.0-only
/* sonidos.c - esquemas y eventos de los sonidos del sistema (D421). Ver userland/inc/sonidos.h. */
#include "sagui.h"
#include "sonidos.h"

static const char *const ev_id[SON_EV_N] = {
    "arranque", "apagar", "reiniciar", "aviso", "error", "pregunta", "usb_conectar", "usb_desconectar", "bateria", "papelera",
};
/* Con los sonidos activados suenan todos menos el arranque, el apagado y el reinicio (esos retrasan o molestan: se piden). */
static const unsigned char ev_defecto[SON_EV_N] = { 0, 0, 0, 1, 1, 1, 1, 1, 1, 1 };

const char *sonidos_evento_id(int i) { return i >= 0 && i < SON_EV_N ? ev_id[i] : NULL; }
int sonidos_evento_defecto(int i) { return i >= 0 && i < SON_EV_N ? ev_defecto[i] : 0; }

int sonidos_evento_indice(const char *id)
{
    for (int i = 0; i < SON_EV_N; i++)
        if (compara_texto(id, ev_id[i]) == 0) return i;
    return -1;
}

void sonidos_conf_defecto(sonidos_conf_t *c)
{
    memset(c, 0, sizeof *c);
    c->volumen = 70;
    copia_texto(c->esquema, "saguios", sizeof c->esquema);
    for (int i = 0; i < SON_EV_N; i++) c->ev[i] = ev_defecto[i];
}

/* Lee un archivo pequeño entero; < 0 si no existe. */
static int ler_pequeno(const char *arq, char *t, int max)
{
    int fd = abrir(arq, ABRIR_LER);
    if (fd < 0) return -1;
    int n = ler(fd, t, (unsigned)(max - 1));
    fechar(fd);
    if (n < 0) return -1;
    t[n] = 0;
    return n;
}

/* Recorre las líneas clave=valor de t (se modifica); llama a cb(clave, valor, dato). */
static void por_linhas(char *t, void (*cb)(const char *, const char *, void *), void *dato)
{
    for (char *p = t; *p;) {
        char *fim = acha_char(p, '\n');
        if (fim) *fim = 0;
        char *eq = acha_char(p, '=');
        if (p[0] != '#' && eq) {
            *eq = 0;
            size_t tv = tam_texto(eq + 1);
            while (tv && (eq[tv] == '\r' || eq[tv] == ' ')) eq[tv--] = 0;
            cb(p, eq + 1, dato);
        }
        if (!fim) break;
        p = fim + 1;
    }
}

static void cb_conf(const char *k, const char *v, void *d)
{
    sonidos_conf_t *c = d;
    if (compara_texto(k, "activo") == 0) c->activo = texto_para_int(v) != 0;
    else if (compara_texto(k, "volumen") == 0) { int x = texto_para_int(v); c->volumen = x < 0 ? 0 : x > 100 ? 100 : x; }
    else if (compara_texto(k, "esquema") == 0) copia_texto(c->esquema, v, sizeof c->esquema);
    else if (comeca_com(k, "ev_")) {
        int i = sonidos_evento_indice(k + 3);
        if (i >= 0) c->ev[i] = texto_para_int(v) != 0;
    }
}

int sonidos_conf_ler(sonidos_conf_t *c)
{
    static char t[1024];
    sonidos_conf_defecto(c);
    if (ler_pequeno(SON_ARQ_CONF, t, sizeof t) < 0) return 1;
    por_linhas(t, cb_conf, c);
    return 0;
}

int sonidos_conf_gravar(const sonidos_conf_t *c)
{
    char o[512];
    int n = formatar_texto(o, sizeof o, "activo=%d\nvolumen=%d\nesquema=%s\n", c->activo ? 1 : 0, c->volumen, c->esquema);
    for (int i = 0; i < SON_EV_N; i++) n += formatar_texto(o + n, sizeof o - (unsigned)n, "ev_%s=%d\n", ev_id[i], c->ev[i] ? 1 : 0);
    criar_pasta("/home/.config");
    int fd = abrir(SON_ARQ_CONF, ABRIR_ESCREVER | ABRIR_CRIAR | ABRIR_TRUNCAR);
    if (fd < 0) return fd;
    int r = escrever_tudo(fd, o, (unsigned)n);
    fechar(fd);
    return r < 0 ? r : 0;
}

/* El id de esquema solo lleva letras, cifras, «-» y «_» (va dentro de una ruta). */
static bool id_valido(const char *s)
{
    if (!s[0] || tam_texto(s) > 31) return false;
    for (; *s; s++)
        if (!((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') || (*s >= '0' && *s <= '9') || *s == '-' || *s == '_')) return false;
    return true;
}

typedef struct { const char *evento; char *dst; int max; int hallado; sonidos_esquema_t *e; } busca_t;

static void cb_esquema(const char *k, const char *v, void *d)
{
    busca_t *b = d;
    if (b->e) {
        if (compara_texto(k, "nombre") == 0) copia_texto(b->e->nombre, v, sizeof b->e->nombre);
        else if (compara_texto(k, "creditos") == 0) copia_texto(b->e->creditos, v, sizeof b->e->creditos);
    }
    if (b->evento && compara_texto(k, b->evento) == 0 && v[0] && acha_char(v, '/') == NULL) {
        copia_texto(b->dst, v, (size_t)b->max);
        b->hallado = 1;
    }
}

static bool carpeta_esquema(const char *id, char *dir, int max)
{
    info_arquivo_t inf;
    formatar_texto(dir, (size_t)max, "%s/%s", SON_DIR_SIS, id);
    if (info_arquivo(dir, &inf) == 0) return true;
    formatar_texto(dir, (size_t)max, "%s/%s", SON_DIR_PRIV, id);
    return info_arquivo(dir, &inf) == 0;
}

int sonidos_ruta(const char *esquema, const char *evento, char *dst, int max)
{
    static char t[1024];
    char dir[96], arq[96], nombre[64];
    if (!id_valido(esquema) || compara_texto(esquema, "ninguno") == 0) return ERRO_NAO_EXISTE;
    if (!carpeta_esquema(esquema, dir, sizeof dir)) return ERRO_NAO_EXISTE;
    formatar_texto(arq, sizeof arq, "%s/esquema.conf", dir);
    if (ler_pequeno(arq, t, sizeof t) < 0) return ERRO_NAO_EXISTE;
    busca_t b = { evento, nombre, sizeof nombre, 0, NULL };
    por_linhas(t, cb_esquema, &b);
    if (!b.hallado) return ERRO_NAO_EXISTE;
    formatar_texto(dst, (size_t)max, "%s/%s", dir, nombre);
    return 0;
}

static int esquemas_de(const char *base, bool privado, sonidos_esquema_t *v, int n, int max)
{
    static char t[1024];
    entrada_pasta_t e;
    for (int i = 0; n < max && listar(base, i, &e) > 0; i++) {
        char arq[128];
        if (e.tipo != TIPO_PASTA || !id_valido(e.nome) || compara_texto(e.nome, "ninguno") == 0) continue;
        bool dup = false;
        for (int k = 0; k < n; k++) if (compara_texto(v[k].id, e.nome) == 0) dup = true;
        if (dup) continue;
        formatar_texto(arq, sizeof arq, "%s/%s/esquema.conf", base, e.nome);
        if (ler_pequeno(arq, t, sizeof t) < 0) continue;
        sonidos_esquema_t *s = &v[n];
        memset(s, 0, sizeof *s);
        copia_texto(s->id, e.nome, sizeof s->id);
        copia_texto(s->nombre, e.nome, sizeof s->nombre);
        s->privado = privado;
        busca_t b = { NULL, NULL, 0, 0, s };
        por_linhas(t, cb_esquema, &b);
        n++;
    }
    return n;
}

int sonidos_esquemas(sonidos_esquema_t *v, int max)
{
    int n = esquemas_de(SON_DIR_SIS, false, v, 0, max);
    return esquemas_de(SON_DIR_PRIV, true, v, n, max);
}

static int lanzar(const char *evento, unsigned max_ms)
{
    sonidos_conf_t c;
    int i = sonidos_evento_indice(evento);
    if (i < 0) return 0;
    sonidos_conf_ler(&c);
    if (!c.activo || !c.ev[i] || !c.volumen || !c.esquema[0] || compara_texto(c.esquema, "ninguno") == 0) return 0;
    char ruta[128], ms[16];
    if (sonidos_ruta(c.esquema, evento, ruta, sizeof ruta) < 0) return 0;   /* el esquema no tiene sonido para este evento */
    int codigo;
    while (recolher(&codigo) > 0) { }   /* los sonidos anteriores ya terminados */
    formatar_texto(ms, sizeof ms, "%u", max_ms);
    char *av[] = { "sonidos", "--tocar", (char *)evento, "--max", ms, NULL };
    int pid = executar("/bin/sonidos", av);
    return pid > 0 ? pid : 0;
}

int sonido_evento(const char *evento) { return lanzar(evento, SON_ARRANQUE_MAX_MS); }

int sonido_evento_esperar(const char *evento, unsigned max_ms)
{
    int pid = lanzar(evento, max_ms);
    if (pid <= 0) return 0;
    unsigned t0 = tempo_ms();
    int estado;
    while (esperar_pid(pid, &estado, ESPERA_SEM_BLOQUEAR) == 0) {
        if (tempo_ms() - t0 > max_ms + 500) { matar(pid); esperar_pid(pid, &estado, 0); break; }
        dormir(20);
    }
    return pid;
}
