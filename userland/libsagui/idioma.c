// SPDX-License-Identifier: GPL-2.0-or-later
/* idioma.c - _("texto") y los catálogos de traducción de los programas (D313, mejora 118).
 *
 * La cadena en español es la clave. El idioma sale de LANG (el entorno manda: «LANG=en arquivos»)
 * y, si no hay, del idioma del sistema (SIS_IDIOMA), que el kernel recibe de
 * «configuracion --aplicar» al arrancar. Con el español no se carga nada.
 * Catálogos (generados por tools/extraer_textos.py desde los .po):
 *   /sistema/idiomas/<idioma>/<programa>.cat   los textos del programa
 *   /sistema/idiomas/<idioma>/libsagui.cat     los de la libsagui (diálogo de archivos, componentes)
 * Si falta la traducción sale el español. Con un idioma que no es el español, lo que no se
 * encuentra se anota una vez en el registro («[IDIOMA] sin traducir»): la prueba h313 lo usa. */
#include "sagui.h"
#include "catalogo.h"
#include "contas.h"

extern char **__argv_inicial;   /* lo guarda inicio.asm */

#ifdef SAGUI_PORTABLE
/* D459: en la capa portable (SDL 2) no hay kernel al que preguntar; la capa contesta (LANG manda). */
int portavel_idioma(unsigned op, const void *p, unsigned n);
#define CHAMAR_IDIOMA(op, p, n) portavel_idioma((op), (p), (n))
#else
static inline int chamar(unsigned n, unsigned a, unsigned b, unsigned c)
{
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c) : "memory");
    return r;
}
#define CHAMAR_IDIOMA(op, p, n) chamar(SIS_IDIOMA, (op), (unsigned)(p), (n))
#endif

#define VISTOS 64

static bool iniciado;
static bool activo;                 /* hay algún catálogo cargado */
static char nome[IDIOMA_MAX] = "es";
static catalogo_t c_prog, c_lib;
static bool tem_prog, tem_lib;
static uint32_t vistos[VISTOS];     /* hashes ya anotados como «sin traducir» */
static int n_vistos;

/* Lee un archivo entero a memoria; NULL si no existe. */
static uint8_t *cargar_arq(const char *caminho, uint32_t *tam)
{
    info_arquivo_t ia;
    if (info_arquivo(caminho, &ia) < 0 || ia.tamanho == 0 || ia.tamanho > 1024 * 1024) return NULL;
    int fd = abrir(caminho, ABRIR_LER);
    if (fd < 0) return NULL;
    uint8_t *d = aloca(ia.tamanho);
    uint32_t n = 0;
    while (d && n < ia.tamanho) {
        int r = ler(fd, d + n, ia.tamanho - n);
        if (r <= 0) break;
        n += (uint32_t)r;
    }
    fechar(fd);
    if (!d || n != ia.tamanho) return NULL;
    *tam = n;
    return d;
}

static bool cargar_cat(const char *prog, catalogo_t *c)
{
    char caminho[96];
    uint32_t tam;
    formatar_texto(caminho, sizeof caminho, "/sistema/idiomas/%s/%s.cat", nome, prog);
    uint8_t *d = cargar_arq(caminho, &tam);
    return d && catalogo_abrir(c, d, tam) == 0;
}

/* Carga los catálogos del idioma "nome" (ya normalizado). */
static void cargar_todo(void)
{
    activo = tem_prog = tem_lib = false;
    n_vistos = 0;
    if (compara_texto(nome, "es") == 0) return;
    char prog[48] = "";
    const char *a0 = __argv_inicial ? __argv_inicial[0] : NULL;
    if (a0) {
        const char *b = a0;
        for (const char *q = a0; *q; q++) if (*q == '/') b = q + 1;
        copia_texto(prog, b, sizeof prog);
    }
    if (prog[0]) tem_prog = cargar_cat(prog, &c_prog);
    tem_lib = cargar_cat("libsagui", &c_lib);
    activo = tem_prog || tem_lib;
}

static void iniciar(void)
{
    iniciado = true;
    const char *l = obter_env("LANG");
    char sis[IDIOMA_MAX];
    if (!l || !*l) {
        sis[0] = 0;
        CHAMAR_IDIOMA(IDIOMA_LER, sis, sizeof sis);
        l = sis;
    }
    catalogo_idioma_norm(l, nome);
    cargar_todo();
}

static void anotar(const char *s)
{
    bool letra = false;
    for (const char *p = s; *p; p++) if ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (uint8_t)*p >= 0xC0) { letra = true; break; }
    if (!letra) return;
    uint32_t h = catalogo_hash(s);
    for (int i = 0; i < n_vistos; i++) if (vistos[i] == h) return;
    if (n_vistos < VISTOS) vistos[n_vistos++] = h;
    registrar("[IDIOMA] sin traducir (%s): %s", nome, s);
}

const char *idioma_traducir(const char *clave)
{
    if (!iniciado) iniciar();
    if (!clave || compara_texto(nome, "es") == 0) return clave;
    const char *t = NULL;
    if (tem_prog) t = catalogo_buscar(&c_prog, clave);
    if (!t && tem_lib) t = catalogo_buscar(&c_lib, clave);
    if (t) return t;
    anotar(clave);
    return clave;
}

/* Para los componentes de la libsagui (botones, diálogos): traduce si puede y calla si no. */
const char *idioma_traducir_sl(const char *clave)
{
    if (!iniciado) iniciar();
    if (!clave || !activo) return clave;
    const char *t = NULL;
    if (tem_prog) t = catalogo_buscar(&c_prog, clave);
    if (!t && tem_lib) t = catalogo_buscar(&c_lib, clave);
    return t ? t : clave;
}

/* Menús de una pieza («Nuevo\tCtrl+N\n-\nAbrir...»): cada línea se traduce por separado (lo que va antes
 * del tabulador; el atajo se deja) y se devuelve el menú entero en un búfer rotatorio. Con el español
 * (o sin catálogo) devuelve el mismo texto. Cada línea es una cadena del catálogo del programa. */
const char *idioma_traducir_menu(const char *menu)
{
    if (!iniciado) iniciar();
    if (!menu || !activo) return menu;
    static char buf[4][1536];
    static int turno;
    char *dst = buf[turno++ & 3];
    int k = 0, max = (int)sizeof buf[0];
    const char *p = menu;
    while (*p && k < max - 1) {
        const char *fin = p;
        while (*fin && *fin != '\n' && *fin != '\t') fin++;
        char linha[96];
        int n = (int)(fin - p);
        if (n >= (int)sizeof linha) { copia_texto(dst, menu, (size_t)max); return dst; }
        for (int i = 0; i < n; i++) linha[i] = p[i];
        linha[n] = 0;
        const char *t = (n && !(n == 1 && linha[0] == '-')) ? idioma_traducir(linha) : linha;
        for (; *t && k < max - 1; t++) dst[k++] = *t;
        p = fin;
        while (*p && *p != '\n' && k < max - 1) dst[k++] = *p++;   /* el atajo (\tCtrl+N) */
        if (*p == '\n' && k < max - 1) { dst[k++] = '\n'; p++; }
    }
    dst[k] = 0;
    return dst;
}

void idioma_normalizar(const char *entrada, char *salida) { catalogo_idioma_norm(entrada, salida); }

int idioma_sistema_ler(char *dst, int max)
{
    int fd = abrir("/etc/idioma", ABRIR_LER);
    if (fd < 0) return -1;
    char b[32];
    int n = ler(fd, b, sizeof b - 1);
    fechar(fd);
    if (n <= 0) return -1;
    b[n] = 0;
    for (int i = 0; i < n; i++) if (b[i] == '\n' || b[i] == '\r') { b[i] = 0; break; }
    if (!b[0]) return -1;
    char norm[IDIOMA_MAX];
    catalogo_idioma_norm(b, norm);
    copia_texto(dst, norm, (size_t)max);
    return 0;
}

int idioma_sistema_gravar(const char *nombre)
{
    char norm[IDIOMA_MAX], b[24];
    catalogo_idioma_norm(nombre, norm);
    int n = formatar_texto(b, sizeof b, "%s\n", norm);
    return conf_gravar("idioma", b, n);
}

const char *idioma_actual(void)
{
    if (!iniciado) iniciar();
    return nome;
}

void idioma_recargar(void) { iniciar(); }

int idioma_fijar(const char *nombre)
{
    int r = CHAMAR_IDIOMA(IDIOMA_FIXAR, nombre, 0);
    if (r < 0) return r;
    /* Este proceso sigue lo que se fijó, aunque tenga LANG en el entorno. */
    iniciado = true;
    catalogo_idioma_norm(nombre, nome);
    cargar_todo();
    return 0;
}
