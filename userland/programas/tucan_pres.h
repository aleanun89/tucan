// SPDX-License-Identifier: GPL-2.0-or-later
/* tucan_pres.h - el modo presentación de Tucán (D369): pantalla completa, avance con teclado, ratón y rueda, transiciones
 * (corte, fundido, desplazar, barrido), animaciones de entrada por objeto (aparecer, fundido), puntero láser,
 * pantalla negra o blanca, ir a una diapositiva por número y avance automático con tiempos.
 * Se incluye en tucan.c. Cada diapositiva se compone en un búfer del tamaño de la ventana (con franjas negras si
 * la proporción no coincide) y las transiciones mezclan dos de esos búferes. */

static uint32_t *pres_frame;                 /* lo que se ve, del tamaño de la ventana */
static int pres_fw, pres_fh;
static int laser_x = -1, laser_y = -1;

static int pres_pasos_de(int i) { return pr_pasos(dk->d[i]); }
static int pres_paso_render(int i, int paso) { return pres_pasos_de(i) ? paso : -1; }

static void pres_asegurar(void)
{
    if (pres_frame && pres_fw == j->larg && pres_fh == j->alt) return;
    libera(pres_frame);
    pres_frame = aloca((size_t)j->larg * (size_t)j->alt * 4);
    pres_fw = j->larg; pres_fh = j->alt;
}

/* Compone la diapositiva i (con paso de animación) en un búfer del tamaño de la ventana. */
static void pres_componer(int i, int paso, uint32_t *dst)
{
    int W = j->larg, H = j->alt;
    for (int k = 0; k < W * H; k++) dst[k] = 0;
    int fw = W, fh = fw * dk->alto / dk->ancho;
    if (fh > H) { fh = H; fw = fh * dk->ancho / dk->alto; }
    if (fw > 2300) { fw = 2300; fh = fw * dk->alto / dk->ancho; }
    int ox = (W - fw) / 2, oy = (H - fh) / 2;
    uint32_t *tmp = aloca((size_t)fw * (size_t)fh * 4);
    if (!tmp) return;
    pr_dibujar(dk, i, tmp, fw, fh, 0, pres_paso_render(i, paso), -1);
    for (int y = 0; y < fh; y++) memcpy(dst + (oy + y) * W + ox, tmp + y * fw, (size_t)fw * 4);
    libera(tmp);
}

static void pres_laser_dibujar(void)
{
    if (!pres_laser || pres_mx < 0) return;
    int r = 7;
    for (int y = -r; y <= r; y++)
        for (int x = -r; x <= r; x++) {
            int d2 = x * x + y * y;
            if (d2 > r * r) continue;
            int X = pres_mx + x, Y = pres_my + y;
            if (X < 0 || Y < 0 || X >= j->larg || Y >= j->alt) continue;
            j->px[Y * j->larg + X] = d2 > (r - 2) * (r - 2) ? 0xFF8080 : 0xFF0000;
        }
}

/* Lo que hay en el búfer, más el láser y el número que se está escribiendo. */
static void pres_mostrar(void)
{
    if (!pres_frame) return;
    if (pres_negra || pres_blanca) { for (int k = 0; k < j->larg * j->alt; k++) j->px[k] = pres_blanca ? 0xFFFFFF : 0; }
    else memcpy(j->px, pres_frame, (size_t)j->larg * (size_t)j->alt * 4);
    pres_laser_dibujar();
    if (pres_num[0]) {
        g_retangulo(j, 8, j->alt - 30, 8 * (int)tam_texto(pres_num) + 12, 22, 0x202020);
        g_texto(j, 14, j->alt - 27, pres_num, 0xFFFFFF);
    }
    jan_mostrar(j);
    laser_x = pres_mx; laser_y = pres_my;
}

static void pres_pintar(int trans)
{
    (void)trans;
    pres_asegurar();
    if (!pres_frame) return;
    pres_componer(pres_i, pres_paso, pres_frame);
    pres_mostrar();
    pres_t0 = tempo_ms();
}

static inline uint32_t mezcla_px(uint32_t a, uint32_t b, int p)   /* p 0..256 */
{
    uint32_t rb = (((a & 0xFF00FF) * (uint32_t)(256 - p) + (b & 0xFF00FF) * (uint32_t)p) >> 8) & 0xFF00FF;
    uint32_t g = (((a & 0x00FF00) * (uint32_t)(256 - p) + (b & 0x00FF00) * (uint32_t)p) >> 8) & 0x00FF00;
    return rb | g;
}

/* De A a B (dos búferes del tamaño de la ventana) en dur ms. */
static void pres_transicion(const uint32_t *A, const uint32_t *B, int tipo, int dur)
{
    int W = j->larg, H = j->alt;
    uint32_t t0 = tempo_ms();
    for (;;) {
        uint32_t el = tempo_ms() - t0;
        int p = el >= (uint32_t)dur ? 256 : (int)(el * 256 / (uint32_t)dur);
        if (tipo == PR_TRANS_FUNDIDO) {
            for (int k = 0; k < W * H; k++) j->px[k] = mezcla_px(A[k], B[k], p);
        } else if (tipo == PR_TRANS_DESPLAZAR) {
            int d = W * p / 256;
            for (int y = 0; y < H; y++) {
                memcpy(j->px + y * W, A + y * W + d, (size_t)(W - d) * 4);
                memcpy(j->px + y * W + (W - d), B + y * W, (size_t)d * 4);
            }
        } else {   /* barrido de izquierda a derecha con un borde suave */
            int c = W * p / 256;
            for (int y = 0; y < H; y++) {
                memcpy(j->px + y * W, B + y * W, (size_t)c * 4);
                memcpy(j->px + y * W + c, A + y * W + c, (size_t)(W - c) * 4);
                if (c < W) j->px[y * W + c] = 0xFFFFFF;
            }
        }
        jan_mostrar(j);
        if (p >= 256) break;
    }
}

static int pres_dur(const pr_diap_t *s, int tipo)
{
    if (s->dur_trans) return s->dur_trans * 100;
    return tipo == PR_TRANS_FUNDIDO ? 500 : 450;
}

static void pres_log(void) { registrar("tucan: presentacion diapositiva %d paso %d\n", pres_i + 1, pres_paso); }

/* Siguiente diapositiva visible tras i (o -1) y anterior. */
static int pres_sig(int i) { for (int k = i + 1; k < dk->n; k++) if (!dk->d[k]->oculta) return k; return -1; }
static int pres_ant(int i) { for (int k = i - 1; k >= 0; k--) if (!dk->d[k]->oculta) return k; return -1; }

static void pres_salir(void)
{
    if (!pres) return;
    pres = false; pres_negra = pres_blanca = false;
    libera(pres_frame); pres_frame = NULL; pres_fw = pres_fh = 0;
    jan_cursor_visivel(j, true);
    jan_tela_cheia(j, false);
    registrar("tucan: presentacion fin\n");
    cur = pres_i < dk->n ? pres_i : 0;
    layout();
    redibujar_todo();
}

/* Pasa a la diapositiva k con la transición de la de destino (corte si trans < 0). */
static void pres_ir(int k, int paso, bool con_trans)
{
    pres_asegurar();
    if (!pres_frame) return;
    pr_diap_t *s = dk->d[k];
    size_t n = (size_t)j->larg * (size_t)j->alt;
    if (con_trans && s->trans != PR_TRANS_CORTE && !pres_negra && !pres_blanca) {
        uint32_t *B = aloca(n * 4);
        if (B) {
            pres_componer(k, paso, B);
            pres_transicion(pres_frame, B, s->trans, pres_dur(s, s->trans));
            memcpy(pres_frame, B, n * 4);
            libera(B);
        }
    }
    pres_i = k; pres_paso = paso;
    pres_negra = pres_blanca = false;
    pres_pintar(-1);
    pres_log();
}

static void pres_avanzar(void)
{
    pr_diap_t *s = dk->d[pres_i];
    int pasos = pr_pasos(s);
    if (pasos && pres_paso < pasos) {   /* el siguiente objeto animado */
        int kk = 0, tipo = 0;
        for (int q = 0; q < s->n; q++) if (s->obj[q].anim && ++kk == pres_paso + 1) tipo = s->obj[q].anim;
        pres_asegurar();
        if (tipo == PR_ANIM_FUNDIDO && pres_frame) {
            size_t n = (size_t)j->larg * (size_t)j->alt;
            uint32_t *B = aloca(n * 4);
            if (B) {
                pres_componer(pres_i, pres_paso + 1, B);
                pres_transicion(pres_frame, B, PR_TRANS_FUNDIDO, 400);
                libera(B);
            }
        }
        pres_paso++;
        pres_negra = pres_blanca = false;
        pres_pintar(-1);
        pres_log();
        return;
    }
    int k = pres_sig(pres_i);
    if (k < 0) { pres_salir(); return; }
    pres_ir(k, 0, true);
}

static void pres_retroceder(void)
{
    if (pr_pasos(dk->d[pres_i]) && pres_paso > 0) { pres_paso--; pres_negra = pres_blanca = false; pres_pintar(-1); pres_log(); return; }
    int k = pres_ant(pres_i);
    if (k < 0) { pres_negra = pres_blanca = false; pres_pintar(-1); return; }
    pres_ir(k, pr_pasos(dk->d[k]), false);
}

static void presentar(int desde)
{
    notas_a_diap();
    int i = desde < 0 ? 0 : desde;
    while (i < dk->n && dk->d[i]->oculta) i++;
    if (i >= dk->n) { mensagem("Tucán", _("Todas las diapositivas están ocultas.")); return; }
    pres = true; pres_entrando = true;
    pres_i = i; pres_paso = 0;
    pres_negra = pres_blanca = false;
    pres_laser = laser_opc;
    pres_num[0] = 0; pres_mx = pres_my = -1;
    edit_obj = -1;
    registrar("tucan: presentacion inicio en %d\n", i + 1);
    jan_tela_cheia(j, true);
    if (pres_laser) jan_cursor_visivel(j, false);
    pres_pintar(-1);
    pres_log();
}

static int pres_espera(void)
{
    if (!dk->auto_avance) return -1;
    pr_diap_t *s = dk->d[pres_i];
    if (s->auto_seg <= 0) return -1;
    int resto = s->auto_seg * 1000 - (int)(tempo_ms() - pres_t0);
    return resto < 0 ? 0 : resto;
}
static void pres_tiempo(void)
{
    if (!pres) return;
    registrar("tucan: presentacion avance automatico\n");
    pres_avanzar();
}

static void pres_laser_mover(int x, int y)
{
    pres_mx = x; pres_my = y;
    if (!pres_laser || !pres_frame) return;
    /* se restaura lo que había bajo el punto anterior y se dibuja en el nuevo sitio */
    int r = 8;
    if (laser_x >= 0) {
        for (int yy = laser_y - r; yy <= laser_y + r; yy++) {
            if (yy < 0 || yy >= j->alt) continue;
            int x0 = laser_x - r < 0 ? 0 : laser_x - r, x1 = laser_x + r >= j->larg ? j->larg - 1 : laser_x + r;
            if (x1 >= x0) memcpy(j->px + yy * j->larg + x0, pres_frame + yy * j->larg + x0, (size_t)(x1 - x0 + 1) * 4);
        }
        jan_mostrar_area(j, laser_x - r, laser_y - r, 2 * r + 1, 2 * r + 1);
    }
    pres_laser_dibujar();
    jan_mostrar_area(j, x - r, y - r, 2 * r + 1, 2 * r + 1);
    laser_x = x; laser_y = y;
}

static void pres_evento(const evento_t *e)
{
    if (e->tipo == EV_MOVER) { if (pres_laser) pres_laser_mover(e->x, e->y); return; }
    if (e->tipo == EV_RODA) { if ((int)e->cliques > 0) pres_avanzar(); else pres_retroceder(); return; }
    if (e->tipo == EV_CLIQUE) {
        if (e->botoes & 2) pres_retroceder(); else pres_avanzar();
        return;
    }
    if (e->tipo != EV_TECLA) return;
    unsigned t = e->tecla;
    if (t >= '0' && t <= '9') {
        size_t l = tam_texto(pres_num);
        if (l < 5) { pres_num[l] = (char)t; pres_num[l + 1] = 0; }
        pres_mostrar();
        return;
    }
    if ((t == '\n' || t == '\r') && pres_num[0]) {
        int n = texto_para_int(pres_num) - 1;
        pres_num[0] = 0;
        if (n < 0) n = 0;
        if (n >= dk->n) n = dk->n - 1;
        while (n < dk->n && dk->d[n]->oculta) n++;
        if (n >= dk->n) n = pres_ant(dk->n);
        pres_ir(n, 0, false);
        return;
    }
    pres_num[0] = 0;
    switch (t) {
    case TECLA_ESC: case 'q': case 'Q': pres_salir(); return;
    case TECLA_DIREITA: case TECLA_BAIXO: case TECLA_PGDN: case ' ': case '\n': case '\r': case 'n': case 'N': pres_avanzar(); return;
    case TECLA_ESQUERDA: case TECLA_CIMA: case TECLA_PGUP: case '\b': case 'p': case 'P': pres_retroceder(); return;
    case TECLA_HOME: { int k = pres_sig(-1); if (k >= 0) pres_ir(k, 0, false); return; }
    case TECLA_END: { int k = pres_ant(dk->n); if (k >= 0) pres_ir(k, pr_pasos(dk->d[k]), false); return; }
    case 'b': case 'B': case '.': pres_negra = !pres_negra; pres_blanca = false; pres_mostrar(); registrar("tucan: presentacion pantalla negra %d\n", pres_negra); return;
    case 'w': case 'W': case ',': pres_blanca = !pres_blanca; pres_negra = false; pres_mostrar(); return;
    case 'l': case 'L':
        pres_laser = !pres_laser;
        jan_cursor_visivel(j, !pres_laser);
        pres_mostrar();
        registrar("tucan: presentacion laser %d\n", pres_laser);
        return;
    default: return;
    }
}
