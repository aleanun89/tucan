// SPDX-License-Identifier: GPL-2.0-or-later
/* tucan_cinta.h - la cinta (Ribbon) de Tucán (D369): Inicio, Insertar, Diseño, Transiciones, Presentación y Ver.
 * Se incluye en tucan.c después de los iconos. Los ids son los de accion(); los menús de la cinta y el
 * teclado mandan las mismas órdenes. Mismo estilo que la cinta de Pluma y Macaco (D334). */

enum {
    A_NUEVO = 1, A_ABRIR, A_GUARDAR, A_GUARDAR_COMO, A_PDF, A_PDF_NOTAS, A_PNG_ACTUAL, A_PNG_TODAS, A_PPTX_EXP, A_SALIR,
    A_DESHACER, A_REHACER, A_CORTAR, A_COPIAR, A_PEGAR, A_BORRAR_OBJ, A_TODO,
    A_DIAP_NUEVA, A_DIAP_DUP, A_DIAP_BORRAR, A_DIAP_OCULTAR, A_DIAP_SUBIR, A_DIAP_BAJAR,
    A_NEGRITA, A_CURSIVA, A_SUBRAYADO, A_ALIN_IZQ, A_ALIN_CEN, A_ALIN_DER, A_VINETAS, A_NUMERACION, A_NIVEL_MAS, A_NIVEL_MENOS,
    A_TEXTO_NUEVO, A_IMAGEN, A_WORDART, A_WORDART_EDITAR, A_TABLA, A_NUM_DIAP, A_PIE, A_GRAFICO_EDITAR,
    A_FRENTE, A_ATRAS, A_AGRUPAR, A_DESAGRUPAR,
    A_TAM_169, A_TAM_43, A_FONDO_COLOR, A_FONDO_DEGRADADO, A_FONDO_IMAGEN, A_FONDO_TEMA,
    A_TRANS_TODAS, A_PRESENTAR, A_PRESENTAR_ACTUAL, A_AUTO, A_LASER, A_NOTAS, A_MINIATURAS, A_AYUDA, A_ACERCA, A_ASISTENTE,
    A_C_TAM = 900, A_C_FONTE, A_C_COR, A_C_RELLENO, A_C_BORDE, A_C_GROSOR, A_C_TEMA, A_C_TRANS, A_C_DUR, A_C_AVANZAR, A_C_ANIM,
    A_C_FTIT, A_C_FCUE, A_C_FONDO1, A_C_FONDO2,
    A_LAYOUT0 = 1000,      /* nueva diapositiva con el diseño i */
    A_APLICAR0 = 1010,     /* aplicar el diseño i a la actual */
    A_FORMA0 = 1020,       /* insertar la forma i */
    A_ALINEAR0 = 1040,     /* alinear: izq, centro, der, arriba, medio, abajo */
    A_GRAF0 = 1050,        /* insertar un gráfico de tipo i */
};

#define CT(idv, tp, fl, tx, tp_tip, kt_, ic) { .id = (idv), .tipo = (tp), .flags = (fl), .texto = (tx), .tip = (tp_tip), .kt = (kt_), .icone = (ic) }

static const int ids_layout_n[] = { A_LAYOUT0, A_LAYOUT0 + 1, A_LAYOUT0 + 2, A_LAYOUT0 + 3, A_LAYOUT0 + 4 };
static const int ids_layout_a[] = { A_APLICAR0, A_APLICAR0 + 1, A_APLICAR0 + 2, A_APLICAR0 + 3, A_APLICAR0 + 4 };
static const int ids_formas[] = { A_FORMA0, A_FORMA0 + 1, A_FORMA0 + 2, A_FORMA0 + 3, A_FORMA0 + 4, A_FORMA0 + 5, A_FORMA0 + 6, A_FORMA0 + 7 };
static const int ids_alinear[] = { A_ALINEAR0, A_ALINEAR0 + 1, A_ALINEAR0 + 2, 0, A_ALINEAR0 + 3, A_ALINEAR0 + 4, A_ALINEAR0 + 5 };
static const int ids_graf[] = { A_GRAF0, A_GRAF0 + 1, A_GRAF0 + 2 };
static const char *const menu_layout = NMENU_("Diapositiva de título\nTítulo y contenido\nDos columnas\nSolo el título\nEn blanco");
static const char *const menu_formas = NMENU_("Rectángulo\nRectángulo redondeado\nElipse\nTriángulo\nRombo\nFlecha\nLínea\nEstrella");
static const char *const menu_alinear = NMENU_("Alinear a la izquierda\nCentrar horizontalmente\nAlinear a la derecha\n-\nAlinear arriba\nCentrar verticalmente\nAlinear abajo");
static const char *const menu_graf = NMENU_("Gráfico de barras\nGráfico de líneas\nGráfico de sectores");
static const char *const tams_txt[] = { "8", "10", "12", "14", "16", "18", "20", "24", "28", "32", "36", "40", "44", "48", "60", "72", "96" };
static const char *const grosores_txt[] = { "0", "1", "2", "3", "4", "6", "8" };
static const char *const durs_txt[] = { "0,3 s", "0,5 s", "1 s", "2 s" };
static const char *const avanzar_txt[] = { "Con un clic", "3 s", "5 s", "10 s", "30 s" };
static const char *lista_trans[] = { N_("Corte"), N_("Fundido"), N_("Desplazar"), N_("Barrido") };
static const char *lista_anim[] = { N_("Ninguna"), N_("Aparecer"), N_("Fundido") };
static const char *lista_temas[10];

static cinta_ctrl_t cc_acceso[] = {
    CT(A_GUARDAR, CC_BOTON, CCF_SIN_TEXTO, N_("Guardar"), N_("Guardar\nGuarda la presentación\nCtrl+S"), "1", ic_guardar),
    CT(A_DESHACER, CC_BOTON, CCF_SIN_TEXTO, N_("Deshacer"), N_("Deshacer\nDeshace la última acción\nCtrl+Z"), "2", ic_deshacer),
    CT(A_REHACER, CC_BOTON, CCF_SIN_TEXTO, N_("Rehacer"), N_("Rehacer\nRepite lo deshecho\nCtrl+Y"), "3", ic_rehacer),
};

/* ---- Inicio ---- */
static cinta_ctrl_t cc_portapapeles[] = {
    CT(A_PEGAR, CC_BOTON, CCF_GRANDE, N_("Pegar"), N_("Pegar\nPega el objeto copiado\nCtrl+V"), "V", ic_pegar),
    CT(A_CORTAR, CC_BOTON, 0, N_("Cortar"), N_("Cortar\nQuita el objeto y lo guarda\nCtrl+X"), "X", ic_cortar),
    CT(A_COPIAR, CC_BOTON, 0, N_("Copiar"), N_("Copiar\nGuarda una copia del objeto\nCtrl+C"), "C", ic_copiar),
};
static cinta_ctrl_t cc_diapositivas[] = {
    { .id = A_DIAP_NUEVA, .tipo = CC_DIVIDIDO, .flags = CCF_GRANDE, .texto = N_("Nueva diapositiva"), .tip = N_("Nueva diapositiva\nAñade una diapositiva después de la actual\nCtrl+M"),
      .kt = "N", .icone = ic_nueva, .menu = NULL, .ids = ids_layout_n },
    { .id = A_APLICAR0, .tipo = CC_DESPLEG, .texto = N_("Diseño"), .tip = N_("Diseño\nCambia la disposición de la diapositiva actual"), .kt = "DS", .menu = NULL, .ids = ids_layout_a },
    CT(A_DIAP_DUP, CC_BOTON, CCF_SIGUE, N_("Duplicar"), N_("Duplicar\nCopia la diapositiva actual justo detrás"), "DU", ic_dup),
    CT(A_DIAP_BORRAR, CC_BOTON, 0, N_("Eliminar"), N_("Eliminar\nQuita la diapositiva actual\nSupr en la lista"), "EL", ic_borrar),
};
static cinta_ctrl_t cc_fuente[] = {
    CT(A_C_FONTE, CC_FUENTES, 0, NULL, N_("Fuente\nFamilia de la letra del objeto"), "FF", NULL),
    { .id = A_C_TAM, .tipo = CC_COMBO, .flags = CCF_SIGUE | CCF_EDITABLE, .tip = N_("Tamaño\nTamaño de la letra en puntos"), .kt = "FS",
      .lista = tams_txt, .n_lista = 17, .ancho = 56 },
    CT(A_NEGRITA, CC_CONMUT, 0, N_("N"), N_("Negrita\nPone el texto del objeto en negrita\nCtrl+B"), "1", NULL),
    CT(A_CURSIVA, CC_CONMUT, CCF_SIGUE, N_("K"), N_("Cursiva\nPone el texto del objeto en cursiva\nCtrl+I"), "2", NULL),
    CT(A_SUBRAYADO, CC_CONMUT, CCF_SIGUE, N_("S"), N_("Subrayado\nSubraya el texto del objeto\nCtrl+U"), "3", NULL),
    CT(A_C_COR, CC_COR, CCF_SIGUE, N_("Color de la letra"), N_("Color de la letra\nCambia el color del texto del objeto"), "4", NULL),
};
static cinta_ctrl_t cc_paragrafo[] = {
    { .id = A_ALIN_IZQ, .tipo = CC_EXCL, .flags = CCF_SIN_TEXTO, .texto = N_("Izquierda"), .tip = N_("Alinear a la izquierda"), .kt = "AI", .icone = ic_esq, .excl = 1 },
    { .id = A_ALIN_CEN, .tipo = CC_EXCL, .flags = CCF_SIN_TEXTO | CCF_SIGUE, .texto = N_("Centro"), .tip = N_("Centrar"), .kt = "AC", .icone = ic_centro, .excl = 1 },
    { .id = A_ALIN_DER, .tipo = CC_EXCL, .flags = CCF_SIN_TEXTO | CCF_SIGUE, .texto = N_("Derecha"), .tip = N_("Alinear a la derecha"), .kt = "AD", .icone = ic_dir, .excl = 1 },
    CT(A_VINETAS, CC_CONMUT, CCF_SIN_TEXTO, N_("Viñetas"), N_("Viñetas\nLista con viñetas"), "LV", ic_vinetas),
    CT(A_NUMERACION, CC_CONMUT, CCF_SIN_TEXTO | CCF_SIGUE, N_("Numeración"), N_("Numeración\nLista numerada"), "LN", ic_numeracion),
    CT(A_NIVEL_MENOS, CC_BOTON, CCF_SIN_TEXTO | CCF_SIGUE, N_("Subir nivel"), N_("Subir nivel\nQuita un nivel de sangría al párrafo (Mayús+Tab)"), "SM", ic_nivel_menos),
    CT(A_NIVEL_MAS, CC_BOTON, CCF_SIN_TEXTO | CCF_SIGUE, N_("Bajar nivel"), N_("Bajar nivel\nAñade un nivel de sangría al párrafo (Tab)"), "SA", ic_nivel_mas),
};
static cinta_ctrl_t cc_dibujo[] = {
    { .id = A_FORMA0, .tipo = CC_DESPLEG, .flags = CCF_GRANDE, .texto = N_("Formas"), .tip = N_("Formas\nInserta una forma en la diapositiva"), .kt = "FO", .icone = ic_forma, .menu = NULL, .ids = ids_formas },
    CT(A_C_RELLENO, CC_COR, 0, N_("Relleno"), N_("Relleno de forma\nColor de relleno de la forma elegida"), "RL", NULL),
    CT(A_C_BORDE, CC_COR, 0, N_("Contorno"), N_("Contorno de forma\nColor del contorno de la forma elegida"), "CT", NULL),
    { .id = A_C_GROSOR, .tipo = CC_COMBO, .flags = CCF_EDITABLE, .tip = N_("Grosor\nGrosor del contorno en puntos (0 = sin contorno)"), .kt = "GR", .lista = grosores_txt, .n_lista = 7, .ancho = 48 },
};
static cinta_ctrl_t cc_organizar[] = {
    CT(A_FRENTE, CC_BOTON, 0, N_("Traer al frente"), N_("Traer al frente\nPone el objeto encima de los demás"), "TF", ic_frente),
    CT(A_ATRAS, CC_BOTON, 0, N_("Enviar al fondo"), N_("Enviar al fondo\nPone el objeto debajo de los demás"), "EF", ic_atras),
    CT(A_AGRUPAR, CC_BOTON, 0, N_("Agrupar"), N_("Agrupar\nUne los objetos elegidos para moverlos juntos\nCtrl+G"), "AG", NULL),
    CT(A_DESAGRUPAR, CC_BOTON, CCF_SIGUE, N_("Desagrupar"), N_("Desagrupar\nSepara los objetos del grupo"), "DG", NULL),
    { .id = A_ALINEAR0, .tipo = CC_DESPLEG, .texto = N_("Alinear"), .tip = N_("Alinear\nAlinea los objetos elegidos o los centra en la diapositiva"), .kt = "AL", .menu = NULL, .ids = ids_alinear },
    CT(A_TODO, CC_BOTON, 0, N_("Seleccionar todo"), N_("Seleccionar todo\nElige todos los objetos de la diapositiva\nCtrl+A"), "SE", NULL),
    CT(A_BORRAR_OBJ, CC_BOTON, CCF_SIGUE, N_("Borrar objeto"), N_("Borrar objeto\nQuita los objetos elegidos\nSupr"), "BO", ic_borrar),
};
static cinta_ctrl_t cc_asistente[] = {
    CT(A_ASISTENTE, CC_CONMUT, CCF_GRANDE, N_("Asistente IA"), N_("Asistente IA\nAbre el panel del asistente\nF12"), "IA", ic_ia),
};
static cinta_ctrl_t cc_ayuda[] = {
    CT(A_AYUDA, CC_BOTON, CCF_GRANDE, N_("Ayuda"), N_("Ayuda\nAtajos y modo presentación\nF1"), "AY", ic_ayuda),
};

/* ---- Insertar ---- */
static cinta_ctrl_t cc_ins_basicos[] = {
    CT(A_TEXTO_NUEVO, CC_BOTON, CCF_GRANDE, N_("Cuadro de texto"), N_("Cuadro de texto\nDibuja un cuadro de texto en la diapositiva"), "T", ic_texto),
    CT(A_IMAGEN, CC_BOTON, CCF_GRANDE, N_("Imagen"), N_("Imagen\nInserta una imagen BMP, PNG, JPEG o GIF"), "I", ic_imagen),
    { .id = A_FORMA0, .tipo = CC_DESPLEG, .flags = CCF_GRANDE, .texto = N_("Formas"), .tip = N_("Formas\nInserta una forma en la diapositiva"), .kt = "F", .icone = ic_forma, .menu = NULL, .ids = ids_formas },
};
static cinta_ctrl_t cc_ins_objetos[] = {
    CT(A_TABLA, CC_BOTON, CCF_GRANDE, N_("Tabla"), N_("Tabla\nInserta una tabla sencilla"), "TB", ic_tabla),
    { .id = A_GRAF0, .tipo = CC_DESPLEG, .flags = CCF_GRANDE, .texto = N_("Gráfico"), .tip = N_("Gráfico\nInserta un gráfico de barras, líneas o sectores"), .kt = "G", .icone = ic_grafico, .menu = NULL, .ids = ids_graf },
    CT(A_WORDART, CC_BOTON, CCF_GRANDE, N_("WordArt"), N_("WordArt\nInserta un texto decorativo"), "W", ic_wordart),
};
static cinta_ctrl_t cc_ins_pie[] = {
    CT(A_NUM_DIAP, CC_CONMUT, 0, N_("Número de diapositiva"), N_("Número de diapositiva\nMuestra el número al pie de cada diapositiva"), "NU", NULL),
    CT(A_PIE, CC_BOTON, 0, N_("Pie de página"), N_("Pie de página\nTexto al pie de todas las diapositivas"), "PI", NULL),
};

/* ---- Diseño ---- */
static cinta_ctrl_t cc_tamano[] = {
    { .id = A_TAM_169, .tipo = CC_EXCL, .texto = N_("16:9"), .tip = N_("Panorámica 16:9"), .kt = "P", .excl = 2, .marcado = true },
    { .id = A_TAM_43, .tipo = CC_EXCL, .texto = N_("4:3"), .tip = N_("Estándar 4:3"), .kt = "E", .excl = 2 },
};
static cinta_ctrl_t cc_temas[] = {
    { .id = A_C_TEMA, .tipo = CC_GALERIA, .tip = N_("Temas\nColores, fuentes y fondo de toda la presentación"), .kt = "TE", .lista = lista_temas, .n_lista = 10 },
};
static cinta_ctrl_t cc_fondo[] = {
    CT(A_FONDO_COLOR, CC_BOTON, 0, N_("Color de fondo"), N_("Color de fondo\nUn color liso para esta diapositiva"), "FC", NULL),
    CT(A_FONDO_DEGRADADO, CC_BOTON, 0, N_("Degradado"), N_("Degradado\nDe un color a otro, de arriba abajo"), "FD", NULL),
    CT(A_FONDO_IMAGEN, CC_BOTON, 0, N_("Imagen de fondo"), N_("Imagen de fondo\nUna imagen que cubre la diapositiva"), "FI", ic_imagen),
    CT(A_FONDO_TEMA, CC_BOTON, 0, N_("Fondo del tema"), N_("Fondo del tema\nVuelve al fondo del tema"), "FT", NULL),
};
static cinta_ctrl_t cc_maestra[] = {
    CT(A_C_FTIT, CC_FUENTES, 0, NULL, N_("Fuente de los títulos\nLa fuente de todos los títulos (diapositiva maestra)"), "MT", NULL),
    CT(A_C_FCUE, CC_FUENTES, 0, NULL, N_("Fuente del cuerpo\nLa fuente de todo el texto (diapositiva maestra)"), "MC", NULL),
};

/* ---- Transiciones ---- */
static cinta_ctrl_t cc_trans[] = {
    { .id = A_C_TRANS, .tipo = CC_GALERIA, .tip = N_("Transición\nCómo entra esta diapositiva en la presentación"), .kt = "TR", .lista = lista_trans, .n_lista = 4 },
};
static cinta_ctrl_t cc_trans_tiempo[] = {
    { .id = A_C_DUR, .tipo = CC_COMBO, .tip = N_("Duración\nCuánto dura la transición"), .kt = "DU", .lista = durs_txt, .n_lista = 4, .ancho = 72 },
    { .id = A_C_AVANZAR, .tipo = CC_COMBO, .tip = N_("Avanzar\nPasa sola a la siguiente tras unos segundos (con «Tiempos automáticos»)"), .kt = "AV", .lista = avanzar_txt, .n_lista = 5, .ancho = 96 },
    CT(A_TRANS_TODAS, CC_BOTON, 0, N_("Aplicar a todas"), N_("Aplicar a todas\nUsa esta transición y estos tiempos en todas las diapositivas"), "AT", NULL),
};
static cinta_ctrl_t cc_anim[] = {
    { .id = A_C_ANIM, .tipo = CC_GALERIA, .tip = N_("Animación de entrada\nCómo aparece el objeto elegido, con cada clic"), .kt = "AN", .lista = lista_anim, .n_lista = 3 },
};

/* ---- Presentación ---- */
static cinta_ctrl_t cc_iniciar[] = {
    CT(A_PRESENTAR, CC_BOTON, CCF_GRANDE, N_("Desde el principio"), N_("Desde el principio\nPantalla completa desde la primera diapositiva\nF5"), "P", ic_presentar),
    CT(A_PRESENTAR_ACTUAL, CC_BOTON, CCF_GRANDE, N_("Desde la actual"), N_("Desde la actual\nPantalla completa desde esta diapositiva\nMayús+F5"), "A", ic_presentar),
};
static cinta_ctrl_t cc_configurar[] = {
    CT(A_DIAP_OCULTAR, CC_CONMUT, 0, N_("Ocultar diapositiva"), N_("Ocultar diapositiva\nNo se muestra ni se exporta a PDF"), "O", NULL),
    CT(A_AUTO, CC_CONMUT, 0, N_("Tiempos automáticos"), N_("Tiempos automáticos\nAvanza sola según los segundos de cada diapositiva"), "T", NULL),
    CT(A_LASER, CC_CONMUT, 0, N_("Puntero láser"), N_("Puntero láser\nUn punto rojo sigue al ratón (tecla L durante la presentación)"), "L", NULL),
};
static cinta_ctrl_t cc_exportar[] = {
    CT(A_PDF, CC_BOTON, 0, N_("Exportar a PDF"), N_("Exportar a PDF\nUna diapositiva por página"), "PD", ic_pdf),
    CT(A_PDF_NOTAS, CC_BOTON, 0, N_("PDF con notas"), N_("PDF con notas\nCada página lleva la diapositiva y sus notas"), "PN", ic_pdf),
};

/* ---- Ver ---- */
static cinta_ctrl_t cc_ver_paneles[] = {
    { .id = A_NOTAS, .tipo = CC_CONMUT, .texto = N_("Notas"), .tip = N_("Notas\nMuestra u oculta el panel de notas del orador"), .kt = "N", .marcado = true },
    { .id = A_MINIATURAS, .tipo = CC_CONMUT, .texto = N_("Miniaturas"), .tip = N_("Miniaturas\nMuestra u oculta la lista de diapositivas"), .kt = "M", .marcado = true },
};

static cinta_grupo_t cg_inicio[] = {
    CINTA_GRUPO(N_("Portapapeles"), cc_portapapeles, 0, 5),
    CINTA_GRUPO(N_("Diapositivas"), cc_diapositivas, 0, 4),
    CINTA_GRUPO(N_("Fuente"), cc_fuente, 0, 4),
    CINTA_GRUPO(N_("Párrafo"), cc_paragrafo, 0, 3),
    CINTA_GRUPO(N_("Dibujo"), cc_dibujo, 0, 2),
    CINTA_GRUPO(N_("Organizar"), cc_organizar, 0, 1),
};
static cinta_grupo_t cg_insertar[] = {
    CINTA_GRUPO(N_("Básicos"), cc_ins_basicos, 0, 1),
    CINTA_GRUPO(N_("Objetos"), cc_ins_objetos, 0, 2),
    CINTA_GRUPO(N_("Pie"), cc_ins_pie, 0, 3),
};
static cinta_grupo_t cg_diseno[] = {
    CINTA_GRUPO(N_("Tamaño"), cc_tamano, 0, 3),
    CINTA_GRUPO(N_("Temas"), cc_temas, 0, 1),
    CINTA_GRUPO(N_("Fondo"), cc_fondo, 0, 2),
    CINTA_GRUPO(N_("Diapositiva maestra"), cc_maestra, 0, 4),
};
static cinta_grupo_t cg_trans[] = {
    CINTA_GRUPO(N_("Transición"), cc_trans, 0, 1),
    CINTA_GRUPO(N_("Tiempo"), cc_trans_tiempo, 0, 2),
    CINTA_GRUPO(N_("Animación"), cc_anim, 0, 3),
};
static cinta_grupo_t cg_present[] = {
    CINTA_GRUPO(N_("Iniciar"), cc_iniciar, 0, 1),
    CINTA_GRUPO(N_("Configurar"), cc_configurar, 0, 2),
    CINTA_GRUPO(N_("Exportar"), cc_exportar, 0, 3),
};
static cinta_grupo_t cg_ver[] = {
    CINTA_GRUPO(N_("Paneles"), cc_ver_paneles, 0, 1),
    CINTA_GRUPO(N_("Asistente"), cc_asistente, 0, 3),
    CINTA_GRUPO(N_("Ayuda"), cc_ayuda, 0, 2),
};
static cinta_pest_t cp_tucan[] = {
    { .nome = N_("Inicio"), .kt = "I", .grupos = cg_inicio, .n = 6 },
    { .nome = N_("Insertar"), .kt = "N", .grupos = cg_insertar, .n = 3 },
    { .nome = N_("Diseño"), .kt = "D", .grupos = cg_diseno, .n = 4 },
    { .nome = N_("Transiciones"), .kt = "T", .grupos = cg_trans, .n = 3 },
    { .nome = N_("Presentación"), .kt = "P", .grupos = cg_present, .n = 3 },
    { .nome = N_("Ver"), .kt = "V", .grupos = cg_ver, .n = 3 },
};
#define N_PEST 6

static const char *const archivo_menu = NMENU_("Nueva\tCtrl+N\nAbrir...\tCtrl+O\n-\nGuardar\tCtrl+S\nGuardar como...\n-\nExportar a PDF...\nExportar a PDF con notas...\nExportar a PowerPoint (.pptx)...\nExportar diapositiva actual a PNG...\nExportar todas a PNG...\n-\nSalir");
static const int archivo_ids[] = { A_NUEVO, A_ABRIR, 0, A_GUARDAR, A_GUARDAR_COMO, 0, A_PDF, A_PDF_NOTAS, A_PPTX_EXP, A_PNG_ACTUAL, A_PNG_TODAS, 0, A_SALIR };
