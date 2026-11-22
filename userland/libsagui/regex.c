// SPDX-License-Identifier: GPL-2.0-or-later
/* regex.c - expresiones regulares propias para grep, find y compañía (D097).
 *
 * Se compilan a un programa pequeño de instrucciones que ejecuta una máquina
 * con retroceso (pila explícita, sin recursión): la primera coincidencia más a
 * la izquierda, con los cuantificadores voraces. No hay capturas ni {n,m}. */
#include "sagui.h"

enum { I_CHAR = 1, I_ANY, I_CLASS, I_SPLIT, I_JMP, I_MATCH, I_BOL, I_EOL, I_WORDB };

typedef struct { er_t *r; const char *p; bool ere; int prof; } comp_t;

static int minuscula(int c)
{
    if (c >= 'A' && c <= 'Z') return c + 32;
    if (c >= 0xC0 && c <= 0xDE && c != 0xD7) return c + 32;
    return c;
}

static bool letra_palavra(int c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c >= 0xC0;
}

static int emitir(comp_t *c, int op, int x, int y)
{
    er_t *r = c->r;
    if (r->n >= ER_MAX_INST) { copia_texto(r->erro, "expresión demasiado larga", sizeof r->erro); return -1; }
    r->inst[r->n].op = (unsigned char)op;
    r->inst[r->n].x = (short)x;
    r->inst[r->n].y = (short)y;
    return r->n++;
}

/* Inserta una instrucción en la posición pos y corrige los saltos que apuntan después. */
static int inserir(comp_t *c, int pos, int op, int x, int y)
{
    er_t *r = c->r;
    if (r->n >= ER_MAX_INST) { copia_texto(r->erro, "expresión demasiado larga", sizeof r->erro); return -1; }
    for (int i = r->n; i > pos; i--) r->inst[i] = r->inst[i - 1];
    r->n++;
    for (int i = 0; i < r->n; i++) {
        if (i == pos) continue;
        er_inst_t *k = &r->inst[i];
        if (k->op == I_SPLIT) { if (k->x >= pos) k->x++; if (k->y >= pos) k->y++; }
        else if (k->op == I_JMP) { if (k->x >= pos) k->x++; }
    }
    r->inst[pos].op = (unsigned char)op;
    r->inst[pos].x = (short)x;
    r->inst[pos].y = (short)y;
    return pos;
}

static int nova_classe(comp_t *c)
{
    if (c->r->nclasses >= ER_MAX_CLASSES) { copia_texto(c->r->erro, "demasiadas clases []", sizeof c->r->erro); return -1; }
    memset(c->r->classes[c->r->nclasses], 0, 32);
    return c->r->nclasses++;
}

static void bit(unsigned char *m, int ch, bool ignorar)
{
    m[ch >> 3] |= (unsigned char)(1 << (ch & 7));
    if (ignorar) {
        int l = minuscula(ch);
        if (l != ch) m[l >> 3] |= (unsigned char)(1 << (l & 7));
        else if ((ch >= 'a' && ch <= 'z') || (ch >= 0xE0 && ch <= 0xFE && ch != 0xF7)) {
            int u = ch - 32;
            m[u >> 3] |= (unsigned char)(1 << (u & 7));
        }
    }
}

static bool en_clase(const unsigned char *m, int ch) { return (m[ch >> 3] >> (ch & 7)) & 1; }

/* Clases abreviadas: \w \s \d y sus negaciones. */
static void clase_abrev(unsigned char *m, int tipo, bool negar)
{
    for (int ch = 0; ch < 256; ch++) {
        bool v = tipo == 'w' ? letra_palavra(ch) : tipo == 's' ? (ch == ' ' || (ch >= 9 && ch <= 13)) : (ch >= '0' && ch <= '9');
        if (v != negar) m[ch >> 3] |= (unsigned char)(1 << (ch & 7));
    }
}

static bool nome_classe(const char *s, const char *nome, int n)
{
    return compara_texto_n(s, nome, (size_t)n) == 0 && s[n] == ':' && s[n + 1] == ']';
}

static int analisar_corchete(comp_t *c)
{
    int k = nova_classe(c);
    if (k < 0) return -1;
    unsigned char *m = c->r->classes[k];
    const char *p = c->p + 1;
    bool neg = false;
    if (*p == '^') { neg = true; p++; }
    bool primeiro = true;
    while (*p && (*p != ']' || primeiro)) {
        primeiro = false;
        if (p[0] == '[' && p[1] == ':') {
            static const struct { const char *n; int len; } cl[] = {
                {"alpha", 5}, {"digit", 5}, {"alnum", 5}, {"upper", 5}, {"lower", 5}, {"space", 5}, {"punct", 5}, {"blank", 5}, {"xdigit", 6} };
            bool achou = false;
            for (unsigned i = 0; i < sizeof cl / sizeof cl[0]; i++) {
                if (!nome_classe(p + 2, cl[i].n, cl[i].len)) continue;
                for (int ch = 0; ch < 256; ch++) {
                    bool alfa = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch >= 0xC0;
                    bool dig = ch >= '0' && ch <= '9';
                    bool v = false;
                    switch (i) {
                    case 0: v = alfa; break;
                    case 1: v = dig; break;
                    case 2: v = alfa || dig; break;
                    case 3: v = (ch >= 'A' && ch <= 'Z') || (ch >= 0xC0 && ch <= 0xDE); break;
                    case 4: v = (ch >= 'a' && ch <= 'z') || (ch >= 0xDF && ch <= 0xFF); break;
                    case 5: v = ch == ' ' || (ch >= 9 && ch <= 13); break;
                    case 6: v = ch > 32 && ch < 127 && !alfa && !dig; break;
                    case 7: v = ch == ' ' || ch == '\t'; break;
                    default: v = dig || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'); break;
                    }
                    if (v) bit(m, ch, false);
                }
                p += 2 + cl[i].len + 2;
                achou = true;
                break;
            }
            if (achou) continue;
        }
        int a = (unsigned char)*p++;
        if (a == '\\' && (*p == 'w' || *p == 's' || *p == 'd')) { clase_abrev(m, *p, false); p++; continue; }
        if (*p == '-' && p[1] && p[1] != ']') {
            int b = (unsigned char)p[1];
            p += 2;
            for (int ch = a; ch <= b; ch++) bit(m, ch, c->r->ignorar);
        } else bit(m, a, c->r->ignorar);
    }
    if (*p != ']') { copia_texto(c->r->erro, "falta ] para cerrar la clase", sizeof c->r->erro); return -1; }
    if (neg) for (int i = 0; i < 32; i++) m[i] = (unsigned char)~m[i];
    m[0] &= (unsigned char)~(1 << 0);   /* el byte 0 nunca casa */
    c->p = p + 1;
    return k;
}

/* ¿En p hay un operador (grupo, alternancia, cuantificador)? Devuelve su letra
 * y cuántos caracteres ocupa en *len. En ERE son los caracteres sueltos; en BRE,
 * solo * y las formas con barra. */
static int operador(const comp_t *c, const char *p, int *len)
{
    *len = 1;
    if (c->ere) {
        if (*p == '(' || *p == ')' || *p == '|' || *p == '*' || *p == '+' || *p == '?') return *p;
        return 0;
    }
    if (*p == '*') return '*';
    if (*p == '\\' && (p[1] == '(' || p[1] == ')' || p[1] == '|' || p[1] == '+' || p[1] == '?')) { *len = 2; return p[1]; }
    return 0;
}

static int analisar_alt(comp_t *c);

/* Un átomo seguido de sus cuantificadores. */
static int analisar_pieza(comp_t *c, bool inicio)
{
    int a = c->r->n, len, op = operador(c, c->p, &len);
    const char *p = c->p;
    if (op == '(') {
        c->p += len;
        if (++c->prof > 20) { copia_texto(c->r->erro, "demasiados paréntesis anidados", sizeof c->r->erro); return -1; }
        if (analisar_alt(c) < 0) return -1;
        c->prof--;
        if (operador(c, c->p, &len) != ')') { copia_texto(c->r->erro, "falta cerrar el paréntesis", sizeof c->r->erro); return -1; }
        c->p += len;
    } else if (*p == '.') { c->p++; if (emitir(c, I_ANY, 0, 0) < 0) return -1; }
    else if (*p == '[') {
        int k = analisar_corchete(c);
        if (k < 0 || emitir(c, I_CLASS, k, 0) < 0) return -1;
    } else if (*p == '^' && (c->ere || inicio)) { c->p++; if (emitir(c, I_BOL, 0, 0) < 0) return -1; return 0; }
    else if (*p == '$' && (c->ere || !p[1] || (p[1] == '\\' && (p[2] == ')' || p[2] == '|')))) {
        c->p++;
        if (emitir(c, I_EOL, 0, 0) < 0) return -1;
        return 0;
    } else if (*p == '\\' && p[1]) {
        int e = (unsigned char)p[1];
        c->p += 2;
        if (e == 'b') { if (emitir(c, I_WORDB, 0, 0) < 0) return -1; return 0; }
        if (e == 'w' || e == 's' || e == 'd' || e == 'W' || e == 'S' || e == 'D') {
            int k = nova_classe(c);
            if (k < 0) return -1;
            clase_abrev(c->r->classes[k], minuscula(e), e < 'a');
            if (emitir(c, I_CLASS, k, 0) < 0) return -1;
        } else {
            if (e == 'n') e = '\n';
            else if (e == 't') e = '\t';
            if (emitir(c, I_CHAR, c->r->ignorar ? minuscula(e) : e, 0) < 0) return -1;
        }
    } else if (*p == '\\') { copia_texto(c->r->erro, "barra invertida al final", sizeof c->r->erro); return -1; }
    else {
        /* '*' (u otro operador) sin nada delante en ERE/BRE se toma como letra */
        int e = (unsigned char)*p;
        c->p++;
        if (emitir(c, I_CHAR, c->r->ignorar ? minuscula(e) : e, 0) < 0) return -1;
    }
    for (;;) {
        op = operador(c, c->p, &len);
        if (op != '*' && op != '+' && op != '?') break;
        c->p += len;
        if (op == '*') {
            if (inserir(c, a, I_SPLIT, a + 1, 0) < 0 || emitir(c, I_JMP, a, 0) < 0) return -1;
            c->r->inst[a].y = (short)c->r->n;
        } else if (op == '+') {
            int s = emitir(c, I_SPLIT, a, 0);
            if (s < 0) return -1;
            c->r->inst[s].y = (short)c->r->n;
        } else {
            if (inserir(c, a, I_SPLIT, a + 1, 0) < 0) return -1;
            c->r->inst[a].y = (short)c->r->n;
        }
    }
    return 0;
}

static int analisar_sec(comp_t *c)
{
    bool inicio = true;
    int len;
    while (*c->p) {
        int op = operador(c, c->p, &len);
        if (op == '|' || op == ')') break;
        if (analisar_pieza(c, inicio) < 0) return -1;
        inicio = false;
    }
    return 0;
}

static int analisar_alt(comp_t *c)
{
    int ini = c->r->n, len;
    if (analisar_sec(c) < 0) return -1;
    while (operador(c, c->p, &len) == '|') {
        c->p += len;
        int s = inserir(c, ini, I_SPLIT, ini + 1, 0);
        if (s < 0) return -1;
        int j = emitir(c, I_JMP, 0, 0);
        if (j < 0) return -1;
        c->r->inst[s].y = (short)c->r->n;
        if (analisar_sec(c) < 0) return -1;
        c->r->inst[j].x = (short)c->r->n;
    }
    return 0;
}

int er_compilar(er_t *r, const char *padrao, bool estendida, bool ignorar)
{
    comp_t c = { r, padrao, estendida, 0 };
    r->n = 0; r->nclasses = 0; r->ignorar = ignorar; r->erro[0] = 0;
    if (analisar_alt(&c) < 0) return -1;
    if (*c.p) { copia_texto(r->erro, "paréntesis ) sin abrir", sizeof r->erro); return -1; }
    if (emitir(&c, I_MATCH, 0, 0) < 0) return -1;
    return 0;
}

#define PILHA_ER 4096
/* D378: 32 KB del malloc la primera vez, no de la BSS (la regex va en libsagui.so). */
static struct er_pilha { short pc; int sp; } *pilha;

static bool executar_er(const er_t *r, const char *t, int n, int inicio, int *fim)
{
    int topo = 0, passos = 0;
    if (!pilha && !(pilha = aloca(PILHA_ER * sizeof *pilha))) return false;
    pilha[topo].pc = 0; pilha[topo].sp = inicio; topo++;
    while (topo > 0) {
        int pc = pilha[--topo].pc, sp = pilha[topo].sp;
        for (;;) {
            if (++passos > 200000) return false;
            const er_inst_t *k = &r->inst[pc];
            int ch = sp < n ? (unsigned char)t[sp] : -1;
            if (k->op == I_CHAR) {
                if (ch < 0 || (r->ignorar ? minuscula(ch) : ch) != k->x) break;
                pc++; sp++;
            } else if (k->op == I_ANY) {
                if (ch < 0) break;
                pc++; sp++;
            } else if (k->op == I_CLASS) {
                if (ch < 0 || !en_clase(r->classes[k->x], ch)) break;
                pc++; sp++;
            } else if (k->op == I_SPLIT) {
                if (topo >= PILHA_ER) return false;
                pilha[topo].pc = k->y; pilha[topo].sp = sp; topo++;
                pc = k->x;
            } else if (k->op == I_JMP) {
                pc = k->x;
            } else if (k->op == I_BOL) {
                if (sp != 0) break;
                pc++;
            } else if (k->op == I_EOL) {
                if (sp != n) break;
                pc++;
            } else if (k->op == I_WORDB) {
                bool a = sp > 0 && letra_palavra((unsigned char)t[sp - 1]);
                bool b = sp < n && letra_palavra((unsigned char)t[sp]);
                if (a == b) break;
                pc++;
            } else {   /* I_MATCH */
                *fim = sp;
                return true;
            }
        }
    }
    return false;
}

bool er_buscar(const er_t *r, const char *texto, int tam, int desde, int *ini, int *fim)
{
    for (int s = desde; s <= tam; s++) {
        int f;
        if (executar_er(r, texto, tam, s, &f)) {
            if (ini) *ini = s;
            if (fim) *fim = f;
            return true;
        }
    }
    return false;
}
