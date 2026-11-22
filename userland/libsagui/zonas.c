// SPDX-License-Identifier: GPL-2.0-or-later
/* zonas.c - la tabla de zonas horarias de SaguiOS (D080).
 *
 * Una lista corta y propia, no la base tz: las zonas más buscadas, cada
 * una con el nombre al estilo de la base tz ("Europe/Madrid"), un nombre en
 * español para mostrar y la regla POSIX TZ vigente hoy (son hechos
 * públicos: el desplazamiento y las fechas de cambio de cada país). No hay
 * histórico: una fecha antigua (antes del último cambio de regla del país) se
 * muestra con la regla de hoy, como hace cualquier sistema que solo tiene la
 * cadena TZ.
 *
 * Orden: por región, como aparece en "zona lista" y en la ventana. */
#include "sagui.h"

static const zona_info_t tabela[] = {
    { "UTC",                            "UTC (tiempo universal coordinado)", "UTC0" },
    /* España y Portugal */
    { "Europe/Madrid",                  "España (Madrid, península)",        "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Atlantic/Canary",                "España (Canarias)",                 "WET0WEST,M3.5.0/1,M10.5.0" },
    { "Africa/Ceuta",                   "España (Ceuta y Melilla)",          "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Lisbon",                  "Portugal (Lisboa)",                 "WET0WEST,M3.5.0/1,M10.5.0" },
    { "Atlantic/Madeira",               "Portugal (Madeira)",                "WET0WEST,M3.5.0/1,M10.5.0" },
    { "Atlantic/Azores",                "Portugal (Azores)",                 "<-01>1<+00>,M3.5.0/0,M10.5.0/1" },
    /* Resto de Europa */
    { "Europe/London",                  "Reino Unido (Londres)",             "GMT0BST,M3.5.0/1,M10.5.0" },
    { "Europe/Dublin",                  "Irlanda (Dublín)",                  "GMT0IST,M3.5.0/1,M10.5.0" },
    { "Europe/Paris",                   "Francia (París)",                   "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Brussels",                "Bélgica (Bruselas)",                "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Amsterdam",               "Países Bajos (Ámsterdam)",          "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Berlin",                  "Alemania (Berlín)",                 "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Zurich",                  "Suiza (Zúrich)",                    "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Rome",                    "Italia (Roma)",                     "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Vienna",                  "Austria (Viena)",                   "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Stockholm",               "Suecia (Estocolmo)",                "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Warsaw",                  "Polonia (Varsovia)",                "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Prague",                  "Chequia (Praga)",                   "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Athens",                  "Grecia (Atenas)",                   "EET-2EEST,M3.5.0/3,M10.5.0/4" },
    { "Europe/Helsinki",                "Finlandia (Helsinki)",              "EET-2EEST,M3.5.0/3,M10.5.0/4" },
    { "Europe/Bucharest",               "Rumanía (Bucarest)",                "EET-2EEST,M3.5.0/3,M10.5.0/4" },
    { "Europe/Kyiv",                    "Ucrania (Kiev)",                    "EET-2EEST,M3.5.0/3,M10.5.0/4" },
    { "Europe/Istanbul",                "Turquía (Estambul)",                "<+03>-3" },
    { "Europe/Moscow",                  "Rusia (Moscú)",                     "MSK-3" },
    /* África y Oriente Próximo */
    { "Africa/Lagos",                   "Nigeria (Lagos)",                   "WAT-1" },
    { "Africa/Cairo",                   "Egipto (El Cairo)",                 "EET-2EEST,M4.5.5/0,M10.5.4/24" },
    { "Africa/Johannesburg",            "Sudáfrica (Johannesburgo)",         "SAST-2" },
    { "Africa/Nairobi",                 "Kenia (Nairobi)",                   "EAT-3" },
    { "Asia/Jerusalem",                 "Israel (Jerusalén)",                "IST-2IDT,M3.4.4/26,M10.5.0" },
    { "Asia/Tehran",                    "Irán (Teherán)",                    "<+0330>-3:30" },
    { "Asia/Dubai",                     "Emiratos Árabes (Dubái)",           "<+04>-4" },
    /* Asia y Oceanía */
    { "Asia/Karachi",                   "Pakistán (Karachi)",                "PKT-5" },
    { "Asia/Kolkata",                   "India (Calcuta, Nueva Delhi)",      "IST-5:30" },
    { "Asia/Kathmandu",                 "Nepal (Katmandú)",                  "<+0545>-5:45" },
    { "Asia/Bangkok",                   "Tailandia (Bangkok)",               "<+07>-7" },
    { "Asia/Jakarta",                   "Indonesia (Yakarta)",               "WIB-7" },
    { "Asia/Shanghai",                  "China (Pekín, Shanghái)",           "CST-8" },
    { "Asia/Hong_Kong",                 "Hong Kong",                         "HKT-8" },
    { "Asia/Singapore",                 "Singapur",                          "<+08>-8" },
    { "Asia/Manila",                    "Filipinas (Manila)",                "PST-8" },
    { "Asia/Seoul",                     "Corea del Sur (Seúl)",              "KST-9" },
    { "Asia/Tokyo",                     "Japón (Tokio)",                     "JST-9" },
    { "Australia/Perth",                "Australia (Perth)",                 "AWST-8" },
    { "Australia/Darwin",               "Australia (Darwin)",                "ACST-9:30" },
    { "Australia/Adelaide",             "Australia (Adelaida)",              "ACST-9:30ACDT,M10.1.0,M4.1.0/3" },
    { "Australia/Brisbane",             "Australia (Brisbane)",              "AEST-10" },
    { "Australia/Sydney",               "Australia (Sídney, Melbourne)",     "AEST-10AEDT,M10.1.0,M4.1.0/3" },
    { "Pacific/Auckland",               "Nueva Zelanda (Auckland)",          "NZST-12NZDT,M9.5.0,M4.1.0/3" },
    { "Pacific/Chatham",                "Nueva Zelanda (Chatham)",           "<+1245>-12:45<+1345>,M9.5.0/2:45,M4.1.0/3:45" },
    /* América del Norte y Central */
    { "Pacific/Honolulu",               "EE. UU. (Hawái)",                   "HST10" },
    { "America/Anchorage",              "EE. UU. (Alaska)",                  "AKST9AKDT,M3.2.0,M11.1.0" },
    { "America/Los_Angeles",            "EE. UU. (Pacífico, Los Ángeles)",   "PST8PDT,M3.2.0,M11.1.0" },
    { "America/Phoenix",                "EE. UU. (Arizona)",                 "MST7" },
    { "America/Denver",                 "EE. UU. (Montaña, Denver)",         "MST7MDT,M3.2.0,M11.1.0" },
    { "America/Chicago",                "EE. UU. (Centro, Chicago)",         "CST6CDT,M3.2.0,M11.1.0" },
    { "America/New_York",               "EE. UU. (Este, Nueva York)",        "EST5EDT,M3.2.0,M11.1.0" },
    { "America/Vancouver",              "Canadá (Vancouver)",                "MST7" },   /* horario de verano permanente desde noviembre de 2026 */
    { "America/Toronto",                "Canadá (Toronto)",                  "EST5EDT,M3.2.0,M11.1.0" },
    { "America/St_Johns",               "Canadá (Terranova)",                "NST3:30NDT,M3.2.0,M11.1.0" },
    { "America/Tijuana",                "México (Tijuana)",                  "PST8PDT,M3.2.0,M11.1.0" },
    { "America/Mexico_City",            "México (Ciudad de México)",         "CST6" },
    { "America/Cancun",                 "México (Cancún)",                   "EST5" },
    { "America/Guatemala",              "Guatemala",                         "CST6" },
    { "America/Costa_Rica",             "Costa Rica",                        "CST6" },
    { "America/Panama",                 "Panamá",                            "EST5" },
    { "America/Havana",                 "Cuba (La Habana)",                  "CST5CDT,M3.2.0/0,M11.1.0/1" },
    { "America/Santo_Domingo",          "República Dominicana",              "AST4" },
    { "America/Puerto_Rico",            "Puerto Rico",                       "AST4" },
    /* América del Sur */
    { "America/Bogota",                 "Colombia (Bogotá)",                 "<-05>5" },
    { "America/Lima",                   "Perú (Lima)",                       "<-05>5" },
    { "America/Caracas",                "Venezuela (Caracas)",               "<-04>4" },
    { "America/La_Paz",                 "Bolivia (La Paz)",                  "<-04>4" },
    { "America/Santiago",               "Chile (Santiago)",                  "<-04>4<-03>,M9.1.6/24,M4.1.6/24" },
    { "America/Argentina/Buenos_Aires", "Argentina (Buenos Aires)",          "<-03>3" },
    { "America/Montevideo",             "Uruguay (Montevideo)",              "<-03>3" },
    { "America/Sao_Paulo",              "Brasil (São Paulo, Brasilia, Río)", "<-03>3" },
    { "America/Bahia",                  "Brasil (Bahía)",                    "<-03>3" },
    { "America/Fortaleza",              "Brasil (Fortaleza, Recife)",        "<-03>3" },
    { "America/Belem",                  "Brasil (Belém)",                    "<-03>3" },
    { "America/Manaus",                 "Brasil (Manaos, Amazonas)",         "<-04>4" },
    { "America/Cuiaba",                 "Brasil (Cuiabá, Mato Grosso)",      "<-04>4" },
    { "America/Rio_Branco",             "Brasil (Acre)",                     "<-05>5" },
    { "America/Noronha",                "Brasil (Fernando de Noronha)",      "<-02>2" },
};

const zona_info_t *zona_tabela(int *n)
{
    *n = (int)(sizeof tabela / sizeof tabela[0]);
    return tabela;
}

/* Por el nombre de la base tz, sin distinguir mayúsculas ("europe/madrid"). */
const zona_info_t *zona_achar(const char *nome)
{
    for (unsigned i = 0; i < sizeof tabela / sizeof tabela[0]; i++) {
        const char *a = tabela[i].nome, *b = nome;
        while (*a && *b) {
            char x = *a, y = *b;
            if (x >= 'A' && x <= 'Z') x = (char)(x + 32);
            if (y >= 'A' && y <= 'Z') y = (char)(y + 32);
            if (x != y) break;
            a++; b++;
        }
        if (!*a && !*b) return &tabela[i];
    }
    return NULL;
}
