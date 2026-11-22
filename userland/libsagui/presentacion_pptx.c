// SPDX-License-Identifier: GPL-2.0-or-later
/* presentacion_pptx.c - PPTX (OOXML PresentationML) de Tucán (D369): escritura y lectura.
 *
 * Escribe un paquete mínimo y válido: una plantilla, una maqueta, un tema, las diapositivas con sus textos, formas,
 * imágenes y tablas, las notas del orador y las transiciones. Lee lo mismo y lo razonable de lo que producen
 * PowerPoint y LibreOffice (textos, formas básicas, imágenes, tablas, fondos, notas); lo que no entiende se
 * ignora y queda anotado en el aviso. Los gráficos y las animaciones no se importan; los gráficos y WordArt de
 * Tucán se exportan como imagen o como texto. */
#include "presentacion.h"
#include "compressao.h"

#define EMU 12700

/* ------------------------------------------------------------ escritura */

static int xt(buf_t *b, const char *s) { return comp_buf_somar(b, s, (int)tam_texto(s)); }

static void xfmt(buf_t *b, const char *fmt, ...)
{
    char t[1600];
    va_list ap;
    va_start(ap, fmt);
    int n = formatar_lista(t, sizeof t, fmt, ap);
    va_end(ap);
    if (n > (int)sizeof t - 1) n = (int)sizeof t - 1;
    if (n > 0) comp_buf_somar(b, t, n);
}

/* Texto Latin-1 -> UTF-8 con las entidades de XML. */
static void xesc(buf_t *b, const char *s)
{
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        switch (*p) {
        case '&': xt(b, "&amp;"); break;
        case '<': xt(b, "&lt;"); break;
        case '>': xt(b, "&gt;"); break;
        case '"': xt(b, "&quot;"); break;
        default:
            if (*p < 0x20) break;
            if (*p < 0x80) comp_buf_somar(b, p, 1);
            else { char u[2] = { (char)(0xC0 | (*p >> 6)), (char)(0x80 | (*p & 0x3F)) }; comp_buf_somar(b, u, 2); }
        }
    }
}

static const char *const xml_cab = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
#define NS_A "xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\""
#define NS_R "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\""
#define NS_P "xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\""
#define RT "http://schemas.openxmlformats.org/officeDocument/2006/relationships/"

typedef struct {
    buf_t sl;                 /* el XML de la diapositiva */
    buf_t rels;               /* sus relaciones */
    pr_deck_t *d;
    int idsig, nrel;
    uint8_t *extra[40]; int extra_n[40]; int n_extra;   /* imágenes generadas (gráficos) */
} ew_t;

static int rid_nuevo(ew_t *w) { return ++w->nrel; }

static void ew_geom(buf_t *b, int x, int y, int cx, int cy, const char *tag)
{
    xfmt(b, "<%s><a:off x=\"%d\" y=\"%d\"/><a:ext cx=\"%d\" cy=\"%d\"/></%s>", tag, x * EMU, y * EMU, cx * EMU, cy * EMU, tag);
}

static const char *prst_de(int forma)
{
    switch (forma) {
    case PF_REDONDEADO: return "roundRect";
    case PF_ELIPSE: return "ellipse";
    case PF_TRIANGULO: return "triangle";
    case PF_ROMBO: return "diamond";
    case PF_FLECHA: return "rightArrow";
    case PF_LINEA: return "line";
    case PF_ESTRELLA: return "star5";
    default: return "rect";
    }
}

static void ew_relleno(buf_t *b, const char *tag, uint32_t c)
{
    xfmt(b, "<%s><a:srgbClr val=\"%06x\"/></%s>", tag, (unsigned)(c & 0xFFFFFF), tag);
}

/* El cuerpo de texto (txBody o la parte de texto de una celda). */
static void ew_cuerpo(buf_t *b, const pr_deck_t *d, const pr_obj_t *o, const char *texto, const char *tag, int valign, int tam)
{
    xfmt(b, "<%s><a:bodyPr wrap=\"square\" lIns=\"%d\" tIns=\"%d\" rIns=\"%d\" bIns=\"%d\" rtlCol=\"0\" anchor=\"%s\"><a:noAutofit/></a:bodyPr><a:lstStyle/>",
         tag, PR_INSET_X * EMU, PR_INSET_Y * EMU, PR_INSET_X * EMU, PR_INSET_Y * EMU, valign == 1 ? "ctr" : valign == 2 ? "b" : "t");
    const char *t = texto ? texto : "";
    uint32_t col = pr_color_texto(d, o);
    const char *fam = pr_fuente_obj(d, o);
    int len = (int)tam_texto(t), pos = 0;
    for (;;) {
        int fin = pos;
        while (fin < len && t[fin] != '\n') fin++;
        int s = pos, lvl = 0;
        while (s < fin && t[s] == '\t' && lvl < 8) { s++; lvl++; }
        xfmt(b, "<a:p><a:pPr algn=\"%s\" lvl=\"%d\"", o->alin == 1 ? "ctr" : o->alin == 2 ? "r" : "l", lvl);
        if (o->lista) xfmt(b, " marL=\"%d\" indent=\"-%d\"", (18 + lvl * 27) * EMU, 18 * EMU);
        xt(b, ">");
        if (o->lista == PR_LISTA_VINETA) xt(b, "<a:buFont typeface=\"Arial\"/><a:buChar char=\"&#8226;\"/>");
        else if (o->lista == PR_LISTA_NUMERO) xt(b, "<a:buFont typeface=\"+mj-lt\"/><a:buAutoNum type=\"arabicPeriod\"/>");
        else xt(b, "<a:buNone/>");
        xt(b, "</a:pPr>");
        if (fin > s) {
            xfmt(b, "<a:r><a:rPr lang=\"es-ES\" sz=\"%d\" b=\"%d\" i=\"%d\"%s dirty=\"0\">", tam * 100, (o->est & PR_NEGRITA) != 0, (o->est & PR_CURSIVA) != 0,
                 (o->est & PR_SUBRAYADO) ? " u=\"sng\"" : "");
            ew_relleno(b, "a:solidFill", col);
            xt(b, "<a:latin typeface=\""); xesc(b, fam); xt(b, "\"/></a:rPr><a:t>");
            char *tmp = aloca((size_t)(fin - s) + 1);
            if (tmp) { memcpy(tmp, t + s, (size_t)(fin - s)); tmp[fin - s] = 0; xesc(b, tmp); libera(tmp); }
            xt(b, "</a:t></a:r>");
        } else xfmt(b, "<a:endParaRPr lang=\"es-ES\" sz=\"%d\" dirty=\"0\"/>", tam * 100);
        xt(b, "</a:p>");
        if (fin >= len) break;
        pos = fin + 1;
    }
    xfmt(b, "</%s>", tag);
}

static void ew_obj(ew_t *w, pr_obj_t *o, int idx, int slide_n)
{
    buf_t *b = &w->sl;
    pr_deck_t *d = w->d;
    int id = ++w->idsig;
    switch (o->tipo) {
    case PR_TEXTO: case PR_FORMA: case PR_WORDART: {
        if (o->tipo == PR_FORMA && o->forma == PF_LINEA) {
            xfmt(b, "<p:cxnSp><p:nvCxnSpPr><p:cNvPr id=\"%d\" name=\"Linea %d\"/><p:cNvCxnSpPr/><p:nvPr/></p:nvCxnSpPr><p:spPr>", id, idx + 1);
            ew_geom(b, o->x, o->y + o->h / 2, o->w, 0, "a:xfrm");
            xfmt(b, "<a:prstGeom prst=\"line\"><a:avLst/></a:prstGeom><a:ln w=\"%d\">", (o->borde ? o->borde : 2) * EMU);
            ew_relleno(b, "a:solidFill", pr_color_borde(d, o));
            xt(b, "</a:ln></p:spPr></p:cxnSp>");
            break;
        }
        bool ph = o->rol != PR_ROL_LIBRE;
        xfmt(b, "<p:sp><p:nvSpPr><p:cNvPr id=\"%d\" name=\"", id);
        xesc(b, o->rol == PR_ROL_TITULO ? "Título" : o->rol == PR_ROL_LIBRE ? (o->tipo == PR_FORMA ? "Forma" : "Cuadro de texto") : "Contenido");
        xfmt(b, " %d\"/>", idx + 1);
        if (ph) {
            xt(b, "<p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr><p:ph");
            if (o->rol == PR_ROL_TITULO) xt(b, " type=\"title\"");
            else if (o->rol == PR_ROL_SUBTITULO) xt(b, " type=\"subTitle\" idx=\"1\"");
            else xt(b, " idx=\"1\"");
            xt(b, "/></p:nvPr>");
        } else xt(b, o->tipo == PR_FORMA ? "<p:cNvSpPr/><p:nvPr/>" : "<p:cNvSpPr txBox=\"1\"/><p:nvPr/>");
        xt(b, "</p:nvSpPr><p:spPr>");
        ew_geom(b, o->x, o->y, o->w, o->h, "a:xfrm");
        xfmt(b, "<a:prstGeom prst=\"%s\"><a:avLst/></a:prstGeom>", o->tipo == PR_FORMA ? prst_de(o->forma) : "rect");
        if (o->rellena) ew_relleno(b, "a:solidFill", pr_color_relleno(d, o)); else xt(b, "<a:noFill/>");
        if (o->borde) { xfmt(b, "<a:ln w=\"%d\">", o->borde * EMU); ew_relleno(b, "a:solidFill", pr_color_borde(d, o)); xt(b, "</a:ln>"); }
        else xt(b, "<a:ln><a:noFill/></a:ln>");
        xt(b, "</p:spPr>");
        if (o->tipo == PR_WORDART) {   /* el WordArt va como texto grande y en negrita (se pierde el efecto) */
            pr_obj_t t = *o;
            t.est |= PR_NEGRITA; t.tam = (uint8_t)(o->h * 6 / 10 > 200 ? 200 : o->h * 6 / 10 < 8 ? 8 : o->h * 6 / 10); t.alin = 1; t.valign = 1;
            t.col = pr_color_relleno(d, o) | 0; if (o->col & PR_COL_TEMA) t.col = (d->tema >= 0 ? pr_temas[d->tema].acento : 0x2E75B6);
            ew_cuerpo(b, d, &t, o->texto, "p:txBody", 1, t.tam);
        } else ew_cuerpo(b, d, o, o->texto, "p:txBody", o->valign, o->tam);
        xt(b, "</p:sp>");
        break;
    }
    case PR_IMAGEN: {
        if (o->medio < 0 || o->medio >= d->n_medios) break;
        int rid = rid_nuevo(w);
        xfmt(&w->rels, "<Relationship Id=\"rId%d\" Type=\"" RT "image\" Target=\"../media/image%d.%s\"/>", rid, o->medio + 1, d->medios[o->medio].ext);
        xfmt(b, "<p:pic><p:nvPicPr><p:cNvPr id=\"%d\" name=\"Imagen %d\"/><p:cNvPicPr><a:picLocks noChangeAspect=\"1\"/></p:cNvPicPr><p:nvPr/></p:nvPicPr>"
                "<p:blipFill><a:blip r:embed=\"rId%d\"/><a:stretch><a:fillRect/></a:stretch></p:blipFill><p:spPr>", id, idx + 1, rid);
        ew_geom(b, o->x, o->y, o->w, o->h, "a:xfrm");
        xt(b, "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom>");
        if (o->borde) { xfmt(b, "<a:ln w=\"%d\">", o->borde * EMU); ew_relleno(b, "a:solidFill", pr_color_borde(d, o)); xt(b, "</a:ln>"); }
        xt(b, "</p:spPr></p:pic>");
        break;
    }
    case PR_GRAFICO: {   /* el gráfico se exporta como imagen */
        int pw = o->w * 3, ph = o->h * 3;
        if (pw < 8 || ph < 8 || w->n_extra >= 40) break;
        uint32_t *px = aloca((size_t)pw * (size_t)ph * 4);
        if (!px) break;
        uint32_t fondo = pr_temas[d->tema].fondo1;
        for (int i = 0; i < pw * ph; i++) px[i] = fondo;
        pr_obj_t t = *o;
        t.x = 0; t.y = 0;
        /* se dibuja en un lienzo del tamaño del gráfico: la escala es pw / ancho de la diapositiva */
        pr_deck_t copia = *d;
        copia.ancho = o->w;
        pr_dibujar_obj(&copia, &t, px, pw, ph, 0);
        imagem_t im = { pw, ph, px, "" };
        uint8_t *png = NULL; int pn = 0;
        if (imagem_codificar_png(&im, &png, &pn) == 0) {
            w->extra[w->n_extra] = png; w->extra_n[w->n_extra] = pn;
            int rid = rid_nuevo(w);
            xfmt(&w->rels, "<Relationship Id=\"rId%d\" Type=\"" RT "image\" Target=\"../media/g%d_%d.png\"/>", rid, slide_n + 1, w->n_extra + 1);
            w->n_extra++;
            xfmt(b, "<p:pic><p:nvPicPr><p:cNvPr id=\"%d\" name=\"Grafico %d\"/><p:cNvPicPr/><p:nvPr/></p:nvPicPr><p:blipFill><a:blip r:embed=\"rId%d\"/><a:stretch><a:fillRect/></a:stretch></p:blipFill><p:spPr>", id, idx + 1, rid);
            ew_geom(b, o->x, o->y, o->w, o->h, "a:xfrm");
            xt(b, "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom></p:spPr></p:pic>");
        }
        libera(px);
        break;
    }
    case PR_TABLA: {
        int F = o->filas, C = o->cols;
        if (F < 1 || C < 1) break;
        xfmt(b, "<p:graphicFrame><p:nvGraphicFramePr><p:cNvPr id=\"%d\" name=\"Tabla %d\"/><p:cNvGraphicFramePr><a:graphicFrameLocks noGrp=\"1\"/></p:cNvGraphicFramePr><p:nvPr/></p:nvGraphicFramePr>", id, idx + 1);
        ew_geom(b, o->x, o->y, o->w, o->h, "p:xfrm");
        xt(b, "<a:graphic><a:graphicData uri=\"http://schemas.openxmlformats.org/drawingml/2006/table\"><a:tbl><a:tblPr firstRow=\"1\" bandRow=\"1\"/><a:tblGrid>");
        for (int c = 0; c < C; c++) xfmt(b, "<a:gridCol w=\"%d\"/>", o->w / C * EMU);
        xt(b, "</a:tblGrid>");
        const char *t = o->texto ? o->texto : "";
        for (int f = 0; f < F; f++) {
            xfmt(b, "<a:tr h=\"%d\">", o->h / F * EMU);
            for (int c = 0; c < C; c++) {
                const char *p = t;
                for (int i = 0; i < f * C + c; i++) { while (*p && *p != '\x1f') p++; if (*p) p++; }
                int n = 0; while (p[n] && p[n] != '\x1f') n++;
                char tmp[200]; if (n > 199) n = 199; memcpy(tmp, p, (size_t)n); tmp[n] = 0;
                xt(b, "<a:tc><a:txBody><a:bodyPr/><a:lstStyle/><a:p>");
                if (n) { xfmt(b, "<a:r><a:rPr lang=\"es-ES\" sz=\"%d\" b=\"%d\" dirty=\"0\"/><a:t>", o->tam * 100, f == 0); xesc(b, tmp); xt(b, "</a:t></a:r>"); }
                else xfmt(b, "<a:endParaRPr lang=\"es-ES\" sz=\"%d\"/>", o->tam * 100);
                xt(b, "</a:p></a:txBody><a:tcPr/></a:tc>");
            }
            xt(b, "</a:tr>");
        }
        xt(b, "</a:tbl></a:graphicData></a:graphic></p:graphicFrame>");
        break;
    }
    }
}

static void ew_rect(buf_t *b, ew_t *w, int x, int y, int cx, int cy, uint32_t c, const char *nombre)
{
    int id = ++w->idsig;
    xfmt(b, "<p:sp><p:nvSpPr><p:cNvPr id=\"%d\" name=\"%s\"/><p:cNvSpPr/><p:nvPr/></p:nvSpPr><p:spPr>", id, nombre);
    ew_geom(b, x, y, cx, cy, "a:xfrm");
    xt(b, "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom>");
    ew_relleno(b, "a:solidFill", c);
    xt(b, "<a:ln><a:noFill/></a:ln></p:spPr></p:sp>");
}

static void ew_fondo(ew_t *w, const pr_diap_t *s)
{
    buf_t *b = &w->sl;
    const pr_tema_t *t = &pr_temas[w->d->tema];
    uint32_t c1 = t->fondo1, c2 = t->fondo2;
    if (s->fondo_tipo == PR_FONDO_COLOR) c1 = c2 = s->fondo1;
    else if (s->fondo_tipo == PR_FONDO_DEGRADADO) { c1 = s->fondo1; c2 = s->fondo2; }
    xt(b, "<p:bg><p:bgPr>");
    if (s->fondo_tipo == PR_FONDO_IMAGEN && s->fondo_medio >= 0 && s->fondo_medio < w->d->n_medios) {
        int rid = rid_nuevo(w);
        xfmt(&w->rels, "<Relationship Id=\"rId%d\" Type=\"" RT "image\" Target=\"../media/image%d.%s\"/>", rid, s->fondo_medio + 1, w->d->medios[s->fondo_medio].ext);
        xfmt(b, "<a:blipFill><a:blip r:embed=\"rId%d\"/><a:stretch><a:fillRect/></a:stretch></a:blipFill>", rid);
    } else if (c1 == c2) ew_relleno(b, "a:solidFill", c1);
    else {
        xfmt(b, "<a:gradFill rotWithShape=\"1\"><a:gsLst><a:gs pos=\"0\"><a:srgbClr val=\"%06x\"/></a:gs><a:gs pos=\"100000\"><a:srgbClr val=\"%06x\"/></a:gs></a:gsLst><a:lin ang=\"5400000\" scaled=\"0\"/></a:gradFill>",
             (unsigned)c1, (unsigned)c2);
    }
    xt(b, "<a:effectLst/></p:bgPr></p:bg>");
}

static void ew_tema(buf_t *b, const pr_deck_t *d)
{
    const pr_tema_t *t = &pr_temas[d->tema];
    xt(b, xml_cab);
    xfmt(b, "<a:theme " NS_A " name=\"%s\"><a:themeElements><a:clrScheme name=\"Tucan\">", "Tucan");
    xfmt(b, "<a:dk1><a:srgbClr val=\"%06x\"/></a:dk1><a:lt1><a:srgbClr val=\"%06x\"/></a:lt1><a:dk2><a:srgbClr val=\"%06x\"/></a:dk2><a:lt2><a:srgbClr val=\"%06x\"/></a:lt2>",
         (unsigned)t->texto, (unsigned)t->fondo1, (unsigned)t->titulo, (unsigned)t->fondo2);
    xfmt(b, "<a:accent1><a:srgbClr val=\"%06x\"/></a:accent1><a:accent2><a:srgbClr val=\"%06x\"/></a:accent2>", (unsigned)t->acento, (unsigned)t->acento2);
    xt(b, "<a:accent3><a:srgbClr val=\"A5A5A5\"/></a:accent3><a:accent4><a:srgbClr val=\"FFC000\"/></a:accent4><a:accent5><a:srgbClr val=\"5B9BD5\"/></a:accent5>"
          "<a:accent6><a:srgbClr val=\"70AD47\"/></a:accent6><a:hlink><a:srgbClr val=\"0563C1\"/></a:hlink><a:folHlink><a:srgbClr val=\"954F72\"/></a:folHlink></a:clrScheme>");
    xt(b, "<a:fontScheme name=\"Tucan\"><a:majorFont><a:latin typeface=\""); xesc(b, d->f_titulo[0] ? d->f_titulo : "Arial");
    xt(b, "\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/></a:majorFont><a:minorFont><a:latin typeface=\""); xesc(b, d->f_cuerpo[0] ? d->f_cuerpo : "Arial");
    xt(b, "\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/></a:minorFont></a:fontScheme>");
    xt(b, "<a:fmtScheme name=\"Tucan\"><a:fillStyleLst><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill></a:fillStyleLst>"
          "<a:lnStyleLst><a:ln w=\"6350\"><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill></a:ln><a:ln w=\"12700\"><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill></a:ln><a:ln w=\"19050\"><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill></a:ln></a:lnStyleLst>"
          "<a:effectStyleLst><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle></a:effectStyleLst>"
          "<a:bgFillStyleLst><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill></a:bgFillStyleLst></a:fmtScheme>"
          "</a:themeElements><a:objectDefaults/><a:extraClrSchemeLst/></a:theme>");
}

#define GRP_NV "<p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr><p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/><a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr>"

static int zadd(zip_escritor_t *z, const char *nombre, buf_t *b)
{
    return zip_adicionar(z, nombre, b->d, (uint32_t)b->n, 1700000000u, 6, false, 0644);
}

int pr_exportar_pptx(const pr_deck_t *dd, const char *ruta)
{
    pr_deck_t *d = (pr_deck_t *)dd;
    zip_escritor_t z;
    memset(&z, 0, sizeof z);
    int e = 0, r = 0;
    buf_t b = { 0 };
    int nd = d->n;
    int W = d->ancho, H = d->alto;
    /* [Content_Types].xml */
    xt(&b, xml_cab);
    xt(&b, "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\"><Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
           "<Default Extension=\"xml\" ContentType=\"application/xml\"/><Default Extension=\"png\" ContentType=\"image/png\"/><Default Extension=\"jpg\" ContentType=\"image/jpeg\"/>"
           "<Override PartName=\"/ppt/presentation.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml\"/>"
           "<Override PartName=\"/ppt/slideMasters/slideMaster1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml\"/>"
           "<Override PartName=\"/ppt/slideLayouts/slideLayout1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml\"/>"
           "<Override PartName=\"/ppt/notesMasters/notesMaster1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.notesMaster+xml\"/>"
           "<Override PartName=\"/ppt/theme/theme1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.theme+xml\"/>"
           "<Override PartName=\"/ppt/theme/theme2.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.theme+xml\"/>");
    for (int i = 0; i < nd; i++)
        xfmt(&b, "<Override PartName=\"/ppt/slides/slide%d.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>"
                 "<Override PartName=\"/ppt/notesSlides/notesSlide%d.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.notesSlide+xml\"/>", i + 1, i + 1);
    xt(&b, "</Types>");
    e |= zadd(&z, "[Content_Types].xml", &b);
    b.n = 0;
    xt(&b, xml_cab);
    xt(&b, "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"" RT "officeDocument\" Target=\"ppt/presentation.xml\"/></Relationships>");
    e |= zadd(&z, "_rels/.rels", &b);
    /* presentation.xml y sus relaciones */
    b.n = 0;
    xt(&b, xml_cab);
    xt(&b, "<p:presentation " NS_A " " NS_R " " NS_P " saveSubsetFonts=\"1\"><p:sldMasterIdLst><p:sldMasterId id=\"2147483648\" r:id=\"rId1\"/></p:sldMasterIdLst>"
           "<p:notesMasterIdLst><p:notesMasterId r:id=\"rId2\"/></p:notesMasterIdLst><p:sldIdLst>");
    for (int i = 0; i < nd; i++) xfmt(&b, "<p:sldId id=\"%d\" r:id=\"rId%d\"/>", 256 + i, 10 + i);
    xfmt(&b, "</p:sldIdLst><p:sldSz cx=\"%d\" cy=\"%d\"/><p:notesSz cx=\"6858000\" cy=\"9144000\"/></p:presentation>", W * EMU, H * EMU);
    e |= zadd(&z, "ppt/presentation.xml", &b);
    b.n = 0;
    xt(&b, xml_cab);
    xt(&b, "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"" RT "slideMaster\" Target=\"slideMasters/slideMaster1.xml\"/>"
           "<Relationship Id=\"rId2\" Type=\"" RT "notesMaster\" Target=\"notesMasters/notesMaster1.xml\"/><Relationship Id=\"rId3\" Type=\"" RT "theme\" Target=\"theme/theme1.xml\"/>");
    for (int i = 0; i < nd; i++) xfmt(&b, "<Relationship Id=\"rId%d\" Type=\"" RT "slide\" Target=\"slides/slide%d.xml\"/>", 10 + i, i + 1);
    xt(&b, "</Relationships>");
    e |= zadd(&z, "ppt/_rels/presentation.xml.rels", &b);
    /* tema */
    b.n = 0; ew_tema(&b, d); e |= zadd(&z, "ppt/theme/theme1.xml", &b);
    e |= zadd(&z, "ppt/theme/theme2.xml", &b);
    /* maqueta y plantilla de diapositiva */
    b.n = 0;
    xt(&b, xml_cab);
    xt(&b, "<p:sldMaster " NS_A " " NS_R " " NS_P "><p:cSld><p:bg><p:bgRef idx=\"1001\"><a:schemeClr val=\"bg1\"/></p:bgRef></p:bg><p:spTree>" GRP_NV "</p:spTree></p:cSld>"
           "<p:clrMap bg1=\"lt1\" tx1=\"dk1\" bg2=\"lt2\" tx2=\"dk2\" accent1=\"accent1\" accent2=\"accent2\" accent3=\"accent3\" accent4=\"accent4\" accent5=\"accent5\" accent6=\"accent6\" hlink=\"hlink\" folHlink=\"folHlink\"/>"
           "<p:sldLayoutIdLst><p:sldLayoutId id=\"2147483649\" r:id=\"rId1\"/></p:sldLayoutIdLst>"
           "<p:txStyles><p:titleStyle><a:lvl1pPr><a:defRPr sz=\"4000\"/></a:lvl1pPr></p:titleStyle><p:bodyStyle><a:lvl1pPr><a:defRPr sz=\"2800\"/></a:lvl1pPr></p:bodyStyle><p:otherStyle><a:lvl1pPr><a:defRPr sz=\"1800\"/></a:lvl1pPr></p:otherStyle></p:txStyles></p:sldMaster>");
    e |= zadd(&z, "ppt/slideMasters/slideMaster1.xml", &b);
    b.n = 0;
    xt(&b, xml_cab);
    xt(&b, "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"" RT "slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/>"
           "<Relationship Id=\"rId2\" Type=\"" RT "theme\" Target=\"../theme/theme1.xml\"/></Relationships>");
    e |= zadd(&z, "ppt/slideMasters/_rels/slideMaster1.xml.rels", &b);
    b.n = 0;
    xt(&b, xml_cab);
    xfmt(&b, "<p:sldLayout " NS_A " " NS_R " " NS_P " type=\"obj\" preserve=\"1\"><p:cSld name=\"Titulo y contenido\"><p:spTree>" GRP_NV
             "<p:sp><p:nvSpPr><p:cNvPr id=\"2\" name=\"Titulo\"/><p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr><p:ph type=\"title\"/></p:nvPr></p:nvSpPr><p:spPr>");
    ew_geom(&b, W * 6 / 100, H * 5 / 100, W - 2 * (W * 6 / 100), H * 18 / 100, "a:xfrm");
    xt(&b, "</p:spPr><p:txBody><a:bodyPr/><a:lstStyle/><a:p><a:endParaRPr lang=\"es-ES\"/></a:p></p:txBody></p:sp>"
           "<p:sp><p:nvSpPr><p:cNvPr id=\"3\" name=\"Contenido\"/><p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr><p:ph idx=\"1\"/></p:nvPr></p:nvSpPr><p:spPr>");
    ew_geom(&b, W * 6 / 100, H * 26 / 100, W - 2 * (W * 6 / 100), H * 62 / 100, "a:xfrm");
    xt(&b, "</p:spPr><p:txBody><a:bodyPr/><a:lstStyle/><a:p><a:endParaRPr lang=\"es-ES\"/></a:p></p:txBody></p:sp></p:spTree></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sldLayout>");
    e |= zadd(&z, "ppt/slideLayouts/slideLayout1.xml", &b);
    b.n = 0;
    xt(&b, xml_cab);
    xt(&b, "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"" RT "slideMaster\" Target=\"../slideMasters/slideMaster1.xml\"/></Relationships>");
    e |= zadd(&z, "ppt/slideLayouts/_rels/slideLayout1.xml.rels", &b);
    /* maestra de notas */
    b.n = 0;
    xt(&b, xml_cab);
    xt(&b, "<p:notesMaster " NS_A " " NS_R " " NS_P "><p:cSld><p:bg><p:bgRef idx=\"1001\"><a:schemeClr val=\"bg1\"/></p:bgRef></p:bg><p:spTree>" GRP_NV
           "<p:sp><p:nvSpPr><p:cNvPr id=\"2\" name=\"Imagen de diapositiva\"/><p:cNvSpPr><a:spLocks noGrp=\"1\" noRot=\"1\" noChangeAspect=\"1\"/></p:cNvSpPr><p:nvPr><p:ph type=\"sldImg\" idx=\"2\"/></p:nvPr></p:nvSpPr>"
           "<p:spPr><a:xfrm><a:off x=\"685800\" y=\"1143000\"/><a:ext cx=\"5486400\" cy=\"3086100\"/></a:xfrm><a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom></p:spPr></p:sp>"
           "<p:sp><p:nvSpPr><p:cNvPr id=\"3\" name=\"Notas\"/><p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr><p:ph type=\"body\" sz=\"quarter\" idx=\"3\"/></p:nvPr></p:nvSpPr>"
           "<p:spPr><a:xfrm><a:off x=\"685800\" y=\"4400550\"/><a:ext cx=\"5486400\" cy=\"3600450\"/></a:xfrm><a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom></p:spPr>"
           "<p:txBody><a:bodyPr/><a:lstStyle/><a:p><a:endParaRPr lang=\"es-ES\"/></a:p></p:txBody></p:sp></p:spTree></p:cSld>"
           "<p:clrMap bg1=\"lt1\" tx1=\"dk1\" bg2=\"lt2\" tx2=\"dk2\" accent1=\"accent1\" accent2=\"accent2\" accent3=\"accent3\" accent4=\"accent4\" accent5=\"accent5\" accent6=\"accent6\" hlink=\"hlink\" folHlink=\"folHlink\"/></p:notesMaster>");
    e |= zadd(&z, "ppt/notesMasters/notesMaster1.xml", &b);
    b.n = 0;
    xt(&b, xml_cab);
    xt(&b, "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"" RT "theme\" Target=\"../theme/theme2.xml\"/></Relationships>");
    e |= zadd(&z, "ppt/notesMasters/_rels/notesMaster1.xml.rels", &b);
    /* imágenes */
    for (int i = 0; i < d->n_medios && !e; i++) {
        char nom[40];
        formatar_texto(nom, sizeof nom, "ppt/media/image%d.%s", i + 1, d->medios[i].ext);
        e |= zip_adicionar(&z, nom, d->medios[i].dados, (uint32_t)d->medios[i].n, 1700000000u, 0, false, 0644);
    }
    /* diapositivas */
    for (int i = 0; i < nd && !e; i++) {
        pr_diap_t *s = d->d[i];
        ew_t w;
        memset(&w, 0, sizeof w);
        w.d = d; w.idsig = 1; w.nrel = 2;
        xt(&w.sl, xml_cab);
        xfmt(&w.sl, "<p:sld " NS_A " " NS_R " " NS_P "%s><p:cSld>", s->oculta ? " show=\"0\"" : "");
        ew_fondo(&w, s);
        xt(&w.sl, "<p:spTree>" GRP_NV);
        const pr_tema_t *t = &pr_temas[d->tema];
        if (t->deco == 1) { ew_rect(&w.sl, &w, 0, H - 12, W, 12, t->acento, "Franja"); ew_rect(&w.sl, &w, 0, H - 16, W * 30 / 100, 4, t->acento2, "Franja 2"); }
        else if (t->deco == 2) { ew_rect(&w.sl, &w, 0, 0, 16, H, t->acento, "Franja"); ew_rect(&w.sl, &w, 16, 0, 4, H, t->acento2, "Franja 2"); }
        else if (t->deco == 3) ew_rect(&w.sl, &w, 0, 0, W, 8, t->acento, "Franja");
        for (int k = 0; k < s->n; k++) ew_obj(&w, &s->obj[k], k, i);
        /* pie y número */
        if ((d->con_pie && d->pie[0]) || d->num) {
            pr_obj_t tmp;
            memset(&tmp, 0, sizeof tmp);
            tmp.tipo = PR_TEXTO; tmp.tam = 12; tmp.col = PR_COL_TEMA; tmp.medio = -1; tmp.valign = 1;
            if (d->con_pie && d->pie[0]) { tmp.x = (int16_t)(W / 20); tmp.y = (int16_t)(H - 40); tmp.w = (int16_t)(W / 2); tmp.h = 24; pr_obj_t *tp = &tmp;
                int id = ++w.idsig;
                xfmt(&w.sl, "<p:sp><p:nvSpPr><p:cNvPr id=\"%d\" name=\"Pie\"/><p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr><p:ph type=\"ftr\" sz=\"quarter\" idx=\"11\"/></p:nvPr></p:nvSpPr><p:spPr>", id);
                ew_geom(&w.sl, tp->x, tp->y, tp->w, tp->h, "a:xfrm");
                xt(&w.sl, "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom><a:noFill/></p:spPr>");
                ew_cuerpo(&w.sl, d, tp, d->pie, "p:txBody", 1, 12);
                xt(&w.sl, "</p:sp>"); }
            if (d->num) { char nb[16]; formatar_texto(nb, sizeof nb, "%d", i + 1);
                tmp.alin = 2; tmp.x = (int16_t)(W - 80 - W / 20); tmp.y = (int16_t)(H - 40); tmp.w = 80; tmp.h = 24;
                int id = ++w.idsig;
                xfmt(&w.sl, "<p:sp><p:nvSpPr><p:cNvPr id=\"%d\" name=\"Numero\"/><p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr><p:ph type=\"sldNum\" sz=\"quarter\" idx=\"12\"/></p:nvPr></p:nvSpPr><p:spPr>", id);
                ew_geom(&w.sl, tmp.x, tmp.y, tmp.w, tmp.h, "a:xfrm");
                xt(&w.sl, "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom><a:noFill/></p:spPr>");
                ew_cuerpo(&w.sl, d, &tmp, nb, "p:txBody", 1, 12);
                xt(&w.sl, "</p:sp>"); }
        }
        xt(&w.sl, "</p:spTree></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>");
        if (s->trans || s->auto_seg) {
            xfmt(&w.sl, "<p:transition spd=\"med\"");
            if (s->auto_seg) xfmt(&w.sl, " advTm=\"%d\"", s->auto_seg * 1000);
            xt(&w.sl, ">");
            if (s->trans == PR_TRANS_FUNDIDO) xt(&w.sl, "<p:fade/>");
            else if (s->trans == PR_TRANS_DESPLAZAR) xt(&w.sl, "<p:push dir=\"l\"/>");
            else if (s->trans == PR_TRANS_BARRIDO) xt(&w.sl, "<p:wipe dir=\"r\"/>");
            xt(&w.sl, "</p:transition>");
        }
        xt(&w.sl, "</p:sld>");
        char nom[64];
        formatar_texto(nom, sizeof nom, "ppt/slides/slide%d.xml", i + 1);
        e |= zadd(&z, nom, &w.sl);
        /* relaciones: 1 = maqueta, 2 = notas, el resto las imágenes */
        buf_t rl = { 0 };
        xt(&rl, xml_cab);
        xt(&rl, "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"" RT "slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/>");
        xfmt(&rl, "<Relationship Id=\"rId2\" Type=\"" RT "notesSlide\" Target=\"../notesSlides/notesSlide%d.xml\"/>", i + 1);
        comp_buf_somar(&rl, w.rels.d, w.rels.n);
        xt(&rl, "</Relationships>");
        formatar_texto(nom, sizeof nom, "ppt/slides/_rels/slide%d.xml.rels", i + 1);
        e |= zadd(&z, nom, &rl);
        comp_buf_liberar(&rl);
        for (int k = 0; k < w.n_extra && !e; k++) {
            char nom2[64];
            formatar_texto(nom2, sizeof nom2, "ppt/media/g%d_%d.png", i + 1, k + 1);
            e |= zip_adicionar(&z, nom2, w.extra[k], (uint32_t)w.extra_n[k], 1700000000u, 0, false, 0644);
            libera(w.extra[k]);
        }
        comp_buf_liberar(&w.sl); comp_buf_liberar(&w.rels);
        /* notas */
        b.n = 0;
        xt(&b, xml_cab);
        xt(&b, "<p:notes " NS_A " " NS_R " " NS_P "><p:cSld><p:spTree>" GRP_NV
               "<p:sp><p:nvSpPr><p:cNvPr id=\"2\" name=\"Imagen\"/><p:cNvSpPr><a:spLocks noGrp=\"1\" noRot=\"1\" noChangeAspect=\"1\"/></p:cNvSpPr><p:nvPr><p:ph type=\"sldImg\"/></p:nvPr></p:nvSpPr><p:spPr/></p:sp>"
               "<p:sp><p:nvSpPr><p:cNvPr id=\"3\" name=\"Notas\"/><p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr><p:ph type=\"body\" idx=\"1\"/></p:nvPr></p:nvSpPr><p:spPr/><p:txBody><a:bodyPr/><a:lstStyle/>");
        const char *nt = s->notas ? s->notas : "";
        int len = (int)tam_texto(nt), pos = 0;
        for (;;) {
            int fin = pos;
            while (fin < len && nt[fin] != '\n') fin++;
            xt(&b, "<a:p>");
            if (fin > pos) {
                char *tmp = aloca((size_t)(fin - pos) + 1);
                if (tmp) { memcpy(tmp, nt + pos, (size_t)(fin - pos)); tmp[fin - pos] = 0; xt(&b, "<a:r><a:rPr lang=\"es-ES\"/><a:t>"); xesc(&b, tmp); xt(&b, "</a:t></a:r>"); libera(tmp); }
            } else xt(&b, "<a:endParaRPr lang=\"es-ES\"/>");
            xt(&b, "</a:p>");
            if (fin >= len) break;
            pos = fin + 1;
        }
        xt(&b, "</p:txBody></p:sp></p:spTree></p:cSld><p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:notes>");
        formatar_texto(nom, sizeof nom, "ppt/notesSlides/notesSlide%d.xml", i + 1);
        e |= zadd(&z, nom, &b);
        b.n = 0;
        xt(&b, xml_cab);
        xfmt(&b, "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"" RT "notesMaster\" Target=\"../notesMasters/notesMaster1.xml\"/>"
                 "<Relationship Id=\"rId2\" Type=\"" RT "slide\" Target=\"../slides/slide%d.xml\"/></Relationships>", i + 1);
        formatar_texto(nom, sizeof nom, "ppt/notesSlides/_rels/notesSlide%d.xml.rels", i + 1);
        e |= zadd(&z, nom, &b);
    }
    comp_buf_liberar(&b);
    if (!e) e = zip_concluir(&z);
    if (e) r = PR_ERR_MEMORIA;
    else if (salvar_arquivo(ruta, z.b.d, z.b.n) != 0) r = PR_ERR_ARCHIVO;
    zip_escritor_liberar(&z);
    return r;
}

/* ------------------------------------------------------------ lectura: XML mínimo */

typedef struct { const char *s; int n, i; } xml_t;
typedef struct {
    int tipo;                     /* 0 fin, 1 abre, 2 cierra, 3 texto */
    const char *nom; int nn;      /* nombre completo (con prefijo) */
    const char *loc; int ln;      /* nombre local */
    const char *at; int an;       /* atributos */
    bool vacio;                   /* <x/> */
    const char *tx; int tn;
} xtok_t;

static bool xml_sig(xml_t *x, xtok_t *t)
{
    for (;;) {
        if (x->i >= x->n) { t->tipo = 0; return false; }
        if (x->s[x->i] != '<') {
            int a = x->i;
            while (x->i < x->n && x->s[x->i] != '<') x->i++;
            t->tipo = 3; t->tx = x->s + a; t->tn = x->i - a;
            return true;
        }
        if (x->i + 3 < x->n && x->s[x->i + 1] == '!' && x->s[x->i + 2] == '-' && x->s[x->i + 3] == '-') {
            x->i += 4;
            while (x->i + 2 < x->n && !(x->s[x->i] == '-' && x->s[x->i + 1] == '-' && x->s[x->i + 2] == '>')) x->i++;
            x->i += 3; continue;
        }
        if (x->i + 8 < x->n && memcmp(x->s + x->i, "<![CDATA[", 9) == 0) {
            int a = x->i + 9;
            x->i = a;
            while (x->i + 2 < x->n && !(x->s[x->i] == ']' && x->s[x->i + 1] == ']' && x->s[x->i + 2] == '>')) x->i++;
            t->tipo = 3; t->tx = x->s + a; t->tn = x->i - a;
            x->i += 3;
            return true;
        }
        if (x->i + 1 < x->n && (x->s[x->i + 1] == '?' || x->s[x->i + 1] == '!')) {
            while (x->i < x->n && x->s[x->i] != '>') x->i++;
            x->i++; continue;
        }
        int a = ++x->i;
        bool cierra = false;
        if (x->i < x->n && x->s[x->i] == '/') { cierra = true; a = ++x->i; }
        while (x->i < x->n && x->s[x->i] != ' ' && x->s[x->i] != '>' && x->s[x->i] != '/' && x->s[x->i] != '\n' && x->s[x->i] != '\t' && x->s[x->i] != '\r') x->i++;
        t->nom = x->s + a; t->nn = x->i - a;
        t->loc = t->nom; t->ln = t->nn;
        for (int k = 0; k < t->nn; k++) if (t->nom[k] == ':') { t->loc = t->nom + k + 1; t->ln = t->nn - k - 1; }
        int b = x->i;
        char q = 0;
        while (x->i < x->n && (q || x->s[x->i] != '>')) {
            if (q) { if (x->s[x->i] == q) q = 0; }
            else if (x->s[x->i] == '"' || x->s[x->i] == '\'') q = x->s[x->i];
            x->i++;
        }
        int fin = x->i;
        t->vacio = false;
        if (fin > b && x->s[fin - 1] == '/') { t->vacio = true; fin--; }
        t->at = x->s + b; t->an = fin - b;
        x->i++;
        t->tipo = cierra ? 2 : 1;
        if (cierra) t->vacio = false;
        return true;
    }
}

static bool loc_es(const xtok_t *t, const char *n) { return (int)tam_texto(n) == t->ln && memcmp(t->loc, n, (size_t)t->ln) == 0; }

/* Valor de un atributo (por su nombre local); false = no está. Decodifica las entidades y pasa a Latin-1. */
static bool xattr(const xtok_t *t, const char *nombre, char *out, int max)
{
    int nl = (int)tam_texto(nombre);
    const char *p = t->at, *e = t->at + t->an;
    while (p < e) {
        while (p < e && (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r')) p++;
        const char *ini = p;
        while (p < e && *p != '=' && *p != ' ') p++;
        const char *fin = p;
        while (p < e && *p != '"' && *p != '\'') p++;
        if (p >= e) break;
        char q = *p++;
        const char *v = p;
        while (p < e && *p != q) p++;
        int vn = (int)(p - v);
        p++;
        int an = (int)(fin - ini), pre = 0;
        for (int k = 0; k < an; k++) if (ini[k] == ':') pre = k + 1;
        if (an - pre == nl && memcmp(ini + pre, nombre, (size_t)nl) == 0) {
            int k = 0;
            for (int i = 0; i < vn && k < max - 1; i++) {
                if (v[i] == '&') {
                    if (i + 3 < vn && memcmp(v + i, "&lt;", 4) == 0) { out[k++] = '<'; i += 3; continue; }
                    if (i + 3 < vn && memcmp(v + i, "&gt;", 4) == 0) { out[k++] = '>'; i += 3; continue; }
                    if (i + 4 < vn && memcmp(v + i, "&amp;", 5) == 0) { out[k++] = '&'; i += 4; continue; }
                    if (i + 5 < vn && memcmp(v + i, "&quot;", 6) == 0) { out[k++] = '"'; i += 5; continue; }
                    if (i + 5 < vn && memcmp(v + i, "&apos;", 6) == 0) { out[k++] = '\''; i += 5; continue; }
                }
                out[k++] = v[i];
            }
            out[k] = 0;
            return true;
        }
    }
    out[0] = 0;
    return false;
}
static int xnum(const xtok_t *t, const char *nombre, int def)
{
    char v[32];
    if (!xattr(t, nombre, v, sizeof v)) return def;
    int neg = 0, r = 0;
    const char *p = v;
    if (*p == '-') { neg = 1; p++; }
    while (*p >= '0' && *p <= '9') r = r * 10 + (*p++ - '0');
    return neg ? -r : r;
}
static unsigned hex6(const char *s)
{
    unsigned v = 0;
    for (int i = 0; i < 6 && s[i]; i++) { char c = s[i]; v = v * 16 + (unsigned)(c <= '9' ? c - '0' : (c | 0x20) - 'a' + 10); }
    return v;
}

/* Texto con entidades (&amp; &#N;) en UTF-8 -> Latin-1, añadido a un buffer. */
static void texto_a_buf(buf_t *b, const char *s, int n)
{
    char *u = aloca((size_t)n + 1);
    char *l = aloca((size_t)n * 3 + 4);
    if (!u || !l) { libera(u); libera(l); return; }
    int k = 0;
    for (int i = 0; i < n; i++) {
        if (s[i] == '&') {
            int j = i + 1;
            while (j < n && j - i < 10 && s[j] != ';') j++;
            if (j < n && s[j] == ';') {
                unsigned cp = 0; bool ok = true;
                if (s[i + 1] == '#') {
                    int q = i + 2;
                    if (q < j && (s[q] == 'x' || s[q] == 'X')) { q++; for (; q < j; q++) cp = cp * 16 + (unsigned)(s[q] <= '9' ? s[q] - '0' : (s[q] | 0x20) - 'a' + 10); }
                    else for (; q < j; q++) cp = cp * 10 + (unsigned)(s[q] - '0');
                } else if (j - i == 3 && s[i + 1] == 'l') cp = '<';
                else if (j - i == 3 && s[i + 1] == 'g') cp = '>';
                else if (j - i == 4 && s[i + 1] == 'a') cp = '&';
                else if (j - i == 5 && s[i + 1] == 'q') cp = '"';
                else if (j - i == 5 && s[i + 1] == 'a') cp = '\'';
                else ok = false;
                if (ok) {
                    /* a UTF-8 en u (el codepoint puede ocupar hasta 3 bytes; u tiene sitio porque la entidad es más larga) */
                    if (cp < 0x80) u[k++] = (char)cp;
                    else if (cp < 0x800) { u[k++] = (char)(0xC0 | (cp >> 6)); u[k++] = (char)(0x80 | (cp & 0x3F)); }
                    else { u[k++] = (char)(0xE0 | (cp >> 12)); u[k++] = (char)(0x80 | ((cp >> 6) & 0x3F)); u[k++] = (char)(0x80 | (cp & 0x3F)); }
                    i = j;
                    continue;
                }
            }
        }
        u[k++] = s[i];
    }
    int m = pr_utf8_a_latin1(u, k, l, n * 3 + 4);
    comp_buf_somar(b, l, m);
    libera(u); libera(l);
}

/* ------------------------------------------------------------ lectura: paquete */

static uint8_t *zleer(const zip_leitor_t *z, const char *nombre, int *n)
{
    for (int i = 0; i < z->cnt; i++)
        if (compara_texto(z->e[i].nome, nombre) == 0) {
            uint8_t *d = NULL;
            int r = zip_extrair(z, i, &d);
            if (r < 0) { libera(d); return NULL; }
            *n = r;
            return d;
        }
    return NULL;
}

/* Une un destino relativo con el directorio de la parte que lo cita y quita los «..». */
static void ruta_unir(const char *parte, const char *dest, char *out, int max)
{
    char tmp[200];
    if (dest[0] == '/') { copia_texto(tmp, dest + 1, sizeof tmp); }
    else {
        copia_texto(tmp, parte, sizeof tmp);
        char *b = acha_ultimo_char(tmp, '/');
        if (b) b[1] = 0; else tmp[0] = 0;
        size_t l = tam_texto(tmp);
        copia_texto(tmp + l, dest, sizeof tmp - l);
    }
    char *o = out; int k = 0;
    const char *p = tmp;
    char partes[16][64]; int np = 0;
    while (*p) {
        int a = 0; char seg[64];
        while (*p && *p != '/' && a < 63) seg[a++] = *p++;
        seg[a] = 0;
        while (*p && *p != '/') p++;
        if (*p == '/') p++;
        if (compara_texto(seg, "..") == 0) { if (np) np--; }
        else if (seg[0] && compara_texto(seg, ".") != 0 && np < 16) copia_texto(partes[np++], seg, 64);
    }
    for (int i = 0; i < np; i++) {
        if (i) o[k++] = '/';
        for (const char *q = partes[i]; *q && k < max - 2; q++) o[k++] = *q;
    }
    o[k] = 0;
}

static void rels_de(const char *parte, char *out, int max)
{
    const char *b = acha_ultimo_char(parte, '/');
    char dir[160];
    if (b) { int l = (int)(b - parte) + 1; if (l > 159) l = 159; memcpy(dir, parte, (size_t)l); dir[l] = 0; b++; }
    else { dir[0] = 0; b = parte; }
    formatar_texto(out, (size_t)max, "%s_rels/%s.rels", dir, b);
}

/* Busca en un archivo de relaciones el destino por id, o por el final del tipo. */
static bool rel_buscar(const uint8_t *r, int n, const char *id, const char *tipo_fin, char *dest, int max)
{
    xml_t x = { (const char *)r, n, 0 };
    xtok_t t;
    while (xml_sig(&x, &t)) {
        if (t.tipo != 1 || !loc_es(&t, "Relationship")) continue;
        char v[128], ty[200];
        if (id) { if (!xattr(&t, "Id", v, sizeof v) || compara_texto(v, id) != 0) continue; }
        if (tipo_fin) {
            if (!xattr(&t, "Type", ty, sizeof ty)) continue;
            int tl = (int)tam_texto(ty), fl = (int)tam_texto(tipo_fin);
            if (tl < fl || compara_texto(ty + tl - fl, tipo_fin) != 0) continue;
        }
        char tg[200], modo[16];
        if (xattr(&t, "TargetMode", modo, sizeof modo) && compara_texto(modo, "External") == 0) continue;
        if (!xattr(&t, "Target", tg, sizeof tg)) continue;
        copia_texto(dest, tg, (size_t)max);
        return true;
    }
    return false;
}

/* ------------------------------------------------------------ lectura: diapositivas */

typedef struct {
    int avisos;                    /* bits de lo que se ignoró */
    int sx, sy;                    /* escala a puntos: milésimas, EMU -> pt */
    int ow, oh;                    /* tamaño original en EMU */
} impctx_t;
#define AV_GRUPOS 1
#define AV_ANIM 2
#define AV_GRAFICO 4
#define AV_FORMA 8
#define AV_OTRO 16
#define AV_FUENTES 32

typedef struct { int x, y, cx, cy; bool ok; } geo_t;
typedef struct { int gx, gy, gcx, gcy, chx, chy, chcx, chcy; bool ok; } gtr_t;   /* transformación de un grupo, en puntos */

/* La geometría de un marcador de posición en una maqueta o plantilla (tipo e índice del marcador). */
static bool ph_geom(const uint8_t *xml, int n, const char *tipo, int idx, geo_t *g)
{
    for (int pasada = 0; pasada < 2; pasada++) {
        xml_t x = { (const char *)xml, n, 0 };
        xtok_t t;
        bool en_sp = false, ph = false, coincide = false;
        char ty[32] = "";
        int ix = -1;
        geo_t gg = { 0, 0, 0, 0, false };
        while (xml_sig(&x, &t)) {
            if (t.tipo == 1 && loc_es(&t, "sp")) { en_sp = true; ph = false; coincide = false; ix = -1; ty[0] = 0; gg.ok = false; }
            else if (t.tipo == 2 && loc_es(&t, "sp")) { if (en_sp && coincide && gg.ok) { *g = gg; return true; } en_sp = false; }
            else if (en_sp && t.tipo == 1 && loc_es(&t, "ph")) {
                ph = true;
                xattr(&t, "type", ty, sizeof ty);
                ix = xnum(&t, "idx", -1);
                bool eq_t = tipo[0] && (compara_texto(ty, tipo) == 0 || (compara_texto(ty, "ctrTitle") == 0 && compara_texto(tipo, "title") == 0) || (compara_texto(ty, "title") == 0 && compara_texto(tipo, "ctrTitle") == 0));
                coincide = pasada == 0 ? (idx >= 0 && ix == idx && (!tipo[0] || !ty[0] || eq_t)) : eq_t;
                (void)ph;
            } else if (en_sp && t.tipo == 1 && loc_es(&t, "off")) { gg.x = xnum(&t, "x", 0); gg.y = xnum(&t, "y", 0); }
            else if (en_sp && t.tipo == 1 && loc_es(&t, "ext")) { gg.cx = xnum(&t, "cx", 0); gg.cy = xnum(&t, "cy", 0); gg.ok = gg.cx > 0; }
        }
    }
    return false;
}

#define PILA_MAX 40
typedef struct {
    int kind;                      /* 0 nada, 1 sp, 2 pic, 3 graphicFrame, 4 cxnSp */
    geo_t g;
    char ph_tipo[24]; int ph_idx; bool es_ph, txbox;
    int forma; bool rell, rell_def; uint32_t c_rell, c_borde; int borde_w; bool sin_borde, borde_def;
    buf_t txt; bool primer_run; int tam; int est; uint32_t col; bool col_def; char fuente[PR_FUENTE_MAX];
    int alin; int lista; int valign; bool alin_def; int lvl; bool par_abierto; int par_n;
    int medio; char embed[32];
    int filas, cols; buf_t celdas; bool en_celda; buf_t celda_txt;
    bool desconocida, rpr_visto;
} pobj_t;

static void pobj_limpiar(pobj_t *p)
{
    comp_buf_liberar(&p->txt); comp_buf_liberar(&p->celdas); comp_buf_liberar(&p->celda_txt);
    memset(p, 0, sizeof *p);
    p->ph_idx = -1; p->medio = -1; p->tam = 0; p->forma = PF_RECT;
}

static int en_pila(char pila[][24], int n, const char *nombre)
{
    for (int i = n - 1; i >= 0; i--) if (compara_texto(pila[i], nombre) == 0) return 1;
    return 0;
}

static void finalizar_obj(pr_deck_t *d, pr_diap_t *s, pobj_t *p, impctx_t *ic, const uint8_t *layout, int ln, const uint8_t *master, int mn,
                          zip_leitor_t *zip, const char *slide_path, const uint8_t *slide_rels, int srn, const gtr_t *gt)
{
    if (p->kind == 0) return;
    if (p->desconocida) {   /* pie y número de diapositiva: pasan a las opciones de la presentación */
        if (compara_texto(p->ph_tipo, "sldNum") == 0) d->num = true;
        else if (compara_texto(p->ph_tipo, "ftr") == 0 && p->txt.n > 0) {
            comp_buf_somar(&p->txt, "", 1);
            copia_texto(d->pie, (const char *)p->txt.d, sizeof d->pie);
            d->con_pie = true;
        }
        pobj_limpiar(p);
        return;
    }
    geo_t g = p->g;
    if (p->es_ph && !g.ok) {   /* hereda la posición de la maqueta o de la plantilla */
        const char *tp = p->ph_tipo[0] ? p->ph_tipo : "body";
        if (!(layout && ph_geom(layout, ln, p->ph_tipo, p->ph_idx, &g)) && !(master && ph_geom(master, mn, tp, p->ph_idx, &g))) g.ok = false;
    }
    if (!g.ok) { pobj_limpiar(p); return; }
    int X = g.x / EMU, Y = g.y / EMU, Wp = g.cx / EMU, Hp = g.cy / EMU;
    if (gt && gt->ok) {   /* dentro de un grupo: se aplica su transformación */
        X = gt->gx + (X - gt->chx) * gt->gcx / gt->chcx; Y = gt->gy + (Y - gt->chy) * gt->gcy / gt->chcy;
        Wp = Wp * gt->gcx / gt->chcx; Hp = Hp * gt->gcy / gt->chcy;
    }
    X = X * ic->sx / 1000; Y = Y * ic->sy / 1000; Wp = Wp * ic->sx / 1000; Hp = Hp * ic->sy / 1000;
    if (p->kind == 4) { Hp = Hp < 4 ? 4 : Hp; }
    pr_obj_t *o = NULL;
    if (p->kind == 2) {   /* imagen */
        char dest[200], ruta[200];
        if (p->embed[0] && slide_rels && rel_buscar(slide_rels, srn, p->embed, NULL, dest, sizeof dest)) {
            ruta_unir(slide_path, dest, ruta, sizeof ruta);
            int mn2 = 0;
            uint8_t *dat = zleer(zip, ruta, &mn2);
            if (dat) {
                int m = pr_medio_anadir(d, dat, mn2);
                libera(dat);
                if (m >= 0) { o = pr_obj_nuevo(s, PR_IMAGEN); if (o) o->medio = m; }
            }
        }
        if (!o) ic->avisos |= AV_OTRO;
    } else if (p->kind == 3) {
        if (p->filas > 0 && p->cols > 0) {
            o = pr_obj_nuevo(s, PR_TABLA);
            if (o) {
                o->filas = (uint8_t)(p->filas > 30 ? 30 : p->filas); o->cols = (uint8_t)(p->cols > 12 ? 12 : p->cols);
                buf_t *cb = &p->celdas;
                comp_buf_somar(cb, "", 1);
                pr_obj_poner_texto(o, (const char *)cb->d);
                o->tam = 14;
            }
        } else ic->avisos |= AV_GRAFICO;
    } else {
        bool vacio = p->txt.n == 0;
        bool es_forma = !p->txbox && !p->es_ph && (p->forma != PF_RECT || p->rell || p->borde_w || p->kind == 4);
        if (p->kind == 4) { es_forma = true; p->forma = PF_LINEA; }
        if (vacio && !es_forma && !p->es_ph) { pobj_limpiar(p); return; }
        o = pr_obj_nuevo(s, es_forma ? PR_FORMA : PR_TEXTO);
        if (o) {
            o->forma = (uint8_t)p->forma;
            if (p->es_ph) {
                o->rol = (uint8_t)((compara_texto(p->ph_tipo, "title") == 0 || compara_texto(p->ph_tipo, "ctrTitle") == 0) ? PR_ROL_TITULO
                                   : compara_texto(p->ph_tipo, "subTitle") == 0 ? PR_ROL_SUBTITULO : PR_ROL_CUERPO);
            }
            comp_buf_somar(&p->txt, "", 1);
            pr_obj_poner_texto(o, (const char *)p->txt.d);
            o->tam = (uint8_t)(p->tam ? (p->tam > 250 ? 250 : p->tam) : (o->rol == PR_ROL_TITULO ? 40 : o->rol == PR_ROL_SUBTITULO ? 26 : o->rol == PR_ROL_CUERPO ? 28 : 18));
            o->est = (uint8_t)p->est; o->alin = (uint8_t)p->alin; o->lista = (uint8_t)p->lista; o->valign = (uint8_t)p->valign;
            if (!p->alin_def && o->rol == PR_ROL_TITULO) o->alin = 0;
            if (p->col_def) o->col = p->col;
            if (p->fuente[0]) { copia_texto(o->fuente, p->fuente, sizeof o->fuente); }
            if (p->rell) { o->rellena = 1; o->relleno = p->c_rell; }
            if (p->borde_w && !p->sin_borde) { o->borde = (uint8_t)(p->borde_w / EMU < 1 ? 1 : p->borde_w / EMU); o->borde_col = p->c_borde; }
            else if (es_forma && !p->sin_borde && !p->rell && p->kind != 4) { o->borde = 1; }
            if (p->kind == 4) { o->borde = (uint8_t)(p->borde_w / EMU < 1 ? 2 : p->borde_w / EMU); if (p->c_borde) o->borde_col = p->c_borde; }
        }
    }
    if (o) { o->x = (int16_t)X; o->y = (int16_t)Y; o->w = (int16_t)(Wp < 4 ? 4 : Wp); o->h = (int16_t)(Hp < 4 ? 4 : Hp); }
    pobj_limpiar(p);
}

static bool importar_slide(pr_deck_t *d, pr_diap_t *s, const uint8_t *xml, int n, impctx_t *ic, const uint8_t *layout, int ln,
                           const uint8_t *master, int mn, zip_leitor_t *zip, const char *slide_path, const uint8_t *srels, int srn)
{
    xml_t x = { (const char *)xml, n, 0 };
    xtok_t t;
    char pila[PILA_MAX][24];
    int np = 0;
    pobj_t p;
    memset(&p, 0, sizeof p);
    pobj_limpiar(&p);
    geo_t grp = { 0 };
    int gch_x = 0, gch_y = 0, gch_cx = 0, gch_cy = 0;
    bool en_grupo = false;
    int grp_prof = 0;
    bool tiene_trans = false;
    bool en_bg = false, bg_grad = false;
    int bg_cols = 0;
    while (xml_sig(&x, &t)) {
        if (t.tipo == 3) {
            if (p.kind && en_pila(pila, np, "t")) {
                if (p.en_celda) texto_a_buf(&p.celda_txt, t.tx, t.tn);
                else {
                    if (!p.primer_run) p.primer_run = true;
                    texto_a_buf(&p.txt, t.tx, t.tn);
                }
            }
            continue;
        }
        if (t.tipo == 2) {
            if (np > 0) np--;
            if (loc_es(&t, "p") && p.kind && !p.en_celda) p.par_abierto = false;
            if (loc_es(&t, "tc") && p.kind == 3) {
                if (p.celda_txt.n) comp_buf_somar(&p.celdas, p.celda_txt.d, p.celda_txt.n);
                comp_buf_somar(&p.celdas, "\x1f", 1);
                p.celda_txt.n = 0;
                p.en_celda = false;
            }
            if (loc_es(&t, "tr") && p.kind == 3) p.filas++;
            if (loc_es(&t, "bg")) en_bg = false;
            if (loc_es(&t, "sp") || loc_es(&t, "pic") || loc_es(&t, "graphicFrame") || loc_es(&t, "cxnSp")) {
                if (p.kind == 3 && p.celdas.n > 0) p.celdas.n--;   /* el último separador sobra */
                gtr_t gt = { grp.x / EMU, grp.y / EMU, grp.cx / EMU, grp.cy / EMU, (int)(gch_x / EMU), (int)(gch_y / EMU), (int)(gch_cx / EMU), (int)(gch_cy / EMU), false };
                gt.ok = en_grupo && gt.chcx > 0 && gt.chcy > 0;
                finalizar_obj(d, s, &p, ic, layout, ln, master, mn, zip, slide_path, srels, srn, &gt);
            }
            if (loc_es(&t, "grpSp") && grp_prof > 0) { grp_prof--; if (!grp_prof) en_grupo = false; }
            continue;
        }
        /* apertura */
        char nombre[24];
        int nl = t.ln > 23 ? 23 : t.ln;
        memcpy(nombre, t.loc, (size_t)nl); nombre[nl] = 0;
        if (!t.vacio && np < PILA_MAX) copia_texto(pila[np++], nombre, 24);
        if (loc_es(&t, "sld")) { char v[8]; if (xattr(&t, "show", v, sizeof v) && v[0] == '0') s->oculta = 1; }
        else if (loc_es(&t, "grpSp")) { ic->avisos |= AV_GRUPOS; en_grupo = true; grp_prof++; memset(&grp, 0, sizeof grp); gch_cx = gch_cy = gch_x = gch_y = 0; }
        else if (loc_es(&t, "sp") && !p.kind) { pobj_limpiar(&p); p.kind = 1; }
        else if (loc_es(&t, "cxnSp")) { pobj_limpiar(&p); p.kind = 4; p.forma = PF_LINEA; }
        else if (loc_es(&t, "pic")) { pobj_limpiar(&p); p.kind = 2; }
        else if (loc_es(&t, "graphicFrame")) { pobj_limpiar(&p); p.kind = 3; }
        else if (loc_es(&t, "timing")) ic->avisos |= AV_ANIM;
        else if (loc_es(&t, "custGeom")) ic->avisos |= AV_FORMA;
        else if (loc_es(&t, "oleObj") || loc_es(&t, "chart") || loc_es(&t, "dgm")) ic->avisos |= AV_GRAFICO;
        else if (loc_es(&t, "bg")) { en_bg = true; bg_grad = false; bg_cols = 0; }
        else if (loc_es(&t, "transition") && !tiene_trans) {
            tiene_trans = true;
            int adv = xnum(&t, "advTm", 0);
            if (adv > 0) s->auto_seg = (adv + 500) / 1000;
        }
        else if (!s->trans && np > 0 && (en_pila(pila, np, "transition"))) {
            if (loc_es(&t, "fade")) s->trans = PR_TRANS_FUNDIDO;
            else if (loc_es(&t, "push") || loc_es(&t, "pull") || loc_es(&t, "cover") || loc_es(&t, "uncover")) s->trans = PR_TRANS_DESPLAZAR;
            else if (loc_es(&t, "wipe") || loc_es(&t, "split") || loc_es(&t, "blinds") || loc_es(&t, "strips") || loc_es(&t, "checker") || loc_es(&t, "randomBar")) s->trans = PR_TRANS_BARRIDO;
        }
        if (en_bg && !p.kind) {
            if (loc_es(&t, "srgbClr")) {
                char v[16];
                if (xattr(&t, "val", v, sizeof v)) {
                    if (bg_grad) { if (bg_cols == 0) s->fondo1 = hex6(v); else if (bg_cols == 1) s->fondo2 = hex6(v); bg_cols++; s->fondo_tipo = PR_FONDO_DEGRADADO; }
                    else { s->fondo_tipo = PR_FONDO_COLOR; s->fondo1 = hex6(v); }
                }
            } else if (loc_es(&t, "gradFill")) { bg_grad = true; bg_cols = 0; }
            else if (loc_es(&t, "blip")) {
                char id[32], dest[200], ruta[200];
                if (xattr(&t, "embed", id, sizeof id) && srels && rel_buscar(srels, srn, id, NULL, dest, sizeof dest)) {
                    ruta_unir(slide_path, dest, ruta, sizeof ruta);
                    int mn2 = 0;
                    uint8_t *dat = zleer(zip, ruta, &mn2);
                    if (dat) { int m = pr_medio_anadir(d, dat, mn2); libera(dat); if (m >= 0) { s->fondo_tipo = PR_FONDO_IMAGEN; s->fondo_medio = m; } }
                }
            }
            continue;
        }
        if (en_grupo && !p.kind) {   /* la transformación del propio grupo */
            if (loc_es(&t, "off") && en_pila(pila, np, "grpSpPr")) { grp.x = xnum(&t, "x", 0); grp.y = xnum(&t, "y", 0); }
            else if (loc_es(&t, "ext") && en_pila(pila, np, "grpSpPr")) { grp.cx = xnum(&t, "cx", 0); grp.cy = xnum(&t, "cy", 0); grp.ok = grp.cx > 0; }
            else if (loc_es(&t, "chOff")) { gch_x = xnum(&t, "x", 0); gch_y = xnum(&t, "y", 0); }
            else if (loc_es(&t, "chExt")) { gch_cx = xnum(&t, "cx", 0); gch_cy = xnum(&t, "cy", 0); }
            continue;
        }
        if (!p.kind) continue;
        /* dentro de un objeto */
        bool en_ln = en_pila(pila, np, "ln");
        bool en_rpr = en_pila(pila, np, "rPr") || en_pila(pila, np, "defRPr") || en_pila(pila, np, "endParaRPr");
        bool en_sppr = en_pila(pila, np, "spPr");
        bool en_tcpr = en_pila(pila, np, "tcPr");
        if (loc_es(&t, "ph")) {
            p.es_ph = true;
            xattr(&t, "type", p.ph_tipo, sizeof p.ph_tipo);
            p.ph_idx = xnum(&t, "idx", -1);
            if (compara_texto(p.ph_tipo, "ftr") == 0 || compara_texto(p.ph_tipo, "sldNum") == 0 || compara_texto(p.ph_tipo, "dt") == 0) p.desconocida = true;
        } else if (loc_es(&t, "cNvSpPr")) { char v[8]; if (xattr(&t, "txBox", v, sizeof v) && v[0] == '1') p.txbox = true; }
        else if (loc_es(&t, "off") && (en_pila(pila, np, "xfrm"))) { p.g.x = xnum(&t, "x", 0); p.g.y = xnum(&t, "y", 0); }
        else if (loc_es(&t, "ext") && (en_pila(pila, np, "xfrm"))) { p.g.cx = xnum(&t, "cx", 0); p.g.cy = xnum(&t, "cy", 0); p.g.ok = p.g.cx > 0 || p.kind == 4; }
        else if (loc_es(&t, "prstGeom")) {
            char v[32];
            xattr(&t, "prst", v, sizeof v);
            if (compara_texto(v, "rect") == 0) p.forma = PF_RECT;
            else if (compara_texto(v, "roundRect") == 0) p.forma = PF_REDONDEADO;
            else if (compara_texto(v, "ellipse") == 0) p.forma = PF_ELIPSE;
            else if (compara_texto(v, "triangle") == 0 || compara_texto(v, "rtTriangle") == 0) p.forma = PF_TRIANGULO;
            else if (compara_texto(v, "diamond") == 0) p.forma = PF_ROMBO;
            else if (compara_texto(v, "rightArrow") == 0) p.forma = PF_FLECHA;
            else if (compara_texto(v, "line") == 0 || compara_texto(v, "straightConnector1") == 0) p.forma = PF_LINEA;
            else if (compara_texto(v, "star5") == 0) p.forma = PF_ESTRELLA;
            else { p.forma = PF_RECT; ic->avisos |= AV_FORMA; }
        } else if (loc_es(&t, "blip") && p.kind == 2) xattr(&t, "embed", p.embed, sizeof p.embed);
        else if (loc_es(&t, "noFill")) {
            if (en_ln && en_sppr) p.sin_borde = true;
            else if (en_sppr) { p.rell_def = true; p.rell = false; }
        } else if (loc_es(&t, "ln") && en_sppr) { p.borde_w = xnum(&t, "w", 12700); }
        else if (loc_es(&t, "srgbClr")) {
            char v[16];
            if (xattr(&t, "val", v, sizeof v)) {
                uint32_t c = hex6(v);
                if (en_rpr) { if (!p.col_def) { p.col = c; p.col_def = true; } }
                else if (en_ln && en_sppr) p.c_borde = c;
                else if (en_sppr && !en_tcpr) { p.rell = true; p.c_rell = c; }
            }
        } else if (loc_es(&t, "solidFill") && en_sppr && !en_ln && !en_rpr) { p.rell = true; if (!p.c_rell) p.c_rell = PR_COL_TEMA; }
        else if (loc_es(&t, "schemeClr") && en_sppr && !en_ln && !en_rpr && !en_tcpr) { p.rell = true; p.c_rell = PR_COL_TEMA; }
        else if (loc_es(&t, "bodyPr")) {
            char v[8];
            if (xattr(&t, "anchor", v, sizeof v)) p.valign = compara_texto(v, "ctr") == 0 ? 1 : compara_texto(v, "b") == 0 ? 2 : 0;
        } else if (loc_es(&t, "p") && (p.kind == 1 || p.kind == 4) && !p.en_celda) {
            if (p.par_n > 0) comp_buf_somar(&p.txt, "\n", 1);
            p.par_n++; p.par_abierto = true; p.lvl = 0;
        } else if (loc_es(&t, "pPr") && p.kind == 1) {
            p.lvl = xnum(&t, "lvl", 0);
            for (int i = 0; i < p.lvl && i < 4; i++) comp_buf_somar(&p.txt, "\t", 1);
            char v[8];
            if (xattr(&t, "algn", v, sizeof v) && !p.alin_def) { p.alin_def = true; p.alin = compara_texto(v, "ctr") == 0 ? 1 : compara_texto(v, "r") == 0 ? 2 : 0; }
        } else if (loc_es(&t, "buChar")) { if (p.kind == 1) p.lista = PR_LISTA_VINETA; }
        else if (loc_es(&t, "buAutoNum")) { if (p.kind == 1) p.lista = PR_LISTA_NUMERO; }
        else if (loc_es(&t, "rPr") && p.kind == 1) {
            if (!p.tam) { int sz = xnum(&t, "sz", 0); if (sz) p.tam = (sz + 50) / 100; }
            char v[8];
            if (!p.est && xattr(&t, "b", v, sizeof v) && v[0] == '1') p.est |= PR_NEGRITA;
            if (xattr(&t, "i", v, sizeof v) && v[0] == '1') p.est |= PR_CURSIVA;
            if (xattr(&t, "u", v, sizeof v) && compara_texto(v, "none") != 0) p.est |= PR_SUBRAYADO;
        } else if ((loc_es(&t, "endParaRPr") || loc_es(&t, "defRPr")) && p.kind == 1) {
            if (!p.tam) { int sz = xnum(&t, "sz", 0); if (sz) p.tam = (sz + 50) / 100; }
        } else if (loc_es(&t, "latin") && en_rpr && p.kind == 1) {
            char v[48];
            if (xattr(&t, "typeface", v, sizeof v) && v[0] != '+' && !p.fuente[0]) { pr_utf8_a_latin1(v, (int)tam_texto(v), p.fuente, sizeof p.fuente); }
        } else if (loc_es(&t, "tbl")) { p.filas = 0; p.cols = 0; }
        else if (loc_es(&t, "gridCol")) p.cols++;
        else if (loc_es(&t, "tc") && p.kind == 3) p.en_celda = true;
        else if (loc_es(&t, "a") && p.kind == 3) { /* nada */ }
    }
    pobj_limpiar(&p);
    return true;
}

/* El texto de las notas (el marcador «body» de la diapositiva de notas). */
static void importar_notas(pr_diap_t *s, const uint8_t *xml, int n)
{
    xml_t x = { (const char *)xml, n, 0 };
    xtok_t t;
    bool en_sp = false, body = false, ph_img = false;
    buf_t b = { 0 };
    int np_ = 0;
    char pila[PILA_MAX][24];
    int sp = 0;
    while (xml_sig(&x, &t)) {
        if (t.tipo == 1 && loc_es(&t, "sp")) { en_sp = true; body = false; ph_img = false; np_ = 0; }
        else if (t.tipo == 2 && loc_es(&t, "sp")) en_sp = false;
        else if (en_sp && t.tipo == 1 && loc_es(&t, "ph")) {
            char v[24];
            xattr(&t, "type", v, sizeof v);
            body = compara_texto(v, "body") == 0;
            ph_img = compara_texto(v, "sldImg") == 0;
        } else if (en_sp && body && !ph_img && t.tipo == 1 && loc_es(&t, "p")) { if (np_++ > 0) comp_buf_somar(&b, "\n", 1); }
        else if (t.tipo == 1 && !t.vacio && sp < PILA_MAX) { char nm[24]; int l = t.ln > 23 ? 23 : t.ln; memcpy(nm, t.loc, (size_t)l); nm[l] = 0; copia_texto(pila[sp++], nm, 24); }
        else if (t.tipo == 2 && sp > 0) sp--;
        else if (t.tipo == 3 && en_sp && body && en_pila(pila, sp, "t")) texto_a_buf(&b, t.tx, t.tn);
    }
    if (b.n) {
        comp_buf_somar(&b, "", 1);
        char *c = aloca((size_t)b.n);
        if (c) { memcpy(c, b.d, (size_t)b.n); libera(s->notas); s->notas = c; }
    }
    comp_buf_liberar(&b);
}

pr_deck_t *pr_importar_pptx(const uint8_t *dat, int n, int *err, char *avisos, int max_avisos)
{
    zip_leitor_t z;
    int e0; if (!err) err = &e0;
    *err = 0;
    if (zip_abrir(&z, dat, n) != 0) { *err = PR_ERR_FORMATO; return NULL; }
    int pn = 0, rn = 0;
    uint8_t *pres = zleer(&z, "ppt/presentation.xml", &pn);
    uint8_t *prels = zleer(&z, "ppt/_rels/presentation.xml.rels", &rn);
    if (!pres || !prels) { libera(pres); libera(prels); zip_leitor_liberar(&z); *err = PR_ERR_FORMATO; return NULL; }
    impctx_t ic = { 0, 1000, 1000, 9144000, 6858000 };
    {   /* tamaño */
        xml_t x = { (const char *)pres, pn, 0 };
        xtok_t t;
        while (xml_sig(&x, &t)) if (t.tipo == 1 && loc_es(&t, "sldSz")) { ic.ow = xnum(&t, "cx", ic.ow); ic.oh = xnum(&t, "cy", ic.oh); break; }
    }
    bool ancha = (long)ic.ow * 2 > (long)ic.oh * 3;
    pr_deck_t *d = pr_deck_vacio(ancha);
    if (!d) { libera(pres); libera(prels); zip_leitor_liberar(&z); *err = PR_ERR_MEMORIA; return NULL; }
    ic.sx = (int)((long)d->ancho * 1000 / (ic.ow / EMU > 0 ? ic.ow / EMU : 960));
    ic.sy = (int)((long)d->alto * 1000 / (ic.oh / EMU > 0 ? ic.oh / EMU : 540));
    /* orden de las diapositivas */
    xml_t x = { (const char *)pres, pn, 0 };
    xtok_t t;
    int total = 0;
    while (xml_sig(&x, &t) && total < PR_DIAP_MAX) {
        if (t.tipo != 1 || !loc_es(&t, "sldId")) continue;
        char dest[200], ruta[200];
        /* el atributo r:id comparte nombre local «id» con id: se busca el que empieza por «rId» o el último */
        char rid[32] = "";
        const char *p = t.at, *e = t.at + t.an;
        while (p < e) {   /* recorre los atributos y se queda con el prefijado r: */
            while (p < e && (*p == ' ' || *p == '\n' || *p == '\t')) p++;
            const char *ini = p;
            while (p < e && *p != '=') p++;
            const char *fin = p;
            while (p < e && *p != '"' && *p != '\'') p++;
            if (p >= e) break;
            char q = *p++;
            const char *v = p;
            while (p < e && *p != q) p++;
            if (fin - ini == 4 && memcmp(ini, "r:id", 4) == 0) { int l = (int)(p - v); if (l > 31) l = 31; memcpy(rid, v, (size_t)l); rid[l] = 0; }
            p++;
        }
        if (!rid[0]) continue;
        if (!rel_buscar(prels, rn, rid, NULL, dest, sizeof dest)) continue;
        ruta_unir("ppt/presentation.xml", dest, ruta, sizeof ruta);
        int sn = 0;
        uint8_t *sx = zleer(&z, ruta, &sn);
        if (!sx) continue;
        pr_diap_t *s = pr_diap_vacia(d);
        if (!s) { libera(sx); break; }
        char rp[200]; int srn = 0, lyn = 0, mn = 0, nn = 0;
        rels_de(ruta, rp, sizeof rp);
        uint8_t *srels = zleer(&z, rp, &srn);
        uint8_t *ly = NULL, *ms = NULL, *nts = NULL;
        char dl[200], rl[200];
        if (srels && rel_buscar(srels, srn, NULL, "/slideLayout", dl, sizeof dl)) {
            ruta_unir(ruta, dl, rl, sizeof rl);
            ly = zleer(&z, rl, &lyn);
            if (ly) {
                char rp2[200], dm[200], rm[200];
                rels_de(rl, rp2, sizeof rp2);
                int lrn = 0;
                uint8_t *lrels = zleer(&z, rp2, &lrn);
                if (lrels && rel_buscar(lrels, lrn, NULL, "/slideMaster", dm, sizeof dm)) { ruta_unir(rl, dm, rm, sizeof rm); ms = zleer(&z, rm, &mn); }
                libera(lrels);
            }
        }
        importar_slide(d, s, sx, sn, &ic, ly, lyn, ms, mn, &z, ruta, srels, srn);
        if (srels && rel_buscar(srels, srn, NULL, "/notesSlide", dl, sizeof dl)) {
            ruta_unir(ruta, dl, rl, sizeof rl);
            nts = zleer(&z, rl, &nn);
            if (nts) importar_notas(s, nts, nn);
        }
        /* el diseño de la diapositiva: título y contenido si trae marcadores */
        bool tit = false, cue = false;
        for (int k = 0; k < s->n; k++) { if (s->obj[k].rol == PR_ROL_TITULO) tit = true; if (s->obj[k].rol >= PR_ROL_CUERPO) cue = true; }
        s->layout = (uint8_t)(tit && cue ? PR_LY_TITULO_CONTENIDO : tit ? PR_LY_SOLO_TITULO : PR_LY_BLANCO);
        libera(sx); libera(srels); libera(ly); libera(ms); libera(nts);
        total++;
    }
    libera(pres); libera(prels);
    zip_leitor_liberar(&z);
    if (d->n == 0) {
        if (!pr_diap_nueva(d, 0, PR_LY_TITULO)) { pr_liberar(d); *err = PR_ERR_MEMORIA; return NULL; }
    }
    if (avisos && max_avisos > 0) {
        avisos[0] = 0;
        int k = 0;
        if (ic.avisos & AV_GRUPOS) k += formatar_texto(avisos + k, (size_t)(max_avisos - k), "grupos aplanados; ");
        if (ic.avisos & AV_ANIM) k += formatar_texto(avisos + k, (size_t)(max_avisos - k), "animaciones ignoradas; ");
        if (ic.avisos & AV_GRAFICO) k += formatar_texto(avisos + k, (size_t)(max_avisos - k), "gráficos y objetos incrustados ignorados; ");
        if (ic.avisos & AV_FORMA) k += formatar_texto(avisos + k, (size_t)(max_avisos - k), "formas especiales simplificadas; ");
        if (ic.avisos & AV_OTRO) k += formatar_texto(avisos + k, (size_t)(max_avisos - k), "alguna imagen no se pudo leer; ");
    }
    return d;
}
