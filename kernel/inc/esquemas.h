// SPDX-License-Identifier: GPL-3.0-only
/* esquemas.h - esquemas de apariencia de todo el sistema (v8.5.5.2, D428).
 *
 * Un esquema es una sola estructura (esquema_t) que comparten el kernel (marcos y títulos de ventana, menús,
 * barra de tareas, avisos, escritorio) y la libsagui (todos los widgets: COR_*, W98_* y mod_tema leen de ella).
 * Este fichero es solo cabecera: define la estructura, los doce esquemas incluidos y el generador que los
 * construye, así que lo incluyen igual el kernel (que guarda el esquema vigente) y la libsagui y Configuración
 * (que dibujan las vistas previas y editan los personalizados).
 *
 *   «SaguiOS» (id 0) es el aspecto de siempre, valor por valor: lo que se ve con él es idéntico a antes del
 *   cambio. Los demás salen de una paleta corta (esq_prim_t) de la que se derivan el resto de colores.
 *
 * Los colores son propios, inspirados en ambientes (arena, ladrillo, pizarra...), no copiados de ningún tema. */
#ifndef SAGUI_ESQUEMAS_H
#define SAGUI_ESQUEMAS_H
/* Quien lo incluye ya trae uint32_t (tipos.h en el kernel, <stdint.h> en el userland). */

/* ------------------------------------------------------------ la estructura */

/* Todos los colores del esquema (0xRRGGBB), en un solo sitio. La macro sirve para declararlos y para recorrerlos. */
#define ESQ_CAMPOS(X) \
    /* escritorio */ \
    X(escritorio_a) X(escritorio_b) X(escritorio_texto) \
    /* ventanas (las pinta el kernel) */ \
    X(titulo_a1) X(titulo_a2) X(titulo_a_texto) X(titulo_i1) X(titulo_i2) X(titulo_i_texto) \
    X(marco_a) X(marco_i) X(contorno) X(cerrar_a) X(cerrar_i) X(cerrar_x) X(boton_a) X(boton_i) \
    /* ventana, texto, controles */ \
    X(fundo) X(texto) X(texto_fraco) X(texto_dis) X(borda) X(botao) X(botao_apertado) X(destaque) X(selecao) \
    X(selecao_texto) X(sel_suave) X(borda_forte) X(cab_fundo) X(cab_linha) \
    X(ar_cab) X(ar_linha) X(ar_atual_sel) X(ar_atual) X(ar_inicio) X(ar_previa) \
    /* controles clásicos (biseles) */ \
    X(cara) X(luz) X(sombra) X(oscuro) X(azul) X(negro) \
    /* campos y listas */ \
    X(campo_fundo) X(campo_texto) X(lista_fundo) X(lista_texto) \
    /* menús */ \
    X(menu_fundo) X(menu_texto) X(menu_sel) X(menu_sel_texto) X(menu_borde) X(menu_sep) X(menu_atalho) \
    /* barra de tareas, menú Inicio, acento y foco */ \
    X(barra) X(barra_texto) X(inicio_a) X(inicio_b) X(acento) X(foco) \
    /* información (tooltip clásico), aviso y panel del kernel, barras de desplazamiento */ \
    X(info_fundo) X(info_texto) X(info_borde) X(painel_fundo) X(painel_borde) X(painel_texto) \
    X(scroll_pista) X(scroll_polegar) \
    /* la paleta de los componentes modernos (mod_tema) */ \
    X(m_acento_hover) X(m_acento_press) X(m_acento_suave) X(m_sobre_acento) X(m_superficie) X(m_barra) \
    X(m_pista) X(m_borde) X(m_borde_forte) X(m_hover) X(m_press) X(m_texto_fraco) X(m_desab_fundo) \
    X(m_desab_texto) X(m_peligro) X(m_peligro_suave) X(m_aviso) X(m_ok) X(m_dica_fundo) X(m_dica_texto)

#define ESQ_DECL(n) uint32_t n;
#define ESQ_CONTAR(n) +1
#define ESQ_NC (0 ESQ_CAMPOS(ESQ_CONTAR))
#define ESQ_NOME_MAX 28

/* Los esquemas incluidos y el id de los personalizados. */
enum {
    ESQ_SAGUIOS, ESQ_CLASICO, ESQ_DESIERTO, ESQ_BERENJENA, ESQ_PIZARRA, ESQ_MARINO, ESQ_LADRILLO, ESQ_LLUVIA,
    ESQ_BOSQUE, ESQ_HC_NEGRO, ESQ_HC_BLANCO, ESQ_OSCURO, ESQ_ARCE, ESQ_LAVANDA, ESQ_OCEANO, ESQ_CEREZA, ESQ_GRAFITO,
    ESQ_NORDICO, ESQ_TERMINAL, ESQ_SOLAR_CLARO, ESQ_SOLAR_OSCURO, ESQ_HC_AMARILLO, ESQ_QTD,
    ESQ_PERSONAL = 100
};

typedef struct {
    char     nome[ESQ_NOME_MAX];
    uint32_t id;              /* ESQ_* o ESQ_PERSONAL */
    uint32_t base;            /* el incluido del que parte (para los personalizados) */
    uint32_t escuro;          /* 1 = fondos oscuros: los iconos de trazo oscuro se aclaran */
    uint32_t barra_propia;    /* 1 = la barra de tareas usa los colores del esquema; 0 = el estilo elegido en Configuración */
    uint32_t escritorio_liso; /* 1 = degradado de dos colores en vez del tricolor de SaguiOS */
    uint32_t fonte;           /* fuente del sistema: 0 SaguiOS, 1 Terminus, 2 Terminus negrita, 3 Fixed (D428; no cambia con el esquema) */
    uint32_t versao;          /* sube en cada cambio (lo pone el kernel) */
    union {
        struct { ESQ_CAMPOS(ESQ_DECL) };
        uint32_t v[ESQ_NC];
    };
} esquema_t;

/* SIS_ESQUEMA (abi.h): operaciones */
#define ESQ_OP_LER         0   /* copia el vigente a (ptr, n); devuelve su versión */
#define ESQ_OP_VERSAO      1   /* solo la versión (barata: la libsagui la mira para saber si recargar) */
#define ESQ_OP_FIJAR       2   /* (ptr, n): lo pone, redibuja todo y avisa a las ventanas (EV_TEMA) */
#define ESQ_OP_FIJAR_LOGIN 3   /* (ptr, n): como FIJAR y además es el de la pantalla de inicio de sesión (solo root) */

/* ------------------------------------------------------------ mezclas */

static inline uint32_t esq_mix(uint32_t a, uint32_t b, int pct)   /* a + (b - a) * pct / 100, por canal */
{
    uint32_t r = 0;
    for (int s = 0; s <= 16; s += 8) {
        int x = (int)((a >> s) & 255), y = (int)((b >> s) & 255);
        r |= (uint32_t)(x + (y - x) * pct / 100) << s;
    }
    return r;
}

/* Luminosidad aproximada 0-255 (para decidir texto claro u oscuro). */
static inline int esq_lum(uint32_t c)
{
    return (int)((((c >> 16) & 255) * 299 + ((c >> 8) & 255) * 587 + (c & 255) * 114) / 1000);
}

/* ------------------------------------------------------------ SaguiOS: el aspecto de siempre, valor por valor */

static inline void esq_saguios(esquema_t *e, uint32_t acento)
{
    *e = (esquema_t){ .nome = "SaguiOS", .id = ESQ_SAGUIOS, .base = ESQ_SAGUIOS, .escuro = 0, .barra_propia = 0, .escritorio_liso = 0 };
    e->escritorio_a = 0x0B7A3E; e->escritorio_b = 0x14337E; e->escritorio_texto = 0xFFFFFF;
    e->titulo_a1 = e->titulo_a2 = e->marco_a = acento;
    e->titulo_a_texto = 0xFFFFFF;
    e->titulo_i1 = e->titulo_i2 = e->marco_i = 0x5A6470;
    e->titulo_i_texto = 0xD0D4D8;
    e->contorno = 0x101010; e->cerrar_a = 0xD03C32; e->cerrar_i = 0x8A5A56; e->cerrar_x = 0xFFFFFF;
    e->boton_a = esq_mix(esq_mix(acento, 0, 25), 0xFFFFFF, 25); e->boton_i = 0x6E7680;   /* maximizar y minimizar */
    e->fundo = 0xF4F4F0; e->texto = 0x1E2328; e->texto_fraco = 0x6A737C; e->texto_dis = 0x9AA3AC; e->borda = 0x8A939C;
    e->botao = 0xE2E6EA; e->botao_apertado = 0xC4CBD2; e->destaque = 0x0B7A3E; e->selecao = 0x2A6FD8; e->selecao_texto = 0xFFFFFF;
    e->sel_suave = 0xB9D3F2; e->borda_forte = 0x5A6470; e->cab_fundo = 0xE6E9EC; e->cab_linha = 0xB8BFC6;
    e->ar_cab = 0xEDF1F5; e->ar_linha = 0xC8D0D8; e->ar_atual_sel = 0x1E5AA8; e->ar_atual = 0xE7EEF8; e->ar_inicio = 0xF1F5F6; e->ar_previa = 0xF6F7F9;   /* Archivos */
    e->cara = 0xC0C0C0; e->luz = 0xFFFFFF; e->sombra = 0x808080; e->oscuro = 0x404040; e->azul = 0x000080; e->negro = 0x000000;
    e->campo_fundo = 0xFFFFFF; e->campo_texto = 0x1E2328; e->lista_fundo = 0xFFFFFF; e->lista_texto = 0x1E2328;
    e->menu_fundo = 0xF4F4F0; e->menu_texto = 0x1E2328; e->menu_sel = 0x0B7A3E; e->menu_sel_texto = 0xFFFFFF;
    e->menu_borde = 0x101010; e->menu_sep = 0xC4CBD2; e->menu_atalho = 0x6A737C;
    e->barra = 0x18202A; e->barra_texto = 0xD8DEE4; e->inicio_a = 0x2A5FC8; e->inicio_b = 0x1B3F94;
    e->acento = acento; e->foco = 0x0A8F96;
    e->info_fundo = 0xFFFFE0; e->info_texto = 0x101010; e->info_borde = 0x404040;
    e->painel_fundo = 0x18202A; e->painel_borde = 0xFFDF4A; e->painel_texto = 0xFFFFFF;
    e->scroll_pista = 0xE4E7EA; e->scroll_polegar = 0x9AA3AC;
    e->m_acento_hover = 0x0E9048; e->m_acento_press = 0x08652F; e->m_acento_suave = 0xD9EEE0; e->m_sobre_acento = 0xFFFFFF;
    e->m_superficie = 0xFFFFFF; e->m_barra = 0xEEF2F3; e->m_pista = 0xD3DBDF; e->m_borde = 0xC2CBD0; e->m_borde_forte = 0x8E9BA3;
    e->m_hover = 0xDDE5E8; e->m_press = 0xCAD4D8; e->m_texto_fraco = 0x5B666F; e->m_desab_fundo = 0xE6EAEC;
    e->m_desab_texto = 0x98A2A9; e->m_peligro = 0xB3261E; e->m_peligro_suave = 0xF7DEDC; e->m_aviso = 0xD98A00; e->m_ok = 0x0B7A3E;
    e->m_dica_fundo = 0x263238; e->m_dica_texto = 0xFFFFFF;
}

/* ------------------------------------------------------------ los demás: de una paleta corta al esquema entero */

typedef struct {
    const char *nome;
    int escuro, hc;            /* hc: alto contraste (texto deshabilitado y bordes más marcados) */
    uint32_t esc_a, esc_b, esc_texto;
    uint32_t ta1, ta2, ta_txt, ti1, ti2, ti_txt;
    uint32_t fundo, texto, contorno;
    uint32_t campo, campo_txt;
    uint32_t sel, sel_txt;
    uint32_t menu, menu_txt, menu_sel, menu_sel_txt;
    uint32_t barra, barra_txt;
    uint32_t acento;
    uint32_t info, info_txt;
} esq_prim_t;

/* Deriva el esquema entero a partir de sus colores primarios. */
static inline void esq_de_prim(esquema_t *e, const esq_prim_t *p, int id)
{
    *e = (esquema_t){ .id = (uint32_t)id, .base = (uint32_t)id, .escuro = (uint32_t)p->escuro, .barra_propia = 1, .escritorio_liso = 1 };
    for (int i = 0; i < ESQ_NOME_MAX - 1 && p->nome[i]; i++) e->nome[i] = p->nome[i];
    uint32_t F = p->fundo, T = p->texto;
    int claro = !p->escuro;
    e->escritorio_a = p->esc_a; e->escritorio_b = p->esc_b; e->escritorio_texto = p->esc_texto;
    e->titulo_a1 = p->ta1; e->titulo_a2 = p->ta2; e->titulo_a_texto = p->ta_txt;
    e->titulo_i1 = p->ti1; e->titulo_i2 = p->ti2; e->titulo_i_texto = p->ti_txt;
    e->marco_a = p->ta1; e->marco_i = p->ti1; e->contorno = p->contorno;
    e->cerrar_a = p->hc ? 0xA00000 : (claro ? 0xD03C32 : 0xC8443B); e->cerrar_i = claro ? 0x8A5A56 : 0x6E4A48; e->cerrar_x = 0xFFFFFF;
    e->boton_a = esq_mix(esq_mix(p->ta2, 0, 25), 0xFFFFFF, 25); e->boton_i = esq_mix(p->ti1, p->ti_txt, 25);
    e->fundo = F; e->texto = T;
    e->texto_fraco = esq_mix(T, F, p->hc ? 20 : 30);
    e->texto_dis = esq_mix(T, F, p->hc ? 35 : 54);
    e->borda = esq_mix(F, T, p->hc ? 90 : 45);
    e->botao = esq_mix(F, T, claro ? 7 : 12);
    e->botao_apertado = esq_mix(F, T, claro ? 20 : 26);
    e->destaque = p->acento; e->selecao = p->sel; e->selecao_texto = p->sel_txt;
    e->cara = F;
    e->luz = claro ? esq_mix(F, 0xFFFFFF, 80) : esq_mix(F, 0xFFFFFF, 22);
    e->sombra = claro ? esq_mix(F, 0, 33) : esq_mix(F, 0, 50);
    e->oscuro = claro ? esq_mix(F, 0, 67) : esq_mix(F, 0, 80);
    e->azul = p->sel; e->negro = T;
    e->sel_suave = esq_mix(p->campo, p->sel, 30); e->borda_forte = esq_mix(F, T, p->hc ? 100 : 70);
    e->cab_fundo = esq_mix(F, T, 6); e->cab_linha = esq_mix(F, T, 28);
    e->ar_cab = esq_mix(p->campo, T, 5); e->ar_linha = esq_mix(F, T, 22); e->ar_atual_sel = p->hc ? p->sel : esq_mix(p->sel, 0, 25);
    e->ar_atual = esq_mix(p->campo, p->sel, 14); e->ar_inicio = esq_mix(F, T, 3); e->ar_previa = esq_mix(p->campo, T, 3);
    e->campo_fundo = p->campo; e->campo_texto = p->campo_txt; e->lista_fundo = p->campo; e->lista_texto = p->campo_txt;
    e->menu_fundo = p->menu; e->menu_texto = p->menu_txt; e->menu_sel = p->menu_sel; e->menu_sel_texto = p->menu_sel_txt;
    e->menu_borde = p->contorno; e->menu_sep = esq_mix(p->menu, p->menu_txt, p->hc ? 60 : 22);
    e->menu_atalho = esq_mix(p->menu_txt, p->menu, p->hc ? 20 : 30);
    e->barra = p->barra; e->barra_texto = p->barra_txt;
    e->inicio_a = p->ta2; e->inicio_b = p->ta1;
    e->acento = p->acento;
    e->foco = claro ? esq_mix(p->acento, 0, 10) : esq_mix(p->acento, 0xFFFFFF, 35);
    if (p->hc) e->foco = claro ? 0x0000FF : 0x00E0FF;
    e->info_fundo = p->info; e->info_texto = p->info_txt; e->info_borde = p->hc ? p->info_txt : esq_mix(p->info, p->info_txt, 60);
    e->painel_fundo = p->barra; e->painel_borde = p->hc ? p->acento : 0xFFDF4A; e->painel_texto = p->barra_txt;
    if (p->hc && !claro) e->painel_borde = 0xFFFF00;
    e->scroll_pista = esq_mix(F, T, 8); e->scroll_polegar = esq_mix(F, T, p->hc ? 80 : 38);
    e->m_acento_hover = esq_mix(p->acento, 0xFFFFFF, 12); e->m_acento_press = esq_mix(p->acento, 0, 20);
    e->m_acento_suave = esq_mix(F, p->acento, 22);
    e->m_sobre_acento = esq_lum(p->acento) > 120 ? 0x000000 : 0xFFFFFF;
    e->m_superficie = claro ? esq_mix(F, 0xFFFFFF, 60) : esq_mix(F, 0xFFFFFF, 5);
    e->m_barra = esq_mix(F, T, 4);
    e->m_pista = esq_mix(F, T, 14); e->m_borde = esq_mix(F, T, p->hc ? 90 : 24); e->m_borde_forte = esq_mix(F, T, p->hc ? 100 : 48);
    e->m_hover = esq_mix(F, T, 12); e->m_press = esq_mix(F, T, 22);
    e->m_texto_fraco = e->texto_fraco; e->m_desab_fundo = esq_mix(F, T, 8); e->m_desab_texto = e->texto_dis;
    e->m_peligro = claro ? 0xB3261E : 0xF2766E; e->m_peligro_suave = esq_mix(F, e->m_peligro, 18);
    e->m_aviso = claro ? 0xD98A00 : 0xF0B040; e->m_ok = claro ? 0x1E7A3E : 0x5CD08A;
    e->m_dica_fundo = claro ? esq_mix(T, F, 12) : esq_mix(F, 0xFFFFFF, 28);
    e->m_dica_texto = claro ? 0xFFFFFF : T;
    if (p->hc) { e->m_dica_fundo = p->info; e->m_dica_texto = p->info_txt; }
}

/* La paleta de cada esquema incluido (del 1 al 11). Los títulos con texto blanco dan al menos 4,5:1; en «Lluvia» los
 * títulos son claros y llevan texto oscuro. */
static const esq_prim_t esq_prims[ESQ_QTD] = {
    [ESQ_CLASICO] = { "SaguiOS clásico", 0, 0, 0x008080, 0x008080, 0xFFFFFF,
        0x000080, 0x0F6FB8, 0xFFFFFF, 0x6E6E6E, 0x8A8A8A, 0xFFFFFF,
        0xC0C0C0, 0x000000, 0x000000, 0xFFFFFF, 0x000000, 0x000080, 0xFFFFFF,
        0xC0C0C0, 0x000000, 0x000080, 0xFFFFFF, 0xC0C0C0, 0x000000, 0x000080, 0xFFFFE1, 0x000000 },
    [ESQ_DESIERTO] = { "Desierto", 0, 0, 0xD9BF8A, 0xC2995A, 0x2B1D0E,
        0x5B3A1E, 0x8E5E2A, 0xFFF6E0, 0x6F5F4A, 0x8A7A62, 0xFFF6E0,
        0xE9DCC0, 0x2B1D0E, 0x3B2812, 0xFFF9EA, 0x2B1D0E, 0x8A4B1F, 0xFFFFFF,
        0xEFE2C4, 0x2B1D0E, 0x7A4220, 0xFFFFFF, 0x4A2F18, 0xF3E6C8, 0x9A5A26, 0xFFF3C2, 0x2B1D0E },
    [ESQ_BERENJENA] = { "Berenjena", 0, 0, 0x4A2358, 0x2A1233, 0xF3E8F8,
        0x3B1647, 0x7A3A8C, 0xFFFFFF, 0x6B5A72, 0x7E6E86, 0xF8F0FB,
        0xE9E1EE, 0x24122B, 0x1A0B20, 0xFAF6FC, 0x24122B, 0x5B2A70, 0xFFFFFF,
        0xEFE8F3, 0x24122B, 0x5B2A70, 0xFFFFFF, 0x2A1233, 0xEADCF0, 0x8E44AD, 0xFFF4C8, 0x24122B },
    [ESQ_PIZARRA] = { "Pizarra", 0, 0, 0x5C6F82, 0x3E4C5B, 0xFFFFFF,
        0x2F3E4E, 0x56697C, 0xFFFFFF, 0x5F6B76, 0x76828D, 0xFFFFFF,
        0xD8DEE4, 0x1B2530, 0x161E27, 0xF4F6F8, 0x1B2530, 0x3B5B7A, 0xFFFFFF,
        0xE0E5EA, 0x1B2530, 0x3B5B7A, 0xFFFFFF, 0x232E3A, 0xDDE4EA, 0x4A6C8C, 0xFFFBD0, 0x1B2530 },
    [ESQ_MARINO] = { "Marino", 0, 0, 0x1F4E96, 0x0B2447, 0xFFFFFF,
        0x0F2D5C, 0x2C5AA0, 0xFFFFFF, 0x596A83, 0x70829C, 0xFFFFFF,
        0xDCE6F2, 0x0E1A2B, 0x0A1424, 0xF6F9FD, 0x0E1A2B, 0x1F4E96, 0xFFFFFF,
        0xE4ECF6, 0x0E1A2B, 0x1F4E96, 0xFFFFFF, 0x0B1D3A, 0xDCE6F5, 0x2A6FD8, 0xFFFBD0, 0x0E1A2B },
    [ESQ_LADRILLO] = { "Ladrillo", 0, 0, 0xB4533C, 0x7C2E20, 0xFFFFFF,
        0x7A2218, 0xB14A32, 0xFFFFFF, 0x735E58, 0x8E7A74, 0xFFFFFF,
        0xEFE0D8, 0x2A1410, 0x24100C, 0xFFF8F4, 0x2A1410, 0x9A3322, 0xFFFFFF,
        0xF3E6DE, 0x2A1410, 0x9A3322, 0xFFFFFF, 0x3A1812, 0xF3DED6, 0xB5432B, 0xFFF3C2, 0x2A1410 },
    [ESQ_LLUVIA] = { "Lluvia", 0, 0, 0xA9BDCB, 0x7F98AB, 0x10222F,
        0x7FA0BA, 0xB7D0E2, 0x10222F, 0xC5D0D8, 0xDDE5EB, 0x41515D,
        0xE6EDF2, 0x1A2630, 0x2B3A46, 0xF8FBFD, 0x1A2630, 0x4F7391, 0xFFFFFF,
        0xEDF2F6, 0x1A2630, 0x4F7391, 0xFFFFFF, 0x3E5568, 0xEAF2F7, 0x4F7391, 0xFFFFE6, 0x1A2630 },
    [ESQ_BOSQUE] = { "Bosque", 0, 0, 0x2E5E3A, 0x15301D, 0xF0F8EC,
        0x1E4D2B, 0x3B7A4A, 0xF4FFF4, 0x5E6F60, 0x78897A, 0xFFFFFF,
        0xE4EBDD, 0x17261A, 0x101C12, 0xF8FBF4, 0x17261A, 0x2F6B3C, 0xFFFFFF,
        0xEAF0E4, 0x17261A, 0x2F6B3C, 0xFFFFFF, 0x16301D, 0xE0ECDC, 0x2F7D43, 0xFFFBD0, 0x17261A },
    [ESQ_HC_NEGRO] = { "Alto contraste negro", 1, 1, 0x000000, 0x000000, 0xFFFFFF,
        0x0000C0, 0x0000C0, 0xFFFFFF, 0x404040, 0x404040, 0xFFFFFF,
        0x000000, 0xFFFFFF, 0xFFFFFF, 0x000000, 0xFFFFFF, 0xFFFF00, 0x000000,
        0x000000, 0xFFFFFF, 0xFFFF00, 0x000000, 0x000000, 0xFFFFFF, 0x0000C0, 0xFFFF00, 0x000000 },
    [ESQ_HC_BLANCO] = { "Alto contraste blanco", 0, 1, 0xFFFFFF, 0xFFFFFF, 0x000000,
        0x000080, 0x000080, 0xFFFFFF, 0xC0C0C0, 0xC0C0C0, 0x000000,
        0xFFFFFF, 0x000000, 0x000000, 0xFFFFFF, 0x000000, 0x000080, 0xFFFFFF,
        0xFFFFFF, 0x000000, 0x000080, 0xFFFFFF, 0xFFFFFF, 0x000000, 0x000080, 0xFFFFFF, 0x000000 },
    [ESQ_OSCURO] = { "Oscuro", 1, 0, 0x20262E, 0x0E1115, 0xE6E9EC,
        0x0A5F32, 0x0B7A3E, 0xFFFFFF, 0x3A4048, 0x4A515A, 0xD4D9DE,
        0x2B2F36, 0xE6E9EC, 0x0A0C0F, 0x1C1F24, 0xE6E9EC, 0x2F6FD0, 0xFFFFFF,
        0x2B2F36, 0xE6E9EC, 0x2F6FD0, 0xFFFFFF, 0x14181D, 0xD8DEE4, 0x11803F, 0x4A525C, 0xFFFFFF },
    [ESQ_ARCE] = { "Arce", 0, 0, 0xA8501A, 0x7A2E10, 0xFFF6E8,
        0x8A3A0E, 0xB4561A, 0xFFFFFF, 0x7A6A5E, 0x8C7C70, 0xFFFFFF,
        0xF4E6D2, 0x2E1A0C, 0x2A1608, 0xFFF8EE, 0x2E1A0C, 0xA8420F, 0xFFFFFF,
        0xF8ECDA, 0x2E1A0C, 0x9C3A0C, 0xFFFFFF, 0x3E1C0A, 0xF6E2C6, 0xB4561A, 0xFFF1B8, 0x2E1A0C },
    [ESQ_LAVANDA] = { "Lavanda", 0, 0, 0xC9B8E8, 0x9E86CC, 0x1E1238,
        0x5B45A0, 0x7458BC, 0xFFFFFF, 0x7A708F, 0x8A80A8, 0xFFFFFF,
        0xEFEAF8, 0x221A3A, 0x2A1E48, 0xFBF9FF, 0x221A3A, 0x5B45A0, 0xFFFFFF,
        0xF3EFFB, 0x221A3A, 0x5B45A0, 0xFFFFFF, 0x2E2150, 0xE6DFF8, 0x6C4FC0, 0xFFF6C8, 0x221A3A },
    [ESQ_OCEANO] = { "Océano", 0, 0, 0x1E7FA8, 0x0A2E58, 0xFFFFFF,
        0x0A4A78, 0x177AA8, 0xFFFFFF, 0x56707F, 0x6C8498, 0xFFFFFF,
        0xE2F1F8, 0x0C2230, 0x0A1C2A, 0xF8FCFE, 0x0C2230, 0x0E6A9A, 0xFFFFFF,
        0xEAF5FA, 0x0C2230, 0x0E6A9A, 0xFFFFFF, 0x072A44, 0xD8EEF8, 0x0E7AAA, 0xFFF8D0, 0x0C2230 },
    [ESQ_CEREZA] = { "Cereza", 0, 0, 0xB83A5E, 0x8A1E3E, 0xFFFFFF,
        0x8E1A3E, 0xC0345C, 0xFFFFFF, 0x7C5C66, 0x8E6E78, 0xFFFFFF,
        0xFBEAF0, 0x3A1020, 0x300C1A, 0xFFF8FA, 0x3A1020, 0xA82048, 0xFFFFFF,
        0xFDEFF4, 0x3A1020, 0xA82048, 0xFFFFFF, 0x4A1428, 0xFBDDE6, 0xB02050, 0xFFF4C8, 0x3A1020 },
    [ESQ_GRAFITO] = { "Grafito", 1, 0, 0x2A2D32, 0x121316, 0xECECEC,
        0xA84A06, 0xB85408, 0xFFFFFF, 0x44474E, 0x54575F, 0xD8DADE,
        0x2C2E33, 0xECECEC, 0x0C0D0F, 0x1E2024, 0xECECEC, 0xF28A2E, 0x000000,
        0x2C2E33, 0xECECEC, 0xF28A2E, 0x000000, 0x17181B, 0xE4E4E4, 0xE8821E, 0x4A4E56, 0xFFFFFF },
    [ESQ_NORDICO] = { "Nórdico", 0, 0, 0xB8C8D8, 0x8CA0B8, 0x1C2836,
        0x4C6A8A, 0x56789A, 0xFFFFFF, 0xC0CAD6, 0xD4DCE6, 0x3A4858,
        0xECEFF4, 0x2E3440, 0x2E3440, 0xFFFFFF, 0x2E3440, 0x4C6E98, 0xFFFFFF,
        0xECEFF4, 0x2E3440, 0x4C6E98, 0xFFFFFF, 0x2E3440, 0xD8DEE9, 0x4C6E98, 0xFFF3CC, 0x2E3440 },
    [ESQ_TERMINAL] = { "Terminal", 1, 0, 0x000000, 0x001400, 0x33FF66,
        0x0A3A12, 0x145A22, 0x9CFFB4, 0x0A1A0E, 0x102014, 0x4CC468,
        0x04140A, 0x3CF070, 0x1E7A32, 0x000000, 0x5CFF8A, 0x33FF66, 0x001A08,
        0x04140A, 0x3CF070, 0x33FF66, 0x001A08, 0x000000, 0x3CF070, 0x1FB84A, 0x0A2A12, 0x9CFFB4 },
    [ESQ_SOLAR_CLARO] = { "Solarizado claro", 0, 0, 0xEFE6CC, 0xD8CCA8, 0x2C3C42,
        0x1F6F8B, 0x2A7E98, 0xFFFFFF, 0xC8C2AA, 0xD8D2BA, 0x4A5A60,
        0xFBF3DC, 0x26363C, 0x26363C, 0xFFFBEA, 0x26363C, 0x2A7E98, 0xFFFFFF,
        0xFBF3DC, 0x26363C, 0x2A7E98, 0xFFFFFF, 0x0F3A48, 0xE0E8E4, 0xC08A00, 0xFFF1B0, 0x26363C },
    [ESQ_SOLAR_OSCURO] = { "Solarizado oscuro", 1, 0, 0x0A3440, 0x052029, 0xD0DCDC,
        0x0F5A70, 0x17728C, 0xFFFFFF, 0x1E4552, 0x2A5666, 0xC8D6D8,
        0x0B2F3A, 0xB8C7C8, 0x03181F, 0x06242E, 0xD0DCDC, 0x187090, 0xFFFFFF,
        0x0B2F3A, 0xB8C7C8, 0x187090, 0xFFFFFF, 0x041E27, 0xC8D6D8, 0xC79A00, 0x17404E, 0xFFFFFF },
    [ESQ_HC_AMARILLO] = { "Alto contraste amarillo", 1, 1, 0x000000, 0x000000, 0xFFFF00,
        0xFFFF00, 0xFFFF00, 0x000000, 0x303030, 0x303030, 0xFFFF80,
        0x000000, 0xFFFF00, 0xFFFF00, 0x000000, 0xFFFF00, 0x00FFFF, 0x000000,
        0x000000, 0xFFFF00, 0xFFFF00, 0x000000, 0x000000, 0xFFFF00, 0xFFFF00, 0xFFFF00, 0x000000 },
};

/* Nombre corto para los ficheros de configuración (en minúsculas, sin espacios). */
static const char *const esq_claves[ESQ_QTD] = {
    "saguios", "clasico", "desierto", "berenjena", "pizarra", "marino", "ladrillo", "lluvia", "bosque",
    "alto-contraste-negro", "alto-contraste-blanco", "oscuro", "arce", "lavanda", "oceano", "cereza", "grafito", "nordico",
    "terminal", "solarizado-claro", "solarizado-oscuro", "alto-contraste-amarillo",
};

/* Construye el esquema incluido `id` (fuera de rango: SaguiOS). El acento solo cuenta en «SaguiOS»; en los demás
 * se pasa 0xFFFFFFFF para dejar el suyo. */
static inline void esq_incluido(esquema_t *e, int id, uint32_t acento)
{
    if (id <= 0 || id >= ESQ_QTD) { esq_saguios(e, (acento & 0xFF000000u) ? 0x0B7A3E : acento); return; }
    esq_de_prim(e, &esq_prims[id], id);
    if (!(acento & 0xFF000000u)) {
        e->acento = e->destaque = acento;
        e->titulo_a1 = e->marco_a = esq_mix(acento, 0, 15); e->titulo_a2 = acento;
    }
}

/* ------------------------------------------------------------ edición de los personalizados */

/* Los elementos que se pueden elegir y cambiar de color (el modo «Personalizado» de Configuración). */
enum {
    EE_ESC_A, EE_ESC_B, EE_TIT_A1, EE_TIT_A2, EE_TIT_A_TXT, EE_TIT_I1, EE_TIT_I2, EE_TIT_I_TXT, EE_MARCO, EE_VENTANA,
    EE_TEXTO, EE_CONTROL, EE_CAMPO, EE_CAMPO_TXT, EE_SELECCION, EE_SEL_TXT, EE_MENU, EE_MENU_TXT, EE_MENU_SEL, EE_BARRA,
    EE_BARRA_TXT, EE_ACENTO, EE_INFO, EE_INFO_TXT, EE_TXT_DIS, EE_SCROLL, EE_QTD
};
static const char *const esq_elementos[EE_QTD] = {
    "Escritorio (arriba)", "Escritorio (abajo)", "Título activo (color 1)", "Título activo (color 2)", "Texto del título activo",
    "Título inactivo (color 1)", "Título inactivo (color 2)", "Texto del título inactivo", "Marco de la ventana", "Ventana",
    "Texto", "Controles", "Campo y lista", "Texto del campo", "Selección", "Texto de la selección", "Menú", "Texto del menú",
    "Resaltado del menú", "Barra de tareas", "Texto de la barra", "Acento y foco", "Información (tooltip)", "Texto de información",
    "Texto deshabilitado", "Barra de desplazamiento",
};

static inline uint32_t esq_dame(const esquema_t *e, int el)
{
    switch (el) {
    case EE_ESC_A: return e->escritorio_a;   case EE_ESC_B: return e->escritorio_b;
    case EE_TIT_A1: return e->titulo_a1;     case EE_TIT_A2: return e->titulo_a2;    case EE_TIT_A_TXT: return e->titulo_a_texto;
    case EE_TIT_I1: return e->titulo_i1;     case EE_TIT_I2: return e->titulo_i2;    case EE_TIT_I_TXT: return e->titulo_i_texto;
    case EE_MARCO: return e->marco_a;        case EE_VENTANA: return e->fundo;       case EE_TEXTO: return e->texto;
    case EE_CONTROL: return e->botao;        case EE_CAMPO: return e->campo_fundo;   case EE_CAMPO_TXT: return e->campo_texto;
    case EE_SELECCION: return e->selecao;    case EE_SEL_TXT: return e->selecao_texto;
    case EE_MENU: return e->menu_fundo;      case EE_MENU_TXT: return e->menu_texto; case EE_MENU_SEL: return e->menu_sel;
    case EE_BARRA: return e->barra;          case EE_BARRA_TXT: return e->barra_texto;
    case EE_ACENTO: return e->acento;        case EE_INFO: return e->info_fundo;     case EE_INFO_TXT: return e->info_texto;
    case EE_TXT_DIS: return e->texto_dis;    case EE_SCROLL: return e->scroll_polegar;
    }
    return 0;
}

/* Cambia un elemento y lo que depende de él (derivados de la misma cara). El esquema pasa a ser personalizado. */
static inline void esq_poner(esquema_t *e, int el, uint32_t c)
{
    c &= 0xFFFFFF;
    switch (el) {
    case EE_ESC_A: e->escritorio_a = c; break;
    case EE_ESC_B: e->escritorio_b = c; break;
    case EE_TIT_A1: e->titulo_a1 = e->marco_a = c; e->inicio_b = c; break;
    case EE_TIT_A2: e->titulo_a2 = c; e->inicio_a = c; e->boton_a = esq_mix(esq_mix(c, 0, 25), 0xFFFFFF, 25); break;
    case EE_TIT_A_TXT: e->titulo_a_texto = c; break;
    case EE_TIT_I1: e->titulo_i1 = e->marco_i = c; break;
    case EE_TIT_I2: e->titulo_i2 = c; break;
    case EE_TIT_I_TXT: e->titulo_i_texto = c; break;
    case EE_MARCO: e->marco_a = e->marco_i = c; break;
    case EE_VENTANA:
        e->fundo = c; e->cara = c;
        e->m_superficie = esq_mix(c, 0xFFFFFF, esq_lum(c) > 128 ? 60 : 5); e->m_barra = esq_mix(c, e->texto, 4);
        e->m_pista = esq_mix(c, e->texto, 14); e->m_desab_fundo = esq_mix(c, e->texto, 8);
        e->ar_inicio = esq_mix(c, e->texto, 3); e->ar_linha = esq_mix(c, e->texto, 22); e->cab_fundo = esq_mix(c, e->texto, 6);
        e->cab_linha = esq_mix(c, e->texto, 28);
        e->luz = esq_lum(c) > 128 ? esq_mix(c, 0xFFFFFF, 80) : esq_mix(c, 0xFFFFFF, 22);
        e->sombra = esq_mix(c, 0, esq_lum(c) > 128 ? 33 : 50); e->oscuro = esq_mix(c, 0, esq_lum(c) > 128 ? 67 : 80);
        break;
    case EE_TEXTO:
        e->texto = e->negro = c; e->texto_fraco = e->m_texto_fraco = esq_mix(c, e->fundo, 38);
        e->borda = esq_mix(e->fundo, c, 45); e->m_borde = esq_mix(e->fundo, c, 24); e->m_borde_forte = esq_mix(e->fundo, c, 48);
        break;
    case EE_CONTROL:
        e->botao = c; e->botao_apertado = esq_mix(c, e->texto, 14); e->m_hover = esq_mix(c, e->texto, 6); e->m_press = esq_mix(c, e->texto, 14);
        break;
    case EE_CAMPO:
        e->campo_fundo = e->lista_fundo = c; e->sel_suave = esq_mix(c, e->selecao, 30);
        e->ar_cab = esq_mix(c, e->texto, 5); e->ar_previa = esq_mix(c, e->texto, 3); e->ar_atual = esq_mix(c, e->selecao, 14);
        break;
    case EE_CAMPO_TXT: e->campo_texto = e->lista_texto = c; break;
    case EE_SELECCION:
        e->selecao = e->azul = c; e->sel_suave = esq_mix(e->campo_fundo, c, 30);
        e->ar_atual_sel = esq_mix(c, 0, 25); e->ar_atual = esq_mix(e->campo_fundo, c, 14);
        break;
    case EE_SEL_TXT: e->selecao_texto = c; break;
    case EE_MENU: e->menu_fundo = c; e->menu_sep = esq_mix(c, e->menu_texto, 22); break;
    case EE_MENU_TXT: e->menu_texto = c; e->menu_atalho = esq_mix(c, e->menu_fundo, 38); break;
    case EE_MENU_SEL: e->menu_sel = c; break;
    case EE_BARRA: e->barra = e->painel_fundo = c; e->barra_propia = 1; break;
    case EE_BARRA_TXT: e->barra_texto = e->painel_texto = c; e->barra_propia = 1; break;
    case EE_ACENTO:
        e->acento = e->destaque = c; e->m_acento_hover = esq_mix(c, 0xFFFFFF, 12); e->m_acento_press = esq_mix(c, 0, 20);
        e->m_acento_suave = esq_mix(e->fundo, c, 22); e->m_sobre_acento = esq_lum(c) > 120 ? 0 : 0xFFFFFF; e->m_ok = c;
        break;
    case EE_INFO: e->info_fundo = e->m_dica_fundo = c; break;
    case EE_INFO_TXT: e->info_texto = e->m_dica_texto = c; break;
    case EE_TXT_DIS: e->texto_dis = e->m_desab_texto = c; break;
    case EE_SCROLL: e->scroll_polegar = c; break;
    default: return;
    }
    e->id = ESQ_PERSONAL;
    if (el == EE_ESC_A || el == EE_ESC_B) e->escritorio_liso = 1;
}

#endif
