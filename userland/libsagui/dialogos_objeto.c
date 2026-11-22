// SPDX-License-Identifier: GPL-2.0-or-later
/* dialogos_objeto.c - diálogos de las imágenes y los WordArt del control de texto
 * (mejora 122, D296): la galería de WordArt con su texto, fuente y tamaño, y «Formato de imagen»
 * (tamaño en cm y en %, ajuste del texto, alineación, borde y pie). Ventanas modales al estilo
 * Win98 con los controles compartidos de edicao_rica_dialogos.c. */
#include "gui.h"
#include "wordart.h"

static bool en(int px, int py, int x, int y, int l, int a) { return px >= x && py >= y && px < x + l && py < y + a; }
static void etiqueta(jan_t *j, int x, int y, const char *t) { rotulo(j, x, y, t, W98_NEGRO); }
static void fondo(jan_t *j) { g_retangulo(j, 0, 0, j->larg, j->alt, W98_FACE); }

static int entero(const char *s)
{
    int v = 0;
    while (*s == ' ') s++;
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return v;
}

/* ------------------------------------------------------------ galería de WordArt */

#define GAL_COLS 5
#define GAL_F    4
#define CEL_L    100
#define CEL_A    50
#define CEL_PASO 104

static void gal_celda(jan_t *j, int x, int y, const imagem_t *im, bool sel)
{
    g_retangulo(j, x, y, CEL_L, CEL_A, COR_CAMPO);
    if (im && im->px) imagem_desenhar(j->px, j->larg, x, y, CEL_L, CEL_A, im, x + 2, y + 2, CEL_L - 4, CEL_A - 4, false);
    g_bisel(j, x, y, CEL_L, CEL_A, true);
    if (sel) {
        g_retangulo(j, x - 2, y - 2, CEL_L + 4, 2, W98_AZUL); g_retangulo(j, x - 2, y + CEL_A, CEL_L + 4, 2, W98_AZUL);
        g_retangulo(j, x - 2, y, 2, CEL_A, W98_AZUL); g_retangulo(j, x + CEL_L, y, 2, CEL_A, W98_AZUL);
    }
}

bool dialogo_wordart(rte_obj_t *o)
{
    static char it_fam[3][64] = { "Sans (Arial)", "Serif (Times)", "Mono (Courier)" };
    bool novo = o->wa_texto[0] == 0;
    jan_t *j = jan_abrir(novo ? "Insertar WordArt" : "Editar WordArt", 556, 520);
    if (!j) return false;
    jan_estilo(j, JAN_DIALOGO, 0, 0);
    imagem_t *th = aloca_zerado(sizeof(imagem_t) * WORDART_ESTILOS);
    if (!th) { jan_fechar(j); return false; }
    int estilo = o->wa_estilo >= 0 && o->wa_estilo < WORDART_ESTILOS ? o->wa_estilo : 3;
    for (int i = 0; i < WORDART_ESTILOS; i++) {
        int fam, est;
        wordart_padrao(i, &fam, &est);
        wordart_renderizar("WordArt", i, fam, est, (CEL_L - 4) * 2, (CEL_A - 4) * 2, &th[i]);
    }
    caixa_t ct, cs;
    memset(&ct, 0, sizeof ct); memset(&cs, 0, sizeof cs);
    ct.x = 14; ct.y = 270; ct.l = 526; ct.foco = true;
    cs.x = 180; cs.y = 322; cs.l = 60;
    caixa_definir(&ct, novo ? "WordArt" : o->wa_texto);
    char t[16];
    int tam0 = o->wa_tam > 0 ? o->wa_tam : 36;
    formatar_texto(t, sizeof t, "%d", tam0);
    caixa_definir(&cs, t);
    int fam0, est0;
    wordart_padrao(estilo, &fam0, &est0);
    lista_t lf = { 14, 322, 150, 64, it_fam, 3, novo ? fam0 : (o->wa_fam < 3 ? o->wa_fam : 0), 0, NULL, NULL };
    casilla_t c_neg = { 180, 362, (novo ? est0 : o->wa_est) & 1 }, c_cur = { 180, 388, ((novo ? est0 : o->wa_est) & 2) != 0 };
    botao_t ok = { 340, 478, 96, 26, "Aceptar", false }, cancel = { 444, 478, 96, 26, "Cancelar", false };
    imagem_t prev;
    memset(&prev, 0, sizeof prev);
    bool prev_sujo = true, aceptar = false, redib = true;
    evento_t e;
    for (;;) {
        if (prev_sujo) {
            imagem_liberar(&prev);
            memset(&prev, 0, sizeof prev);
            int tam = entero(cs.texto);
            if (tam < 8) tam = 8;
            if (tam > 200) tam = 200;
            int est = (c_neg.marcada ? 1 : 0) | (c_cur.marcada ? 2 : 0), nl, na;
            const char *txt = ct.texto[0] ? ct.texto : " ";
            wordart_tamanho_natural(txt, estilo, lf.sel, est, tam, &nl, &na);
            int pl = nl / 15, pa = na / 15;
            if (pl > 200) { pa = pa * 200 / pl; pl = 200; }
            if (pa > 82) { pl = pl * 82 / pa; pa = 82; }
            if (pl < 16) pl = 16;
            if (pa < 16) pa = 16;
            wordart_renderizar(txt, estilo, lf.sel, est, pl * 2, pa * 2, &prev);
            prev_sujo = false;
            redib = true;
        }
        if (redib) {
            fondo(j);
            etiqueta(j, 14, 8, "Elija un estilo de WordArt:");
            for (int i = 0; i < WORDART_ESTILOS; i++)
                gal_celda(j, 14 + (i % GAL_COLS) * CEL_PASO, 28 + (i / GAL_COLS) * 56, &th[i], i == estilo);
            etiqueta(j, 14, 252, "Texto:");
            caixa_desenhar(j, &ct);
            etiqueta(j, 14, 304, "Fuente:");
            g_bisel(j, 12, 320, 154, 68, true); lista_desenhar(j, &lf);
            etiqueta(j, 180, 304, "Tamaño (pt):");
            caixa_desenhar(j, &cs);
            casilla98(j, &c_neg, "Negrita"); casilla98(j, &c_cur, "Cursiva");
            etiqueta(j, 290, 304, "Vista previa:");
            g_retangulo(j, 290, 322, 250, 134, COR_BRANCO);
            g_bisel(j, 288, 320, 254, 138, true);
            if (prev.px) {
                int pw = prev.larg / 2, ph = prev.alt / 2;
                imagem_desenhar(j->px, j->larg, 290, 322, 250, 134, &prev, 290 + (250 - pw) / 2, 322 + (134 - ph) / 2, pw, ph, true);
            }
            boton98(j, &ok); boton98(j, &cancel);
            jan_mostrar(j);
            redib = false;
        }
        if (proximo_evento(j->id, &e, true) != 1) continue;
        if (e.tipo == EV_FECHAR) break;
        if (e.tipo == EV_TECLA && e.tecla == TECLA_ESC) break;
        if (boton98_tratar(j, &ok, &e)) { aceptar = true; break; }
        if (boton98_tratar(j, &cancel, &e)) break;
        if (casilla98_tratar(j, &c_neg, "Negrita", &e) || casilla98_tratar(j, &c_cur, "Cursiva", &e)) { prev_sujo = true; continue; }
        if (e.tipo == EV_CLIQUE && (e.botoes & 1)) {
            for (int i = 0; i < WORDART_ESTILOS; i++)
                if (en(e.x, e.y, 14 + (i % GAL_COLS) * CEL_PASO, 28 + (i / GAL_COLS) * 56, CEL_L, CEL_A)) {
                    estilo = i;
                    int f2, s2;
                    wordart_padrao(i, &f2, &s2);
                    lf.sel = f2; c_neg.marcada = (s2 & 1) != 0; c_cur.marcada = (s2 & 2) != 0;
                    prev_sujo = true;
                }
            if (en(e.x, e.y, ct.x, ct.y, ct.l, 24)) { ct.foco = true; cs.foco = false; redib = true; }
            else if (en(e.x, e.y, cs.x, cs.y, cs.l, 24)) { cs.foco = true; ct.foco = false; redib = true; }
            if (en(e.x, e.y, lf.x, lf.y, lf.l, lf.a)) { lista_tratar(j, &lf, &e); prev_sujo = true; }
            if (prev_sujo || redib) continue;
        }
        if (e.tipo == EV_TECLA && e.tecla == '\t') { ct.foco = !ct.foco; cs.foco = !ct.foco; redib = true; continue; }
        if (e.tipo == EV_TECLA && (e.tecla == '\n' || e.tecla == '\r')) { aceptar = true; break; }
        if (e.tipo == EV_TECLA) {
            caixa_t *c = ct.foco ? &ct : &cs;
            char antes[128];
            copia_texto(antes, c->texto, sizeof antes);
            caixa_tratar(j, c, &e);
            if (compara_texto(antes, c->texto) != 0) prev_sujo = true;
        } else if (e.tipo == EV_RODA) {
            lista_tratar(j, &lf, &e);
        }
    }
    if (aceptar) {
        int est = (c_neg.marcada ? 1 : 0) | (c_cur.marcada ? 2 : 0);
        int tam = entero(cs.texto);
        if (tam < 8) tam = 8;
        if (tam > 200) tam = 200;
        const char *txt = ct.texto[0] ? ct.texto : "WordArt";
        bool cambio_tam = novo || tam != tam0;
        o->tipo = RTE_OBJ_WORDART;
        o->wa_estilo = estilo; o->wa_fam = lf.sel; o->wa_est = est; o->wa_tam = tam;
        copia_texto(o->wa_texto, txt, sizeof o->wa_texto);
        if (cambio_tam) wordart_tamanho_natural(o->wa_texto, estilo, lf.sel, est, tam, &o->l_tw, &o->a_tw);
    }
    for (int i = 0; i < WORDART_ESTILOS; i++) imagem_liberar(&th[i]);
    libera(th);
    imagem_liberar(&prev);
    jan_fechar(j);
    return aceptar;
}

/* ------------------------------------------------------------ Formato de imagen */

static void pon_cm(caixa_t *c, int tw) { char t[24]; caixa_definir(c, cm_texto_de_twips(t, sizeof t, tw)); }
static void pon_pct(caixa_t *c, int tw, int nat) { char t[16]; formatar_texto(t, sizeof t, "%d", nat > 0 ? (int)((long)tw * 100 / nat) : 100); caixa_definir(c, t); }

bool dialogo_formato_imagem(rte_obj_t *o, int *alinhamento)
{
    static char it_aj[2][64] = { "En línea con el texto", "Arriba y abajo" };
    static char it_al[3][64] = { "Izquierda", "Centro", "Derecha" };
    static char it_bo[4][64] = { "Sin borde", "Fino (0,75 pt)", "Medio (1,5 pt)", "Grueso (3 pt)" };
    jan_t *j = jan_abrir("Formato de imagen", 470, 420);
    if (!j) return false;
    jan_estilo(j, JAN_DIALOGO, 0, 0);
    int nat_l = o->l_tw, nat_a = o->a_tw;
    const imagem_t *im = rte_obj_imagem(o);
    int bl = 0, ba = 0;
    rte_obj_blob(o, NULL, NULL, &bl, &ba);
    if (bl > 0 && ba > 0) { nat_l = bl * 15; nat_a = ba * 15; }
    else if (im) { nat_l = im->larg * 15; nat_a = im->alt * 15; }
    if (o->tipo == RTE_OBJ_WORDART) { nat_l = o->l_tw; nat_a = o->a_tw; }
    int l_tw = o->l_tw, a_tw = o->a_tw;
    caixa_t cx[4], cp;
    memset(cx, 0, sizeof cx); memset(&cp, 0, sizeof cp);
    for (int i = 0; i < 4; i++) { cx[i].x = (i % 2 ? 330 : 100); cx[i].y = 30 + (i / 2) * 32; cx[i].l = 80; }
    cp.x = 100; cp.y = 322; cp.l = 340;
    pon_cm(&cx[0], l_tw); pon_cm(&cx[1], a_tw);
    pon_pct(&cx[2], l_tw, nat_l); pon_pct(&cx[3], a_tw, nat_a);
    caixa_definir(&cp, o->pie);
    cx[0].foco = true;
    casilla_t c_prop = { 14, 100, true };
    lista_t laj = { 14, 148, 190, 44, it_aj, 2, o->ajuste == RTE_AJ_CIMA_BAIXO ? 1 : 0, 0, NULL, NULL };
    lista_t lal = { 224, 148, 100, 64, it_al, 3, alinhamento && *alinhamento >= 0 && *alinhamento < 3 ? *alinhamento : 0, 0, NULL, NULL };
    lista_t lbo = { 340, 148, 116, 84, it_bo, 4, o->borde < 4 ? o->borde : 0, 0, NULL, NULL };
    botao_t ok = { 250, 380, 96, 26, "Aceptar", false }, cancel = { 356, 380, 96, 26, "Cancelar", false };
    int sucio = -1;       /* último cuadro de tamaño editado */
    bool aceptar = false, redib = true;
    evento_t e;
    for (;;) {
        if (redib) {
            fondo(j);
            etiqueta(j, 14, 8, "Tamaño");
            g_retangulo(j, 60, 15, 396, 1, W98_SOMBRA);
            etiqueta(j, 14, 34, "Ancho (cm):"); etiqueta(j, 214, 34, "Alto (cm):");
            etiqueta(j, 14, 66, "Ancho (%):"); etiqueta(j, 214, 66, "Alto (%):");
            for (int i = 0; i < 4; i++) caixa_desenhar(j, &cx[i]);
            casilla98(j, &c_prop, "Mantener la proporción");
            etiqueta(j, 14, 128, "Ajuste del texto:"); etiqueta(j, 224, 128, "Alineación:"); etiqueta(j, 340, 128, "Borde:");
            g_bisel(j, 12, 146, 194, 48, true); lista_desenhar(j, &laj);
            g_bisel(j, 222, 146, 104, 68, true); lista_desenhar(j, &lal);
            g_bisel(j, 338, 146, 120, 88, true); lista_desenhar(j, &lbo);
            etiqueta(j, 14, 204, "Ajuste «cuadrado»: no disponible.");
            etiqueta(j, 14, 300, "Pie de imagen:");
            caixa_desenhar(j, &cp);
            boton98(j, &ok); boton98(j, &cancel);
            jan_mostrar(j);
            redib = false;
        }
        if (proximo_evento(j->id, &e, true) != 1) continue;
        if (e.tipo == EV_FECHAR) break;
        if (e.tipo == EV_TECLA && e.tecla == TECLA_ESC) break;
        if (boton98_tratar(j, &ok, &e)) { aceptar = true; break; }
        if (boton98_tratar(j, &cancel, &e)) break;
        if (casilla98_tratar(j, &c_prop, "Mantener la proporción", &e)) continue;
        /* aplica el cuadro editado cuando cambia el foco */
        int foco_ant = -1;
        for (int i = 0; i < 4; i++) if (cx[i].foco) foco_ant = i;
        if (cp.foco) foco_ant = 4;
        if (e.tipo == EV_CLIQUE && (e.botoes & 1)) {
            int nuevo = -1;
            for (int i = 0; i < 4; i++) if (en(e.x, e.y, cx[i].x, cx[i].y, cx[i].l, 24)) nuevo = i;
            if (en(e.x, e.y, cp.x, cp.y, cp.l, 24)) nuevo = 4;
            if (nuevo >= 0 && nuevo != foco_ant) {
                for (int i = 0; i < 4; i++) cx[i].foco = i == nuevo;
                cp.foco = nuevo == 4;
                goto recalcular;
            }
            if (en(e.x, e.y, laj.x, laj.y, laj.l, laj.a)) lista_tratar(j, &laj, &e);
            else if (en(e.x, e.y, lal.x, lal.y, lal.l, lal.a)) lista_tratar(j, &lal, &e);
            else if (en(e.x, e.y, lbo.x, lbo.y, lbo.l, lbo.a)) lista_tratar(j, &lbo, &e);
            redib = true;
            continue;
        }
        if (e.tipo == EV_TECLA && e.tecla == '\t') {
            int sig = (foco_ant + 1) % 5;
            for (int i = 0; i < 4; i++) cx[i].foco = i == sig;
            cp.foco = sig == 4;
            goto recalcular;
        }
        if (e.tipo == EV_TECLA && (e.tecla == '\n' || e.tecla == '\r')) { aceptar = true; break; }
        if (e.tipo == EV_TECLA) {
            caixa_t *c = foco_ant == 4 ? &cp : &cx[foco_ant < 0 ? 0 : foco_ant];
            char antes[128];
            copia_texto(antes, c->texto, sizeof antes);
            caixa_tratar(j, c, &e);
            if (compara_texto(antes, c->texto) != 0 && foco_ant >= 0 && foco_ant < 4) sucio = foco_ant;
        }
        continue;
recalcular:
        if (sucio >= 0) {
            if (sucio == 0 || sucio == 2) {
                l_tw = sucio == 0 ? twips_de_cm_texto(cx[0].texto) : (int)((long)nat_l * entero(cx[2].texto) / 100);
                if (c_prop.marcada && nat_l > 0) a_tw = (int)((long)l_tw * nat_a / nat_l);
            } else {
                a_tw = sucio == 1 ? twips_de_cm_texto(cx[1].texto) : (int)((long)nat_a * entero(cx[3].texto) / 100);
                if (c_prop.marcada && nat_a > 0) l_tw = (int)((long)a_tw * nat_l / nat_a);
            }
            if (l_tw < 150) l_tw = 150;
            if (a_tw < 150) a_tw = 150;
            pon_cm(&cx[0], l_tw); pon_cm(&cx[1], a_tw);
            pon_pct(&cx[2], l_tw, nat_l); pon_pct(&cx[3], a_tw, nat_a);
            sucio = -1;
        }
        redib = true;
    }
    if (aceptar) {
        if (sucio >= 0) {          /* un cuadro editado y no confirmado */
            if (sucio == 0 || sucio == 2) {
                l_tw = sucio == 0 ? twips_de_cm_texto(cx[0].texto) : (int)((long)nat_l * entero(cx[2].texto) / 100);
                if (c_prop.marcada && nat_l > 0) a_tw = (int)((long)l_tw * nat_a / nat_l);
            } else {
                a_tw = sucio == 1 ? twips_de_cm_texto(cx[1].texto) : (int)((long)nat_a * entero(cx[3].texto) / 100);
                if (c_prop.marcada && nat_a > 0) l_tw = (int)((long)a_tw * nat_l / nat_a);
            }
        }
        if (l_tw < 150) l_tw = 150;
        if (a_tw < 150) a_tw = 150;
        o->l_tw = l_tw; o->a_tw = a_tw;
        o->ajuste = laj.sel == 1 ? RTE_AJ_CIMA_BAIXO : RTE_AJ_LINHA;
        o->borde = lbo.sel < 0 ? 0 : lbo.sel;
        copia_texto(o->pie, cp.texto, sizeof o->pie);
        if (alinhamento) *alinhamento = lal.sel < 0 ? 0 : lal.sel;
    }
    jan_fechar(j);
    return aceptar;
}
