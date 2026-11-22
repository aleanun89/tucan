// SPDX-License-Identifier: GPL-3.0-only
/* entrada.c - lectura de líneas. */
#include "sagui.h"

/* Lee hasta el final de la línea (el '\n' no entra en el buffer). Devuelve el tamaño
 * de la línea, o -1 si el archivo terminó antes de leer nada. Lee un
 * byte por vez: así nunca consume lo que viene después de la línea, lo que
 * importa cuando otro programa va a leer el resto del mismo pipe. */
int ler_linha(int fd, char *buf, unsigned max)
{
    unsigned n = 0;
    for (;;) {
        char c;
        int r = ler(fd, &c, 1);
        if (r <= 0) {
            if (n == 0) return -1;
            break;
        }
        if (c == '\n') break;
        if (n + 1 < max) buf[n++] = c;
    }
    buf[n] = 0;
    return (int)n;
}
