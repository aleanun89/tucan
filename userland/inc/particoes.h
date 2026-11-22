// SPDX-License-Identifier: GPL-2.0-or-later
/* particoes.h - tabla de particiones MBR (primarias, extendida y lógicas), GPT en
 * solo lectura, formateo FAT32/ext2 y deshacer (D179).
 *
 * Lo usan "particiones" (terminal), la ventana de "discos" y el instalador. Todo va
 * por SIS_DISCO_BLOCOS: el kernel pone las reglas de seguridad (reserva, nada
 * montado, disco de arranque, GPT, tabla coherente en el sector 0); aquí se
 * repiten y se amplían antes de escribir nada (pt_validar).
 *
 * Los sectores son de 512 bytes y absolutos dentro del disco, salvo donde se dice.
 * D266: una GPT se lee con comprobación de CRC (gpt.h) y se edita (crear, borrar, formatear): sus
 * entradas de las ranuras 0 a 3 aparecen también como primarias 1 a 4 (tipo MBR equivalente), así
 * que la ventana y el terminal las tratan como las del MBR. */
#ifndef SAGUI_PARTICOES_H
#define SAGUI_PARTICOES_H
#include "sagui.h"

#define PT_MAX_LOG   16          /* como el kernel (MAX_LOGICAS) */
#define PT_MAX_GPT   32
#define PT_MB        2048u       /* sectores en 1 MB: la alineación de todo */

enum { PT_SEM_TABELA, PT_MBR, PT_GPT, PT_VOLUME };   /* esquema del disco (PT_VOLUME = FAT32/ext sin tabla) */

typedef struct {
    uint8_t  tipo;               /* tipo MBR; 0 = entrada libre */
    bool     ativa;              /* 0x80 */
    uint32_t inicio, setores;    /* absolutos */
    uint32_t ebr;                /* lógicas: dónde está su EBR */
} pt_part_t;

typedef struct {
    uint32_t inicio, setores;
    char     tipo[40];           /* el GUID de tipo, conocido o en texto */
    char     nome[40];           /* el nombre de la entrada (UTF-16 -> Latin-1) */
    int      slot;               /* la entrada de la tabla (0..127); el número de partición es slot + 1 */
    int      numero;             /* el número con que la ve el kernel (hdb2 = 2), 0 si no la ve (D266) */
    uint8_t  tipo_mbr;           /* el tipo equivalente del MBR (0xEF = ESP, 0xDA = SaguiOS...) */
} pt_gpt_t;

typedef struct {
    char      disco[12];
    uint32_t  setores;
    uint32_t  esc;               /* D283: sectores de 512 por bloque real del disco (1, u 8 en un NVMe de 4096): el MBR cuenta en bloques */
    int       esquema;
    uint8_t   setor0[512];       /* el sector 0 leído (el código de arranque se conserva) */
    pt_part_t prim[4];           /* por ranura: número 1 a 4 */
    int       nlog;
    pt_part_t log[PT_MAX_LOG];   /* en el orden de la cadena: números 5, 6... */
    int       ngpt;
    pt_gpt_t  gpt[PT_MAX_GPT];   /* GPT (D266): todas las entradas; las de las ranuras 0 a 3 también van en prim[] */
    bool      gpt_mala;          /* el MBR dice GPT pero no hay una GPT válida (ni la principal ni el respaldo) */
    bool      gpt_respaldo;      /* se leyó el respaldo: la cabecera principal está dañada */
    bool      gpt_sem_respaldo;  /* la principal vale, pero el respaldo no */
    char      gpt_aviso[160];    /* el motivo si gpt_mala */
} pt_tabela_t;

typedef struct {
    uint32_t inicio, setores;    /* ya alineados a 1 MB */
    bool     logica;             /* dentro de la extendida */
} pt_livre_t;

/* Acceso en bruto (chamadas.c). */
int blocos_info(const char *nome, info_bloco_t *i);
int blocos_reservar(const char *disco, int conf);
int blocos_liberar(const char *disco);
int blocos_reler(const char *disco);
int blocos_ler(const char *nome, uint32_t lba, uint32_t n, void *buf);        /* n sin límite: va por trozos */
int blocos_escrever(const char *nome, uint32_t lba, uint32_t n, const void *buf);
int blocos_descartar(const char *nome, uint32_t lba, uint32_t n);
int blocos_zerar(const char *nome, uint32_t lba, uint32_t n);                 /* escribe ceros */

/* Tabla. */
int  pt_ler(const char *disco, pt_tabela_t *t);
bool pt_validar(const pt_tabela_t *t, char *erro, int max);
int  pt_gravar(pt_tabela_t *t, const char *descricao, char *erro, int max);   /* el disco tiene que estar reservado */
int  pt_extendida(const pt_tabela_t *t);                      /* ranura (0..3) o -1 */
pt_part_t *pt_achar(pt_tabela_t *t, int numero);              /* 1..4 primarias, 5.. lógicas */
void pt_nome_particao(const char *disco, int numero, char *s, int max);       /* "hdb5", "usb0p1" */
int  pt_livres(const pt_tabela_t *t, pt_livre_t *v, int max);
const char *pt_nome_tipo(uint8_t tipo);
bool pt_tipo_extendida(uint8_t t);

/* Operaciones sobre la tabla en memoria (no escriben nada; luego pt_gravar).
 * Devuelven el número de la partición nueva o un error (y el texto en erro). */
int pt_criar(pt_tabela_t *t, uint32_t inicio, uint32_t setores, uint8_t tipo, bool logica, char *erro, int max);
int pt_apagar(pt_tabela_t *t, int numero, char *erro, int max);
int pt_redimensionar(pt_tabela_t *t, int numero, uint32_t setores, char *erro, int max);
int pt_ativar(pt_tabela_t *t, int numero, char *erro, int max);
int pt_mudar_tipo(pt_tabela_t *t, int numero, uint8_t tipo, char *erro, int max);

/* Deshacer: cada pt_gravar guarda los sectores de antes en /tmp/particiones/<disco>.deshacer. */
int  pt_deshacer(const char *disco, char *msg, int max);
int  pt_passos_deshacer(const char *disco);                   /* cuántos cambios se pueden deshacer */

/* Sistemas de archivos (formatacao.c). "volume" es el nombre de la partición. */
typedef void (*fs_progresso_t)(int por_cento, void *ctx);
int  fs_formatar_fat32(const char *volume, const char *rotulo, fs_progresso_t p, void *ctx, char *erro, int max);
int  fs_formatar_exfat(const char *volume, const char *rotulo, fs_progresso_t p, void *ctx, char *erro, int max);   /* D325 */
int  fs_formatar_ext2(const char *volume, const char *rotulo, fs_progresso_t p, void *ctx, char *erro, int max);
/* D338: poner a cero [lba, lba+n) de un volumen sin reescribir lo que ya es cero (lee antes, salvo
 * en un disco con TRIM o si lo que va leyendo casi nunca es cero). "z" a cero la primera vez. */
typedef struct { bool decidido, so_escrever; uint32_t lidos, sujos, escritos; } fs_zerar_t;
int  fs_zerar(const char *volume, uint32_t lba, uint32_t n, fs_zerar_t *z);
/* Después de cambiar el tamaño de la partición: un FAT32 crece hasta donde le da la FAT; si no
 * se puede, se dice por qué. Encoger un sistema de archivos no se hace (D179). */
int  fs_crescer_fat32(const char *volume, char *msg, int max);
/* Tamaño que ocupa el sistema de archivos de un volumen (sectores), 0 si no se reconoce. */
uint32_t fs_tamanho(const char *volume, char *tipo, int max_tipo);

/* D213 (redimensao.c). Comprobar sin escribir nada (un fsck de solo lectura) un FAT32 o un
 * ext2: 0 si está como para tocarlo; si no, el error y el motivo en msg. */
int  fs_verificar(const char *volume, char *msg, int max);
/* Cambiar el tamaño de un ext2 para que quepa en "setores" (crecer o encoger, sin mover
 * datos: encoger se niega si lo que se corta está en uso). La partición ya tiene que medir
 * al menos eso. */
int  fs_redimensionar_ext2(const char *volume, uint32_t setores, fs_progresso_t p, void *ctx, char *msg, int max);
/* D388: lo que se va a escribir al crecer un ext2 (o al formatear uno nuevo): grupos nuevos y MB de tablas de
 * inodos y mapas. Es un máximo (lo que ya es cero no se reescribe). */
typedef struct { uint32_t grupos, mb; } fs_ext2_est_t;
int  fs_ext2_estimar(const char *volume, uint32_t setores, fs_ext2_est_t *est);
int  fs_ext2_estimar_novo(uint32_t setores, fs_ext2_est_t *est);
uint32_t fs_ext2_maximo(const char *volume);   /* hasta dónde crece sin rehacerlo (sectores; 0 = no se reconoce) */
/* Lo mínimo (sectores, múltiplo de 1 MB) a lo que se puede encoger un ext2 sin mover datos. */
uint32_t fs_ext2_minimo(const char *volume);
/* D318: encoger un FAT32 sin mover datos (solo si lo que se corta está libre y quedan al menos
 * 65.525 clústeres); lo mínimo de un FAT32 y de cualquiera de los dos (0 = no se reconoce). */
int      fs_reduzir_fat32(const char *volume, uint32_t setores, char *msg, int max);
uint32_t fs_fat32_minimo(const char *volume);
uint32_t fs_minimo(const char *volume);

/* D397 (reparo_ext2.c): reparar un ext2 que quedó mal tras un corte (lo que hace e2fsck con -y, con una
 * implementación propia). reparo_analisar lo recorre entero SIN ESCRIBIR NADA: todo lo que habría que
 * cambiar se prepara en memoria y se cuenta (cada problema, por aviso, una línea). Devuelve cuántos
 * problemas hay (0 = está bien) o un error (con el motivo en msg: un volumen que no sabe tratar, ext3/ext4,
 * la raíz destruida...). Con lo analizado: reparo_resumo (el resumen y los archivos tocados, por aviso),
 * reparo_guardar_copia (los bloques que se van a cambiar, tal y como están ahora, a un archivo),
 * reparo_gravar (lo escribe en un orden seguro: el disco tiene que estar reservado y el volumen sin montar)
 * y reparo_fechar. reparo_desfazer vuelve a poner una copia. */
typedef struct reparo reparo_t;
typedef void (*reparo_aviso_t)(const char *linha, void *ctx);
int      reparo_analisar(const char *volume, reparo_aviso_t aviso, void *ctx, reparo_t **r, char *msg, int max);
uint32_t reparo_problemas(const reparo_t *r);
uint32_t reparo_arriscados(const reparo_t *r);   /* arreglos que pierden algo (preen: no se hacen solos) */
uint32_t reparo_blocos(const reparo_t *r, uint32_t *bytes);   /* bloques que cambian (y lo que ocupa su copia) */
void     reparo_resumo(reparo_t *r);
int      reparo_guardar_copia(reparo_t *r, const char *arquivo, char *msg, int max);
int      reparo_gravar(reparo_t *r, char *msg, int max);
void     reparo_fechar(reparo_t *r);
int      reparo_desfazer(const char *arquivo, const char *volume, char *msg, int max);

/* Una operación completa (particoes_acoes.c): reserva el disco, cambia, verifica, escribe,
 * guarda el deshacer y suelta el disco (el kernel vuelve a leer la tabla). La usan el
 * terminal y la ventana, así las dos hacen exactamente lo mismo.
 * D213: la reserva se pide con DBLOCO_CONF_MONTADO: lo que esté montado (y la partición de
 * SaguiOS del disco de arranque) no se toca, pero el resto del disco sí; una operación sobre
 * una partición montada se niega con un mensaje que dice cuál desmontar. */
enum { PA_CRIAR, PA_APAGAR, PA_REDIMENSIONAR, PA_FORMATAR, PA_ATIVAR, PA_TRIM, PA_DESHACER, PA_TABELA_NOVA,
       PA_AMPLIAR,     /* D213: la extendida hasta el final del disco */
       PA_TABELA_GPT };   /* D266: una tabla GPT vacía (con su MBR protector) */
enum { PA_FS_NENHUM, PA_FS_FAT32, PA_FS_EXT2, PA_FS_EXFAT };
typedef struct {
    int      op;
    char     disco[12];
    int      numero;             /* la partición (1..4 primarias, 5.. lógicas) */
    uint32_t inicio, setores;    /* crear: 0 = el primer hueco / todo el hueco; redimensionar: el tamaño nuevo */
    uint8_t  tipo;               /* crear: tipo MBR (0 = según el sistema de archivos) */
    bool     logica;             /* crear: dentro de la extendida */
    int      fs;                 /* crear (opcional) y formatear: PA_FS_* */
    char     rotulo[20];
    bool     trim;               /* borrar: además, descartar el contenido (sin vuelta atrás) */
    int      conf;               /* DBLOCO_CONF_* (el disco de arranque pide la confirmación doble) */
    fs_progresso_t progresso;
    void    *ctx;
    int      resultado;          /* crear: el número de la partición nueva */
} pt_acao_t;
int  pt_executar(pt_acao_t *a, char *msg, int max);

/* D318 (particoes_tamanho.c): cambiar el tamaño de una partición hasta donde haga falta. Para
 * crecer más allá de la lógica que la sigue, las lógicas de detrás se copian al final del disco
 * (a espacio libre que no se solapa con el suyo: si no cabe, no se mueve nada) y se releen antes
 * de cambiar la tabla; después la partición crece y luego su sistema de archivos. Encoger es
 * PA_REDIMENSIONAR (primero el sistema de archivos). */
typedef struct {
    char     disco[12];
    int      numero;
    uint32_t antes, depois;            /* tamaño de ahora y el final (sectores) */
    uint32_t maximo;                   /* lo máximo posible (moviendo, si se permite) */
    uint32_t maximo_sem_mover;
    uint32_t ext_fim;                  /* 0 = la extendida no cambia; si no, su final nuevo */
    int      nmover, nmover_possivel;  /* lógicas que se mueven (y las que habría que mover para el máximo) */
    int      mover[PT_MAX_LOG];        /* sus números (5..) */
    uint32_t novo_ini[PT_MAX_LOG];     /* su inicio nuevo */
    uint32_t mover_setores;            /* lo que hay que copiar */
    uint32_t copiados;
    fs_progresso_t progresso;
    void    *ctx;
} pt_tamanho_t;
/* Solo cálculos (no lee ni escribe el disco). setores = 0: lo máximo. */
int  pt_planejar_tamanho(const pt_tabela_t *t, int numero, uint32_t setores, bool mover, pt_tamanho_t *p, char *erro, int max);
/* Lo hace. conf: DBLOCO_CONF_ARRANQUE en el disco de arranque. Con desmontar_permitido, lo montado
 * que se toca se desmonta y se vuelve a montar donde estaba al acabar (también si falla). */
int  pt_mudar_tamanho(pt_tamanho_t *p, int conf, bool desmontar_permitido, char *msg, int max);
bool pt_volume_de_ponto(const char *ponto, char *vol, int max);   /* "/home" -> "hda5" */
/* D388: la raíz doble de la live. «/» es la SAGUI-RAIZ (el sistema de serie, de solo lectura) con la capa de
 * cambios encima (SAGUI-PERSIST, o la RAM): las dos particiones salen «montadas en /». pt_raiz_live dice si
 * «/» es una live (y, si sus punteros no son NULL, los volúmenes de la base y de la capa: "" si la capa va en
 * la RAM); pt_papel_raiz, qué es una partición montada en «/» ("sistema de serie (base de /, solo lectura)",
 * "tus cambios (capa de /)", "raíz del sistema") o NULL si no lo está. */
bool pt_raiz_live(char *base, char *capa, int max);
const char *pt_papel_raiz(const info_disco_t *d);
/* D388: la primera partición del plan (la que cambia o una que se mueve) montada en «/» (la raíz en uso:
 * SAGUI-RAIZ, o SAGUI-PERSIST en la live), con su nombre en vol; 0 si ninguna. pt_mudar_tamanho se niega. */
int  pt_tamanho_raiz_em_uso(const pt_tamanho_t *p, char *vol, int max);

/* D318 (home_ampliar.c): «ampliar /home a todo el espacio libre» en el disco de arranque. Si /home
 * es una partición, crece con pt_mudar_tamanho; si está en la RAM, se crea una lógica ext2
 * SAGUI-DATOS con todo el hueco, se copia /home y se monta. */
typedef struct {
    char         disco[12];      /* el de arranque */
    char         vol[16];        /* la partición de /home ("" si está en la RAM) */
    bool         criar;          /* no hay partición: se crea */
    bool         ext_ampliar;    /* criar: la extendida crece antes */
    uint32_t     criar_ini, criar_set;
    uint32_t     ganho;          /* sectores que gana /home (0 = nada que hacer) */
    uint32_t     total_novo;     /* el tamaño final de /home */
    pt_tamanho_t plano;          /* !criar */
    /* D388: el espacio está, pero no se puede usar sin mover la raíz en uso (la disposición de antes de
     * la v8.5.4.10r2 en la live o en un sistema instalado): home_ampliar se niega y msg lo explica. */
    bool         bloqueado;
    /* D388: el ext2 de /home no puede crecer hasta el final del disco (poca reserva de la tabla de grupos, imágenes
     * grabadas con bloques de 1 KB): se ofrece el máximo posible y se dice. limite_fs = ese máximo (sectores). */
    bool         limitado;
    uint32_t     limite_fs;
} home_plano_t;
int home_planejar(home_plano_t *h, char *msg, int max);
/* D388: lo mismo pero con /home de "total" sectores como mucho (0 = todo lo posible): el tamaño menor que elige
 * el usuario. Rehace el plan entero (h se pisa). */
int home_planejar_ate(home_plano_t *h, uint32_t total, char *msg, int max);
/* D388: la estimación del trabajo antes de empezar: grupos nuevos, MB que se escribirán como mucho, la velocidad
 * de escritura que se calcula (MB/s, medida leyendo unos MB del disco, con prudencia) y los segundos. La
 * velocidad se mide una vez (mbs = 0) y se pasa de vuelta en las siguientes. */
typedef struct { uint32_t grupos, mb, mbs, segundos; } home_estim_t;
int home_estimar(const home_plano_t *h, uint32_t mbs, home_estim_t *e);
/* "unos 3 min", "menos de 1 min", "1 h 20 min": para los textos de estimación y de tiempo restante. */
void home_texto_tempo(uint32_t segundos, char *s, int max);
int home_ampliar(home_plano_t *h, fs_progresso_t p, void *ctx, char *msg, int max);
bool pt_separar_nome(const char *nome, char *disco, int max, int *numero);   /* "hdb5" -> "hdb", 5 */
const char *pt_nome_fs(int fs);

#endif
