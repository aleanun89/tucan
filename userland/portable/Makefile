# Makefile - las aplicaciones de SaguiOS en Linux con SDL 2 (D459, docs/apps-portables.md).
#
#   make            compila mico, pluma, macaco, tucan, paint, planomini y gebramini en build/portable/
#   make mico       solo una
#   make probar     ejecuta las pruebas de humo (SDL_VIDEODRIVER=dummy)
#
# Se compila desde la raiz del arbol de SaguiOS (o de un repo portable, que tiene la misma disposicion
# con solo lo necesario). Las fuentes de las aplicaciones y de la libsagui son las mismas que las de SaguiOS:
# solo cambia la capa de llamadas al sistema (userland/portable/).
RAIZ   ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)
B      ?= $(RAIZ)/build/portable
CC     ?= gcc
SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL_LIBS   := $(shell sdl2-config --libs 2>/dev/null)
CFLAGS ?= -O2
CFLAGS += -std=gnu11 -DSAGUI_HOST -DSAGUI_PORTABLE -fno-strict-aliasing -fcommon -w \
  -finput-charset=UTF-8 -fexec-charset=ISO-8859-1 -D_GNU_SOURCE \
  -I$(RAIZ)/userland/inc -I$(RAIZ)/userland/portable -I$(RAIZ)/userland/libsagui -I$(RAIZ)/userland/libfontes \
  -I$(RAIZ)/userland/ia -I$(RAIZ)/userland/planomini -I$(RAIZ)/userland/gebramini -idirafter $(RAIZ)/kernel/inc $(SDL_CFLAGS)
LDLIBS := $(SDL_LIBS) -lm

# La libsagui: solo lo que usan estas aplicaciones (el archivo .a deja fuera lo que nadie pide).
LIB_NOMES := presentacion presentacion_io presentacion_pptx compressao deflado_continuo difer edicao_rica edicao_rica_dialogos edicao_rica_obj formatacao formatar gui \
  gui_arquivos gui_arvore gui_barras gui_cinta gui_componentes gui_explorador gui_moderno gui_pestanas gui_revisar \
  hora icones iconos98 idioma imagem imagem_gif imagem_idct imagem_jpeg imagem_png_escribir inflado_continuo \
  pdf_escribir regex rtf texto texto_rico utilitarios wordart entrada zonas sincronia http sonidos conf_maquina contas empacotar dialogos_cf dialogos_objeto
KLIB_NOMES := fuso catalogo sha256
PORT_NOMES := janelas_sdl sistema_sdl sistema_nulo rapido_c
LIB_OBJ  := $(addprefix $(B)/libsagui/,$(addsuffix .o,$(LIB_NOMES))) \
            $(addprefix $(B)/kernel/lib/,$(addsuffix .o,$(KLIB_NOMES))) \
            $(addprefix $(B)/portable/,$(addsuffix .o,$(PORT_NOMES))) \
            $(B)/fonte8x16.o $(B)/libtls/hash.o
FONTES_OBJ := $(patsubst $(RAIZ)/userland/libfontes/%.c,$(B)/libfontes/%.o,$(wildcard $(RAIZ)/userland/libfontes/*.c))
IA_OBJ     := $(patsubst $(RAIZ)/userland/ia/%.c,$(B)/ia/%.o,$(wildcard $(RAIZ)/userland/ia/*.c))
MICO_OBJ   := $(patsubst $(RAIZ)/userland/programas/mico/%.c,$(B)/programas/mico/%.o,$(wildcard $(RAIZ)/userland/programas/mico/*.c))

GBM_OBJ    := $(patsubst $(RAIZ)/userland/gebramini/%.c,$(B)/obj/gebramini/%.o,$(wildcard $(RAIZ)/userland/gebramini/*.c))
PM_OBJ     := $(patsubst $(RAIZ)/userland/planomini/%.c,$(B)/obj/planomini/%.o,$(wildcard $(RAIZ)/userland/planomini/*.c))

APPS ?= mico pluma macaco tucan paint planomini gebramini
all: $(addprefix $(B)/,$(APPS)) $(B)/datos
# Un repositorio portable lleva las fuentes del sistema en datos/ (las siete DejaVu); el ejecutable las busca en ./datos.
$(B)/datos:
	@mkdir -p $(B); if [ -d $(RAIZ)/datos ]; then ln -sfn $(RAIZ)/datos $@; fi

$(B)/libsagui.a: $(LIB_OBJ)
	@rm -f $@ && ar rcs $@ $^

$(B)/%.o: $(RAIZ)/userland/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
$(B)/libsagui/%.o: $(RAIZ)/userland/libsagui/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
$(B)/libfontes/%.o: $(RAIZ)/userland/libfontes/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
$(B)/ia/%.o: $(RAIZ)/userland/ia/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
$(B)/obj/gebramini/%.o: $(RAIZ)/userland/gebramini/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
$(B)/obj/planomini/%.o: $(RAIZ)/userland/planomini/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
$(B)/libtls/%.o: $(RAIZ)/userland/libtls/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
$(B)/kernel/lib/%.o: $(RAIZ)/kernel/lib/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
$(B)/portable/%.o: $(RAIZ)/userland/portable/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
$(B)/programas/%.o: $(RAIZ)/userland/programas/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# La fuente 8x16 de la interfaz se genera del texto (tools/fonte/); un repo portable lleva el .c ya generado.
$(B)/fonte8x16.c: $(RAIZ)/tools/fonte/sagui8x16.txt $(RAIZ)/tools/mkfonte.py
	@mkdir -p $(dir $@)
	python3 $(RAIZ)/tools/mkfonte.py $< $@
$(B)/fonte8x16.o: $(B)/fonte8x16.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(B)/programas/macaco.o $(B)/programas/pluma.o $(B)/programas/tucan.o: $(wildcard $(RAIZ)/userland/programas/*.h)

$(B)/mico:   $(MICO_OBJ) $(IA_OBJ) $(B)/libsagui.a
	$(CC) -o $@ $^ $(LDLIBS)
$(B)/pluma:  $(B)/programas/pluma.o $(FONTES_OBJ) $(IA_OBJ) $(B)/libsagui.a
	$(CC) -o $@ $^ $(LDLIBS)
$(B)/macaco: $(B)/programas/macaco.o $(FONTES_OBJ) $(IA_OBJ) $(B)/libsagui.a
	$(CC) -o $@ $^ $(LDLIBS)
$(B)/tucan:  $(B)/programas/tucan.o $(FONTES_OBJ) $(IA_OBJ) $(B)/libsagui.a
	$(CC) -o $@ $^ $(LDLIBS)
$(B)/paint:  $(B)/programas/paint.o $(FONTES_OBJ) $(B)/libsagui.a
	$(CC) -o $@ $^ $(LDLIBS)
$(B)/planomini: $(B)/programas/planomini.o $(PM_OBJ) $(GBM_OBJ) $(B)/libsagui.a
	$(CC) -o $@ $^ $(LDLIBS)
$(B)/gebramini: $(B)/programas/gebramini.o $(GBM_OBJ) $(B)/libsagui.a
	$(CC) -o $@ $^ $(LDLIBS)

clean:
	rm -rf $(B)
.PHONY: all clean
