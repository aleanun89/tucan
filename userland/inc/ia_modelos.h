// SPDX-License-Identifier: GPL-3.0-or-later
/* ia_modelos.h - el descargador de modelos de la IA local (D380, fase 2).
 *
 * La lista de modelos que se pueden bajar no va en el código: está en /etc/ia/modelos.lista
 * (una línea por modelo, campos separados por «|»), con su URL de Hugging Face, el tamaño y
 * la suma SHA-256 fijos. La usan `ia descargar` (terminal) y Configuración > IA (que lanza
 * `ia descargar ... --estado FICHERO` y enseña la barra de avance que ese fichero dice).
 *
 * La descarga va por HTTPS con el TLS propio (libtls, D062), por trozos con «Range» (así
 * cada respuesta es pequeña y una descarga cortada sigue donde iba: el fichero a medias es
 * NOMBRE.gguf.parte), comprueba el espacio libre antes de empezar y la suma al acabar (si no
 * coincide, se borra lo bajado). Destino de serie: /home/compartido/modelos (D380). */
#ifndef SAGUI_IA_MODELOS_H
#define SAGUI_IA_MODELOS_H
#include "sagui.h"

#define IA_CATALOGO        "/etc/ia/modelos.lista"
#define IA_PASTA_MODELOS   "/home/compartido/modelos"
#define IA_CAT_MAX         32
#define IA_CAT_URL_MAX     512

typedef struct {
    char id[32];                  /* «lfm3-1.7b-q4km» */
    char nome[96];                /* lo que se enseña */
    char ficheiro[96];            /* nombre en la carpeta de modelos */
    unsigned long long bytes;     /* tamaño exacto */
    char sha256[65];              /* en hexadecimal (minúsculas); vacío = pendiente de comprobar */
    char url[IA_CAT_URL_MAX];
    unsigned ram_mb;              /* memoria recomendada para el equipo */
    char extra[32];               /* id de otro fichero que va con este (el mmproj de un VL) o vacío */
    char nota[160];
} ia_modelo_cat_t;

/* Lee la lista (NULL = IA_CATALOGO, o la de IA_CATALOGO en el entorno). -> cuántos (<0 error) */
int ia_catalogo_ler(const char *caminho, ia_modelo_cat_t *v, int max);
/* El modelo con ese id, o NULL. */
const ia_modelo_cat_t *ia_catalogo_buscar(const ia_modelo_cat_t *v, int n, const char *id);

/* Estado de un modelo en la carpeta: 0 no está, 1 a medias (*feito bytes), 2 completo. */
int ia_modelo_estado(const ia_modelo_cat_t *m, const char *pasta, unsigned long long *feito);

typedef struct {
    /* fase: 0 bajando, 1 comprobando la suma. kbs: velocidad (0 = no se sabe). */
    void (*progresso)(void *ctx, int fase, unsigned long long feito, unsigned long long total, unsigned kbs);
    int  (*parar)(void *ctx);     /* 1 = cancelar (lo bajado se queda para seguir después) */
    void (*aviso)(void *ctx, const char *texto);
    void *ctx;
    bool inseguro;                /* -k: sin validar el certificado (solo para pruebas) */
} ia_desc_cb_t;

/* Baja un modelo a «pasta» (NULL = IA_PASTA_MODELOS). 0 = completo y con la suma correcta;
 * <0 = error (texto en erro); 1 = cancelado (se puede seguir). */
int ia_modelo_baixar(const ia_modelo_cat_t *m, const char *pasta, const ia_desc_cb_t *cb, char *erro, int max);

/* Espacio libre (KB) del sistema de archivos donde está «caminho» (0 = no se sabe). */
unsigned long long ia_espaco_livre_kb(const char *caminho);

#endif
