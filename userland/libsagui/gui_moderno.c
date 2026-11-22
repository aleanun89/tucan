// SPDX-License-Identifier: GPL-3.0-only
/* gui_moderno.c - componentes modernos de la libsagui (v8.5.5.2, D423).
 *
 * Botones con esquinas redondeadas y estados (normal, hover, pulsado, deshabilitado, foco), control
 * segmentado, interruptor, chips, progreso, insignia, campo de búsqueda, tarjeta, tooltip con retardo y
 * toast. Todo bebe de mod_tema: la paleta y las métricas están en un solo sitio. La fuente sigue siendo
 * la de mapa de bits de 8x16 y los iconos los de icones.h; lo nuevo es la forma (esquinas suavizadas,
 * bordes de 1 px, hover suave) y el color (verde y turquesa de SaguiOS sobre superficies claras).
 *
 * Las esquinas se suavizan con 4x4 muestras por píxel de esquina y se mezclan con lo que ya hay dibujado,
 * así que valen sobre cualquier fondo. Ideas de diseño tomadas de las guías de interfaz de sistemas
 * modernos (botón primario/secundario/plano, píldoras, anillo de foco): solo ideas, ningún código. */
#include "gui.h"
#include "icones.h"
#include "fontes_sistema.h"

/* ------------------------------------------------------------ tema */

#define TEMA_CLARO { \
    .acento = 0x0B7A3E, .acento_hover = 0x0E9048, .acento_press = 0x08652F, .acento_suave = 0xD9EEE0, .sobre_acento = 0xFFFFFF, \
    .foco = 0x0A8F96, \
    .superficie = 0xFFFFFF, .barra = 0xEEF2F3, .campo = 0xFFFFFF, \
    .pista = 0xD3DBDF, .borde = 0xC2CBD0, .borde_forte = 0x8E9BA3, \
    .hover = 0xDDE5E8, .press = 0xCAD4D8, \
    .texto = 0x1E2328, .texto_fraco = 0x5B666F, \
    .desab_fundo = 0xE6EAEC, .desab_texto = 0x98A2A9, \
    .peligro = 0xB3261E, .peligro_suave = 0xF7DEDC, .aviso = 0xD98A00, .ok = 0x0B7A3E, \
    .dica_fundo = 0x263238, .dica_texto = 0xFFFFFF, \
    .radio = 4, .radio_cartao = 6, .altura = 28, \
}

mod_tema_t mod_tema = TEMA_CLARO;

/* ------------------------------------------------------------ esquema de apariencia (D428) */

extern unsigned char fonte_sagui[256][16];
static unsigned char fonte_orig[256][16];
static bool fonte_orig_ok;
static uint32_t fonte_atual;

/* D428: la fuente del sistema (ver kernel/gui/aparencia.c): se copia la tabla elegida sobre fonte_sagui. */
static void fonte_aplicar(uint32_t f)
{
    if (f > 3) f = 0;
    if (f == fonte_atual) return;
    if (!fonte_orig_ok) { memcpy(fonte_orig, fonte_sagui, sizeof fonte_orig); fonte_orig_ok = true; }
    const unsigned char (*t)[16] = f == 1 ? fonte_sis_terminus : f == 2 ? fonte_sis_terminusb : f == 3 ? fonte_sis_fixed : fonte_orig;
    memcpy(fonte_sagui, t, sizeof fonte_orig);
    fonte_atual = f;
}

esquema_t sagui_esq;                 /* se rellena con «SaguiOS» al cargar la biblioteca y con el del kernel al abrir una ventana */
static int esq_versao = -1;

__attribute__((constructor)) static void esq_iniciar(void)
{
    esq_saguios(&sagui_esq, 0x0B7A3E);
}

/* mod_tema sale del esquema: con «SaguiOS» da exactamente TEMA_CLARO. */
void esq_para_mod_tema(void)
{
    mod_tema_t *t = &mod_tema;
    t->acento = sagui_esq.destaque; t->acento_hover = sagui_esq.m_acento_hover; t->acento_press = sagui_esq.m_acento_press;
    t->acento_suave = sagui_esq.m_acento_suave; t->sobre_acento = sagui_esq.m_sobre_acento; t->foco = sagui_esq.foco;
    t->superficie = sagui_esq.m_superficie; t->barra = sagui_esq.m_barra; t->campo = sagui_esq.campo_fundo;
    t->pista = sagui_esq.m_pista; t->borde = sagui_esq.m_borde; t->borde_forte = sagui_esq.m_borde_forte;
    t->hover = sagui_esq.m_hover; t->press = sagui_esq.m_press; t->texto = sagui_esq.texto; t->texto_fraco = sagui_esq.m_texto_fraco;
    t->desab_fundo = sagui_esq.m_desab_fundo; t->desab_texto = sagui_esq.m_desab_texto;
    t->peligro = sagui_esq.m_peligro; t->peligro_suave = sagui_esq.m_peligro_suave; t->aviso = sagui_esq.m_aviso; t->ok = sagui_esq.m_ok;
    t->dica_fundo = sagui_esq.m_dica_fundo; t->dica_texto = sagui_esq.m_dica_texto;
}

int esq_recargar(void)
{
    esquema_t n;
    int v = esquema_op(ESQ_OP_LER, &n, sizeof n);
    if (v < 0) return v;
    sagui_esq = n;
    esq_versao = v;
    fonte_aplicar(n.fonte);
    esq_para_mod_tema();
    return 0;
}

/* Mira la versión (llamada barata) y recarga si cambió: así una ventana nueva ya nace con el esquema vigente. */
void esq_asegurar(void)
{
    int v = esquema_op(ESQ_OP_VERSAO, 0, 0);
    if (v > 0 && v != esq_versao) esq_recargar();
}

int esq_aplicar(const esquema_t *e, bool login)
{
    int r = esquema_op(login ? ESQ_OP_FIJAR_LOGIN : ESQ_OP_FIJAR, (void *)e, sizeof *e);
    if (r > 0) esq_recargar();
    return r < 0 ? r : 0;
}

/* ------------------------------------------------------------ texto del esquema (ficheros de Configuración) */

static const char hexd[] = "0123456789abcdef";

/* Formato (una clave por línea): «estilo=clave» para un incluido sin tocar (con «acento=RRGGBB» si es SaguiOS y el
 * acento no es el de serie); para un personalizado, «estilo=personal», «nombre=», «base=», «banderas=» (oscuro, barra
 * propia, escritorio liso) y «valores=» con todos los colores en hexadecimal. Devuelve los bytes escritos. */
int esq_para_texto(const esquema_t *e, char *buf, int max)
{
    esquema_t ref;
    int n = 0;
    if (e->id < ESQ_QTD) {
        esq_incluido(&ref, (int)e->id, e->id == ESQ_SAGUIOS ? e->acento : 0xFFFFFFFFu);
        if (ref.id == e->id && memcmp(ref.v, e->v, sizeof ref.v) == 0) {
            n = formatar_texto(buf, (size_t)max, "estilo=%s\n", esq_claves[e->id]);
            if (e->id == ESQ_SAGUIOS && e->acento != 0x0B7A3E) n += formatar_texto(buf + n, (size_t)max - (size_t)n, "acento=%06x\n", e->acento);
            if (e->fonte) n += formatar_texto(buf + n, (size_t)max - (size_t)n, "fonte=%u\n", e->fonte);
            return n;
        }
    }
    n = formatar_texto(buf, (size_t)max, "estilo=personal\nnombre=%s\nbase=%u\nfonte=%u\nbanderas=%u%u%u\nvalores=", e->nome, e->base, e->fonte,
                       e->escuro ? 1u : 0u, e->barra_propia ? 1u : 0u, e->escritorio_liso ? 1u : 0u);
    for (int i = 0; i < ESQ_NC && n + 8 < max; i++)
        for (int sh = 20; sh >= 0; sh -= 4) buf[n++] = hexd[(e->v[i] >> sh) & 15];
    buf[n++] = '\n';
    buf[n] = 0;
    return n;
}

static int hex_valor(int c)
{
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}

/* Lo contrario: true si el texto describe un esquema válido (lo deja en *e). */
bool esq_de_texto(const char *txt, esquema_t *e)
{
    char clave[ESQ_NOME_MAX] = "";
    uint32_t acento = 0xFFFFFFFFu;
    esquema_t personal;
    bool tiene_valores = false, es_personal = false;
    uint32_t fonte = 0;
    memset(&personal, 0, sizeof personal);
    for (const char *p = txt; *p;) {
        const char *fin = acha_char(p, '\n');
        int tl = fin ? (int)(fin - p) : (int)tam_texto(p);
        const char *eq = p;
        while (eq < p + tl && *eq != '=') eq++;
        if (eq < p + tl && p[0] != '#') {
            int tk = (int)(eq - p);
            const char *v = eq + 1;
            int tv = tl - tk - 1;
            while (tv > 0 && (v[tv - 1] == '\r' || v[tv - 1] == ' ')) tv--;
            if (tk == 6 && memcmp(p, "estilo", 6) == 0) { int m = tv < (int)sizeof clave - 1 ? tv : (int)sizeof clave - 1; memcpy(clave, v, (size_t)m); clave[m] = 0; }
            else if (tk == 6 && memcmp(p, "acento", 6) == 0 && tv == 6) {
                uint32_t c = 0;
                for (int i = 0; i < 6; i++) { int h = hex_valor(v[i]); if (h < 0) { c = 0xFFFFFFFFu; break; } c = c << 4 | (uint32_t)h; }
                acento = c;
            } else if (tk == 6 && memcmp(p, "nombre", 6) == 0) {
                int m = tv < ESQ_NOME_MAX - 1 ? tv : ESQ_NOME_MAX - 1;
                memcpy(personal.nome, v, (size_t)m);
                personal.nome[m] = 0;
            } else if (tk == 4 && memcmp(p, "base", 4) == 0) personal.base = (uint32_t)texto_para_int(v);
            else if (tk == 5 && memcmp(p, "fonte", 5) == 0) { int f = texto_para_int(v); fonte = f >= 0 && f <= 3 ? (uint32_t)f : 0; }
            else if (tk == 8 && memcmp(p, "banderas", 8) == 0 && tv >= 3) {
                personal.escuro = v[0] == '1'; personal.barra_propia = v[1] == '1'; personal.escritorio_liso = v[2] == '1';
            } else if (tk == 7 && memcmp(p, "valores", 7) == 0 && tv == ESQ_NC * 6) {
                tiene_valores = true;
                for (int i = 0; i < ESQ_NC && tiene_valores; i++) {
                    uint32_t c = 0;
                    for (int k = 0; k < 6; k++) { int h = hex_valor(v[i * 6 + k]); if (h < 0) { tiene_valores = false; break; } c = c << 4 | (uint32_t)h; }
                    personal.v[i] = c;
                }
            }
        }
        if (!fin) break;
        p = fin + 1;
    }
    if (compara_texto(clave, "personal") == 0) es_personal = true;
    if (es_personal) {
        if (!tiene_valores) return false;
        personal.id = ESQ_PERSONAL;
        if (personal.base >= ESQ_QTD) personal.base = ESQ_SAGUIOS;
        if (!personal.nome[0]) copia_texto(personal.nome, "Personal", sizeof personal.nome);
        personal.fonte = fonte;
        *e = personal;
        return true;
    }
    for (int i = 0; i < ESQ_QTD; i++)
        if (compara_texto(clave, esq_claves[i]) == 0) { esq_incluido(e, i, i == ESQ_SAGUIOS ? (acento == 0xFFFFFFFFu ? 0x0B7A3E : acento) : 0xFFFFFFFFu); e->fonte = fonte; return true; }
    return false;
}

/* ------------------------------------------------------------ vista previa en miniatura */

static void prev_grad(jan_t *j, int x, int y, int l, int a, uint32_t c1, uint32_t c2, bool vertical)
{
    if (c1 == c2) { g_retangulo(j, x, y, l, a, c1); return; }
    int n = vertical ? a : l;
    for (int k = 0; k < n; k++) {
        uint32_t c = esq_mix(c1, c2, n > 1 ? k * 100 / (n - 1) : 0);
        if (vertical) g_retangulo(j, x, y + k, l, 1, c);
        else g_retangulo(j, x + k, y, 1, a, c);
    }
}

/* Una ventana de muestra: marco, título con degradado y texto, y su cuerpo (que dibuja quien llama). */
static void prev_ventana(jan_t *j, int x, int y, int l, int a, const esquema_t *e, bool activa, const char *titulo)
{
    uint32_t marco = activa ? e->marco_a : e->marco_i;
    g_retangulo(j, x, y, l, a, marco);
    prev_grad(j, x, y, l, 16, activa ? e->titulo_a1 : e->titulo_i1, activa ? e->titulo_a2 : e->titulo_i2, false);
    g_moldura(j, x - 1, y - 1, l + 2, a + 2, e->contorno);
    g_texto(j, x + 4, y, titulo, activa ? e->titulo_a_texto : e->titulo_i_texto);
    int bx = x + l - 14;
    g_retangulo(j, bx, y + 2, 12, 12, activa ? e->cerrar_a : e->cerrar_i);
    g_linha(j, bx + 3, y + 5, bx + 8, y + 10, e->cerrar_x);
    g_linha(j, bx + 8, y + 5, bx + 3, y + 10, e->cerrar_x);
    g_retangulo(j, x + 2, y + 16, l - 4, a - 18, e->fundo);
}

/* Dibuja, en un rectángulo de 320 x 200, un escritorio con una ventana activa, otra inactiva, un menú abierto, un
 * botón y un campo, todo con los colores del esquema: lo que ve la persona antes de elegirlo. */
void esq_vista_previa(jan_t *j, int x, int y, int l, int a, const esquema_t *e)
{
    if (l < 320 || a < 200) return;
    uint32_t f1 = e->escritorio_a, f2 = e->escritorio_b;
    if (!e->escritorio_liso) {   /* el tricolor de SaguiOS */
        for (int k = 0; k < a; k++) {
            int h = a / 2;
            uint32_t c = k < h ? esq_mix(0x0B7A3E, 0xE0B92A, k * 100 / h) : esq_mix(0xE0B92A, 0x14337E, (k - h) * 100 / (a - h));
            g_retangulo(j, x, y + k, l, 1, c);
        }
    } else prev_grad(j, x, y, l, a, f1, f2, true);
    /* barra de tareas */
    g_retangulo(j, x, y, l, 16, e->barra);
    g_retangulo(j, x + 2, y + 1, 52, 14, e->acento);
    g_texto(j, x + 4, y, "Inicio", e->m_sobre_acento);
    g_retangulo(j, x + 60, y + 1, 66, 14, esq_mix(e->barra, e->barra_texto, 22));
    g_texto(j, x + 62, y, "Ventana", e->barra_texto);
    /* ventana inactiva y activa */
    prev_ventana(j, x + 10, y + 28, 168, 86, e, false, "Inactiva");
    g_texto(j, x + 16, y + 48, "Texto", e->texto_dis);
    prev_ventana(j, x + 82, y + 64, 232, 130, e, true, "Ventana activa");
    /* barra de menú y menú abierto */
    g_retangulo(j, x + 84, y + 82, 228, 16, e->cara);
    g_texto(j, x + 88, y + 82, "Archivo", e->texto);
    g_texto(j, x + 152, y + 82, "Ver", e->texto);
    g_retangulo(j, x + 86, y + 98, 84, 52, e->menu_fundo);
    g_moldura(j, x + 86, y + 98, 84, 52, e->menu_borde);
    g_retangulo(j, x + 88, y + 100, 80, 16, e->menu_sel);
    g_texto(j, x + 92, y + 100, "Abrir", e->menu_sel_texto);
    g_texto(j, x + 92, y + 116, "Copiar", e->menu_texto);
    g_texto(j, x + 92, y + 132, "Pegar", e->texto_dis);
    /* botón y campo */
    g_retangulo(j, x + 182, y + 104, 62, 22, e->botao);
    g_moldura(j, x + 182, y + 104, 62, 22, e->borda);
    g_texto(j, x + 190, y + 107, "Acepto", e->texto);
    g_retangulo(j, x + 182, y + 132, 126, 20, e->campo_fundo);
    g_moldura(j, x + 182, y + 132, 126, 20, e->borda);
    g_retangulo(j, x + 184, y + 134, 48, 16, e->selecao);
    g_texto(j, x + 186, y + 134, "Sagui", e->selecao_texto);
    g_texto(j, x + 234, y + 134, "OS", e->campo_texto);
    g_retangulo(j, x + 182, y + 158, 126, 2, e->scroll_polegar);
    g_texto(j, x + 184, y + 172, "Info", e->texto);
    g_retangulo(j, x + 232, y + 172, 70, 18, e->info_fundo);
    g_moldura(j, x + 232, y + 172, 70, 18, e->info_borde);
    g_texto_n(j, x + 236, y + 172, "Tip", 3, e->info_texto);
}

/* Variante clara de un color de trazo oscuro sobre un fondo oscuro (los iconos de icones.h son de trazo oscuro). */
uint32_t esq_trazo(uint32_t c)
{
    if (!sagui_esq.escuro || esq_lum(c) >= 90) return c;
    return esq_mix(c, 0xFFFFFF, 80);
}

void mod_tema_padrao(mod_tema_t *t)
{
    static const mod_tema_t p = TEMA_CLARO;
    *t = p;
}

uint32_t mod_mezclar(uint32_t a, uint32_t b, unsigned t)
{
    if (t >= 255) return b & 0xFFFFFF;
    if (t == 0) return a & 0xFFFFFF;
    unsigned ia = 255 - t;
    unsigned r = (((a >> 16) & 255) * ia + ((b >> 16) & 255) * t + 127) / 255;
    unsigned g = (((a >> 8) & 255) * ia + ((b >> 8) & 255) * t + 127) / 255;
    unsigned bl = ((a & 255) * ia + (b & 255) * t + 127) / 255;
    return (r << 16) | (g << 8) | bl;
}

/* ------------------------------------------------------------ primitivas con esquinas suaves */

static void pix(jan_t *j, int x, int y, uint32_t cor, unsigned alfa)
{
    if ((unsigned)x >= (unsigned)j->larg || (unsigned)y >= (unsigned)j->alt || !alfa) return;
    uint32_t *p = &j->px[y * j->larg + x];
    *p = alfa >= 255 ? (cor & 0xFFFFFF) : mod_mezclar(*p, cor, alfa);
}

/* Cobertura (0..16) del píxel (px, py) de un rectángulo redondeado l x a con radio r; 4x4 muestras en las esquinas. */
static int cobertura(int px, int py, int l, int a, int r)
{
    if (px < 0 || py < 0 || px >= l || py >= a) return 0;
    if (px >= r && px < l - r) return 16;
    if (py >= r && py < a - r) return 16;
    int cx8 = (px < r ? r : l - r) * 8, cy8 = (py < r ? r : a - r) * 8, R8 = r * 8, n = 0;
    for (int iy = 0; iy < 4; iy++)
        for (int ix = 0; ix < 4; ix++) {
            int dx = px * 8 + 2 * ix + 1 - cx8, dy = py * 8 + 2 * iy + 1 - cy8;
            if (dx * dx + dy * dy <= R8 * R8) n++;
        }
    return n;
}

static int acotar_radio(int l, int a, int r)
{
    if (r > l / 2) r = l / 2;
    if (r > a / 2) r = a / 2;
    return r < 0 ? 0 : r;
}

void g_retangulo_red_a(jan_t *j, int x, int y, int l, int a, int r, uint32_t cor, unsigned alfa)
{
    if (l <= 0 || a <= 0 || !alfa) return;
    r = acotar_radio(l, a, r);
    for (int yy = 0; yy < a; yy++) {
        int xa = 0, xb = l;
        if (yy < r || yy >= a - r) {
            for (int xx = 0; xx < r; xx++) pix(j, x + xx, y + yy, cor, alfa * (unsigned)cobertura(xx, yy, l, a, r) / 16);
            for (int xx = l - r; xx < l; xx++) pix(j, x + xx, y + yy, cor, alfa * (unsigned)cobertura(xx, yy, l, a, r) / 16);
            xa = r; xb = l - r;
        } else if (alfa >= 255) {
            int y2 = yy;
            while (y2 < a - r) y2++;   /* las filas rectas se pintan de un golpe */
            g_retangulo(j, x, y + yy, l, y2 - yy, cor);
            yy = y2 - 1;
            continue;
        }
        if (xb > xa) {
            if (alfa >= 255) g_retangulo(j, x + xa, y + yy, xb - xa, 1, cor);
            else for (int xx = xa; xx < xb; xx++) pix(j, x + xx, y + yy, cor, alfa);
        }
    }
}

void g_retangulo_red(jan_t *j, int x, int y, int l, int a, int r, uint32_t cor)
{
    g_retangulo_red_a(j, x, y, l, a, r, cor, 255);
}

void g_aro_red(jan_t *j, int x, int y, int l, int a, int r, int esp, uint32_t cor)
{
    if (l <= 0 || a <= 0 || esp <= 0) return;
    r = acotar_radio(l, a, r);
    int ri = r > esp ? r - esp : 0, li = l - 2 * esp, ai = a - 2 * esp;
    int banda = r > esp ? r : esp;   /* arriba y abajo: filas con esquina o con el grosor entero */
    for (int yy = 0; yy < a; yy++) {
        if (yy >= banda && yy < a - banda) {
            g_retangulo(j, x, y + yy, esp, a - 2 * banda, cor);
            g_retangulo(j, x + l - esp, y + yy, esp, a - 2 * banda, cor);
            yy = a - banda - 1;
            continue;
        }
        for (int xx = 0; xx < l; xx++) {
            int c = cobertura(xx, yy, l, a, r) - (li > 0 && ai > 0 ? cobertura(xx - esp, yy - esp, li, ai, ri) : 0);
            if (c > 0) pix(j, x + xx, y + yy, cor, (unsigned)c * 255 / 16);
        }
    }
}

void g_caixa_red(jan_t *j, int x, int y, int l, int a, int r, uint32_t fundo, uint32_t borde)
{
    g_retangulo_red(j, x, y, l, a, r, borde);
    g_retangulo_red(j, x + 1, y + 1, l - 2, a - 2, r > 0 ? r - 1 : 0, fundo);
}

void g_sombra_red(jan_t *j, int x, int y, int l, int a, int r, int fuerza)
{
    unsigned f = fuerza >= 2 ? 2 : 1;
    g_retangulo_red_a(j, x - 2, y, l + 4, a + 3, r + 2, 0x10202A, 9 * f);
    g_retangulo_red_a(j, x - 1, y, l + 2, a + 2, r + 1, 0x10202A, 11 * f);
    g_retangulo_red_a(j, x, y + 1, l, a + 1, r, 0x10202A, 14 * f);
}

static const char *tr(const char *t) { return t && t[0] ? idioma_traducir_sl(t) : ""; }

static int ancho_texto(const char *t) { return g_largura_texto(tr(t)); }

/* Icono de una sola tinta: la silueta de icones.h con la tinta de cor (los tonos claros del dibujo se atenúan).
 * Se usa sobre el relleno de acento (blanco) y en los botones deshabilitados (gris). */
static void icone_mono(jan_t *j, int x, int y, const char *chave, uint32_t cor)
{
    const icone_t *ic = icones_procurar(chave);
    if (!ic) return;
    for (int yy = 0; yy < ICONES_LADO; yy++)
        for (int xx = 0; xx < ICONES_LADO; xx++) {
            char c = ic->linhas[yy][xx];
            if (c == '.' || icones_cor(c) == 0xFF000000) continue;
            unsigned a = (c == 'P' || c == 'D') ? 255 : (c == 'B' || c == 'L') ? 70 : (c == 'C') ? 170 : 230;
            pix(j, x + xx, y + yy, cor, a);
        }
}

/* El icono de un botón: en color, o de una tinta si va sobre el acento o está deshabilitado. */
static void icone_estado(jan_t *j, int x, int y, const char *ic, bool desab, uint32_t mono)
{
    if (!ic) return;
    if (desab || mono) icone_mono(j, x, y, ic, desab ? mod_tema.desab_texto : mono);
    else icone_desenhar(j, x, y, ic, 1);
}

/* El anillo de foco: turquesa por dentro del borde, con un filete claro por dentro para que se vea sobre el relleno. */
static void anillo_foco(jan_t *j, int x, int y, int l, int a, int r)
{
    g_aro_red(j, x, y, l, a, r, 2, mod_tema.foco);
    g_aro_red(j, x + 2, y + 2, l - 4, a - 4, r > 2 ? r - 2 : 0, 1, mod_tema.superficie);
}

void mod_separador(jan_t *j, int x, int y, int a)
{
    g_retangulo(j, x, y, 1, a, mod_tema.borde);
}

/* ------------------------------------------------------------ botón */

#define MB_PAD 10
#define MB_GAP 6

void mbotao_ajustar(mbotao_t *b)
{
    if (b->a <= 0) b->a = mod_tema.altura;
    bool solo_icono = !b->texto || !b->texto[0];
    if (b->l > 0) return;
    if (b->variante == MB_ICONO || b->variante == MB_ICONO_REDONDO || (solo_icono && b->icone)) { b->l = b->a; return; }
    int c = ancho_texto(b->texto);
    if (b->icone) c += MB_ICONO_L + (b->texto && b->texto[0] ? MB_GAP : 0);
    b->l = c + 2 * MB_PAD;
}

void mbotao_desenhar(jan_t *j, mbotao_t *b)
{
    mbotao_ajustar(b);
    const mod_tema_t *t = &mod_tema;
    uint32_t fundo = b->fundo ? b->fundo : t->barra;
    bool redondo = b->variante == MB_ICONO_REDONDO;
    int r = redondo ? b->a / 2 : t->radio;
    bool hov = b->hover && !b->desabilitado, pres = b->apertado && !b->desabilitado;
    g_retangulo(j, b->x, b->y, b->l, b->a, fundo);

    uint32_t cor_texto = t->texto;
    switch (b->variante) {
    case MB_PRIMARIO:
        if (b->desabilitado) { g_retangulo_red(j, b->x, b->y, b->l, b->a, r, t->desab_fundo); cor_texto = t->desab_texto; }
        else { g_retangulo_red(j, b->x, b->y, b->l, b->a, r, pres ? t->acento_press : hov ? t->acento_hover : t->acento); cor_texto = t->sobre_acento; }
        break;
    case MB_SECUNDARIO:
        if (b->desabilitado) { g_caixa_red(j, b->x, b->y, b->l, b->a, r, t->superficie, t->desab_fundo); cor_texto = t->desab_texto; }
        else g_caixa_red(j, b->x, b->y, b->l, b->a, r, pres ? t->press : hov ? t->hover : t->superficie, hov || pres ? t->borde_forte : t->borde);
        break;
    default:   /* plano, peligro, icono, icono redondo: sin nada hasta el hover */
        if (b->desabilitado) cor_texto = t->desab_texto;
        else if (b->variante == MB_PELIGRO) {
            cor_texto = t->peligro;
            if (pres || hov) g_retangulo_red(j, b->x, b->y, b->l, b->a, r, pres ? mod_mezclar(t->peligro_suave, t->peligro, 40) : t->peligro_suave);
        } else {
            if (b->ativo) { g_retangulo_red(j, b->x, b->y, b->l, b->a, r, pres ? mod_mezclar(t->acento_suave, t->acento, 50) : t->acento_suave); cor_texto = t->acento; }
            else if (pres || hov) g_retangulo_red(j, b->x, b->y, b->l, b->a, r, pres ? t->press : t->hover);
            if (b->ativo && hov && !pres) g_aro_red(j, b->x, b->y, b->l, b->a, r, 1, t->acento);
        }
        break;
    }

    bool solo_icono = !b->texto || !b->texto[0];
    int tw = solo_icono ? 0 : ancho_texto(b->texto);
    int cw = (b->icone ? MB_ICONO_L : 0) + (b->icone && !solo_icono ? MB_GAP : 0) + tw;
    int cx = b->x + (b->l - cw) / 2 + (pres ? 0 : 0), cy = b->y + (b->a - MB_ICONO_L) / 2 + (pres && b->variante == MB_PRIMARIO ? 1 : 0);
    if (b->icone) {
        icone_estado(j, cx, cy, b->icone, b->desabilitado, b->variante == MB_PRIMARIO ? t->sobre_acento : 0);
        cx += MB_ICONO_L + (solo_icono ? 0 : MB_GAP);
    }
    if (!solo_icono) g_texto(j, cx, b->y + (b->a - LETRA_A) / 2 + (pres && b->variante == MB_PRIMARIO ? 1 : 0), tr(b->texto), cor_texto);
    if (b->foco && !b->desabilitado) anillo_foco(j, b->x, b->y, b->l, b->a, r);
    jan_mostrar_area(j, b->x, b->y, b->l, b->a);
    GR_REG(j, "mbotao", b, b->x, b->y, b->l, b->a, b->texto ? b->texto : "", b->l - 2 * MB_PAD + 4, GRF_CONTROL);
}

static bool dentro(int px, int py, int x, int y, int l, int a)
{
    return px >= x && py >= y && px < x + l && py < y + a;
}

bool mbotao_tratar(jan_t *j, mbotao_t *b, const evento_t *e)
{
    mbotao_ajustar(b);
    bool in = dentro(e->x, e->y, b->x, b->y, b->l, b->a);
    if (b->desabilitado) {
        if (b->hover || b->apertado) { b->hover = b->apertado = false; mbotao_desenhar(j, b); }
        return false;
    }
    if (e->tipo == EV_MOVER) {
        bool cambio = false;
        if (in != b->hover) { b->hover = in; cambio = true; }
        if (b->apertado && !(e->botoes & 1)) { b->apertado = false; cambio = true; }
        if (cambio) mbotao_desenhar(j, b);
        return false;
    }
    if (e->tipo == EV_CLIQUE && (e->botoes & 1) && in) {
        b->apertado = b->hover = true;
        mbotao_desenhar(j, b);
        return false;
    }
    if (e->tipo == EV_SOLTOU && b->apertado) {
        b->apertado = false;
        b->hover = in;
        mbotao_desenhar(j, b);
        return in;
    }
    if (e->tipo == EV_TECLA && b->foco && (e->tecla == '\n' || e->tecla == ' ')) return true;
    return false;
}

/* ------------------------------------------------------------ control segmentado */

#define SEG_BORDE 2   /* margen del indicador dentro de la pista */

void segmentado_ajustar(segmentado_t *s)
{
    if (s->a <= 0) s->a = mod_tema.altura;
    int total = 2 * SEG_BORDE;
    for (int k = 0; k < s->n; k++) {
        bool tx = s->textos[k] && s->textos[k][0];
        int c = (s->icones[k] ? MB_ICONO_L : 0) + (s->icones[k] && tx ? MB_GAP : 0) + (tx ? ancho_texto(s->textos[k]) : 0);
        s->larg[k] = tx ? c + 2 * MB_PAD : s->a + 2;   /* solo icono: casi cuadrado */
        total += s->larg[k];
    }
    s->l = total;
}

void segmentado_rect(const segmentado_t *s, int k, int *x, int *y, int *l, int *a)
{
    int px = s->x + SEG_BORDE;
    for (int i = 0; i < k; i++) px += s->larg[i];
    *x = px; *y = s->y + SEG_BORDE; *l = s->larg[k]; *a = s->a - 2 * SEG_BORDE;
}

int segmentado_em(const segmentado_t *s, int x, int y)
{
    if (!dentro(x, y, s->x, s->y, s->l, s->a)) return -1;
    for (int k = 0; k < s->n; k++) {
        int px, py, pl, pa;
        segmentado_rect(s, k, &px, &py, &pl, &pa);
        if (x >= px && x < px + pl) return k;
    }
    return -1;
}

void segmentado_desenhar(jan_t *j, segmentado_t *s)
{
    segmentado_ajustar(s);
    const mod_tema_t *t = &mod_tema;
    uint32_t fundo = s->fundo ? s->fundo : t->barra;
    g_retangulo(j, s->x, s->y, s->l, s->a, fundo);
    g_caixa_red(j, s->x, s->y, s->l, s->a, t->radio + 1, t->pista, t->borde);
    for (int k = 0; k < s->n; k++) {
        int px, py, pl, pa;
        segmentado_rect(s, k, &px, &py, &pl, &pa);
        bool sel = k == s->sel, hov = k == s->hover && !sel;
        if (sel) { g_retangulo_red(j, px, py, pl, pa, t->radio - 1, t->acento_suave); g_aro_red(j, px, py, pl, pa, t->radio - 1, 1, t->acento); }
        else if (hov) g_retangulo_red(j, px, py, pl, pa, t->radio - 1, t->hover);
        bool tx = s->textos[k] && s->textos[k][0];
        int cw = (s->icones[k] ? MB_ICONO_L : 0) + (s->icones[k] && tx ? MB_GAP : 0) + (tx ? ancho_texto(s->textos[k]) : 0);
        int cx = px + (pl - cw) / 2;
        if (s->icones[k]) {
            icone_estado(j, cx, py + (pa - MB_ICONO_L) / 2, s->icones[k], false, 0);
            cx += MB_ICONO_L + (tx ? MB_GAP : 0);
        }
        if (tx) g_texto(j, cx, py + (pa - LETRA_A) / 2, tr(s->textos[k]), sel ? t->acento : t->texto);
    }
    if (s->foco) g_aro_red(j, s->x - 1, s->y - 1, s->l + 2, s->a + 2, t->radio + 2, 2, t->foco);
    jan_mostrar_area(j, s->x - 1, s->y - 1, s->l + 2, s->a + 2);
    GR_REG(j, "segmentado", s, s->x, s->y, s->l, s->a, "", 0, GRF_CONTROL);
}

int segmentado_tratar(jan_t *j, segmentado_t *s, const evento_t *e)
{
    segmentado_ajustar(s);
    if (e->tipo == EV_MOVER) {
        int k = segmentado_em(s, e->x, e->y);
        if (k != s->hover) { s->hover = k; segmentado_desenhar(j, s); }
        return -1;
    }
    if (e->tipo == EV_CLIQUE && (e->botoes & 1)) {
        int k = segmentado_em(s, e->x, e->y);
        if (k >= 0) { s->sel = k; s->hover = k; segmentado_desenhar(j, s); }
        return k;
    }
    if (e->tipo == EV_TECLA && s->foco && s->n > 0) {
        if (e->tecla == TECLA_ESQUERDA && s->sel > 0) { s->sel--; segmentado_desenhar(j, s); return s->sel; }
        if (e->tecla == TECLA_DIREITA && s->sel < s->n - 1) { s->sel++; segmentado_desenhar(j, s); return s->sel; }
    }
    return -1;
}

/* ------------------------------------------------------------ interruptor */

int interruptor_largura(const interruptor_t *s)
{
    return INTERRUPTOR_L + (s->texto && s->texto[0] ? 8 + ancho_texto(s->texto) : 0);
}

static void interruptor_pintar(jan_t *j, interruptor_t *s, int paso)   /* paso 0..4: posición de la bolita */
{
    const mod_tema_t *t = &mod_tema;
    uint32_t fundo = s->fundo ? s->fundo : t->barra;
    int l = interruptor_largura(s);
    g_retangulo(j, s->x - 2, s->y - 2, l + 4, INTERRUPTOR_A + 4, fundo);
    uint32_t off = s->hover ? t->borde_forte : mod_mezclar(t->borde_forte, t->pista, 70);
    uint32_t pista = s->desabilitado ? t->desab_fundo : mod_mezclar(off, s->hover ? t->acento_hover : t->acento, (unsigned)paso * 255 / 4);
    g_retangulo_red(j, s->x, s->y, INTERRUPTOR_L, INTERRUPTOR_A, INTERRUPTOR_A / 2, pista);
    int bola = INTERRUPTOR_A - 6, rec = INTERRUPTOR_L - 6 - bola;
    int bx = s->x + 3 + rec * paso / 4;
    g_retangulo_red_a(j, bx, s->y + 4, bola, bola, bola / 2, 0x10202A, 40);   /* sombra de la bolita */
    g_retangulo_red(j, bx, s->y + 3, bola, bola, bola / 2, s->desabilitado ? mod_mezclar(t->superficie, t->desab_fundo, 128) : 0xFFFFFF);
    if (s->texto && s->texto[0])
        g_texto(j, s->x + INTERRUPTOR_L + 8, s->y + (INTERRUPTOR_A - LETRA_A) / 2, tr(s->texto), s->desabilitado ? t->desab_texto : t->texto);
    if (s->foco && !s->desabilitado) g_aro_red(j, s->x - 2, s->y - 2, INTERRUPTOR_L + 4, INTERRUPTOR_A + 4, INTERRUPTOR_A / 2 + 2, 2, t->foco);
    jan_mostrar_area(j, s->x - 2, s->y - 2, l + 4, INTERRUPTOR_A + 4);
}

void interruptor_desenhar(jan_t *j, interruptor_t *s)
{
    interruptor_pintar(j, s, s->ligado ? 4 : 0);
    GR_REG(j, "interruptor", s, s->x, s->y, interruptor_largura(s), INTERRUPTOR_A, s->texto ? s->texto : "", 0, GRF_CONTROL);
}

bool interruptor_tratar(jan_t *j, interruptor_t *s, const evento_t *e)
{
    if (s->desabilitado) return false;
    int l = interruptor_largura(s);
    bool in = dentro(e->x, e->y, s->x, s->y, l, INTERRUPTOR_A);
    if (e->tipo == EV_MOVER) {
        if (in != s->hover) { s->hover = in; interruptor_desenhar(j, s); }
        return false;
    }
    bool pulsa = (e->tipo == EV_CLIQUE && (e->botoes & 1) && in) || (e->tipo == EV_TECLA && s->foco && (e->tecla == ' ' || e->tecla == '\n'));
    if (!pulsa) return false;
    s->ligado = !s->ligado;
    for (int p = 1; p <= 4; p++) {   /* la bolita se desliza */
        interruptor_pintar(j, s, s->ligado ? p : 4 - p);
        if (p < 4) dormir(14);
    }
    return true;
}

/* ------------------------------------------------------------ chip */

#define CHIP_ALT 22

void chip_ajustar(chip_t *c)
{
    if (c->a <= 0) c->a = CHIP_ALT;
    if (c->l > 0) return;
    c->l = 2 * 9 + ancho_texto(c->texto) + (c->cor ? 14 : 0) + (c->fechavel ? 18 : 0);
}

static void cruz(jan_t *j, int cx, int cy, uint32_t cor)   /* una «x» de 7x7 con trazo de 1 px */
{
    for (int k = -3; k <= 3; k++) {
        g_retangulo(j, cx + k, cy + k, 1, 1, cor);
        g_retangulo(j, cx + k, cy - k, 1, 1, cor);
    }
}

void chip_desenhar(jan_t *j, chip_t *c)
{
    chip_ajustar(c);
    const mod_tema_t *t = &mod_tema;
    uint32_t fundo = c->fundo ? c->fundo : t->barra;
    g_retangulo(j, c->x, c->y, c->l, c->a, fundo);
    uint32_t rel = c->sel ? t->acento_suave : c->hover ? t->hover : t->superficie;
    g_caixa_red(j, c->x, c->y, c->l, c->a, c->a / 2, rel, c->sel ? t->acento : c->hover ? t->borde_forte : t->borde);
    int x = c->x + 9;
    if (c->cor) {
        g_retangulo_red(j, x, c->y + (c->a - 8) / 2, 8, 8, 4, c->cor);
        x += 14;
    }
    g_texto(j, x, c->y + (c->a - LETRA_A) / 2, tr(c->texto), c->sel ? t->acento : t->texto);
    if (c->fechavel) {
        int cx = c->x + c->l - 12, cy = c->y + c->a / 2;
        if (c->hover_x) g_retangulo_red(j, cx - 7, cy - 7, 14, 14, 7, t->press);
        cruz(j, cx, cy, c->hover_x ? t->texto : t->texto_fraco);
    }
    jan_mostrar_area(j, c->x, c->y, c->l, c->a);
    GR_REG(j, "chip", c, c->x, c->y, c->l, c->a, c->texto ? c->texto : "", c->l - 18, 0);
}

int chip_tratar(jan_t *j, chip_t *c, const evento_t *e)
{
    chip_ajustar(c);
    bool in = dentro(e->x, e->y, c->x, c->y, c->l, c->a);
    bool inx = in && c->fechavel && e->x >= c->x + c->l - 20;
    if (e->tipo == EV_MOVER) {
        if (in != c->hover || inx != c->hover_x) { c->hover = in; c->hover_x = inx; chip_desenhar(j, c); }
        return CHIP_NADA;
    }
    if (e->tipo == EV_CLIQUE && (e->botoes & 1) && in) return inx ? CHIP_FECHAR : CHIP_CLIQUE;
    return CHIP_NADA;
}

/* ------------------------------------------------------------ progreso e insignia */

void progresso_mod_desenhar(jan_t *j, int x, int y, int l, int a, int pct, unsigned flags, uint32_t fundo)
{
    const mod_tema_t *t = &mod_tema;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    if (fundo != MOD_FUNDO_NENHUM) g_retangulo(j, x, y, l, a, fundo ? fundo : t->superficie);
    g_retangulo_red(j, x, y, l, a, a / 2, t->pista);
    int w = l * pct / 100;
    if (pct > 0 && w < a) w = a;   /* un relleno más corto que el alto no se vería redondo */
    uint32_t cor = t->acento;
    if (flags & MP_USO) cor = pct >= 90 ? t->peligro : pct >= 75 ? t->aviso : t->acento;
    if (pct > 0) g_retangulo_red(j, x, y, w, a, a / 2, cor);
    jan_mostrar_area(j, x, y, l, a);
}

void progresso_mod_indet(jan_t *j, int x, int y, int l, int a, int fase, uint32_t fundo)
{
    const mod_tema_t *t = &mod_tema;
    g_retangulo(j, x, y, l, a, fundo ? fundo : t->superficie);
    g_retangulo_red(j, x, y, l, a, a / 2, t->pista);
    int bw = l / 3;
    int x0 = -bw + (l + bw) * (fase % 100) / 100, x1 = x0 + bw;
    if (x0 < 0) x0 = 0;
    if (x1 > l) x1 = l;
    if (x1 - x0 > 2) g_retangulo_red(j, x + x0, y, x1 - x0, a, (x1 - x0) < a ? (x1 - x0) / 2 : a / 2, t->acento);
    jan_mostrar_area(j, x, y, l, a);
}

int insignia_desenhar(jan_t *j, int x, int y, int n, uint32_t cor)
{
    char s[8];
    if (n > 99) copia_texto(s, "99+", sizeof s); else formatar_texto(s, sizeof s, "%d", n);
    int w = g_largura_texto(s) + 10;
    if (w < 18) w = 18;
    g_retangulo_red(j, x, y, w, 18, 9, cor ? cor : mod_tema.peligro);
    g_texto(j, x + (w - g_largura_texto(s)) / 2, y + 1, s, 0xFFFFFF);
    jan_mostrar_area(j, x, y, w, 18);
    return w;
}

/* ------------------------------------------------------------ campo */

#define CAMPO_PAD 10

static int campo_x_inicio(unsigned flags)
{
    if (flags & CAMPO_RECTO) return 5;
    return (flags & CAMPO_LUPA) ? CAMPO_PAD + MB_ICONO_L + 6 : CAMPO_PAD;
}

void campo_desenhar(jan_t *j, caixa_t *c, int a, const char *dica, unsigned flags, uint32_t fundo)
{
    const mod_tema_t *t = &mod_tema;
    if (a <= 0) a = t->altura;
    int r = (flags & CAMPO_RECTO) ? t->radio : a / 2;   /* píldora, o con el radio normal */
    g_retangulo(j, c->x - 2, c->y - 2, c->l + 4, a + 4, fundo ? fundo : t->barra);
    g_caixa_red(j, c->x, c->y, c->l, a, r, t->campo, c->foco ? t->acento : (flags & CAMPO_HOVER) ? t->borde_forte : t->borde);
    if (c->foco) g_aro_red(j, c->x - 1, c->y - 1, c->l + 2, a + 2, r + 1, 1, mod_mezclar(fundo ? fundo : t->barra, t->foco, 140));
    int tx = c->x + campo_x_inicio(flags);
    if (flags & CAMPO_LUPA) icone_desenhar(j, c->x + CAMPO_PAD, c->y + (a - MB_ICONO_L) / 2, "procurar", 1);
    int fin = c->x + c->l - ((flags & CAMPO_LIMPAR) ? CAMPO_PAD + 18 : CAMPO_PAD);
    int cabem = (fin - tx) / LETRA_L;
    if (cabem < 1) cabem = 1;
    int nt = (int)tam_texto(c->texto);
    if (c->cursor > nt) c->cursor = nt;
    if (c->cursor < 0) c->cursor = 0;
    int ini = c->cursor > cabem ? c->cursor - cabem : 0;
    int ty = c->y + (a - LETRA_A) / 2;
    if (nt == 0 && dica && dica[0]) g_texto_n(j, tx, ty, tr(dica), cabem, t->texto_fraco);
    else g_texto_n(j, tx, ty, c->texto + ini, cabem, t->texto);
    if (c->foco) g_retangulo(j, tx + (c->cursor - ini) * LETRA_L, ty, 1, LETRA_A, t->texto);
    if ((flags & CAMPO_LIMPAR) && nt > 0) {
        int cx = c->x + c->l - CAMPO_PAD - 7, cy = c->y + a / 2;
        if (flags & CAMPO_HOVER_X) g_retangulo_red(j, cx - 9, cy - 9, 18, 18, 9, t->press);
        cruz(j, cx, cy, (flags & CAMPO_HOVER_X) ? t->texto : t->texto_fraco);
    }
    jan_mostrar_area(j, c->x - 2, c->y - 2, c->l + 4, a + 4);
    GR_REG(j, "campo", c, c->x, c->y, c->l, a, "", 0, GRF_CONTROL);
}

bool campo_x_em(const caixa_t *c, int a, int x, int y)
{
    if (a <= 0) a = mod_tema.altura;
    return c->texto[0] && dentro(x, y, c->x + c->l - CAMPO_PAD - 7 - 10, c->y, 22, a);
}

void campo_busca_desenhar(jan_t *j, campo_busca_t *b)
{
    unsigned f = CAMPO_LUPA | CAMPO_LIMPAR | (b->hover ? CAMPO_HOVER : 0) | (b->hover_x ? CAMPO_HOVER_X : 0);
    campo_desenhar(j, &b->c, b->a, b->dica ? b->dica : "Buscar", f, b->fundo);
}

int campo_busca_tratar(jan_t *j, campo_busca_t *b, const evento_t *e)
{
    caixa_t *c = &b->c;
    if (b->a <= 0) b->a = mod_tema.altura;
    bool in = dentro(e->x, e->y, c->x, c->y, c->l, b->a);
    if (e->tipo == EV_MOVER) {
        bool x = in && campo_x_em(c, b->a, e->x, e->y);
        if (in != b->hover || x != b->hover_x) { b->hover = in; b->hover_x = x; campo_busca_desenhar(j, b); }
        return BUSCA_NADA;
    }
    if (e->tipo == EV_CLIQUE && (e->botoes & 1)) {
        bool estaba = c->foco;
        if (in && campo_x_em(c, b->a, e->x, e->y)) {
            c->texto[0] = 0; c->cursor = 0; c->foco = true; b->hover_x = false;
            campo_busca_desenhar(j, b);
            return BUSCA_LIMPOU;
        }
        c->foco = in;
        if (in) {
            int tx = c->x + campo_x_inicio(CAMPO_LUPA), n = (int)tam_texto(c->texto);
            int cabem = (c->x + c->l - CAMPO_PAD - 18 - tx) / LETRA_L;
            if (cabem < 1) cabem = 1;
            int ini = c->cursor > cabem ? c->cursor - cabem : 0;
            int col = ini + (e->x - tx + LETRA_L / 2) / LETRA_L;
            c->cursor = col < 0 ? 0 : col > n ? n : col;
        }
        if (in || estaba) campo_busca_desenhar(j, b);
        return BUSCA_NADA;
    }
    if (e->tipo != EV_TECLA || !c->foco) return BUSCA_NADA;
    int n = (int)tam_texto(c->texto);
    unsigned t = e->tecla;
    if (t == '\n') return BUSCA_ENTER;
    if (t == TECLA_ESC) {
        bool habia = n > 0;
        c->texto[0] = 0; c->cursor = 0; c->foco = false;
        campo_busca_desenhar(j, b);
        return habia ? BUSCA_LIMPOU : BUSCA_NADA;
    }
    if (t == '\b') {
        if (c->cursor > 0) { memmove(c->texto + c->cursor - 1, c->texto + c->cursor, (size_t)(n - c->cursor + 1)); c->cursor--; }
        else return BUSCA_NADA;
    } else if (t == TECLA_DELETE) {
        if (c->cursor < n) memmove(c->texto + c->cursor, c->texto + c->cursor + 1, (size_t)(n - c->cursor));
        else return BUSCA_NADA;
    } else if (t == TECLA_ESQUERDA) { if (c->cursor > 0) c->cursor--; }
    else if (t == TECLA_DIREITA) { if (c->cursor < n) c->cursor++; }
    else if (t == TECLA_HOME) c->cursor = 0;
    else if (t == TECLA_END) c->cursor = n;
    else if (t >= 0x20 && t < 0x100 && t != 0x7F && n < (int)sizeof c->texto - 1) {
        memmove(c->texto + c->cursor + 1, c->texto + c->cursor, (size_t)(n - c->cursor + 1));
        c->texto[c->cursor++] = (char)t;
    } else return BUSCA_NADA;
    campo_busca_desenhar(j, b);
    return (t == TECLA_ESQUERDA || t == TECLA_DIREITA || t == TECLA_HOME || t == TECLA_END) ? BUSCA_NADA : BUSCA_MUDOU;
}

/* ------------------------------------------------------------ tarjeta */

#define CARTAO_CAB 26

void cartao_interior(const cartao_t *c, int *x, int *y, int *l, int *a)
{
    int cab = c->titulo ? CARTAO_CAB : 0;
    *x = c->x + 10; *y = c->y + cab + (c->titulo ? 2 : 10); *l = c->l - 20; *a = c->a - cab - (c->titulo ? 2 : 10) - 8;
}

void cartao_desenhar(jan_t *j, cartao_t *c)
{
    const mod_tema_t *t = &mod_tema;
    uint32_t fundo = c->fundo ? c->fundo : t->barra;
    int r = t->radio_cartao;
    g_retangulo(j, c->x - 3, c->y - 3, c->l + 6, c->a + 7, fundo);
    g_sombra_red(j, c->x, c->y, c->l, c->a, r, c->hover || c->sel ? 2 : 1);
    g_caixa_red(j, c->x, c->y, c->l, c->a, r, t->superficie, c->sel ? t->acento : c->hover ? t->acento : t->borde);
    if (c->sel) g_aro_red(j, c->x + 1, c->y + 1, c->l - 2, c->a - 2, r - 1, 1, t->acento);
    if (c->titulo) {
        int x = c->x + 10;
        if (c->icone) { icone_desenhar(j, x, c->y + 5, c->icone, 1); x += MB_ICONO_L + 6; }
        g_texto_n(j, x, c->y + 5, tr(c->titulo), (c->x + c->l - 8 - x) / LETRA_L, t->texto);
    }
    jan_mostrar_area(j, c->x - 3, c->y - 3, c->l + 6, c->a + 7);
}

bool cartao_tratar(jan_t *j, cartao_t *c, const evento_t *e)
{
    bool in = dentro(e->x, e->y, c->x, c->y, c->l, c->a);
    if (e->tipo == EV_MOVER && in != c->hover) { c->hover = in; return false; }   /* quien la usa redibuja su contenido */
    if (e->tipo == EV_CLIQUE && (e->botoes & 1) && in) { (void)j; return true; }
    return false;
}

/* ------------------------------------------------------------ superposiciones: tooltip y toast */

static void guardar_area(jan_t *j, uint32_t *dst, int x, int y, int l, int a)
{
    for (int yy = 0; yy < a; yy++)
        for (int xx = 0; xx < l; xx++) {
            int px = x + xx, py = y + yy;
            dst[yy * l + xx] = (unsigned)px < (unsigned)j->larg && (unsigned)py < (unsigned)j->alt ? j->px[py * j->larg + px] : 0;
        }
}

static void restaurar_area(jan_t *j, const uint32_t *src, int x, int y, int l, int a)
{
    for (int yy = 0; yy < a; yy++)
        for (int xx = 0; xx < l; xx++) {
            int px = x + xx, py = y + yy;
            if ((unsigned)px < (unsigned)j->larg && (unsigned)py < (unsigned)j->alt) j->px[py * j->larg + px] = src[yy * l + xx];
        }
    jan_mostrar_area(j, x, y, l, a);
}

void dica_iniciar(dica_t *d)
{
    d->alvo = -1; d->texto[0] = 0; d->visivel = false; d->gasta = false; d->t0 = 0;
}

static void dica_apagar(jan_t *j, dica_t *d)
{
    if (!d->visivel) return;
    restaurar_area(j, d->salvo, d->rx, d->ry, d->rl, d->ra);
    d->visivel = false;
}

void dica_ocultar(jan_t *j, dica_t *d)
{
    dica_apagar(j, d);
    d->gasta = true;
}

void dica_mover(jan_t *j, dica_t *d, int alvo, const char *texto, int ex, int ey, int el, int ea)
{
    if (alvo == d->alvo) return;
    dica_apagar(j, d);
    d->alvo = alvo;
    d->gasta = false;
    d->t0 = tempo_ms();
    if (alvo >= 0) {
        copia_texto(d->texto, texto ? texto : "", sizeof d->texto);
        d->ex = ex; d->ey = ey; d->el = el; d->ea = ea;
    }
}

bool dica_tic(jan_t *j, dica_t *d)
{
    if (d->alvo < 0 || d->visivel || d->gasta || !d->texto[0]) return false;
    if (tempo_ms() - d->t0 < DICA_RETARDO) return false;
    const char *t = tr(d->texto);
    int tw = g_largura_texto(t), l = tw + 16;
    if (l > DICA_MAX_L) { l = DICA_MAX_L; tw = l - 16; }
    int a = DICA_ALT;
    int x = d->ex + (d->el - l) / 2, y = d->ey + d->ea + 6;
    if (x + l > j->larg - 4) x = j->larg - 4 - l;
    if (x < 4) x = 4;
    if (y + a > j->alt - 4) y = d->ey - a - 6;
    if (y < 0) y = 0;
    d->rx = x; d->ry = y; d->rl = l; d->ra = a;
    guardar_area(j, d->salvo, x, y, l, a);
    g_retangulo_red(j, x, y, l, a, 4, mod_tema.dica_fundo);
    g_texto_n(j, x + 8, y + (a - LETRA_A) / 2, t, tw / LETRA_L, mod_tema.dica_texto);
    jan_mostrar_area(j, x, y, l, a);
    d->visivel = true;
    d->gasta = true;
    return true;
}

void toast_iniciar(toast_t *t)
{
    t->visivel = false; t->texto[0] = 0; t->dur = 0; t->t0 = 0; t->tipo = TOAST_INFO;
}

void toast_ocultar(jan_t *j, toast_t *t)
{
    if (!t->visivel) return;
    restaurar_area(j, t->salvo, t->rx, t->ry, t->rl, t->ra);
    t->visivel = false;
}

void toast_mostrar(jan_t *j, toast_t *t, const char *texto, int tipo, unsigned dur_ms)
{
    toast_ocultar(j, t);
    copia_texto(t->texto, texto ? texto : "", sizeof t->texto);
    t->tipo = tipo; t->t0 = tempo_ms(); t->dur = dur_ms;
    const char *s = tr(t->texto);
    int tw = g_largura_texto(s), l = tw + 16 + 18 + 8;
    if (l > TOAST_MAX_L) { l = TOAST_MAX_L; tw = l - 16 - 18 - 8; }
    if (l > j->larg - 16) { l = j->larg - 16; tw = l - 16 - 18 - 8; }
    int a = TOAST_ALT, x = (j->larg - l) / 2, y = j->alt - a - 14;
    t->rx = x; t->ry = y; t->rl = l; t->ra = a;
    guardar_area(j, t->salvo, x, y, l, a);
    g_retangulo_red(j, x, y, l, a, a / 2, mod_tema.dica_fundo);
    uint32_t cor = tipo == TOAST_OK ? 0x3DD17F : tipo == TOAST_AVISO ? 0xF2B134 : tipo == TOAST_ERRO ? 0xFF6B60 : 0x3CC8CF;
    g_retangulo_red(j, x + 12, y + (a - 10) / 2, 10, 10, 5, cor);
    g_texto_n(j, x + 12 + 18, y + (a - LETRA_A) / 2, s, tw / LETRA_L, mod_tema.dica_texto);
    jan_mostrar_area(j, x, y, l, a);
    t->visivel = true;
}

bool toast_tic(jan_t *j, toast_t *t)
{
    if (!t->visivel || tempo_ms() - t->t0 < t->dur) return false;
    toast_ocultar(j, t);
    return true;
}
