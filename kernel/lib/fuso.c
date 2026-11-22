// SPDX-License-Identifier: GPL-2.0-or-later
/* fuso.c - intérprete de cadenas POSIX TZ y calendario (D080).
 *
 * Formato (POSIX.1, "TZ"), con la extensión de RFC 8536 para horas de la regla
 * entre -167 y +167:
 *
 *     estándar desfase [verano [desfase] [,inicio[/hora],fin[/hora]]]
 *
 *   - nombres: 3 o más letras ("CET") o cualquier texto entre < > con
 *     letras, dígitos, + y - ("<-03>", "<+0545>");
 *   - desfase: [+-]hh[:mm[:ss]], con el signo al revés de lo habitual (es
 *     lo que se SUMA a la hora local para llegar al UTC: Madrid es "-1");
 *   - sin desfase de verano: una hora más que el estándar;
 *   - reglas: Jn (1..365, el 29/2 nunca cuenta), n (0..365, cuenta) o Mm.w.d
 *     (día d de la semana w del mes m; w = 5 es "la última"); hora estándar 02:00;
 *   - verano sin reglas: las de EE. UU. (M3.2.0,M11.1.0), como hace glibc.
 *
 * Escrito desde cero a partir de la norma; no usa nada de la base tz ni de glibc
 * (que solo sirven de referencia en la prueba h67, en el anfitrión). */
#include "fuso.h"

/* ------------------------------------------------------------ calendario */

bool fuso_bissexto(int ano) { return (ano % 4 == 0 && ano % 100 != 0) || ano % 400 == 0; }

int fuso_dias_mes(int ano, int mes)
{
    static const uint8_t d[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (mes < 1 || mes > 12) return 30;
    return d[mes - 1] + (mes == 2 && fuso_bissexto(ano));
}

/* Días desde 1/1/1970 (algoritmo "days from civil" de H. Hinnant: cuenta en
 * eras de 400 años empezando en marzo, lo que pone el 29/2 al final del año). */
static int32_t dias_desde_epoca(int ano, int mes, int dia)
{
    int32_t a = ano - (mes <= 2);
    int32_t era = (a >= 0 ? a : a - 399) / 400;
    int32_t ano_era = a - era * 400;
    int32_t dia_ano = (153 * (mes + (mes > 2 ? -3 : 9)) + 2) / 5 + dia - 1;
    int32_t dia_era = ano_era * 365 + ano_era / 4 - ano_era / 100 + dia_ano;
    return era * 146097 + dia_era - 719468;
}

uint32_t fuso_compor(int ano, int mes, int dia, int hora, int minuto, int segundo)
{
    int32_t d = dias_desde_epoca(ano, mes, dia);
    int64_t t = (int64_t)d * 86400 + hora * 3600 + minuto * 60 + segundo;
    if (t < 0) return 0;
    if (t > 0xFFFFFFFFll) return 0xFFFFFFFFu;
    return (uint32_t)t;
}

void fuso_decompor(uint32_t t, fuso_data_t *d)
{
    uint32_t dias = t / 86400u, seg = t % 86400u;
    d->hora = (uint8_t)(seg / 3600);
    d->minuto = (uint8_t)(seg / 60 % 60);
    d->segundo = (uint8_t)(seg % 60);
    d->dia_semana = (uint8_t)((dias + 4) % 7);   /* 1/1/1970 fue un jueves */
    /* El inverso de dias_desde_epoca ("civil from days"). */
    uint32_t z = dias + 719468u;
    uint32_t era = z / 146097u, dia_era = z - era * 146097u;
    uint32_t ano_era = (dia_era - dia_era / 1460 + dia_era / 36524 - dia_era / 146096) / 365;
    uint32_t dia_ano = dia_era - (365 * ano_era + ano_era / 4 - ano_era / 100);
    uint32_t mp = (5 * dia_ano + 2) / 153;
    uint32_t mes = mp < 10 ? mp + 3 : mp - 9;
    uint32_t ano = ano_era + era * 400 + (mes <= 2);
    d->dia = (uint8_t)(dia_ano - (153 * mp + 2) / 5 + 1);
    d->mes = (uint8_t)mes;
    d->ano = (uint16_t)ano;
    d->dia_ano = (uint16_t)(dias - (uint32_t)dias_desde_epoca((int)ano, 1, 1));
    d->verao = false;
    d->desvio = 0;
    d->abrev[0] = 0;
}

/* ------------------------------------------------------------ lectura de la cadena */

static bool letra(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static bool digito(char c) { return c >= '0' && c <= '9'; }

static bool ler_nome(const char **p, char *nome)
{
    const char *s = *p;
    int n = 0;
    if (*s == '<') {
        s++;
        while (*s && *s != '>') {
            if (!letra(*s) && !digito(*s) && *s != '+' && *s != '-') return false;
            if (n < FUSO_NOME_MAX - 1) nome[n++] = *s;
            s++;
        }
        if (*s != '>' || n < 3) return false;
        s++;
    } else {
        while (letra(*s)) {
            if (n < FUSO_NOME_MAX - 1) nome[n++] = *s;
            s++;
        }
        if (n < 3) return false;
    }
    nome[n] = 0;
    *p = s;
    return true;
}

static bool ler_numero(const char **p, int32_t *v, int32_t max)
{
    if (!digito(**p)) return false;
    int32_t n = 0;
    while (digito(**p)) {
        n = n * 10 + (**p - '0');
        if (n > max) return false;
        (*p)++;
    }
    *v = n;
    return true;
}

/* [+-]hh[:mm[:ss]] -> segundos con signo. */
static bool ler_hora(const char **p, int32_t *seg, int32_t max_h)
{
    int sinal = 1;
    if (**p == '+') (*p)++;
    else if (**p == '-') { sinal = -1; (*p)++; }
    int32_t h, m = 0, s = 0;
    if (!ler_numero(p, &h, max_h)) return false;
    if (**p == ':') {
        (*p)++;
        if (!ler_numero(p, &m, 59)) return false;
        if (**p == ':') {
            (*p)++;
            if (!ler_numero(p, &s, 59)) return false;
        }
    }
    *seg = sinal * (h * 3600 + m * 60 + s);
    return true;
}

static bool ler_regra(const char **p, fuso_regra_t *r)
{
    int32_t a, b, c;
    if (**p == 'J') {
        (*p)++;
        if (!ler_numero(p, &a, 365) || a < 1) return false;
        r->tipo = FUSO_REGRA_J;
        r->dia = (uint16_t)a;
    } else if (**p == 'M') {
        (*p)++;
        if (!ler_numero(p, &a, 12) || a < 1 || **p != '.') return false;
        (*p)++;
        if (!ler_numero(p, &b, 5) || b < 1 || **p != '.') return false;
        (*p)++;
        if (!ler_numero(p, &c, 6)) return false;
        r->tipo = FUSO_REGRA_M;
        r->mes = (uint8_t)a; r->semana = (uint8_t)b; r->dia_semana = (uint8_t)c;
    } else {
        if (!ler_numero(p, &a, 365)) return false;
        r->tipo = FUSO_REGRA_N;
        r->dia = (uint16_t)a;
    }
    r->hora = 2 * 3600;
    if (**p == '/') {
        (*p)++;
        if (!ler_hora(p, &r->hora, 167)) return false;
    }
    return true;
}

void fuso_utc(fuso_t *f)
{
    for (unsigned i = 0; i < sizeof *f; i++) ((char *)f)[i] = 0;
    f->nome_padrao[0] = 'U'; f->nome_padrao[1] = 'T'; f->nome_padrao[2] = 'C';
}

bool fuso_ler(const char *tz, fuso_t *f)
{
    fuso_t r;
    fuso_utc(&r);
    const char *p = tz;
    if (!p || *p == ':') goto invalida;   /* ":archivo" es la forma de la base tz, que no tenemos */
    int32_t desvio;
    if (!ler_nome(&p, r.nome_padrao) || !ler_hora(&p, &desvio, 24)) goto invalida;
    r.desvio_padrao = -desvio;
    r.desvio_verao = r.desvio_padrao;
    if (*p) {
        if (!ler_nome(&p, r.nome_verao)) goto invalida;
        r.tem_verao = true;
        r.desvio_verao = r.desvio_padrao + 3600;
        if (*p && *p != ',') {
            if (!ler_hora(&p, &desvio, 24)) goto invalida;
            r.desvio_verao = -desvio;
        }
        if (*p == ',') {
            p++;
            if (!ler_regra(&p, &r.inicio) || *p != ',') goto invalida;
            p++;
            if (!ler_regra(&p, &r.fim)) goto invalida;
        } else {
            const char *eua = "M3.2.0,M11.1.0";
            ler_regra(&eua, &r.inicio);
            eua++;
            ler_regra(&eua, &r.fim);
        }
        if (*p) goto invalida;
    }
    *f = r;
    return true;
invalida:
    fuso_utc(f);
    return false;
}

/* ------------------------------------------------------------ conversiones */

/* Segundos (hora local, desde 1970) en que la regla cae en el año. */
static int64_t transicao(const fuso_regra_t *r, int ano)
{
    int32_t dia;   /* días desde 1970 */
    if (r->tipo == FUSO_REGRA_J) {
        dia = dias_desde_epoca(ano, 1, 1) + r->dia - 1;
        if (fuso_bissexto(ano) && r->dia >= 60) dia++;   /* J60 es siempre el 1 de marzo */
    } else if (r->tipo == FUSO_REGRA_N) {
        dia = dias_desde_epoca(ano, 1, 1) + r->dia;
    } else {
        int32_t primeiro = dias_desde_epoca(ano, r->mes, 1);
        int dsem = (int)((primeiro + 4) % 7);
        int d = 1 + (r->dia_semana - dsem + 7) % 7 + (r->semana - 1) * 7;
        while (d > fuso_dias_mes(ano, r->mes)) d -= 7;   /* semana 5 = la última */
        dia = primeiro + d - 1;
    }
    return (int64_t)dia * 86400 + r->hora;
}

static int ano_de(int64_t t)
{
    fuso_data_t d;
    fuso_decompor(t < 0 ? 0 : t > 0xFFFFFFFFll ? 0xFFFFFFFFu : (uint32_t)t, &d);
    return d.ano;
}

int32_t fuso_desvio(const fuso_t *f, uint32_t utc, bool *verao)
{
    bool v = false;
    if (f->tem_verao) {
        int ano = ano_de((int64_t)utc + f->desvio_padrao);
        int64_t ini = transicao(&f->inicio, ano) - f->desvio_padrao;   /* en UTC */
        int64_t fim = transicao(&f->fim, ano) - f->desvio_verao;
        int64_t t = utc;
        /* Hemisferio norte: verano entre inicio y fin. Sur (inicio después del
         * fin en el mismo año): verano fuera del intervalo fin..inicio. */
        v = ini < fim ? (t >= ini && t < fim) : !(t >= fim && t < ini);
    }
    if (verao) *verao = v;
    return v ? f->desvio_verao : f->desvio_padrao;
}

void fuso_local(const fuso_t *f, uint32_t utc, fuso_data_t *d)
{
    bool v;
    int32_t desvio = fuso_desvio(f, utc, &v);
    int64_t l = (int64_t)utc + desvio;
    fuso_decompor(l < 0 ? 0 : l > 0xFFFFFFFFll ? 0xFFFFFFFFu : (uint32_t)l, d);
    d->verao = v;
    d->desvio = desvio;
    const char *n = v ? f->nome_verao : f->nome_padrao;
    int i = 0;
    for (; n[i] && i < FUSO_NOME_MAX - 1; i++) d->abrev[i] = n[i];
    d->abrev[i] = 0;
}

static uint32_t limitar(int64_t t) { return t < 0 ? 0 : t > 0xFFFFFFFFll ? 0xFFFFFFFFu : (uint32_t)t; }

/* Las dos lecturas posibles de la hora local (con el desfase estándar y con el de
 * verano) y cuáles de ellas son coherentes. Ambas: la hora repetida al final del
 * verano (se queda la primera vez, con el desfase de antes del cambio). Ninguna: el
 * agujero del inicio del verano (vale el desfase de antes, el menor). Es lo que hace
 * el zoneinfo de Python con fold=0. */
uint32_t fuso_para_utc(const fuso_t *f, uint32_t local, int *situacao)
{
    if (situacao) *situacao = FUSO_NORMAL;
    if (!f->tem_verao) return limitar((int64_t)local - f->desvio_padrao);
    uint32_t c_padrao = limitar((int64_t)local - f->desvio_padrao);
    uint32_t c_verao = limitar((int64_t)local - f->desvio_verao);
    bool v;
    fuso_desvio(f, c_padrao, &v);
    bool ok_padrao = !v;
    fuso_desvio(f, c_verao, &v);
    bool ok_verao = v;
    if (ok_padrao && ok_verao) {
        if (situacao && c_padrao != c_verao) *situacao = FUSO_REPETIDA;
        return c_padrao < c_verao ? c_padrao : c_verao;
    }
    if (ok_padrao) return c_padrao;
    if (ok_verao) return c_verao;
    if (situacao) *situacao = FUSO_INEXISTENTE;
    return c_padrao > c_verao ? c_padrao : c_verao;
}
