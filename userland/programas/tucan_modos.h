// SPDX-License-Identifier: GPL-2.0-or-later
/* tucan_modos.h - los modos sin ventana de Tucán (para las pruebas y para convertir desde el terminal):
 *   tucan --convertir entrada salida [--notas] [--diap N] [--ancho PX]   .tcn/.pptx -> .tcn/.pptx/.pdf/.png
 *   tucan --info archivo                                                 diapositivas, objetos y textos
 *   tucan --demo salida                                                  una presentación de ejemplo (.tcn/.pptx/.pdf/.png)
 *   tucan --fuentes                                                      familias de fuentes disponibles
 * Se incluye en tucan.c. */

static bool termina_en(const char *t, const char *suf)
{
    int a = (int)tam_texto(t), b = (int)tam_texto(suf);
    if (a < b) return false;
    for (int i = 0; i < b; i++) {
        char x = t[a - b + i], y = suf[i];
        if (x >= 'A' && x <= 'Z') x = (char)(x + 32);
        if (x != y) return false;
    }
    return true;
}

/* Guarda según la extensión de la ruta; 0 = ok. */
static int guardar_segun_ext(pr_deck_t *d, const char *ruta, bool notas, int diap, int ancho)
{
    if (termina_en(ruta, ".pdf")) return pr_exportar_pdf(d, ruta, notas);
    if (termina_en(ruta, ".pptx")) return pr_exportar_pptx(d, ruta);
    if (termina_en(ruta, ".png")) return pr_exportar_png(d, diap, ruta, ancho);
    return pr_guardar(d, ruta);
}

static int modo_convertir(const char *ent, const char *sai, bool notas, int diap, int ancho)
{
    int err = 0;
    pr_deck_t *d = pr_abrir(ent, &err);
    if (!d) { imprimir("tucan: no se puede abrir %s (%s)\n", ent, pr_err_texto(err)); return 1; }
    int rc = guardar_segun_ext(d, sai, notas, diap, ancho);
    imprimir("tucan: convertido %s -> %s (%d diapositivas, rc=%d)%s%s\n", ent, sai, d->n, rc, pr_avisos[0] ? " avisos: " : "", pr_avisos);
    pr_liberar(d);
    return rc ? 1 : 0;
}

static int modo_info(const char *ruta)
{
    int err = 0;
    pr_deck_t *d = pr_abrir(ruta, &err);
    if (!d) { imprimir("tucan: no se puede abrir %s (%s)\n", ruta, pr_err_texto(err)); return 1; }
    imprimir("tucan: %d diapositivas %dx%d tema %s medios=%d\n", d->n, d->ancho, d->alto, pr_temas[d->tema].nombre, d->n_medios);
    if (pr_avisos[0]) imprimir("tucan: avisos: %s\n", pr_avisos);
    for (int i = 0; i < d->n; i++) {
        pr_diap_t *s = d->d[i];
        imprimir("tucan: diap %d objetos=%d layout=%d trans=%d oculta=%d pasos=%d\n", i + 1, s->n, s->layout, s->trans, s->oculta, pr_pasos(s));
        for (int k = 0; k < s->n; k++) {
            pr_obj_t *o = &s->obj[k];
            char t[200];
            copia_texto(t, o->texto ? o->texto : "", sizeof t);
            for (char *p = t; *p; p++) if (*p == '\n') *p = '|'; else if (*p == '\x1f') *p = '/'; else if (*p == '\t') *p = '>';
            imprimir("tucan:   obj tipo=%d rol=%d forma=%d x=%d y=%d w=%d h=%d tam=%d texto=[%s]\n", o->tipo, o->rol, o->forma, o->x, o->y, o->w, o->h, o->tam, t);
        }
        if (s->notas && s->notas[0]) {
            char t[200];
            copia_texto(t, s->notas, sizeof t);
            for (char *p = t; *p; p++) if (*p == '\n') *p = '|';
            imprimir("tucan:   notas=[%s]\n", t);
        }
    }
    pr_liberar(d);
    return 0;
}

static pr_obj_t *poner(pr_diap_t *s, int tipo, int x, int y, int w, int h, const char *texto)
{
    pr_obj_t *o = pr_obj_nuevo(s, tipo);
    if (!o) return NULL;
    o->x = (int16_t)x; o->y = (int16_t)y; o->w = (int16_t)w; o->h = (int16_t)h;
    if (texto) pr_obj_poner_texto(o, texto);
    return o;
}

static void rol_texto(pr_diap_t *s, int rol, const char *t)
{
    for (int i = 0; i < s->n; i++) if (s->obj[i].rol == rol) { pr_obj_poner_texto(&s->obj[i], t); return; }
}

/* La presentación de ejemplo (cinco diapositivas con todo tipo de objetos). */
static pr_deck_t *crear_demo(void)
{
    pr_deck_t *d = pr_nueva(true);
    if (!d) return NULL;
    pr_aplicar_tema(d, 1);
    pr_diap_t *s = d->d[0];
    rol_texto(s, PR_ROL_TITULO, "Presentación de ejemplo");
    rol_texto(s, PR_ROL_SUBTITULO, "Tucán en SaguiOS");
    s->trans = PR_TRANS_FUNDIDO;
    pr_diap_t *s1 = pr_diap_nueva(d, 1, PR_LY_TITULO_CONTENIDO);
    rol_texto(s1, PR_ROL_TITULO, "Puntos clave");
    rol_texto(s1, PR_ROL_CUERPO, "Primer punto importante\n\tUn subpunto con más detalle\nSegundo punto con acentos: ñandú, camión\nTercer punto");
    for (int i = 0; i < s1->n; i++) if (s1->obj[i].rol == PR_ROL_CUERPO) s1->obj[i].anim = PR_ANIM_APARECER;
    s1->notas = aloca(40);
    if (s1->notas) copia_texto(s1->notas, "Notas de la segunda diapositiva", 40);
    s1->trans = PR_TRANS_DESPLAZAR;
    pr_diap_t *s2 = pr_diap_nueva(d, 2, PR_LY_SOLO_TITULO);
    rol_texto(s2, PR_ROL_TITULO, "Formas y figuras");
    for (int i = 0; i < s2->n; i++) if (s2->obj[i].rol == PR_ROL_TITULO) copia_texto(s2->obj[i].fuente, "DejaVu Serif", sizeof s2->obj[i].fuente);   /* una fuente del sistema: el PDF la incrusta */
    pr_obj_t *o = poner(s2, PR_FORMA, 60, 150, 200, 120, "Rectángulo");
    if (o) { o->forma = PF_REDONDEADO; o->rellena = 1; o->borde = 2; o->alin = 1; o->valign = 1; o->tam = 20; }
    o = poner(s2, PR_FORMA, 300, 150, 160, 120, "Elipse");
    if (o) { o->forma = PF_ELIPSE; o->rellena = 1; o->relleno = 0x4FC3F7; o->alin = 1; o->valign = 1; o->tam = 20; }
    o = poner(s2, PR_FORMA, 500, 150, 160, 120, NULL);
    if (o) { o->forma = PF_ESTRELLA; o->rellena = 1; o->relleno = 0xFFD54F; }
    o = poner(s2, PR_FORMA, 700, 150, 200, 120, "Flecha");
    if (o) { o->forma = PF_FLECHA; o->rellena = 1; o->relleno = 0xE53935; o->alin = 1; o->valign = 1; o->tam = 18; }
    o = poner(s2, PR_WORDART, 200, 320, 560, 140, "TUCAN");
    if (o) { o->wa_estilo = 3; }
    s2->trans = PR_TRANS_BARRIDO;
    pr_diap_t *s3 = pr_diap_nueva(d, 3, PR_LY_SOLO_TITULO);
    rol_texto(s3, PR_ROL_TITULO, "Datos");
    o = poner(s3, PR_TABLA, 60, 150, 420, 160, "Mes\x1fVentas\x1fGastos\x1f" "Enero\x1f" "10\x1f" "7\x1f" "Febrero\x1f" "14\x1f" "9\x1f" "Marzo\x1f" "21\x1f" "12");
    if (o) { o->filas = 4; o->cols = 3; o->tam = 16; }
    o = poner(s3, PR_GRAFICO, 520, 150, 380, 300, "Enero=10,Febrero=14,Marzo=21,Abril=17");
    if (o) o->graf = PR_GRAF_BARRAS;
    pr_diap_t *s4 = pr_diap_nueva(d, 4, PR_LY_SOLO_TITULO);
    rol_texto(s4, PR_ROL_TITULO, "Una imagen");
    imagem_t im;
    if (imagem_criar(&im, 160, 100, 0)) {
        for (int y = 0; y < 100; y++) for (int x = 0; x < 160; x++) im.px[y * 160 + x] = (uint32_t)((x * 255 / 160) << 16 | (y * 255 / 100) << 8 | 0x80);
        uint8_t *png = NULL; int pn = 0;
        if (imagem_codificar_png(&im, &png, &pn) == 0) {
            int m = pr_medio_anadir(d, png, pn);
            libera(png);
            o = poner(s4, PR_IMAGEN, 280, 150, 400, 250, NULL);
            if (o) { o->medio = m; o->borde = 3; }
        }
        imagem_liberar(&im);
    }
    d->num = true;
    return d;
}

static int modo_demo(const char *sai)
{
    pr_deck_t *d = crear_demo();
    if (!d) return 1;
    int rc = guardar_segun_ext(d, sai, false, 0, 1280);
    imprimir("tucan: demo %s (%d diapositivas, rc=%d)\n", sai, d->n, rc);
    pr_liberar(d);
    return rc ? 1 : 0;
}

static int modo_fuentes(void)
{
    const fuentes_prov_t *p = fuentes_prov_obter();
    int n = p && p->cantidad ? p->cantidad() : 3;
    imprimir("tucan: fuentes %d familias\n", n);
    for (int i = 0; i < n && i < 40; i++) imprimir("tucan: familia %d %s\n", i, pdfw_nombre_familia(i));
    return 0;
}

/* Cuánto cuesta dibujar: una diapositiva completa a varios tamaños, una miniatura, la copia desde la caché que hace el
 * editor al repintar sin cambios, y exportar a PDF y PPTX. Para equipos antiguos (D369). */
static int modo_bench(void)
{
    pr_deck_t *d = crear_demo();
    if (!d) return 1;
    static const int tam[4][2] = { { 1024, 576 }, { 746, 420 }, { 400, 225 }, { 128, 72 } };
    for (int t = 0; t < 4; t++) {
        int w = tam[t][0], h = tam[t][1];
        uint32_t *px = aloca((size_t)w * (size_t)h * 4);
        if (!px) continue;
        pr_dibujar(d, 0, px, w, h, 0, -1, -1);                 /* la primera vez carga fuentes y caché de glifos */
        uint32_t t0 = tempo_ms();
        int n = 0;
        for (int rep = 0; rep < 3; rep++) for (int i = 0; i < d->n; i++) { pr_dibujar(d, i, px, w, h, 0, -1, -1); n++; }
        uint32_t ms = tempo_ms() - t0;
        imprimir("tucan: bench dibujo %dx%d: %d dibujos en %d ms (%d ms cada uno)\n", w, h, n, (int)ms, n ? (int)ms / n : 0);
        if (t == 1) {   /* repintar sin cambios: copiar el lienzo de la caché a la ventana */
            uint32_t *dst = aloca((size_t)w * (size_t)h * 4);
            if (dst) {
                t0 = tempo_ms();
                for (int rep = 0; rep < 200; rep++) memcpy(dst, px, (size_t)w * (size_t)h * 4);
                ms = tempo_ms() - t0;
                imprimir("tucan: bench repintado desde la cache: %d copias en %d ms\n", 200, (int)ms);
                libera(dst);
            }
        }
        libera(px);
    }
    uint32_t t0 = tempo_ms();
    int r1 = pr_exportar_pdf(d, "/tmp/bench.pdf", false);
    imprimir("tucan: bench PDF de %d paginas en %d ms (rc=%d)\n", d->n, (int)(tempo_ms() - t0), r1);
    t0 = tempo_ms();
    int r2 = pr_exportar_pptx(d, "/tmp/bench.pptx");
    imprimir("tucan: bench PPTX en %d ms (rc=%d)\n", (int)(tempo_ms() - t0), r2);
    t0 = tempo_ms();
    pr_deck_t *d2 = pr_abrir("/tmp/bench.pptx", &r2);
    imprimir("tucan: bench importar PPTX en %d ms (%d diapositivas)\n", (int)(tempo_ms() - t0), d2 ? d2->n : 0);
    pr_liberar(d2);
    pr_liberar(d);
    return 0;
}

static int num_arg(const char *s)
{
    int v = 0;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return v;
}
