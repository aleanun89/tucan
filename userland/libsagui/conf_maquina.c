// SPDX-License-Identifier: GPL-2.0-or-later
/* conf_maquina.c - la configuración de la máquina en /etc (SAGUI-CONF), separada de la de la persona
 * (D371, fase 1). Ver conf_maquina.h. */
#include "sagui.h"
#include "contas.h"
#include "conf_maquina.h"

/* Las claves de sagui.conf que son de la máquina (D371, P9): aceleración (2D y 3D: tocan la GPU; la SDL 2 por la
 * GPU no, y sigue siendo de la persona) y caché del disco, la tarjeta de
 * sonido y sus ajustes, la calibración del mando del puerto de juegos, y el teclado del sistema (el de la
 * pantalla de inicio de sesión; la persona puede tener el suyo, P7). */
static const char *const claves[] = {
    "acel2d", "gpu3d", "disco_dma_cache", "salida", "latencia", "balance",
    "micro_entrada", "micro_ganancia", "micro_silencio", "mando_puerto", "teclado",
    "inicio_automatico",   /* fase 6 (P5): entrar sin pedir la cuenta; lo lee el kernel */
};

bool maquina_clave(const char *clave)
{
    for (unsigned i = 0; i < sizeof claves / sizeof claves[0]; i++)
        if (compara_texto(clave, claves[i]) == 0) return true;
    return false;
}

bool maquina_separada(void)
{
    if (!conf_persistente()) return false;
    info_arquivo_t inf;
    return info_arquivo(MAQ_ARQ_CONF, &inf) == 0;
}

/* Lee un archivo pequeño entero ("max" incluye el cero). -1 si no existe. */
static int ler_pequeno(const char *arq, char *t, int max)
{
    int fd = abrir(arq, ABRIR_LER);
    if (fd < 0) return -1;
    int n = ler(fd, t, (unsigned)(max - 1));
    fechar(fd);
    if (n < 0) n = 0;
    t[n] = 0;
    return n;
}

/* ¿La línea "l" (sin '\n') es de la clave "k"? */
static bool linha_de(const char *l, int tl, const char *k)
{
    int tk = (int)tam_texto(k);
    return tl > tk && l[tk] == '=' && compara_texto_n(l, k, (size_t)tk) == 0;
}

int maquina_ler(const char *clave, char *valor, int max)
{
    static char t[2048];
    if (ler_pequeno(MAQ_ARQ_CONF, t, sizeof t) < 0) return 0;
    for (char *p = t; *p;) {
        char *fim = acha_char(p, '\n');
        int tl = fim ? (int)(fim - p) : (int)tam_texto(p);
        if (tl && p[tl - 1] == '\r') tl--;
        if (p[0] != '#' && linha_de(p, tl, clave)) {
            int k = (int)tam_texto(clave) + 1, n = tl - k;
            if (n >= max) n = max - 1;
            memcpy(valor, p + k, (size_t)n);
            valor[n] = 0;
            return 1;
        }
        if (!fim) break;
        p = fim + 1;
    }
    return 0;
}

/* Escribe como root: si no lo es, el diálogo de permisos (y se suelta al acabar). */
static int gravar_como_root(const char *rel, const void *dados, int n, const char *motivo)
{
    bool elevado = false;
    if (!sou_root()) {
        int e = elevar_grafico(motivo ? motivo : "guardar la configuración de la máquina");
        if (e != 0) {
            registrar("conf_maquina: %s no se guarda: hace falta ser administrador (%d)", rel, e);
            return ERRO_ACESSO;
        }
        elevado = true;
    }
    int r = conf_gravar(rel, dados, n);
    if (elevado) elevacao_soltar();
    registrar("conf_maquina: /etc/%s guardado (%d bytes): %d", rel, n, r);
    return r;
}

int maquina_gravar_arquivo(const char *rel, const void *dados, int n, const char *motivo)
{
    return gravar_como_root(rel, dados, n, motivo);
}

/* ¿Está la clave de la línea "l" en el texto "linhas"? */
static bool clave_em(const char *l, int tl, const char *linhas)
{
    const char *eq = l;
    while (eq < l + tl && *eq != '=') eq++;
    if (eq >= l + tl) return false;
    char k[32];
    int tk = (int)(eq - l);
    if (tk <= 0 || tk >= (int)sizeof k) return false;
    memcpy(k, l, (size_t)tk);
    k[tk] = 0;
    for (const char *p = linhas; *p;) {
        const char *fim = acha_char(p, '\n');
        int tp = fim ? (int)(fim - p) : (int)tam_texto(p);
        if (linha_de(p, tp, k)) return true;
        if (!fim) break;
        p = fim + 1;
    }
    return false;
}

int maquina_gravar_linhas(const char *linhas, const char *motivo)
{
    static char velho[2048], novo[2304];
    int nv = ler_pequeno(MAQ_ARQ_CONF, velho, sizeof velho);
    if (nv < 0) velho[0] = 0;
    /* Lo de antes que no se cambia (comentarios, otras claves), y después las líneas nuevas de la máquina. */
    int n = 0;
    if (!velho[0]) n += formatar_texto(novo, sizeof novo, "# Configuración de la máquina (D371): solo la cambia el administrador.\n");
    for (char *p = velho; *p;) {
        char *fim = acha_char(p, '\n');
        int tl = fim ? (int)(fim - p) : (int)tam_texto(p);
        int tc = tl && p[tl - 1] == '\r' ? tl - 1 : tl;
        if (tc && !clave_em(p, tc, linhas) && n + tc + 2 < (int)sizeof novo) {
            memcpy(novo + n, p, (size_t)tc);
            n += tc;
            novo[n++] = '\n';
        }
        if (!fim) break;
        p = fim + 1;
    }
    for (const char *p = linhas; *p;) {   /* solo las claves de la máquina */
        const char *fim = acha_char(p, '\n');
        int tl = fim ? (int)(fim - p) : (int)tam_texto(p);
        const char *eq = p;
        while (eq < p + tl && *eq != '=') eq++;
        char k[32];
        int tk = (int)(eq - p);
        if (eq < p + tl && tk > 0 && tk < (int)sizeof k) {
            memcpy(k, p, (size_t)tk);
            k[tk] = 0;
            if (maquina_clave(k) && tl > tk + 1 && n + tl + 2 < (int)sizeof novo) {   /* "clave=" vacía: se quita */
                memcpy(novo + n, p, (size_t)tl);
                n += tl;
                novo[n++] = '\n';
            }
        }
        if (!fim) break;
        p = fim + 1;
    }
    novo[n] = 0;
    if (nv >= 0 && compara_texto(novo, velho) == 0) return 0;   /* sin cambios: no se pide nada */
    return gravar_como_root(MAQ_CONF, novo, n, motivo);
}

/* ---------------------------------------------------------------- la migración */

/* Copia "de" (en /home) a /etc/"rel" si existe. */
static void migrar_arquivo(const char *de, const char *rel)
{
    static char t[8192];
    int n = ler_pequeno(de, t, sizeof t);
    if (n < 0) return;
    int r = conf_gravar(rel, t, n);
    registrar("conf_maquina: migrado %s -> /etc/%s: %d", de, rel, r);
}

/* La tabla del bloqueo DNS (binaria, hasta unos MB) va a SAGUI-CONF fuera de etc/ (no se copia a la RAM). */
static void migrar_tabla(const char *de)
{
    int fd = abrir(de, ABRIR_LER);
    if (fd < 0) return;
    criar_pasta("/sistema/conf/bloqueo");
    int fo = abrir(MAQ_BLOQ_TABLA ".tmp", ABRIR_ESCREVER | ABRIR_CRIAR | ABRIR_TRUNCAR);
    int r = fo;
    if (fo >= 0) {
        static char b[4096];
        int k;
        while ((k = ler(fd, b, sizeof b)) > 0)
            if ((r = escrever_tudo(fo, b, (unsigned)k)) < 0) break;
        if (k < 0) r = k;
        fechar(fo);
        if (r >= 0) {
            r = renomear(MAQ_BLOQ_TABLA ".tmp", MAQ_BLOQ_TABLA);
            if (r == ERRO_JA_EXISTE) { apagar(MAQ_BLOQ_TABLA); r = renomear(MAQ_BLOQ_TABLA ".tmp", MAQ_BLOQ_TABLA); }
        }
        if (r < 0) apagar(MAQ_BLOQ_TABLA ".tmp");
    }
    fechar(fd);
    registrar("conf_maquina: migrada la tabla del bloqueo DNS: %d", r);
}

int maquina_migrar(void)
{
    if (!sou_root() || !conf_persistente() || maquina_separada()) return 0;
    /* Los archivos sueltos primero; /etc/sistema.conf, que es la marca, lo último: si se corta a medias, el
     * siguiente arranque lo vuelve a hacer entero (copias, nada se borra). */
    migrar_arquivo("/home/.zona", "zona");
    migrar_arquivo("/home/.frecuencia", "frecuencia");
    migrar_arquivo("/home/.config/pantallas.conf", "pantallas.conf");
    migrar_arquivo("/home/.config/sonido.conf", "sonido.conf");
    migrar_arquivo("/home/.config/wifi-desempenho.conf", "wifi-desempenho.conf");
    migrar_arquivo("/home/.config/bloqueo/bloqueo.conf", "bloqueo.conf");
    migrar_tabla("/home/.config/bloqueo/tabla.bin");
    /* Las claves de la máquina de sagui.conf (el de sagui: lo que valía hasta ahora para toda la máquina). */
    static char t[3072], m[1024];
    int n = ler_pequeno("/home/.config/sagui.conf", t, sizeof t);
    int u = formatar_texto(m, sizeof m, "# Configuración de la máquina (D371): solo la cambia el administrador.\n"
                                        "# Migrada de /home/.config/sagui.conf.\n");
    for (char *p = n > 0 ? t : (char *)""; *p;) {
        char *fim = acha_char(p, '\n');
        int tl = fim ? (int)(fim - p) : (int)tam_texto(p);
        if (tl && p[tl - 1] == '\r') tl--;
        char *eq = p;
        while (eq < p + tl && *eq != '=') eq++;
        char k[32];
        int tk = (int)(eq - p);
        if (p[0] != '#' && eq < p + tl && tk > 0 && tk < (int)sizeof k) {
            memcpy(k, p, (size_t)tk);
            k[tk] = 0;
            if (maquina_clave(k) && u + tl + 2 < (int)sizeof m) {
                memcpy(m + u, p, (size_t)tl);
                u += tl;
                m[u++] = '\n';
            }
        }
        if (!fim) break;
        p = fim + 1;
    }
    m[u] = 0;
    int r = conf_gravar(MAQ_CONF, m, u);
    registrar("conf_maquina: migracion de la configuracion de la maquina a /etc: %d", r);
    return r < 0 ? r : 1;
}
