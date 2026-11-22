// SPDX-License-Identifier: GPL-2.0-or-later
/* hora.c - hora UTC, zona del proceso y formato de fechas (D080).
 *
 * El kernel entrega el UTC (SIS_HORA) y la zona del sistema (SIS_ZONA). La
 * conversión a la hora local se hace aquí, con el mismo fuso.c del kernel,
 * porque cada proceso puede tener su zona: la variable de entorno TZ, si
 * existe, manda (como en Unix). Acepta un nombre de la tabla
 * ("America/Sao_Paulo") o una regla POSIX ("<-03>3"). */
#include "sagui.h"

#ifdef SAGUI_PORTABLE
/* D459: en la capa portable (SDL 2) estas seis salen de janelas_sdl.c / sistema_sdl.c, sin llamadas al kernel. */
#else
static inline int chamar(unsigned n, unsigned a, unsigned b, unsigned c)
{
    int r;
    __asm__ volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c) : "memory");
    return r;
}

uint32_t hora_utc(hora_t *h)       { return (uint32_t)chamar(SIS_HORA, HORA_UTC, (unsigned)h, 0); }
int  ajustar_hora(uint32_t utc, int opcoes) { return chamar(SIS_HORA, HORA_AJUSTAR, utc, (unsigned)opcoes); }
int  ler_rtc(data_t *d)            { return chamar(SIS_HORA, HORA_LER_RTC, (unsigned)d, 0); }
int  ler_zona(config_zona_t *z)    { return chamar(SIS_ZONA, ZONA_LER, (unsigned)z, 0); }
int  fixar_zona(const config_zona_t *z) { return chamar(SIS_ZONA, ZONA_FIXAR, (unsigned)z, 0); }
int  datas_arquivo(const char *c, datas_arquivo_t *d) { return chamar(SIS_DATAS_ARQUIVO, (unsigned)c, (unsigned)d, 0); }
#endif

/* La zona del proceso, guardada hasta que cambien la TZ o la zona del sistema. */
static fuso_t fuso;
static char tz_visto[FUSO_TZ_MAX + 1];
static char nome_zona[48];
static uint32_t ajustes_vistos = 0xFFFFFFFFu;
static bool tem_fuso;

static void carregar(void)
{
    const char *tz = obter_env("TZ");
    hora_t h;
    if (tz && *tz) {
        if (tem_fuso && compara_texto(tz, tz_visto) == 0) return;
        copia_texto(tz_visto, tz, sizeof tz_visto);
        const zona_info_t *z = zona_achar(tz);
        if (z) { fuso_ler(z->tz, &fuso); copia_texto(nome_zona, z->nome, sizeof nome_zona); }
        else if (fuso_ler(tz, &fuso)) copia_texto(nome_zona, tz, sizeof nome_zona);
        else copia_texto(nome_zona, "UTC", sizeof nome_zona);   /* TZ no válida: UTC, como glibc */
        tem_fuso = true;
        ajustes_vistos = 0xFFFFFFFFu;
        return;
    }
    hora_utc(&h);
    if (tem_fuso && !tz_visto[0] && h.ajustes == ajustes_vistos) return;
    config_zona_t c;
    if (ler_zona(&c) < 0 || !fuso_ler(c.tz, &fuso)) { fuso_utc(&fuso); copia_texto(c.nome, "UTC", sizeof c.nome); }
    copia_texto(nome_zona, c.nome, sizeof nome_zona);
    tz_visto[0] = 0;
    ajustes_vistos = h.ajustes;
    tem_fuso = true;
}

const fuso_t *zona_do_processo(const char **nome)
{
    carregar();
    if (nome) *nome = nome_zona;
    return &fuso;
}

void hora_local(uint32_t utc, fuso_data_t *d)
{
    carregar();
    fuso_local(&fuso, utc, d);
}

void agora_local(fuso_data_t *d) { hora_local(hora_utc(NULL), d); }

uint32_t local_para_utc(uint32_t local, int *situacao)
{
    carregar();
    return fuso_para_utc(&fuso, local, situacao);
}

/* ------------------------------------------------------------ formato */

static const char *const dias_curto[] = { "dom", "lun", "mar", "mié", "jue", "vie", "sáb" };
static const char *const dias_longo[] = { "domingo", "lunes", "martes", "miércoles", "jueves", "viernes", "sábado" };
static const char *const meses_curto[] = { "ene", "feb", "mar", "abr", "may", "jun", "jul", "ago", "sep", "oct", "nov", "dic" };
static const char *const meses_longo[] = { "enero", "febrero", "marzo", "abril", "mayo", "junio", "julio", "agosto",
                                           "septiembre", "octubre", "noviembre", "diciembre" };

/* Un subconjunto de strftime, con nombres en español: %Y %m %d %e %H %I %M
 * %S %p %Z %z %a %A %b %B %j %s %F %T %R %D %%. */
int formatar_data(char *s, int max, const char *fmt, const fuso_data_t *d)
{
    int k = 0;
    char t[32];
    if (max <= 0) return 0;
    for (const char *p = fmt; *p; p++) {
        const char *ins = t;
        t[0] = 0;
        if (*p != '%' || !p[1]) { t[0] = *p; t[1] = 0; }
        else {
            unsigned m = d->mes >= 1 && d->mes <= 12 ? d->mes - 1u : 0;
            unsigned h12 = d->hora % 12 ? d->hora % 12u : 12;
            int z = d->desvio < 0 ? -d->desvio : d->desvio;
            switch (*++p) {
            case 'Y': formatar_texto(t, sizeof t, "%u", d->ano); break;
            case 'm': formatar_texto(t, sizeof t, "%02u", d->mes); break;
            case 'd': formatar_texto(t, sizeof t, "%02u", d->dia); break;
            case 'e': formatar_texto(t, sizeof t, "%2u", d->dia); break;
            case 'H': formatar_texto(t, sizeof t, "%02u", d->hora); break;
            case 'I': formatar_texto(t, sizeof t, "%02u", h12); break;
            case 'M': formatar_texto(t, sizeof t, "%02u", d->minuto); break;
            case 'S': formatar_texto(t, sizeof t, "%02u", d->segundo); break;
            case 'p': ins = d->hora < 12 ? "a. m." : "p. m."; break;
            case 'Z': ins = d->abrev[0] ? d->abrev : "UTC"; break;
            case 'z': formatar_texto(t, sizeof t, "%c%02d%02d", d->desvio < 0 ? '-' : '+', z / 3600, z / 60 % 60); break;
            case 'a': ins = dias_curto[d->dia_semana % 7]; break;
            case 'A': ins = dias_longo[d->dia_semana % 7]; break;
            case 'b': ins = meses_curto[m]; break;
            case 'B': ins = meses_longo[m]; break;
            case 'j': formatar_texto(t, sizeof t, "%03u", d->dia_ano + 1u); break;
            case 's': formatar_texto(t, sizeof t, "%u",
                                     fuso_compor(d->ano, d->mes, d->dia, d->hora, d->minuto, d->segundo) - (uint32_t)d->desvio);
                      break;
            case 'F': formatar_texto(t, sizeof t, "%u-%02u-%02u", d->ano, d->mes, d->dia); break;
            case 'T': formatar_texto(t, sizeof t, "%02u:%02u:%02u", d->hora, d->minuto, d->segundo); break;
            case 'R': formatar_texto(t, sizeof t, "%02u:%02u", d->hora, d->minuto); break;
            case 'D': formatar_texto(t, sizeof t, "%02u/%02u/%02u", d->mes, d->dia, d->ano % 100); break;
            case '%': ins = "%"; break;
            default:  formatar_texto(t, sizeof t, "%%%c", *p); break;
            }
        }
        for (; *ins && k < max - 1; ins++) s[k++] = *ins;
    }
    s[k] = 0;
    return k;
}
