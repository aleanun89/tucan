// SPDX-License-Identifier: GPL-2.0-or-later
/* familias.c - proveedor de fuentes de documento para Pluma (D319).
 *
 * Registra las familias del sistema (fuentes_sistema_listar: /fontes, /apps/fuentes y /home/fuentes)
 * con un número estable de 3 en adelante (las ya registradas no cambian aunque se añadan más), y se
 * lo da a la libsagui (fuentes_prov_instalar) para que el control rte mida con los anchos reales,
 * dibuje con la fuente vectorial con antialias y el PDF incruste la que se use. Las familias
 * clásicas 0..2 (Arial, Times New Roman y Courier New) se dibujan con DejaVu Sans, Serif y Mono,
 * escaladas para que el ancho medio coincida con Helvetica, Times y Courier (con esas medidas se
 * maqueta y se pagina el PDF, así que pantalla y PDF coinciden). Todo se carga cuando hace falta;
 * una fuente que no se puede abrir da -1 / NULL y los llamadores vuelven a la 8x16 o a Helvetica. */
#include "sagui.h"
#include "fontes.h"
#include "pdf_escribir.h"

#define ID_MAX (PDFW_FAM_MAX + 1)
#define ANCHO_PEND (-2)

static familia_fuente_t reg[ID_MAX];     /* reg[id]: las familias del sistema (los ids 0..2 no se usan) */
static int nreg = PDFW_FAM_SIS;          /* mayor id + 1 */
static int clasica_id[3] = { -1, -1, -1 };

typedef struct {
    fonte_t *f;
    bool tentou, obl;
    int  idx;                            /* qué archivo de la familia se usó (0..3) */
} cargada_t;
static cargada_t cargadas[ID_MAX][4];
static int16_t *anchos[ID_MAX][4];
static int esc_clasica[3][4];            /* milésimas; 0 = sin calcular */

static bool igual_sc(const char *a, const char *b)
{
    for (; *a && *b; a++, b++) if ((*a | 32) != (*b | 32)) return false;
    return *a == *b;
}

static int buscar_nombre(const char *n)
{
    for (int i = PDFW_FAM_SIS; i < nreg; i++) if (reg[i].familia[0] && igual_sc(reg[i].familia, n)) return i;
    return -1;
}

static void enlazar_clasicas(void)
{
    static const char *const dv[3] = { "DejaVu Sans", "DejaVu Serif", "DejaVu Sans Mono" };
    for (int i = 0; i < 3; i++) clasica_id[i] = buscar_nombre(dv[i]);
}

/* Lee las carpetas y registra lo nuevo; devuelve cuántas familias nuevas. */
static int escanear(void)
{
    static familia_fuente_t v[ID_MAX];
    int n = fuentes_sistema_listar(v, ID_MAX - PDFW_FAM_SIS), nuevas = 0;
    for (int i = 0; i < n; i++) {
        int id = buscar_nombre(v[i].familia);
        if (id < 0) {
            if (nreg >= ID_MAX) continue;
            id = nreg++;
            reg[id] = v[i];
            nuevas++;
        } else {                           /* ya conocida: se completan los estilos que faltaban */
            for (int e = 0; e < 4; e++) if (!reg[id].ruta[e][0] && v[i].ruta[e][0]) { copia_texto(reg[id].ruta[e], v[i].ruta[e], sizeof reg[id].ruta[e]); cargadas[id][e].tentou = false; }
        }
    }
    /* D356: lo que ya no está en las carpetas (fuente quitada) se olvida: el nombre y el número se conservan (por si
     * se vuelve a instalar), pero sin archivos la familia no aparece en las listas y se dibuja y exporta como la clásica. */
    for (int id = PDFW_FAM_SIS; id < nreg; id++)
        for (int e = 0; e < 4; e++) {
            if (!reg[id].ruta[e][0]) continue;
            bool sigue = false;
            for (int i = 0; i < n && !sigue; i++) sigue = igual_sc(v[i].ruta[e], reg[id].ruta[e]);
            if (!sigue) { reg[id].ruta[e][0] = 0; cargadas[id][e].tentou = false; }
        }
    enlazar_clasicas();
    return nuevas;
}

static int rid_de(int fam) { return fam >= 0 && fam < 3 ? clasica_id[fam] : fam; }

/* El archivo de la familia id para el estilo est (N/C) y si hay que inclinarlo: se prueba el estilo
 * pedido, luego sin cursiva, sin negrita y la regular. */
static int elegir_archivo(int id, int est, bool *obl)
{
    int cand[4] = { est, est & 1, est & 2, 0 };
    for (int i = 0; i < 4; i++) {
        int c = cand[i];
        if (reg[id].ruta[c][0]) { *obl = (est & 2) && !(c & 2); return c; }
    }
    return -1;
}

static fonte_t *fuente_de(int id, int est)
{
    if (id < PDFW_FAM_SIS || id >= nreg || !reg[id].familia[0]) return NULL;
    cargada_t *c = &cargadas[id][est & 3];
    if (c->tentou) return c->f;
    c->tentou = true;
    bool obl = false;
    int idx = elegir_archivo(id, est & 3, &obl);
    if (idx < 0) return NULL;
    fonte_t *f = fonte_abrir_archivo(reg[id].ruta[idx]);
    if (f && obl) { fonte_t *o = fonte_obliqua(f); if (o) f = o; }
    c->f = f; c->obl = obl; c->idx = idx;
    return f;
}

static int ancho_real(int fam, int est, unsigned char c)
{
    int id = fam;
    fonte_t *f = fuente_de(id, est);
    if (!f) return -1;
    int16_t **t = &anchos[id][est & 3];
    if (!*t) {
        *t = aloca(256 * sizeof(int16_t));
        if (!*t) return fonte_avanco(f, c, 500) / 32;
        for (int i = 0; i < 256; i++) (*t)[i] = ANCHO_PEND;
    }
    if ((*t)[c] == ANCHO_PEND) (*t)[c] = (int16_t)(fonte_avanco(f, c, 500) / 32);   /* 1/64 de píxel a 500 px -> milésimas de em */
    return (*t)[c];
}

static int p_ancho(int fam, int est, unsigned char c)
{
    if (fam < PDFW_FAM_SIS) return -1;
    return ancho_real(fam, est & 3, c);
}

/* Escala de DejaVu para que su ancho medio (ASCII imprimible) iguale al de la fuente estándar. */
static int escala_clasica(int fam, int est)
{
    int *e = &esc_clasica[fam][est & 3];
    if (*e) return *e;
    long sh = 0, sd = 0;
    for (int c = 33; c < 127; c++) {
        int d = ancho_real(clasica_id[fam], est & 3, (unsigned char)c);
        if (d < 0) break;
        sh += pdfw_largura_char(fam, est & 3, (unsigned char)c);
        sd += d;
    }
    int v = sd > 0 ? (int)(sh * 1000 / sd) : 1000;
    if (v < 800) v = 800;
    if (v > 1100) v = 1100;
    return *e = v;
}

static const uint8_t *p_glifo(int fam, int est, unsigned char c, int px, int *l, int *a, int *ox, int *oy)
{
    int id = rid_de(fam);
    if (id < PDFW_FAM_SIS) return NULL;
    fonte_t *f = fuente_de(id, est & 3);
    if (!f || !fonte_tem(f, c)) return NULL;
    if (fam < 3) px = px * escala_clasica(fam, est) / 1000;
    if (px < 3) px = 3;
    return fonte_glifo(f, c, px, l, a, ox, oy);
}

static uint8_t *leer_archivo(const char *ruta, unsigned *n)
{
    info_arquivo_t inf;
    if (info_arquivo(ruta, &inf) < 0 || inf.tamanho < 64 || inf.tamanho > (16u << 20)) return NULL;
    int fd = abrir(ruta, ABRIR_LER);
    if (fd < 0) return NULL;
    uint8_t *d = aloca(inf.tamanho);
    if (!d) { fechar(fd); return NULL; }
    unsigned got = 0;
    int r;
    while (got < inf.tamanho && (r = ler(fd, d + got, inf.tamanho - got)) > 0) got += (unsigned)r;
    fechar(fd);
    if (got != inf.tamanho) { libera(d); return NULL; }
    *n = got;
    return d;
}

static bool p_info(int fam, int est, pdfw_finfo_t *o)
{
    if (fam < PDFW_FAM_SIS || fam >= nreg) return false;
    bool obl = false;
    int idx = elegir_archivo(fam, est & 3, &obl);
    if (idx < 0) return false;
    unsigned n;
    uint8_t *d = leer_archivo(reg[fam].ruta[idx], &n);
    if (!d) return false;
    bool ok = fonte_info_pdf(d, n, reg[fam].ruta[idx], o);
    libera(d);
    if (!ok) return false;
    o->oblicua = obl;
    if (obl) { o->flags |= 64; o->angulo = -12; }
    return true;
}

static uint8_t *p_archivo(int fam, int est, const uint8_t *usadas, int *n, int *original)
{
    if (fam < PDFW_FAM_SIS || fam >= nreg) return NULL;
    bool obl = false;
    int idx = elegir_archivo(fam, est & 3, &obl);
    if (idx < 0) return NULL;
    unsigned nd;
    uint8_t *d = leer_archivo(reg[fam].ruta[idx], &nd);
    if (!d) return NULL;
    uint8_t *sub = NULL;
    unsigned ns = 0;
    if (fonte_subconjunto_ttf(d, nd, usadas, &sub, &ns)) { libera(d); *n = (int)ns; *original = (int)ns; return sub; }
    *n = (int)nd; *original = (int)nd;      /* OpenType CFF (o algo que no sabemos recortar): la fuente entera */
    return d;
}

static int p_cantidad(void) { return nreg; }
static const char *p_nombre(int fam)
{
    if (fam < PDFW_FAM_SIS || fam >= nreg || !reg[fam].familia[0]) return NULL;
    for (int e = 0; e < 4; e++) if (reg[fam].ruta[e][0]) return reg[fam].familia;
    return NULL;                                   /* la fuente se quitó */
}
static int p_resolver(const char *nombre) { return buscar_nombre(nombre); }

/* La clásica más parecida a una familia: por el nombre y, si no, sans. */
static int p_clase(int fam)
{
    if (fam < PDFW_FAM_SIS || fam >= nreg) return fam >= 0 && fam < 3 ? fam : 0;
    const char *n = reg[fam].familia;
    for (int i = 0; n[i]; i++) {
        const char *s = n + i;
        if (((s[0] | 32) == 'm' && (s[1] | 32) == 'o' && (s[2] | 32) == 'n' && (s[3] | 32) == 'o') ||
            ((s[0] | 32) == 'c' && (s[1] | 32) == 'o' && (s[2] | 32) == 'u' && (s[3] | 32) == 'r') ||
            ((s[0] | 32) == 'c' && (s[1] | 32) == 'o' && (s[2] | 32) == 'd' && (s[3] | 32) == 'e')) return 2;
        if (((s[0] | 32) == 's' && (s[1] | 32) == 'e' && (s[2] | 32) == 'r' && (s[3] | 32) == 'i' && (s[4] | 32) == 'f') ||
            ((s[0] | 32) == 't' && (s[1] | 32) == 'i' && (s[2] | 32) == 'm' && (s[3] | 32) == 'e') ||
            ((s[0] | 32) == 'g' && (s[1] | 32) == 'a' && (s[2] | 32) == 'r' && (s[3] | 32) == 'a') ||
            ((s[0] | 32) == 'b' && (s[1] | 32) == 'o' && (s[2] | 32) == 'o' && (s[3] | 32) == 'k')) {
            if (!((i >= 3) && (n[i - 1] | 32) == 's' && (n[i - 2] | 32) == 'n' && (n[i - 3] | 32) == 'a')) return 1;   /* «Sans Serif» es sans */
        }
    }
    return 0;
}

/* D327: los estilos reales de la familia (las clásicas se dibujan con DejaVu, que trae los cuatro). */
static int p_estilos(int fam)
{
    if (fam < 3) return 15;
    if (fam >= nreg) return 1;
    int m = 1;
    for (int e = 0; e < 4; e++) if (reg[fam].ruta[e][0]) m |= 1 << e;
    return m;
}

static const fuentes_prov_t proveedor = {
    p_ancho, p_info, p_archivo, p_glifo, p_cantidad, p_nombre, p_resolver, p_clase, escanear, p_estilos,
};

int fuentes_doc_actualizar(void) { return escanear(); }

void fuentes_doc_instalar(void)
{
    static bool hecho;
    if (hecho) return;
    hecho = true;
    escanear();
    fuentes_prov_instalar(&proveedor);
}
