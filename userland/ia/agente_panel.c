// SPDX-License-Identifier: GPL-2.0-or-later
/* agente_panel.c - panel gráfico reutilizable de agentes de IA, y sus diálogos (D295).
 *
 * Es un componente que se incrusta en cualquier ventana (Chat IA, Mico, y las aplicaciones que
 * registren herramientas con ag_herr_registrar): la conversación con el agente, los pasos (cada uno
 * con su resultado, que se despliega al elegirlo), el selector de agente y el diálogo de confirmación.
 * Aspecto Windows 98 (gris y biseles), como el correo (D289) y Pluma (D291). Vive en userland/ia/
 * y no en libsagui porque depende del cliente de IA (que trae TLS); solo lo enlazan los programas
 * con ventana que lo usan (IA_GUI_OBJ en el Makefile).
 *
 * El texto del programa va en Latin-1 (como el resto de la interfaz); el del modelo llega en UTF-8. */
#include "agentes.h"
#include "gui.h"

static int fmt(char *d, int max, const char *f, ...)
{
    if (max <= 0) return 0;
    va_list ap;
    va_start(ap, f);
    int n = formatar_lista(d, (size_t)max, f, ap);
    va_end(ap);
    return n < max ? n : max - 1;
}

static bool en(int x, int y, int rx, int ry, int rl, int ra) { return x >= rx && x < rx + rl && y >= ry && y < ry + ra; }

/* Parte un texto en líneas de «cols» columnas y lo dibuja; devuelve el alto usado. */
static int texto_ajustado(jan_t *j, int x, int y, const char *t, int cols, uint32_t cor, int max_lineas)
{
    int lineas = 0;
    while (*t && lineas < max_lineas) {
        int n = 0;
        while (t[n] && t[n] != '\n' && n < cols) n++;
        int salto = 0;
        if (t[n] && t[n] != '\n') {                 /* cortar en el último espacio */
            int k = n;
            while (k > 0 && t[k] != ' ') k--;
            if (k > 0) { n = k; salto = 1; }
        } else if (t[n] == '\n') salto = 1;
        g_texto_n(j, x, y + lineas * (LETRA_A + 2), t, n, cor);
        lineas++;
        t += n + salto;
    }
    return lineas * (LETRA_A + 2);
}

/* ------------------------------------------------------------------ diálogo de confirmación */

#define COR_ROJO 0xB00020

int ag_dialogo_confirmar(const char *herr, const char *desc, const char *aviso, bool puede_siempre)
{
    const int L = 620, A = 470;      /* revisión de seguridad: sitio para hasta 7 líneas de avisos (antes 3, y se cortaban) */
    jan_t *j = jan_abrir("El agente pide permiso", L, A);
    if (!j) return AG_NEGAR;
    jan_estilo(j, JAN_DIALOGO, 0, 0);
    area_t ar;
    area_iniciar(&ar, 16, 56, L - 32, 200);
    area_definir(&ar, desc, (int)tam_texto(desc));
    ar.somente_leitura = true;
    ar.cursor = 0;
    botao_t b_una = { 16, A - 44, 170, 28, "Permitir una vez", false };
    botao_t b_sie = { 194, A - 44, 250, 28, "Siempre en esta sesion", false };
    botao_t b_den = { L - 16 - 120, A - 44, 120, 28, "Denegar", false };
    int resposta = -1;
    /* Revisión de seguridad: cuántas líneas trae (si no caben, se dice) y una pausa antes de aceptar
     * clics: el diálogo sale cuando el modelo quiere y un clic que iba a otra cosa no debe permitir nada. */
    int lineas = 1;
    for (const char *q = desc; *q; q++) if (*q == '\n') lineas++;
    unsigned abierto = 0;
    /* Dibujo */
    for (int paso = 0; ; paso++) {
        g_retangulo(j, 0, 0, L, A, W98_FACE);
        char t[160];
        fmt(t, sizeof t, "El agente quiere usar la herramienta: %s", herr);
        g_texto(j, 16, 12, t, W98_NEGRO);
        g_texto(j, 16, 34, "Esto es exactamente lo que se hara (orden o cambios):", W98_NEGRO);
        area_desenhar(j, &ar);
        if (lineas > area_linhas_visiveis(&ar)) {
            char t2[120];
            fmt(t2, sizeof t2, "%d lineas: usa la rueda o las flechas para verlas todas antes de decidir", lineas);
            g_texto(j, 16, 56 + 200 + 2, t2, COR_ROJO);
        }
        int y = 56 + 200 + 8 + (lineas > area_linhas_visiveis(&ar) ? LETRA_A + 2 : 0);
        if (aviso && aviso[0]) y += texto_ajustado(j, 16, y, aviso, (L - 32) / LETRA_L, COR_ROJO, 7);
        else g_texto(j, 16, y, "Solo se permite esta accion. Denegar no cambia nada.", W98_SOMBRA);
        boton98(j, &b_una);
        if (puede_siempre) boton98(j, &b_sie);
        else { g_texto(j, b_sie.x + 4, b_sie.y + 6, "(\"siempre\" no disponible aqui)", W98_SOMBRA); }
        boton98(j, &b_den);
        jan_mostrar(j);
        abierto = tempo_ms();
        if (paso == 0) {
            /* Para las pruebas y la accesibilidad: dónde están los botones, en el registro del sistema. */
            registrar("agente: dialogo %s botones permitir=%d,%d siempre=%d,%d denegar=%d,%d\n", herr,
                      b_una.x + b_una.l / 2, b_una.y + b_una.a / 2, puede_siempre ? b_sie.x + b_sie.l / 2 : -1,
                      b_sie.y + b_sie.a / 2, b_den.x + b_den.l / 2, b_den.y + b_den.a / 2);
        }
        evento_t e;
        for (;;) {
            if (proximo_evento(j->id, &e, true) != 1) continue;
            if (e.tipo == EV_FECHAR) { resposta = AG_NEGAR; break; }
            if (e.tipo == EV_TECLA && e.tecla == TECLA_ESC) { resposta = AG_NEGAR; break; }
            if (e.tipo == EV_CLIQUE && (e.botoes & 1) && tempo_ms() - abierto >= 400) {
                if (en(e.x, e.y, b_una.x, b_una.y, b_una.l, b_una.a)) { resposta = AG_UNA_VEZ; break; }
                if (puede_siempre && en(e.x, e.y, b_sie.x, b_sie.y, b_sie.l, b_sie.a)) { resposta = AG_SIEMPRE; break; }
                if (en(e.x, e.y, b_den.x, b_den.y, b_den.l, b_den.a)) { resposta = AG_NEGAR; break; }
            }
            /* Enter NO permite (la opcion segura es la de por omision). */
            if (area_tratar(j, &ar, &e)) { /* solo lectura */ }
            area_desenhar(j, &ar);
            jan_mostrar_area(j, ar.x, ar.y, ar.l, ar.a);
        }
        break;
    }
    jan_fechar(j);
    registrar("agente: confirmacion %s -> %s\n", herr, resposta == AG_NEGAR ? "denegar" : resposta == AG_SIEMPRE ? "siempre" : "una vez");
    return resposta;
}

/* ------------------------------------------------------------------ registro y editor */

static void dialogo_texto_solo_lectura(const char *titulo, const char *texto, int n)
{
    const int L = 700, A = 460;
    jan_t *j = jan_abrir(titulo, L, A);
    if (!j) return;
    jan_estilo(j, JAN_DIALOGO, 0, 0);
    area_t ar;
    area_iniciar(&ar, 12, 12, L - 24, A - 64);
    area_definir(&ar, texto, n);
    ar.somente_leitura = true;
    botao_t cerrar = { L - 12 - 100, A - 40, 100, 28, "Cerrar", false };
    evento_t e;
    for (;;) {
        g_retangulo(j, 0, 0, L, A, W98_FACE);
        area_desenhar(j, &ar);
        boton98(j, &cerrar);
        jan_mostrar(j);
        for (;;) {
            if (proximo_evento(j->id, &e, true) != 1) continue;
            if (e.tipo == EV_FECHAR || (e.tipo == EV_TECLA && e.tecla == TECLA_ESC)) goto fin;
            if (e.tipo == EV_CLIQUE && en(e.x, e.y, cerrar.x, cerrar.y, cerrar.l, cerrar.a)) goto fin;
            if (area_tratar(j, &ar, &e)) { /* solo lectura */ }
            area_desenhar(j, &ar);
            jan_mostrar_area(j, ar.x, ar.y, ar.l, ar.a);
        }
    }
fin:
    jan_fechar(j);
}

static void ver_registro_dialogo(void)
{
    enum { TOPE = 12000 };
    char *t = aloca(TOPE + 1);
    if (!t) return;
    int n = 0;
    int fd = abrir(ag_caminho_registro(), ABRIR_LER);
    if (fd >= 0) {
        info_arquivo_t inf;
        if (info_arquivo(ag_caminho_registro(), &inf) == 0 && inf.tamanho > TOPE) posicionar(fd, (int)inf.tamanho - TOPE, 0);
        int k;
        while (n < TOPE && (k = ler(fd, t + n, (unsigned)(TOPE - n))) > 0) n += k;
        fechar(fd);
    }
    t[n] = 0;
    if (!n) n = fmt(t, TOPE, "(el registro esta vacio)");
    dialogo_texto_solo_lectura("Registro de agentes", t, n);
    libera(t);
}

/* Editor de un agente: devuelve true si se guardó. */
static bool editor_agente(ag_def_t *d, bool nuevo)
{
    const int L = 640, A = 520;
    jan_t *j = jan_abrir(nuevo ? "Nuevo agente" : "Editar agente", L, A);
    if (!j) return false;
    jan_estilo(j, JAN_DIALOGO, 0, 0);
    caixa_t c_nome, c_desc, c_mod, c_pasos, c_herr;
    memset(&c_nome, 0, sizeof c_nome); c_desc = c_mod = c_pasos = c_herr = c_nome;
    caixa_definir(&c_nome, d->nome); caixa_definir(&c_desc, d->descricao); caixa_definir(&c_mod, d->modelo);
    char tp[8];
    fmt(tp, sizeof tp, "%d", d->passos);
    caixa_definir(&c_pasos, tp); caixa_definir(&c_herr, d->herramientas);
    caixa_t *cs[5] = { &c_nome, &c_desc, &c_mod, &c_pasos, &c_herr };
    const char *rot[5] = { "Nombre", "Descripcion", "Modelo (vacio = el de Chat IA)", "Limite de pasos", "Herramientas (separadas por coma; vacio = todas)" };
    int y = 12;
    for (int i = 0; i < 5; i++) { cs[i]->x = 16; cs[i]->y = y + 18; cs[i]->l = L - 32; y += 56; }
    area_t ar;
    area_iniciar(&ar, 16, y + 18, L - 32, A - y - 18 - 56);
    area_definir(&ar, d->sistema, (int)tam_texto(d->sistema));
    botao_t b_ok = { L - 16 - 100, A - 44, 100, 28, "Guardar", false }, b_no = { L - 16 - 210, A - 44, 100, 28, "Cancelar", false };
    int foco = 0;
    bool guardado = false;
    evento_t e;
    for (;;) {
        g_retangulo(j, 0, 0, L, A, W98_FACE);
        int yy = 12;
        for (int i = 0; i < 5; i++) { g_texto(j, 16, yy, rot[i], W98_NEGRO); cs[i]->foco = foco == i; caixa_desenhar(j, cs[i]); yy += 56; }
        g_texto(j, 16, yy, "Instrucciones del sistema", W98_NEGRO);
        ar.foco = foco == 5;
        area_desenhar(j, &ar);
        boton98(j, &b_ok); boton98(j, &b_no);
        jan_mostrar(j);
        if (proximo_evento(j->id, &e, true) != 1) continue;
        if (e.tipo == EV_FECHAR || (e.tipo == EV_TECLA && e.tecla == TECLA_ESC)) break;
        if (e.tipo == EV_CLIQUE && (e.botoes & 1)) {
            if (en(e.x, e.y, b_no.x, b_no.y, b_no.l, b_no.a)) break;
            if (en(e.x, e.y, b_ok.x, b_ok.y, b_ok.l, b_ok.a)) {
                copia_texto(d->nome, c_nome.texto, sizeof d->nome);
                copia_texto(d->descricao, c_desc.texto, sizeof d->descricao);
                copia_texto(d->modelo, c_mod.texto, sizeof d->modelo);
                copia_texto(d->herramientas, c_herr.texto, sizeof d->herramientas);
                d->passos = texto_para_int(c_pasos.texto);
                if (d->passos < 1) d->passos = AG_PASSOS_PADRAO;
                if (d->passos > AG_PASSOS_MAX) d->passos = AG_PASSOS_MAX;
                copia_texto(d->sistema, ar.buf ? ar.buf : "", sizeof d->sistema);
                if (!d->nome[0]) { mensagem("Agente", "El agente necesita un nombre."); continue; }
                char c[100], slug[40];
                int k = 0;
                for (const char *p = d->nome; *p && k < 30; p++) slug[k++] = (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') ? *p : (*p >= 'A' && *p <= 'Z') ? (char)(*p + 32) : '-';
                slug[k] = 0;
                fmt(c, sizeof c, "%s/%s.agente", ag_pasta(), slug);
                if (ag_def_gravar(c, d) < 0) { mensagem("Agente", "No se pudo guardar el agente."); continue; }
                ag_registrar("-", "agente %s guardado en %s", d->nome, c);
                guardado = true;
                break;
            }
            for (int i = 0; i < 5; i++) if (en(e.x, e.y, cs[i]->x, cs[i]->y, cs[i]->l, 24)) { foco = i; caixa_tratar(j, cs[i], &e); }
            if (en(e.x, e.y, ar.x, ar.y, ar.l, ar.a)) { foco = 5; area_tratar(j, &ar, &e); }
            continue;
        }
        if (e.tipo == EV_TECLA && e.tecla == '\t') { foco = (foco + 1) % 6; continue; }
        if (foco < 5) caixa_tratar(j, cs[foco], &e); else area_tratar(j, &ar, &e);
    }
    jan_fechar(j);
    return guardado;
}

/* ------------------------------------------------------------------ el panel */

#define PASOS_MAX 48
struct ag_panel {
    jan_t *j;
    int x, y, l, a;
    void *app;
    rico_t *visor;
    area_t entrada, detalle;
    lista_t lista;
    char itens[PASOS_MAX][64];
    char *res[PASOS_MAX];            /* resultado completo de cada paso (memoria dinámica) */
    char herr[PASOS_MAX][32];
    char args[PASOS_MAX][200];
    int estado_p[PASOS_MAX];
    int n_pasos;
    bool detalle_visible;
    botao_t bt_enviar, bt_deter, bt_agente, bt_nuevo, bt_editar, bt_registro, bt_ocultar;
    ag_def_t *defs;
    int ndefs, sel;
    ag_hist_t hist;
    ag_sesion_t ses;
    ia_config_t cfg;
    bool ocupada, parar, cerrar;
    char estado[200];
    bool estado_erro;
    int sec_texto;                   /* sección del asistente que está llegando (-1 = ninguna) */
    char cola[4];
    int n_cola;
    bool (*filtro)(const ag_def_t *d);   /* agentes que el panel ofrece (NULL = todos) */
};

/* Opciones de la línea de órdenes de las aplicaciones (--url, --modelo): ag_app_opciones() las guarda
 * aquí y todo panel que se cree después las aplica sobre la configuración de la IA. */
static char g_ovr_url[IA_URL_MAX], g_ovr_modelo[IA_MODELO_MAX];

void ag_app_opciones(int *argc, char **argv)
{
    int w = 1;
    for (int i = 1; i < *argc; i++) {
        if ((compara_texto(argv[i], "--url") == 0 || compara_texto(argv[i], "--modelo") == 0) && i + 1 < *argc) {
            if (compara_texto(argv[i], "--url") == 0) copia_texto(g_ovr_url, argv[i + 1], sizeof g_ovr_url);
            else copia_texto(g_ovr_modelo, argv[i + 1], sizeof g_ovr_modelo);
            i++;
            continue;
        }
        argv[w++] = argv[i];
    }
    *argc = w;
    argv[w] = NULL;
}

static void recargar_agentes(ag_panel_t *p)
{
    char actual[AG_NOME_MAX] = "";
    if (p->defs && p->sel >= 0 && p->sel < p->ndefs) copia_texto(actual, p->defs[p->sel].nome, sizeof actual);
    if (!p->defs) p->defs = aloca(sizeof *p->defs * 32);
    p->ndefs = p->defs ? ag_def_listar(p->defs, 32) : 0;
    if (p->filtro) {                                 /* la aplicación limita qué agentes se ofrecen */
        int w = 0;
        for (int i = 0; i < p->ndefs; i++) if (p->filtro(&p->defs[i])) p->defs[w++] = p->defs[i];
        p->ndefs = w;
    }
    p->sel = 0;
    for (int i = 0; i < p->ndefs; i++) if (compara_texto(p->defs[i].nome, actual) == 0) p->sel = i;
}

static int altura_pasos(const ag_panel_t *p) { return 74 + (p->detalle_visible ? 86 : 0); }
static int altura_entrada(void) { return 50; }

static void maquetar(ag_panel_t *p)
{
    int x = p->x, y = p->y, l = p->l, a = p->a;
    int la = l - 250 - 4;                    /* el selector no pisa los botones de la derecha */
    if (la > 262) la = 262;
    if (la < 60) la = 60;
    p->bt_agente = (botao_t){ x, y, la, 24, "Agente", false };
    p->bt_nuevo = (botao_t){ x + l - 250, y, 80, 24, "Nuevo", false };
    p->bt_editar = (botao_t){ x + l - 166, y, 80, 24, "Editar", false };
    p->bt_registro = (botao_t){ x + l - 82, y, 82, 24, "Registro", false };
    int ent_y = y + a - altura_entrada() - 20;
    int pasos_y = ent_y - altura_pasos(p) - 4;
    rico_mover(p->visor, x, y + 30, l, pasos_y - (y + 30) - 4);
    p->lista.x = x; p->lista.y = pasos_y; p->lista.l = l; p->lista.a = 74;
    p->lista.itens = p->itens;
    if (p->detalle_visible) {
        area_mudar_tamanho(&p->detalle, l, 66);
        p->detalle.x = x; p->detalle.y = pasos_y + 78;
        p->bt_ocultar = (botao_t){ x + l - 80, pasos_y + 78 + 66 + 2, 80, 18, "Plegar", false };
    }
    p->entrada.x = x; p->entrada.y = ent_y;
    area_mudar_tamanho(&p->entrada, l - 214, altura_entrada());
    p->bt_enviar = (botao_t){ x + l - 204, ent_y, 100, 28, "Enviar", false };
    p->bt_deter = (botao_t){ x + l - 98, ent_y, 98, 28, "Detener", false };
}

ag_panel_t *ag_panel_criar(void *jv, int x, int y, int l, int a, void *app)
{
    ag_panel_t *p = aloca_zerado(sizeof *p);
    if (!p) return NULL;
    p->j = jv;
    p->x = x; p->y = y; p->l = l; p->a = a;
    p->app = app;
    p->sec_texto = -1;
    p->visor = rico_criar(x, y + 30, l, 100);
    if (!p->visor || !area_iniciar(&p->entrada, x, y, l, 40) || !area_iniciar(&p->detalle, x, y, l, 60)) { ag_panel_liberar(p); return NULL; }
    p->detalle.somente_leitura = true;
    p->lista.sel = -1;
    ia_config_carregar(&p->cfg);
    ia_config_escolher(&p->cfg, IA_USO_CHATIA);
    if (g_ovr_url[0]) copia_texto(p->cfg.url, g_ovr_url, sizeof p->cfg.url);
    if (g_ovr_modelo[0]) copia_texto(p->cfg.modelo, g_ovr_modelo, sizeof p->cfg.modelo);
    recargar_agentes(p);
    maquetar(p);
    char motivo[100];
    if (!ag_permitido(motivo, sizeof motivo)) fmt(p->estado, sizeof p->estado, "No disponible: %s", motivo), p->estado_erro = true;
    else fmt(p->estado, sizeof p->estado, "Elige un agente y escribe. Intro: enviar");
    return p;
}

void ag_panel_liberar(ag_panel_t *p)
{
    if (!p) return;
    for (int i = 0; i < p->n_pasos; i++) if (p->res[i]) libera(p->res[i]);
    ag_hist_liberar(&p->hist);
    if (p->visor) rico_liberar(p->visor);
    if (p->defs) libera(p->defs);
    libera(p);
}

void ag_panel_mudar_tamanho(ag_panel_t *p, int x, int y, int l, int a)
{
    p->x = x; p->y = y; p->l = l; p->a = a;
    maquetar(p);
}

void ag_panel_config(ag_panel_t *p, const ia_config_t *cfg) { p->cfg = *cfg; }

void ag_panel_escolher(ag_panel_t *p, const char *nome)
{
    for (int i = 0; i < p->ndefs; i++) if (compara_texto(p->defs[i].nome, nome) == 0) p->sel = i;
}

void ag_panel_desenhar(ag_panel_t *p)
{
    jan_t *j = p->j;
    g_retangulo(j, p->x, p->y, p->l, p->a, W98_FACE);
    char t[240];
    /* en un panel estrecho el selector solo muestra el nombre, para que el texto no se corte */
    const char *nom = p->ndefs ? p->defs[p->sel].nome : "(ninguno)";
    if (p->bt_agente.l < 190) copia_texto(t, nom, sizeof t); else fmt(t, sizeof t, "Agente: %s", nom);
    p->bt_agente.texto = t;
    boton98(j, &p->bt_agente);
    p->bt_agente.texto = "Agente";
    if (p->ndefs) {
        int ancho = p->l - 250 - 270 - 8;   /* solo si cabe junto al selector */
        if (ancho > 40) {
            int cols = ancho / LETRA_L;
            char d[160];
            copia_texto(d, p->defs[p->sel].descricao, (size_t)cols < sizeof d ? (size_t)cols : sizeof d);
            g_texto(j, p->x + 270, p->y + 4, d, W98_OSCURO);
        }
    }
    boton98(j, &p->bt_nuevo); boton98(j, &p->bt_editar); boton98(j, &p->bt_registro);
    rico_desenhar(j, p->visor);
    p->lista.qtd = p->n_pasos;
    g_texto(j, p->lista.x, p->lista.y - 16, "", W98_NEGRO);
    lista_en_marco(j, &p->lista);
    if (!p->n_pasos) g_texto_n(j, p->lista.x + 6, p->lista.y + 6, "(los pasos del agente apareceran aqui)", (p->lista.l - 12) / LETRA_L > 0 ? (p->lista.l - 12) / LETRA_L : 0, W98_SOMBRA);
    if (p->detalle_visible) {
        area_desenhar(j, &p->detalle);
        boton98(j, &p->bt_ocultar);
    }
    p->entrada.foco = !p->ocupada;
    area_desenhar(j, &p->entrada);
    boton98(j, &p->bt_enviar);
    boton98(j, &p->bt_deter);
    uint32_t cor = p->estado_erro ? COR_ROJO : p->ocupada ? W98_AZUL : W98_OSCURO;
    g_texto_n(j, p->x, p->y + p->a - 18, p->estado, p->l / LETRA_L < (int)tam_texto(p->estado) ? p->l / LETRA_L : (int)tam_texto(p->estado), cor);
    jan_mostrar_area(j, p->x, p->y, p->l, p->a);
}

static void estado_p(ag_panel_t *p, bool erro, const char *t) { p->estado_erro = erro; copia_texto(p->estado, t, sizeof p->estado); }

static void mostrar_detalle(ag_panel_t *p, int i)
{
    if (i < 0 || i >= p->n_pasos) return;
    int cap = 400 + (p->res[i] ? (int)tam_texto(p->res[i]) : 0);
    char *t = aloca((size_t)cap);
    if (!t) return;
    int k = fmt(t, cap, "Herramienta: %s\nArgumentos: %s\nResultado:\n%s", p->herr[i], p->args[i], p->res[i] ? p->res[i] : "(sin resultado todavia)");
    area_definir(&p->detalle, t, k);
    p->detalle.cursor = 0;
    libera(t);
    if (!p->detalle_visible) { p->detalle_visible = true; maquetar(p); }
}

/* ---- callbacks del bucle ---- */

static void cb_texto(const char *t, int n, void *ctx)
{
    ag_panel_t *p = ctx;
    char tmp[1100], buf[1100];
    while (n > 0) {
        int parte = n > 512 ? 512 : n;
        int k = p->n_cola;
        memcpy(tmp, p->cola, (size_t)k);
        memcpy(tmp + k, t, (size_t)parte);
        k += parte; t += parte; n -= parte;
        int corte = k;
        for (int b = 1; b <= 3 && b <= k; b++) {
            unsigned char c = (unsigned char)tmp[k - b];
            if ((c & 0xC0) == 0x80) continue;
            int largo = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
            if (largo > b) corte = k - b;
            break;
        }
        p->n_cola = k - corte;
        memcpy(p->cola, tmp + corte, (size_t)p->n_cola);
        int m = ia_utf8_latin1(tmp, corte, buf, sizeof buf);
        if (m <= 0) continue;
        bool seguia = rico_no_fim(p->visor);
        if (p->sec_texto < 0) { p->sec_texto = rico_secao(p->visor, "Agente", W98_AZUL, true); }
        rico_juntar(p->visor, buf, m);
        if (seguia) rico_ir_fim(p->visor);
        rico_desenhar(p->j, p->visor);
        jan_mostrar_area(p->j, p->visor ? p->x : 0, p->y + 30, p->l, p->a - 30);
    }
}

static void cb_passo(int num, const char *herr, const char *args, void *ctx)
{
    ag_panel_t *p = ctx;
    p->sec_texto = -1;
    if (p->n_pasos >= PASOS_MAX) return;
    int i = p->n_pasos++;
    copia_texto(p->herr[i], herr, sizeof p->herr[i]);
    copia_texto(p->args[i], args, sizeof p->args[i]);
    p->res[i] = NULL;
    p->estado_p[i] = -1;
    char t[96];
    fmt(t, sizeof t, "%d. %s %s", num, herr, args);
    copia_texto(p->itens[i], t, 64);
    p->lista.sel = i;
    if (p->lista.qtd = p->n_pasos, p->n_pasos > 4) p->lista.topo = p->n_pasos - 4;
    char r[320];
    fmt(r, sizeof r, "Paso %d: %s", num, herr);
    rico_secao(p->visor, r, W98_OSCURO, false);
    rico_juntar(p->visor, args, (int)tam_texto(args));
    rico_ir_fim(p->visor);
    estado_p(p, false, "Trabajando... Esc: detener");
    ag_panel_desenhar(p);
}

static void cb_resultado(int num, const char *herr, const char *res, int estado, void *ctx)
{
    ag_panel_t *p = ctx;
    (void)herr;
    if (p->n_pasos == 0 || p->n_pasos > PASOS_MAX) return;
    int i = p->n_pasos - 1;
    if (p->n_pasos >= PASOS_MAX && p->res[i]) return;
    int n = (int)tam_texto(res);
    if (n > 3000) n = 3000;
    p->res[i] = aloca((size_t)n + 1);
    if (p->res[i]) { memcpy(p->res[i], res, (size_t)n); p->res[i][n] = 0; }
    p->estado_p[i] = estado;
    char t[96];
    fmt(t, sizeof t, "%d. %s -> %s", num, p->herr[i], estado == 0 ? "ok" : estado == 2 ? "DENEGADO" : "error");
    copia_texto(p->itens[i], t, 64);
    char r[40];
    int k = fmt(r, sizeof r, "-> %s (%d bytes)", estado == 0 ? "ok" : estado == 2 ? "denegado" : "error", (int)tam_texto(res));
    rico_juntar(p->visor, "\n", 1);
    rico_juntar(p->visor, r, k);
    rico_ir_fim(p->visor);
    registrar("agente: panel paso %d %s %s\n", num, p->herr[i], estado == 0 ? "ok" : estado == 2 ? "denegado" : "error");
    ag_panel_desenhar(p);
}

static int cb_confirmar(const char *herr, const char *desc, const char *aviso, bool puede_siempre, void *ctx)
{
    (void)ctx;
    return ag_dialogo_confirmar(herr, desc, aviso, puede_siempre);
}

static int cb_parar(void *ctx)
{
    ag_panel_t *p = ctx;
    evento_t e;
    while (proximo_evento(p->j->id, &e, false) == 1) {
        if (e.tipo == EV_TECLA && e.tecla == TECLA_ESC) p->parar = true;
        else if (e.tipo == EV_CLIQUE && (e.botoes & 1) && en(e.x, e.y, p->bt_deter.x, p->bt_deter.y, p->bt_deter.l, p->bt_deter.a)) p->parar = true;
        else if (e.tipo == EV_FECHAR) { p->parar = true; p->cerrar = true; }
        else if (e.tipo == EV_RODA || e.tipo == EV_CLIQUE) {
            if (e.tipo == EV_CLIQUE && en(e.x, e.y, p->lista.x, p->lista.y, p->lista.l, p->lista.a)) {
                if (lista_tratar(p->j, &p->lista, &e) != LISTA_NADA) { mostrar_detalle(p, p->lista.sel); ag_panel_desenhar(p); }
            } else if (rico_tratar(p->j, p->visor, &e) != RICO_NADA) { /* desplazado */ }
        }
    }
    return p->parar ? 1 : 0;
}

static void enviar_texto(ag_panel_t *p, const char *tarea);

static void enviar(ag_panel_t *p)
{
    if (p->ocupada || !p->ndefs) return;
    if (!p->entrada.buf || !p->entrada.buf[0]) return;
    char *tarea = aloca((size_t)p->entrada.tam + 1);
    if (!tarea) return;
    copia_texto(tarea, p->entrada.buf, (size_t)p->entrada.tam + 1);
    area_definir(&p->entrada, "", 0);
    p->entrada.cursor = 0;
    enviar_texto(p, tarea);
    libera(tarea);
}

/* Lanza el agente elegido con una tarea (la escrita por la persona o una acción rápida de la aplicación). */
static void enviar_texto(ag_panel_t *p, const char *tarea)
{
    if (p->ocupada || !p->ndefs) return;
    if (!p->cfg.url[0]) { estado_p(p, true, "Falta el servidor: Configuracion > IA o las Preferencias del Chat IA"); ag_panel_desenhar(p); return; }
    p->sec_texto = -1;
    rico_secao(p->visor, "Tu", W98_SOMBRA, false);
    rico_juntar(p->visor, tarea, (int)tam_texto(tarea));
    rico_ir_fim(p->visor);
    p->ocupada = true;
    p->parar = false;
    p->n_cola = 0;
    estado_p(p, false, "Trabajando... Esc: detener");
    ag_panel_desenhar(p);
    ag_ui_t ui = { cb_texto, cb_passo, cb_resultado, cb_confirmar, cb_parar, p };
    ag_resultado_t res;
    char erro[256];
    registrar("agente: panel tarea con %s\n", p->defs[p->sel].nome);
    int r = ag_correr(&p->cfg, &p->defs[p->sel], &p->hist, &p->ses, tarea, &ui, p->app, &res, erro, sizeof erro);
    p->ocupada = false;
    if (r < 0 && !p->parar) {
        char t[300];
        fmt(t, sizeof t, "Error: %s", erro);
        estado_p(p, true, t);
        rico_secao(p->visor, "Error", COR_ROJO, false);
        rico_juntar(p->visor, t, (int)tam_texto(t));
    } else if (res.cancelado) estado_p(p, false, "Detenido");
    else if (res.limite_passos) estado_p(p, true, "El agente llego al limite de pasos y se detuvo");
    else if (res.limite_tiempo) estado_p(p, true, "El agente llego al limite de tiempo y se detuvo");
    else if (res.sin_herramientas) estado_p(p, false, "Este modelo no admite herramientas: se contesto como chat normal");
    else { char t[120]; fmt(t, sizeof t, "Listo (%d pasos, %d herramientas, %d denegadas)", res.passos_usados, res.herramientas_usadas, res.denegadas); estado_p(p, false, t); }
    registrar("agente: panel fin pasos=%d herramientas=%d denegadas=%d\n", res.passos_usados, res.herramientas_usadas, res.denegadas);
    rico_ir_fim(p->visor);
    ag_panel_desenhar(p);
}

bool ag_panel_tratar(ag_panel_t *p, const void *ev)
{
    const evento_t *e = ev;
    jan_t *j = p->j;
    if (e->tipo == EV_TECLA) {
        if (e->tecla == TECLA_PGUP || e->tecla == TECLA_PGDN) { rico_tratar(j, p->visor, e); return true; }
        if (e->tecla == '\n' && !(e->modificadores & 1)) { enviar(p); return true; }
        if (area_tratar(j, &p->entrada, e)) { }
        ag_panel_desenhar(p);
        return true;
    }
    if (e->tipo == EV_RODA) {
        if (en(e->x, e->y, p->lista.x, p->lista.y, p->lista.l, p->lista.a)) { lista_tratar(j, &p->lista, e); ag_panel_desenhar(p); return true; }
        return rico_tratar(j, p->visor, e) != RICO_NADA;
    }
    if (e->tipo == EV_MOVER || e->tipo == EV_SOLTOU) {
        area_tratar(j, &p->entrada, e);
        return false;
    }
    if (e->tipo != EV_CLIQUE) return false;
    if (!en(e->x, e->y, p->x, p->y, p->l, p->a)) return false;
    if (!(e->botoes & 1)) return true;
    if (en(e->x, e->y, p->bt_agente.x, p->bt_agente.y, p->bt_agente.l, p->bt_agente.a)) {
        if (p->ndefs) {
            p->sel = (p->sel + 1) % p->ndefs;
            registrar("agente: panel agente %s\n", p->defs[p->sel].nome);
            /* Revisión de seguridad: otro agente es otra conversación. Ni la historia (con lo leído de fuera)
             * ni los «Siempre» del agente anterior pasan al nuevo, que puede tener otras herramientas. */
            if (p->hist.n || p->ses.n_siempre || p->ses.manchada) {
                ag_hist_liberar(&p->hist);
                memset(&p->ses, 0, sizeof p->ses);
                char t[96];
                fmt(t, sizeof t, "Conversacion nueva con %s", p->defs[p->sel].nome);
                p->sec_texto = -1;
                rico_secao(p->visor, t, W98_SOMBRA, false);
                rico_ir_fim(p->visor);
            }
        }
        ag_panel_desenhar(p);
        return true;
    }
    if (en(e->x, e->y, p->bt_nuevo.x, p->bt_nuevo.y, p->bt_nuevo.l, p->bt_nuevo.a)) {
        ag_def_t *d = aloca_zerado(sizeof *d);
        if (d) {
            d->passos = AG_PASSOS_PADRAO;
            copia_texto(d->sistema, "Eres un asistente que ayuda con las herramientas disponibles.", sizeof d->sistema);
            if (editor_agente(d, true)) { recargar_agentes(p); ag_panel_escolher(p, d->nome); }
            libera(d);
        }
        ag_panel_desenhar(p);
        return true;
    }
    if (en(e->x, e->y, p->bt_editar.x, p->bt_editar.y, p->bt_editar.l, p->bt_editar.a) && p->ndefs) {
        ag_def_t *d = aloca(sizeof *d);
        if (d) {
            *d = p->defs[p->sel];
            if (d->de_serie) {                           /* los de serie no se tocan: se guarda una copia propia */
                char nuevo[AG_NOME_MAX];
                fmt(nuevo, sizeof nuevo, "%s (copia)", d->nome);
                copia_texto(d->nome, nuevo, sizeof d->nome);
                d->de_serie = false;
            }
            if (editor_agente(d, false)) { recargar_agentes(p); ag_panel_escolher(p, d->nome); }
            libera(d);
        }
        ag_panel_desenhar(p);
        return true;
    }
    if (en(e->x, e->y, p->bt_registro.x, p->bt_registro.y, p->bt_registro.l, p->bt_registro.a)) { ver_registro_dialogo(); ag_panel_desenhar(p); return true; }
    if (p->detalle_visible && en(e->x, e->y, p->bt_ocultar.x, p->bt_ocultar.y, p->bt_ocultar.l, p->bt_ocultar.a)) {
        p->detalle_visible = false; maquetar(p); ag_panel_desenhar(p); return true;
    }
    if (en(e->x, e->y, p->bt_enviar.x, p->bt_enviar.y, p->bt_enviar.l, p->bt_enviar.a)) { enviar(p); return true; }
    if (en(e->x, e->y, p->lista.x, p->lista.y, p->lista.l, p->lista.a)) {
        int antes = p->lista.sel;
        if (lista_tratar(j, &p->lista, e) != LISTA_NADA) {
            if (p->lista.sel == antes && p->detalle_visible && p->lista.sel >= 0) { p->detalle_visible = false; maquetar(p); }   /* segundo clic: plegar */
            else mostrar_detalle(p, p->lista.sel);
        }
        ag_panel_desenhar(p);
        return true;
    }
    if (en(e->x, e->y, p->detalle.x, p->detalle.y, p->detalle.l, p->detalle.a) && p->detalle_visible) { area_tratar(j, &p->detalle, e); ag_panel_desenhar(p); return true; }
    if (en(e->x, e->y, p->entrada.x, p->entrada.y, p->entrada.l, p->entrada.a)) { area_tratar(j, &p->entrada, e); ag_panel_desenhar(p); return true; }
    rico_tratar(j, p->visor, e);
    return true;
}

/* ------------------------------------------------------------------ ayudas para las aplicaciones (D298) */

void ag_panel_filtro(ag_panel_t *p, bool (*ok)(const ag_def_t *d))
{
    p->filtro = ok;
    recargar_agentes(p);
}

bool ag_panel_ocupado(const ag_panel_t *p) { return p->ocupada; }

bool ag_panel_tarea(ag_panel_t *p, const char *agente, const char *tarea)
{
    if (p->ocupada || !p->ndefs) return false;
    if (agente && agente[0]) ag_panel_escolher(p, agente);
    enviar_texto(p, tarea);
    return true;
}
