// SPDX-License-Identifier: GPL-2.0-or-later
/* sistema_nulo.c - las llamadas de SaguiOS que no existen fuera de él (D459).
 *
 * Discos, procesos, hilos del kernel, usuarios, red, audio y el esquema de apariencia del escritorio no tienen
 * equivalente en la capa portable: devuelven un error limpio para que los programas lo traten como lo harían
 * en SaguiOS sin esa función (sin disco, sin red, sin sonido...). La red (TLS) queda desactivada en las
 * aplicaciones portables, así que el asistente de IA no puede conectar. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sagui.h"
#include "tls.h"

int info_sistema(int tipo, void *buf, unsigned n) { (void)tipo; (void)buf; (void)n; return ERRO_NAO_SUPORTADO; }
int usuario_op(int op, int a, int b) { (void)op; (void)a; (void)b; return ERRO_NAO_SUPORTADO; }
int servicos_kernel(int op, int arg) { (void)op; (void)arg; return ERRO_NAO_SUPORTADO; }
int esquema_op(unsigned op, void *ptr, unsigned n) { (void)op; (void)ptr; (void)n; return ERRO_NAO_SUPORTADO; }
int telas_sis(int op, unsigned a, unsigned b) { (void)op; (void)a; (void)b; return ERRO_NAO_SUPORTADO; }
int listar_discos(int indice, info_disco_t *d) { (void)indice; (void)d; return 0; }
int montar_em(const char *nome, const char *ponto, int opcoes) { (void)nome; (void)ponto; (void)opcoes; return ERRO_NAO_SUPORTADO; }
int desmontar(const char *nome) { (void)nome; return ERRO_NAO_SUPORTADO; }
int modo_terminal(int modo) { (void)modo; return 0; }
int acento_cor(int rgb) { (void)rgb; return 0x0B7A3E; }

/* procesos */
int executar(const char *c, char *const argv[]) { (void)c; (void)argv; return ERRO_NAO_SUPORTADO; }
int executar_env(const char *c, char *const argv[], char *const envp[]) { (void)c; (void)argv; (void)envp; return ERRO_NAO_SUPORTADO; }
int esperar(int pid, int *codigo) { (void)pid; (void)codigo; return ERRO_SEM_PROCESSO; }
int recolher(int *codigo) { (void)codigo; return 0; }
int esperar_pid(int pid, int *estado, int opcoes) { (void)pid; (void)estado; (void)opcoes; return ERRO_SEM_PROCESSO; }
int matar(int pid) { (void)pid; return ERRO_SEM_PROCESSO; }
int futex_esperar(volatile uint32_t *p, uint32_t v, unsigned ms) { (void)p; (void)v; (void)ms; return 1; }
int futex_acordar(volatile uint32_t *p, int q) { (void)p; (void)q; return 0; }

/* audio */
int audio_abrir(int taxa, int canais, int bits) { (void)taxa; (void)canais; (void)bits; return ERRO_NAO_EXISTE; }
int audio_controle(int fd, int op, int valor) { (void)fd; (void)op; (void)valor; return ERRO_NAO_EXISTE; }
int audio_escrever_sem_bloquear(int fd, const void *b, unsigned n) { (void)fd; (void)b; (void)n; return ERRO_NAO_EXISTE; }
int tocar(const nota_t *notas, int n, int modo) { (void)notas; (void)n; (void)modo; return 0; }
void silenciar(void) {}

/* red y TLS: desactivados */
int conexao_abrir(conexao_t *c, const url_t *u, const tls_config_t *cfg) { (void)c; (void)u; (void)cfg; return ERRO_SEM_REDE; }
int conexao_ler(void *c, void *buf, unsigned n) { (void)c; (void)buf; (void)n; return ERRO_SEM_REDE; }
int conexao_escrever(conexao_t *c, const void *buf, unsigned n) { (void)c; (void)buf; (void)n; return ERRO_SEM_REDE; }
void conexao_fechar(conexao_t *c) { (void)c; }
bool tls_config_padrao(tls_config_t *cfg, const char *cacert, char *erro, unsigned max)
{
    (void)cfg; (void)cacert;
    if (max) copia_texto(erro, "sin red en la version portable", max);
    return false;
}
int abrir_tv(int fds[2]) { (void)fds; return ERRO_NAO_SUPORTADO; }
int conectar_tcp(const char *host, int porta, unsigned tempo_ms) { (void)host; (void)porta; (void)tempo_ms; return ERRO_SEM_REDE; }
