// SPDX-License-Identifier: GPL-2.0-or-later
/* sistema_sdl.c - archivos, carpetas, reloj, memoria y el resto de llamadas «de kernel» de la capa
 * «libsagui sobre SDL 2» (D459), con la libc del anfitrión.
 *
 * Las rutas de SaguiOS se traducen a las del PC (portavel_ruta): /home es la carpeta personal del usuario
 * ($HOME), /sistema, /fontes, /apps/fuentes, /ejemplos e /imagenes son la carpeta de datos del programa y todo lo demás (/tmp, /usr...)
 * es la ruta real. Lo que no tiene sentido fuera de SaguiOS (discos, procesos, red, audio, usuarios)
 * devuelve un error limpio (sistema_nulo.c). */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "sagui.h"
#include "portavel.h"

/* ---------------------------------------------------------------- rutas */

static char datos_dir[1024];

const char *portavel_datos(void)
{
    if (datos_dir[0]) return datos_dir;
    const char *e = getenv("SAGUI_DATOS");
    if (e && *e) { snprintf(datos_dir, sizeof datos_dir, "%s", e); return datos_dir; }
    char exe[900];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof exe - 1);
    if (n > 0) {
        exe[n] = 0;
        char *b = strrchr(exe, '/');
        if (b) *b = 0;
        /* junto al ejecutable (./datos) o instalado (../share/sagui-portable) */
        char t[1000];
        snprintf(t, sizeof t, "%s/datos", exe);
        struct stat st;
        if (stat(t, &st) == 0) { snprintf(datos_dir, sizeof datos_dir, "%s", t); return datos_dir; }
        snprintf(t, sizeof t, "%s/../share/sagui-portable", exe);
        if (stat(t, &st) == 0) { snprintf(datos_dir, sizeof datos_dir, "%s", t); return datos_dir; }
    }
    snprintf(datos_dir, sizeof datos_dir, "datos");
    return datos_dir;
}

static const char *casa(void)
{
    const char *h = getenv("HOME");
    return h && *h ? h : "/tmp";
}

static int prefijo(const char *s, const char *p)
{
    size_t n = strlen(p);
    return !strncmp(s, p, n) && (s[n] == 0 || s[n] == '/');
}

const char *portavel_ruta(const char *s, char *buf, size_t max)
{
    if (!s) return "";
    if (s[0] != '/') return s;
    const char *h = casa();   /* una ruta que ya es de la carpeta personal real (/home/ana/x) se queda como está */
    if (prefijo(s, h)) return s;
    if (!strncmp(s, "/home/", 6)) {   /* /home/ana/x: si /home/ana existe de verdad en el PC, es la ruta real */
        char c[512];
        size_t n = strcspn(s + 6, "/");
        if (n < 400) {
            snprintf(c, sizeof c, "/home/%.*s", (int)n, s + 6);
            struct stat st;
            if (stat(c, &st) == 0 && S_ISDIR(st.st_mode)) return s;
        }
    }
    if (prefijo(s, "/home")) { snprintf(buf, max, "%s%s", casa(), s + 5); return buf; }
    if (prefijo(s, "/sistema") || prefijo(s, "/fontes")) { snprintf(buf, max, "%s%s", portavel_datos(), s); return buf; }
    if (prefijo(s, "/apps/fuentes")) { snprintf(buf, max, "%s/fontes%s", portavel_datos(), s + 13); return buf; }
    if (prefijo(s, "/apps") || prefijo(s, "/bin") || prefijo(s, "/etc") || prefijo(s, "/ejemplos") || prefijo(s, "/imagenes")) { snprintf(buf, max, "%s%s", portavel_datos(), s); return buf; }
    return s;
}

#define RUTA(c) char _r[2048]; const char *c##_h = portavel_ruta((c), _r, sizeof _r)

/* ---------------------------------------------------------------- archivos */

int abrir(const char *caminho, int modo)
{
    RUTA(caminho);
    int f = 0;
    if ((modo & ABRIR_LER) && (modo & ABRIR_ESCREVER)) f = O_RDWR;
    else if (modo & ABRIR_ESCREVER) f = O_WRONLY;
    else f = O_RDONLY;
    if (modo & ABRIR_CRIAR) f |= O_CREAT;
    if (modo & ABRIR_TRUNCAR) f |= O_TRUNC;
    if (modo & ABRIR_ACRESCENTAR) f |= O_APPEND;
    struct stat st;
    if (stat(caminho_h, &st) == 0 && S_ISDIR(st.st_mode)) return ERRO_E_PASTA;
    int fd = open(caminho_h, f, 0666);
    if (fd < 0) return errno == ENOENT ? ERRO_NAO_EXISTE : errno == EACCES || errno == EPERM ? ERRO_ACESSO : errno == EEXIST ? ERRO_JA_EXISTE : errno == ENOSPC ? ERRO_SEM_ESPACO : ERRO_INVALIDO;
    return fd;
}

int ler(int fd, void *buf, unsigned n)
{
    ssize_t r = read(fd, buf, n);
    return r < 0 ? ERRO_DESCRITOR : (int)r;
}

int escrever(int fd, const void *buf, unsigned n)
{
    ssize_t r = write(fd, buf, n);
    return r < 0 ? (errno == ENOSPC ? ERRO_SEM_ESPACO : ERRO_DESCRITOR) : (int)r;
}

int fechar(int fd) { return close(fd) == 0 ? 0 : ERRO_DESCRITOR; }

int posicionar(int fd, int desloc, int origem)
{
    off_t r = lseek(fd, desloc, origem == POS_FIM ? SEEK_END : origem == POS_ATUAL ? SEEK_CUR : SEEK_SET);
    return r < 0 ? ERRO_INVALIDO : (int)r;
}

int info_arquivo(const char *caminho, info_arquivo_t *info)
{
    RUTA(caminho);
    struct stat st;
    if (stat(caminho_h, &st) != 0) return ERRO_NAO_EXISTE;
    info->tipo = S_ISDIR(st.st_mode) ? TIPO_PASTA : TIPO_ARQUIVO;
    info->tamanho = (unsigned)st.st_size;
    return 0;
}

int truncar(int fd, unsigned tamanho) { return ftruncate(fd, tamanho) == 0 ? 0 : ERRO_INVALIDO; }
int sincronizar(int fd) { return fsync(fd) == 0 ? 0 : ERRO_INVALIDO; }

int criar_pasta(const char *caminho)
{
    RUTA(caminho);
    if (mkdir(caminho_h, 0777) == 0) return 0;
    return errno == EEXIST ? ERRO_JA_EXISTE : errno == ENOENT ? ERRO_NAO_EXISTE : ERRO_ACESSO;
}

int remover_pasta(const char *caminho)
{
    RUTA(caminho);
    if (rmdir(caminho_h) == 0) return 0;
    return errno == ENOTEMPTY ? ERRO_NAO_VAZIA : errno == ENOENT ? ERRO_NAO_EXISTE : ERRO_ACESSO;
}

int apagar(const char *caminho)
{
    RUTA(caminho);
    if (unlink(caminho_h) == 0) return 0;
    return errno == ENOENT ? ERRO_NAO_EXISTE : errno == EISDIR ? ERRO_E_PASTA : ERRO_ACESSO;
}

int renomear(const char *origem, const char *destino)
{
    char a[2048], b[2048];
    if (rename(portavel_ruta(origem, a, sizeof a), portavel_ruta(destino, b, sizeof b)) == 0) return 0;
    return errno == ENOENT ? ERRO_NAO_EXISTE : ERRO_ACESSO;
}

/* Listado por índice: se lee la carpeta entera al pedir el 0 (o al cambiar de carpeta) y se sirve de la copia. */
static char lst_ruta[2048];
static entrada_pasta_t *lst_v;
static int lst_n;

static int cmp_ent(const void *a, const void *b) { return strcmp(((const entrada_pasta_t *)a)->nome, ((const entrada_pasta_t *)b)->nome); }

int listar(const char *caminho, int indice, entrada_pasta_t *e)
{
    RUTA(caminho);
    if (indice == 0 || strcmp(lst_ruta, caminho_h)) {
        DIR *d = opendir(caminho_h);
        if (!d) return ERRO_NAO_EXISTE;
        free(lst_v); lst_v = NULL; lst_n = 0;
        struct dirent *de;
        int cap = 0;
        while ((de = readdir(d))) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..") || strlen(de->d_name) >= NOME_MAX) continue;
            if (lst_n == cap) { cap = cap ? cap * 2 : 64; lst_v = realloc(lst_v, sizeof *lst_v * (size_t)cap); }
            entrada_pasta_t *x = &lst_v[lst_n++];
            memset(x, 0, sizeof *x);
            strcpy(x->nome, de->d_name);
            char p[3200];
            struct stat st;
            snprintf(p, sizeof p, "%s/%s", caminho_h, de->d_name);
            if (stat(p, &st) == 0) { x->tipo = S_ISDIR(st.st_mode) ? TIPO_PASTA : TIPO_ARQUIVO; x->tamanho = (unsigned)st.st_size; }
            else x->tipo = TIPO_ARQUIVO;
        }
        closedir(d);
        qsort(lst_v, (size_t)lst_n, sizeof *lst_v, cmp_ent);
        snprintf(lst_ruta, sizeof lst_ruta, "%s", caminho_h);
    }
    if (indice < 0 || indice >= lst_n) return 0;
    *e = lst_v[indice];
    return 1;
}

int mudar_pasta(const char *caminho)
{
    RUTA(caminho);
    return chdir(caminho_h) == 0 ? 0 : ERRO_NAO_EXISTE;
}

int pasta_atual(char *buf, unsigned n)
{
    char t[2048];
    if (!getcwd(t, sizeof t)) return ERRO_INVALIDO;
    const char *h = casa();
    size_t hl = strlen(h);
    if (!strncmp(t, h, hl) && (t[hl] == 0 || t[hl] == '/')) snprintf(buf, n, "/home%s", t + hl);
    else snprintf(buf, n, "%s", t);
    return 0;
}

int mascara_criacao(int nova)
{
    if (nova < 0) { mode_t m = umask(022); umask(m); return (int)m; }
    return (int)umask((mode_t)nova);
}

int permisos_arquivo(const char *caminho, info_permisos_t *p)
{
    RUTA(caminho);
    struct stat st;
    if (stat(caminho_h, &st) != 0) return ERRO_NAO_EXISTE;
    memset(p, 0, sizeof *p);
    p->uid = st.st_uid; p->gid = st.st_gid; p->modo = st.st_mode & 07777;
    p->tipo = S_ISDIR(st.st_mode) ? TIPO_PASTA : TIPO_ARQUIVO; p->com_donos = 1;
    return 0;
}

int mudar_permisos(const char *caminho, unsigned modo)
{
    RUTA(caminho);
    return chmod(caminho_h, modo & 07777) == 0 ? 0 : ERRO_ACESSO;
}

int mudar_dono(const char *caminho, int uid, int gid)
{
    RUTA(caminho);
    return chown(caminho_h, (uid_t)uid, (gid_t)gid) == 0 ? 0 : ERRO_ACESSO;
}

int fixar_datas(const char *caminho, unsigned modificado, unsigned acessado)
{
    (void)caminho; (void)modificado; (void)acessado;
    return 0;
}

int datas_arquivo(const char *caminho, datas_arquivo_t *d)
{
    RUTA(caminho);
    struct stat st;
    if (stat(caminho_h, &st) != 0) return ERRO_NAO_EXISTE;
    d->modificado = (unsigned)st.st_mtime; d->criado = (unsigned)st.st_ctime; d->acessado = (unsigned)st.st_atime;
    return 0;
}

int tipo_descritor(int fd)
{
    struct stat st;
    if (fstat(fd, &st) != 0) return ERRO_DESCRITOR;
    return S_ISREG(st.st_mode) ? TIPO_ARQUIVO : S_ISFIFO(st.st_mode) ? TIPO_PIPE : TIPO_TERMINAL;
}

int duplicar(int antigo, int novo) { return dup2(antigo, novo) < 0 ? ERRO_DESCRITOR : novo; }
int criar_pipe(int fds[2]) { return pipe(fds) == 0 ? 0 : ERRO_LIMITE; }
int disponivel(int fd) { (void)fd; return 0; }

/* ---------------------------------------------------------------- reloj */

unsigned tempo_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (unsigned)(t.tv_sec * 1000 + t.tv_nsec / 1000000);
}

int dormir(unsigned ms)
{
    struct timespec t = { ms / 1000, (long)(ms % 1000) * 1000000L };
    nanosleep(&t, NULL);
    return 0;
}

int ler_data(data_t *d)
{
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    d->ano = (unsigned short)(tm.tm_year + 1900); d->mes = (unsigned char)(tm.tm_mon + 1); d->dia = (unsigned char)tm.tm_mday;
    d->hora = (unsigned char)tm.tm_hour; d->minuto = (unsigned char)tm.tm_min; d->segundo = (unsigned char)tm.tm_sec;
    d->dia_semana = (unsigned char)tm.tm_wday;
    return 0;
}

uint32_t hora_utc(hora_t *h)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (h) {
        struct tm tm;
        time_t t = tv.tv_sec;
        localtime_r(&t, &tm);
        h->segundos = (unsigned)tv.tv_sec; h->ms = (unsigned)(tv.tv_usec / 1000); h->desvio = (int)tm.tm_gmtoff;
        h->verao = (unsigned)(tm.tm_isdst > 0); h->ajustes = 0;
    }
    return (uint32_t)tv.tv_sec;
}

int ajustar_hora(uint32_t utc, int opcoes) { (void)utc; (void)opcoes; return ERRO_ACESSO; }
int ler_rtc(data_t *d) { return ler_data(d); }

/* La zona del proceso sale de TZ (si la hay, como en SaguiOS) o del desfase actual del PC. */
int ler_zona(config_zona_t *z)
{
    memset(z, 0, sizeof *z);
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    long off = tm.tm_gmtoff;
    int m = (int)(off < 0 ? -off : off) / 60;
    snprintf(z->nome, sizeof z->nome, "UTC%+d", (int)(off / 3600));
    if (m % 60) snprintf(z->tz, sizeof z->tz, "<%c%02d%02d>%c%d:%02d", off < 0 ? '-' : '+', m / 60, m % 60, off < 0 ? '+' : '-', m / 60, m % 60);
    else snprintf(z->tz, sizeof z->tz, "<%c%02d>%c%d", off < 0 ? '-' : '+', m / 60, off < 0 ? '+' : '-', m / 60);
    z->formato_24h = 1;
    return 0;
}

int fixar_zona(const config_zona_t *z) { (void)z; return ERRO_ACESSO; }

/* ---------------------------------------------------------------- memoria */

void *aloca(size_t n) { return malloc(n ? n : 1); }
void *aloca_zerado(size_t n) { return calloc(1, n ? n : 1); }
void *realoca(void *p, size_t n) { return realloc(p, n ? n : 1); }
void libera(void *p) { free(p); }
unsigned memoria_total(void) { return 0x7FFFFFFFu; }
size_t memoria_tamanho(const void *p) { (void)p; return 0; }
void *aloca_alinhado(size_t alin, size_t n) { void *p = NULL; return posix_memalign(&p, alin < sizeof(void *) ? sizeof(void *) : alin, n) == 0 ? p : NULL; }
void *mapear_memoria(size_t n) { return calloc(1, (n + 4095) & ~(size_t)4095); }
int desmapear_memoria(void *p, size_t n) { (void)n; free(p); return 0; }

void *mapear_arquivo(int fd, unsigned desloc, size_t n)
{
    void *m = calloc(1, n ? n : 1);
    if (!m) return NULL;
    ssize_t r = pread(fd, m, n, desloc);
    (void)r;
    return m;
}

/* ---------------------------------------------------------------- entorno, registro y salida */

const char *obter_env(const char *nome) { return getenv(nome); }

void sair(int codigo) { exit(codigo); }
int meu_pid(void) { return (int)getpid(); }
int thread_id(void) { return 1; }

int registrar_texto(const char *t)
{
    if (getenv("SAGUI_REGISTRO")) fputs(t, stderr);
    return 0;
}

int bytes_aleatorios(void *buf, unsigned n)
{
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) return ERRO_NAO_EXISTE;
    size_t r = fread(buf, 1, n, f);
    fclose(f);
    return (int)r;
}

/* ---------------------------------------------------------------- idioma: SIS_IDIOMA sin kernel */

int portavel_idioma(unsigned op, const void *p, unsigned n)
{
    if (op == IDIOMA_LER) {
        const char *l = getenv("LANG");
        snprintf((char *)(uintptr_t)p, n, "%s", l && *l && strcmp(l, "C") && strcmp(l, "POSIX") ? l : "es");
        return 0;
    }
    (void)p; (void)n;
    return 0;
}

/* ---------------------------------------------------------------- arranque */

extern char **__argv_inicial;
char **__argv_inicial;

int main(int argc, char **argv)
{
    __argv_inicial = argv;
    return principal(argc, argv);
}
