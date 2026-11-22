// SPDX-License-Identifier: GPL-2.0-or-later
/* sincronia.c - primitivas atómicas, mutex y semáforo para los programas (D150).
 *
 * Esto es la base del multihilo: las piezas que necesitará el día que un
 * proceso pueda tener varios hilos.
 *
 * Las atómicas van en asm en línea con el prefijo lock, como en el kernel
 * (kernel/inc/atomicas.h): el programa se compila con -mgeneral-regs-only
 * y sin las llamadas a la biblioteca de apoyo del GCC, así que los __sync y
 * los __atomic no sirven. Con lock, la operación es atómica y hace de
 * barrera de memoria, que es justo lo que necesita un cerrojo.
 *
 * El mutex y el semáforo son *por proceso*: viven en la memoria del
 * programa y protegen los datos que comparten los hilos de ese proceso.
 *
 * D167: quien no consigue el cerrojo, tras unas vueltas, duerme en el
 * kernel sobre la propia palabra del cerrojo (futex_esperar) y quien lo
 * suelta despierta a uno (futex_acordar) solo si alguien duerme: sin
 * nadie esperando, tomar y soltar no entran en el kernel. Es el mutex de
 * tres estados de U. Drepper («Futexes are tricky»).
 *
 * El mutex no es reentrante: tomarlo dos veces desde el mismo hilo se
 * queda esperando para siempre (como en pthread sin PTHREAD_MUTEX_RECURSIVE). */
#include "sagui.h"

/* ------------------------------------------------------------ atómicas */

/* Comparación y cambio: si *p vale "esperado", pasa a valer "novo" y
 * devuelve 1; si no, lo deja como estaba y devuelve 0. */
int atomico_cas32(volatile uint32_t *p, uint32_t esperado, uint32_t novo)
{
    uint32_t reg = esperado;
    /* Tras el cmpxchg, EAX vale lo que había en *p: si sigue siendo el
     * esperado, el cambio se hizo. */
    __asm__ volatile("lock; cmpxchgl %2, %0"
                     : "+m"(*p), "+a"(reg)
                     : "r"(novo)
                     : "cc", "memory");
    return (reg == esperado) ? 1 : 0;
}

/* Suma atómica: devuelve el valor que tenía *p antes de la suma. */
uint32_t atomico_add32(volatile uint32_t *p, uint32_t valor)
{
    uint32_t r = valor;
    /* xadd deja en el registro el valor anterior de la memoria. */
    __asm__ volatile("lock; xaddl %0, %1" : "+r"(r), "+m"(*p) : : "cc", "memory");
    return r;
}

/* Resta atómica: devuelve el valor que tenía *p antes de la resta. */
uint32_t atomico_sub32(volatile uint32_t *p, uint32_t valor)
{
    uint32_t r = (uint32_t)(0u - valor);
    __asm__ volatile("lock; xaddl %0, %1" : "+r"(r), "+m"(*p) : : "cc", "memory");
    return r;
}

uint32_t atomico_inc32(volatile uint32_t *p) { return atomico_add32(p, 1); }
uint32_t atomico_dec32(volatile uint32_t *p) { return atomico_sub32(p, 1); }

/* Intercambio atómico: pasa *p a "novo" y devuelve el valor anterior. */
uint32_t atomico_troca32(volatile uint32_t *p, uint32_t novo)
{
    uint32_t r = novo;
    __asm__ volatile("lock; xchgl %0, %1" : "+r"(r), "+m"(*p) : : "cc", "memory");
    return r;
}

/* Barrera de memoria: no se reordena nada alrededor de la llamada. En x86
 * un lock sobre una variable inofensiva vale; no hace falta mfence (que
 * además pediría SSE2, que el programa puede no usar). */
static volatile uint32_t barrera_atomico;

void atomico_barreira(void)
{
    __asm__ volatile("lock; orl $0, %0" : "+m"(barrera_atomico) : : "cc", "memory");
}

/* ---- 64 bits: necesitan cmpxchg8b (Pentium en adelante) ---- */

typedef union {
    uint64_t total;
    uint32_t metade[2];
} u64_t;

static inline void cpuid(uint32_t folha, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
    __asm__ volatile("cpuid" : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) : "a"(folha), "c"(0));
}

/* 0 = todavía no mirado, 1 = hay cmpxchg8b, 2 = no hay. */
static int estado_cas64;

bool atomico_tem_cas64(void)
{
    if (estado_cas64 == 0) {
        uint32_t a, b, c, d;
        cpuid(0, &a, &b, &c, &d);
        if (a >= 1) {
            cpuid(1, &a, &b, &c, &d);
            estado_cas64 = (d & (1u << 8)) ? 1 : 2;   /* EDX bit 8 = CX8 */
        } else {
            estado_cas64 = 2;
        }
    }
    return estado_cas64 == 1;
}

bool atomico_cas64(volatile uint64_t *p, uint64_t esperado, uint64_t novo)
{
    if (!atomico_tem_cas64()) return false;
    if (((unsigned long)p & 7u) != 0) return false;   /* alineación de 8 bytes */
    u64_t velho, nuevo64;
    velho.total = esperado;
    nuevo64.total = novo;
    __asm__ volatile("lock; cmpxchg8b %0"
                     : "+m"(*p), "+a"(velho.metade[0]), "+d"(velho.metade[1])
                     : "b"(nuevo64.metade[0]), "c"(nuevo64.metade[1])
                     : "cc", "memory");
    return (velho.total == esperado);
}

uint64_t atomico_add64(volatile uint64_t *p, uint64_t valor)
{
    if (!atomico_tem_cas64()) return 0;
    if (((unsigned long)p & 7u) != 0) return 0;
    uint64_t antigo = *p;
    for (;;) {
        u64_t velho, novo64;
        velho.total = antigo;
        novo64.total = antigo + valor;
        __asm__ volatile("lock; cmpxchg8b %0"
                         : "+m"(*p), "+a"(velho.metade[0]), "+d"(velho.metade[1])
                         : "b"(novo64.metade[0]), "c"(novo64.metade[1])
                         : "cc", "memory");
        if (velho.total == antigo) return antigo;
        antigo = velho.total;
    }
}

/* ------------------------------------------------------------ mutex */

/* Estados: 0 libre, 1 tomado, 2 tomado y puede haber alguien durmiendo. */
#define VOLTAS_GIRO 128

void mutex_criar(mutex_t *m)
{
    if (!m) return;
    m->estado = 0;
    m->dono = 0;
}

bool mutex_tentar(mutex_t *m)
{
    if (!m) return false;
    return atomico_cas32(&m->estado, 0, 1) != 0;
}

bool mutex_tomar(mutex_t *m)
{
    if (!m) return false;
    /* El caso fácil, sin entrar en el kernel: libre, o se suelta enseguida. */
    for (int i = 0; i < VOLTAS_GIRO; i++) {
        if (atomico_cas32(&m->estado, 0, 1)) return true;
        __asm__ volatile("pause" ::: "memory");
    }
    /* Se marca "2" (hay quien espera) y se duerme mientras siga tomado. Al
     * despertar se vuelve a marcar "2": puede haber otros durmiendo, y quien
     * suelte tiene que despertarlos. */
    while (atomico_troca32(&m->estado, 2) != 0)
        futex_esperar(&m->estado, 2, 0);
    m->dono = (uint32_t)thread_id();   /* ya hubo que esperar: el coste no se nota */
    return true;
}

void mutex_largar(mutex_t *m)
{
    if (!m) return;
    /* El intercambio con lock es una barrera completa: todo lo escrito
     * bajo el cerrojo queda visible antes de soltarlo. */
    m->dono = 0;
    if (atomico_troca32(&m->estado, 0) == 2)
        futex_acordar(&m->estado, 1);
}

/* ------------------------------------------------------------ semáforo */

void semaforo_criar(semaforo_t *s, uint32_t valor)
{
    if (!s) return;
    s->valor = valor;
    s->esperando = 0;
}

bool semaforo_tentar(semaforo_t *s)
{
    if (!s) return false;
    for (;;) {
        uint32_t v = s->valor;
        if (v == 0) return false;
        if (atomico_cas32(&s->valor, v, v - 1)) return true;
    }
}

/* Quien duerme se apunta en "esperando" ANTES de mirar el contador, y quien
 * sinaliza suma al contador ANTES de mirar "esperando" (las dos con lock,
 * barreras completas): o el que duerme ve el contador nuevo, o el que
 * sinaliza lo ve apuntado y lo despierta. Y el kernel solo lo duerme si la
 * palabra sigue en 0 (futex_esperar compara con el cerrojo del kernel). */
void semaforo_tomar(semaforo_t *s)
{
    if (!s) return;
    for (int i = 0; i < VOLTAS_GIRO; i++) {
        if (semaforo_tentar(s)) return;
        __asm__ volatile("pause" ::: "memory");
    }
    for (;;) {
        if (semaforo_tentar(s)) return;
        atomico_inc32(&s->esperando);
        if (s->valor == 0)
            futex_esperar(&s->valor, 0, 0);
        atomico_dec32(&s->esperando);
    }
}

void semaforo_sinalizar(semaforo_t *s)
{
    if (!s) return;
    atomico_add32(&s->valor, 1);
    if (s->esperando)
        futex_acordar(&s->valor, 1);
}

uint32_t semaforo_valor(semaforo_t *s)
{
    if (!s) return 0;
    return s->valor;
}
