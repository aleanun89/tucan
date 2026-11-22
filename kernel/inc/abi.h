// SPDX-License-Identifier: GPL-3.0-only
/* abi.h - o contrato entre o kernel e os programas do SaguiOS.
 *
 * Este arquivo é incluído pelo kernel e pela libsagui. Convenção (PRD 4.5):
 * "int 0x80", número da chamada em EAX, argumentos em EBX, ECX, EDX (e ESI,
 * EDI quando precisa de mais), resultado em EAX; negativo = erro.
 * Os números são agrupados como no livro do Tanenbaum. */
#ifndef SAGUI_ABI_H
#define SAGUI_ABI_H

/* Processos */
#define SIS_EXECUTAR        1   /* (caminho, argv, envp ou 0) -> pid; envp = "NOME=valor"... NULL (D037) */
#define SIS_ESPERAR         2   /* (pid ou -1, &codigo, sem_bloquear) -> pid (0 = ninguém terminou) */
#define SIS_SAIR            3   /* (codigo) */
#define SIS_PID             4   /* () -> pid */
#define SIS_MATAR           5   /* (pid) */
/* Arquivos */
#define SIS_ABRIR          10   /* (caminho, modo) -> fd */
#define SIS_LER            11   /* (fd, buf, n) -> lidos (0 = fim) */
#define SIS_ESCREVER       12   /* (fd, buf, n) -> escritos */
#define SIS_FECHAR         13   /* (fd) */
#define SIS_POSICIONAR     14   /* (fd, desloc, origem) -> nova posição */
#define SIS_INFO_ARQUIVO   15   /* (caminho, &info_arquivo) */
/* Diretórios */
#define SIS_CRIAR_PASTA    20   /* (caminho) */
#define SIS_REMOVER_PASTA  21   /* (caminho) */
#define SIS_APAGAR         22   /* (caminho) */
#define SIS_LISTAR         23   /* (caminho, indice, &entrada_pasta) -> 1 achou, 0 fim */
#define SIS_MUDAR_PASTA    24   /* (caminho) */
#define SIS_PASTA_ATUAL    25   /* (buf, n) */
#define SIS_RENOMEAR       26   /* (origem, destino) */
/* Comunicação */
#define SIS_PIPE           30   /* (int fds[2]) */
#define SIS_DUPLICAR       31   /* (fd_antigo, fd_novo) -> fd_novo */
#define SIS_DISPONIVEL     32   /* (fd) -> bytes num pipe sem bloquear; ERRO_PIPE_QUEBRADO = acabou */
/* Diversas */
#define SIS_DORMIR         40   /* (ms) */
#define SIS_TEMPO          41   /* () -> milissegundos desde o boot */
#define SIS_DATA           42   /* (&data_hora) */
#define SIS_INFO_SISTEMA   43   /* (tipo, buf, n) */
#define SIS_AUMENTAR_MEM   44   /* (bytes) -> fim antigo da área de dados */
#define SIS_REINICIAR      45   /* () */
#define SIS_MODO_TERMINAL  46   /* (TERM_LINHA ou TERM_BRUTO) */
#define SIS_LIMPAR_TELA    47   /* () */
#define SIS_REGISTRAR      48   /* (texto) -> escreve no log do sistema (serial), não na tela */
/* Janelas (fase 5) */
#define SIS_CRIAR_JANELA   50   /* (titulo, largura | altura << 16, &pixels) -> id */
#define SIS_FECHAR_JANELA  51   /* (id) */
#define SIS_ATUALIZAR_JANELA 52 /* (id, &retangulo ou 0 = tudo) */
#define SIS_PROXIMO_EVENTO 53   /* (id ou 0 = qualquer janela minha, &evento, esperar) -> 1 se houve; esperar =
                                   0 (não espera), 1 (sem prazo) ou EVENTO_PRAZO | instante em ms (D083) */
#define SIS_TITULO_JANELA  54   /* (id, titulo) */
/* Dispositivos (fase 6) */
#define SIS_LISTAR_DISCOS  55   /* (indice, &info_disco) -> 1 achou, 0 fim */
#define SIS_MONTAR         56   /* (nome, ponto, opcoes): ponto 0 = forma antiga, monta em /usb (D040) */
#define SIS_DESMONTAR      57   /* (nome do disco ou pasta montada) -> grava o que falta e libera */
/* Som (bônus B3) */
#define SIS_BIP            58   /* (&notas, quantidade, modo) -> volta na hora; o tique do relógio toca */
/* Jogos (bônus B4) */
#define SIS_TECLAS_SEGURADAS 59 /* (&mapa[32]) -> 1 se a janela do processo está em foco */
/* Teclado */
#define SIS_TECLADO        60   /* (TECLADO_ES, TECLADO_ABNT2 ou -1 = só consultar) -> layout atual */
/* Terminal virtual (D036) */
#define SIS_ABRIR_TV       61   /* (int fds[2]) -> fds[0] = mestre (tela), fds[1] = escravo (programas) */
#define SIS_PRIMEIRO_PLANO 62   /* (grupo ou 0) -> anterior; o Ctrl+C no terminal da entrada 0 encerra esse grupo */
/* Shell completo (D037) */
/* Aleatoriedade (D061): RDRAND/RDSEED se houver, senão TSC + jitter das IRQ, num reservatório SHA-256 */
#define SIS_ALEATORIO      63   /* (buf, n) -> n bytes imprevisíveis (no máximo 4096 por chamada) */
#define SIS_INFO_ALEATORIO 64   /* (&info_aleatorio_t) -> 0 */

#define SIS_GRUPO          70   /* (pid ou 0 = eu, grupo ou 0 = o próprio pid) -> põe o processo (eu ou um filho) num grupo */
#define SIS_INTERRUPCOES   71   /* () -> quantos Ctrl+C o terminal da entrada 0 já recebeu */
#define SIS_TIPO_DESCRITOR 72   /* (fd) -> TIPO_TERMINAL, TIPO_PIPE ou TIPO_ARQUIVO */
/* Janelas de tamanho variável e tela cheia (D044) */
#define SIS_ESTILO_JANELA  78   /* (id, estilo JAN_*, min_largura | min_altura << 16) */
#define SIS_REDIMENSIONAR_JANELA 79 /* (id, largura | altura << 16, &pixels) -> 0; os pixels ficam no mesmo endereço */
/* Administrador de tareas (D043) */
#define SIS_PEDIR_FECHAR   75   /* (id da janela) -> entrega EV_FECHAR ao dono, como o X da moldura */
/* Modos de vídeo (D058, D059) */
#define SIS_MODOS_VIDEO    76   /* (modo_video_t[], n) -> quantos modos copiou */
#define SIS_MUDAR_MODO     77   /* (largura | altura << 16, MUDAR_*) -> MUDAR_* do que foi feito, ou erro */
/* Rede (D046). Faixa 80-89 reservada para a rede. IPs como número na ordem
 * "humana": 10.0.2.15 = 0x0A00020F. */
#define SIS_SOCKET         80   /* (SOCK_TCP ou SOCK_UDP) -> fd (ler/escrever/fechar valem nele) */
#define SIS_CONECTAR       81   /* (fd, ip, porta) -> 0; TCP: abre a conexão; UDP: fixa o destino */
#define SIS_ESCUTAR        82   /* (fd, porta, fila) -> 0; TCP: espera conexões; UDP: recebe nessa porta */
#define SIS_ACEITAR        83   /* (fd, &endereco_rede_t ou 0) -> fd da conexão que chegou */
#define SIS_ENVIAR_PARA    84   /* (fd, buf, n, ip, porta) -> n (UDP) */
#define SIS_RECEBER_DE     85   /* (fd, buf, n, &endereco_rede_t ou 0) -> n (UDP: um datagrama) */
#define SIS_RESOLVER       86   /* (nome, &ip, servidor ou 0, porta ou 0) -> 0; pergunta ao DNS */
#define SIS_CONFIG_REDE    87   /* (REDE_*, &config_rede_t) -> 0 */
#define SIS_PING           88   /* (ip, &info_ping_t, prazo_ms) -> 0; eco ICMP */
#define SIS_OPCAO_SOCKET   89   /* (fd, OPC_*, valor) -> depende da opção */
/* Menu de contexto (D051) */
#define SIS_MENU_CONTEXTUAL 90  /* (id, x, y, itens, codigo) -> 0; mostra um menu sobre a janela (x, y relativos
                                   ao conteúdo). itens = textos separados por '\n'; "-" = separador; "~Texto" =
                                   desativado; "Texto\tAtalho" = atalho alinhado à direita; "*Texto" =
                                   marcado (a opção atual); ">Texto" abre um submenu com as linhas seguintes,
                                   até uma linha "<" (D064). O índice da resposta conta todas as linhas,
                                   ">" e "<" inclusive. A resposta chega depois como EV_MENU. */
/* Hora e fuso horário (D080, D081). O kernel guarda a hora em UTC. */
#define SIS_HORA           91   /* (HORA_*, arg, arg2): ver abaixo */
#define SIS_ZONA           92   /* (ZONA_LER ou ZONA_FIXAR, &config_zona_t) -> 0 */
#define SIS_DATAS_ARQUIVO  93   /* (caminho, &datas_arquivo_t) -> 0; datas em UTC (0 = desconhecida) */
/* Área de transferência (D074): um texto Latin-1 de até PORTAPAPEIS_MAX bytes, o último copiado */
#define SIS_COPIAR_TEXTO   94   /* (buf, n) -> bytes guardados (corta em PORTAPAPEIS_MAX; n = 0 esvazia) */
#define SIS_COLAR_TEXTO    95   /* (buf, max) -> tamanho do texto guardado; copia min(tamanho, max) bytes, sem zero */
#define PORTAPAPEIS_MAX    65536
/* Som de verdade: placa AC'97 ou Intel HDA e o misturador do kernel (D088-D090).
 * Um fluxo é um descritor: escrever() nele também toca (bloqueando) e
 * fechar() descarta o que faltava tocar. */
#define SIS_AUDIO_ABRIR    96   /* (taxa em Hz, canais | bits << 8) -> fd; ERRO_NAO_EXISTE = sem placa de som */
#define SIS_AUDIO_ESCREVER 97   /* (fd, buf, n, sem_bloquear) -> bytes aceitos (só quadros inteiros; 0 = cheio) */
#define SIS_AUDIO_CONTROLE 98   /* (fd ou -1, AUDIO_*, valor) -> depende da operação (abaixo) */
#define SIS_AUDIO_FECHAR   99   /* (fd, esperar) -> 0; esperar = 1: volta só depois de tocar tudo */
#define SIS_MOVER_JANELA  100   /* (id, x, y) -> 0; posição da moldura na tela (D102); x ou y < 0: relativo à borda
                                   direita/de baixo (-1 = encostada). Fora do desktop: ERRO_ACESSO */
#define SIS_TRUNCAR       111   /* (fd, novo tamanho) -> 0; reduz (libera) ou aumenta (rellena con ceros) (D116) */
#define SIS_SINCRONIZAR   112   /* (fd) -> 0; fuerza a disco lo pendiente del archivo (D116) */
#define SIS_FREQUENCIA    101   /* (FREQ_OP_*, arg) -> 0 o ERRO_*; control de la frecuencia de la CPU (D106).
                                   La consulta del estado es INFO_FREQUENCIA */
/* Diagnóstico y ajuste manual del códec HDA (D107): solo con el driver hda. */
#define SIS_AUDIO_HDA     102   /* (HDA_OP_*, a, b) -> depende de la operación (abajo); ERRO_NAO_EXISTE sin HDA */
/* Aparência do escritório (D112): fundo escolhido e cor de destaque. */
#define SIS_FUNDO         103   /* (FUNDO_*, ptr, y, n): imagem de fundo ja escalada para a tela, por faixas de linhas (abaixo) */
#define SIS_ACENTO        104   /* (rgb 0xRRGGBB ou -1 = so consultar) -> cor de destaque atual (barra de titulo e de tarefas) */
/* Apagado por ACPI e botão de energia (D113). */
#define SIS_DESLIGAR      105   /* () -> não volta: grava os discos, ACPI S5; sem ACPI, "Ya puedes apagar el PC" */
#define SIS_ACPI          106   /* (ACPI_OP_*) -> 0; ERRO_NAO_EXISTE = sem ACPI utilizável */
#define ACPI_OP_DESATIVAR   1   /* não usar mais o ACPI (apagar mostra a mensagem; o botão deixa de valer) */
#define ACPI_OP_ATIVAR      2   /* volta a usar (se as tabelas conferem e não foi a opção de arranque) */
/* Servidores con muchos sockets (lite-monkey, D117). */
#define SIS_ESPERAR_REDE  113   /* (visto, ms) -> contador de eventos de la red (31 bits); si sigue igual a
                                   "visto", duerme hasta que algún socket reciba algo o pasen ms */
#define SIS_CAPTURAR_TELA 114   /* (id|0=pantalla|-1=activa, &int[4] x,y,l,a (entra/sale), destino, max_pixels) -> pixels; la pantalla compuesta, sin cursor (D125) */
#define SIS_AVISAR        115   /* (texto, archivo|0) -> 0; notificación en la esquina; con archivo, al pulsarla se abre en el Visor (D125) */
#define SIS_COPIAR_IMAGEM 116   /* (ancho | alto << 16, píxeles 0x00RRGGBB) -> 0; portapapeles de imagen (D125); 0x0 vacía; máx. PORTAPAPEIS_IMG_PX */
#define SIS_COLAR_IMAGEM  117   /* (destino|0, max_pixels) -> ancho | alto << 16 (0 = no hay imagen); copia si destino != 0 y cabe */
#define PORTAPAPEIS_IMG_PX 2500000
/* Los BAR de un dispositivo PCI en el espacio del programa (D158). El kernel
 * elige qué páginas se mapean (las que el propio dispositivo declara en su BAR)
 * y devuelve la dirección virtual; el programa nunca ve una dirección física. */
#define SIS_MAPEAR_BAR    118   /* (&pedido_bar_t, &direccion) -> 0; la dirección virtual del mapa */
#define SIS_DESEJAR_BAR   119   /* (dirección virtual) -> 0; deshace el mapa de un BAR */
/* Hilos de usuario (D167). Un hilo comparte el espacio, los ficheros, la
 * carpeta y las ventanas del proceso; tiene su pila, sus registros, su FPU y
 * su número (tid), que no es el pid del proceso. */
#define SIS_HILO_CRIAR    120   /* (entrada, topo da pilha, arg) -> tid; con pilha != 0 la pila ya lleva el retorno
                                 * y los argumentos; con pilha = 0 el kernel da una de 1 MB con guarda y pone "arg" (D186) */
#define SIS_HILO_SAIR     121   /* (codigo) -> no vuelve; en el hilo principal, como SIS_SAIR */
#define SIS_HILO_ESPERAR  122   /* (tid, &codigo) -> 0; espera a que el hilo acabe y lo libera */
#define SIS_CEDER         123   /* () -> 0; cede el turno a otro hilo o proceso listo */
#define SIS_FUTEX_ESPERAR 124   /* (&palabra, valor, ms o 0) -> 0 despertado o plazo; 1 si la palabra no vale "valor" */
#define SIS_FUTEX_ACORDAR 125   /* (&palabra, cuantos) -> cuantos despertó */
#define SIS_HILO_ID       126   /* () -> tid del hilo que llama (el pid en el principal) */
/* D174: los planificadores. Una sola llamada con la operación en el primer
 * argumento (PLAN_*), para no gastar números. */
#define SIS_PLANIFICADOR  127   /* (PLAN_*, a, b, c) -> según la operación */
/* D179: acceso en bruto a los sectores de un disco (particiones, formateo, instalador).
 * Una sola llamada con la operación en el primer argumento (DBLOCO_*, abajo). */
#define SIS_DISCO_BLOCOS  128   /* (DBLOCO_*, nombre, a, b, c) -> según la operación */
/* D209: páginas del área de datos. Una sola llamada con la operación en el
 * primer argumento (MEM_*, abajo). Es lo que deja al malloc de la libsagui
 * devolver al sistema la memoria de los bloques grandes que se liberan. */
#define SIS_MEMORIA       129   /* (MEM_*, endereço, bytes) -> según la operación */
/* D215: el Wi-Fi (pincho RTL8188EU). Una llamada con la operación en el primer
 * argumento (WIFI_*, abajo). Buscar y conectar solo piden: la tarea «wifi» del
 * kernel lo hace y el programa consulta WIFI_ESTADO hasta que acaba. */
#define SIS_WIFI          130   /* (WIFI_*, ptr, n) -> según la operación */
/* D216: el gestor de servicios (/bin/servicios). Una sola llamada con la
 * operación en el primer argumento (SERV_*, abajo). */
#define SIS_SERVICOS      131   /* (SERV_*, arg) -> según la operación */

/* D218: el bloqueo de publicidad y rastreo por DNS. Una llamada con la operación
 * en el primer argumento (BLOQ_*, abajo). */
#define SIS_BLOQUEO       132   /* (BLOQ_*, a, b) -> según la operación */
/* Mejora 097 (v8.0b6, VNC): inyectar teclado y ratón como si vinieran del
 * hardware. Solo para el servicio de escritorio remoto (vncd); al resto,
 * error. Operación en el primer argumento (INY_*, abajo). */
#define SIS_INYECTAR      133   /* (INY_*, a, b) -> 0 o error */
/* D255 (mejora 098, fase 1): usuarios. Operación en el primer argumento (USU_*, abajo). */
#define SIS_USUARIO       134   /* (USU_*, a, b) -> según la operación */
/* D279 (v8.2, mejora 059): Bluetooth con un controlador USB estándar (p. ej. CSR8510). Operación en el
 * primer argumento (BT_*, abajo); las que cambian algo piden permisos de administrador (D255, D269). */
#define SIS_BLUETOOTH     135   /* (BT_*, ptr/indice, n) -> según la operación */
/* D290 (v8.3, mejora 060): varias pantallas. Operación en el primer argumento (TELAS_OP_*, abajo). */
#define SIS_TELAS         136   /* (TELAS_OP_*, a, b) -> según la operación */
/* D282 (mejora 098, fase 2): permisos de archivos. Operación en el primer argumento (PARQ_*, abajo). */
#define SIS_PERMISOS_ARQ  137   /* (PARQ_*, a, b, c) -> según la operación */
/* D313 (v8.4, mejora 118): idioma de la interfaz. (IDIOMA_*, ptr, n) -> según la operación.
 * (Era la 137 en la rama de idiomas; pasó a la 138 al chocar con SIS_PERMISOS_ARQ.) */
#define SIS_IDIOMA        138
#define IDIOMA_LER        0   /* (buf, n) -> 0: nombre del idioma actual ("es", "pt_BR", "en", "xx") */
#define IDIOMA_FIXAR      1   /* (texto) -> 0: cambia el idioma, recarga el catálogo del kernel y redibuja el escritorio */
/* D365 (v8.5.3): widgets de escritorio. (titulo, largura | altura << 16, &pixels, opacidad) -> id de ventana.
 * Una ventana «de escritorio»: sin marco ni botón en la barra, siempre por debajo de las ventanas normales
 * y por encima del fondo, sin tomar el foco del teclado. Recibe el ratón (EV_CLIQUE, EV_SOLTOU, EV_RODA) y se
 * coloca con Alt+arrastrar o en el modo «editar widgets»; el kernel avisa con EV_WIDGET. opacidad: bits 0-7 =
 * 1-100 % (0 = 100; por debajo de 100 se mezcla con el fondo); WIDGET_CLAVE (bit 8) = los píxeles con el byte
 * alto a 0xFF (0xFF000000) son transparentes (esquinas redondeadas, fondo «transparente» de Conky). SIS_MOVER_JANELA, SIS_ATUALIZAR_JANELA y SIS_FECHAR_JANELA
 * valen igual; el tamaño es fijo (SIS_ESTILO_JANELA y SIS_REDIMENSIONAR_JANELA dan ERRO_INVALIDO). */
#define SIS_CRIAR_WIDGET  139
#define WIDGET_CLAVE      0x100
/* D401 (v8.5.5.1): procesos POSIX. Ver «Procesos POSIX» más abajo. */
#define SIS_BIFURCAR      140   /* () -> pid del hijo al padre, 0 al hijo (fork con copia al escribir) */
#define SIS_SUBSTITUIR    141   /* (caminho, argv, envp ou 0) -> no vuelve; error si no se pudo cargar (execve) */
#define SIS_ESPERAR_PID   142   /* (pid, &estado ou 0, ESPERA_*) -> pid; 0 con ESPERA_SEM_BLOQUEAR y nadie (waitpid) */
#define SIS_SINAL         143   /* (SINOP_*, a, b, c) -> según la operación (señales) */
#define SIS_SINAL_VOLTAR  144   /* (&contexto_sinal_t) -> no vuelve: restaura el contexto (sigreturn) */
#define SIS_SONDAR        145   /* (&sondagem_t[], n, ms o -1) -> descriptores listos (poll) */
#define SIS_DESCRITOR     146   /* (fd, DESC_*, arg) -> según la operación (fcntl) */
/* D403 (v8.5.5): funciones propias de los portátiles por ACPI (brillo de la pantalla, teclas de función,
 * radio) y el volcado de las tablas ACPI. Operación en el primer argumento (PORTATIL_*, abajo). La 150 y no la
 * 140: las 140-146 son de la rama de procesos POSIX (v8.5.5.1). */
#define SIS_PORTATIL      150   /* (PORTATIL_*, a, b, c) -> según la operación */
#define SIS_ESQUEMA       151   /* D428: (op ESQ_OP_*, ptr, n) esquema de apariencia de todo el sistema (esquemas.h) */
#define SIS_MAX           152

/* SIS_USUARIO (D255). Cada proceso tiene un usuario (uid) y un grupo (gid);
 * los hijos y los hilos los heredan. Solo root (uid 0) los cambia. Lo que
 * puede hacer cada uno lo decide un solo módulo del kernel (permisos.c). */
#define UID_ROOT     0
#define GID_ROOT     0
#define UID_SAGUI    1000          /* el usuario del escritorio (entrada automática) */
#define GID_SAGUI    1000
#define UID_MAX      65535
#define USU_CONSULTAR  0   /* () -> uid | gid << 16 de quien llama */
#define USU_FIJAR      1   /* (uid, gid) -> 0; solo root: cambia la identidad del proceso (y de sus hilos) */
#define USU_COMO       2   /* (uid, gid) -> 0; solo root: el próximo executar() de este proceso crea el hijo
                            * con esa identidad (una sola vez); uid = -1 lo anula */
#define USU_DE_PID     3   /* (pid) -> uid | gid << 16 de ese proceso, o ERRO_SEM_PROCESSO */
#define USU_ESTADO     4   /* () -> USU_EST_* de quien llama y del arranque */
#define USU_EST_RESCATE  0x01   /* el sistema arrancó en modo rescate (shell de root en la consola) */
#define USU_EST_SERVICIO 0x02   /* el proceso viene de un servicio (sshd, vncd, cron...): no es la sesión local */
#define USU_EST_PRUEBA   0x04   /* imagen de prueba con la sesión como root (mkimagem.py --sesion-root) */
#define USU_EST_EN_VIVO  0x08   /* sin la partición SAGUI-CONF: la configuración del sistema no se guarda */
#define USU_EST_SIN_MIGRAR 0x10 /* D371: imagen de prueba que no pasa sagui a /home/sagui (mkimagem.py --sin-migrar-home) */
#define USU_EST_SISTEMA  0x20   /* D378: el programa se cargó de una ruta del sistema que da permisos (/bin, D251/D269): proceso seguro */
/* D340 (mejora 098, fase 3): varias cuentas. */
#define USU_SESSAO     5   /* () -> uid | gid << 16 de la sesión local (la cuenta que entró en el escritorio) */
#define USU_CERRAR_SESION 6 /* (USU_SAIR_*) -> 0: cierra todos los programas de la sesión local y vuelve a la
                            * pantalla de inicio de sesión. Solo desde la sesión local (no un servicio) */
#define USU_SAIR_CERRAR   0   /* «Cerrar sesión» */
#define USU_SAIR_CAMBIAR  1   /* «Cambiar de usuario»: la pantalla empieza en otra cuenta */
#define USU_CASA_SOLTAR 7   /* () -> 0; solo root: el proceso deja la vista de la carpeta de la cuenta de la
                             * que se elevó (D371, P2: «su» es hacerse root del todo, con /root y el árbol real) */

/* SIS_PERMISOS_ARQ (D282). Cada archivo y carpeta tiene dueño (uid), grupo (gid)
 * y 9 bits rwx (dueño, grupo, otros) más el pegajoso; sin ACL. El kernel los
 * comprueba en todas las llamadas de archivos (permisos.c); root lo puede todo.
 * ext2: los del inodo. Árbol en RAM: todo de root salvo /home (de sagui) y /tmp
 * (1777). FAT32, ISO 9660, UDF: los de las opciones del montaje (uid=, gid=,
 * umask=). En /home, lo que es de root se ve como de sagui (un solo usuario). */
#define PARQ_CONSULTAR 0   /* (caminho, &info_permisos_t) -> 0 */
#define PARQ_MODO      1   /* (caminho, modo) -> 0; el dueño o root; modo & 07777 (sin setuid: 01777) */
#define PARQ_DONO      2   /* (caminho, uid o -1, gid o -1) -> 0; el dueño solo root; el grupo, root o el
                            * dueño (a su propio grupo) */
#define PARQ_UMASK     3   /* (nueva o -1) -> la anterior; del proceso, la heredan los hijos (022 al arrancar) */
/* D341: fijar las fechas (UTC). Como utimensat: PARQ_DATA_AGORA = la hora actual, PARQ_DATA_MANTER = no
 * cambia. Una fecha concreta, el dueño o root; «ahora» (o nada) basta con permiso de escritura. Un
 * sistema de solo lectura o sin fechas (ISO 9660, UDF) da ERRO_SO_LEITURA o ERRO_NAO_SUPORTADO. */
#define PARQ_DATAS     4   /* (caminho, modificado, acessado) -> 0 */
#define PARQ_DATAS_FD  5   /* (fd, modificado, acessado) -> 0; el archivo abierto (futimens) */
#define PARQ_DATA_AGORA  0xFFFFFFFFu
#define PARQ_DATA_MANTER 0xFFFFFFFEu
#define MODO_PEGAJOSO  01000   /* en una carpeta: solo el dueño de cada archivo (o de la carpeta) lo borra */
#define MODO_PERMISOS  01777
#define UMASK_PADRAO   022
typedef struct {
    unsigned int uid, gid;
    unsigned int modo;             /* 01777: rwx de dueño, grupo y otros, y el pegajoso */
    unsigned int tipo;             /* TIPO_ARQUIVO, TIPO_PASTA... */
    unsigned int com_donos;        /* 1 = el sistema de archivos los guarda (ext2, RAM); 0 = del montaje */
    unsigned int reservado[3];
} info_permisos_t;

/* SIS_INYECTAR (mejora 097).
 * INY_TECLA:  a = código de tecla como en EV_TECLA (Latin-1, o TECLA_* desde 0x100);
 *             b = INY_PULSADA (o 0 si se suelta) | INY_MOD_* con los modificadores
 *             que el cliente tiene pulsados en ese momento. Con a = 0 solo cambian
 *             los modificadores (pulsar o soltar Shift, Ctrl o Alt solos: soltar
 *             Alt cierra el Alt+Tab). Ctrl+letra se convierte en el código de
 *             control, como en el teclado real. TECLA_SUPER pulsada/soltada es la
 *             tecla Windows (sola abre el menú Inicio al soltarla).
 * INY_RATON:  a = (x << 16) | y, posición absoluta en la pantalla;
 *             b = botones pulsados (INY_BOTON_*).
 * INY_RUEDA:  a = entalles con signo (positivo hacia abajo, como EV_RODA).
 * INY_ESTADO: a = 1 para activar el permiso en este proceso, 0 para soltarlo;
 *             devuelve error si el proceso no puede inyectar. Permisos (D251): solo
 *             /bin/vncd o /apps/bin/vncd lanzado por el gestor de servicios, y uno
 *             a la vez (ERRO_ACESSO, ERRO_OCUPADO); argumentos fuera de rango,
 *             ERRO_INVALIDO; la posición se recorta a la pantalla. */
#define INY_TECLA         1
#define INY_RATON         2
#define INY_RUEDA         3
#define INY_ESTADO        4
#define INY_PULSADA       0x01
#define INY_MOD_SHIFT     0x02
#define INY_MOD_CTRL      0x04
#define INY_MOD_ALT       0x08
#define INY_BOTON_IZQ     0x01
#define INY_BOTON_MEDIO   0x02
#define INY_BOTON_DER     0x04

/* SIS_SERVICOS (D216). El supervisor es el programa que el kernel lanza al
 * arrancar; solo puede haber uno vivo. Al apagar o reiniciar, el kernel le
 * avisa (SERV_APAGANDO deja de ser 0) y espera, como mucho 20 s, su SERV_PRONTO. */
#define SERV_REGISTRAR  1   /* () -> 0; quien llama pasa a ser el supervisor (ERRO_OCUPADO si hay otro vivo) */
#define SERV_APAGANDO   2   /* () -> 0, o SERV_MOTIVO_*: el sistema se está apagando o reiniciando */
#define SERV_PRONTO     3   /* () -> 0; solo el supervisor: ya paró los servicios, el apagado sigue */
#define SERV_ATIVOS     4   /* (cuántos) -> 0; solo el supervisor: servicios en marcha (0 = el apagado no espera) */
#define SERV_SUPERVISOR 5   /* () -> pid del supervisor vivo, o 0 */
/* D255: elevación sin setuid. Un programa (su, admin, passwd) deja en el kernel
 * un pedido con la contraseña; el supervisor (root, de /bin/servicios) lo recoge,
 * comprueba la contraseña en la sombra y responde. Si concede una elevación, el
 * kernel cambia la identidad del proceso que lo pidió, que sigue con su misma
 * entrada y salida. Un pedido por proceso; 8 a la vez como mucho. */
#define SERV_PEDIR      6   /* (&pedido_conta_t) -> id (> 0); ERRO_NAO_EXISTE sin supervisor, ERRO_OCUPADO lleno */
#define SERV_RESULTADO  7   /* (id) -> 0 aún sin respuesta, 1 hecho, o el error con que se negó */
#define SERV_PROXIMO    8   /* (&pedido_recebido_t) -> 1 si había uno, 0 si no; solo el supervisor */
#define SERV_RESPONDER  9   /* (id, resultado) -> 0; solo el supervisor; 1 = concedido (eleva si es PED_ELEVAR) */
#define PED_ELEVAR      1   /* pasar a ser uid_alvo (root, normalmente) con su contraseña */
#define PED_SENHA       2   /* cambiar la contraseña de uid_alvo (la actual en senha, la nueva en senha_nova) */
#define PED_SENHA_MAX   64
/* D340: entrar en el escritorio (inicio de sesión) con la cuenta uid_alvo. Solo lo crea el propio kernel
 * (la pantalla de inicio de sesión, pid 0, uid PED_UID_NINGUEM); un programa recibe ERRO_INVALIDO. */
#define PED_ENTRAR      3
#define PED_UID_NINGUEM 65534
/* D269: con PED_GRAFICO (se suma a PED_ELEVAR) el kernel no espera una contraseña del programa: el
 * gestor de ventanas pinta el diálogo de permisos (atenúa el escritorio), pide la contraseña de
 * root a la persona y la entrega él mismo al supervisor. El programa solo espera el resultado
 * con SERV_RESULTADO (1 = ya es root; ERRO_ACESSO = «No» o contraseña mala; ERRO_ESPERA = bloqueo). */
#define PED_GRAFICO     0x100
/* D372: con PED_SIN_RECORDAR (junto a PED_GRAFICO) el diálogo pregunta siempre: ni usa ni guarda la
 * autorización recordada 5 min por programa. Para lo que no debe repetirse sin que la persona lo vea
 * (encender una VPN: otro programa podría lanzar /bin/wg con su propio .conf en esos 5 min). */
#define PED_SIN_RECORDAR 0x200
#define PED_MOTIVO_MAX  80
typedef struct {
    unsigned int tipo;                   /* PED_* */
    unsigned int uid_alvo;
    char         senha[PED_SENHA_MAX];   /* la contraseña actual de uid_alvo */
    char         senha_nova[PED_SENHA_MAX];   /* PED_SENHA, o la primera contraseña de root si no tenía */
    char         motivo[PED_MOTIVO_MAX];      /* PED_GRAFICO: qué quiere hacer el programa (lo muestra el diálogo) */
} pedido_conta_t;
typedef struct {
    unsigned int id;
    unsigned int tipo, uid_alvo;
    int          pid;                    /* quien lo pidió */
    unsigned int uid, gid;               /* su identidad al pedirlo */
    unsigned int servicio;               /* 1 = viene de un servicio (sshd...), no de la sesión local */
    char         senha[PED_SENHA_MAX];
    char         senha_nova[PED_SENHA_MAX];
} pedido_recebido_t;
#define SERV_MOTIVO_APAGAR    1
#define SERV_MOTIVO_REINICIAR 2

/* SIS_MEMORIA (D209). Las direcciones y los tamaños van en páginas enteras
 * (múltiplos de 4096) y dentro del área de datos ([inicio, fim) de
 * aumentar_memoria): fuera de ella, ERRO_INVALIDO. */
#define MEM_RESERVAR  1   /* (0, bytes) -> fin antiguo del área; la alarga SIN asignar páginas */
#define MEM_DEVOLVER  2   /* (endereço, bytes) -> páginas devueltas; quedan sin asignar (tocarlas = fallo) */
#define MEM_ASIGNAR   3   /* (endereço, bytes) -> 0; asigna (a cero) las páginas que falten */
/* D349: (endereço, bytes, MEM_PROT_*) -> 0; cambia los permisos de las páginas ya
 * asignadas (como mprotect). Con NX, el área de datos (montón) y la pila no son
 * ejecutables: quien genere código (el recompilador de DOSBox, un JIT) pide aquí
 * MEM_PROT_EXEC. Sin NX (CPU sin él, o sin PAE) la ejecución no se puede impedir.
 * Todas las páginas del rango tienen que estar ya asignadas: si falta alguna,
 * ERRO_SEM_MEMORIA y no se cambia nada (como el ENOMEM de mprotect). */
#define MEM_PROTEGER  4
/* D352: (endereço, bytes) -> páginas del rango que están en la swap comprimida al terminar; las
 * presentes se sacan ya (como MADV_PAGEOUT de Linux; también para las pruebas). 0 sin swap. */
#define MEM_SACAR     5
/* D378 (fase 3): (endereço, bytes, fd << 8 | MEM_PROT_EXEC o 0, desloc) -> 1 si reutilizó páginas
 * que ya tenía otro proceso, 0 si las cargó, o un error. Mapea de solo lectura (nunca escritura;
 * ejecutables solo con MEM_PROT_EXEC) las páginas [desloc, desloc+bytes) del fichero abierto «fd»
 * en [endereço, endereço+bytes) del área de datos, que tiene que estar sin asignar. Solo ficheros
 * de root sin escritura de grupo ni de otros, con sus carpetas igual (si no, ERRO_ACESSO); lo que
 * pasa del final del fichero queda a cero. Las páginas se comparten entre los procesos que mapean
 * el mismo trozo del mismo fichero (comparado byte a byte). MEM_DEVOLVER las quita; MEM_PROTEGER
 * no les cambia los permisos. Con error, el tramo puede quedar con páginas propias normales: hay
 * que devolverlo con MEM_DEVOLVER. Es lo que usa ld-sagui para el código de las bibliotecas. */
#define MEM_COMPARTIR 6
#define MEM_PROT_ESCRITA 2   /* = PROT_WRITE */
#define MEM_PROT_EXEC    4   /* = PROT_EXEC */

/* SIS_DISCO_BLOCOS (D179). El nombre es el de "discos" (hdb, usb0, hdb1...).
 * Leer no pide nada; escribir, descartar y releer la tabla solo valen para el
 * proceso que "reservó" el disco entero, y la reserva solo se da si no hay nada
 * montado de ese disco (o, con DBLOCO_CONF_MONTADO, protegiendo lo montado: D213). El disco del que arrancó el sistema pide además
 * DBLOCO_CONF_ARRANQUE (la confirmación doble de la interfaz). */
#define DBLOCO_RESERVAR   1   /* (nombre del disco entero, DBLOCO_CONF_*) -> 0 */
#define DBLOCO_LIBERAR    2   /* (nombre) -> 0; suelta la reserva y vuelve a leer la tabla */
#define DBLOCO_LER        3   /* (nombre, lba, sectores <= DBLOCO_MAX_SETORES, destino) -> 0 */
#define DBLOCO_ESCREVER   4   /* (nombre, lba, sectores <= DBLOCO_MAX_SETORES, origen) -> 0 */
#define DBLOCO_DESCARTAR  5   /* (nombre, lba, sectores) -> 0; TRIM; ERRO_NAO_SUPORTADO si no se puede */
#define DBLOCO_RELER      6   /* (nombre) -> 0; vuelve a leer la tabla sin soltar la reserva */
#define DBLOCO_INFO       7   /* (nombre, &info_bloco_t) -> 0 */
/* D233: CD/DVD. Desmonta lo que haya del disco y abre la bandeja (START STOP UNIT con
 * LoEj); con a = 1, la cierra. ERRO_OCUPADO si un programa lo tiene reservado (el
 * instalador copiando del CD) o no se puede desmontar; ERRO_NAO_SUPORTADO si no es un CD. */
#define DBLOCO_EXPULSAR   8   /* (nombre, cerrar) -> 0 */
#define DBLOCO_CONF_ARRANQUE 1   /* reservar el disco de arranque: la persona lo confirmó dos veces */
#define DBLOCO_CONF_GPT      2   /* reservar un disco GPT para borrarlo entero (el instalador) */
/* D213: reservar aunque haya particiones montadas (el disco de arranque con /home, /apps...).
 * Lo montado y, en el disco de arranque, la partición de SaguiOS (0xDA) quedan intocables: no
 * se escribe ni se descarta nada dentro de ellas, y cada escritura de la tabla (MBR o EBR) tiene
 * que dejarlas exactamente donde estaban (mismo inicio, tamaño y tipo). */
#define DBLOCO_CONF_MONTADO  4
/* D392: con DBLOCO_CONF_MONTADO (y DBLOCO_CONF_ARRANQUE), la partición de SaguiOS (0xDA) del disco de
 * arranque deja de ser intocable: se puede escribir DENTRO de ella (la etapa 2 nueva, el kernel de
 * prueba, la cabecera). Lo montado sigue protegido y la tabla tiene que seguir describiéndola igual.
 * Es lo que usa «arranque» (probar un kernel con el sistema en marcha). Un kernel de antes lo ignora
 * (la escritura se rechaza con ERRO_OCUPADO, sin escribir nada). */
#define DBLOCO_CONF_SAGUI    8
#define DBLOCO_MAX_SETORES 256   /* D338: 128 KB por llamada (antes 64 KB) */

typedef struct {
    unsigned int setores;          /* del volumen (disco o partición) */
    unsigned int inicio;           /* primer sector dentro del disco (0 = disco entero) */
    unsigned int trim;             /* TRIM_* abajo */
    unsigned int arranque_real;    /* 1 = es (o es parte de) el disco del que arrancó la máquina */
    unsigned int reservado_pid;    /* quién lo tiene reservado (0 = nadie) */
    unsigned int gpt;              /* 1 = el disco tiene tabla GPT (solo lectura) */
    unsigned int tipo_mbr;         /* tipo de la partición en la tabla (0 en el disco entero) */
    unsigned int cab_lba;          /* D270: en el disco de arranque, dónde está su cabecera: 1, o en un disco
                                    * instalado con GPT el primer sector de la partición de SaguiOS (0 = no lo es) */
    unsigned int bloco_fisico;     /* D283: 512, o 4096 en un NVMe de bloques de 4096 (las tablas MBR/GPT cuentan en ellos) */
    unsigned int reservado[7];
} info_bloco_t;
#define TRIM_NAO          0        /* el disco no lo admite (IDENTIFY palabra 169) */
#define TRIM_SIM          1        /* lo admite y el driver lo manda (AHCI) */
#define TRIM_SEM_DRIVER   2        /* lo admite, pero el driver no sabe mandarlo (IDE por PIO) */

/* SIS_MAPEAR_BAR: la petición va en un struct en el espacio del programa. */
typedef struct {
    unsigned int barramento, dispositivo, funcao;   /* donde está el dispositivo (PCI) */
    unsigned int indice;      /* 0..5: qué BAR del dispositivo */
    unsigned int desloc;      /* desde dónde dentro del BAR, en bytes */
    unsigned int tamanho;     /* cuántos bytes se quieren ver */
    unsigned int permissao;   /* BAR_LEITURA (0) o BAR_ESCRITA (1) */
} pedido_bar_t;

#define BAR_LEITURA 0
#define BAR_ESCRITA 1
#define BAR_TAMANHO_MAX (16u * 1024u * 1024u)   /* por petición */

/* Erros (sempre negativos) */
#define ERRO_INVALIDO      -1
#define ERRO_NAO_EXISTE    -2
#define ERRO_JA_EXISTE     -3
#define ERRO_SEM_MEMORIA   -4
#define ERRO_ENDERECO      -5   /* ponteiro inválido vindo do programa */
#define ERRO_NAO_E_PASTA   -6
#define ERRO_E_PASTA       -7
#define ERRO_NAO_VAZIA     -8
#define ERRO_LIMITE        -9   /* tabela cheia (arquivos, processos) */
#define ERRO_DESCRITOR    -10
#define ERRO_PIPE_QUEBRADO -11
#define ERRO_SEM_PROCESSO -12
#define ERRO_FORMATO      -13   /* não é um executável ELF válido */
#define ERRO_ACESSO       -14   /* operação não permitida */
#define ERRO_OCUPADO      -15
#define ERRO_SO_LEITURA   -16   /* sistema de arquivos montado só para leitura (D040) */
#define ERRO_NAO_SUPORTADO -17  /* el dispositivo o el driver no lo admite (D179: TRIM) */
#define ERRO_DISCO_ARRANQUE -18 /* es el disco de arranque: hace falta la confirmación doble (D179) */
#define ERRO_SEM_SENHA     -19  /* D255: la cuenta no tiene contraseña y hay que fijarla desde la sesión local */
#define ERRO_ESPERA        -20  /* D255: demasiados intentos fallidos: hay que esperar */
#define ERRO_SEM_ESPACO    -21  /* v8.5.4.2 (D379): no queda espacio en el volumen (ENOSPC; la capa de la live en la RAM, /tmp) */
#define ERRO_INTERROMPIDO  -22  /* D401: una señal interrumpió la espera (EINTR) */
#define ERRO_TENTAR        -23  /* D401: descriptor no bloqueante: tendría que esperar (EAGAIN) */
#define ERRO_EM_CURSO      -24  /* D401: connect no bloqueante: la conexión sigue en curso (EINPROGRESS) */
#define ERRO_JA_EM_CURSO   -25  /* D401: connect otra vez mientras sigue en curso (EALREADY) */

/* ------------------------------------------------------------ Procesos POSIX (D401) */
/* SIS_ESPERAR_PID: pid > 0 ese hijo; -1 cualquiera; 0 los de mi grupo; < -1 los del grupo -pid.
 * El estado, como en Linux: (código & 0xff) << 8 si salió; el número de la señal (| 0x80 si
 * «core») si lo terminó una señal. Un proceso que terminó el kernel sin señal cuenta como SIGKILL. */
#define ESPERA_SEM_BLOQUEAR  1   /* = WNOHANG */

/* Señales: los números de Linux (los mismos de signal.h de la libc común). */
#define SINAL_HUP    1
#define SINAL_INT    2
#define SINAL_QUIT   3
#define SINAL_ILL    4
#define SINAL_TRAP   5
#define SINAL_ABRT   6
#define SINAL_BUS    7
#define SINAL_FPE    8
#define SINAL_KILL   9
#define SINAL_USR1  10
#define SINAL_SEGV  11
#define SINAL_USR2  12
#define SINAL_PIPE  13
#define SINAL_ALRM  14
#define SINAL_TERM  15
#define SINAL_CHLD  17
#define SINAL_CONT  18
#define SINAL_STOP  19
#define SINAL_TSTP  20
#define SINAL_TTIN  21
#define SINAL_TTOU  22
#define SINAL_URG   23
#define SINAL_XCPU  24
#define SINAL_XFSZ  25
#define SINAL_WINCH 28
#define SINAL_SYS   31
#define SINAIS_N    32           /* 1..31; el bit n de una máscara es la señal n */
#define SINAL_TRATADOR_DFL 0u    /* = SIG_DFL */
#define SINAL_TRATADOR_IGN 1u    /* = SIG_IGN */
/* Banderas de la acción (las de Linux). */
#define SINAL_SA_NOCLDSTOP 0x00000001u
#define SINAL_SA_NOCLDWAIT 0x00000002u
#define SINAL_SA_SIGINFO   0x00000004u
#define SINAL_SA_RESTORER  0x04000000u
#define SINAL_SA_ONSTACK   0x08000000u
#define SINAL_SA_RESTART   0x10000000u
#define SINAL_SA_NODEFER   0x40000000u
#define SINAL_SA_RESETHAND 0x80000000u
/* Operaciones de SIS_SINAL. */
#define SINOP_ACAO        0   /* (sinal, &acao_sinal_t nueva o 0, &acao_sinal_t vieja o 0) -> 0 (sigaction) */
#define SINOP_MASCARA     1   /* (SINOP_BLOQUEAR..., &nueva o 0, &vieja o 0) -> 0 (sigprocmask, del hilo) */
#define SINOP_ENVIAR      2   /* (pid, sinal) -> 0; pid > 0 ese proceso, 0 mi grupo, -1 todos los míos, < -1 el grupo -pid;
                               * sinal 0 = solo comprobar (kill) */
#define SINOP_HILO        3   /* (tid, sinal) -> 0: a un hilo de mi proceso (raise, pthread_kill) */
#define SINOP_PENDENTES   4   /* (&mascara) -> 0 (sigpending) */
#define SINOP_SUSPENDER   5   /* (&mascara) -> ERRO_INTERROMPIDO cuando llega una (sigsuspend) */
#define SINOP_ALARME      6   /* (ms, 0 = quitarla) -> ms que le quedaban a la anterior (alarm, setitimer) */
#define SINOP_PILHA       7   /* (&pilha_sinal_t nueva o 0, &pilha_sinal_t vieja o 0) -> 0 (sigaltstack) */
#define SINOP_PAI         8   /* () -> pid del padre (getppid; 0 = ninguno) */
#define SINOP_GRUPO       9   /* (pid o 0 = yo) -> su grupo de procesos (getpgid, getpgrp) */
#define SINOP_BLOQUEAR    0   /* como SIG_BLOCK */
#define SINOP_DESBLOQUEAR 1   /* SIG_UNBLOCK */
#define SINOP_FIXAR       2   /* SIG_SETMASK */
typedef struct {
    unsigned int tratador;     /* función, SINAL_TRATADOR_DFL o SINAL_TRATADOR_IGN */
    unsigned int mascara;      /* bloqueadas mientras corre el tratador */
    unsigned int flags;        /* SINAL_SA_* */
    unsigned int restaurador;  /* la función de la libc que llama a SIS_SINAL_VOLTAR (obligatoria con tratador) */
} acao_sinal_t;
typedef struct {
    unsigned int base, flags, tamanho;   /* = stack_t (ss_sp, ss_flags, ss_size) */
} pilha_sinal_t;
#define SINAL_PILHA_EM_USO  1   /* SS_ONSTACK */
#define SINAL_PILHA_DESLIGADA 2 /* SS_DISABLE */
/* Lo que el kernel deja en la pila al llamar a un tratador, como en Linux i386: el siginfo_t de
 * 128 bytes y el ucontext_t (gregs en el orden de Linux: REG_GS 0 ... REG_EIP 14 ... REG_SS 18).
 * El área de la FPU (FXSAVE de 512 bytes o XSAVE de 832, alineada a 64) va encima y «fpregs»
 * la apunta (0 = el hilo no había usado la FPU). */
typedef struct {
    int signo, erro, codigo;   /* si_signo, si_errno, si_code */
    union {                    /* desde el byte 12, como la unión de Linux */
        struct { int pid; unsigned int uid; int estado; } proc;   /* kill (si_pid, si_uid) y SIGCHLD (+ si_status) */
        unsigned int endereco;                                  /* SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGTRAP: si_addr */
    } u;
    unsigned int resto[26];
} info_sinal_t;                /* 128 bytes */
typedef struct {
    unsigned int flags, ligacao;
    unsigned int pilha_base, pilha_flags, pilha_tamanho;   /* uc_stack */
    unsigned int gregs[19];
    unsigned int fpregs, oldmask, cr2;
    unsigned int mascara;      /* uc_sigmask: la máscara del hilo antes del tratador */
    unsigned int fpu_formato;  /* SINAL_FPU_* */
    unsigned int reservado[3];
} contexto_sinal_t;
#define SINAL_FPU_NENHUM 0
#define SINAL_FPU_FNSAVE 1
#define SINAL_FPU_FXSAVE 2
#define SINAL_FPU_XSAVE  3
/* si_code */
#define SINAL_SI_USER    0
#define SINAL_SI_KERNEL  0x80
#define SINAL_SEGV_MAPERR 1
#define SINAL_SEGV_ACCERR 2
#define SINAL_FPE_INTDIV 1
#define SINAL_FPE_FLTDIV 3
#define SINAL_FPE_FLTINV 7
#define SINAL_ILL_ILLOPC 1
#define SINAL_BUS_ADRALN 1
#define SINAL_TRAP_BRKPT 1
#define SINAL_CLD_EXITED 1
#define SINAL_CLD_KILLED 2
#define SINAL_CLD_DUMPED 3

/* SIS_SONDAR: como struct pollfd de Linux. */
typedef struct {
    int   fd;
    short eventos;    /* events */
    short prontos;    /* revents */
} sondagem_t;
#define SONDA_LER     0x001   /* POLLIN */
#define SONDA_URGENTE 0x002   /* POLLPRI */
#define SONDA_ESCREVER 0x004  /* POLLOUT */
#define SONDA_ERRO    0x008   /* POLLERR */
#define SONDA_FECHADO 0x010   /* POLLHUP */
#define SONDA_INVALIDO 0x020  /* POLLNVAL */
#define SONDA_MAX     256     /* entradas por llamada */

/* SIS_DESCRITOR (fcntl). */
#define DESC_LER_FD     1   /* () -> DESC_FD_* del descriptor (F_GETFD) */
#define DESC_FIXAR_FD   2   /* (DESC_FD_*) -> 0 (F_SETFD) */
#define DESC_LER_FL     3   /* () -> DESC_FL_* del fichero abierto (F_GETFL) */
#define DESC_FIXAR_FL   4   /* (DESC_FL_*) -> 0 (F_SETFL: solo NAO_BLOQUEAR y ACRESCENTAR) */
#define DESC_DUPLICAR   5   /* (mínimo) -> el primer descriptor libre >= mínimo, al mismo fichero (F_DUPFD) */
#define DESC_FD_FECHAR_EXEC 1     /* FD_CLOEXEC */
#define DESC_FL_LER         0x0001
#define DESC_FL_ESCREVER    0x0002
#define DESC_FL_ACRESCENTAR 0x0400   /* O_APPEND */
#define DESC_FL_NAO_BLOQUEAR 0x0800  /* O_NONBLOCK */

/* D255: la configuración del sistema. Los programas leen /etc como siempre; la
 * partición SAGUI-CONF (ext2, solo root) se monta en CONF_SISTEMA_PONTO y su
 * etc/ se copia sobre el /etc de la RAM al arrancar (kernel/fs/conf_sistema.c).
 * Quien cambia algo del sistema lo escribe en los dos sitios con
 * conf_gravar() (libsagui/conf_sistema.c, temporal + renombrar). Sin la
 * partición, «modo en vivo»: nada se guarda y root no tiene contraseña (solo
 * desde la sesión local). El kernel protege estas rutas a quien no es root:
 * la sombra y la clave privada del sshd no se leen; /etc, /bin y /sistema no se
 * escriben; /sistema tampoco se lee. */
#define CONF_SISTEMA_PONTO    "/sistema/conf"
#define CONTAS_ARQ_USUARIOS   "/etc/usuarios"
#define CONTAS_ARQ_SOMBRA     "/etc/sombra"
#define CONTAS_ARQ_GRUPOS     "/etc/grupos"   /* D340: «admin:10:ana,sagui»; lo leen todos */
#define CONTAS_CLAVES_ROOT    "/etc/ssh/authorized_keys"   /* las claves SSH que entran como root */

/* Opções do SIS_MONTAR (D040). Sem nenhuma das duas primeiras: o padrão
 * (leitura e escrita nos pendrives; só leitura nos discos internos). */
#define MONTAR_SO_LEITURA      0x01
#define MONTAR_LEITURA_ESCRITA 0x02   /* pedido explícito: único jeito de gravar num disco interno */
/* D282: dueño y permisos de los sistemas sin dueños (FAT32, ISO 9660, UDF), como
 * uid=, gid= y umask= de Linux. Con MONTAR_DONO, el cuarto argumento de SIS_MONTAR es
 * uid | gid << 16; con MONTAR_COM_UMASK, la máscara va en los bits 16-24 de las
 * opciones. Sin ellos: sagui (1000) y 022; en /tmp y /logs, root y 0 (todos escriben). */
#define MONTAR_DONO            0x04
#define MONTAR_COM_UMASK       0x08
#define MONTAR_UMASK(o)        (((o) >> 16) & 0777)
#define MONTAR_COM_MASCARA(m)  (MONTAR_COM_UMASK | ((unsigned)(m) & 0777) << 16)
#define MONTAR_TIPO(o)   (((o) >> 8) & 0xF)
#define MONTAR_AUTO      0x000
#define MONTAR_FAT32     0x100
#define MONTAR_EXT2      0x200
#define MONTAR_EXT3      0x300
#define MONTAR_EXT4      0x400
#define MONTAR_ISO9660   0x500   /* D207: CD/DVD, siempre solo lectura */
#define MONTAR_UDF       0x600   /* D233: DVD y CD grabados (UDF 1.02-2.01), solo lectura */
#define MONTAR_EXFAT     0x700   /* D325: pendrives y tarjetas exFAT, lectura y escritura */
/* SIS_HORA (D080) */
#define HORA_UTC          0   /* (&hora_t) -> segundos UTC desde 1/1/1970 (ler como unsigned) */
#define HORA_AJUSTAR      1   /* (segundos UTC, HORA_*) -> 0: acerta o relógio do sistema */
#define HORA_LER_RTC      2   /* (&data_t) -> 0: o que está gravado no relógio da placa, sem conversão */
#define HORA_GRAVAR_RTC   1   /* opção de HORA_AJUSTAR: grava também o RTC (em UTC ou local, conforme a zona) */
/* SIS_ZONA (D080) */
#define ZONA_LER          0
#define ZONA_FIXAR        1   /* aplica já (a barra e as datas mudam); quem grava /etc/zona é o programa */
/* De onde veio a configuração (config_zona_t.origem) */
#define ZONA_PADRAO       0   /* nada configurado: UTC */
#define ZONA_ARQUIVO      1   /* /etc/zona (na RAM) */
#define ZONA_PERSISTENTE  2   /* /home/.zona (disco persistente, D040) */
#define ZONA_PROGRAMA     3   /* mudada com SIS_ZONA depois do boot */

/* Rede (D046): -40 a -44 */
#define ERRO_TEMPO        -40   /* a rede não respondeu a tempo */
#define ERRO_SEM_REDE     -41   /* sem placa de rede ou ainda sem endereço IP */
#define ERRO_RECUSADA     -42   /* o outro computador recusou a conexão */
#define ERRO_INALCANCAVEL -43   /* sem rota ou o ARP não achou o destino */
#define ERRO_CONEXAO_CAIU -44   /* o outro lado cortou a conexão (RST) ou parou de responder */

/* Layouts do teclado (SIS_TECLADO) */
#define TECLADO_ES    0
#define TECLADO_ABNT2 1

/* Modos de abrir */
#define ABRIR_LER       1
#define ABRIR_ESCREVER  2
#define ABRIR_CRIAR     4
#define ABRIR_TRUNCAR   8
#define ABRIR_ACRESCENTAR 16

/* Origem do posicionar */
#define POS_INICIO 0
#define POS_ATUAL  1
#define POS_FIM    2

/* Tipos de entrada */
#define TIPO_ARQUIVO  1
#define TIPO_PASTA    2
#define TIPO_PROGRAMA 3
#define TIPO_TERMINAL 4
#define TIPO_PIPE     5

#define NOME_MAX 64

typedef struct {
    unsigned int tipo;
    unsigned int tamanho;
} info_arquivo_t;

typedef struct {
    char         nome[NOME_MAX];
    unsigned int tipo;
    unsigned int tamanho;
} entrada_pasta_t;

/* SIS_INFO_ALEATORIO (D061) */
#define ALEA_RDRAND 1   /* a CPU tem RDRAND (e ele funciona) */
#define ALEA_RDSEED 2   /* a CPU tem RDSEED */
#define ALEA_TSC    4   /* o TSC entra nas medidas de tempo */
typedef struct {
    unsigned int fontes;         /* ALEA_* */
    unsigned int eventos;        /* interrupções misturadas desde o boot */
    unsigned int pedidos;        /* chamadas atendidas */
    unsigned int reservado;
} info_aleatorio_t;

/* SIS_DATA: a hora LOCAL da zona do sistema (desde D080; antes, o RTC "a secas"). */
typedef struct {
    unsigned short ano;
    unsigned char  mes, dia, hora, minuto, segundo, dia_semana;
} data_t;

/* SIS_HORA, HORA_UTC (D080) */
typedef struct {
    unsigned int segundos;    /* UTC desde 1/1/1970 */
    unsigned int ms;          /* 0..999 */
    int          desvio;      /* da zona do sistema agora: segundos a somar ao UTC */
    unsigned int verao;       /* 1 = horário de verão agora */
    unsigned int ajustes;     /* muda quando acertam a hora ou trocam a zona */
} hora_t;

/* SIS_ZONA (D080) */
typedef struct {
    char         nome[40];    /* "Europe/Madrid" ("UTC" sem configuração) */
    char         tz[64];      /* regra POSIX TZ: "CET-1CEST,M3.5.0,M10.5.0/3" */
    unsigned int rtc_local;   /* 1 = o relógio da placa guarda a hora local (Windows); 0 = UTC */
    unsigned int rtc_auto;    /* 1 = ninguém escolheu: o kernel decidiu (UTC numa máquina virtual, local num PC) */
    unsigned int formato_24h; /* 1 = 24 horas; 0 = 12 horas (a. m./p. m.) */
    unsigned int origem;      /* ZONA_* (só leitura) */
} config_zona_t;

/* SIS_DATAS_ARQUIVO (D080) */
typedef struct {
    unsigned int modificado;  /* UTC desde 1970; 0 = o sistema de arquivos não guarda */
    unsigned int criado;
    unsigned int acessado;
} datas_arquivo_t;

/* SIS_BIP: uma nota. freq 0 = pausa. Duração arredondada para 10 ms. */
typedef struct {
    unsigned short freq;   /* Hz */
    unsigned short ms;
} nota_t;
#define BIP_SUBSTITUIR 0   /* para o que estiver tocando e começa esta lista */
#define BIP_ENFILEIRAR 1   /* toca depois do que já está na fila */

/* Áudio (D088-D090). Formatos de SIS_AUDIO_ABRIR: PCM de 8 bits sem sinal ou
 * de 16 bits com sinal (little-endian), mono ou estéreo intercalado, de
 * AUDIO_TAXA_MIN a AUDIO_TAXA_MAX Hz; o kernel converte para a saída da placa. */
#define AUDIO_TAXA_MIN  4000
#define AUDIO_TAXA_MAX  96000
#define AUDIO_FLUXOS    8       /* fluxos abertos ao mesmo tempo, de todos os programas */
/* SIS_AUDIO_CONTROLE: valor -1 = só consultar */
#define AUDIO_VOLUME    1   /* (fd, _, 0-100): volume do fluxo; fd = -1: o mestre -> volume atual */
#define AUDIO_SILENCIO  2   /* (-1, _, 0 ou 1): cala tudo sem perder o volume -> 1 se calado */
#define AUDIO_PENDENTE  3   /* (fd) -> ms que ainda faltam tocar do que foi escrito */
#define AUDIO_LIVRE     4   /* (fd) -> bytes que cabem agora sem bloquear */
#define AUDIO_DESCARTAR 5   /* (fd) -> 0; joga fora o que ainda não tocou */
#define AUDIO_EQ        6   /* (-1, _, &eq_audio_t o 0): ecualizador (D201); 0 = consultar -> nº de etapas activas */
#define AUDIO_SALIDA    7   /* (-1, _, índice | -1): salida de sonido (D275) -> índice actual; ERRO_INVALIDO si no existe */

#define AUDIO_BALANCE   8   /* (-1, _, 0-200 | -1): balance (D292): 0 todo a la izquierda, 100 centro, 200 todo a la derecha -> valor actual */
#define AUDIO_LATENCIA  9   /* (-1, _, ms 20-300 | -1): cuánto escribe el mezclador por delante de la tarjeta (D292) -> ms actuales */

/* AUDIO_EQ (D201): cascada de hasta 5 biquads por canal, en punto fijo. Los coeficientes
 * (b0 b1 b2 a1 a2, ya divididos por a0) los calcula el userland en Q28; y = b0 x + b1 x1 + b2 x2 - a1 y1 - a2 y2.
 * La ganancia previa (margen) va ya incluida en los b de la primera etapa. etapas = 0: derivación total. */
#define EQ_ETAPAS 5
#define EQ_BANDAS 5
typedef struct {
    unsigned int etapas;                 /* 0-5 */
    int          coef[EQ_ETAPAS][5];     /* Q28 */
    signed char  db[EQ_BANDAS];          /* los realces pedidos (-12..+12), solo para informar */
} eq_audio_t;

/* SIS_AUDIO_HDA (D107). Los verbos van al códec que usa el driver. */
#define HDA_OP_VERBO   1   /* (op, no << 20 | verbo << 8 | carga, &resposta): verbo de 12 bits con carga
                              de 8, o verbo de 4 bits (<< 16) con carga de 16, como en la norma -> 0 */
#define HDA_OP_ESTADO  2   /* (op, &info_hda_t, sizeof) -> 0 */
#define HDA_OP_PINO    3   /* (op, nodo, 1 o 0): busca un camino al DAC y enciende el pin / lo apaga -> 0 */
#define HDA_OP_GPIO    4   /* (op, datos, máscara): los bits de la máscara pasan a salidas con esos datos;
                              máscara 0 = volver a escribir los GPIO del ajuste (no apaga otros) -> 0 */
#define HDA_OP_AUTOSILENCIO 5  /* (op, 1/0/-1): callar los altavoces con auriculares enchufados -> estado */
/* D302: con el driver sb16, la misma llamada (SIS_AUDIO_HDA) lleva estas órdenes para la Sound Blaster 16. */
#define SB16_OP_ESTADO 0x40  /* (op, &info_sb16_t, sizeof) -> 0 */
#define SB16_OP_CANAL  0x41  /* (op, canal SB16_CANAL_*, 0-100 o -1): nivel del mezclador CT1745 -> nivel actual */
#define SB16_CANAL_MAESTRO 0
#define SB16_CANAL_PCM     1
#define SB16_CANAL_LINEA   2
#define SB16_CANAL_CD      3
#define SB16_CANAL_MICRO   4   /* el micrófono hacia la salida (0 = apagado: evita el acople) */
#define SB16_CANAL_ALTAVOZ 5   /* la entrada del altavoz del PC */
#define SB16_CANALES       6
typedef struct {
    unsigned short puerto, opl_puerto;      /* 0x220...; OPL: 0 = no hay */
    unsigned char  irq, dma8, dma16;        /* 0xFF = ninguno */
    unsigned char  dsp_mayor, dsp_menor;    /* versión del DSP (4.xx = SB16) */
    unsigned char  opl;                     /* 0 ninguno, 2 OPL2, 3 OPL3 */
    unsigned char  pnp;                     /* 1 = la configuró SaguiOS por ISA PnP */
    unsigned char  modo;                    /* 0 parada, 1 reproduciendo, 2 grabando */
    unsigned char  nivel[SB16_CANALES];     /* 0-100 */
    unsigned char  reservado[2];
    unsigned int   irqs, irqs_8, irqs_16;   /* IRQ atendidas: total, de 8 y de 16 bits */
    char           pnp_id[12];              /* "CTL0070" si vino de ISA PnP */
} info_sb16_t;
#define HDA_MAX_PINOS  8
typedef struct {
    unsigned char  codec, afg, primeiro_no, n_nos;   /* dirección del códec en el enlace; nodos */
    unsigned int   id, revisao;                      /* parámetros 0 y 2 del nodo 0 */
    unsigned int   ssid_pci, ssid_codec;             /* subsistema del controlador (PCI) y del códec (F20) */
    char           ajuste[24], modelo[24];           /* ajuste aplicado (vacío = ninguno) */
    unsigned char  n_gpio, gpio_mascara, gpio_direcao, gpio_dados;   /* lo que escribió el driver */
    unsigned char  autosilencio, fones_presentes, n_pinos, reservado;
    unsigned char  pino[HDA_MAX_PINOS];              /* pines encendidos por el driver */
    unsigned char  pino_papel[HDA_MAX_PINOS];        /* 0 línea, 1 altavoz, 2 auriculares */
    unsigned char  pino_dac[HDA_MAX_PINOS];
    unsigned int   pino_config[HDA_MAX_PINOS];       /* configuración usada (tras el ajuste) */
} info_hda_t;

/* SIS_INFO_SISTEMA */
#define INFO_MEMORIA   1
#define INFO_PROCESSOS 2
#define INFO_VERSAO    3
#define INFO_USB       4   /* relatório em texto: controladores, portas e dispositivos */
/* Informação do hardware para o infopc (D041). Faixa 20-29 reservada para isto. */
#define INFO_E820     20   /* (info_e820_t[], n) -> quantas regiões copiou */
#define INFO_SMBIOS   21   /* (buf, n bytes) -> bytes: info_smbios_t seguido da tabela SMBIOS crua */
#define INFO_PCI      22   /* (info_pci_t[], n) -> quantos dispositivos copiou */
#define INFO_VIDEO    23   /* (&info_video_t) */
#define INFO_CPU_MHZ  24   /* (&info_tsc_t) -> kHz do TSC medido contra o PIT no boot (0 = sem medida) */
/* Vídeo (D057-D059). Faixa 45-49. */
#define INFO_TELA     45   /* (&info_tela_t): modo atual, troca a quente, preferência, write-combining */
/* Informe de compatibilidade (D076). */
#define INFO_LOG      46   /* (buf, n) -> bytes: o fim do log do kernel (os últimos 32 KB, linhas inteiras) */
#define INFO_ACPI     47   /* (&info_acpi_t) -> 0: RSDP e assinaturas das tabelas (sem interpretar a AML) */
#define INFO_ARRANQUE 48   /* (&info_arranque_t) -> 0: de onde e como o carregador leu o sistema */
/* Rede (D046). Faixa 40-44 reservada para a rede. */
#define INFO_REDE      40   /* (&info_rede_t) -> 1 se há placa (suportada ou não), 0 se não há nenhuma */
#define INFO_CONEXOES  41   /* (info_conexao_t[], n) -> quantas copiou */
#define INFO_ARP       42   /* (info_arp_t[], n) -> quantas copiou */
/* Rede no Administrador de tareas (D079). Faixa 50-54. */
#define INFO_INTERFACES 50  /* (info_rede_t[], n) -> quantas: eth0 (se houver placa) e lo */
#define INFO_SOCKETS    51  /* (info_socket_t[], n) -> quantos: INFO_CONEXOES com o estado TCP detalhado */
#define INFO_TRAFEGO    52  /* (&info_trafego_t) -> 0: bytes por segundo da eth0 nos últimos 60 s */
/* Som (D088). */
#define INFO_AUDIO      53  /* (&info_audio_t) -> 0: placa, códec, volume e os fluxos abertos */
#define INFO_FREQUENCIA 56  /* (&info_frequencia_t, sizeof) -> 0: driver de frecuencia de la CPU (D106) */
/* Motor 3D por la GPU (D158): lo que el kernel sabe de la aceleración. La familia
 * y los registros los lee el programa (userland/libamd), no el kernel. */
#define INFO_MOTOR_GPU  57  /* (&info_motor_gpu_t, sizeof) -> 0 */

/* Estado del motor 3D (D158). El camino por defecto es el motor por software. */
#define MOTOR_GPU_SOFTWARE     0   /* ahora mismo: motor por software */
#define MOTOR_GPU_POR_HARDWARE 1   /* la GPU identificada puede acelerar (el motor aún no existe) */
#define MOTOR_GPU_DESATIVADO   2   /* la opción de arranque quitó la aceleración */
#define MOTOR_GPU_VIGILANTE_MS 2000   /* plazo máximo del vigilante de la GPU */
typedef struct {
    unsigned int versao;       /* 1 */
    unsigned int estado;        /* MOTOR_GPU_* */
    unsigned int aceleracao;    /* 1 = la aceleración por hardware está permitida */
    unsigned int desativado;    /* 1 = la opción de arranque la quitó (SGL_MOTOR=software) */
    unsigned int boot_opcoes;   /* las opciones de arranque, sin interpretar */
    unsigned int gpu_amd;       /* 1 = hay una GPU AMD en el bus PCI */
    unsigned int modelo;        /* device id del PCI (0 = ninguna) */
    unsigned int revisao;       /* revision id del PCI */
    unsigned int watchdog_ms;   /* el plazo máximo del vigilante de la GPU */
    char         motivo[64];    /* por qué no se acelera (vacío si sí se puede) */
    unsigned int reservado[8];
} info_motor_gpu_t;

/* 2D por la GPU para el escritorio (D195): qué acelerador se usa y por qué.
 * Los números 66 y 67 (el 64 y el 65 quedan libres para otras ramas). */
#define INFO_ACEL2D       66  /* (&info_acel2d_t, sizeof) -> 0 */
#define INFO_ACEL2D_PROVA 67  /* (&info_acel2d_prova_t, sizeof) -> 0: autoprueba completa y medida, ahora */
typedef struct {
    unsigned int versao;        /* 1 */
    unsigned int ativo;         /* 1 = la GPU copia y rellena; 0 = la CPU */
    unsigned int modo;          /* lo que pidió la cabecera (ACEL2D_* de acel2d.h) */
    unsigned int fabricante, modelo;   /* tarjeta elegida (0 = ninguna) */
    char         nome[16];      /* "vmware", "intel", "radeon" o "cpu" */
    char         motivo[160];   /* qué tarjeta y por qué (o por qué la CPU) */
    unsigned int copias, rellenos, falhas;   /* órdenes hechas por la GPU desde el arranque */
    unsigned int arrastos, quadros;          /* ventanas arrastradas y cuadros de arrastre medidos */
    unsigned int kciclos_quadro;             /* media de miles de ciclos (TSC) por cuadro de arrastre */
    unsigned int us_autoprova;               /* lo que tardó la autoprueba del arranque (0 = no se hizo) */
    unsigned int us_gpu, us_cpu;             /* la comparación de velocidad del arranque (0 = no se hizo) */
    unsigned int confirmada;                 /* D244: 1 = tarjeta confirmada; 0 = «por autoprueba» */
    unsigned int apagado;                    /* D244: 0 = no; 1 = lo apagó el vigilante; 2 = a mano */
    unsigned int prazo_ms;                   /* D244: plazo máximo de cada espera de la GPU en sesión */
    unsigned int reservado[2];
    char         placa[48];                  /* nombre de la tarjeta según la tabla del acelerador (vacío si no es de ella) */
    /* D346 (solo lectura, sin ser administrador): reparto GPU / software desde el arranque. Los índices de sw_* son
     * ACEL_SW_* de acel2d.h: 0 mover rectángulos, 1 desplazar la consola, 2 rellenos del escritorio, 3 volcado del
     * búfer de atrás a la pantalla (regiones). Operaciones y miles de píxeles. */
    unsigned int kpx_gpu_copias, kpx_gpu_rellenos;
    unsigned int sw_ops[4], sw_kpx[4];
    /* D387: volcado del búfer de atrás por la GPU (GART). El kernel acepta también el tamaño de antes (sin esto). */
    unsigned int volcado_estado;            /* VOLCADO_* de acel2d.h: 0 no hay, 1 activo, 2 apagado (kconfig), 3 a mano, 4 falló, 5 más lento */
    unsigned int volcado_ops, volcado_kpx;  /* volcados hechos por la GPU y miles de píxeles */
    unsigned int volcado_us_gpu, volcado_us_cpu;   /* medida de la autoprueba: lo mismo por la GPU y por la CPU */
    unsigned int volcado_cpu_ops;           /* volcados que hizo la CPU estando el de la GPU activo (pequeños, cursor...) */
    unsigned int volcado_simulado;          /* 1 = la «GPU» de software de las pruebas */
    unsigned int volcado_reservado[3];
    char         volcado_motivo[96];        /* por qué no, o con qué (GART en MC ...) */
    /* D391: la vuelta del cuadro del 3D (VRAM) a la RAM por la GPU. El kernel acepta también los dos tamaños de antes. */
    unsigned int retorno_estado;            /* VOLCADO_* (los mismos valores; 2 = kconfig vuelta_gpu=no) */
    unsigned int retorno_ops, retorno_kpx;  /* cuadros que trajo la GPU y miles de píxeles */
    unsigned int retorno_cpu_ops;           /* cuadros que tuvo que leer la CPU estando la vuelta activa (destino que no sirve...) */
    unsigned int retorno_us_gpu, retorno_us_cpu;   /* medida de la autoprueba (o de «acel2d vuelta medir»): GPU y CPU */
    unsigned int retorno_mascara;           /* 1 = la GPU quita el alfa (como la CPU); 0 = lo copia (solo RGB igual) */
    unsigned int retorno_reservado[3];
    char         retorno_motivo[96];
    /* D387 (Amilo, 2026-11-19): la memoria de vídeo REAL de la tarjeta, leída por su driver (el VBE anuncia a
     * veces solo una parte: 16 MB en una X1400 de 128 MB). 0 = el driver no la sabe. */
    unsigned int vram_kb;
    unsigned int vram_reservado[3];
    char         vram_fonte[32];            /* de dónde sale («CONFIG_MEMSIZE», «PFB 0x10020c»...) */
} info_acel2d_t;
#define INFO_ACEL2D_TAM_V1 ((unsigned int)__builtin_offsetof(info_acel2d_t, volcado_estado))
#define INFO_ACEL2D_TAM_V2 ((unsigned int)__builtin_offsetof(info_acel2d_t, retorno_estado))
#define INFO_ACEL2D_TAM_V3 ((unsigned int)__builtin_offsetof(info_acel2d_t, vram_kb))
#define INFO_ACEL2D_CTL   69  /* D244: (op, 0): op 0 = apagar, 1 = encender -> 0 / error; D387: 2 = volcado por GPU apagar,
                                * 3 = encender (repite su autoprueba), 4 = medir (volcados de la pantalla entera, al log);
                                * D391: 5 = vuelta por GPU apagar, 6 = encender (repite su autoprueba), 7 = medir */
/* Cámara (D277): INFO_CAMARA (&info_camara_t, sizeof) -> el campo "resultado" y el valor devuelto
 * (>= 0 bien, < 0 error). La orden va en el primer campo. Una sola cámara a la vez: la UVC integrada
 * o enchufada, o la fuente de prueba del kernel (un generador de patrón que pasa por el mismo montador
 * de cuadros, para probar sin cámara). Los servicios (de_servico) no pueden usarla.
 *   CAM_ESTADO  describe la fuente pedida (fuente 0 = la real, 2 = la de prueba): presente, nombre y modos.
 *   CAM_ABRIR   empieza a capturar con el modo "modo" (índice de ESTADO); la fuente 0 es la real y la 2 la de prueba.
 *   CAM_CUADRO  copia el último cuadro completo a buf si su seq es distinta de seq_visto (YUY2: larg*alt*2 bytes
 *               en orden Y0 U Y1 V; MJPEG: el JPEG tal cual); devuelve los bytes copiados, 0 = no hay cuadro nuevo.
 *   CAM_CERRAR  deja de capturar.
 *   CAM_AJUSTE  brillo/contraste de la unidad de procesamiento (control 0 o 1; escribir = 1 pone "valor"). */
#define INFO_CAMARA       70
#define CAM_ESTADO  0
#define CAM_ABRIR   1
#define CAM_CUADRO  2
#define CAM_CERRAR  3
#define CAM_AJUSTE  4
#define CAM_FUENTE_REAL   0
#define CAM_FUENTE_PRUEBA 2
#define CAM_FMT_YUY2   1
#define CAM_FMT_MJPEG  2
#define CAM_MODOS_MAX  24
#define CAM_CTL_BRILLO    0
#define CAM_CTL_CONTRASTE 1
typedef struct {
    unsigned short larg, alt;
    unsigned char  formato;     /* CAM_FMT_* */
    unsigned char  fps;
    unsigned char  reservado[2];
} info_camara_modo_t;
typedef struct {
    unsigned int   orden;       /* CAM_* */
    int            resultado;   /* lo mismo que devuelve la llamada */
    unsigned int   fuente;      /* CAM_FUENTE_* */
    unsigned int   modo;        /* ABRIR: índice del modo; ESTADO: el modo abierto */
    unsigned int   presente;    /* ESTADO: la fuente existe */
    unsigned int   abierta;     /* ESTADO: está capturando */
    unsigned int   n_modos;
    char           nome[48];
    info_camara_modo_t modos[CAM_MODOS_MAX];
    void          *buf;         /* CUADRO: donde copiar */
    unsigned int   buf_tam;
    unsigned int   seq_visto;   /* CUADRO: la seq del último cuadro que ya se tenía */
    unsigned int   seq;         /* CUADRO: seq del cuadro copiado; ESTADO: cuadros completos hasta ahora */
    unsigned short larg, alt;   /* CUADRO: tamaño del cuadro copiado */
    unsigned char  formato;     /* CUADRO: CAM_FMT_* del cuadro copiado */
    unsigned char  controles;   /* ESTADO: bit 0 brillo, bit 1 contraste disponibles */
    unsigned short reservado;
    unsigned int   control;     /* AJUSTE: CAM_CTL_* */
    unsigned int   escribir;    /* AJUSTE: 1 = poner "valor" */
    int            valor, minimo, maximo, defecto;   /* AJUSTE */
    unsigned int   descartados, paquetes, errores;   /* ESTADO: diagnóstico del flujo */
} info_camara_t;

/* 3D por la GPU (D285): la capa común (kernel/gui/gpu3d.c). SaguiGL manda los
 * triángulos ya recortados y en coordenadas del destino; el kernel los pasa al
 * driver de la tarjeta (Intel Gen3...) y devuelve el cuadro. Los números 71 a 74
 * (el 70 es INFO_CAMARA). */
#define INFO_GPU3D        71  /* (&info_gpu3d_t, sizeof) -> 0 */
#define INFO_GPU3D_OP     72  /* (&gpu3d_pedido_t, sizeof) -> 0, GPU3D_SEM_SUPORTE (> 0) o error (< 0) */
#define INFO_GPU3D_PROVA  73  /* (&info_gpu3d_prova_t, sizeof) -> 0: autoprueba y medida, ahora */
#define INFO_CAB_OPC      78  /* D323: (CAB_OPC_PEDIDO(desplazamiento, máscara, valor), 0) -> 0 / error: cambia bits de la cabecera de arranque del disco de arranque (solo administrador); solo CAB_ACEL2D, CAB_GPU3D y CAB_DISCO_OPC */
#define CAB_OPC_PEDIDO(d, m, v) ((unsigned)(d) | (unsigned)(m) << 8 | (unsigned)(v) << 16)
/* D353: opciones del kernel que se guardan desde el sistema y valen en el siguiente arranque (kconfig). */
#define INFO_KCONF        79  /* (info_kconf_t[], n) -> cuántas | KCONF_EST_* << 16: la lista (cualquier usuario) */
#define INFO_KCONF_GRAVAR 80  /* (KCONF_PEDIDO(op, hueco, valor), 0) -> 0 / error: guarda en la cabecera de arranque (solo administrador) */
/* D366 (htop): los hilos de cada proceso, uno por entrada (los hilos principales ya salen en INFO_PROCESSOS).
 * El tid vale para PLAN_INFO (nice, última CPU) y para SIS_MATAR de un hilo no: matar actúa sobre procesos. */
#define INFO_HILOS        83  /* (info_hilo_t[], n) -> cuántos (D366; 81 y 82 son de la barra, D361) */
typedef struct {
    int          tid;       /* identificador del hilo (el de SIS_HILO_ID) */
    int          pid;       /* el proceso al que pertenece */
    unsigned int estado;    /* como info_processo_t.estado */
    unsigned int cpu_ms;    /* tiempo de CPU de este hilo */
    unsigned int cpu;       /* la CPU donde corre */
} info_hilo_t;
/* D422: energía y pantalla: salvapantallas, bloqueo y apagado de la pantalla por inactividad, aviso de batería y
 * «no molestar». La configuración es de la máquina: /etc/energia.conf (clave=valor); el kernel la lee al arrancar y
 * con OCIO_OP_RECARGAR. INFO_OCIO la puede leer cualquiera; INFO_OCIO_CTL: recargar, no molestar y actividad, cualquiera;
 * simular, probar y batería simulada, solo el administrador (son para las pruebas). */
#define INFO_OCIO      100  /* (&info_ocio_t, sizeof) -> 0 */
#define INFO_OCIO_CTL  101  /* (OCIO_PEDIDO(op, arg), 0) -> 0 o ERRO_* */
#define OCIO_PEDIDO(op, arg)  ((((unsigned int)(op)) << 16) | ((unsigned int)(arg) & 0xFFFFu))
#define OCIO_OP_RECARGAR      1   /* vuelve a leer /etc/energia.conf */
#define OCIO_OP_NO_MOLESTAR   2   /* arg 1/0: el programa que llama pide / suelta «no molestar» (se suelta solo si acaba) */
#define OCIO_OP_ACTIVIDAD     3   /* como si se hubiera tocado una tecla (despierta y reinicia la cuenta) */
#define OCIO_OP_SIMULAR       4   /* arg = segundos de inactividad que se suman (pruebas) */
#define OCIO_OP_PROBAR        5   /* arg = efecto (OCIO_EF_*): lo muestra ya, aunque esté apagado en la configuración */
#define OCIO_OP_BATERIA_SIM   6   /* arg = porcentaje | 0x100 si está enchufada | 0x200 sin batería; 0xFFFF = la real */
#define OCIO_ESTADO_ACTIVO    0
#define OCIO_ESTADO_SALVA     1   /* salvapantallas en marcha */
#define OCIO_ESTADO_APAGADA   2   /* pantalla apagada (en negro) */
typedef struct {
    unsigned int versao;             /* 1 */
    unsigned int estado;             /* OCIO_ESTADO_* */
    unsigned int inactivo_s;         /* segundos sin teclado ni ratón (sin «no molestar») */
    unsigned int no_molestar;        /* peticiones vivas de «no molestar» (+1 si hay una ventana a pantalla completa) */
    unsigned int bloqueada;          /* 1 = la pantalla de bloqueo está puesta */
    unsigned int cargado;            /* 1 = había /etc/energia.conf */
    unsigned int salva_efecto;       /* 0 = apagado, 1-4 (OCIO_EF_*) */
    unsigned int salva_min;
    unsigned int bloqueo_modo;       /* 0 nunca, 1 tras bloqueo_min, 2 al salir del salvapantallas */
    unsigned int bloqueo_min;
    unsigned int pantalla_bat_min;   /* 0 = nunca */
    unsigned int pantalla_red_min;
    unsigned int bat_aviso, bat_critica, bat_accion;   /* %, %, 1 = apagar ordenadamente al llegar a crítico */
    unsigned int gob_bat, gob_red;   /* 0 rendimiento, 1 ahorro, 2 a demanda, 3 no tocar */
    unsigned int con_bateria;        /* hay batería (o simulada) */
    int          bat_pct;            /* -1 = no se sabe */
    unsigned int enchufado;          /* 1 = con el cargador */
    unsigned int simulada;           /* la batería es simulada */
    unsigned int veces_salva, veces_bloqueo, veces_apagada, avisos_bateria;
} info_ocio_t;
#define INFO_WG           85  /* (&info_wg_t, sizeof) -> 1 si el túnel wg0 está activo, 0 si no (D363; sin secretos) */
/* D399: energía de la GPU: relojes del motor y de la memoria (actuales y por defecto), los estados de la BIOS,
 * la temperatura (o «sin sensor») con las zonas térmicas del ACPI aparte, la VRAM y el modo. Leer lo puede
 * cualquiera; INFO_GPU_ENERGIA_CTL (cambiar el modo, leer los relojes por la BIOS, ensayar) solo el administrador.
 * Los relojes van en kHz; 0 = no se sabe. */
#define INFO_GPU_ENERGIA     86  /* (&info_gpu_energia_t, sizeof) -> 0 */
#define INFO_GPU_ENERGIA_CTL 87  /* (GPU_EN_PEDIDO(op, arg), 0) -> 0 bien, < 0 no (el porqué en info_gpu_energia_t.ultimo) */
#define GPU_EN_PEDIDO(op, arg)  ((((unsigned int)(op)) << 16) | ((unsigned int)(arg) & 0xFFFFu))
#define GPU_EN_OP_MODO       1   /* arg = GPU_EN_MODO_*: lo aplica ahora (bios vuelve a los relojes del arranque) */
#define GPU_EN_OP_LEER       2   /* lee los relojes ahora por la BIOS (GetEngineClock/GetMemoryClock) */
#define GPU_EN_OP_ESTADO     3   /* arg = índice de la tabla: ese estado, a mano (el modo pasa a «manual») */
#define GPU_EN_OP_SIMULAR    4   /* arg = índice: ensaya el cambio sin escribir nada (lo que haría la BIOS, al registro) */
#define GPU_EN_OP_DETALLE    5   /* registros de los PLL y las tablas al registro [GPUENERGIA] */
#define GPU_EN_OP_DESBLOQUEAR 6  /* tras un fallo: deja volver a intentarlo en este arranque (a mano) */
#define GPU_EN_MODO_BIOS     1   /* no se toca nada (los relojes que dejó la BIOS) */
#define GPU_EN_MODO_BAJO     2
#define GPU_EN_MODO_ALTO     3
#define GPU_EN_MODO_AUTO     4   /* bajo en reposo, sube con el 3D o el volcado trabajando (como el dynpm de Linux) */
#define GPU_EN_MODO_MANUAL   5   /* un estado elegido con GPU_EN_OP_ESTADO */
#define GPU_EN_SIN           0   /* soporte: no hay driver de energía para esta tarjeta */
#define GPU_EN_LECTURA       1   /* se leen las tablas; cambiar no se puede (o falta algo) */
#define GPU_EN_CONTROL       2   /* se puede cambiar */
#define GPU_EN_SIN_SENSOR    (-300000)   /* temp_mili_c: la tarjeta no tiene sensor legible */
#define GPU_EN_MAX_ESTADOS_ABI 8
#define GPU_EN_MAX_ZONAS     4
typedef struct {
    unsigned int  sclk_khz, mclk_khz;
    unsigned int  misc, misc2;          /* ulMiscInfo de la BIOS (ATOM_PM_MISCINFO_*) */
    unsigned char tipo;                 /* 0 -, 1 ahorro, 2 batería, 3 equilibrado, 4 rendimiento, 5 por defecto */
    unsigned char tensao;               /* 0 ninguna, 1 por GPIO, 2 por identificador de VDDC */
    unsigned char sem_ecra, so_um_ecra; /* no vale con la pantalla encendida / solo con una pantalla */
    unsigned char pcie_lanes, indice_bios, reservado[2];
} info_gpu_estado_t;
typedef struct {
    unsigned int versao;                /* 1 */
    unsigned int soporte;               /* GPU_EN_SIN, GPU_EN_LECTURA, GPU_EN_CONTROL */
    unsigned int fabricante, modelo;
    char         driver[16];            /* "radeon-r5xx", "simulado" o "" */
    char         motivo[120];           /* qué tarjeta y qué se pudo leer, o por qué no */
    char         bios[32];              /* el nombre de la BIOS de vídeo (ATOM) */
    unsigned int modo;                  /* GPU_EN_MODO_* en uso */
    unsigned int modo_kconfig;          /* lo que dice kconfig gpu_energia (0 = automático) */
    unsigned int confirmada;            /* 1: la tarjeta está en la lista de probadas (el automático la usa) */
    unsigned int bloqueado;             /* 1: falló un cambio y se volvió a la BIOS: no se reintenta en este arranque */
    int          estado;                /* índice del estado puesto; -1 = los relojes de la BIOS (sin tocar) */
    unsigned int sclk_khz, mclk_khz;    /* relojes actuales */
    unsigned int relojes_fuente;        /* 0 no se sabe, 1 leídos por la BIOS (GetEngineClock), 2 los de la BIOS sin leer (por defecto), 3 los últimos puestos */
    unsigned int relojes_edad_s;        /* hace cuánto se leyeron (fuente 1) */
    unsigned int sclk_def_khz, mclk_def_khz;   /* por defecto de la BIOS (FirmwareInfo) */
    unsigned int sclk_max_khz, mclk_max_khz;   /* máximos del chip según la BIOS */
    unsigned int sclk_arranque_khz, mclk_arranque_khz;   /* los del arranque (a los que vuelve «bios») */
    unsigned int ref_khz;               /* reloj de referencia de los PLL */
    unsigned int tensao_mv;             /* tensión actual si se sabe (0 = no) */
    unsigned int tensao_txt_baixa;      /* 1 = el estado puesto baja la tensión (GPIO) */
    unsigned int n_estados;
    info_gpu_estado_t estados[GPU_EN_MAX_ESTADOS_ABI];
    int          estado_bajo, estado_alto;   /* lo que serían «bajo» y «alto» con las pantallas de ahora */
    int          temp_mili_c;           /* GPU_EN_SIN_SENSOR si la tarjeta no lo tiene */
    char         temp_fuente[48];       /* de dónde sale, o por qué no hay */
    unsigned int n_zonas;               /* zonas térmicas del ACPI (aparte: no son de la tarjeta) */
    int          zona_mili_c[GPU_EN_MAX_ZONAS];
    char         zona_nome[GPU_EN_MAX_ZONAS][8];
    unsigned int vram_kb;               /* memoria de la tarjeta */
    unsigned int vram_usada_kb;         /* lo que usa SaguiOS: pantalla, 3D y tablas (estimado) */
    unsigned int cambios, fallos;       /* cambios de estado desde el arranque */
    unsigned int ultimo_us;             /* lo que tardó el último cambio (con todo parado) */
    unsigned int actividad;             /* auto: ventanas ocupadas seguidas (0-) */
    char         ultimo[96];            /* el resultado de la última orden o cambio */
    unsigned int reservado[8];
} info_gpu_energia_t;
/* D368 (Administrador de tareas): actividad de la caché de disco, para las gráficas de Disco. Los contadores
 * son de 32 bits desde el arranque; el programa saca las tasas por segundo de la diferencia entre dos lecturas.
 * Un bloque son 4 KB. Sin caché de disco (--sem-cache-disco) todo vale 0 y "ativa" 0. */
#define INFO_DISCO_CACHE        84  /* (&info_disco_cache_t, sizeof) -> 0 */
typedef struct {
    unsigned int versao;       /* 1 */
    unsigned int ativa;        /* 1 = hay caché de disco */
    unsigned int blocos, max_blocos, sujos;   /* bloques en la caché, tope y pendientes de grabar */
    unsigned int acertos, falhas;             /* lecturas servidas por la caché / que fueron al disco */
    unsigned int anticipados;                 /* bloques leídos por adelantado */
    unsigned int gravados, vaciados;          /* bloques grabados en el disco; vaciados de sucios */
    unsigned int diretos, erros;              /* accesos sin caché y errores de E/S */
    unsigned int reservado[4];
} info_disco_cache_t;
#define KCONF_PEDIDO(op, h, v) ((unsigned)(op) | (unsigned)(h) << 8 | (unsigned)(v) << 16)
#define KCONF_OP_FIJAR    1   /* guarda "valor" para el hueco */
#define KCONF_OP_QUITAR   2   /* olvida lo guardado del hueco: vuelve el de la imagen */
#define KCONF_OP_QUITAR_TODO 3   /* olvida todo (kconfig --por-defecto) */
#define KCONF_SINO   1        /* 0 = no, 1 = sí */
#define KCONF_NUM    2        /* 0 = automático, o de min a max */
#define KCONF_LISTA  3        /* 0 = automático, o de 1 a max (los nombres los pone el programa) */
#define KCONF_EST_RESCATE  1  /* este arranque es el modo rescate: lo guardado no se aplicó */
#define KCONF_EST_DANADA   2  /* la zona guardada no pasa la suma: no se aplicó */
#define KCONF_EST_CD       4  /* arranque desde CD: no se puede guardar */
#define KCONF_IGN_RANGO    1  /* info_kconf_t.ignorada: el valor guardado está fuera de rango */
#define KCONF_IGN_RESCATE  2  /* info_kconf_t.ignorada: no se aplicó por el modo rescate */
typedef struct {
    char           nome[24];     /* la clave de kconfig: "pae", "cache_max_mb"... */
    unsigned char  hueco, tipo;  /* KCONF_SINO, KCONF_NUM, KCONF_LISTA */
    unsigned char  guardado;     /* 1 si hay un valor guardado (lo de "valor_guardado") */
    unsigned char  ignorada;     /* KCONF_IGN_*: lo guardado no se aplicó en este arranque */
    unsigned short min, max;
    unsigned short imagem;       /* el de la imagen (mkimagem.py) */
    unsigned short valor_guardado;
    unsigned short efetivo;      /* el de este arranque (el gobernador: el de ahora) */
    unsigned short proximo;      /* el del siguiente arranque: el guardado o, sin él, el de la imagen */
} info_kconf_t;
#define INFO_GPU3D_CTL    74  /* (op, 0): 0 = apagar, 1 = encender (lotes), 2 = encender con las órdenes en el anillo; D358: 3/4 = como 1/2 con RAM en el GTT (Intel Gen3); D406: 5/6 = como 1/2 sin la GART que las R7xx confirmadas encienden solas */

/* Captura de sonido / micrófono (D292). Una sola llamada, como la cámara: info_sistema(INFO_CAPTURA,
 * &info_captura_t, sizeof). El campo "orden" dice qué se pide; el resultado va también en "resultado".
 * Los servicios (de_servico) no pueden grabar (ERRO_ACESSO). Una captura abierta a la vez, de un proceso.
 * Las muestras son PCM de 16 bits con signo, entrelazadas; el kernel convierte desde la tasa nativa de
 * la tarjeta (48 kHz) a la pedida (8000 a 48000 Hz, p. ej. 44100) y a mono o estéreo.
 *   CAP_ESTADO    describe la fuente pedida (CAP_FUENTE_REAL / CAP_FUENTE_PRUEBA): presente, nombre, entradas,
 *                 entrada elegida, ganancia, silencio, nivel (pico y RMS de los últimos ~50 ms), cuadros perdidos.
 *   CAP_ABRIR     empieza a capturar (taxa, canais) -> 0 / ERRO_OCUPADO / ERRO_NAO_EXISTE.
 *   CAP_LER       copia como mucho buf_tam bytes ya convertidos a buf (no bloquea) -> bytes copiados (0 = aún no hay).
 *   CAP_FECHAR    para la captura.
 *   CAP_ENTRADA   escribir = 1: elige la entrada "entrada" (índice de ESTADO); vale con la captura cerrada.
 *   CAP_GANANCIA  escribir = 1: ganancia 0-100 (valor) y silencio (mudo); vale con la captura cerrada.
 * La fuente de prueba es un tono de 1 kHz (amplitud 8000) generado en el kernel con el mismo
 * canal de conversión: sirve para probar sin micrófono. */
#define INFO_CAPTURA      75  /* (&info_captura_t, sizeof) -> 0 / error (< 0) */
#define INFO_ISA          76  /* (info_isa_t[], n) -> cuántos: dispositivos ISA detectados (D302) */
typedef struct {
    char           nome[48];               /* "Sound Blaster 16" */
    char           detalhe[64];            /* "DSP 4.05, OPL3 en 388h" */
    unsigned short porta;                  /* puerto base */
    unsigned char  irq, dma8, dma16;       /* 0xFF = ninguno */
    unsigned char  reservado[3];
} info_isa_t;
#define CAP_ESTADO    0
#define CAP_ABRIR     1
#define CAP_LER       2
#define CAP_FECHAR    3
#define CAP_ENTRADA   4
#define CAP_GANANCIA  5
#define CAP_FUENTE_REAL   0
#define CAP_FUENTE_PRUEBA 2
#define CAP_ENTRADAS_MAX  8
#define CAP_TONO_PRUEBA   1000   /* Hz */
#define CAP_TONO_AMPLITUD 8000
typedef struct {
    unsigned int   orden;        /* CAP_* */
    int            resultado;    /* lo mismo que devuelve la llamada */
    unsigned int   fuente;       /* CAP_FUENTE_* */
    unsigned int   taxa;         /* ABRIR: Hz pedidos; ESTADO: tasa nativa */
    unsigned int   canais;       /* ABRIR: 1 o 2 */
    unsigned int   presente;     /* ESTADO: la fuente existe */
    unsigned int   abierta;      /* ESTADO: está capturando */
    unsigned int   n_entradas;   /* ESTADO */
    unsigned int   entrada;      /* ESTADO: la elegida; ENTRADA: la que se pide */
    unsigned int   ganancia;     /* ESTADO: 0-100; GANANCIA: la que se pide */
    unsigned int   mudo;         /* ESTADO: silencio; GANANCIA: el que se pide */
    unsigned int   escribir;     /* ENTRADA y GANANCIA: 1 = cambiar */
    char           nome[48];     /* ESTADO: la tarjeta */
    char           entradas[CAP_ENTRADAS_MAX][24];   /* ESTADO: nombres de las entradas */
    unsigned int   pico, rms;    /* ESTADO: 0-32767, de los últimos ~50 ms */
    unsigned int   cuadros;      /* ESTADO: cuadros nativos capturados desde que se abrió */
    unsigned int   perdidos;     /* ESTADO: cuadros nativos perdidos (nadie leyó a tiempo) */
    void          *buf;          /* LER */
    unsigned int   buf_tam;      /* LER */
} info_captura_t;

/* Mandos y joysticks (D307): USB HID (joystick 0x04, mando 0x05, multieje 0x08), el mando de
 * Xbox 360 con cable y el joystick analógico del puerto de juegos (0x201). Se leen por sondeo,
 * como el estado del teclado de los juegos: info_sistema(INFO_MANDOS, &info_mandos_t, sizeof).
 * El campo "orden" dice qué se pide; el resultado va también en "resultado".
 *   MANDOS_LISTA     rellena "lista" (n mandos) y "geracao", que cambia con cada conexión o
 *                    desconexión: quien la guarda sabe si tiene que volver a listar.
 *   MANDOS_ESTADO    el estado del mando "indice" de la lista (ejes de -32768 a 32767, X a la
 *                    derecha y Y hacia abajo positivos; botones bit a bit; hats MANDO_HAT_*).
 *                    Si es el del puerto de juegos, se mide en ese momento (como mucho cada
 *                    20 ms) y "crudo" trae los tiempos de cada eje (décimas de us).
 *   MANDOS_DETECTAR  vuelve a buscar el joystick del puerto de juegos -> 1 si lo hay, 0 si no.
 *   MANDOS_CALIBRAR  "cal": calibración de los 4 ejes del puerto (mínimo, centro y máximo en
 *                    décimas de us); escribir = 1 la pone (0 = la lee); escribir = 2 vuelve a
 *                    la automática.
 *   MANDOS_INYECTAR  solo con la imagen de prueba (mkimagem.py --mandos-prueba): un informe
 *                    crudo para el mando de prueba "indice", por el mismo camino que los USB.
 *   MANDOS_ESTADO también deja en "informe"/"informe_n" el último informe crudo del mando (USB).
 *   MANDOS_DESCRIPTOR  el descriptor de informe del mando "indice" desde el byte "escribir":
 *                    hasta 64 bytes en "informe" ("informe_n" bytes); "n" = su tamaño total.
 * El número 77 (el 76 es INFO_ISA de la v8.3.3-sb16). */
#define INFO_MANDOS       77  /* (&info_mandos_t, sizeof) -> 0 / ERRO_* (< 0) */
#define MANDOS_LISTA      0
#define MANDOS_ESTADO     1
#define MANDOS_DETECTAR   2
#define MANDOS_CALIBRAR   3
#define MANDOS_INYECTAR   4
#define MANDOS_DESCRIPTOR 5
#define MANDOS_MAX        8
#define MANDO_EIXOS_MAX   8
#define MANDO_BOTOES_MAX  32
#define MANDO_HATS_MAX    4
#define MANDO_TIPO_HID     1   /* USB HID */
#define MANDO_TIPO_XBOX360 2   /* mando de Xbox 360 con cable (clase 0xFF/0x5D/0x01) */
#define MANDO_TIPO_PUERTO  3   /* joystick analógico del puerto de juegos */
#define MANDO_TIPO_PRUEBA  4   /* mando de prueba (--mandos-prueba): "via" dice qué decodificador usa */
#define MANDO_HAT_CENTRO   0
#define MANDO_HAT_CIMA     1
#define MANDO_HAT_DIREITA  2
#define MANDO_HAT_BAIXO    4
#define MANDO_HAT_ESQUERDA 8
/* Distribución conocida de los botones, para el mapeo de SDL_GameController. */
#define MANDO_MAPA_NENHUM      0
#define MANDO_MAPA_XBOX360     1   /* A B X Y LB RB Back Start Guía L3 R3; ejes LX LY LT RX RY RT */
#define MANDO_MAPA_DUALACTION  2   /* Logitech Dual Action / F310 en modo D: 1=X 2=A 3=B 4=Y 5=LB 6=RB 7=LT 8=RT 9=Back 10=Start 11=L3 12=R3 */
typedef struct {
    unsigned int   id;           /* único: cambia al desconectar y volver a conectar */
    unsigned char  tipo;         /* MANDO_TIPO_* */
    unsigned char  n_eixos, n_botoes, n_hats;
    unsigned short fabricante, produto;   /* USB: VID y PID; puerto: 0 */
    unsigned char  mapa;         /* MANDO_MAPA_* */
    unsigned char  reservado[3];
    char           nome[48];
    char           via[16];      /* "USB xHCI", "puerto 0x201"... */
} info_mando_t;
typedef struct {
    unsigned int   id;           /* el del mando leído */
    unsigned int   informes;     /* informes recibidos (o lecturas del puerto) */
    short          eixos[MANDO_EIXOS_MAX];
    unsigned int   botoes;       /* bit 0 = botón 1 */
    unsigned char  hats[MANDO_HATS_MAX];
} estado_mando_t;
typedef struct {
    unsigned int   orden;        /* MANDOS_* */
    int            resultado;    /* lo mismo que devuelve la llamada */
    unsigned int   indice;       /* ESTADO e INYECTAR: el mando */
    unsigned int   geracao;      /* LISTA */
    unsigned int   n;            /* LISTA: cuántos */
    info_mando_t   lista[MANDOS_MAX];
    estado_mando_t estado;       /* ESTADO */
    unsigned short crudo[4];     /* ESTADO del puerto: tiempo de cada eje (décimas de us; 0xFFFF = sin eje) */
    unsigned short cal[4][3];    /* CALIBRAR: mínimo, centro y máximo de cada eje (décimas de us) */
    unsigned int   escribir;     /* CALIBRAR */
    unsigned int   calibrado;    /* CALIBRAR: 1 = puesta a mano; 0 = la automática */
    unsigned int   informe_n;    /* INYECTAR, ESTADO (último informe crudo) y DESCRIPTOR */
    unsigned char  informe[64];  /* INYECTAR, ESTADO y DESCRIPTOR */
} info_mandos_t;

/* Estados de la capa. */
#define GPU3D_SIN_PROBAR   0  /* hay driver; se enciende y se prueba con el primer uso */
#define GPU3D_ATIVO        1
#define GPU3D_FALHOU       2  /* la autoprueba falló o el vigilante lo apagó: software */
#define GPU3D_APAGADO      3  /* a mano («gpu3d apagar») */
#define GPU3D_SEM_GPU      4  /* ninguna tarjeta con driver 3D */
#define GPU3D_DESATIVADO   5  /* la opción de arranque (SGL_MOTOR=software) */

#define GPU3D_SEM_SUPORTE  1  /* la GPU no sabe hacer eso (textura grande, destino que no cabe): quien
                                 llama sigue por software, sin apagar el 3D para los demás */
#define GPU3D_TEXTURAS    32  /* huecos de textura por dueño (D404: eran 8; la SaguiGL lo pregunta
                                 subiendo una de 1x1 al hueco 31, y la SDL 2 sigue con 8) */
#define GPU3D_MAX_LADO  2048  /* lado máximo del destino y de una textura */
#define GPU3D_MAX_VERT  3072  /* vértices por petición (múltiplo de 3) */

/* Un vértice: ocho palabras de 32 bits. x, y, z, rhw, u y v son los bits de un
 * float IEEE-754 (el kernel no usa coma flotante: los copia). x e y en píxeles
 * del destino (origen arriba a la izquierda, centros en .5), z de 0 a 1, rhw =
 * 1/w (corrección de perspectiva), cor = 0xAARRGGBB, espec = 0x00RRGGBB que se
 * suma al final (la niebla la calcula quien llama), u y v de 0 a 1. */
typedef struct { unsigned int x, y, z, rhw, cor, espec, u, v; } gpu3d_vert_t;

#define G3D_Z_TESTE      0x01
#define G3D_Z_ESCRITA    0x02
#define G3D_MEZCLA       0x04
#define G3D_TEXTURA      0x08
#define G3D_TEX_REPETE_S 0x10   /* si no, se sujeta al borde */
#define G3D_TEX_REPETE_T 0x20
#define G3D_TEX_LINEAR   0x40   /* si no, el vecino más próximo */
#define G3D_Z_MENOR       0
#define G3D_Z_MENOR_IGUAL 1
#define G3D_Z_SEMPRE      2
#define G3D_F_ZERO        0
#define G3D_F_UM          1
#define G3D_F_ALFA        2     /* alfa del origen */
#define G3D_F_UM_MENOS_ALFA 3
typedef struct {
    unsigned int  flags;        /* G3D_* */
    unsigned char func_z;       /* G3D_Z_MENOR... */
    unsigned char mez_orig, mez_dest;   /* G3D_F_* */
    unsigned char textura;      /* hueco */
} gpu3d_estado_t;

#define GPU3D_OP_ABRIR      1   /* a = ancho, b = alto, c = bits de Z (16 o 24) | bits de esténcil << 8 (D425: 0 u 8): toma el motor para este proceso */
#define GPU3D_OP_FECHAR     2
#define GPU3D_OP_LIMPAR     3   /* a = 1 color | 2 Z, b = color 0x00RRGGBB (Z siempre al fondo) */
#define GPU3D_OP_TEXTURA    4   /* a = hueco, b = ancho, c = alto, ptr = píxeles 0xAARRGGBB (ptr 0: soltarla) */
#define GPU3D_OP_ESTADO     5   /* ptr = gpu3d_estado_t */
#define GPU3D_OP_TRIANGULOS 6   /* ptr = gpu3d_vert_t[], a = vértices (múltiplo de 3) */
#define GPU3D_OP_LER        7   /* ptr = destino 0x00RRGGBB, a = píxeles por fila del destino: espera a la GPU y copia */
typedef struct { unsigned int op, a, b, c, ptr, bytes; } gpu3d_pedido_t;

/* ---- gpu3d v2 (D425): OpenGL ES 1.1 completo por la GPU ----
 *
 * Todo lo de arriba (v1) sigue igual: un driver sin v2 funciona como antes y
 * SaguiGL solo usa estas operaciones si el driver las anuncia en sus bits de
 * capacidad (GPU3D_OP_CAPACIDADES). Lo que no cabe en lo anunciado lo dibuja
 * SaguiGL por software. Detalles y reglas para los drivers: docs/gpu3d.md,
 * sección «gpu3d v2». Como en v1, solo enteros de 32 bits o menos. */
#define GPU3D_OP_CAPACIDADES 8  /* ptr = gpu3d_capas_t: qué sabe hacer el driver (antes o después de ABRIR) */
#define GPU3D_OP_ESTADO2     9  /* ptr = gpu3d_estado2_t */
#define GPU3D_OP_TRIANGULOS2 10 /* ptr = gpu3d_vert2_t[], a = vértices (múltiplo de 3, hasta GPU3D_MAX_VERT) */
#define GPU3D_OP_TEXTURA2    11 /* ptr = gpu3d_textura2_t: un nivel de una cara de una textura (o soltar el hueco) */
#define GPU3D_OP_LIMPAR2     12 /* ptr = gpu3d_limpar2_t */
#define GPU3D_OP_DESTINO     13 /* a = hueco de textura o GPU3D_DESTINO_PRINCIPAL, b = bits de Z (0, 16, 24), c = bits de esténcil (0, 8) */
#define GPU3D_DESTINO_PRINCIPAL 0xFFFFFFFFu
/* D432: presentar sin volver a la RAM. Copia (escalando) un rectángulo del destino principal o de un
 * hueco de textura a la superficie visible de la pantalla. Solo para el dueño del motor, con una ventana
 * suya a pantalla completa y con el foco: el compositor le cede la pantalla mientras dure (docs/gpu3d.md,
 * «Presentar sin volver a la RAM»). GPU3D_SEM_SUPORTE: ahora no (hay un aviso, Alt+Tab, el salvapantallas,
 * varias pantallas, el driver no sabe...): se presenta como siempre (LER y actualizar la ventana). */
#define GPU3D_OP_APRESENTAR  14 /* ptr = gpu3d_apresentar_t */
typedef struct {
    unsigned int   fonte;          /* GPU3D_DESTINO_PRINCIPAL o un hueco de textura ARGB */
    unsigned int   janela;         /* la ventana (del proceso, a pantalla completa y con el foco) */
    unsigned short ox, oy, ol, oa; /* rectángulo de la fuente, en sus píxeles */
    unsigned short dx, dy, dl, da; /* rectángulo de destino, en píxeles del contenido de la ventana; el resto queda negro */
    unsigned int   flags;          /* G3D_APRES_* */
    unsigned int   reservado[3];
} gpu3d_apresentar_t;
#define G3D_APRES_LINEAR     0x1u   /* filtro bilineal al escalar (sin él, vecino más próximo) */
#define G3D_APRES_VSYNC      0x2u   /* esperar al retrazado vertical antes de copiar (si el driver sabe; con plazo) */

/* Bits de capacidad (gpu3d_capas_t.bits). Un driver solo anuncia lo que pasa
 * su propia prueba; la capa quita los que dependen de una función que el
 * driver no tiene (G3D_CAP_V2 sin desenhar2, formatos sin textura2...). */
#define G3D_CAP_V2           0x00000001u  /* entiende gpu3d_estado2_t y gpu3d_vert2_t (desenhar2) */
#define G3D_CAP_COMBINAR     0x00000002u  /* todas las funciones de G3D_CB_* salvo DOT3 */
#define G3D_CAP_DOT3         0x00000004u  /* G3D_CB_DOT3_RGB y G3D_CB_DOT3_RGBA */
#define G3D_CAP_ALFA_TESTE   0x00000008u
#define G3D_CAP_ESTENCIL     0x00000010u  /* esténcil de 8 bits (y LIMPAR2 con esténcil) */
#define G3D_CAP_TIJERA       0x00000020u
#define G3D_CAP_MASCARAS     0x00000040u  /* máscara de color por canal (la de Z ya está en v1) */
#define G3D_CAP_Z_FUNCS      0x00000080u  /* las 8 funciones de comparación para el Z */
#define G3D_CAP_MEZCLA_EXT   0x00000100u  /* factores de mezcla G3D_F_* 4 a 10 */
#define G3D_CAP_MEZCLA_SEP   0x00000200u  /* factores distintos para el color y el alfa */
#define G3D_CAP_MEZCLA_EQ    0x00000400u  /* ecuaciones de resta (G3D_EQ_*) */
#define G3D_CAP_LOGICA       0x00000800u  /* operación lógica (G3D_LOG_*) */
#define G3D_CAP_NEBLINA      0x00001000u  /* niebla por el factor del vértice (alfa de espec) */
#define G3D_CAP_TEX_ESPELHO  0x00002000u  /* G3D_ENV_ESPELHO */
#define G3D_CAP_TEX_NPOT     0x00004000u  /* lados que no son potencia de 2, con cualquier envoltura, sin mipmaps */
#define G3D_CAP_TEX_MIPMAP   0x00008000u  /* niveles > 0 y filtros G3D_FIL_MIP_* */
#define G3D_CAP_TEX_CUBO     0x00010000u  /* G3D_TEX_CUBO */
#define G3D_CAP_TEX_ANISO    0x00020000u  /* gpu3d_unidade_t.aniso > 1 */
#define G3D_CAP_TEX_PROJ     0x00040000u  /* coordenadas proyectivas (s/q, t/q) en 2D */
#define G3D_CAP_DESTINO_TEX  0x00080000u  /* GPU3D_OP_DESTINO: dibujar en una textura */
#define G3D_CAP_LIMPAR2      0x00100000u  /* GPU3D_OP_LIMPAR2 (máscaras, tijera, Z y esténcil dados) */
#define G3D_CAP_TEX_NPOT_MIP 0x00200000u  /* 2026-11-20: además, lados que no son potencia de 2 con mipmaps (con G3D_CAP_TEX_MIPMAP) */
#define G3D_CAP_APRESENTAR   0x00400000u  /* D432: GPU3D_OP_APRESENTAR (no depende de G3D_CAP_V2) */

/* Formatos de textura (gpu3d_textura2_t.formato; gpu3d_capas_t.formatos = bits 1 << G3D_FMT_*). */
#define G3D_FMT_ARGB8888 0   /* palabras 0xAARRGGBB, como en v1 */
#define G3D_FMT_DXT1     1   /* S3TC: bloques de 4x4 de 8 bytes (con alfa de 1 bit) */
#define G3D_FMT_DXT3     2   /* bloques de 16 bytes, alfa explícito de 4 bits */
#define G3D_FMT_DXT5     3   /* bloques de 16 bytes, alfa interpolado */

typedef struct {
    unsigned int versao;        /* 2 */
    unsigned int bits;          /* G3D_CAP_* */
    unsigned int unidades;      /* unidades de textura en un dibujo (1 a 4; 1 en un driver v1) */
    unsigned int max_lado;      /* lado máximo de textura y destino */
    unsigned int max_aniso;     /* 1 = sin anisótropo */
    unsigned int formatos;      /* bits 1 << G3D_FMT_* */
    unsigned int zbits;         /* bits: 1 = Z de 16, 2 = Z de 24 */
    unsigned int estencil_bits; /* 0 u 8 */
    unsigned int reservado[8];
} gpu3d_capas_t;

/* Un vértice v2: como gpu3d_vert_t (bits de float en x, y, z, rhw) pero con
 * cuatro juegos de coordenadas de textura (s, t, r, q), uno por unidad. Para
 * una textura 2D se muestrea en (s/q, t/q); para un cubo, la dirección (s, t, r).
 * espec = 0xFFRRGGBB: el RGB se suma tras la última unidad; el byte alto es el
 * factor de niebla (255 = sin niebla) si el estado lleva G3D2_NEBLINA. */
typedef struct {
    unsigned int x, y, z, rhw, cor, espec;
    unsigned int tc[4][4];
} gpu3d_vert2_t;

/* Funciones de comparación (Z, alfa, esténcil): el orden de GL (GL_NEVER + n). */
#define G3D_CMP_NUNCA        0
#define G3D_CMP_MENOR        1
#define G3D_CMP_IGUAL        2
#define G3D_CMP_MENOR_IGUAL  3
#define G3D_CMP_MAIOR        4
#define G3D_CMP_DIFERENTE    5
#define G3D_CMP_MAIOR_IGUAL  6
#define G3D_CMP_SEMPRE       7
/* Factores de mezcla: los cuatro de v1 (G3D_F_ZERO..G3D_F_UM_MENOS_ALFA) y estos. */
#define G3D_F_COR_ORIG          4
#define G3D_F_UM_MENOS_COR_ORIG 5
#define G3D_F_COR_DEST          6
#define G3D_F_UM_MENOS_COR_DEST 7
#define G3D_F_ALFA_DEST         8
#define G3D_F_UM_MENOS_ALFA_DEST 9
#define G3D_F_ALFA_SATURADO     10   /* GL_SRC_ALPHA_SATURATE (solo de origen) */
#define G3D_EQ_SOMA          0
#define G3D_EQ_SUBTRAI       1   /* origen - destino */
#define G3D_EQ_SUBTRAI_INV   2   /* destino - origen */
/* Operación lógica: el orden de GL (GL_CLEAR + n): 0 CLEAR, 1 AND, 2 AND_REVERSE, 3 COPY,
 * 4 AND_INVERTED, 5 NOOP, 6 XOR, 7 OR, 8 NOR, 9 EQUIV, 10 INVERT, 11 OR_REVERSE,
 * 12 COPY_INVERTED, 13 OR_INVERTED, 14 NAND, 15 SET. */
#define G3D_LOG_COPIA        3
/* Esténcil: qué hacer con el valor. */
#define G3D_EST_MANTER       0
#define G3D_EST_ZERO         1
#define G3D_EST_SUBSTITUIR   2
#define G3D_EST_INCR         3   /* saturando en 255 */
#define G3D_EST_DECR         4   /* saturando en 0 */
#define G3D_EST_INVERTER     5
#define G3D_EST_INCR_CICLO   6
#define G3D_EST_DECR_CICLO   7

/* Combinación de una unidad (GL_COMBINE de TexEnv). SaguiGL convierte los
 * modos clásicos (MODULATE, REPLACE, DECAL, BLEND, ADD) a esta forma: el
 * driver solo ve combinaciones. Resultado = escala × func(arg0, arg1, arg2),
 * saturado a 0..1, con argN = operando(fonte). */
#define G3D_CB_SUBSTITUIR    0   /* arg0 */
#define G3D_CB_MODULAR       1   /* arg0 × arg1 */
#define G3D_CB_SOMAR         2   /* arg0 + arg1 */
#define G3D_CB_SOMAR_SINAL   3   /* arg0 + arg1 - 0,5 */
#define G3D_CB_INTERPOLAR    4   /* arg0 × arg2 + arg1 × (1 - arg2) */
#define G3D_CB_SUBTRAIR      5   /* arg0 - arg1 */
#define G3D_CB_DOT3_RGB      6   /* 4 × suma((arg0 - 0,5) × (arg1 - 0,5)) en R, G y B (solo color) */
#define G3D_CB_DOT3_RGBA     7   /* lo mismo también en el alfa (solo color; el alfa de la unidad se ignora) */
#define G3D_FT_TEXTURA       0   /* el texel de esta unidad */
#define G3D_FT_CONSTANTE     1   /* gpu3d_unidade_t.constante */
#define G3D_FT_PRIMARIA      2   /* el color del vértice (cor) */
#define G3D_FT_ANTERIOR      3   /* lo que sale de la unidad anterior (en la 0, la primaria) */
#define G3D_OPD_COR          0   /* solo para el color */
#define G3D_OPD_UM_MENOS_COR 1
#define G3D_OPD_ALFA         2
#define G3D_OPD_UM_MENOS_ALFA 3

#define G3D_TEX_2D           0
#define G3D_TEX_CUBO         1
#define G3D_ENV_REPETIR      0
#define G3D_ENV_BORDE        1   /* GL_CLAMP_TO_EDGE */
#define G3D_ENV_ESPELHO      2   /* GL_MIRRORED_REPEAT */
#define G3D_FIL_LINEAR_MAG   0x01   /* ampliar bilineal (si no, vecino) */
#define G3D_FIL_LINEAR_MIN   0x02   /* reducir bilineal dentro del nivel */
#define G3D_FIL_MIP_VECINO   0x04   /* con mipmaps: el nivel más próximo */
#define G3D_FIL_MIP_LINEAR   0x08   /* con mipmaps: entre dos niveles */

typedef struct {
    unsigned char ativa;        /* 0: la unidad pasa lo anterior tal cual */
    unsigned char vaga;         /* hueco de la textura */
    unsigned char tipo;         /* G3D_TEX_* */
    unsigned char filtro;       /* G3D_FIL_* */
    unsigned char env_s, env_t; /* G3D_ENV_* (en un cubo, siempre borde) */
    unsigned char aniso;        /* 1 a max_aniso */
    unsigned char coord;        /* juego de coordenadas del vértice (0..3); hoy siempre = la unidad */
    unsigned char rgb_func, alfa_func;          /* G3D_CB_* (alfa: hasta SUBTRAIR) */
    unsigned char rgb_escala, alfa_escala;      /* 1, 2 o 4 */
    unsigned char rgb_fonte[3], rgb_opd[3];     /* G3D_FT_*, G3D_OPD_* */
    unsigned char alfa_fonte[3], alfa_opd[3];   /* G3D_FT_*, G3D_OPD_ALFA o UM_MENOS_ALFA */
    unsigned int  constante;    /* 0xAARRGGBB (GL_TEXTURE_ENV_COLOR) */
} gpu3d_unidade_t;

/* gpu3d_estado2_t.flags */
#define G3D2_Z_TESTE      0x0001
#define G3D2_Z_ESCRITA    0x0002
#define G3D2_MEZCLA       0x0004
#define G3D2_ALFA_TESTE   0x0008
#define G3D2_ESTENCIL     0x0010
#define G3D2_TIJERA       0x0020
#define G3D2_NEBLINA      0x0040
#define G3D2_LOGICA       0x0080   /* en vez de la mezcla */
#define G3D_MC_R 1
#define G3D_MC_G 2
#define G3D_MC_B 4
#define G3D_MC_A 8

typedef struct {
    unsigned int  versao;       /* 2 */
    unsigned int  flags;        /* G3D2_* */
    unsigned char func_z;       /* G3D_CMP_* */
    unsigned char func_alfa;    /* G3D_CMP_* */
    unsigned char ref_alfa;     /* 0..255 */
    unsigned char mascara_cor;  /* G3D_MC_*: canales que se escriben (15 = todos) */
    unsigned char mez_orig_rgb, mez_dest_rgb, mez_orig_alfa, mez_dest_alfa;   /* G3D_F_* */
    unsigned char mez_eq_rgb, mez_eq_alfa;      /* G3D_EQ_* */
    unsigned char op_logica;    /* G3D_LOG_* (0..15) */
    unsigned char unidades;     /* unidades que cuentan (0..4): de la 0 a unidades-1 */
    unsigned char est_func, est_ref, est_mascara, est_escrita;   /* G3D_CMP_*, referencia, máscara de lectura y de escritura */
    unsigned char est_falha, est_zfalha, est_zpassa, reservado0; /* G3D_EST_* */
    unsigned short tijera_x, tijera_y, tijera_l, tijera_a;       /* píxeles del destino, origen arriba a la izquierda */
    unsigned int  cor_neblina;  /* 0x00RRGGBB */
    gpu3d_unidade_t u[4];
    unsigned int  reservado[4];
} gpu3d_estado2_t;

typedef struct {
    unsigned int vaga;          /* 0 .. GPU3D_TEXTURAS - 1 */
    unsigned int formato;       /* G3D_FMT_* */
    unsigned int tipo;          /* G3D_TEX_* */
    unsigned int face;          /* cubo: 0 +X, 1 -X, 2 +Y, 3 -Y, 4 +Z, 5 -Z; 2D: 0 */
    unsigned int nivel;         /* 0 = el grande; el nivel 0 de la cara 0 (re)crea el hueco */
    unsigned int niveis;        /* niveles que tendrá (en el nivel 0 de la cara 0) */
    unsigned int larg, alt;     /* de este nivel */
    unsigned int dados;         /* puntero del programa (0: soltar el hueco entero) */
    unsigned int bytes;         /* ARGB: larg*alt*4; DXT1: bloques*8; DXT3/5: bloques*16 */
    unsigned int reservado[2];
} gpu3d_textura2_t;

#define G3D_LIMPA_COR 1
#define G3D_LIMPA_Z   2
#define G3D_LIMPA_EST 4
typedef struct {
    unsigned int  mascara;      /* G3D_LIMPA_* */
    unsigned int  cor;          /* 0xAARRGGBB */
    unsigned int  z;            /* 0 .. 0xFFFFFF (0xFFFFFF = el fondo, como v1) */
    unsigned int  estencil;     /* 0..255 */
    unsigned char mascara_cor;  /* G3D_MC_* */
    unsigned char mascara_est;  /* bits del esténcil que se escriben */
    unsigned char tijera;       /* 1: solo dentro del rectángulo */
    unsigned char reservado0;
    unsigned short tx, ty, tl, ta;
    unsigned int  reservado[2];
} gpu3d_limpar2_t;

typedef struct {
    unsigned int versao;        /* 1 */
    unsigned int estado;        /* GPU3D_* */
    unsigned int fabricante, modelo;
    char         nome[16];      /* "intel-gen3", "simulado"... o "software" */
    char         placa[40];
    char         motivo[160];
    unsigned int dono;          /* pid que tiene el motor (0 = libre) */
    unsigned int larg, alt, zbits;   /* destino abierto */
    unsigned int triangulos, lotes, quadros, falhas, recusas;   /* desde el arranque */
    unsigned int us_autoprova;
    unsigned int memoria_kb, livre_kb;   /* memoria de GPU de la capa */
    unsigned int variante;      /* bits GPU3D_VAR_*: 0 = lotes (batch buffers), 1 = órdenes en el anillo */
    unsigned int prazo_ms;      /* plazo del vigilante */
    unsigned int pontos;        /* última puntuación 3D medida (0 = sin medir) */
    unsigned int tri_s, kpix_s; /* y sus medidas */
    unsigned int reservado[6];  /* D358: [0] = KB de RAM del sistema en el GTT (con GPU3D_VAR_GTT);
                                 * D425: [1] = bits G3D_CAP_* del driver encendido, [2] = sus unidades de textura;
                                 * D432: [3] = presentaciones directas a la pantalla, [4] = las que no se pudieron
                                 * (se presentó como siempre), [5] = ventana con la pantalla en exclusiva + 1 (0: nadie) */
} info_gpu3d_t;
/* D358: info_gpu3d_t.variante, lo que de verdad quedó en marcha (bits). */
#define GPU3D_VAR_ANEL      0x01  /* órdenes en el anillo (pedido, o el lote no se ejecutaba) */
#define GPU3D_VAR_GTT       0x02  /* destino en la RAM del sistema mapeada en el GTT (Intel Gen3) */
#define GPU3D_VAR_GTT_TEX   0x04  /* y también las texturas */

#define GPU3D_N_PROVAS 8
/* info_gpu3d_prova_t.versao de entrada (bits; D390). 2 sigue siendo «con detalle». */
#define GPU3D_PROVA_DETALLE 0x02  /* los registros y el análisis del destino al log [GPU3D] */
#define GPU3D_PROVA_VER     0x04  /* guarda la imagen de cada etapa (info_gpu3d_etapa_t) */
typedef struct {
    unsigned int versao;        /* entrada: GPU3D_PROVA_* (2 = con detalle en el log); salida: 1 */
    int          resultado;     /* 1 = bien, 0 = mal, -1 = sin GPU con 3D */
    char         texto[160];
    unsigned int provas;        /* bit n = la prueba n pasó */
    unsigned int feitas;        /* bit n = la prueba n se hizo */
    int          falhou;        /* primera que falló (-1 = ninguna) */
    int          x, y;          /* píxel que no cuadra */
    unsigned int esperado, obtido;
    unsigned int us_autoprova;
    /* medida: triángulos pequeños (transformación y órdenes) y grandes (relleno) */
    unsigned int tri_pequenos, us_pequenos, tri_grandes, us_grandes, pix_grandes;
    unsigned int us_leitura, pix_leitura;   /* traer el cuadro a la RAM */
    unsigned int tri_s, kpix_s, pontos;
    unsigned int reservado[6];  /* D390: [0] ajustes de /sistema/conf/gpu3d-<driver>.conf en uso, [1] líneas malas,
                                 * [2] 1 = hay imágenes de las etapas (GPU3D_PROVA_VER) */
} info_gpu3d_prova_t;
/* D390: «gpu3d probar --ver»: la imagen de una etapa de la última autoprueba con
 * GPU3D_PROVA_VER. Se pide por INFO_GPU3D_PROVA con versao = GPU3D_ETAPA_VERSAO y
 * n = sizeof(info_gpu3d_etapa_t) (no gasta un número de INFO nuevo). */
#define GPU3D_ETAPA_VERSAO  0x45544150u   /* «PATE» */
#define GPU3D_VER_L 128                   /* el destino de la autoprueba */
#define GPU3D_VER_A 96
#define GPU3D_ETAPA_NADA        0         /* no se llegó a ella (o no hubo --ver) */
#define GPU3D_ETAPA_BIEN        1
#define GPU3D_ETAPA_MAL         2
#define GPU3D_ETAPA_SIN_IMAGEN  3         /* falló antes de poder leer el destino */
typedef struct {
    unsigned int   versao;      /* entrada: GPU3D_ETAPA_VERSAO; salida: 1 */
    int            etapa;       /* entrada: 0 .. GPU3D_N_PROVAS - 1 */
    int            estado;      /* GPU3D_ETAPA_* */
    unsigned int   larg, alt;
    int            com_z;       /* z[] vale (la CPU ve el Z de esta tarjeta) */
    /* el análisis del destino tras la etapa (gpu3d_diag.h): la sentinela de color es a1b2c3 */
    unsigned int   cor_intacta, z_intacto, z_maximo;
    unsigned int   cores[4], n_cores[4], resto;
    int            x, y;        /* píxel que no cuadra (-1: ninguno) */
    unsigned int   esperado, obtido;
    unsigned int   reservado[4];
    unsigned int   px[GPU3D_VER_L * GPU3D_VER_A];   /* 0x00RRGGBB, lo que dejó la GPU */
    unsigned short z[GPU3D_VER_L * GPU3D_VER_A];    /* 8 bits altos del Z; 0xFFFF = sentinela intacta */
} info_gpu3d_etapa_t;
typedef struct {
    unsigned int versao;        /* 1 */
    int          resultado;     /* 1 = la GPU dio los mismos píxeles que la CPU; 0 = no; -1 = sin acelerador */
    unsigned int ordens;        /* copias y rellenos comprobados */
    unsigned int diferentes;    /* píxeles distintos */
    unsigned int soma_gpu, soma_cpu;   /* suma de los píxeles de la zona de prueba */
    unsigned int us_gpu, us_cpu;       /* medida: las mismas copias por la GPU y por la CPU */
    char         texto[160];    /* resumen para la orden */
    /* D243: medida por tipo de orden (infopc, pestaña GPU): 0 rellenos, 1 copias sin solape,
     * 2 copias solapadas. Las mismas órdenes por la GPU y por la CPU, con la suma de la zona. */
    unsigned int bn;            /* pruebas hechas (0 = no se midió) */
    struct { unsigned int ops, pixeis, us_gpu, us_cpu, soma_gpu, soma_cpu; } b[3];
} info_acel2d_prova_t;

/* La vía de cada IRQ (D211): por qué controlador y qué GSI llega cada
 * línea, con sus funciones PCI. Una fila por función PCI con interrupción y
 * una por cada línea ISA en uso que no tiene funciones PCI. El 68 (el 65
 * queda libre para otras ramas). */
#define INFO_IRQ_VIAS 68   /* (info_irq_via_t[], n) -> cuántas filas */
#define IRQ_VIA_PIC     0  /* sin IOAPIC: el 8259 de siempre */
#define IRQ_VIA_IOAPIC  1  /* por el IOAPIC */
#define IRQ_VIA_MISTA   2  /* con el IOAPIC activo, la línea entra por el 8259 (vuelta atrás de la autoprueba) */
#define IRQ_PROVA_NADA  0  /* sin autoprueba (el dispositivo no sabe provocar una interrupción) */
#define IRQ_PROVA_OK    1  /* la interrupción provocada llegó */
#define IRQ_PROVA_PIC   2  /* no llegó por el IOAPIC; por el 8259 sí */
#define IRQ_PROVA_FALHA 3  /* no llegó por ninguna vía: el driver sondea */
#define IRQ_PROVA_OCUPADO 4 /* no se pudo provocar (dispositivo ocupado) */
typedef struct {
    unsigned char irq;          /* la línea (vector 0x20 + irq) */
    unsigned char via;          /* IRQ_VIA_* */
    unsigned char apic;         /* el APIC de la CPU que la recibe */
    unsigned char prova;        /* IRQ_PROVA_* */
    unsigned char bus, disp, fun;   /* la función PCI; bus = 0xFF en una fila ISA */
    unsigned char pino;         /* 1-4 = INTA-INTD; 0 = ISA */
    unsigned int  gsi;          /* la GSI por la que llega (0xFFFFFFFF = ninguna: solo el 8259) */
    unsigned int  atendidas;    /* interrupciones atendidas en la línea */
    unsigned char prt;          /* 1 = la GSI la dio el _PRT; 0 = la línea de la BIOS (D166) */
    unsigned char nivel, baixo; /* forma de la entrada */
    unsigned char tormenta;     /* D244: 1 = enmascarada ahora por una tormenta */
    char          nome[12];     /* "EHCI", "e1000", "HDA"... si tiene autoprueba */
    unsigned int  por_segundo;  /* D244: interrupciones por segundo (última ventana) */
    unsigned int  tormentas;    /* D244: tormentas detectadas en la línea desde el arranque */
} info_irq_via_t;

/* Los núcleos (D157): uno por procesador, con su estado y su uso.
 * El número 61 (el 57 lo tiene INFO_MOTOR_GPU y el 58 INFO_APARENCIA). */
#define INFO_NUCLEOS  61  /* (info_nucleo_t[], n) -> quantas CPUs: uma por processador (D157) */
/* El driver de frecuencia por CPU (D175): una entrada por procesador. */
#define INFO_FREQ_NUCLEOS 62  /* (info_freq_nucleo_t[], n) -> cuántas CPU (ERRO_NAO_EXISTE sin driver) */
/* La pila que de verdad ocupa cada proceso (D186): las pilas crecen al
 * tocarlas, así que esto es lo que llegó a usar (no se devuelve al bajar). */
#define INFO_PILHAS   63  /* (info_pilha_t[], n) -> cuántos procesos */
typedef struct {
    int          pid;
    unsigned int pilha_kb;   /* la del hilo principal */
    unsigned int hilos_kb;   /* la suma de las de sus hilos con pila del kernel */
} info_pilha_t;

/* INFO_AUDIO */
#define AUDIO_SEM_PLACA   0   /* nenhuma placa de som no PCI */
#define AUDIO_FUNCIONANDO 1
#define AUDIO_SEM_DRIVER  2   /* há placa (fabricante/modelo), mas sem driver ou o driver falhou */
typedef struct {
    int           pid;
    unsigned int  taxa;
    unsigned char canais, bits, volume, reservado;
    unsigned int  pendente_ms;
} info_fluxo_audio_t;
#define AUDIO_SALIDAS 4
typedef struct {
    unsigned char usada, actual, reservado[2];   /* hay tarjeta en ese índice; es la salida actual */
    char          driver[8];                     /* "hda", "ac97" o "usb" */
    char          nombre[48];
} info_salida_audio_t;
typedef struct {
    unsigned char estado;            /* AUDIO_SEM_PLACA, AUDIO_FUNCIONANDO, AUDIO_SEM_DRIVER */
    unsigned char irq;               /* 0xFF = sondagem a cada tique do relógio */
    unsigned char volume, silencio;  /* volume mestre (0-100) e se está calado */
    char          driver[8];         /* "ac97" ou "hda" */
    char          placa[48];         /* o controlador */
    char          codec[64];         /* o códec e a saída usada */
    unsigned short fabricante, modelo;   /* PCI do controlador */
    unsigned int  taxa;              /* Hz da saída (48000) */
    unsigned int  tocando;           /* 1 = o DMA da placa está rodando */
    unsigned int  latencia_ms;       /* quanto o misturador escreve à frente da placa */
    unsigned int  quadros;           /* quadros tocados desde o boot (o relógio da placa) */
    unsigned int  interrupcoes, falhas;   /* IRQs da placa; vezes que o misturador se atrasou */
    unsigned int  nfluxos;
    info_fluxo_audio_t fluxo[AUDIO_FLUXOS];
    unsigned char eq_etapas;         /* 0 = ecualizador plano (derivación) */
    signed char   eq_db[EQ_BANDAS];  /* 60 Hz, 250 Hz, 1 kHz, 4 kHz, 12 kHz */
    info_salida_audio_t salidas[AUDIO_SALIDAS];   /* D275: las tarjetas conocidas (PCI y USB) */
    unsigned char balance;           /* D292: 0 izquierda, 100 centro, 200 derecha */
    unsigned char captura;           /* D292: 1 = hay entrada de sonido (micrófono) */
    unsigned short latencia_base_ms; /* D292: la latencia configurada (latencia_ms puede crecer tras un corte) */
} info_audio_t;

/* SIS_BLUETOOTH (D279) */
#define BT_ESTADO        0   /* (&info_bt_t, sizeof) -> 0; ERRO_NAO_EXISTE si no hay controlador */
#define BT_BUSCAR        1   /* () -> 0: empieza una búsqueda de ~8 s (info_bt_t.buscando) */
#define BT_DISPOSITIVOS  2   /* (disp_bt_t[], n) -> cuántos */
#define BT_EMPAREJAR     3   /* (indice) -> 0: solo emparejar (guarda la clave y se desconecta); pide administrador */
#define BT_CONECTAR      4   /* (indice) -> 0: conectar (y emparejar si hace falta); pide administrador */
#define BT_DESCONECTAR   5   /* (indice) -> 0; pide administrador */
#define BT_OLVIDAR       6   /* (indice) -> 0: desconecta y borra la clave; pide administrador */
#define BT_CONFIRMAR     7   /* (1 = coincide, 0 = no) -> 0: contesta a la comparación numérica; pide administrador */
#define BT_ENERGIA       8   /* (1 = encender, 0 = apagar) -> 0; pide administrador */
/* Estados del adaptador (info_bt_t.estado) */
#define BTA_SEM_ADAPTADOR 0
#define BTA_INICIANDO     1
#define BTA_PRONTO        2
#define BTA_FALHOU        3
#define BTA_APAGADO       4
/* Estados de un dispositivo (disp_bt_t.estado) */
#define BTD_LIVRE         0
#define BTD_LIGANDO       1
#define BTD_AUTENTICANDO  2
#define BTD_CIFRANDO      3
#define BTD_SDP           4
#define BTD_HID           5
#define BTD_CONECTADO     6
#define BTD_ERRO          7
/* Tipo de dispositivo (disp_bt_t.tipo) y modo de la confirmación pendiente (info_bt_t.conf_modo) */
#define BTT_OUTRO   0
#define BTT_TECLADO 1
#define BTT_RATO    2
#define BTT_COMBO   3
#define BTT_BLE     4   /* baja energía (BLE): solo se detecta (D411) */
#define BTT_BLE_HID 5   /* BLE con apariencia o servicio HID: se detecta, aún no se conecta */
#define BTC_NADA     0
#define BTC_COMPARAR 1   /* ¿coincide el número con el del dispositivo? BT_CONFIRMAR 1 o 0 */
#define BTC_MOSTRAR  2   /* escribe el número en el teclado del dispositivo (sin respuesta) */

typedef struct info_bt {
    unsigned int   estado;         /* BTA_* */
    char           chip[40];       /* "CSR8510 A10 (0a12:0001)" */
    unsigned char  addr[6];        /* en orden de lectura: addr[0] es el primer byte de AA:BB:CC:DD:EE:FF */
    unsigned char  buscando;
    unsigned char  energia;
    unsigned char  conf_modo;      /* BTC_* */
    signed char    conf_disp;      /* índice del dispositivo que pide la confirmación (-1 = ninguno) */
    unsigned char  n_disp;
    unsigned int   conf_num;       /* el número de seis cifras de la comparación numérica o del passkey */
    unsigned char  hci_ver, lmp_ver;
    unsigned short fabricante, acl_mtu;
    unsigned char  acl_num, reservado[3];
    unsigned int   eventos, acl_rx, acl_tx, erros_hci;
    char           erro[64];       /* último problema del adaptador ("" = ninguno) */
} info_bt_t;

typedef struct disp_bt {
    unsigned char  addr[6];        /* en orden de lectura */
    unsigned char  estado;         /* BTD_* */
    unsigned char  tipo;           /* BTT_* */
    unsigned char  pareado;        /* hay clave de enlace guardada */
    unsigned char  visto;          /* salió en la última búsqueda */
    signed char    rssi;
    unsigned int   clase;
    char           nome[32];
    char           erro[48];       /* por qué falló el último intento ("" = ninguno) */
} disp_bt_t;

/* SIS_SOCKET */
#define SOCK_TCP 1
#define SOCK_UDP 2
/* D363: la interfaz del túnel (wg0), como un TUN: ler da el siguiente paquete IPv4 que la pila
 * manda por el túnel y escrever mete uno descifrado como si hubiera llegado. Solo uno a la vez;
 * crearlo pide root o la sesión local (como el Wi-Fi, D354). Al cerrarlo (o al morir el demonio)
 * la interfaz, sus rutas y su DNS desaparecen. disponivel = bytes del siguiente paquete, 0 si no
 * hay, ERRO_PIPE_QUEBRADO si alguien pidió apagarla (REDE_TUNEL_DESLIGAR). */
#define SOCK_TUN 3
/* SIS_OPCAO_SOCKET */
#define OPC_TEMPO_LIMITE 1   /* valor = ms para ler/aceitar/conectar (0 = sem limite) -> 0 */
#define OPC_FECHAR_ENVIO 2   /* TCP: manda o FIN, mas continua lendo -> 0 */
#define OPC_ENDERECOS    3   /* valor = &enderecos_socket_t -> 0 */
#define OPC_ESTADO       4   /* -> CON_* (TCP) */
#define OPC_FISICA       5   /* UDP, D363: (1) sale siempre por la interfaz física aunque haya ruta por el túnel;
                                solo el proceso dueño del túnel (su demonio) o root */
#define OPC_TUN_CONFIG   6   /* SOCK_TUN: (&config_tunel_t) dirección, máscara, MTU, DNS y rutas -> 0 */
#define OPC_TUN_ESTADO   7   /* SOCK_TUN: (&info_wg_demonio_t) el demonio publica el estado para «wg show» -> 0 */
#define OPC_ERRO_CONEXAO 8   /* D401: TCP: 0 conectada, 1 aún en curso, < 0 el error de la conexión (SO_ERROR) */
/* D363: la interfaz del túnel (SOCK_TUN, OPC_TUN_*, INFO_WG) */
#define TUNEL_ROTAS_MAX  32
#define WG_INFO_PARES    8
typedef struct { unsigned int ip, bits; } rota_tunel_t;
typedef struct {
    char           nome[16];       /* "wg0" */
    unsigned int   ip, mascara;    /* la dirección de la interfaz (Address) */
    unsigned int   mtu;            /* 576-1500; 0 = 1420 */
    unsigned int   dns;            /* 0 = el de la interfaz física */
    unsigned int   nrotas;         /* AllowedIPs de todos los pares (0.0.0.0/0 = todo menos la red local) */
    rota_tunel_t   rotas[TUNEL_ROTAS_MAX];
} config_tunel_t;
typedef struct {
    char           publica[48];    /* base64 */
    unsigned int   endpoint_ip, endpoint_porta;
    unsigned int   ultimo_hs;      /* hora Unix del último saludo completo; 0 = nunca */
    unsigned int   keepalive_s, tem_psk;
    unsigned long long rx_bytes, tx_bytes;
    unsigned int   nips;
    rota_tunel_t   ips[8];
} info_wg_par_t;
typedef struct {
    char           publica[48];    /* la clave pública de la interfaz (base64) */
    unsigned int   porta;          /* ListenPort */
    unsigned int   npares;
    info_wg_par_t  pares[WG_INFO_PARES];
    unsigned int   descartes_repeticao, descartes_mac, descartes_origem, descartes_outros;
    unsigned int   cookies_recebidas, cookies_enviadas;
    unsigned int   reservado[4];
} info_wg_demonio_t;
typedef struct {
    unsigned int   ativo, pid, uid;
    unsigned int   desligando;     /* REDE_TUNEL_DESLIGAR pedido: el demonio está saliendo */
    config_tunel_t config;
    unsigned int   rx_pacotes, tx_pacotes, rx_bytes, tx_bytes, descartados;   /* el tráfico de dentro del túnel */
    info_wg_demonio_t wg;
} info_wg_t;
/* SIS_WIFI (D215) */
#define WIFI_ESTADO      0   /* (&info_wifi_t, sizeof) -> 0; ERRO_NAO_EXISTE si no hay pincho */
#define WIFI_BUSCAR      1   /* (0) -> 0: empieza una búsqueda (unos 3 s; WIFI_ESTADO.buscando) */
#define WIFI_REDES       2   /* (rede_wifi_t[], n) -> cuántas redes de la última búsqueda */
#define WIFI_CONECTAR    3   /* (&pedido_wifi_t) -> 0: empieza; WIFI_ESTADO dice cómo acaba */
#define WIFI_DESCONECTAR 4   /* (0) -> 0 */

/* Estados (info_wifi_t.estado) */
#define WIFI_SEM_PINCHO   0   /* no hay ningún adaptador */
#define WIFI_INICIANDO    1   /* eFuse, firmware y tablas del chip */
#define WIFI_PRONTO       2   /* encendido, sin conectar */
#define WIFI_CONECTANDO   3   /* autenticación y asociación */
#define WIFI_SAUDACAO     4   /* saludo de cuatro pasos de WPA (WPA1, WPA2 o WPA3) */
#define WIFI_CONECTADO    5
#define WIFI_FALHOU       6   /* el chip no arrancó: ver «erro» */
#define WIFI_SEM_FIRMWARE 7   /* compilación sin firmware (SEM_FIRMWARE=1) o falta el archivo */

/* Seguridad de una red (rede_wifi_t.seguranca), la de kernel/inc/wpa.h */
#define WIFI_ABERTA    0
#define WIFI_WEP       1
#define WIFI_WPA1      2   /* WPA (normalmente TKIP) */
#define WIFI_WPA2      3
#define WIFI_OUTRA     4   /* Enterprise y demás: no se admite */
#define WIFI_WPA3      5   /* solo SAE (D220) */
#define WIFI_WPA2_WPA3 6   /* transición: PSK y SAE (D220) */
#define WIFI_WPA1_WPA2 7   /* mixto: WPA y WPA2 (D220) */
/* Detalles (rede_wifi_t.detalhe e info_wifi_t.detalhe, D220) */
#define WIFI_DET_TKIP   0x01   /* se usa TKIP (de par o de grupo) */
#define WIFI_DET_PMF    0x02   /* 802.11w (obligatoria en la red, o activa en la conexión) */
#define WIFI_DET_H2E    0x04   /* SAE hash-to-element */
#define WIFI_DET_SO_H2E 0x08   /* la red solo admite H2E */
#define WIFI_MAX_PWE    4      /* PWE de SAE recordados por red (uno por punto de acceso) */

typedef struct {
    char           ssid[33];
    unsigned char  bssid[6];
    unsigned char  canal;
    signed char    sinal;          /* dBm */
    unsigned char  seguranca;      /* WIFI_ABERTA... */
    unsigned char  suportada;      /* 1 = se puede conectar (con la clave que haga falta) */
    unsigned char  qualidade;      /* 0..100 */
    unsigned char  oculta;         /* SSID vacío */
    unsigned char  detalhe;        /* WIFI_DET_* (D220) */
    unsigned char  reservado[2];
} rede_wifi_t;

typedef struct {
    unsigned char  bssid[6];
    unsigned char  pwe[64];        /* x || y */
} pwe_wifi_t;

typedef struct {
    char           ssid[33];
    unsigned char  tem_pmk;        /* 1 = red WPA/WPA2: pmk vale; 0 = abierta */
    unsigned char  pmk[32];        /* PBKDF2(contraseña, SSID): la contraseña no entra en el kernel */
    unsigned char  reservado[2];
    /* D220: WEP y WPA3 (SAE). Nada de esto es la contraseña, salvo la clave
     * WEP, que es la clave misma (WEP no tiene derivada). */
    unsigned char  nwep;           /* 0, 5 o 13 bytes */
    unsigned char  wep[13];
    unsigned char  tem_pt;         /* SAE hash-to-element: PT de (contraseña, SSID) */
    unsigned char  npwe;           /* SAE por cazar y picotear: PWE de (contraseña, MAC, BSSID) */
    unsigned char  pt[64];
    pwe_wifi_t     pwe[WIFI_MAX_PWE];
} pedido_wifi_t;

typedef struct info_wifi {
    unsigned int   estado;         /* WIFI_* */
    char           chip[40];       /* "Realtek RTL8188EUS" */
    char           firmware[24];   /* versión del firmware cargado */
    unsigned char  mac[6];
    unsigned char  buscando;       /* 1 = búsqueda en curso */
    unsigned char  canal;
    char           ssid[33];
    unsigned char  bssid[6];
    signed char    sinal;          /* dBm de las últimas balizas */
    unsigned char  qualidade;      /* 0..100 */
    unsigned char  seguranca;
    unsigned char  taxa_mbps;      /* tasa de datos actual (Mb/s; 5 = 5,5) */
    unsigned char  detalhe;        /* WIFI_DET_* de la conexión (D220) */
    unsigned char  reservado[1];
    char           erro[80];       /* el último problema ("" = ninguno) */
    unsigned int   redes;          /* de la última búsqueda */
    unsigned int   rx_quadros, tx_quadros, balizas, erros_usb;
    unsigned int   conectado_s;    /* segundos desde la conexión */
    unsigned int   respostas_sondeo;  /* respuestas de sondeo dirigidas a nosotros: la transmisión funciona */
    /* D238: rendimiento (al final: los programas viejos piden menos y no los ven) */
    unsigned int   rx_bytes, tx_bytes;     /* bytes de datos (tramas Ethernet) entregados a la red y mandados */
    unsigned int   rx_usb, tx_usb;         /* transferencias USB de recepción leídas y tramas mandadas al USB */
    unsigned int   rx_agregadas_max;       /* el máximo de tramas que trajo una transferencia */
    unsigned int   tx_cola_llena, tx_perdidas, addba_rechazados;
    unsigned short taxa_dec;               /* tasa de envío en décimas de Mb/s (65 = 6,5) */
    unsigned short taxa_rx_dec;            /* la de la última trama de datos recibida */
    unsigned char  mcs;                    /* MCS de envío (0..7); 255 = tasa b/g */
    unsigned char  mcs_rx;                 /* MCS de la última trama de datos recibida; 255 = b/g */
    unsigned char  ht;                     /* 1 = conexión 802.11n (HT); 2 = la red es HT pero se conectó b/g */
    unsigned char  sgi;                    /* 1 = GI corto (400 ns) en el envío */
    unsigned char  rx_cola, tx_cola;       /* transferencias de cada cola USB (0 = una cada vez, D215) */
    unsigned char  rx_en_vuelo, tx_en_vuelo;   /* armadas o sin terminar ahora mismo */
} info_wifi_t;

/* SIS_BLOQUEO (D218). La tabla la compila el programa «bloqueo» en
 * /home/.config/bloqueo/tabla.bin (formato en kernel/inc/bloqueo_tabla.h) y el
 * kernel la carga al arrancar y con BLOQ_RECARGAR si bloqueo.conf dice «activo 1». */
#define BLOQ_ESTADO      0   /* (&info_bloqueo_t, sizeof) -> 0 */
#define BLOQ_RECARGAR    1   /* () -> 0; relee bloqueo.conf y tabla.bin (con error, se queda la de antes) */
#define BLOQ_ULTIMOS     2   /* (char[n][64], n) -> cuántos; el más reciente primero */
#define BLOQ_CONSULTAR   3   /* (nombre, BLOQC_*) -> BLOQ_LIBRE / BLOQ_BLOQUEADO / BLOQ_PERMITIDO */
#define BLOQ_CONTADORES  4   /* () -> 0; pone a cero las consultas, las bloqueadas y los últimos */
/* Opciones de BLOQ_CONSULTAR */
#define BLOQC_CONTAR     1   /* cuenta la consulta y, si se bloquea, la anota en los últimos */
#define BLOQC_SIEMPRE    2   /* mira la tabla aunque el bloqueo esté desactivado (si está cargada) */

typedef struct {
    unsigned int   activo;         /* 1 = filtrando (bloqueo.conf dice activo y la tabla está cargada) */
    unsigned int   pedido;         /* 1 = bloqueo.conf dice «activo 1» */
    unsigned int   subdominios;    /* 1 = un dominio de la lista bloquea también sus subdominios */
    unsigned int   bloqueados;     /* entradas de la tabla de bloqueo */
    unsigned int   permitidos;
    unsigned int   bytes;          /* memoria del kernel que ocupa la tabla */
    unsigned int   consultas;      /* nombres consultados desde el arranque (o desde BLOQ_CONTADORES) */
    unsigned int   bloqueadas;
    unsigned int   cargas;         /* veces que se cargó la tabla */
    int            erro;           /* la última carga: 0 = bien; ERRO_* */
    unsigned int   carga_us;       /* lo que tardó la última carga (microsegundos) */
    unsigned int   reservado[5];
} info_bloqueo_t;

/* SIS_CONFIG_REDE */
#define REDE_CONSULTAR    0   /* preenche config_rede_t */
#define REDE_FIXAR        1   /* endereço estático (para o DHCP) */
#define REDE_DHCP         2   /* pede endereço por DHCP e espera até 10 s */
#define REDE_DNS          3   /* só troca o servidor DNS */
#define REDE_EXPERIMENTAL 4   /* liga o driver experimental (RTL810x) */
#define REDE_RENOVAR      5   /* (0) renova (ou pede) o DHCP e volta na hora, sem esperar (D079) */
#define REDE_DESATIVAR    6   /* (0) desliga a eth0: nada entra nem sai; o endereço fica (D079) */
#define REDE_ATIVAR       7   /* (0) religa a eth0 (e renova o DHCP, se era o caso) */
#define REDE_FECHAR       8   /* (id de info_socket_t, no lugar do ponteiro) corta a conexão TCP com RST */
#define REDE_AJUSTAR_DRIVER 9 /* (&ajuste_driver_t) D124: ajustes en caliente del driver experimental (r8169) */
#define REDE_TUNEL_DESLIGAR 10 /* (0) D363: pide al demonio del túnel (wg0) que lo apague (wg-quick down); root o la sesión local */
/* Como o endereço foi obtido */
#define REDE_SEM_IP   0
#define REDE_POR_DHCP 1
#define REDE_ESTATICO 2
/* Estados de uma conexão (INFO_CONEXOES e OPC_ESTADO) */
#define CON_FECHADA     0
#define CON_ESCUTANDO   1
#define CON_SYN_ENVIADO 2
#define CON_SYN_RECEBIDO 3
#define CON_ESTABELECIDA 4
#define CON_FECHANDO    5   /* um dos lados já mandou o FIN */
#define CON_UDP         6
/* Estado TCP detalhado (INFO_SOCKETS, D079): os do RFC 793 que a pilha usa
 * (sem TIME-WAIT, ver D046). */
#define TCP_FECHADA       0
#define TCP_ESCUTA        1
#define TCP_SYN_ENVIADO   2
#define TCP_SYN_RECEBIDO  3
#define TCP_ESTABELECIDA  4
#define TCP_FIN_ESPERA    5   /* FIN-WAIT: nós mandamos o FIN */
#define TCP_FECHAR_ESPERA 6   /* CLOSE-WAIT: o outro lado mandou o FIN */
#define TCP_ULTIMO_ACK    7   /* os dois mandaram: falta a última confirmação */

typedef struct {
    unsigned int ip, mascara, roteador, dns;
    unsigned int metodo;           /* REDE_SEM_IP, REDE_POR_DHCP, REDE_ESTATICO */
} config_rede_t;

/* D124: "ifconfig eth0 r8169 rxcfg=... rms=... multi=0|1 buf=... reiniciar-rx".
 * Solo cuenta lo marcado en "mascara" (AJUSTE_*). */
#define AJUSTE_RXCFG     0x01
#define AJUSTE_RMS       0x02
#define AJUSTE_MULTI     0x04
#define AJUSTE_BUF       0x08
#define AJUSTE_REINICIAR 0x10   /* reprograma la tarjeta y rellena los anillos */
typedef struct {
    unsigned int mascara;
    unsigned int rxcfg, rms, multi, buf;
} ajuste_driver_t;

typedef struct {
    unsigned int ip;
    unsigned int porta;
} endereco_rede_t;

typedef struct {
    unsigned int ip_local, porta_local, ip_remoto, porta_remota;
} enderecos_socket_t;

typedef struct {
    unsigned int seq;              /* entrada: número de sequência; tamanho: bytes de dados (0 = 56) */
    unsigned int tamanho;
    unsigned int us;               /* saída: tempo até a resposta, em microssegundos */
    unsigned int ttl;
} info_ping_t;

typedef struct {
    char           nome[8];        /* "eth0" */
    char           driver[12];     /* "e1000"; "" = placa sem driver */
    char           descricao[40];
    unsigned char  mac[6];
    unsigned char  cabo;           /* 1 = enlace ativo */
    unsigned char  ativa;          /* 1 = driver ligado; 0 = só detectada */
    unsigned char  experimental;
    unsigned char  irq;            /* 0xFF = sem interrupção (sondagem a cada 10 ms) */
    unsigned short fabricante, modelo;
    unsigned char  barramento, dispositivo, funcao, dhcp_estado;   /* 0 parado 1 descobrindo 2 pedindo 3 pronto 4 renovando */
    unsigned int   ip, mascara, roteador, dns, metodo;
    unsigned int   servidor_dhcp, concessao_s;   /* segundos que faltam da concessão */
    unsigned int   rx_pacotes, tx_pacotes, rx_bytes, tx_bytes;
    unsigned int   rx_erros, tx_erros, descartados, irqs;
    /* D079: campos novos no fim. INFO_REDE copia só os n bytes pedidos
     * (n = sizeof do programa), então quem foi compilado antes continua igual. */
    unsigned int   desativada;     /* 1 = desligada com REDE_DESATIVAR */
    unsigned int   pode_desativar; /* 1 = REDE_DESATIVAR/REDE_ATIVAR valem nela */
    unsigned int   loopback;       /* 1 = "lo" (127.0.0.1): só conta o tráfego para a própria máquina */
    /* D091: por que os quadros recebidos foram descartados e o diagnóstico
     * do driver experimental (linhas separadas por \n; "" = nada). */
    char           diagnostico[3072];
} info_rede_t;

typedef struct {
    unsigned int   tipo;           /* SOCK_TCP ou SOCK_UDP */
    unsigned int   estado;         /* CON_* */
    unsigned int   ip_local, porta_local, ip_remoto, porta_remota;
    unsigned int   na_fila_rx, na_fila_tx;
    int            pid;            /* dono (0 = sem descritor, terminando) */
} info_conexao_t;

/* INFO_SOCKETS (D079) */
typedef struct {
    info_conexao_t c;              /* o mesmo de INFO_CONEXOES (pid 0 = sem dono) */
    unsigned int   estado_tcp;     /* TCP_* (UDP: TCP_FECHADA) */
    unsigned int   id;             /* para REDE_FECHAR (0 = não dá para fechar daqui) */
    unsigned int   rx_bytes, tx_bytes;   /* dados recebidos e enviados nesta conexão */
} info_socket_t;

/* INFO_TRAFEGO (D079): o kernel guarda uma amostra por segundo, sempre (é só
 * uma subtração por segundo), para o gráfico já abrir com o último minuto. */
#define TRAFEGO_AMOSTRAS 60
typedef struct {
    unsigned int amostras;         /* quantas valem (cresce até 60 depois do boot) */
    unsigned int rx[TRAFEGO_AMOSTRAS], tx[TRAFEGO_AMOSTRAS];   /* bytes em cada segundo; a mais nova no fim */
} info_trafego_t;

typedef struct {
    unsigned int  ip;
    unsigned char mac[6];
    unsigned char completa, reservado;
    unsigned int  idade_s;
} info_arp_t;

typedef struct {
    unsigned long long base;
    unsigned long long tamanho;
    unsigned int       tipo;    /* 1 livre, 2 reservada, 3 ACPI, 4 ACPI NVS, 5 defeituosa */
    unsigned int       extra;   /* atributo estendido ACPI 3.0 (bit 0 = válida) */
} info_e820_t;

typedef struct {
    unsigned int  achou;          /* 0 = sem SMBIOS (o resto vale zero) */
    unsigned char versao_maior, versao_menor, entrada_64, reservado;
    unsigned int  endereco;       /* endereço físico da tabela */
    unsigned int  tamanho;        /* tamanho da tabela segundo a entrada */
    unsigned int  copiado;        /* bytes da tabela que vêm depois deste cabeçalho */
    unsigned int  estruturas;     /* quantidade anunciada (0 na entrada de 64 bits) */
} info_smbios_t;

typedef struct {
    unsigned char  barramento, dispositivo, funcao, revisao;
    unsigned char  classe, subclasse, interface, irq;
    unsigned short fabricante, modelo;
    unsigned short sub_fabricante, sub_modelo;
} info_pci_t;

typedef struct {
    unsigned int grafico;         /* 1 = o estágio 2 trocou para um modo VESA */
    unsigned int escritorio;      /* 1 = a área de trabalho está ativa (dá para abrir janelas) */
    unsigned int modo, largura, altura, bpp, pitch, framebuffer;
    unsigned char verm_tam, verm_pos, verde_tam, verde_pos, azul_tam, azul_pos, reservado[2];
    unsigned int vbe_versao;      /* 0x0300 = VBE 3.0; 0 = sem informação */
    unsigned int memoria_kb;      /* memória de vídeo anunciada pelo VBE */
    char         oem[48], fabricante[32], produto[32], revisao[16];
} info_video_t;

/* SIS_TELAS (D290, mejora 060): varias pantallas. Una "pantalla" es una salida de vídeo; la 0 es la del
 * arranque (la de INFO_TELA, SIS_MUDAR_MODO y el programa pantalla). */
#define TELAS_OP_INFO      0   /* (info_telas_t *, sizeof) -> número de pantallas */
#define TELAS_OP_CONFIG    1   /* (modo | lado << 8 | principal << 16) -> 0 o error; modo/lado: TELAS_MODO_*, TELAS_LADO_* */
#define TELAS_OP_RESOLUCAO 2   /* (pantalla, largura | altura << 16) -> 0 o error (la 0 usa SIS_MUDAR_MODO) */
#define TELAS_OP_GUARDAR   3   /* () -> 0 o error: /home/.config/pantallas.conf */
#define TELAS_MODO_EXTENDER 0
#define TELAS_MODO_ESPELHO  1
#define TELAS_LADO_DIREITA  0  /* las demás a la derecha de la principal */
#define TELAS_LADO_ABAIXO   1  /* las demás debajo de la principal */
#define TELAS_NUM_MAX       4
typedef struct {
    unsigned int qtd;              /* pantallas presentes */
    unsigned int modo, lado, principal;
    unsigned int desk_largura, desk_altura;   /* el escritorio entero */
    struct {
        int x, y;                  /* posición en el escritorio */
        unsigned int largura, altura, max_largura, max_altura;
        unsigned int a_quente;     /* 1 = cambia de resolución sin reiniciar */
        char nome[24];
    } t[TELAS_NUM_MAX];
} info_telas_t;

/* SIS_MODOS_VIDEO: um modo da lista (os do VBE do boot e, com a placa do
 * Bochs/QEMU, os que ela aceita a quente). */
typedef struct {
    unsigned short largura, altura;
    unsigned short modo;          /* número VBE (0 = só pela placa do Bochs/QEMU) */
    unsigned char  bpp;
    unsigned char  flags;         /* MODO_* */
} modo_video_t;
#define MODO_ATUAL     1          /* é o modo da tela agora */
#define MODO_A_QUENTE  2          /* dá para trocar sem reiniciar */
#define MODO_PREFERIDO 4          /* é a resolução gravada no disco de arranque */
#define MODO_VBE       8          /* a BIOS (VBE) oferece: vale para o próximo arranque */
/* SIS_MUDAR_MODO: opções e resultado */
#define MUDAR_AGORA    1          /* trocar já (só com troca a quente) */
#define MUDAR_GUARDAR  2          /* gravar no disco de arranque para os próximos (0x0 = automática) */

typedef struct {
    unsigned int largura, altura, bpp, pitch;
    unsigned int modo;            /* número VBE do modo ligado no boot (0 depois de uma troca a quente) */
    unsigned int a_quente;        /* 1 = placa do Bochs/QEMU/VirtualBox: troca sem reiniciar */
    unsigned int max_largura, max_altura;   /* limites da troca a quente */
    unsigned int pref_largura, pref_altura; /* preferência no disco de arranque (0 = automática) */
    unsigned int pref_gravavel;   /* 1 = achou o disco de arranque para gravar a preferência */
    unsigned int wc;              /* 1 = framebuffer em write-combining (D057) */
    char         wc_via[8];       /* "PAT" ou "MTRR" */
    char         wc_motivo[80];   /* por que não (vazio se ativo) */
    unsigned int memoria_kb;      /* memória de vídeo mapeada pelo kernel */
    unsigned int arranque_cd;     /* 1 = arrancou de um CD: a preferência não se grava (D077) */
    unsigned int wc_mbs;          /* D245: MB/s medidos ao escrever no framebuffer (0 = sem medida) */
    unsigned int fps_tela_cheia;  /* D245: quadros por segundo da tela cheia (0 = sem tela cheia) */
} info_tela_t;

/* INFO_ACPI (D076): só o que se lê sem intérprete de AML. */
#define ACPI_MAX_TABELAS 40
typedef struct {
    char           assinatura[5];  /* "FACP", "APIC"... e zero */
    char           oem[7];         /* OEM ID da tabela */
    unsigned int   endereco, tamanho;
    unsigned char  revisao, soma_ok, reservado[2];
} info_acpi_tabela_t;
typedef struct {
    unsigned int   achou;          /* 1 = RSDP encontrado */
    unsigned int   rsdp;           /* endereço físico do RSDP */
    unsigned int   revisao;        /* 0 = ACPI 1.0 (só RSDT); 2 = 2.0+ (XSDT) */
    char           oem[8];         /* OEM ID do RSDP */
    unsigned int   rsdt;
    unsigned int   xsdt_baixo, xsdt_alto;
    unsigned int   usou_xsdt;      /* a lista veio da XSDT (senão, da RSDT) */
    unsigned int   qtd;
    info_acpi_tabela_t tabela[ACPI_MAX_TABELAS];
} info_acpi_t;

/* INFO_ARRANQUE (D077). */
#define ARRANQUE_MAX_FASES 16
typedef struct {
    unsigned int   disco;          /* número do disco da BIOS (0x80 = primeiro disco ou o CD emulado) */
    unsigned int   cd;             /* 1 = arrancou de um CD (El Torito) */
    unsigned int   leitura_chs;    /* 1 = a BIOS não tinha as extensões do INT 13h (lido por CHS) */
    unsigned int   bpp_pref;       /* profundidade pedida no cabeçalho (0 = automática, D078) */
    unsigned int   res_pref;       /* largura | altura << 16 (D058) */
    unsigned int   opcoes;         /* CAB_OPCOES */
    /* D201: marcas de tiempo del arranque, en ms desde el encendido (TSC; la
     * primera incluye la BIOS y la lectura de la imagen por el INT 13h). */
    unsigned int   fases;
    unsigned int   fase_ms[ARRANQUE_MAX_FASES];
    char           fase_nome[ARRANQUE_MAX_FASES][16];
    unsigned int   firmware;       /* D270: 0 = BIOS, 1 = UEFI x64, 2 = UEFI IA32 (boot_info v2, D264) */
} info_arranque_t;

typedef struct {
    unsigned int       khz;       /* frequência do TSC; 0 = sem medida (CPU sem TSC) */
    unsigned int       janela_ms; /* duração da medida */
    unsigned long long ciclos;    /* ciclos do TSC contados na janela */
} info_tsc_t;
#define INFO_JANELAS  35   /* (info_janela_t[], n) -> quantas; de baixo para cima (D043) */
#define INFO_MONTAGENS 10  /* (info_montagem_t[n]) -> quantas montagens (D040) */

typedef struct {
    unsigned int ram_total_kb;
    unsigned int gerida_kb;      /* baja + alta (D348) */
    unsigned int livre_kb;
    unsigned int usada_kb;
    /* D348: memoria alta (por encima de 1,875 GB, solo para los programas). Un
     * programa anterior que pide solo los 16 bytes de arriba no los recibe. */
    unsigned int alta_gerida_kb;
    unsigned int alta_livre_kb;
    unsigned int janelas_usos;   /* veces que el kernel usó una ventana temporal (memalta) */
    unsigned int modo;           /* D349: MEM_MODO_* (era «reservado», a cero) */
    /* D349: de la alta, la que está por encima de 4 GB (solo con PAE). */
    unsigned int superior_gerida_kb;
    unsigned int superior_livre_kb;
    /* D350: la caché de disco (eran «reservado», a cero; mismo tamaño, así que los programas que
     * ya pedían sizeof(info_memoria_t) los reciben). Cuenta como usada en gerida/livre/usada (y
     * en alta_* lo que está en la alta), pero se devuelve a los programas cuando la piden. */
    unsigned int cache_kb;       /* bloques de la caché de disco */
    unsigned int cache_alta_kb;  /* de ellos, con los datos en la memoria alta o superior */
    /* D352: la swap comprimida en RAM (el kernel los rellena si el programa pide este tamaño).
     * El almacén cuenta como usado en gerida/livre/usada (y en alta_* lo que está en la alta). */
    unsigned int troca_teto_kb;       /* tope del almacén; 0 = sin swap (--sin-swap) */
    unsigned int troca_kb;            /* páginas de programas en la swap (lo que ocupaban sin comprimir) */
    unsigned int troca_comprimida_kb; /* lo que ocupan comprimidas (bloques de 64 bytes) */
    unsigned int troca_armazem_kb;    /* páginas del almacén (lo comprimido más el hueco entre trozos) */
    unsigned int troca_saidas;        /* páginas sacadas a la swap desde el arranque */
    unsigned int troca_entradas;      /* páginas traídas de vuelta */
    /* D378 (fase 3): páginas de solo lectura de las bibliotecas compartidas entre procesos (el
     * kernel las rellena si el programa pide este tamaño). Cuentan como usadas una sola vez. */
    unsigned int compartida_kb;       /* lo que ocupan (una vez) */
    unsigned int compartida_mapeada_kb; /* lo mapeado sumando todos los procesos */
    unsigned int compartida_objetos;  /* trozos de fichero cargados */
    unsigned int compartida_reutilizadas; /* veces que un proceso usó páginas que ya estaban */
} info_memoria_t;
#define INFO_MEMORIA_V1 16       /* bytes de la versión sin memoria alta */
#define INFO_MEMORIA_V2 32       /* bytes de la versión sin memoria superior (D348) */
#define INFO_MEMORIA_V3 48       /* bytes de la versión sin swap (D350) */
#define INFO_MEMORIA_V4 72       /* bytes de la versión sin páginas compartidas (D378, fase 3) */
#define MEM_MODO_PAE      1      /* paginación PAE (3 niveles, hasta 64 GB) */
#define MEM_MODO_NX       2      /* NX activo: pila y montón no ejecutables */
#define MEM_MODO_CPU_PAE  4      /* la CPU tiene PAE (aunque no se use: --sem-pae) */
#define MEM_MODO_CPU_NX   8      /* la CPU tiene NX */

typedef struct {
    int          pid;
    int          pai;
    unsigned int estado;     /* 1 pronto, 2 executando, 3 bloqueado, 4 zumbi */
    unsigned int memoria_kb;
    unsigned int cpu_ms;
    unsigned int usuario;    /* 1 = programa no anel 3 */
    char         nome[32];
    unsigned int janelas;    /* janelas abertas pelo processo (D043) */
    unsigned int cpu;        /* a CPU onde este processo corre (D157) */
    unsigned int n_hilos;    /* quantos fios tem o processo (D157) */
} info_processo_t;

/* Uma entrada por processador (D157). O Administrador de tarefas usa isto
 * para mostrar os núcleos e o uso de cada um. */
typedef struct {
    unsigned int indice;         /* número da CPU (0 = o principal) */
    unsigned int estado;         /* 0 livre, 1 pronta, 2 ociosa */
    unsigned int apic_id;        /* identificador do APIC local */
    unsigned int nucleo;         /* núcleo físico (para agrupar hyperthreads) */
    unsigned int pid;            /* o processo que esta CPU está a executar (0 = ocioso) */
    unsigned int tiques;         /* tiques totais desta CPU */
    unsigned int tiques_ociosos; /* tiques em que esta CPU não tinha processo nenhum */
    unsigned int fatia;          /* tiques que ainda faltam ao processo atual */
    unsigned int n_processos;    /* quantos processos há no sistema todo */
    unsigned int hilo;           /* D174: qué hilo de su núcleo físico es (0, 1...) */
    unsigned int paquete;        /* D174: el paquete (zócalo) */
    unsigned int planificador;   /* D174: el planificador activo (PLANIFICADOR_*) */
} info_nucleo_t;

/* D174: los planificadores (SIS_PLANIFICADOR). Los números son también los
 * de mkimagem.py --planificador (bits 9-11 de las opciones de arranque). */
#define PLANIFICADOR_RR         1   /* round-robin global: el de siempre (por defecto) */
#define PLANIFICADOR_AFINIDAD   2   /* cada proceso vuelve a su CPU; robo solo si la CPU está libre */
#define PLANIFICADOR_EQUILIBRIO 3   /* una cola por CPU y reparto periódico de la carga */
#define PLANIFICADOR_PRIORIDAD  4   /* prioridad (nice) y preferencia por los interactivos */
#define PLANIFICADOR_MAX        4

/* Operaciones de SIS_PLANIFICADOR */
#define PLAN_CONSULTAR   0   /* (0, buf, n) -> el activo (PLANIFICADOR_*); su nombre en buf si buf != 0 */
#define PLAN_ESCOLHER    1   /* (numero) -> el anterior; cambia en caliente */
#define PLAN_NOME        2   /* (numero, buf, n) -> 0; el nombre de ese planificador (ERRO_INVALIDO si no existe) */
#define PLAN_AFINIDADE   3   /* (pid o 0, mascara o -1 para consultar) -> la máscara anterior; 0 = todas las CPU */
#define PLAN_PRIORIDADE  4   /* (pid o 0, nice -20..19) -> 0; se consulta con PLAN_INFO (un nice negativo
                                 * se confundiría con un error) */
#define PLAN_INFO        5   /* (pid/tid o 0, &info_planificacion_t) -> 0 */
#define PLAN_AUDIO       6   /* D436: (1 encender, 0 apagar, -1 consultar) -> 0/1 antes; solo el hilo que llama:
                                 * «alta prioridad de audio». Al despertar pasa delante de los demás, pero solo
                                 * mientras gaste poco (PLAN_AUDIO_MAX_USO tics de cada ~250 ms): no hace falta ser
                                 * root, porque un hilo que acapara la CPU la pierde solo. Uno por proceso. */
#define PLAN_AUDIO_MAX_USO 5

/* Lo que el planificador sabe de un proceso o hilo (PLAN_INFO). */
typedef struct {
    int          pid;          /* el número de esta entrada (tid, si es un hilo) */
    unsigned int cpu;          /* la última CPU donde corrió */
    unsigned int mascara;      /* CPU permitidas (bit n = CPU n); 0 = todas */
    int          nice;         /* -20 (más prioridad) .. 19 */
    unsigned int migracoes;    /* veces que cambió de CPU */
    unsigned int execucoes;    /* veces que una CPU lo tomó */
    unsigned int recente;      /* uso reciente de CPU en tiques (el de "prioridad") */
    int          prioridade;   /* la prioridad efectiva ahora (menor = antes) */
    unsigned int lat_media_us; /* de despertar (listo) a correr: la media, en microsegundos */
    unsigned int lat_max_us;   /* y la peor */
    unsigned int despertares;  /* cuántas veces se midió */
    unsigned int lat_total_us; /* la suma (para medias de un tramo: diferencia de dos lecturas) */
} info_planificacion_t;

typedef struct {
    int          id;
    int          pid;        /* dono */
    int          x, y;       /* canto da moldura na tela */
    int          largura, altura;   /* área de conteúdo */
    unsigned int foco;       /* 1 = é a janela de cima */
    char         titulo[48];
} info_janela_t;

typedef struct {
    char         versao[32];
    unsigned int imagem_bytes;   /* tamanho útil da imagem de boot */
    unsigned int segundos_ligado;
    unsigned int largura, altura;
} info_versao_t;

typedef struct {
    char         nome[12];         /* "usb0", "usb0p1", "hda", "hda1" */
    char         fabricante[32];
    char         produto[48];
    char         formato[40];      /* "FAT32", "ext2"... ou "...: formato no soportado" */
    char         rotulo[20];
    unsigned int setores;          /* de 512 bytes */
    unsigned int montado;          /* ele (ou, no disco inteiro, alguma partição) está montado */
    unsigned int suportado;        /* tem um volume que dá para montar */
    /* D040 */
    char         pai[12];          /* "" = disco inteiro */
    char         tipo_fs[8];       /* "fat32", "exfat", "ext2", "ext3", "ext4" ou "" */
    char         ponto[64];        /* onde está montado ("" = não está) */
    unsigned int particao;         /* 0 = disco inteiro */
    unsigned int flags;            /* DISCO_* abaixo */
} info_disco_t;
#define DISCO_REMOVIVEL   1        /* pendrive USB */
#define DISCO_INTERNO     2        /* disco ATA */
#define DISCO_BOOT        4        /* o disco de onde o SaguiOS ligou */
#define DISCO_SO_LEITURA  8        /* montado só para leitura */
#define DISCO_ARRANQUE_REAL 16     /* D179: es el disco del que arrancó (cabecera del LBA 1), o podría
                                      serlo si eso no se sabe: los cambios piden la confirmación doble */
#define DISCO_TRIM        32       /* D179: admite TRIM y el driver lo manda */
#define DISCO_RESERVADO   64       /* D179: un programa lo tiene reservado (particiones, instalador) */
#define DISCO_CD          128      /* D207: lector de CD/DVD ("cd0") o la imagen El Torito de su disco ("cd0img"): solo lectura */
#define DISCO_XHCI        256      /* D214: pendrive conectado por el controlador xHCI */
#define DISCO_USB3        512      /* D214: pendrive USB 3.x (SuperSpeed) */
#define DISCO_NVME        1024     /* D265: disco NVMe */
#define DISCO_BLOCO4K     2048     /* D265: su bloque real es de 4096 bytes (se presenta en sectores de 512) */

typedef struct {
    char         dispositivo[12];
    char         ponto[64];
    char         tipo[8];          /* "fat32", "ext2"... */
    unsigned int opcoes;           /* MONTAR_SO_LEITURA se só leitura */
    unsigned int total_kb, livre_kb;
} info_montagem_t;

/* SIS_TECLAS_SEGURADAS: posição de algumas teclas no mapa (código de
 * varredura do conjunto 1; 128 + código para as teclas estendidas E0). */
#define CV_ESC        0x01
#define CV_CTRL       0x1D
#define CV_SHIFT      0x2A
#define CV_ALT        0x38
#define CV_ESPACO     0x39
#define CV_B          0x30
#define CV_CTRL_DIR   (0x80 | 0x1D)
#define CV_CIMA       (0x80 | 0x48)
#define CV_BAIXO      (0x80 | 0x50)
#define CV_ESQUERDA   (0x80 | 0x4B)
#define CV_DIREITA    (0x80 | 0x4D)

/* Modos do terminal */
#define TERM_LINHA 0
#define TERM_BRUTO 1

/* No modo bruto, teclas sem caractere chegam como ESC (27) + uma letra; a
 * própria tecla Esc chega como 27, 27. */
#define TECLA_B_CIMA     'A'
#define TECLA_B_BAIXO    'B'
#define TECLA_B_DIREITA  'C'
#define TECLA_B_ESQUERDA 'D'
#define TECLA_B_HOME     'H'
#define TECLA_B_END      'F'
#define TECLA_B_DELETE   'X'
#define TECLA_B_PGUP     'P'
#define TECLA_B_PGDN     'N'
#define TECLA_B_REDESENHAR 'R'   /* não é tecla: outro processo escreveu na tela */

/* Sequências que o terminal entende na saída: ESC [ D (cursor à esquerda),
 * ESC [ C (à direita), ESC [ K (apaga até o fim da linha), ESC [ J (limpa a tela). */

/* Eventos de janela */
#define EV_CLIQUE    1   /* botão apertado; "cliques" = 2 no duplo clique */
#define EV_SOLTOU    2
#define EV_MOVER     3   /* movimento sobre a janela */
#define EV_TECLA     4   /* "tecla": caractere Latin-1, ou 0x100+ para setas etc. */
#define EV_FECHAR    5   /* clicaram no X da moldura */
#define EV_REDIMENSIONAR 10  /* pedem outro tamanho: x = largura, y = altura, tecla = JAN_ESTADO_*.
                                O programa responde com SIS_REDIMENSIONAR_JANELA (D044). */
#define EV_RODA      11  /* roda do mouse (D048): "cliques" = entalhes, como int com sinal
                            (+ para baixo, - para cima); x, y = posição do cursor. Vai para a
                            janela debaixo do cursor, mesmo sem foco (como no Windows 10+).
                            Quem não trata, ignora. Ler com (int)e.cliques. */
#define EV_MENU      12  /* resposta de SIS_MENU_CONTEXTUAL (D051): x = índice da linha escolhida
                            (contando separadores e desativadas, a partir de 0) ou -1 se cancelado;
                            tecla = o "codigo" passado na chamada. */
#define MENU_MAX_ITENS 96   /* linhas num menu de contexto, somando as dos submenus (eram 20 antes de D064;
                               64 hasta D229: el menú Inicio con todas las apps ya pasaba de 64; como mucho 127, menu.c guarda el panel en int8_t) */
#define MENU_MAX_TEXTO 40   /* letras por linha (o resto é cortado) */
#define RODA_LINHAS  3   /* linhas roladas por entalhe nos componentes da libsagui */
#define EV_TELA      16  /* a resolução da tela mudou (D059): x = largura, y = altura. Vai para
                            todas as janelas; as maximizadas e em tela cheia recebem também
                            EV_REDIMENSIONAR. Quem não trata, ignora. */

#define EV_MOVER_CRU 17  /* D242: con el ratón capturado (JAN_CAPTURA): x, y = desplazamiento crudo
                            (con signo, y hacia abajo), botoes = botones apretados. El cursor no se mueve. */
#define EV_CAPTURA_SOLTA 18  /* D242: el sistema soltó la captura (Ctrl+Alt, perdió el foco, menú...) */
#define EV_WIDGET    19  /* D365: aviso a una ventana de escritorio (SIS_CRIAR_WIDGET); tecla = EVW_*:
                            EVW_MOVIDO (x, y = posición nueva en la pantalla), EVW_QUITAR (la persona eligió
                            «Quitar widget»: el programa se desactiva y termina), EVW_FIN_SESION (termina). */
#define EV_RODA_H    20  /* D407: rueda horizontal (desplazar con dos dedos o por el borde inferior del panel táctil):
                            "cliques" = muescas con signo (+ hacia la derecha), x, y y la ventana como EV_RODA.
                            Quien no la trata, la ignora (no se convierte en vertical). Ler com (int)e.cliques. */
#define EV_TEMA      21  /* D428: cambió el esquema de apariencia (Configuración > Estilos): los colores del sistema son otros.
                            La libsagui ya los recargó al entregar el evento (esq, mod_tema); el programa repinta. Quien no la
                            trata, la ignora y coge el esquema al abrirse. */
#define EVW_MOVIDO     1
#define EVW_QUITAR     2
#define EVW_FIN_SESION 3

/* SIS_PROXIMO_EVENTO: esperar até um instante (ms de SIS_TEMPO, 31 bits) e
 * devolver 0 se nada chegar (D083). */
#define EVENTO_PRAZO 0x80000000u

/* Estilos de janela (SIS_ESTILO_JANELA). Sem chamar, a janela tem tamanho fixo. */
#define JAN_REDIMENSIONAVEL 0x1   /* borda/canto, maximizar, duplo clique no título e F11 */
#define JAN_TELA_CHEIA_OK   0x2   /* aceita F11 mesmo com tamanho fixo (o kernel amplia) */
#define JAN_TELA_CHEIA      0x4   /* entrar em tela cheia agora (sem ele: sair) */
#define JAN_IMPRPANT        0x10  /* quiere recibir la tecla Impr Pant (D125) */
#define JAN_ESC_LIVRE       0x20  /* D204: en tela cheia el Esc llega al programa (sale solo con F11) */
#define JAN_DIALOGO         0x8   /* diálogo (D065): fica fora da barra de tarefas e não minimiza */
#define JAN_SEM_CURSOR      0x40  /* D230: con el foco, el puntero del sistema no se dibuja sobre el contenido
                                     (el programa dibuja el suyo); fuera de él, en menús y paneles, sí */
#define JAN_SO_CURSOR       0x80  /* D230: en SIS_ESTILO_JANELA, cambia solo JAN_SEM_CURSOR (lo demás,
                                     pantalla completa incluida, se queda como está) */
#define JAN_SO_CAPTURA      0x100 /* D242: en SIS_ESTILO_JANELA, cambia solo la captura del ratón:
                                     con JAN_CAPTURA la pide (solo la ventana con foco), sin él la suelta */
#define JAN_CAPTURA         0x200
/* Estado que acompanha EV_REDIMENSIONAR */
#define JAN_ESTADO_NORMAL      0
#define JAN_ESTADO_MAXIMIZADA  1
#define JAN_ESTADO_TELA_CHEIA  2

/* Teclas especiais nos eventos de janela. São os mesmos valores do enum do
 * kernel (teclado.h), que por isso não precisa destas definições. */
#ifndef SAGUI_KERNEL
#define TECLA_CIMA      0x100
#define TECLA_BAIXO     0x101
#define TECLA_ESQUERDA  0x102
#define TECLA_DIREITA   0x103
#define TECLA_HOME      0x104
#define TECLA_END       0x105
#define TECLA_PGUP      0x106
#define TECLA_PGDN      0x107
#define TECLA_INSERT    0x108
#define TECLA_DELETE    0x109
#define TECLA_F11       0x114   /* o gerenciador de janelas usa (tela cheia) */
#define TECLA_F12       0x115   /* Asistente IA en Pluma, Macaco y Correo (D298) */
#define TECLA_ESC       0x116
#define TECLA_SUPER     0x117   /* tecla Windows (E0 5B/5C): o gerenciador de janelas usa (menu Inicio) */
#endif

#define JANELA_LARG_MAX 1000
#define JANELA_ALT_MAX  700

typedef struct {
    unsigned int tipo;
    int          janela;
    int          x, y;        /* relativo à área de conteúdo da janela */
    unsigned int botoes;     /* 1 esquerdo, 2 direito */
    unsigned int cliques;
    unsigned int tecla;
    unsigned int modificadores;   /* 1 Shift, 2 Ctrl; nas teclas também 4 Alt (D075) */
} evento_t;

typedef struct { int x, y, largura, altura; } retangulo_t;

/* Código de saída de um processo encerrado pelo kernel (erro do programa). */
#define SAIDA_ENCERRADO_PELO_KERNEL -1000
#define SAIDA_INTERROMPIDO          130   /* encerrado com Ctrl+C (como 128 + SIGINT nos Unix) */

/* Monitor de hardware (D092, mejora 050). Faixa 54-55. Só leitura: o kernel
 * nunca escreve num MSR, no SMBus nem num Super I/O para isto; a única
 * escrita é o índice SMN dos Ryzen (salvo e restaurado, D092). A leitura
 * acontece só quando alguém pede, e no máximo uma vez por segundo (as
 * chamadas no meio recebem a mesma amostra, com idade_ms). */
#define INFO_SENSORES  54   /* (&info_sensores_t, sizeof) -> 0 */
/* SIS_FUNDO: FUNDO_LINHAS copia n linhas (largura da tela, 0x00RRGGBB) de ptr para a linha y da imagem
 * de fundo; FUNDO_FIM mostra o resultado; FUNDO_PADRAO volta ao degradê. Fora do desktop: ERRO_ACESSO. */
#define FUNDO_PADRAO 0
#define FUNDO_LINHAS 1
#define FUNDO_FIM    2
/* D380: el logo «IA Powered» abajo a la izquierda del escritorio (y = 1 lo muestra, 0 lo quita).
 * Lo pone `llm --anunciar` cuando la IA local puede funcionar (más de 1,8 GB y un modelo). */
#define FUNDO_MARCA_IA 3
#define ACENTO_PADRAO 0x0B7A3E   /* o verde do SaguiOS */

#define INFO_APARENCIA 58   /* (&info_aparencia_t, sizeof) -> 0 (D112) */
typedef struct {
    unsigned int acento;         /* 0xRRGGBB */
    unsigned int fundo_imagem;   /* 1 = ha imagem de fundo escolhida */
    unsigned int largura, altura;/* a tela (o tamanho que FUNDO_LINHAS espera) */
} info_aparencia_t;
/* D361: la barra de tareas y el menú Inicio personalizables. Un vector de bytes con una posición por ajuste;
 * INFO_BARRA lo lee (más lo que no se elige: si la barra está visible y su rectángulo) y INFO_BARRA_GRAVAR lo aplica
 * al momento (cualquier usuario: es preferencia de su sesión). Los valores fuera de rango se ajustan al más cercano. */
enum {
    BC_POSICION,     /* 0 arriba, 1 abajo */
    BC_ALTURA,       /* 20..48 píxeles (24 de serie; con iconos grandes sube sola hasta lo que hace falta) */
    BC_AUTOOCULTAR,  /* 0 no, 1 sí: la barra solo aparece al llevar el ratón a su borde */
    BC_RELOJ,        /* 0 oculto, 1 visible */
    BC_FECHA,        /* 0 automática (según el ancho de la pantalla), 1 con fecha, 2 solo la hora */
    BC_SEGUNDOS,     /* 0 no, 1 sí: la hora con segundos */
    BC_BAND_VOL, BC_BAND_RED, BC_BAND_TEMP, BC_BAND_BAT,   /* 0 oculto, 1 visible (la temperatura y la batería solo si existen) */
    BC_AGRUPAR,      /* 0 un botón por ventana, 1 un botón por programa (con el número de ventanas) */
    BC_ROTULOS,      /* botones de las tareas: 0 icono y texto, 1 solo icono, 2 solo texto */
    BC_ICONOS,       /* 0 iconos de 16 píxeles, 1 de 32 */
    BC_INICIO_TXT,   /* botón Inicio: 0 logo y texto, 1 solo el logo, 2 solo el texto */
    BC_TEMA,         /* color de la barra: 0 oscuro, 1 claro, 2 del acento, 3 negro, 4 azul noche */
    BC_MENU,         /* menú Inicio: 0 SaguiOS, 1 Clásico, 2 Dos columnas, 3 Moderno */
    BC_BUSCADOR,     /* Moderno: 0 buscador arriba, 1 abajo */
    BC_TRANSLUCIDO,  /* Moderno: 1 translúcido, 0 degradado oscuro */
    BC_N
};
#define BC_POS_ARRIBA 0
#define BC_POS_ABAJO  1
#define BC_MENU_SAGUI 0
#define BC_MENU_CLASICO 1
#define BC_MENU_DOS_COL 2
#define BC_MENU_MODERNO 3
#define INFO_BARRA        81   /* (&info_barra_t, sizeof) -> 0 (D361) */
#define INFO_BARRA_GRAVAR 82   /* (&info_barra_t, sizeof) -> 0: aplica v[] (D361); versao tiene que ser 1 */
typedef struct {
    unsigned int versao;           /* 1 */
    unsigned char v[BC_N];         /* los ajustes, por BC_* */
    unsigned char pad[32 - BC_N];
    int          visivel;          /* solo lectura: 1 si la barra está en la pantalla ahora */
    int          x, y, l, a;       /* solo lectura: el rectángulo de la barra */
    int          menu_aberto;      /* solo lectura: el menú Inicio está abierto */
    int          menu_x, menu_y, menu_l, menu_a;   /* solo lectura: su rectángulo (0 si no hay) */
} info_barra_t;
/* D407: el panel táctil (touchpad) PS/2. INFO_PAINEL dice qué se reconoció (Synaptics, ALPS, Elantech o nada: ratón
 * PS/2 de siempre), sus contadores y las opciones; INFO_PAINEL_GRAVAR aplica las opciones (cualquier usuario: es
 * preferencia de su sesión, como el teclado). Los números 96 y 97 dejan sitio a los INFO_* de otras ramas de la v8.5.5. */
#define INFO_PAINEL        96   /* (&info_painel_t, sizeof) -> 0 */
#define INFO_PAINEL_GRAVAR 97   /* (&info_painel_t, sizeof) -> 0: aplica las opciones; versao tiene que ser 1 */
#define PAINEL_TIPO_NENHUM    0
#define PAINEL_TIPO_SYNAPTICS 1
#define PAINEL_TIPO_ALPS      2
#define PAINEL_TIPO_ELANTECH  3
typedef struct {
    unsigned int  versao;          /* 1 */
    /* las opciones (se leen y se escriben) */
    unsigned char toque;           /* tocar = clic (dos dedos = derecho, tres = central) */
    unsigned char dois_dedos;      /* desplazamiento con dos dedos */
    unsigned char borda;           /* desplazamiento por el borde derecho e inferior */
    unsigned char natural;         /* sentido natural */
    unsigned char sensib;          /* 1-10 (5 = normal) */
    unsigned char sem_usb;         /* no usar el panel mientras haya un ratón USB */
    unsigned char modo_ps2;        /* 1 = no usar el modo absoluto: el panel como ratón PS/2 de siempre */
    unsigned char pad0;
    unsigned int  ms_teclado;      /* no hacer caso del panel estos ms tras una tecla (0 = nunca; hasta 2000) */
    /* solo lectura */
    unsigned int  tipo;            /* PAINEL_TIPO_* en uso (NENHUM = ratón PS/2) */
    unsigned int  reconhecido;     /* PAINEL_TIPO_* reconocido al arrancar aunque se use como ratón PS/2 */
    unsigned int  versao_hw;       /* Synaptics: mayor; ALPS: 2; Elantech: 2, 3 o 4 */
    unsigned int  firmware;        /* Synaptics: mayor << 8 | menor; Elantech: los 3 bytes del firmware */
    unsigned int  capacidades;     /* Synaptics: capacidades; ALPS: firma E7 */
    int           xmin, xmax, ymin, ymax;
    unsigned int  multidedo, dois_dedos_pos, palanca;   /* cuenta dedos; da la posición del segundo; tiene palanca */
    unsigned int  pacotes, invalidos, toques, rolagens, palmas, por_teclado;   /* contadores */
    unsigned int  ratos_usb;       /* ratones USB enchufados ahora */
    unsigned int  desligado;       /* el panel no hace caso ahora (ratón USB, o vuelta al PS/2 por paquetes malos) */
    char          nome[40];        /* "Synaptics 8.1", "ALPS v2 (73 02 50)", "Elantech v4 (fw 381f17)"... */
} info_painel_t;
#define INFO_MSR_PROVA 55   /* (0, 0) -> bit 0: MSR inexistente deu "não disponível"; bit 1: idem o x2APIC ID (0x802) */

#define INFO_BANDEJA   60   /* (&info_bandeja_t, sizeof) -> 0: o que a bandeja da barra mostra agora (D114) */
typedef struct {
    unsigned int versao;          /* 1 */
    unsigned int som;             /* AUDIO_SEM_PLACA, AUDIO_FUNCIONANDO, AUDIO_SEM_DRIVER */
    unsigned int volume, silencio;
    unsigned int rede;            /* 0 sem placa, 1 sem conexão, 2 conectado */
    unsigned int ip;              /* eth0, como em info_rede_t (primeiro octeto nos bits altos) */
    unsigned int temp_ok;         /* 0 = a CPU não informa temperatura (ícone oculto) */
    int          temp_mili_c;
    unsigned int limitada;        /* 1 = marcada como limitada por temperatura */
    unsigned int painel;          /* 0 nenhum, 1 volume, 2 rede, 3 temperatura, 4 batería (D192) */
    unsigned int icones;          /* ícones visíveis */
    /* versión 2 (D192): la batería (bateria.c); sin batería el icono no aparece */
    unsigned int bateria;         /* 1 = hay batería puesta (icono visible) */
    int          bateria_pct;     /* 0-100; -1 = desconocido o sin batería */
    unsigned int bateria_estado;  /* BATERIA_* (cargando, descargando) */
} info_bandeja_t;

#define SENS_FONTE_NENHUMA 0
#define SENS_FONTE_INTEL   1   /* MSR IA32_THERM_STATUS (0x19C) e IA32_PACKAGE_THERM_STATUS (0x1B1) */
#define SENS_FONTE_AMD_K8  2   /* PCI 00:18.3, registro 0xE4 */
#define SENS_FONTE_AMD_K10 3   /* PCI 00:18.3, registro 0xA4 (famílias 10h-16h) */
#define SENS_FONTE_AMD_ZEN 4   /* SMN 0x59800 pelo índice/dado de 00:00.0 (17h/19h; 15h modelos 60h-7Fh: 0xD8200CA4) */
/* info_sensores_t.throttling */
#define SENS_THR_TERMICO   0x01   /* reduzindo agora por temperatura */
#define SENS_THR_REGISTRO  0x02   /* já reduziu desde o arranque (bit de registro) */
#define SENS_THR_PROCHOT   0x04   /* PROCHOT#/FORCEPR# (aviso externo) */
#define SENS_THR_ENERGIA   0x08   /* limite de potência */
#define SENS_THR_SABE      0x80   /* a fonte informa throttling (sem este bit: desconhecido) */
/* info_sensores_t.turbo */
#define SENS_TURBO_ND      0
#define SENS_TURBO_NAO     1   /* a CPU não tem */
#define SENS_TURBO_DESLIG  2   /* tem, mas a BIOS desligou */
#define SENS_TURBO_LIGADO  3
#define SENS_MAX_TEMP      4
#define SENS_MAX_PSTATES   8

typedef struct {
    char         nome[12];        /* "Núcleo 0", "Paquete", "Tctl", "Tdie" */
    int          mili_c;          /* milésimos de grau Celsius */
} info_temp_t;

typedef struct {
    unsigned int versao;          /* 1 */
    unsigned int hipervisor;      /* 1 = o CPUID diz que é uma máquina virtual */
    unsigned int fonte;           /* SENS_FONTE_* */
    char         fonte_txt[56];   /* de onde veio a temperatura */
    char         motivo[88];      /* sem temperatura: por quê */
    unsigned int qtd_temp;
    info_temp_t  temp[SENS_MAX_TEMP];
    int          max_mili_c;      /* TjMax ou limite (0 = desconhecido) */
    char         max_origem[40];  /* "MSR 0x1A2", "tabla por modelo"... */
    unsigned int throttling;      /* SENS_THR_* */
    unsigned int tsc_khz;         /* frequência nominal (TSC medido no arranque) */
    unsigned int real_khz;        /* frequência efetiva por APERF/MPERF (0 = n/d) */
    unsigned int real_janela_ms;  /* intervalo dessa medida */
    unsigned int base_khz, max_khz;   /* CPUID 0x16 (0 = n/d) */
    unsigned int mult_x10;        /* multiplicador atual x 10 (0 = n/d) */
    unsigned int mult_max_x10;    /* máximo sem turbo x 10 */
    unsigned int mult_turbo_x10;  /* turbo com 1 núcleo x 10 */
    int          pstate;          /* AMD: P-state atual (-1 = n/d) */
    unsigned int qtd_pstates;
    unsigned int pstate_khz[SENS_MAX_PSTATES];
    unsigned int vid_mv;          /* tensão pedida (VID), aproximada (0 = n/d) */
    unsigned int turbo;           /* SENS_TURBO_* */
    unsigned int microcodigo;     /* revisão (0 = n/d) */
    unsigned int msr_falhas;      /* #GP que o leitor protegido absorveu desde o arranque */
    unsigned int idade_ms;        /* idade desta amostra */
    unsigned int reservado[8];    /* placa (Super I/O, SMBus): pendente, desligado por padrão */
} info_sensores_t;

/* Control de la frecuencia de la CPU (D106, mejora 061). Driver aparte del
 * monitor: solo elige entre los P-states que la BIOS ya declaró (AMD 10h en
 * adelante: PStateCtl, MSR 0xC0010062, respetando PStateCurLim; Intel con
 * EIST desde Nehalem: el multiplicador de IA32_PERF_CTL). Nunca toca
 * tensiones (VID/FID). Límite de temperatura de 79 °C, que se puede bajar
 * pero no subir: al llegar, el estado más lento hasta bajar de límite - 7. */
#define FREQ_EST_NAO_DISPONIVEL 0   /* CPU sin mecanismo conocido, máquina virtual... (motivo) */
#define FREQ_EST_ATIVO          1
#define FREQ_EST_DESATIVADO     2   /* por la opción de arranque o "frecuencia --desactivar" */
#define FREQ_EST_FALHOU         3   /* una escritura dio #GP: desactivado hasta reiniciar */

#define FREQ_MEC_NENHUM   0
#define FREQ_MEC_AMD      1   /* PStateCtl (0xC0010062) */
#define FREQ_MEC_INTEL    2   /* IA32_PERF_CTL (0x199), multiplicador */
#define FREQ_MEC_SIMULADO 3   /* imagen de prueba (mkimagem.py --frecuencia-simulada): nada de hardware */

#define FREQ_POL_RENDIMENTO 0   /* por defecto: el estado más rápido */
#define FREQ_POL_ECONOMIA   1   /* el más lento */
#define FREQ_POL_DEMANDA    2   /* sube con carga y baja en reposo */

/* Operaciones de SIS_FREQUENCIA */
#define FREQ_OP_POLITICA  1   /* (FREQ_POL_*) */
#define FREQ_OP_LIMITE    2   /* (milésimas de °C): de 40000 a 79000; más de 79000 -> ERRO_INVALIDO */
#define FREQ_OP_DESATIVAR 3   /* deja la CPU en el estado de la BIOS y no escribe más */
#define FREQ_OP_ATIVAR    4   /* vuelve a activarlo (no después de un #GP) */
#define FREQ_OP_FORCAR    5   /* (0/1): subir aunque no se pueda leer la temperatura */
#define FREQ_OP_SIM_TEMP  6   /* solo simulado: (milésimas de °C, o FREQ_TEMP_ND) */
#define FREQ_OP_SIM_USO   7   /* solo simulado: uso de CPU en % para «a demanda» (-1 = el real) */
#define FREQ_OP_SIM_GP    8   /* solo simulado: (CPU) la próxima escritura de esa CPU va a un MSR inexistente (#GP) */
#define FREQ_OP_COORDENACAO 9 /* (FREQ_COORD_*): un estado por núcleo físico o el mismo en todo el paquete (D175) */
#define FREQ_OP_SIM_USO_CPU 10 /* solo simulado: (CPU << 16 | uso en %, 0xFFFF = el real) (D175) */

#define FREQ_COORD_NUCLEO  0   /* de serie: cada núcleo físico según su carga */
#define FREQ_COORD_PAQUETE 1   /* todas las CPU el mismo estado (el de la más cargada) */
#define FREQ_TEMP_ND      ((int)0x80000000)

#define FREQ_MAX_ESTADOS  8

typedef struct {
    unsigned int versao;           /* 1 */
    unsigned int estado;           /* FREQ_EST_* */
    unsigned int mecanismo;        /* FREQ_MEC_* */
    char         mecanismo_txt[48];
    char         motivo[88];       /* sin control: por qué */
    unsigned int politica;         /* FREQ_POL_* */
    unsigned int qtd_estados;
    unsigned int estado_khz[FREQ_MAX_ESTADOS];   /* 0 = el más rápido (0 kHz = desconocida) */
    char         estado_nome[FREQ_MAX_ESTADOS][8];   /* "P0", "x22"... */
    int          atual;            /* estado que dice la CPU ahora (-1 = n/d) */
    int          pedido;           /* último estado escrito (-1 = ninguno) */
    int          bios;             /* el que dejó la BIOS */
    int          rapido;           /* el más rápido que la plataforma permite (PStateCurLim) */
    int          limite_mc;        /* límite de temperatura (79000 de serie, nunca más) */
    int          retorno_mc;       /* vuelve a subir por debajo de esto */
    int          temp_mc;          /* última lectura (FREQ_TEMP_ND = sin lectura) */
    unsigned int temp_simulada;    /* 1 = la temperatura es la inyectada (simulado) */
    unsigned int limitada;         /* 1 = limitada por temperatura ahora */
    unsigned int vezes_limitada;   /* veces desde el arranque */
    unsigned int forcado;          /* 1 = sube sin lectura de temperatura */
    unsigned int uso_cpu;          /* % del último periodo (lo que ve «a demanda») */
    unsigned int periodo_ms;       /* cada cuánto decide */
    unsigned int escritas;         /* escrituras en la CPU desde el arranque */
    unsigned int origem_desativado; /* 1 = opción de arranque, 2 = /home/.frecuencia o /etc/frecuencia, 3 = comando */
    unsigned int cpus;             /* versión 2 (D175): CPU con el driver (las arrancadas; detalle en INFO_FREQ_NUCLEOS) */
    unsigned int coordenacao;      /* versión 2: FREQ_COORD_* */
    unsigned int reservado[6];
} info_frequencia_t;

/* INFO_FREQ_NUCLEOS (D175). Cada CPU escribe su propio registro de P-state;
 * «atual» es lo que dice el registro de estado de ESA CPU, que puede ser más
 * rápido que lo pedido si el hardware comparte el estado con otra (SMT,
 * módulo o paquete: manda el más rápido). */
typedef struct {
    unsigned int cpu;        /* número de la CPU (0 = la principal) */
    unsigned int ligada;     /* 1 = arrancada (con --smp); 0 = apagada: el resto no vale */
    unsigned int nucleo;     /* núcleo físico (dominio del P-state por núcleo) */
    int          atual;      /* estado según el registro de estado de esa CPU (-1 = aún sin leer) */
    int          pedido;     /* último estado escrito por esa CPU (-1 = ninguno) */
    int          alvo;       /* lo que la política decidió para ella (-1 = nada aún) */
    int          bios;       /* el que dejó la BIOS en esa CPU (-1 = aún sin leer) */
    unsigned int uso;        /* % del último periodo */
    unsigned int khz;        /* frecuencia del estado actual (0 = n/d) */
    unsigned int escritas;   /* escrituras de esa CPU desde el arranque */
    unsigned int pendente;   /* 1 = aún no aplicó lo último que se le pidió (lo hace en su tic) */
    unsigned int reservado[3];
} info_freq_nucleo_t;
/* INFO_ENERGIA (D113): apagado por ACPI e botão de energia. */
#define INFO_ENERGIA   59   /* (&info_energia_t, sizeof) -> 0 */
#define ENERGIA_SEM_ACPI     0   /* não há RSDP/FADT */
#define ENERGIA_TABELAS_MAS  1   /* FADT ou DSDT com soma errada, ou registros PM1 ausentes */
#define ENERGIA_SEM_S5       2   /* não achei um \_S5_ que se leia sem intérprete de AML */
#define ENERGIA_DISPONIVEL   3
#define ENERGIA_DESATIVADO   4   /* opção de arranque (mkimagem.py --sem-acpi) ou comando "acpi desactivar" */
typedef struct {
    unsigned int   estado;          /* ENERGIA_* */
    unsigned char  slp_typ_a, slp_typ_b;   /* do pacote \_S5_ */
    unsigned char  botao;           /* 1 = botão de energia fixo atendido pelo SCI */
    unsigned char  modo_acpi;       /* 0 = não ativado, 1 = já estava (SCI_EN), 2 = ativado pelo SaguiOS */
    char           origem_s5[5];    /* "DSDT" ou "SSDT" */
    unsigned char  reduzido;        /* FADT HW_REDUCED_ACPI */
    unsigned char  botao_metodo;    /* FADT PWR_BUTTON: botão como dispositivo AML (não atendido) */
    unsigned char  reservado;
    unsigned int   sci;             /* IRQ do SCI (FADT) */
    unsigned int   smi_cmd, acpi_enable;
    unsigned int   pm1a_evt, pm1b_evt, pm1a_cnt, pm1b_cnt;   /* portas de E/S (0 = não há) */
    unsigned int   pm1_evt_len;
    unsigned int   pulsacoes;       /* vezes que o botão foi apertado */
    unsigned int   sci_total;       /* SCIs atendidos */
    unsigned int   fadt_revisao;
    char           motivo[64];      /* por que não está disponível */
} info_energia_t;

/* INFO_BATERIA (D192): la batería del portátil, leída con el intérprete de AML
 * (uACPI) de los dispositivos ACPI PNP0C0A (batería) y ACPI0003 (cargador).
 * Con varias baterías, las capacidades y el consumo son la suma; los textos,
 * de la primera. Lo que no se sabe vale BATERIA_DESCONHECIDO (o -1). */
#define INFO_BATERIA   64   /* (&info_bateria_t, sizeof) -> 0 */
#define UACPI_DESLIGADO   0   /* imagen sin --uacpi (por defecto), con --sem-acpi o sin modo ACPI */
#define UACPI_INICIANDO   1   /* cargando el espacio de nombres (unos segundos tras el arranque) */
#define UACPI_ATIVO       2
#define UACPI_FALHOU      3   /* uACPI no pudo cargar: el sistema sigue sin él (motivo) */
#define BATERIA_DESCARREGANDO 1   /* bits de _BST */
#define BATERIA_CARREGANDO    2
#define BATERIA_CRITICA       4
#define BATERIA_DESCONHECIDO  0xFFFFFFFFu
typedef struct {
    unsigned int versao;          /* 1 */
    unsigned int uacpi;           /* UACPI_* */
    unsigned int baterias;        /* dispositivos PNP0C0A que hay (puestas o no) */
    unsigned int presente;        /* 1 = hay al menos una batería puesta (_STA bit 4) */
    unsigned int adaptador;       /* 0 = no hay ACPI0003; 1 = desenchufado; 2 = enchufado */
    unsigned int estado;          /* BATERIA_* de _BST (0 = ni carga ni descarga: llena y enchufada) */
    int          porcentagem;     /* 0-100; -1 = desconocido */
    int          minutos;         /* restantes (descargando) o hasta llenarse (cargando); -1 = desconocido */
    unsigned int unidade_ma;      /* 0 = mWh y mW; 1 = mAh y mA (_BIF/_BIX "Power Unit") */
    unsigned int projeto;         /* capacidad de diseño */
    unsigned int total;           /* capacidad de la última carga completa */
    unsigned int restante;        /* capacidad que queda (_BST) */
    unsigned int taxa;            /* consumo o corriente de carga (_BST) */
    unsigned int tensao_mv;       /* tensión actual (_BST) */
    unsigned int projeto_mv;      /* tensión de diseño */
    unsigned int aviso, baixo;    /* niveles de aviso y de batería baja de la BIOS */
    unsigned int ciclos;          /* ciclos de carga (_BIX; BATERIA_DESCONHECIDO con _BIF) */
    unsigned int leituras;        /* veces que se ha leído _BST */
    unsigned int notificacoes;    /* avisos Notify del firmware (0x80/0x81) */
    unsigned int idade_ms;        /* tiempo desde la última lectura */
    unsigned int ec;              /* 1 = hay controlador embebido (PNP0C09) atendido */
    unsigned int consultas_ec;    /* eventos del EC atendidos (_Qxx) */
    char         modelo[24], serie[16], tipo[12], fabricante[24];
    char         motivo[64];      /* por qué uACPI no está activo o la batería no se lee */
} info_bateria_t;

/* INFO_RADIOS (D405): las radios (Wi-Fi, Bluetooth, WWAN) y el panel de teclas de los
 * portátiles HP, por el puente WMI de ACPI (PNP0C14) con uACPI. Además, los bloques que
 * se encontraron en los _WDG (de cualquier fabricante): sirven para identificar
 * máquinas nuevas.
 * INFO_RADIOS_MUDAR: (radio | ligar << 8, 0) -> 0 o ERRO_*: enciende o apaga una radio
 * (RADIO_*); espera hasta 5 s a la tarea de ACPI (ERRO_OCUPADO si no ha terminado aún).
 * INFO_ACPI_TABELA: (&info_acpi_tabela_dados_t, sizeof) -> bytes copiados: un trozo de
 * una tabla ACPI de la lista de INFO_ACPI (indice, deslocamento), tal cual está en la
 * memoria (solo lectura: para volcarlas y estudiar el AML de una máquina). */
#define INFO_RADIOS        90
#define INFO_RADIOS_MUDAR  91
#define INFO_ACPI_TABELA   92
#define RADIO_WIFI   0
#define RADIO_BT     1
#define RADIO_WWAN   2
#define RADIOS_MAX   3
#define RADIOS_WMI_MAX 16
typedef struct {
    char          guid[37];
    char          id[3];          /* ObjectId (WMxx/WQxx); "" en un evento */
    unsigned char flags;          /* 1 caro, 2 método, 4 texto, 8 evento */
    unsigned char instancias;
    unsigned char notificacao;    /* NotifyId de un evento */
} info_wmi_bloco_t;
typedef struct {
    unsigned int  versao;         /* 1 */
    unsigned int  uacpi;          /* UACPI_* */
    unsigned int  hp;             /* 1 = hay WMI de HP (GUID 5FB7F034-...) */
    unsigned int  lido;           /* 1 = el estado de las radios se leyó bien */
    unsigned int  codigo;         /* último código de la BIOS (0 = bien) */
    unsigned int  presentes;      /* bit RADIO_*: la radio existe */
    unsigned int  sw;             /* bit RADIO_*: encendida por software */
    unsigned int  hw;             /* bit RADIO_*: no bloqueada por el botón o el interruptor */
    unsigned int  eventos;        /* eventos WMI atendidos */
    unsigned int  teclas;         /* teclas del panel */
    unsigned int  ultimo_evento, ultimo_dado, ultima_tecla;
    unsigned int  pci_novos;      /* funciones PCI que han aparecido desde el arranque */
    unsigned int  reiniciar;      /* 1 = la tarjeta se encendió pero hay que reiniciar para usarla */
    unsigned int  n_wmi;          /* bloques WMI (puede haber más que RADIOS_WMI_MAX) */
    info_wmi_bloco_t wmi[RADIOS_WMI_MAX];
    char          nota[96];       /* lo último que pasó o por qué no hay radios */
} info_radios_t;
#define ACPI_TABELA_TROZO 4096
typedef struct {
    unsigned int  indice;         /* en la lista de INFO_ACPI */
    unsigned int  deslocamento;   /* desde el principio de la tabla */
    unsigned int  tamanho;        /* (salida) de la tabla entera */
    char          assinatura[5];  /* (salida) */
    unsigned char reservado[3];
    unsigned char dados[ACPI_TABELA_TROZO];
} info_acpi_tabela_dados_t;

/* SIS_PORTATIL (D403): lo propio de los portátiles, leído y manejado con el intérprete de AML
 * (uACPI, solo con --uacpi) desde la tarea "acpi-aml":
 *   - el brillo de la pantalla: ACPI Video (salida con _BCL/_BCM/_BQC de un dispositivo de vídeo
 *     con _DOD/_DOS), si no Fujitsu (FUJ02B1: GBLL/SBLL/RBLL), si no Toshiba (HCI 0x002A);
 *   - las teclas de función que avisa el firmware: Notify 0x86/0x87 (brillo) y 0x80-0x84 (pantalla)
 *     del ACPI Video, FUJ02E3 (FUNC 0x1002, anillo de botones) y TOS1900/TOS620x (INFO o HCI 0x0016);
 *   - las acciones comunes (ACAO_*): volumen, silencio, brillo, radio, que llegan también por las
 *     teclas multimedia del teclado (E0 20, E0 2E, E0 30) y salen con un aviso en pantalla. */
#define PORTATIL_ESTADO     0   /* (&info_portatil_t, sizeof) -> 0 */
#define PORTATIL_BRILHO     1   /* (PORTATIL_BRILHO_*, valor) -> brillo en % tras el cambio; ERRO_NAO_EXISTE sin control */
#define PORTATIL_NOTIFICAR  2   /* (PORTATIL_ALVO_*, valor) -> 0; solo root: como un Notify del firmware (pruebas) */
#define PORTATIL_TABELA     3   /* (índice de INFO_ACPI, buf, n) -> bytes de la tabla (copia hasta n); solo root */
#define PORTATIL_ACAO       4   /* (ACAO_*) -> 0: como la tecla (con su aviso) */
#define PORTATIL_BRILHO_LER    0
#define PORTATIL_BRILHO_FIXAR  1   /* valor = 0-100 % (se elige el nivel más cercano) */
#define PORTATIL_BRILHO_PASSO  2   /* valor = pasos, con signo (+1 sube un nivel, -1 baja) */
#define PORTATIL_ALVO_VIDEO    1   /* la salida con brillo (0x86 sube, 0x87 baja...) */
#define PORTATIL_ALVO_BUS      2   /* el dispositivo de vídeo (0x80: cambio de pantalla) */
#define PORTATIL_ALVO_FUJ_BL   3   /* FUJ02B1 (0x80: el brillo cambió) */
#define PORTATIL_ALVO_FUJ_EXT  4   /* FUJ02E3 (0x80: hay botones en el anillo) */
#define PORTATIL_ALVO_TOSHIBA  5   /* TOS1900/TOS620x (0x80: hay una tecla) */
#define PORTATIL_FONTE_NADA    0
#define PORTATIL_FONTE_VIDEO   1
#define PORTATIL_FONTE_FUJITSU 2
#define PORTATIL_FONTE_TOSHIBA 3
/* Acciones del sistema (el punto de entrada común, kernel/nucleo/acoes.c). */
#define ACAO_VOLUME_MAIS   1
#define ACAO_VOLUME_MENOS  2
#define ACAO_SILENCIO      3   /* alterna */
#define ACAO_BRILHO_MAIS   4
#define ACAO_BRILHO_MENOS  5
#define ACAO_RADIO         6   /* alterna el modo avión */
#define ACAO_TELA          7   /* la tecla de cambio de pantalla (solo el aviso) */
#define ACAO_MAX           8
/* De dónde vino (para el registro y info_portatil_t.ultima_acao). */
#define ORIGEM_ACAO_TECLADO  1
#define ORIGEM_ACAO_VIDEO    2
#define ORIGEM_ACAO_FUJITSU  3
#define ORIGEM_ACAO_TOSHIBA  4
#define ORIGEM_ACAO_ORDEN    5
#define ORIGEM_ACAO_HP       6   /* HP WMI: el panel QuickTouch (D405) */
typedef struct {
    unsigned int versao;            /* 1 */
    unsigned int uacpi;             /* UACPI_* */
    unsigned int brilho_fonte;      /* PORTATIL_FONTE_* */
    int          brilho_pct;        /* 0-100; -1 = desconocido o sin control */
    unsigned int brilho_nivel;      /* índice del nivel actual (0 = el más bajo) */
    unsigned int brilho_niveis;     /* cuántos niveles hay */
    unsigned int brilho_valor;      /* valor del nivel tal como lo da el firmware (_BCL, GBLL, HCI) */
    unsigned int brilho_min, brilho_max;   /* valores del primer y del último nivel */
    unsigned int bqc_indice;        /* 1 = el _BQC de esta BIOS da el índice y no el valor */
    unsigned int video_buses;       /* dispositivos con _DOD o _DOS */
    unsigned int video_saidas;      /* salidas con _BCL y _BCM */
    unsigned int dos_chamado;       /* 1 = se llamó a _DOS(0) */
    unsigned int fujitsu;           /* bit 0: FUJ02B1 (brillo); bit 1: FUJ02E3 (teclas) */
    unsigned int fujitsu_funcoes;   /* FUNC(0x1000, 0): lo que declara el FUJ02E3 */
    unsigned int toshiba;           /* 1 = hay TOS1900/TOS620x con GHCI o SPFC */
    unsigned int toshiba_teclas;    /* 0 no; 1 por INFO; 2 por HCI_SYSTEM_EVENT */
    int          toshiba_ventilador;   /* -1 = no lo expone; si no, el valor de HCI_FAN */
    int          toshiba_radio;     /* -1 = no se sabe; 1 = el interruptor de la radio está encendido */
    unsigned int radio_apagada;     /* 1 = modo avión (ACAO_RADIO) */
    unsigned int eventos;           /* avisos del firmware atendidos */
    unsigned int ultimo_evento;     /* PORTATIL_ALVO_* << 16 | valor (Notify o código de tecla) */
    unsigned int acoes;             /* acciones hechas */
    unsigned int ultima_acao;       /* ACAO_* << 8 | ORIGEM_ACAO_* */
    char         video_lcd[40];     /* ruta de la salida con brillo */
    char         fujitsu_no[40];
    char         toshiba_no[40];
    char         motivo[64];        /* por qué no hay control del brillo */
} info_portatil_t;

#endif
