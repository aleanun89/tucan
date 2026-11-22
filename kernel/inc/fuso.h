// SPDX-License-Identifier: GPL-2.0-or-later
/* fuso.h - husos horarios con reglas POSIX TZ (D080).
 *
 * El mismo fuso.c entra en el kernel (reloj de la barra, fechas del FAT32, RTC en
 * hora local), en la libsagui (date, ls -l, la variable TZ) y en un programa de
 * prueba compilado en el anfitrión (tests/h67), que compara el resultado con el
 * zoneinfo de Python y con el tzset de la glibc. Por eso no depende de
 * nada: ni de texto.h ni de la libsagui.
 *
 * Los instantes son segundos desde 1/1/1970 en UTC en un uint32_t (vale hasta
 * 2106) y las cuentas evitan división de 64 bits, que ni el kernel ni los
 * programas enlazan (sin libgcc). */
/* En los programas, sagui.h incluye este archivo al final; incluir sagui.h antes
 * de la guarda deja que funcionen ambos órdenes. */
#if !defined(SAGUI_KERNEL) && !defined(FUSO_ANFITRIAO)
#include "sagui.h"
#endif
#ifndef SAGUI_FUSO_H
#define SAGUI_FUSO_H

#if defined(SAGUI_KERNEL)
#include "tipos.h"
#elif defined(FUSO_ANFITRIAO)
#include <stdint.h>
#include <stdbool.h>
#endif

#define FUSO_NOME_MAX 12      /* abreviatura: "CEST", "-03", "+0545" */
#define FUSO_TZ_MAX   64      /* una cadena POSIX TZ */

/* Cuándo empieza o termina el horario de verano. */
enum { FUSO_REGRA_J = 1,      /* Jn: dia 1..365, sin contar nunca el 29 de febrero */
       FUSO_REGRA_N,          /* n: dia 0..365, contando el 29 de febrero */
       FUSO_REGRA_M };        /* Mm.w.d: día d (0 = domingo) de la semana w (5 = última) del mes m */

typedef struct {
    uint8_t  tipo, mes, semana, dia_semana;
    uint16_t dia;
    int32_t  hora;            /* segundos después de la medianoche local (-167 h a +167 h; por defecto 2 h) */
} fuso_regra_t;

typedef struct {
    char    nome_padrao[FUSO_NOME_MAX], nome_verao[FUSO_NOME_MAX];
    int32_t desvio_padrao;    /* segundos a SUMAR al UTC (Madrid: +3600; São Paulo: -10800) */
    int32_t desvio_verao;
    bool    tem_verao;
    fuso_regra_t inicio, fim; /* inicio: en la hora estándar; fim: en la hora de verano (como manda POSIX) */
} fuso_t;

/* Una fecha y hora "de pared" (hora local). */
typedef struct {
    uint16_t ano;
    uint8_t  mes, dia, hora, minuto, segundo;
    uint8_t  dia_semana;      /* 0 = domingo */
    uint16_t dia_ano;         /* 0..365 */
    bool     verao;
    int32_t  desvio;          /* el desvío en vigor (segundos a sumar al UTC) */
    char     abrev[FUSO_NOME_MAX];
} fuso_data_t;

/* Para fuso_para_utc: cómo era la hora local pedida. */
#define FUSO_NORMAL      0
#define FUSO_REPETIDA    1    /* existe dos veces (el reloj retrocedió): queda la primera */
#define FUSO_INEXISTENTE 2    /* cayó en el hueco (el reloj adelantó): vale el desvío de antes */

/* Interpreta una cadena POSIX TZ ("CET-1CEST,M3.5.0,M10.5.0/3", "<-03>3").
 * false = inválida; en ese caso *f queda en UTC. */
bool     fuso_ler(const char *tz, fuso_t *f);
void     fuso_utc(fuso_t *f);
/* Desvío en vigor en un instante UTC. */
int32_t  fuso_desvio(const fuso_t *f, uint32_t utc, bool *verao);
/* UTC -> fecha y hora locales. */
void     fuso_local(const fuso_t *f, uint32_t utc, fuso_data_t *d);
/* Hora local (en segundos, como si fuera UTC) -> instante UTC. */
uint32_t fuso_para_utc(const fuso_t *f, uint32_t local, int *situacao);

/* Calendario puro (sin huso). */
uint32_t fuso_compor(int ano, int mes, int dia, int hora, int minuto, int segundo);
void     fuso_decompor(uint32_t t, fuso_data_t *d);   /* desvío 0, sin abreviatura */
int      fuso_dias_mes(int ano, int mes);
bool     fuso_bissexto(int ano);

#endif
