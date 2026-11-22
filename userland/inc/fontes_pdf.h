// SPDX-License-Identifier: GPL-2.0-or-later
/* fontes_pdf.h - datos de una fuente para incrustarla en un PDF (D319): lo que pide el
 * FontDescriptor y el subconjunto TrueType (userland/libfontes/subconjunto.c). Sin dependencias,
 * para poder compilarlo también en el PC (prueba de unidad). */
#ifndef FONTES_PDF_H
#define FONTES_PDF_H
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char psname[64];                 /* nombre PostScript sin espacios */
    int  flags, bbox[4], angulo, ascent, descent, capheight, stemv;   /* en milésimas de em */
    bool oblicua;                    /* la cursiva es sintética (la familia no tiene): el PDF inclina el texto 12 grados */
    bool cff;                        /* OpenType con contornos CFF (FontFile3) en vez de TrueType (FontFile2) */
} pdfw_finfo_t;

/* Lee la cabecera del sfnt; false si no es una fuente. ruta solo sirve para inventar el nombre. */
bool fonte_info_pdf(const uint8_t *d, unsigned n, const char *ruta, pdfw_finfo_t *o);
/* Subconjunto TrueType con los bytes WinAnsi marcados en usadas[32] (bit b&7 del byte b>>3).
 * *out se reserva con aloca (libera). false = no es TrueType con glyf o sin memoria. */
bool fonte_subconjunto_ttf(const uint8_t *d, unsigned n, const uint8_t *usadas, uint8_t **out, unsigned *nout);
#endif
