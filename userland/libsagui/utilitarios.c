// SPDX-License-Identifier: GPL-2.0-or-later
/* utilitarios.c - piezas pequeñas que comparten los comandos del terminal (D097):
 * análisis de opciones al estilo getopt, lectura de líneas con búfer, nombres de
 * archivo, listados de carpeta, tamaños legibles y comodines. La expresión
 * regular de grep y find está en regex.c. */
#include "sagui.h"

/* ---------------------------------------------------------------- opciones */

static bool es_opcion(const char *a) { return a[0] == '-' && a[1] != 0; }

/* ¿La letra c acepta argumento según la cadena "curtas" (como getopt: "nc:")? */
static int letra_con_arg(const char *curtas, int c)
{
    for (const char *p = curtas; *p; p++)
        if (*p == c && c != ':') return p[1] == ':';
    return -1;   /* no existe */
}

/* Cuántos argumentos consume la opción larga o corta que empieza en a (0 o 1 más). */
static int consume_siguiente(const char *a, const char *curtas, const opcoes_t *o)
{
    if (a[1] == '-') {
        if (!a[2]) return 0;
        for (const opcao_longa_t *l = o->longas; l && l->nome; l++) {
            size_t n = tam_texto(l->nome);
            if (compara_texto_n(a + 2, l->nome, n) == 0 && (a[2 + n] == 0 || a[2 + n] == '=')) {
                if (a[2 + n] == '=') return 0;
                return letra_con_arg(curtas, l->curta) == 1;
            }
        }
        return 0;
    }
    if (o->numerica && a[1] >= '0' && a[1] <= '9') return 0;
    for (const char *p = a + 1; *p; p++) {
        int r = letra_con_arg(curtas, *p);
        if (r == 1) return p[1] == 0;   /* argumento pegado (-n5) o en el token siguiente */
    }
    return 0;
}

/* Pone las opciones (con sus argumentos) delante de los operandos, para que
 * "ls carpeta -l" funcione como en Unix. Un "--" corta el reordenamiento. */
static void reordenar(int argc, char **argv, const char *curtas, const opcoes_t *o)
{
    char *opc[128], *ops[128];
    int no = 0, np = 0, i = 1;
    if (argc > 127) return;
    for (; i < argc; i++) {
        char *a = argv[i];
        if (compara_texto(a, "--") == 0) break;
        if (es_opcion(a)) {
            opc[no++] = a;
            if (consume_siguiente(a, curtas, o) && i + 1 < argc) opc[no++] = argv[++i];
        } else ops[np++] = a;
    }
    int k = 1;
    for (int j = 0; j < no; j++) argv[k++] = opc[j];
    for (int j = 0; j < np; j++) argv[k++] = ops[j];
}

int obter_opcao(int argc, char **argv, const char *curtas, opcoes_t *o)
{
    if (o->indice == 0) { reordenar(argc, argv, curtas, o); o->indice = 1; o->_pos = NULL; }
    o->valor = NULL;
    if (!o->_pos) {
        if (o->indice >= argc) return -1;
        const char *a = argv[o->indice];
        if (compara_texto(a, "--") == 0) { o->indice++; return -1; }
        if (!es_opcion(a)) return -1;
        if (a[1] == '-') {   /* opción larga */
            o->indice++;
            if (compara_texto(a, "--help") == 0 || compara_texto(a, "--ayuda") == 0) return OPC_AJUDA;
            for (const opcao_longa_t *l = o->longas; l && l->nome; l++) {
                size_t n = tam_texto(l->nome);
                if (compara_texto_n(a + 2, l->nome, n) != 0 || (a[2 + n] != 0 && a[2 + n] != '=')) continue;
                if (letra_con_arg(curtas, l->curta) == 1) {
                    if (a[2 + n] == '=') o->valor = a + 3 + n;
                    else if (o->indice < argc) o->valor = argv[o->indice++];
                    else { o->errada = a; return ':'; }
                } else if (a[2 + n] == '=') { o->errada = a; return '?'; }
                return l->curta;
            }
            o->errada = a;
            return '?';
        }
        if (o->numerica && a[1] >= '0' && a[1] <= '9') { o->valor = a + 1; o->indice++; return '#'; }
        o->_pos = a + 1;
    }
    int c = (unsigned char)*o->_pos++;
    int r = letra_con_arg(curtas, c);
    static char errada[3];
    if (r < 0) {
        errada[0] = '-'; errada[1] = (char)c; errada[2] = 0;
        o->errada = errada;
        if (!*o->_pos) { o->_pos = NULL; o->indice++; }
        return '?';
    }
    if (r == 1) {
        if (*o->_pos) { o->valor = o->_pos; o->_pos = NULL; o->indice++; }
        else {
            o->_pos = NULL;
            o->indice++;
            if (o->indice < argc) o->valor = argv[o->indice++];
            else { errada[0] = '-'; errada[1] = (char)c; errada[2] = 0; o->errada = errada; return ':'; }
        }
        return c;
    }
    if (!*o->_pos) { o->_pos = NULL; o->indice++; }
    return c;
}

/* Mensaje estándar para una opción incorrecta; devuelve 2 (código de uso). */
int opcao_invalida(const char *cmd, int r, const opcoes_t *o)
{
    if (r == ':') imprimir_em(ERROS, "%s: la opción %s necesita un valor (prueba %s --help)\n", cmd, o->errada, cmd);
    else imprimir_em(ERROS, "%s: opción desconocida: %s (prueba %s --help)\n", cmd, o->errada, cmd);
    return 2;
}

/* Entero decimal no negativo; false si el texto no lo es. */
bool texto_para_numero(const char *s, int *v)
{
    if (!*s) return false;
    int r = 0;
    for (; *s; s++) {
        if (*s < '0' || *s > '9') return false;
        r = r * 10 + (*s - '0');
    }
    *v = r;
    return true;
}

/* -------------------------------------------------------------- entrada */

/* "-" es la entrada estándar; si no, abre el archivo (y avisa si falla). */
int abrir_entrada(const char *cmd, const char *caminho)
{
    if (compara_texto(caminho, "-") == 0) return ENTRADA;
    int fd = abrir(caminho, ABRIR_LER);
    if (fd < 0) { imprimir_em(ERROS, "%s: %s: %s\n", cmd, caminho, nome_erro(fd)); return -1; }
    info_arquivo_t inf;
    if (info_arquivo(caminho, &inf) == 0 && inf.tipo == TIPO_PASTA) {
        fechar(fd);
        imprimir_em(ERROS, "%s: %s: es una carpeta\n", cmd, caminho);
        return -1;
    }
    return fd;
}

void fechar_entrada(int fd) { if (fd > ERROS) fechar(fd); }

void leitor_iniciar(leitor_linhas_t *l, int fd, bool latin1)
{
    l->fd = fd; l->ini = l->fim = 0; l->latin1 = latin1; l->acabou = false;
}

/* Lee una línea (sin el \n) en dst, cortándola si es más larga que max - 1.
 * Devuelve el largo, o -1 al final. *con_salto indica si terminaba en \n. */
int leitor_linha(leitor_linhas_t *l, char *dst, int max, bool *com_salto)
{
    int n = 0;
    bool algo = false, salto = false;
    for (;;) {
        if (l->ini >= l->fim) {
            if (l->acabou) break;
            int r = ler(l->fd, l->buf, sizeof l->buf);
            if (r <= 0) { l->acabou = true; break; }
            l->ini = 0; l->fim = r;
        }
        algo = true;
        char c = l->buf[l->ini++];
        if (c == '\n') { salto = true; break; }
        if (n + 1 < max) dst[n++] = c;
    }
    if (!algo) return -1;
    dst[n] = 0;
    if (l->latin1) {
        bool alto = false;
        for (int i = 0; i < n; i++) if ((unsigned char)dst[i] >= 0xC2) { alto = true; break; }
        if (alto) { n = (int)utf8_para_latin1(dst, (size_t)n, NULL); dst[n] = 0; }
    }
    if (com_salto) *com_salto = salto;
    return n;
}

/* Lee todo un descriptor a memoria (con un cero al final). NULL si falla. */
char *ler_todo_fd(int fd, unsigned *tam)
{
    unsigned cap = 4096, n = 0;
    char *b = aloca(cap);
    if (!b) return NULL;
    for (;;) {
        if (n + 1024 + 1 > cap) {
            cap *= 2;
            char *nb = realoca(b, cap);
            if (!nb) { libera(b); return NULL; }
            b = nb;
        }
        int r = ler(fd, b + n, cap - n - 1);
        if (r <= 0) break;
        n += (unsigned)r;
    }
    b[n] = 0;
    if (tam) *tam = n;
    return b;
}

/* Escribe una línea y un salto; <0 si el otro extremo del pipe ya no lee. */
int saida_linha(const char *s, int n)
{
    if (n < 0) n = (int)tam_texto(s);
    int r = escrever_tudo(SAIDA, s, (unsigned)n);
    if (r < 0) return r;
    return escrever_tudo(SAIDA, "\n", 1);
}

bool eh_terminal(int fd) { return tipo_descritor(fd) == TIPO_TERMINAL; }

/* ¿Hay un terminal con colores en el entorno? TERM definido (lo ponen el Terminal y sshd; la
 * consola de texto y el puerto serie no) y distinto de "dumb", y NO_COLOR sin definir (D263). */
bool cor_do_ambiente(void)
{
    const char *t = obter_env("TERM"), *n = obter_env("NO_COLOR");
    if (!t || !*t || compara_texto(t, "dumb") == 0) return false;
    return !(n && *n);
}

/* Quita de argv las opciones --color, --color=WHEN y --colour: devuelve 0 (never), 1 (always),
 * 2 (auto, por defecto). Así cada programa decide con cor_para() sin tocar su obter_opcao. */
int cor_opcao(int *argc, char **argv)
{
    int modo = 2, k = 1;
    for (int i = 1; i < *argc; i++) {
        const char *a = argv[i];
        if (compara_texto(a, "--") == 0) { while (i < *argc) argv[k++] = argv[i++]; break; }
        const char *v = NULL;
        if (compara_texto(a, "--color") == 0 || compara_texto(a, "--colour") == 0) v = "always";
        else if (compara_texto_n(a, "--color=", 8) == 0) v = a + 8;
        else if (compara_texto_n(a, "--colour=", 9) == 0) v = a + 9;
        if (!v) { argv[k++] = argv[i]; continue; }
        if (compara_texto(v, "never") == 0 || compara_texto(v, "no") == 0 || compara_texto(v, "none") == 0) modo = 0;
        else if (compara_texto(v, "auto") == 0 || compara_texto(v, "tty") == 0 || compara_texto(v, "if-tty") == 0) modo = 2;
        else modo = 1;
    }
    argv[k] = NULL;
    *argc = k;
    return modo;
}

/* ¿Se pintan colores en fd? always: sí; never: no; auto: solo si es un terminal con colores. */
bool cor_para(int fd, int modo)
{
    if (modo == 0) return false;
    if (modo == 1) return true;
    return eh_terminal(fd) && cor_do_ambiente();
}

/* -------------------------------------------------------------- nombres */

const char *nome_base(const char *caminho)
{
    const char *r = caminho;
    for (const char *p = caminho; *p; p++)
        if (*p == '/' && p[1] && p[1] != '/') r = p + 1;
    return r;
}

/* carpeta + "/" + nombre, sin duplicar la barra. */
void caminho_juntar(char *dst, size_t max, const char *pasta, const char *nome)
{
    copia_texto(dst, pasta, max);
    size_t n = tam_texto(dst);
    if (n && dst[n - 1] != '/') junta_texto(dst, "/", max);
    junta_texto(dst, nome, max);
}

static int cmp_entradas(const void *a, const void *b)
{
    return compara_texto(((const entrada_pasta_t *)a)->nome, ((const entrada_pasta_t *)b)->nome);
}

/* Lee todas las entradas de una carpeta, ordenadas por nombre (inserción:
 * las carpetas suelen ser pequeñas). Devuelve cuántas, o un error negativo;
 * hay que liberar *v con libera. */
int listar_tudo(const char *pasta, entrada_pasta_t **v)
{
    unsigned cap = 32, n = 0;
    entrada_pasta_t *a = aloca(cap * sizeof *a);
    if (!a) return ERRO_SEM_MEMORIA;
    for (;;) {
        if (n == cap) {
            cap *= 2;
            entrada_pasta_t *nb = realoca(a, cap * sizeof *a);
            if (!nb) { libera(a); return ERRO_SEM_MEMORIA; }
            a = nb;
        }
        int r = listar(pasta, (int)n, &a[n]);
        if (r < 0) { libera(a); return r; }
        if (r == 0) break;
        n++;
    }
    for (unsigned i = 1; i < n; i++) {
        entrada_pasta_t t = a[i];
        unsigned j = i;
        while (j > 0 && cmp_entradas(&a[j - 1], &t) > 0) { a[j] = a[j - 1]; j--; }
        a[j] = t;
    }
    *v = a;
    return (int)n;
}

/* ------------------------------------------------------------ tamaños */

/* 1234 -> "1.2K", 3 -> "3": como ls -h y df -h. */
void formatar_tamanho(char *s, size_t max, uint32_t bytes)
{
    static const char un[] = "KMG";
    if (bytes < 1024) { formatar_texto(s, max, "%u", bytes); return; }
    uint32_t v = bytes;
    int u = -1;
    uint32_t resto = 0;
    while (v >= 1024 && u < 2) { resto = v % 1024; v /= 1024; u++; }
    uint32_t dec = resto * 10 / 1024;
    if (v >= 10) formatar_texto(s, max, "%u%c", v + (resto >= 512 ? 1 : 0), un[u]);
    else formatar_texto(s, max, "%u.%u%c", v, dec, un[u]);
}

/* -------------------------------------------------------------- comodines */

/* Comodines * ? [abc] [a-z] [!a] (y \ para escapar), como el shell. */
bool casar_glob(const char *p, const char *t)
{
    const char *sp = NULL, *st = NULL;
    while (*t) {
        if (*p == '*') { sp = ++p; st = t; continue; }
        bool ok = false;
        if (*p == '?') { ok = true; p++; }
        else if (*p == '[') {
            const char *q = p + 1;
            bool neg = *q == '!' || *q == '^', dentro = false;
            if (neg) q++;
            bool primero = true;
            while (*q && (*q != ']' || primero)) {
                primero = false;
                if (q[1] == '-' && q[2] && q[2] != ']') {
                    if ((unsigned char)*t >= (unsigned char)q[0] && (unsigned char)*t <= (unsigned char)q[2]) dentro = true;
                    q += 3;
                } else { if (*q == *t) dentro = true; q++; }
            }
            if (*q == ']') { ok = dentro != neg; p = q + 1; }
            else { ok = *t == '['; p++; }   /* corchete sin cerrar: literal */
        } else if (*p == '\\' && p[1]) { ok = p[1] == *t; p += 2; }
        else if (*p) { ok = *p == *t; p++; }
        if (ok) { t++; continue; }
        if (!sp) return false;
        p = sp;
        t = ++st;
    }
    while (*p == '*') p++;
    return *p == 0;
}

/* ------------------------------------------------------------- ordenar */

/* Ordena n punteros con un criterio (mezcla estable; no hay qsort). */
static void mezcla(void **v, void **aux, int n, int (*cmp)(const void *, const void *))
{
    if (n < 2) return;
    int m = n / 2;
    mezcla(v, aux, m, cmp);
    mezcla(v + m, aux, n - m, cmp);
    int i = 0, j = m, k = 0;
    while (i < m && j < n) aux[k++] = cmp(v[j], v[i]) < 0 ? v[j++] : v[i++];
    while (i < m) aux[k++] = v[i++];
    while (j < n) aux[k++] = v[j++];
    for (k = 0; k < n; k++) v[k] = aux[k];
}

int ordenar_ponteiros(void **v, int n, int (*cmp)(const void *, const void *))
{
    void **aux = aloca((size_t)(n ? n : 1) * sizeof *aux);
    if (!aux) return ERRO_SEM_MEMORIA;
    mezcla(v, aux, n, cmp);
    libera(aux);
    return 0;
}
