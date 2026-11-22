// SPDX-License-Identifier: GPL-3.0-only
/* formatar.c - imprimir y formatear texto en los programas.
 *
 * Misma idea que el formateador del kernel: cada número se escribe de atrás para
 * adelante en un buffer pequeño, y una única rutina se ocupa del ancho y del
 * alineado. */
#include "sagui.h"

typedef struct { char *d; size_t max, pos; } destino_t;

static void poe(destino_t *s, char c)
{
    if (s->pos + 1 < s->max) s->d[s->pos] = c;
    s->pos++;
}

static void campo(destino_t *s, const char *t, size_t n, int larg, bool esq, char pre)
{
    int falta = larg > (int)n ? larg - (int)n : 0;
    if (!esq && pre == '0' && n && *t == '-') { poe(s, '-'); t++; n--; }
    if (!esq) while (falta--) poe(s, pre);
    while (n--) poe(s, *t++);
    if (esq) while (falta-- > 0) poe(s, ' ');
}

static char *digitos(char *fim, uint32_t v, unsigned base)
{
    *fim = 0;
    do { *--fim = "0123456789abcdef"[v % base]; v /= base; } while (v);
    return fim;
}

int formatar_lista(char *dst, size_t max, const char *fmt, va_list ap)
{
    destino_t s = { dst, max, 0 };
    char buf[16];
    for (; *fmt; fmt++) {
        if (*fmt != '%') { poe(&s, *fmt); continue; }
        fmt++;
        bool esq = false;
        char pre = ' ';
        int larg = 0;
        for (;; fmt++) {
            if (*fmt == '-') esq = true;
            else if (*fmt == '0') pre = '0';
            else break;
        }
        if (*fmt == '*') { larg = va_arg(ap, int); if (larg < 0) { esq = true; larg = -larg; } fmt++; }
        while (*fmt >= '0' && *fmt <= '9') larg = larg * 10 + (*fmt++ - '0');
        while (*fmt == 'l') fmt++;
        char *t;
        switch (*fmt) {
        case 's': t = va_arg(ap, char *); if (!t) t = "(nulo)"; campo(&s, t, tam_texto(t), larg, esq, ' '); break;
        case 'c': buf[0] = (char)va_arg(ap, int); campo(&s, buf, 1, larg, esq, ' '); break;
        case 'd': case 'i': {
            int v = va_arg(ap, int);
            t = digitos(buf + 15, v < 0 ? 0u - (uint32_t)v : (uint32_t)v, 10);
            if (v < 0) *--t = '-';
            campo(&s, t, tam_texto(t), larg, esq, pre);
            break;
        }
        case 'u': t = digitos(buf + 15, va_arg(ap, uint32_t), 10); campo(&s, t, tam_texto(t), larg, esq, pre); break;
        case 'x': t = digitos(buf + 15, va_arg(ap, uint32_t), 16); campo(&s, t, tam_texto(t), larg, esq, pre); break;
        case 'o': t = digitos(buf + 15, va_arg(ap, uint32_t), 8); campo(&s, t, tam_texto(t), larg, esq, pre); break;   /* D282: permisos */
        case 'p': t = digitos(buf + 15, (uint32_t)va_arg(ap, void *), 16); poe(&s, '0'); poe(&s, 'x'); campo(&s, t, tam_texto(t), 8, false, '0'); break;
        case '%': poe(&s, '%'); break;
        case 0: fmt--; break;
        default: poe(&s, '%'); poe(&s, *fmt);
        }
    }
    if (max) dst[s.pos < max ? s.pos : max - 1] = 0;
    return (int)s.pos;
}

int formatar_texto(char *dst, size_t max, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = formatar_lista(dst, max, fmt, ap);
    va_end(ap);
    return n;
}

/* Un pipe o el terminal pueden aceptar menos bytes de los que pedimos: insiste
 * hasta escribir todo (o dar error). */
int escrever_tudo(int fd, const void *buf, unsigned n)
{
    const char *p = buf;
    unsigned feito = 0;
    while (feito < n) {
        int r = escrever(fd, p + feito, n - feito);
        if (r < 0) return r;
        if (r == 0) break;
        feito += (unsigned)r;
    }
    return (int)feito;
}

int escrever_texto(int fd, const char *s)
{
    return escrever_tudo(fd, s, tam_texto(s));
}

static int imprimir_lista(int fd, const char *fmt, va_list ap)
{
    char buf[512];
    int n = formatar_lista(buf, sizeof buf, fmt, ap);
    if (n >= (int)sizeof buf) n = sizeof buf - 1;
    return escrever_tudo(fd, buf, (unsigned)n);
}

int imprimir(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = imprimir_lista(SAIDA, fmt, ap);
    va_end(ap);
    return n;
}

int imprimir_em(int fd, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = imprimir_lista(fd, fmt, ap);
    va_end(ap);
    return n;
}

/* Una línea en el log del sistema (serie), sin aparecer en pantalla. */
void registrar(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    formatar_lista(buf, sizeof buf, fmt, ap);
    va_end(ap);
    size_t n = tam_texto(buf);
    if (n && buf[n - 1] == '\n') buf[n - 1] = 0;
    registrar_texto(buf);
}

const char *nome_erro(int e)
{
    switch (e) {
    case ERRO_INVALIDO: return "argumento no válido";
    case ERRO_NAO_EXISTE: return "no existe";
    case ERRO_JA_EXISTE: return "ya existe";
    case ERRO_SEM_MEMORIA: return "sin memoria";
    case ERRO_ENDERECO: return "dirección no válida";
    case ERRO_NAO_E_PASTA: return "no es una carpeta";
    case ERRO_E_PASTA: return "es una carpeta";
    case ERRO_NAO_VAZIA: return "la carpeta no está vacía";
    case ERRO_LIMITE: return "límite alcanzado";
    case ERRO_DESCRITOR: return "descriptor no válido";
    case ERRO_PIPE_QUEBRADO: return "nadie lee el pipe";
    case ERRO_SEM_PROCESSO: return "el proceso no existe";
    case ERRO_FORMATO: return "no es un programa válido";
    /* D255: a quien no es root casi siempre se le niega por eso: se le dice cómo seguir. */
    case ERRO_ACESSO:
#ifdef SAGUI_HOST
        return "operación no permitida";   /* las pruebas en el anfitrión no tienen SIS_USUARIO */
#else
        return (usuario_op(USU_CONSULTAR, 0, 0) & 0xFFFF) != UID_ROOT
            ? "operación no permitida (hace falta ser administrador: usa «admin orden»)" : "operación no permitida";
#endif
    case ERRO_OCUPADO: return "en uso";
    case ERRO_SO_LEITURA: return "sistema de archivos de solo lectura";
    case ERRO_NAO_SUPORTADO: return "no admitido por el disco o el driver";
    case ERRO_DISCO_ARRANQUE: return "es el disco de arranque";
    case ERRO_SEM_SENHA: return "la cuenta aún no tiene contraseña (se fija desde la sesión local)";
    case ERRO_ESPERA: return "demasiados intentos fallidos: espera un minuto";
    case ERRO_SEM_ESPACO: return "no queda espacio";
    case ERRO_TEMPO: return "sin respuesta (tiempo agotado)";
    case ERRO_SEM_REDE: return "sin red (no hay tarjeta o aún no tiene dirección IP)";
    case ERRO_RECUSADA: return "conexión rechazada";
    case ERRO_INALCANCAVEL: return "destino inalcanzable";
    case ERRO_CONEXAO_CAIU: return "el otro extremo cortó la conexión";
    default: return "error desconocido";
    }
}
