// SPDX-License-Identifier: GPL-3.0-only
/* sagui.h - libsagui, la biblioteca de los programas de SaguiOS.
 *
 * Todo lo que usa un programa viene de aquí: no hay libc. Los nombres son en
 * portugués. Un programa define "int principal(int argc, char **argv)"; la
 * libsagui llama a esa función y, cuando vuelve, termina el proceso con el
 * valor devuelto. */
#ifndef SAGUI_H
#define SAGUI_H
#include "abi.h"

/* D194: dónde se buscan los programas sin PATH: los de la base en /bin y las
 * aplicaciones de la partición SAGUI-APPS (o del ramdisk, con APPS=0) en /apps/bin. */
#define SAGUI_PATH_PADRAO "/bin:/apps/bin"

#ifdef SAGUI_HOST
/* Compilando para o anfitrión (as provas de comparação com o decodificador
 * de referência do JPEG, por exemplo): os tipos e as funções de string são os
 * da libc, e não os daqui, que são de 32 bits e brigariam com eles. */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#else
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned int       size_t;
typedef _Bool              bool;
#define true  1
#define false 0
#ifndef NULL   /* D384: TCC ya lo define en su stddef.h (y avisa si se redefine) */
#define NULL  ((void *)0)
#endif

typedef __builtin_va_list va_list;
#ifndef va_start   /* la stdarg.h del gcc (que trae la libc común) ya las define */
#define va_start(l, u) __builtin_va_start(l, u)
#define va_arg(l, t)   __builtin_va_arg(l, t)
#define va_end(l)      __builtin_va_end(l)
#endif
#endif

#define ENTRADA 0
#define SAIDA   1
#define ERROS   2

/* ---- processos ---- */
int  executar(const char *caminho, char *const argv[]);   /* el hijo hereda el entorno (environ) */
int  executar_env(const char *caminho, char *const argv[], char *const envp[]);
int  definir_grupo(int pid, int grupo);   /* pid 0 = yo; grupo 0 = grupo nuevo dirigido por pid (D037) */
int  esperar(int pid, int *codigo);
int  recolher(int *codigo);   /* un hijo que ya terminó, sin esperar; 0 si ninguno */
void sair(int codigo) __attribute__((noreturn));
int  meu_pid(void);
int  matar(int pid);
/* D401: procesos POSIX. bifurcar = fork (0 en el hijo); substituir = execve (no vuelve si va bien);
 * esperar_pid = waitpid (estado como Linux, ESPERA_*); sinal_op = SIS_SINAL (SINOP_*); sondar = poll
 * (sondagem_t, ms < 0 sin plazo); descritor_op = fcntl (DESC_*). La libc común los envuelve. */
int  bifurcar(void);
int  substituir(const char *caminho, char *const argv[], char *const envp[]);
int  esperar_pid(int pid, int *estado, int opcoes);
int  sinal_op(int op, unsigned a, unsigned b, unsigned c);
int  sondar(sondagem_t *v, unsigned n, int ms);
int  descritor_op(int fd, int op, unsigned arg);
/* ---- archivos ---- */
int  abrir(const char *caminho, int modo);
int  ler(int fd, void *buf, unsigned n);
int  escrever(int fd, const void *buf, unsigned n);
int  fechar(int fd);
int  posicionar(int fd, int desloc, int origem);
int  info_arquivo(const char *caminho, info_arquivo_t *info);
/* ---- permisos (D282): dueño, grupo y rwx; ver SIS_PERMISOS_ARQ en abi.h ---- */
int  permisos_arquivo(const char *caminho, info_permisos_t *p);
int  mudar_permisos(const char *caminho, unsigned modo);          /* chmod: el dueño o root */
int  mudar_dono(const char *caminho, int uid, int gid);       /* chown/chgrp: -1 = no cambia */
int  mascara_criacao(int nova);                               /* umask: -1 = solo consultar; devuelve la anterior */
/* D341: fija las fechas (UTC). PARQ_DATA_AGORA = ahora; PARQ_DATA_MANTER = no cambia. Una fecha
 * concreta: el dueño o root; «ahora»: basta poder escribir. */
int  fixar_datas(const char *caminho, unsigned modificado, unsigned acessado);
int  fixar_datas_fd(int fd, unsigned modificado, unsigned acessado);   /* el archivo abierto */

void texto_modo(unsigned modo, char saida[10]);               /* "rwxr-xr-x" */
/* ---- carpetas ---- */
int  criar_pasta(const char *caminho);
int  remover_pasta(const char *caminho);
int  apagar(const char *caminho);
int  listar(const char *caminho, int indice, entrada_pasta_t *e);
int  mudar_pasta(const char *caminho);
int  pasta_atual(char *buf, unsigned n);
int  renomear(const char *origem, const char *destino);
int  truncar(int fd, unsigned tamanho);   /* reduce o aumenta (con ceros) un archivo abierto para escribir (D116) */
int  sincronizar(int fd);                 /* fuerza a disco lo pendiente (D116) */
/* ---- comunicación ---- */
int  criar_pipe(int fds[2]);
int  abrir_tv(int fds[2]);         /* terminal virtual: fds[0] = maestro (pantalla), fds[1] = esclavo (D036) */
int  primeiro_plano(int grupo);    /* Ctrl+C en el terminal de la entrada 0 termina este grupo (0 = ninguno); devuelve el anterior */
int  interrupcoes(void);           /* cuántos Ctrl+C ha recibido ya el terminal de la entrada 0 (D037) */
int  tipo_descritor(int fd);       /* TIPO_TERMINAL, TIPO_PIPE o TIPO_ARQUIVO */
int  duplicar(int antigo, int novo);
int  disponivel(int fd);   /* bytes para leer en un pipe sin bloquear; ERRO_PIPE_QUEBRADO = terminó */
/* ---- varias ---- */
int      dormir(unsigned ms);
unsigned tempo_ms(void);
int      ler_data(data_t *d);
int  copiar_imagem(int larg, int alt, const uint32_t *px);   /* D125: portapapeles de imagen; 0 = ok */
int  colar_imagem(uint32_t *destino, unsigned max_px);      /* -> larg | alt << 16 (0 = nada); destino NULL = sólo pregunta */
int  aviso_tela(const char *texto, const char *arquivo);   /* D125: notificación en pantalla */
int  capturar_tela(int id, int rect[4], uint32_t *destino, unsigned max_px);   /* D125: pantalla (0), ventana activa (-1) o por id */
int  inyectar_entrada(int op, int a, int b);   /* mejora 097: SIS_INYECTAR (INY_*), solo para vncd */
int      info_sistema(int tipo, void *buf, unsigned n);
void    *aumentar_memoria(unsigned bytes);
/* D209: páginas del área de datos (MEM_RESERVAR, MEM_DEVOLVER, MEM_ASIGNAR en
 * abi.h). Es la base del malloc; los programas normales usan aloca o mapear_memoria. */
int      memoria_paginas(int op, void *endereco, unsigned bytes);
/* D349: permisos de páginas ya asignadas del área de datos (MEM_PROT_ESCRITA,
 * MEM_PROT_EXEC; como mprotect). Con NX, el montón no es ejecutable: quien
 * genere código y lo ejecute (un recompilador, un JIT) lo pide aquí. Las páginas
 * tienen que estar asignadas (MEM_ASIGNAR o malloc): si no, ERRO_SEM_MEMORIA. */
int      memoria_proteger(void *endereco, unsigned bytes, int prot);
int      reiniciar(void);          /* no vuelve (con servicios, el kernel los para antes, D216); ERRO_ACESSO si no se puede (D255) */
/* D450: lo mismo, empezando por un método (para probar cuál funciona en una máquina): */
#define REINICIO_AUTO    0   /* ACPI, teclado, 0xCF9 y triple fallo */
#define REINICIO_ACPI    1   /* RESET_REG de la FADT */
#define REINICIO_TECLADO 2   /* 8042 (0xFE) */
#define REINICIO_CF9     3   /* 0xCF9 */
#define REINICIO_TRIPLE  4   /* triple fallo */
int      reiniciar_metodo(int m);
int      servicos_kernel(int op, int arg);   /* SIS_SERVICOS: SERV_* en abi.h (D216); lo usan servicios y servicio */
int      servicos_kernel3(int op, int a, int b);   /* lo mismo con dos argumentos (SERV_RESPONDER, D255) */
int      usuario_op(int op, int a, int b);   /* SIS_USUARIO: USU_* en abi.h (D255); ver contas.h */
int      desligar(void);           /* apaga el PC (D113): graba los discos y apaga por ACPI; no vuelve (o ERRO_ACESSO, D255) */
int      acpi_controle(int op);    /* ACPI_OP_DESATIVAR / ACPI_OP_ATIVAR (D113) */
int      modos_video(modo_video_t *v, int n);   /* lista de modos de vídeo (D058) -> cuántos */
int      telas_sis(int op, unsigned a, unsigned b);   /* D290: TELAS_OP_INFO (&info_telas_t, sizeof), _CONFIG, _RESOLUCAO, _GUARDAR */
int      trocar_modo_video(int largura, int altura, int op);   /* MUDAR_AGORA | MUDAR_GUARDAR (D059) -> lo que hizo */
int      modo_terminal(int modo);
int      bytes_aleatorios(void *buf, unsigned n);        /* bytes impredecibles del kernel (D061); devuelve n (hasta 4096) */
int      info_aleatorio(info_aleatorio_t *i);
int      limpar_tela(void);

/* ---- sincronización (D150, D167): atómicas, mutex y semáforo ----
 * El estado se actualiza con atómicas (lock cmpxchg/xchg), que valen con
 * varios núcleos; quien tiene que esperar duerme en el kernel (futex,
 * D167) en vez de girar. Todo esto es *por proceso*: protege los datos que
 * comparten los hilos de un mismo programa, no los de dos programas. */

/* Atómicas. Devuelven el valor anterior (o 1/0 en la comparación y cambio). */
int      atomico_cas32(volatile uint32_t *p, uint32_t esperado, uint32_t novo);
uint32_t atomico_add32(volatile uint32_t *p, uint32_t valor);
uint32_t atomico_sub32(volatile uint32_t *p, uint32_t valor);
uint32_t atomico_inc32(volatile uint32_t *p);
uint32_t atomico_dec32(volatile uint32_t *p);
uint32_t atomico_troca32(volatile uint32_t *p, uint32_t novo);
void     atomico_barreira(void);
bool     atomico_cas64(volatile uint64_t *p, uint64_t esperado, uint64_t novo);
uint64_t atomico_add64(volatile uint64_t *p, uint64_t valor);
bool     atomico_tem_cas64(void);   /* ¿la CPU tiene cmpxchg8b? */

/* Mutex: no es reentrante (tomarlo dos veces se queda esperando). */
typedef struct {
    volatile uint32_t estado;   /* 0 = livre, 1 = tomado, 2 = tomado y alguien duerme esperándolo */
    volatile uint32_t dono;     /* tid de quien lo tomó esperando (0 si fue sin esperar; solo diagnóstico) */
} mutex_t;
#define MUTEX_INICIAL { 0, 0 }

void mutex_criar(mutex_t *m);
bool mutex_tomar(mutex_t *m);     /* bloqueante: espera a que se pueda */
bool mutex_tentar(mutex_t *m);    /* sin esperar: false si otro lo tiene */
void mutex_largar(mutex_t *m);

/* Semáforo contador: "sinalizar" suma uno, "tomar" resta uno (y espera). */
typedef struct {
    volatile uint32_t valor;      /* contador */
    volatile uint32_t esperando;  /* hilos dormidos en tomar (D167) */
} semaforo_t;

void     semaforo_criar(semaforo_t *s, uint32_t valor);
void     semaforo_tomar(semaforo_t *s);       /* bloqueante */
bool     semaforo_tentar(semaforo_t *s);      /* sin esperar */
void     semaforo_sinalizar(semaforo_t *s);   /* suma uno */
uint32_t semaforo_valor(semaforo_t *s);

/* ---- hilos (D167, docs/multihilo.md) ----
 * Los hilos de un proceso comparten la memoria, los ficheros, la carpeta y
 * las ventanas; cada uno tiene su pila y sus registros y puede correr en
 * otra CPU a la vez (con --smp). salir() en cualquier hilo, o un fallo,
 * termina el proceso entero. meu_pid() es el del proceso en todos. */
typedef struct thread thread_t;
thread_t *thread_create(void *(*funcao)(void *), void *arg, unsigned pilha);   /* pilha <= 960 KB (o 0): la del kernel, con guarda (D186) */
int       thread_join(thread_t *t);     /* espera que acabe; devuelve (int) lo que devolvió la función */
void      thread_yield(void);           /* cede el turno */
void      thread_sair(void);            /* acaba este hilo (en el principal, el proceso) */
int       thread_id(void);              /* el número de este hilo (tid; el pid en el principal) */
int       thread_tid(const thread_t *t);
/* Lo de debajo, para quien quiera otra cosa (otra pila, otra convención). */
int  hilo_criar(void (*entrada)(void), void *pilha_topo);   /* -> tid */
int  hilo_criar_kernel(void (*entrada)(void), void *arg);  /* -> tid; pila de 1 MB del kernel con guarda (D186) */
void hilo_sair(int codigo) __attribute__((noreturn));
int  hilo_esperar(int tid, int *codigo);
void hilo_ceder(void);
int  futex_esperar(volatile uint32_t *palavra, uint32_t valor, unsigned ms);   /* 1 = ya no valía "valor" */
int  futex_acordar(volatile uint32_t *palavra, int quantos);                   /* -> cuántos despertó */

/* ---- planificadores (D174) ----
 * PLANIFICADOR_RR, _AFINIDAD, _EQUILIBRIO, _PRIORIDAD (abi.h). La máscara de
 * CPU: bit n = CPU n, 0 = todas; vale para todo el proceso (sus hilos) y la
 * heredan los hijos. El nice (-20..19) solo cuenta con "prioridad". pid 0 =
 * el que llama. */
int planificador_consultar(char *nome, unsigned n);        /* -> el activo; su nombre en nome si no es NULL */
int planificador_escolher(int numero);                     /* -> el anterior (en caliente) */
int planificador_nome(int numero, char *nome, unsigned n); /* -> 0, o ERRO_INVALIDO si no existe */
int planificador_numero(const char *nome);                 /* "afinidad" -> PLANIFICADOR_AFINIDAD; -1 */
int afinidade_cpu(int pid, unsigned mascara);              /* -> la máscara anterior */
int afinidade_consultar(int pid);
int prioridade_processo(int pid, int nice);                /* -> 0 (el nice actual, en planificacion_info) */
int planificador_hilo_audio(int ligar);                    /* D436: alta prioridad de audio del hilo que llama; -> antes */
int planificacion_info(int pid, info_planificacion_t *v);

/* ---- ventanas (las funciones de dibujo y los componentes están en gui.h) ---- */
int  criar_janela(const char *titulo, int largura, int altura, uint32_t **pixels);
int  fechar_janela(int id);
int  atualizar_janela(int id, const retangulo_t *r);   /* r = NULL: toda la ventana */
int  proximo_evento(int id, evento_t *e, bool esperar); /* 1 = llegó un evento */
int  esperar_evento(int id, evento_t *e, int ms);      /* hasta ms milisegundos (< 0 = sin plazo); 1 = llegó (D083) */
int  titulo_janela(int id, const char *titulo);
int  mover_janela(int id, int x, int y);   /* marco en la pantalla; x/y < 0 = relativo al borde derecho/inferior (D102) */
int  estilo_janela(int id, unsigned estilo, int min_l, int min_a);   /* JAN_* (D044) */
/* D230: false = el puntero del sistema no se dibuja sobre el contenido de la ventana mientras
 * tenga el foco (el programa dibuja el suyo); vuelve fuera de ella, en menús y al cerrar. */
int  cursor_janela(int id, bool visivel);
int  redimensionar_janela(int id, int largura, int altura, uint32_t **pixels);
int  pedir_fechar_janela(int id);   /* desde cualquier proceso: el dueño recibe EV_FECHAR, como en la X (D043) */
/* Menú contextual (D051): muestra las líneas (separadas por '\n'; "-" = separador,
 * "~Texto" = desactivada; "Texto\tAtalho" alinea el atajo a la derecha) en (x, y) de la ventana, que debe estar en foco. Vuelve
 * al instante; la elección llega después como EV_MENU: e.x = índice de la línea (todas
 * cuentan, a partir de 0) o -1 si se canceló, e.tecla = código. */
int  menu_contextual(int id, int x, int y, const char *itens, int codigo);
/* Área de transferencia del sistema (D074): un texto Latin-1 de hasta
 * PORTAPAPEIS_MAX bytes, el último que algún programa copió. */
int  copiar_texto(const char *t, unsigned n);   /* -> bytes guardados */
int  colar_texto(char *buf, unsigned max);      /* -> tamaño guardado; copia hasta max - 1 y pone el cero */

/* ---- dispositivos (fase 6) ---- */
int  listar_discos(int indice, info_disco_t *d);   /* 1 = encontró, 0 = terminó */
int  montar(const char *nome);                     /* "usb0" -> /usb */
int  montar_em(const char *nome, const char *ponto, int opcoes);   /* MONTAR_* en abi.h (D040) */
int  montar_dono(const char *nome, const char *ponto, int opcoes, int uid, int gid);   /* D282: uid=, gid= */
int  desmontar(const char *nome);                  /* nombre del disco o carpeta montada */
int  expulsar_disco(const char *nome, int fechar);  /* D233: CD/DVD: desmontar y abrir (1: cerrar) la bandeja */

/* ---- sonido (bonus B3): vuelve al instante, el kernel toca en segundo plano ---- */
int  tocar(const nota_t *notas, int n, int modo);  /* modo BIP_SUBSTITUIR o BIP_ENFILEIRAR */
int  bip(int freq, int ms);
void silenciar(void);

/* ---- audio por la tarjeta de sonido (D088-D090) ----
 * audio_abrir devuelve un descriptor (ERRO_NAO_EXISTE = sin tarjeta: use el bip).
 * Formato: tasa 4000-96000 Hz, 1 o 2 canales, 8 bits sin signo o 16 con signo.
 * fechar(fd) calla al instante; audio_fechar(fd, 1) espera a terminar de tocar. */
int  audio_abrir(int taxa, int canais, int bits);
int  audio_escrever(int fd, const void *buf, unsigned n);              /* bloquea hasta que quepa todo */
int  audio_escrever_sem_bloquear(int fd, const void *buf, unsigned n); /* -> bytes aceptados (0 = lleno) */
int  audio_controle(int fd, int op, int valor);                        /* AUDIO_* en abi.h */
int  audio_volume(int fd, int v);        /* 0-100; -1 = sólo consultar */
int  audio_volume_mestre(int v);         /* idem, de todo el sonido */
int  audio_silencio(int v);              /* 1 calla, 0 vuelve, -1 consulta */
int  audio_salida(int i);               /* salida de sonido (D275): índice actual; i >= 0 la cambia, < 0 consulta */
int  audio_pendente(int fd);             /* ms que aún faltan por tocar */
int  audio_livre(int fd);                /* bytes que caben sin bloquear */
int  audio_fechar(int fd, int esperar);
/* Ecualizador de 5 bandas (D201): db[EQ_BANDAS] en -12..+12 (60 Hz, 250 Hz, 1 kHz, 4 kHz, 12 kHz). */
int  eq_aplicar(const int *db);          /* calcula los biquads (x87) y los manda al mezclador; 0 = bien */
int  eq_calcular(const int *db, unsigned taxa, eq_audio_t *e);
int  eq_calcular_m(const int *db, unsigned taxa, eq_audio_t *e, int margen);   /* margen 0: sin ganancia previa */
int  eq_preajuste(const char *nome, int *db);   /* plano, graves, voz, agudos; 0 = existe */
const char *eq_nome_preajuste(const int *db);   /* el nombre del preajuste que coincide, o NULL */
int  eq_freq_banda(int i);
int  eq_texto_ler(const char *v, int *db);      /* "0,+3,-2,0,12" -> db[5]; 0 = bien */
int  eq_conf_ler(int *db);                      /* la clave ecualizador= de /home/.config/sagui.conf; 0 = hay */
int  eq_conf_gravar(const int *db);             /* la pone en sagui.conf sin tocar el resto; 1 = /home no persiste */
char *eq_db_texto(char *d, int v);              /* "+3", "-2", "0" (d: 4 bytes) */
/* Diagnóstico del códec HDA (D107): HDA_OP_* en abi.h; ERRO_NAO_EXISTE sin el driver hda. */
int  audio_hda(int op, unsigned a, unsigned b);
/* Efectos de los juegos (D090): la lista de notas por la tarjeta de sonido, o por el bip si no hay. */
int  efeito_tocar(const nota_t *notas, int n);
void efeito_parar(void);

/* ---- juegos (bonus B4) ---- */
int  teclas_seguradas(unsigned char mapa[32]);   /* 1 = en foco; CV_* en abi.h */
#define SEGURADA(mapa, cv) (((mapa)[(cv) >> 3] >> ((cv) & 7)) & 1)

/* ---- teclado ---- */
int  teclado_layout(int layout);   /* TECLADO_ES, TECLADO_ABNT2 o -1 = sólo consultar; devuelve el actual */

/* ---- apariencia del escritorio (D112) ---- */
int  fundo_enviar(int op, const uint32_t *px, int y, int n);   /* FUNDO_LINHAS/FIM/PADRAO; n líneas del ancho de la pantalla */
int  acento_cor(int rgb);                                      /* -1 = sólo consultar; devuelve el color actual */
void tema_ao_mudar(void (*repintar)(void));   /* D428: el programa dice cómo repintarse cuando llega EV_TEMA (tras recargar el esquema) */
int  esquema_op(unsigned op, void *ptr, unsigned n);   /* D428: SIS_ESQUEMA (esquemas.h): ESQ_OP_LER / VERSAO / FIJAR / FIJAR_LOGIN */

/* ---- red (D046). IPs como número: 10.0.2.15 = 0x0A00020F ---- */
int  criar_socket(int tipo);                                      /* SOCK_TCP o SOCK_UDP -> fd */
int  conectar(int fd, uint32_t ip, int porta);                   /* TCP: abre; UDP: fija el destino */
int  escutar(int fd, int porta, int fila);
int  aceitar_conexao(int fd, endereco_rede_t *de);                        /* -> fd de la conexión nueva */
int  enviar_para(int fd, const void *buf, unsigned n, uint32_t ip, int porta);
int  receber_de(int fd, void *buf, unsigned n, endereco_rede_t *de);
int  opcao_socket(int fd, int opcao, unsigned valor);             /* OPC_* */
int  tempo_limite(int fd, unsigned ms);                           /* 0 = sin límite */
int  resolver(const char *nome, uint32_t *ip);                    /* "10.0.2.2" o "ejemplo.com"; 0 = ok */
int  resolver_em(const char *nome, uint32_t *ip, uint32_t servidor, int porta);   /* pregunta a otro DNS */
int  config_rede(int op, config_rede_t *c);                       /* REDE_* */
int  ping(uint32_t ip, info_ping_t *info, unsigned prazo_ms);
bool texto_para_ip(const char *s, uint32_t *ip);
char *ip_para_texto(uint32_t ip, char *buf);                      /* buf de 16 bytes */
/* Resuelve, crea el socket y conecta (espera el DHCP del arranque hasta 5 s). */
int  conectar_tcp(const char *host, int porta, unsigned tempo_ms);
/* D117: devuelve el contador de eventos de la red; si sigue siendo "visto",
 * duerme hasta que algún socket reciba algo o pasen ms. Para servidores con
 * varios sockets: mirar todos con disponivel() y, si nada, visto = esperar_rede(visto, ms). */
int  esperar_rede(unsigned visto, unsigned ms);
/* D218: el bloqueo de publicidad y rastreo por DNS del kernel (BLOQ_* en abi.h). */
int  bloqueo_dns(int op, const void *a, unsigned b);
#define IP_PARTES(ip) (unsigned)((ip) >> 24), (unsigned)(((ip) >> 16) & 255), (unsigned)(((ip) >> 8) & 255), (unsigned)((ip) & 255)

/* ---- HTTP (http.c): el cliente de curl y de Cipó ---- */
typedef struct {
    char host[128];
    int  porta;
    char caminho[512];
    bool https;
} url_t;
bool url_separar(const char *url, url_t *u);       /* false = dirección inválida */
bool url_juntar(const char *base, const char *rel, char *saida, int max);   /* resuelve un enlace relativo */
/* De dónde se lee la respuesta: NULL = ler(fd); el HTTPS pasa la lectura del TLS (D062). */
typedef int (*http_ler_t)(void *ctx, void *buf, unsigned n);
typedef struct {
    int  fd;
    http_ler_t ler_fn;
    void *ctx;
    int  codigo;                 /* 200, 404... */
    char cabecalho[4096];        /* línea de estado y campos, terminados en cero */
    int  tam_cabecalho;
    int  restante;               /* Content-Length que falta (-1 = hasta cerrar) */
    bool pedacos;                /* Transfer-Encoding: chunked */
    int  pedaco;                 /* bytes que faltan del trozo actual */
    bool acabou;
    char buf[4096];
    int  ini, fim;
} http_resposta_t;
/* Conecta y envía "metodo caminho HTTP/1.1" con Host y Connection: close;
 * devuelve el descriptor. "extra" (puede ser NULL) son campos de más, cada uno
 * terminado en \r\n. "pedido_enviado" (puede ser NULL) recibe el texto enviado. */
int  http_pedir(const url_t *u, const char *metodo, const char *extra, char *pedido_enviado, int max);
int  http_ler_cabecalho(http_resposta_t *r, int fd);   /* 0 = ok */
int  http_ler_cabecalho_de(http_resposta_t *r, http_ler_t f, void *ctx);   /* igual, leyendo por f (HTTPS) */
/* Sólo arma el texto de la petición (lo que envía http_pedir); devuelve el tamaño. */
int  http_montar_pedido(const url_t *u, const char *metodo, const char *extra, char *buf, int max);
int  http_ler_corpo(http_resposta_t *r, void *dst, int max);   /* bytes; 0 = fin; <0 = error */
bool http_campo(const char *cabecalho, const char *nome, char *valor, int max);

/* ---- memoria y texto ---- */
#ifndef SAGUI_HOST
void  *memcpy(void *d, const void *s, size_t n);
void  *memmove(void *d, const void *s, size_t n);
void  *memset(void *d, int v, size_t n);
int    memcmp(const void *a, const void *b, size_t n);
#endif
size_t tam_texto(const char *s);
int    compara_texto(const char *a, const char *b);
int    compara_texto_n(const char *a, const char *b, size_t n);
char  *copia_texto(char *dst, const char *src, size_t max);    /* siempre termina en cero */
char  *junta_texto(char *dst, const char *src, size_t max);
char  *acha_char(const char *s, int c);
char  *acha_ultimo_char(const char *s, int c);
bool   comeca_com(const char *s, const char *prefixo);
int    texto_para_int(const char *s);
size_t utf8_para_latin1(char *s, size_t n, size_t *sobra);   /* texto UTF-8 de fuera -> Latin-1 */
bool   eh_espaco(int c);

/* ---- memoria y píxeles rápidos (D042): rep movsd o SSE2, elegido por CPUID ---- */
#define MEM_AUTO    0   /* predeterminado: con SSE2, mide las dos versiones en las primeras llamadas y se queda con la mejor */
#define MEM_SIMPLES 1   /* bucles en C, byte a byte (la forma antigua; para comparar) */
#define MEM_REP     2   /* rep movsd / rep stosd */
#define MEM_SSE2    3
#define MEM_SSE     4   /* D208: fuerzan hasta ese juego de instrucciones (la mayor variante que cada función tiene) */
#define MEM_SSE3    5
#define MEM_SSSE3   6
int  mem_metodo(int m);   /* fuerza un método para las pruebas y el bench; devuelve el anterior */
/* CPUID.1:EDX bit 26 (leído una vez). Quien necesita elegir entre código SSE2
 * y x87 pregunta aquí: el cc (D076), rapido.c y, después, el lanzador del
 * python (versiones con y sin SSE2). */
bool cpu_tem_sse2(void);
bool cpu_tem_sse(void);     /* D208: CPUID.1 (con FXSR): para elegir variantes de las rutas rápidas */
bool cpu_tem_sse3(void);
bool cpu_tem_ssse3(void);
/* D381: AVX solo si la CPU lo tiene Y el kernel guarda los YMM (CR4.OSXSAVE, que se ve en
 * CPUID.1:ECX bit 27, y XCR0 con SSE y AVX, que se lee con XGETBV). Sin eso, una instrucción
 * AVX da #UD o, peor, otro proceso pisa los YMM. AVX2 y FMA, además, con su bit de CPUID. */
bool cpu_tem_avx(void);
bool cpu_tem_avx2(void);
bool cpu_tem_fma(void);
int  mem_descrever(char *s, size_t max);   /* "copias: SSE3, memset: SSE2, ..." (mide si hace falta) */
void trocar_rb32(uint32_t *d, const uint32_t *o, size_t n);   /* intercambia rojo y azul: 0xAARRGGBB <-> 0xAABBGGRR */
void misturar32(uint32_t *d, const uint32_t *o, size_t n, unsigned a);   /* d = (o*a + d*(256-a)) >> 8 por canal, a de 0 a 256 */
int  mem_escolhas(char *s, size_t max);   /* lo que eligió el modo automático (para el bench) */
void preencher32(uint32_t *d, uint32_t cor, size_t n);             /* n píxeles */
void ampliar2x32(uint32_t *d, uint32_t *d2, const uint32_t *o, size_t n);   /* d[2i] = d[2i+1] = o[i]; d2 (o NULL): otra línea igual */
void escurecer32(uint32_t *d, size_t n, int desloc, uint32_t mascara);   /* d[i] = (d[i] >> desloc) & mascara */

void *aloca(size_t n);
void *aloca_zerado(size_t n);
void *realoca(void *p, size_t n);
void  libera(void *p);
unsigned memoria_total(void);   /* bytes con memoria física que tiene el malloc (D118, D209) */
size_t memoria_tamanho(const void *p);         /* bytes utilizables de un bloque de aloca; 0 = no es de aloca */
void  *aloca_alinhado(size_t alin, size_t n);  /* alineado a alin (potencia de 2); se libera con libera() */
/* Mapas de memoria (D209): páginas a cero, alineadas a 4 KB, que vuelven al
 * sistema con desmapear_memoria (también un trozo; el resto sigue valiendo).
 * mapear_arquivo lee [desloc, desloc+n) del fichero en un mapa nuevo (copia
 * privada: escribir en él no cambia el fichero). Se liberan con desmapear_memoria. */
void *mapear_memoria(size_t n);
int   desmapear_memoria(void *p, size_t n);
void *mapear_arquivo(int fd, unsigned desloc, size_t n);

/* ---- salida formateada (%s %c %d %u %x %p, ancho, '-', '0', '*') ---- */
int  formatar_lista(char *dst, size_t max, const char *fmt, va_list ap);
int  formatar_texto(char *dst, size_t max, const char *fmt, ...);
int  imprimir(const char *fmt, ...);
int  imprimir_em(int fd, const char *fmt, ...);
int  escrever_tudo(int fd, const void *buf, unsigned n);
int  escrever_texto(int fd, const char *s);
const char *nome_erro(int erro);
void registrar(const char *fmt, ...);   /* registro del sistema (serie), no aparece en pantalla */
int  registrar_texto(const char *t);

/* ---- entorno (D037): textos "NOME=valor"; el kernel los entrega en _inicio ---- */
extern char **environ;
const char *obter_env(const char *nome);   /* valor de NOME, o NULL */

/* ---- idioma (D313, mejora 118): la cadena en español es la clave ----
 * _("texto") devuelve la traducción del idioma elegido (LANG del entorno; si no hay, el del
 * sistema) o el mismo texto en español. N_("texto") solo marca la cadena para
 * tools/extraer_textos.py (tablas estáticas): se traduce al usarla con _(). Los catálogos
 * son /sistema/idiomas/<idioma>/<programa>.cat y libsagui.cat (los textos comunes). */
const char *idioma_traducir(const char *clave);
const char *idioma_actual(void);                 /* "es", "pt_BR", "en", "xx" (pseudo-idioma de pruebas) */
int  idioma_fijar(const char *nombre);           /* cambia el idioma de este proceso y del escritorio (SIS_IDIOMA); 0 = bien */
void idioma_recargar(void);                      /* vuelve a leer LANG o el idioma del sistema (tras un cambio) */
const char *idioma_traducir_sl(const char *clave);   /* como _() pero sin anotar lo que falta: lo usan los componentes (botones, diálogos) */
void idioma_normalizar(const char *entrada, char *salida);   /* "pt", "pt_BR.ISO-8859-1", "EN" -> "pt_BR", "pt_BR", "en"; salida de 12 bytes */
int  idioma_sistema_ler(char *dst, int max);     /* el idioma del sistema (/etc/idioma); 0 = hay, <0 = no */
int  idioma_sistema_gravar(const char *nombre);  /* solo root: lo escribe en /etc/idioma y en la partición de configuración (D255) */
const char *idioma_traducir_menu(const char *menu);   /* menú de líneas separadas por \n (y atajo tras \t): traduce cada línea */
#define _(s)  idioma_traducir(s)
#define N_(s) (s)
#define MENU_(s)  idioma_traducir_menu(s)   /* el extractor saca cada línea del menú como una cadena */
#define NMENU_(s) (s)                       /* solo marca (tablas estáticas); se traduce con MENU_() al usarlo */

/* ---- formatos regionales (D313): decimal, miles y fecha según el idioma ---- */
char regional_decimal(void);                     /* ',' (es, pt_BR) o '.' (en) */
char regional_miles(void);                       /* '.' (es, pt_BR) o ',' (en) */
int  formatar_inteiro_regional(char *dst, size_t max, long n);                              /* 1234567 -> "1.234.567" / "1,234,567" */
int  formatar_decimal_regional(char *dst, size_t max, long mantisa, int decimales, bool miles);   /* (12345, 2) -> "123,45" / "123.45" */
int  formatar_data_regional(char *dst, size_t max, int dia, int mes, int ano);              /* dd/mm/aaaa (es, pt_BR) o mm/dd/aaaa (en) */

/* ---- entrada ---- */
int  ler_linha(int fd, char *buf, unsigned max);   /* tamaño, o -1 al final del archivo */

int principal(int argc, char **argv);

/* ---- hora y zona horaria (D080): el kernel da el UTC; la hora local se
 * calcula aquí con la zona del proceso (la variable TZ, si la hay; si no, la
 * del sistema). fuso_data_t y las funciones fuso_* vienen de fuso.h. ---- */
#include "fuso.h"
typedef struct {
    const char *nome;      /* "Europe/Madrid" */
    const char *mostrar;   /* "España (Madrid, península)" */
    const char *tz;        /* "CET-1CEST,M3.5.0,M10.5.0/3" */
} zona_info_t;
uint32_t hora_utc(hora_t *h);                       /* segundos UTC desde 1970; h puede ser NULL */
int      ajustar_hora(uint32_t utc, int opcoes);    /* HORA_GRAVAR_RTC: graba también el reloj de la placa */
int      ler_rtc(data_t *d);                        /* lo que tiene grabado el RTC, sin conversión */
int      ler_zona(config_zona_t *z);
int      fixar_zona(const config_zona_t *z);        /* aplica en el kernel (no graba /etc/zona) */
int      datas_arquivo(const char *caminho, datas_arquivo_t *d);   /* UTC; 0 = sin fecha */
void     hora_local(uint32_t utc, fuso_data_t *d);  /* UTC -> hora local del proceso */
void     agora_local(fuso_data_t *d);
uint32_t local_para_utc(uint32_t local, int *situacao);   /* FUSO_NORMAL, FUSO_REPETIDA, FUSO_INEXISTENTE */
const fuso_t *zona_do_processo(const char **nome);  /* nombre: "Europe/Madrid", la TZ o "UTC" */
int      formatar_data(char *s, int max, const char *fmt, const fuso_data_t *d);   /* %Y-%m-%d %H:%M:%S %Z... */
const zona_info_t *zona_tabela(int *n);
const zona_info_t *zona_achar(const char *nome);    /* sin distinguir mayúsculas; NULL = no está */

/* ---- utilidades de los comandos del terminal (utilitarios.c y regex.c, D097) ---- */
/* Análisis de opciones al estilo getopt. "curtas" es como en getopt ("nc:" = -n sin valor,
 * -c con valor). Las opciones pueden ir después de los operandos (se reordenan);
 * al acabar (-1), los operandos empiezan en o.indice. Devuelve la letra de la opción,
 * '?' (desconocida), ':' (falta el valor), OPC_AJUDA (--help o --ayuda) y, con
 * o.numerica, '#' para "-5" (el número en o.valor). */
#define OPC_AJUDA 256
typedef struct { const char *nome; int curta; } opcao_longa_t;   /* lista terminada con nome = NULL */
typedef struct {
    int indice;
    const char *valor;
    const char *errada;
    const opcao_longa_t *longas;
    bool numerica;
    const char *_pos;
} opcoes_t;
int  obter_opcao(int argc, char **argv, const char *curtas, opcoes_t *o);
int  opcao_invalida(const char *cmd, int r, const opcoes_t *o);   /* escribe el error; devuelve 2 */
bool texto_para_numero(const char *s, int *v);                    /* decimal sin signo; false si no lo es */
int  abrir_entrada(const char *cmd, const char *caminho);         /* "-" = entrada estándar; avisa si falla */
void fechar_entrada(int fd);
typedef struct { int fd, ini, fim; bool latin1, acabou; char buf[2048]; } leitor_linhas_t;
void leitor_iniciar(leitor_linhas_t *l, int fd, bool latin1);            /* latin1: convierte UTF-8 de los archivos */
int  leitor_linha(leitor_linhas_t *l, char *dst, int max, bool *com_salto);   /* largo sin \n; -1 = fin */
char *ler_todo_fd(int fd, unsigned *tam);                            /* todo el descriptor, con un 0 al final */
int  saida_linha(const char *s, int n);                           /* línea + \n a la salida; <0 si el pipe se cerró */
bool eh_terminal(int fd);
bool cor_do_ambiente(void);                  /* TERM definido y NO_COLOR no (D263) */
int  cor_opcao(int *argc, char **argv);      /* quita --color[=WHEN]: 0 never, 1 always, 2 auto */
bool cor_para(int fd, int modo);             /* ¿colores ANSI en fd? solo terminal con TERM, o always */
const char *nome_base(const char *caminho);
void caminho_juntar(char *dst, size_t max, const char *pasta, const char *nome);
int  listar_tudo(const char *pasta, entrada_pasta_t **v);         /* ordenadas; libera(*v) después */
void formatar_tamanho(char *s, size_t max, uint32_t bytes);       /* 1234 -> "1.2K" */
bool casar_glob(const char *padrao, const char *texto);           /* * ? [abc] [!a] */
int  ordenar_ponteiros(void **v, int n, int (*cmp)(const void *, const void *));   /* estable */
/* Expresiones regulares (regex.c): ERE con . * + ? | ( ) [] ^ $ \w \s \d \b; con
 * extendida = false funcionan como BRE de GNU (+ ? | ( ) literales; \+ \? \| \( \) operan). */
#define ER_MAX_INST 400
#define ER_MAX_CLASSES 24
typedef struct { unsigned char op; short x, y; } er_inst_t;
typedef struct {
    er_inst_t inst[ER_MAX_INST];
    unsigned char classes[ER_MAX_CLASSES][32];
    int n, nclasses;
    bool ignorar;
    char erro[48];
} er_t;
int  er_compilar(er_t *r, const char *padrao, bool estendida, bool ignorar);   /* 0 = ok; si no, r->erro */
bool er_buscar(const er_t *r, const char *texto, int tam, int desde, int *ini, int *fim);   /* primera coincidencia */

/* La precisión de la x87 (D236). Todo proceso empieza a 53 bits (X87_53_BITS,
 * la de un double, como el SSE2): lo deja así el kernel en el estado limpio de
 * la FPU. Quien calcula en long double y necesita los 64 bits de mantisa (el
 * cc y el Pascal al leer constantes reales) pone X87_64_BITS. Devuelve la
 * palabra de control que había. Solo en programas que ya usan la x87: en una
 * CPU sin FPU, la instrucción termina el programa. */
#define X87_53_BITS 0x027F   /* excepciones enmascaradas, al más próximo, 53 bits */
#define X87_64_BITS 0x037F   /* lo mismo con 64 bits: la del FNINIT */
static inline uint16_t x87_precisao(uint16_t cw)
{
    uint16_t antes;
    __asm__ volatile("fnstcw %0" : "=m"(antes) : : "memory");
    __asm__ volatile("fldcw %0" : : "m"(cw) : "memory");
    return antes;
}

#endif
