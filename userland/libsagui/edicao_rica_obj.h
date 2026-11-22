// SPDX-License-Identifier: GPL-2.0-or-later
/* edicao_rica_obj.h - interno: datos de los objetos del control de texto (mejora 122, D296). */
#ifndef SAGUI_EDICAO_RICA_OBJ_H
#define SAGUI_EDICAO_RICA_OBJ_H
#include "gui.h"

struct rte_obj_dados {
    int ref;
    imagem_t img;                 /* píxeles de pantalla (como mucho 1600 por lado) */
    uint8_t *blob; int n_blob, blob_tipo, blob_l, blob_a;   /* JPEG o PNG original */
    imagem_t cache; int cache_l, cache_a;                    /* la imagen ya escalada al tamaño en pantalla */
};

rte_obj_dados_t *obj_dados_ref(rte_obj_dados_t *d);
void obj_dados_unref(rte_obj_dados_t *d);
rte_obj_t *obj_clonar(const rte_obj_t *o);      /* copia con una referencia más a los datos */
void obj_liberar(rte_obj_t *o);
/* Dibuja los datos escalados a l x a en (x, y) de la ventana, dentro del recorte. */
void obj_blit(jan_t *j, rte_obj_dados_t *d, int x, int y, int l, int a, int cx0, int cy0, int cx1, int cy1);
/* JPEG: dimensiones y número de componentes (1 o 3) leídos del marcador SOF; false si no es JPEG baseline. */
bool obj_jpeg_info(const uint8_t *d, int n, int *l, int *a, int *ncomp);

#endif
