// SPDX-License-Identifier: GPL-2.0-or-later
/* instalar.c - validar, instalar y quitar fuentes del sistema (D356).
 *
 * Lo usan el programa «fuentes» (terminal y ventana) y, por tanto, Archivos y Configuración.
 * Una fuente solo se copia si la libfontes la abre (TrueType u OpenType-CFF), trae algún glifo
 * de texto y sale con un nombre de familia: un archivo dañado se rechaza sin tocar nada.
 * Las carpetas son FUENTES_DIR_HOME (cada cuenta: /home/fuentes, o /home/<cuenta>/fuentes si el
 * entorno trae HOME) y FUENTES_DIR_APPS (todas las cuentas, hace falta ser root). */
#include "sagui.h"
#include "fontes.h"

const char *fuentes_error_texto(int r)
{
    switch (r) {
    case FUENTES_OK:        return "bien";
    case FUENTES_E_NOEXISTE: return "el archivo no existe o no se puede leer";
    case FUENTES_E_EXT:     return "solo se instalan archivos .ttf y .otf";
    case FUENTES_E_TAM:     return "el archivo es demasiado pequeño o demasiado grande";
    case FUENTES_E_DANADA:  return "no es una fuente válida (archivo dañado)";
    case FUENTES_E_VACIA:   return "la fuente no trae glifos de texto";
    case FUENTES_E_COPIA:   return "no se pudo copiar a la carpeta de destino";
    case FUENTES_E_PERMISO: return "sin permiso para escribir en la carpeta de destino";
    }
    return "error desconocido";
}

static bool ext_fuente(const char *n)
{
    size_t l = tam_texto(n);
    if (l < 5) return false;
    const char *e = n + l - 4;
    return e[0] == '.' && (((e[1] | 32) == 'o' && (e[2] | 32) == 't' && (e[3] | 32) == 'f') ||
                           ((e[1] | 32) == 't' && (e[2] | 32) == 't' && (e[3] | 32) == 'f'));
}

static const char *base_de(const char *ruta)
{
    const char *b = ruta;
    for (const char *p = ruta; *p; p++) if (*p == '/') b = p + 1;
    return b;
}

void fuentes_dir_usuario(char *dst, int n)
{
    const char *h = obter_env("HOME");
    size_t l = h ? tam_texto(h) : 0;
    if (h && l > 6 && l < 48 && compara_texto_n(h, "/home/", 6) == 0) formatar_texto(dst, n, "%s/fuentes", h);
    else copia_texto(dst, FUENTES_DIR_HOME, n);
}

int fuentes_validar(const char *ruta, char *familia, int nf, char *estilo, int ne)
{
    if (familia && nf > 0) familia[0] = 0;
    if (estilo && ne > 0) estilo[0] = 0;
    if (!ext_fuente(ruta)) return FUENTES_E_EXT;
    info_arquivo_t inf;
    if (info_arquivo(ruta, &inf) < 0) return FUENTES_E_NOEXISTE;
    if (inf.tamanho < 256 || inf.tamanho > (16u << 20)) return FUENTES_E_TAM;
    fonte_t *f = fonte_abrir_archivo(ruta);
    if (!f) return FUENTES_E_DANADA;
    char fam[48], est[24];
    fam[0] = est[0] = 0;
    fonte_nombre(f, fam, sizeof fam, est, sizeof est);
    if (!fam[0]) copia_texto(fam, base_de(ruta), sizeof fam);   /* sin tabla name: el nombre del archivo */
    int glifos = 0;
    static const unsigned prueba[] = { 'A', 'a', 'e', 'o', '0' };
    for (unsigned i = 0; i < sizeof prueba / sizeof prueba[0]; i++) {
        if (!fonte_tem(f, prueba[i])) continue;
        int l, a, ox, oy;
        if (fonte_glifo(f, prueba[i], 24, &l, &a, &ox, &oy)) glifos++;
    }
    fonte_fechar(f);
    if (!glifos) return FUENTES_E_VACIA;
    if (familia) copia_texto(familia, fam, nf);
    if (estilo) copia_texto(estilo, est, ne);
    return FUENTES_OK;
}

static int crear_arbol(const char *dir)
{
    info_arquivo_t inf;
    if (info_arquivo(dir, &inf) == 0) return 0;
    char t[96];
    copia_texto(t, dir, sizeof t);
    for (char *p = t + 1; *p; p++) {
        if (*p != '/') continue;
        *p = 0;
        if (info_arquivo(t, &inf) < 0) criar_pasta(t);
        *p = '/';
    }
    int r = criar_pasta(t);
    return r == 0 || info_arquivo(t, &inf) == 0 ? 0 : r;
}

int fuentes_instalar(const char *origen, const char *dir, char *familia, int nf)
{
    char fam[48];
    int r = fuentes_validar(origen, fam, sizeof fam, NULL, 0);
    if (r != FUENTES_OK) return r;
    if (familia && nf > 0) copia_texto(familia, fam, nf);
    if (crear_arbol(dir) < 0) return FUENTES_E_PERMISO;
    char dest[160], tmp[160];
    formatar_texto(dest, sizeof dest, "%s/%s", dir, base_de(origen));
    formatar_texto(tmp, sizeof tmp, "%s/.instalando", dir);
    int fi = abrir(origen, ABRIR_LER);
    if (fi < 0) return FUENTES_E_NOEXISTE;
    int fo = abrir(tmp, ABRIR_ESCREVER | ABRIR_CRIAR | ABRIR_TRUNCAR);
    if (fo < 0) { fechar(fi); return FUENTES_E_PERMISO; }
    static uint8_t buf[8192];
    int ok = 1, n;
    while ((n = ler(fi, buf, sizeof buf)) > 0) if (escrever(fo, buf, (unsigned)n) != n) { ok = 0; break; }
    if (n < 0) ok = 0;
    fechar(fi);
    fechar(fo);
    if (!ok) { apagar(tmp); return FUENTES_E_COPIA; }
    apagar(dest);
    if (renomear(tmp, dest) < 0) { apagar(tmp); return FUENTES_E_COPIA; }
    mudar_permisos(dest, 0644);
    return FUENTES_OK;
}

int fuentes_instalar_carpeta(const char *carpeta, const char *dir, int *malas)
{
    int bien = 0;
    entrada_pasta_t e;
    if (malas) *malas = 0;
    for (int i = 0; listar(carpeta, i, &e) > 0; i++) {
        if (e.tipo == TIPO_PASTA || !ext_fuente(e.nome)) continue;
        char ruta[200];
        formatar_texto(ruta, sizeof ruta, "%s/%s", carpeta, e.nome);
        if (fuentes_instalar(ruta, dir, NULL, 0) == FUENTES_OK) bien++;
        else if (malas) (*malas)++;
    }
    return bien;
}

static bool igual_sc(const char *a, const char *b)
{
    for (; *a && *b; a++, b++) if ((*a | 32) != (*b | 32)) return false;
    return *a == *b;
}

int fuentes_quitar(const char *dir, const char *familia, int *errores)
{
    static fuente_info_t cat[FUENTES_MAX];
    int nc = fuentes_catalogo_dir(dir, cat, FUENTES_MAX), quitadas = 0;
    if (errores) *errores = 0;
    for (int i = 0; i < nc; i++) {
        /* la familia pedida, o «Familia Estilo» (el nombre con que la lista agrupa lo que no es negrita ni cursiva) */
        char nom[80];
        formatar_texto(nom, sizeof nom, "%s %s", cat[i].familia, cat[i].estilo);
        if (!igual_sc(cat[i].familia, familia) && !igual_sc(nom, familia)) continue;
        if (apagar(cat[i].ruta) == 0) quitadas++;
        else if (errores) (*errores)++;
    }
    return quitadas;
}
