// SPDX-License-Identifier: GPL-2.0-or-later
/* contas.c - las cuentas de usuario: /etc/usuarios y la sombra (D255). Ver contas.h. */
#include "sagui.h"
#include "contas.h"

#define ARQ_MAX 16384   /* las dos tablas son pequeñas: más que esto es un archivo dañado */

/* Las cuentas de serie: valen siempre que el archivo falte o no las tenga. */
static const conta_t serie[] = {
    { "root", UID_ROOT, GID_ROOT, "/root", "/bin/sagsh", "Administrador" },
    { "sagui", UID_SAGUI, GID_SAGUI, "/home", "/bin/sagsh", "Usuario del escritorio" },
    { "servicio", 100, 100, "/", "", "Servicios sin escritorio" },
};
#define N_SERIE ((int)(sizeof serie / sizeof serie[0]))

/* Lee un archivo entero (terminado en cero). NULL si no existe o no se puede. */
static char *ler_tudo(const char *arq, int *erro)
{
    int fd = abrir(arq, ABRIR_LER);
    if (fd < 0) { if (erro) *erro = fd; return NULL; }
    char *b = aloca(ARQ_MAX + 1);
    if (!b) { fechar(fd); if (erro) *erro = ERRO_SEM_MEMORIA; return NULL; }
    int n = 0, r;
    while (n < ARQ_MAX && (r = ler(fd, b + n, (unsigned)(ARQ_MAX - n))) > 0) n += r;
    fechar(fd);
    b[n] = 0;
    if (erro) *erro = 0;
    return b;
}

/* Corta "s" en campos separados por ':' (sin copiar). Devuelve cuántos. */
static int campos(char *s, char **c, int max)
{
    int n = 0;
    c[n++] = s;
    for (; *s && n < max; s++)
        if (*s == ':') { *s = 0; c[n++] = s + 1; }
    return n;
}

static bool numero(const char *s, unsigned *v)
{
    unsigned x = 0;
    if (!*s) return false;
    for (; *s; s++) {
        if (*s < '0' || *s > '9' || x > UID_MAX) return false;
        x = x * 10 + (unsigned)(*s - '0');
    }
    if (x > UID_MAX) return false;
    *v = x;
    return true;
}

static bool nome_valido(const char *n)
{
    if (!*n || tam_texto(n) >= CONTA_NOME_MAX) return false;
    for (; *n; n++)
        if (!((*n >= 'a' && *n <= 'z') || (*n >= '0' && *n <= '9') || *n == '_' || *n == '-')) return false;
    return true;
}

static int achar_uid(const conta_t *v, int n, unsigned uid)
{
    for (int i = 0; i < n; i++) if (v[i].uid == uid) return i;
    return -1;
}

static int achar_nome(const conta_t *v, int n, const char *nome)
{
    for (int i = 0; i < n; i++) if (compara_texto(v[i].nome, nome) == 0) return i;
    return -1;
}

int contas_ler(conta_t *v, int max, bool *danado)
{
    int n = 0;
    bool mal = false;
    char *b = ler_tudo(CONTAS_ARQ_USUARIOS, NULL);
    if (!b) mal = true;
    for (char *l = b; l && *l && n < max;) {
        char *fim = l;
        while (*fim && *fim != '\n') fim++;
        char *prox = *fim ? fim + 1 : fim;
        *fim = 0;
        if (fim > l && fim[-1] == '\r') fim[-1] = 0;
        while (*l == ' ' || *l == '\t') l++;
        if (*l && *l != '#') {
            char *c[6];
            int k = campos(l, c, 6);
            conta_t x;
            memset(&x, 0, sizeof x);
            if (k >= 3 && nome_valido(c[0]) && numero(c[1], &x.uid) && numero(c[2], &x.gid) &&
                achar_uid(v, n, x.uid) < 0 && achar_nome(v, n, c[0]) < 0 &&
                /* root es el 0 y el 0 es root: nada de otro nombre con uid 0 */
                ((x.uid == 0) == (compara_texto(c[0], "root") == 0))) {
                copia_texto(x.nome, c[0], sizeof x.nome);
                copia_texto(x.casa, k > 3 && c[3][0] ? c[3] : "/home", sizeof x.casa);
                if (x.uid == UID_ROOT) copia_texto(x.casa, "/root", sizeof x.casa);   /* D371: siempre /root */
                copia_texto(x.shell, k > 4 ? c[4] : "", sizeof x.shell);
                copia_texto(x.descricao, k > 5 ? c[5] : "", sizeof x.descricao);
                v[n++] = x;
            } else {
                mal = true;
            }
        }
        l = prox;
    }
    if (b) libera(b);
    /* Las de serie que falten (por nombre y por uid). */
    for (int i = 0; i < N_SERIE && n < max; i++)
        if (achar_nome(v, n, serie[i].nome) < 0 && achar_uid(v, n, serie[i].uid) < 0) {
            v[n++] = serie[i];
            if (i < 2) mal = true;   /* root o sagui faltaban: el archivo no está bien */
        }
    if (danado) *danado = mal;
    return n;
}

bool conta_por_uid(unsigned uid, conta_t *c)
{
    conta_t v[CONTAS_MAX];
    int n = contas_ler(v, CONTAS_MAX, NULL);
    int i = achar_uid(v, n, uid);
    if (i < 0) return false;
    *c = v[i];
    return true;
}

bool conta_por_nome(const char *nome, conta_t *c)
{
    conta_t v[CONTAS_MAX];
    int n = contas_ler(v, CONTAS_MAX, NULL);
    int i = achar_nome(v, n, nome);
    if (i < 0) return false;
    *c = v[i];
    return true;
}

const char *conta_nome(unsigned uid, char *buf, int max)
{
    conta_t c;
    if (conta_por_uid(uid, &c)) copia_texto(buf, c.nome, (size_t)max);
    else formatar_texto(buf, (size_t)max, "%u", uid);
    return buf;
}

/* ---------------------------------------------------------------- sombra */

int sombra_ler(const char *nome, char *hash, int max)
{
    int erro;
    char *b = ler_tudo(CONTAS_ARQ_SOMBRA, &erro);
    if (!b) return erro == ERRO_NAO_EXISTE ? 0 : erro;
    int achou = 0;
    int nn = (int)tam_texto(nome);
    for (char *l = b; *l;) {
        char *fim = l;
        while (*fim && *fim != '\n') fim++;
        char *prox = *fim ? fim + 1 : fim;
        *fim = 0;
        if (fim > l && fim[-1] == '\r') fim[-1] = 0;
        if (compara_texto_n(l, nome, (size_t)nn) == 0 && l[nn] == ':') {
            copia_texto(hash, l + nn + 1, (size_t)max);
            achou = 1;
            break;
        }
        l = prox;
    }
    memset(b, 0, ARQ_MAX);   /* no dejar hashes por la memoria */
    libera(b);
    return achou;
}

/* escrever_tudo de la libsagui devuelve los bytes escritos: aquí, 0 o el error. */
static int escrever_todo(int fd, const char *s, int n)
{
    int r = escrever_tudo(fd, s, (unsigned)n);
    return r == n ? 0 : r < 0 ? r : ERRO_LIMITE;
}

/* hash NULL = quitar la línea (D340). */
static int sombra_trocar(const char *nome, const char *hash)
{
    if (!nome_valido(nome) || (hash && (acha_char(hash, '\n') || acha_char(hash, ':')))) return ERRO_INVALIDO;
    int erro;
    char *b = ler_tudo(CONTAS_ARQ_SOMBRA, &erro);
    if (!b && erro != ERRO_NAO_EXISTE) return erro;
    char *novo = aloca(ARQ_MAX + SOMBRA_TEXTO_MAX + 64);
    if (!novo) { if (b) libera(b); return ERRO_SEM_MEMORIA; }
    int n = 0;
    int nn = (int)tam_texto(nome);
    bool posto = false;
    for (char *l = b; l && *l;) {
        char *fim = l;
        while (*fim && *fim != '\n') fim++;
        char *prox = *fim ? fim + 1 : fim;
        *fim = 0;
        if (compara_texto_n(l, nome, (size_t)nn) == 0 && l[nn] == ':') {
            if (!posto && hash) n += formatar_texto(novo + n, (size_t)(ARQ_MAX + SOMBRA_TEXTO_MAX - n), "%s:%s\n", nome, hash);
            posto = true;
        } else if (*l && n + (int)tam_texto(l) + 2 < ARQ_MAX) {
            n += formatar_texto(novo + n, (size_t)(ARQ_MAX + SOMBRA_TEXTO_MAX - n), "%s\n", l);
        }
        l = prox;
    }
    if (!posto && hash) n += formatar_texto(novo + n, (size_t)(ARQ_MAX + SOMBRA_TEXTO_MAX - n), "%s:%s\n", nome, hash);
    if (b) { memset(b, 0, ARQ_MAX); libera(b); }
    int r = conf_gravar("sombra", novo, n);
    memset(novo, 0, (size_t)n);
    libera(novo);
    return r;
}

int sombra_gravar(const char *nome, const char *hash) { return sombra_trocar(nome, hash); }
int sombra_remover(const char *nome) { return sombra_trocar(nome, NULL); }

/* ---------------------------------------------------------------- configuración del sistema */

bool conf_persistente(void)
{
    int e = usuario_op(USU_ESTADO, 0, 0);
    return e >= 0 && !(e & USU_EST_EN_VIVO);
}

/* Crea las carpetas que falten hasta la última "/" de "caminho". */
static void criar_pais(const char *caminho)
{
    char c[160];
    copia_texto(c, caminho, sizeof c);
    for (char *p = c + 1; *p; p++)
        if (*p == '/') { *p = 0; criar_pasta(c); *p = '/'; }
}

/* D282: lo que solo debe leer root (las contraseñas y las claves privadas) nace
 * ya con 0600; el resto, con la máscara de siempre (0644). */
static bool secreto(const char *rel)
{
    return compara_texto(rel, "sombra") == 0 || compara_texto(rel, "vnc/vncd.passwd") == 0 ||
           comeca_com(rel, "ssh/ssh_host_");
}

/* Escribe "arq" de una vez: temporal al lado y renombrar encima. */
static int gravar_atomico(const char *arq, const void *dados, int n, bool privado)
{
    char tmp[176];
    formatar_texto(tmp, sizeof tmp, "%s.tmp", arq);
    criar_pais(arq);
    apagar(tmp);   /* uno viejo podría tener otros permisos */
    int antes = privado ? mascara_criacao(077) : -1;
    int fd = abrir(tmp, ABRIR_ESCREVER | ABRIR_CRIAR | ABRIR_TRUNCAR);
    if (privado && antes >= 0) mascara_criacao(antes);
    if (fd < 0) return fd;
    if (privado) mudar_permisos(tmp, 0600);   /* por si el sistema de archivos no usa la máscara al crear */
    int r = escrever_todo(fd, (const char *)dados, n);
    if (r == 0) sincronizar(fd);
    fechar(fd);
    if (r >= 0) {
        r = renomear(tmp, arq);
        if (r == ERRO_JA_EXISTE) { apagar(arq); r = renomear(tmp, arq); }   /* por si el sistema de archivos no reemplaza */
    }
    if (r < 0) apagar(tmp);
    return r < 0 ? r : 0;
}

int conf_gravar(const char *rel, const void *dados, int n)
{
    if (!rel[0] || rel[0] == '/' || acha_char(rel, '.') == rel) return ERRO_INVALIDO;
    char ram[160], disco[176];
    formatar_texto(ram, sizeof ram, "/etc/%s", rel);
    formatar_texto(disco, sizeof disco, "%s/etc/%s", CONF_SISTEMA_PONTO, rel);
    int rd = 0;
    if (conf_persistente()) rd = gravar_atomico(disco, dados, n, secreto(rel));
    int rr = gravar_atomico(ram, dados, n, secreto(rel));
    return rd < 0 ? rd : rr;
}

int conf_apagar(const char *rel)
{
    char ram[160], disco[176];
    formatar_texto(ram, sizeof ram, "/etc/%s", rel);
    formatar_texto(disco, sizeof disco, "%s/etc/%s", CONF_SISTEMA_PONTO, rel);
    if (conf_persistente()) apagar(disco);
    return apagar(ram);
}

/* ---------------------------------------------------------------- identidad */

unsigned meu_uid(void) { return (unsigned)usuario_op(USU_CONSULTAR, 0, 0) & 0xFFFF; }
unsigned meu_gid(void) { return ((unsigned)usuario_op(USU_CONSULTAR, 0, 0) >> 16) & 0xFFFF; }
bool     sou_root(void) { return meu_uid() == UID_ROOT; }

bool falta_admin(const char *programa)
{
    if (sou_root()) return false;
    imprimir_em(ERROS, "%s: hace falta ser administrador: usa «admin %s ...» (o «su»)\n", programa, programa);
    return true;
}

/* ---------------------------------------------------------------- pedir al gestor de servicios */

int ler_senha(const char *pergunta, char *buf, int max)
{
    imprimir_em(ERROS, "%s", pergunta);
    int n = 0;
    if (tipo_descritor(ENTRADA) != TIPO_TERMINAL) {
        /* Entrada redirigida (una tubería, ssh sin terminal): una línea. */
        n = ler_linha(ENTRADA, buf, (unsigned)max);
        imprimir_em(ERROS, "\n");
        if (n < 0) { buf[0] = 0; return -1; }
        buf[max - 1] = 0;
        int t = (int)tam_texto(buf);
        while (t > 0 && (buf[t - 1] == '\r' || buf[t - 1] == '\n')) buf[--t] = 0;
        return t;
    }
    modo_terminal(TERM_BRUTO);   /* sin eco */
    for (;;) {
        unsigned char c;
        int r = ler(ENTRADA, &c, 1);
        if (r <= 0 || c == '\n' || c == '\r') break;
        if (c == 3) { n = -1; break; }                       /* Ctrl+C */
        if (c == 8 || c == 127) { if (n > 0) n--; continue; }
        if (c == 27 || c < 32) continue;
        if (n < max - 1) buf[n++] = (char)c;
    }
    modo_terminal(TERM_LINHA);
    imprimir_em(ERROS, "\n");
    if (n < 0) { memset(buf, 0, (size_t)max); return -1; }
    buf[n] = 0;
    return n;
}

int pedir_conta(pedido_conta_t *p)
{
    int id = servicos_kernel(SERV_PEDIR, (int)p);
    if (id < 0) return id;
    unsigned fim = tempo_ms() + 90000;   /* la comprobación tarda; en una máquina emulada, más */
    for (;;) {
        int r = servicos_kernel(SERV_RESULTADO, id);
        if (r != 0) return r;
        if ((int)(tempo_ms() - fim) >= 0) return ERRO_TEMPO;
        dormir(50);
    }
}

/* Pide la contraseña nueva dos veces. 0 = bien. */
static int senha_nova_dupla(char *nova, int max)
{
    char otra[PED_SENHA_MAX];
    for (int i = 0; i < 3; i++) {
        if (ler_senha("Contraseña nueva: ", nova, max) < 0) return -1;
        if ((int)tam_texto(nova) < 4) { imprimir_em(ERROS, "Demasiado corta (al menos 4 caracteres).\n"); continue; }
        if (ler_senha("Repítela: ", otra, sizeof otra) < 0) return -1;
        bool igual = compara_texto(nova, otra) == 0;
        memset(otra, 0, sizeof otra);
        if (igual) return 0;
        imprimir_em(ERROS, "No coinciden.\n");
    }
    return -1;
}

int elevar_interativo(unsigned uid_alvo, const char *programa)
{
    if (meu_uid() == uid_alvo) return 0;
    pedido_conta_t p;
    memset(&p, 0, sizeof p);
    p.tipo = PED_ELEVAR;
    p.uid_alvo = uid_alvo;
    char alvo[CONTA_NOME_MAX];
    conta_nome(uid_alvo, alvo, sizeof alvo);
    /* Primero sin contraseña: el supervisor dice si la cuenta tiene (sin contar como fallo). */
    int r = pedir_conta(&p);
    for (int intento = 0; r != 1 && intento < 3; intento++) {
        if (r == ERRO_SEM_SENHA) {
            if (usuario_op(USU_ESTADO, 0, 0) & USU_EST_SERVICIO) break;
            imprimir_em(ERROS, "%s aún no tiene contraseña. Elige una (se pedirá a partir de ahora):\n", alvo);
            if (senha_nova_dupla(p.senha_nova, sizeof p.senha_nova) < 0) { r = ERRO_INVALIDO; break; }
        } else if (r == ERRO_ACESSO) {
            char q[64], eu[CONTA_NOME_MAX];
            conta_nome(meu_uid(), eu, sizeof eu);
            /* D340: un administrador puede dar la suya. */
            if (uid_alvo == UID_ROOT && conta_admin(eu)) formatar_texto(q, sizeof q, "Contraseña (la tuya o la de %s): ", alvo);
            else formatar_texto(q, sizeof q, "Contraseña de %s: ", alvo);
            if (ler_senha(q, p.senha, sizeof p.senha) < 0) break;
        } else break;
        r = pedir_conta(&p);
        if (r == ERRO_ACESSO && p.senha[0]) imprimir_em(ERROS, "Contraseña incorrecta.\n");
    }
    memset(&p, 0, sizeof p);
    if (r == 1) return 0;
    if (r == ERRO_NAO_EXISTE)
        imprimir_em(ERROS, "%s: el gestor de servicios no está en marcha: no se puede comprobar la contraseña "
                    "(arranca en modo rescate: Ctrl al encender)\n", programa);
    else if (r == ERRO_NAO_SUPORTADO && uid_alvo == UID_ROOT)   /* D371 (P3 b): lo dice el kernel */
        imprimir_em(ERROS, "%s: la primera contraseña de root se elige en la pantalla del sistema (al arrancar) o en el "
                    "diálogo de permisos, no desde un programa\n", programa);
    else if (r != ERRO_INVALIDO && !(r == ERRO_ACESSO))
        imprimir_em(ERROS, "%s: %s\n", programa, nome_erro(r));
    else if (r == ERRO_ACESSO)
        imprimir_em(ERROS, "%s: no se concedió\n", programa);
    return r < 0 ? r : ERRO_ACESSO;
}

/* ---------------------------------------------------------------- diálogo de permisos (D269) */

static unsigned uid_antes_grafico = (unsigned)-1, gid_antes_grafico;

int elevar_grafico(const char *motivo) { return elevar_grafico_ex(motivo, 0); }

int elevar_grafico_ex(const char *motivo, unsigned opcoes)
{
    if (sou_root()) return 0;
    unsigned uid0 = meu_uid(), gid0 = meu_gid();
    pedido_conta_t p;
    memset(&p, 0, sizeof p);
    p.tipo = PED_ELEVAR | PED_GRAFICO | (opcoes & PED_SIN_RECORDAR);
    p.uid_alvo = UID_ROOT;
    if (motivo) copia_texto(p.motivo, motivo, sizeof p.motivo);
    int id = servicos_kernel(SERV_PEDIR, (int)&p);
    if (id < 0) return id;
    unsigned fim = tempo_ms() + 310000;   /* el diálogo caduca solo a los 5 min */
    for (;;) {
        int r = servicos_kernel(SERV_RESULTADO, id);
        if (r == 1) {
            if (uid_antes_grafico == (unsigned)-1) { uid_antes_grafico = uid0; gid_antes_grafico = gid0; }
            return 0;
        }
        if (r != 0) return r;
        if ((int)(tempo_ms() - fim) >= 0) return ERRO_TEMPO;
        dormir(30);
    }
}

void elevacao_soltar(void)
{
    if (uid_antes_grafico == (unsigned)-1 || !sou_root()) return;
    usuario_op(USU_FIJAR, (int)uid_antes_grafico, (int)gid_antes_grafico);
    uid_antes_grafico = (unsigned)-1;
}

/* ---------------------------------------------------------------- D340: altas, bajas y cambios */

bool conta_nome_valido(const char *nome) { return nome_valido(nome); }

int contas_resumo(conta_resumo_t *r, int max)
{
    conta_t v[CONTAS_MAX];
    int n = contas_ler(v, CONTAS_MAX, NULL), k = 0;
    for (int i = 0; i < n && k < max; i++) {
        if (v[i].uid < UID_SAGUI || v[i].uid > CONTA_UID_ULTIMO || !v[i].shell[0]) continue;
        memset(&r[k], 0, sizeof r[k]);
        copia_texto(r[k].nome, v[i].nome, sizeof r[k].nome);
        r[k].uid = v[i].uid;
        copia_texto(r[k].descricao, v[i].descricao, sizeof r[k].descricao);
        r[k].admin = conta_admin(v[i].nome);
        k++;
    }
    return k;
}

int contas_gravar(const conta_t *v, int n)
{
    char *b = aloca(ARQ_MAX);
    if (!b) return ERRO_SEM_MEMORIA;
    int k = formatar_texto(b, ARQ_MAX, "# Cuentas de SaguiOS (D255, D340): nombre:uid:gid:carpeta:shell:descripción\n");
    for (int i = 0; i < n && k < ARQ_MAX - 256; i++) {
        if (acha_char(v[i].descricao, ':') || acha_char(v[i].descricao, '\n')) { libera(b); return ERRO_INVALIDO; }
        k += formatar_texto(b + k, (size_t)(ARQ_MAX - k), "%s:%u:%u:%s:%s:%s\n", v[i].nome, v[i].uid, v[i].gid,
                            v[i].casa, v[i].shell, v[i].descricao);
    }
    int r = conf_gravar("usuarios", b, k);
    libera(b);
    return r;
}

/* /etc/grupos: la lista de miembros de "admin" (sin copiar: apunta dentro de "b"). */
static char *membros_admin(char *b)
{
    for (char *l = b; l && *l;) {
        char *fim = l;
        while (*fim && *fim != '\n') fim++;
        if (compara_texto_n(l, GRUPO_ADMIN ":", 6) == 0) {
            char *m = l + 6;
            while (m < fim && *m != ':') m++;
            return m < fim ? m + 1 : fim;
        }
        l = *fim ? fim + 1 : fim;
    }
    return NULL;
}

bool conta_admin(const char *nome)
{
    if (!nome_valido(nome)) return false;
    char *b = ler_tudo(CONTAS_ARQ_GRUPOS, NULL);
    if (!b) return false;
    bool sim = false;
    char *m = membros_admin(b);
    size_t nn = tam_texto(nome);
    while (m && *m && *m != '\n' && *m != '\r') {
        char *f = m;
        while (*f && *f != ',' && *f != '\n' && *f != '\r') f++;
        if ((size_t)(f - m) == nn && compara_texto_n(m, nome, nn) == 0) sim = true;
        m = *f == ',' ? f + 1 : f;
    }
    libera(b);
    return sim;
}

int conta_admin_fixar(const char *nome, bool sim)
{
    if (!nome_valido(nome)) return ERRO_INVALIDO;
    if (conta_admin(nome) == sim) return 0;
    char *b = ler_tudo(CONTAS_ARQ_GRUPOS, NULL);
    char *novo = aloca(ARQ_MAX + 64);
    if (!novo) { if (b) libera(b); return ERRO_SEM_MEMORIA; }
    int n = 0;
    bool tinha = false;
    for (char *l = b; l && *l;) {
        char *fim = l;
        while (*fim && *fim != '\n') fim++;
        char *prox = *fim ? fim + 1 : fim;
        *fim = 0;
        if (fim > l && fim[-1] == '\r') fim[-1] = 0;
        if (compara_texto_n(l, GRUPO_ADMIN ":", 6) == 0) {
            tinha = true;
            char *m = membros_admin(l) ? l + 6 : NULL;
            while (m && *m && *m != ':') m++;
            if (m && *m == ':') m++;
            n += formatar_texto(novo + n, (size_t)(ARQ_MAX - n), "%s:%u:", GRUPO_ADMIN, GID_ADMIN);
            bool prim = true;
            size_t nn = tam_texto(nome);
            while (m && *m) {
                char *f = m;
                while (*f && *f != ',') f++;
                bool este = (size_t)(f - m) == nn && compara_texto_n(m, nome, nn) == 0;
                if (f > m && !este && n + (f - m) + 2 < ARQ_MAX) {
                    if (!prim) novo[n++] = ',';
                    memcpy(novo + n, m, (size_t)(f - m));
                    n += (int)(f - m);
                    prim = false;
                }
                m = *f ? f + 1 : f;
            }
            if (sim) n += formatar_texto(novo + n, (size_t)(ARQ_MAX - n), "%s%s", prim ? "" : ",", nome);
            novo[n++] = '\n';
        } else if (*l && n + (int)tam_texto(l) + 2 < ARQ_MAX) {
            n += formatar_texto(novo + n, (size_t)(ARQ_MAX - n), "%s\n", l);
        }
        l = prox;
    }
    if (!tinha) {
        if (n == 0) n += formatar_texto(novo, ARQ_MAX, "# Grupos de SaguiOS (D340): nombre:gid:miembros\n");
        n += formatar_texto(novo + n, (size_t)(ARQ_MAX - n), "%s:%u:%s\n", GRUPO_ADMIN, GID_ADMIN, sim ? nome : "");
    }
    if (b) libera(b);
    int r = conf_gravar("grupos", novo, n);
    libera(novo);
    return r;
}

bool conta_home_com_donos(void)
{
    info_permisos_t p;
    return permisos_arquivo("/home", &p) == 0 && p.com_donos;
}

/* ¿Cuenta que no se borra ni se renombra? root, sagui, las de servicio y la de la sesión local. */
static bool conta_protegida(const conta_t *c)
{
    int s = usuario_op(USU_SESSAO, 0, 0);
    return c->uid < CONTA_UID_PRIMEIRO || c->uid > CONTA_UID_ULTIMO || !c->shell[0] ||
           (s >= 0 && ((unsigned)s & 0xFFFF) == c->uid);
}

/* El primer uid libre: ni de una cuenta ni dueño de una carpeta de /home (una cuenta borrada que
 * dejó sus archivos: su uid no se da a otra, que vería sus archivos). */
static unsigned uid_livre(const conta_t *v, int n)
{
    unsigned u = CONTA_UID_PRIMEIRO;
    for (int i = 0; i < n; i++)
        if (v[i].uid >= u && v[i].uid <= CONTA_UID_ULTIMO) u = v[i].uid + 1;
    entrada_pasta_t e;
    for (int i = 0; listar("/home", i, &e) == 1; i++) {
        if (e.tipo != TIPO_PASTA) continue;
        char c[96];
        formatar_texto(c, sizeof c, "/home/%s", e.nome);
        info_permisos_t p;
        if (permisos_arquivo(c, &p) == 0 && p.uid >= u && p.uid <= CONTA_UID_ULTIMO) u = p.uid + 1;
    }
    return u;
}

static int pasta_da_conta(const char *c, unsigned uid, unsigned gid)
{
    int r = criar_pasta(c);
    if (r < 0 && r != ERRO_JA_EXISTE) return r;
    if ((r = mudar_dono(c, (int)uid, (int)gid)) < 0) return r;
    if ((r = mudar_permisos(c, 0700)) < 0) return r;
    info_permisos_t p;
    if (permisos_arquivo(c, &p) < 0 || !p.com_donos || p.uid != uid || (p.modo & 0777) != 0700) return ERRO_NAO_SUPORTADO;
    return 0;
}

/* Borra "c" con todo lo de dentro. Solo lo usa conta_apagar, como root, en /home/<cuenta>. */
static int apagar_arvore(const char *c, int prof)
{
    info_arquivo_t inf;
    int r = info_arquivo(c, &inf);
    if (r < 0) return r;
    if (inf.tipo != TIPO_PASTA) return apagar(c);
    if (prof > 32) return ERRO_LIMITE;
    char *f = aloca(256);
    if (!f) return ERRO_SEM_MEMORIA;
    entrada_pasta_t e;
    int voltas = 0;
    while (listar(c, 0, &e) == 1 && voltas++ < 100000) {
        formatar_texto(f, 256, "%s/%s", c, e.nome);
        if (tam_texto(f) >= 250 || (r = apagar_arvore(f, prof + 1)) < 0) { libera(f); return r < 0 ? r : ERRO_LIMITE; }
    }
    libera(f);
    return remover_pasta(c);
}

/* D385: el ~/.sagshrc de ejemplo de /etc/skel para la cuenta nueva, suyo, y la marca que le dice a sagsh
 * que ya se le dio (así no lo vuelve a crear si la persona lo borra). Si falla, la cuenta sigue igual. */
static void rc_de_exemplo(const char *casa, unsigned uid)
{
    int fd = abrir("/etc/skel/.sagshrc", ABRIR_LER);
    if (fd < 0) return;
    unsigned tam = 0;
    char *b = ler_todo_fd(fd, &tam);
    fechar(fd);
    if (!b) return;
    char c[96];
    formatar_texto(c, sizeof c, "%s/.sagshrc", casa);
    fd = abrir(c, ABRIR_ESCREVER | ABRIR_CRIAR | ABRIR_TRUNCAR);
    if (fd >= 0) {
        escrever(fd, b, tam);
        fechar(fd);
        mudar_dono(c, (int)uid, (int)uid);
        mudar_permisos(c, 0644);
        formatar_texto(c, sizeof c, "%s/.config/sagshrc-ejemplo", casa);
        fd = abrir(c, ABRIR_ESCREVER | ABRIR_CRIAR | ABRIR_TRUNCAR);
        if (fd >= 0) { fechar(fd); mudar_dono(c, (int)uid, (int)uid); }
    }
    libera(b);
}

int conta_criar(const char *nome, const char *descricao, const char *senha, bool admin, unsigned *uid_saida)
{
    if (!sou_root()) return ERRO_ACESSO;
    if (!nome_valido(nome) || acha_char(descricao, ':') || acha_char(descricao, '\n')) return ERRO_INVALIDO;
    if (senha[0] && tam_texto(senha) < 4) return ERRO_INVALIDO;
    if (!conta_home_com_donos()) return ERRO_NAO_SUPORTADO;
    conta_t v[CONTAS_MAX];
    int n = contas_ler(v, CONTAS_MAX, NULL);
    if (achar_nome(v, n, nome) >= 0) return ERRO_JA_EXISTE;
    if (n >= CONTAS_MAX) return ERRO_LIMITE;
    char casa[64];
    formatar_texto(casa, sizeof casa, "/home/%s", nome);
    info_arquivo_t inf;
    if (info_arquivo(casa, &inf) == 0) return ERRO_JA_EXISTE;   /* una carpeta de sagui (o de una cuenta borrada) */
    unsigned uid = uid_livre(v, n);
    if (uid > CONTA_UID_ULTIMO) return ERRO_LIMITE;
    /* Primero la sombra con «!» (sin entrada): la cuenta nunca existe sin la contraseña elegida. */
    int r = sombra_gravar(nome, "!");
    if (r < 0) return r;
    if ((r = pasta_da_conta(casa, uid, uid)) < 0) { remover_pasta(casa); sombra_remover(nome); return r; }
    char cfg[80];
    formatar_texto(cfg, sizeof cfg, "%s/.config", casa);
    pasta_da_conta(cfg, uid, uid);
    rc_de_exemplo(casa, uid);
    conta_t x;
    memset(&x, 0, sizeof x);
    copia_texto(x.nome, nome, sizeof x.nome);
    x.uid = x.gid = uid;
    copia_texto(x.casa, casa, sizeof x.casa);
    copia_texto(x.shell, "/bin/sagsh", sizeof x.shell);
    copia_texto(x.descricao, descricao, sizeof x.descricao);
    v[n++] = x;
    if ((r = contas_gravar(v, n)) < 0) { apagar_arvore(casa, 0); sombra_remover(nome); return r; }
    if ((r = conta_senha(nome, senha)) < 0) return r;   /* la cuenta queda con «!» (no entra): se puede repetir */
    if (admin && (r = conta_admin_fixar(nome, true)) < 0) return r;
    if (uid_saida) *uid_saida = uid;
    registrar("contas: cuenta %s creada (uid %u, %s, %s)", nome, uid, senha[0] ? "con contraseña" : "sin contraseña",
              admin ? "administrador" : "normal");
    return 0;
}

int conta_senha(const char *nome, const char *senha)
{
    if (!sou_root()) return ERRO_ACESSO;
    conta_t c;
    if (!conta_por_nome(nome, &c)) return ERRO_NAO_EXISTE;
    if (!senha[0]) return sombra_gravar(nome, "");   /* sin contraseña */
    if (tam_texto(senha) < 4) return ERRO_INVALIDO;
    /* El hash lo hace el gestor de servicios (D255): root no necesita la contraseña de antes. */
    pedido_conta_t p;
    memset(&p, 0, sizeof p);
    p.tipo = PED_SENHA;
    p.uid_alvo = c.uid;
    copia_texto(p.senha_nova, senha, sizeof p.senha_nova);
    int r = pedir_conta(&p);
    memset(&p, 0, sizeof p);
    return r == 1 ? 0 : r < 0 ? r : ERRO_ACESSO;
}

int conta_apagar(const char *nome, bool con_arquivos)
{
    if (!sou_root()) return ERRO_ACESSO;
    conta_t v[CONTAS_MAX];
    int n = contas_ler(v, CONTAS_MAX, NULL);
    int i = achar_nome(v, n, nome);
    if (i < 0) return ERRO_NAO_EXISTE;
    conta_t c = v[i];
    if (conta_protegida(&c)) return ERRO_ACESSO;
    for (int k = i; k < n - 1; k++) v[k] = v[k + 1];
    n--;
    int r = contas_gravar(v, n);   /* primero deja de existir (ya no entra) */
    if (r < 0) return r;
    sombra_remover(nome);
    conta_admin_fixar(nome, false);
    if (con_arquivos) {
        char casa[64];
        formatar_texto(casa, sizeof casa, "/home/%s", nome);
        info_permisos_t p;
        /* Solo su carpeta: /home/<nombre>, y de esa cuenta (nunca una de sagui con el mismo nombre). */
        if (compara_texto(c.casa, casa) == 0 && permisos_arquivo(casa, &p) == 0 && p.uid == c.uid && p.tipo == TIPO_PASTA)
            r = apagar_arvore(casa, 0);
        registrar("contas: carpeta %s borrada: %d", casa, r);
    }
    registrar("contas: cuenta %s (uid %u) borrada%s", nome, c.uid, con_arquivos ? " con sus archivos" : "");
    return r < 0 ? r : 0;
}

int conta_renomear(const char *velho, const char *novo)
{
    if (!sou_root()) return ERRO_ACESSO;
    if (!nome_valido(novo)) return ERRO_INVALIDO;
    conta_t v[CONTAS_MAX];
    int n = contas_ler(v, CONTAS_MAX, NULL);
    int i = achar_nome(v, n, velho);
    if (i < 0) return ERRO_NAO_EXISTE;
    if (achar_nome(v, n, novo) >= 0) return ERRO_JA_EXISTE;
    if (conta_protegida(&v[i])) return ERRO_ACESSO;
    char ca[64], cn[64];
    formatar_texto(ca, sizeof ca, "/home/%s", velho);
    formatar_texto(cn, sizeof cn, "/home/%s", novo);
    info_arquivo_t inf;
    if (info_arquivo(cn, &inf) == 0) return ERRO_JA_EXISTE;
    /* La contraseña primero con el nombre nuevo: en ningún momento hay una cuenta sin la suya. */
    char h[SOMBRA_TEXTO_MAX] = "";
    int r = sombra_ler(velho, h, sizeof h);
    if (r < 0) return r;
    r = sombra_gravar(novo, r > 0 ? h : "");   /* sin línea = sin contraseña, igual con el nombre nuevo */
    memset(h, 0, sizeof h);
    if (r < 0) return r;
    bool admin = conta_admin(velho);
    if (compara_texto(v[i].casa, ca) == 0 && info_arquivo(ca, &inf) == 0) {
        if ((r = renomear(ca, cn)) < 0) { sombra_remover(novo); return r; }
        copia_texto(v[i].casa, cn, sizeof v[i].casa);
    }
    copia_texto(v[i].nome, novo, sizeof v[i].nome);
    if ((r = contas_gravar(v, n)) < 0) return r;
    sombra_remover(velho);
    if (admin) { conta_admin_fixar(novo, true); conta_admin_fixar(velho, false); }
    registrar("contas: cuenta %s renombrada a %s", velho, novo);
    return 0;
}
