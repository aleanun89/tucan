/* icones.c - biblioteca de iconos del SaguiOS (D135).
 *
 * Generado por tools/gerar-icones.py: no editar a mano. Los iconos vienen
 * de los "Fluent UI System Icons" de Microsoft (licencia MIT), descargados
 * del Iconify y rasterizados a 16x16 píxeles. El formato es el del proyecto:
 * cada línea tiene 16 caracteres y cada carácter es un tono de la paleta
 * compartida (abajo).
 *
 * La licencia y el aviso de copyright están en userland/terceros/fluentui/
 * (vea docs/terceros.md). */
#include "icones.h"

static const char *const ic_inicio[] = {
    "................", ".......PP.......", "......POOP......", ".....POYYOP.....",
    "....POYYYYOP....", "...POYYYYYYOP...", "..POYYYYYYYYOP..", ".POYYYYYYYYYYOP.",
    "..PYYYYYYYYYYP..", "..PYYYAOOAYYYP..", "..PYYYOKKOYYYP..", "..PYYYOKKOYYYP..",
    "..PYYYOKKOYYAP..", "..PPPPPPPPPPPP..", "................", "................",
};

static const char *const ic_casa[] = {
    "................", "................", "...PPPPPPP......", "...PTTTTTP......",
    "...PTLTLTP......", "...PTTTTTP......", "...PTTTTVP......", "...PTTTTVVPPP...",
    "...PTTTVVROOOP..", "...PTLVVROAYAOP.", "...PTVVROYYYYYOP", "...PTVVCYYYYYYP.",
    "...PVVVVYYOKOYP.", "...PPPPPYYOKOYP.", "........PPPPPPP.", "................",
};

static const char *const ic_edificio[] = {
    "................", "................", "....PPPPPPP.....", "....PTTTTTP.....",
    "....PTLTLTP.....", "....PTTTTTP.....", "....PTTTTTP.....", "....PTTTTTVPP...",
    "....PTTTTVVVP...", "....PTLTLVLVP...", "....PTTVVVVVP...", "....PTVVVVVVP...",
    "....PVVVVVDVP...", "....PPPPPPPPP...", "................", "................",
};

static const char *const ic_aplicativos[] = {
    "................", "................", "..........PP....", "..PPPPPP.PTTP...",
    "..PCCCCCPTTTVP..", "..PCCCCCTTTVVP..", "..PCCCCCPTVVP...", "..PCCCCP.PVP....",
    "..PDDDDDPCVCP...", "..PDDDDDCCCCP...", "..PDDDDDCCCDP...", "..PDDDDDCCDDP...",
    "..PDDDDDCDDDP...", "..PPPPPPPPPPP...", "................", "................",
};

static const char *const ic_quadro[] = {
    "................", "................", "...PPPPPPPPPP...", "..PCCCCCGGGGGP..",
    "..PCCCGGGGGGGP..", "..PGGGGGGGGGGP..", "..PGGGGGGGGGGP..", "..PGGGGGGGGGGP..",
    "..PGGGGDGGGGGP..", "..PGGGDDGGGGGP..", "..PGGDDDCGGGGP..", "..PGDDDDGGGGGP..",
    "..PDDDDDGGGGGP..", "...PPPPPPPPPP...", "................", "................",
};

static const char *const ic_ajustes[] = {
    "................", "................", ".......PP.......", "......PCCP......",
    "...PPPCCCCPPP...", "...PCCCPPCCCP...", "..PCCCP..PCCCP..", "...PCP....PCP...",
    "...PCP....PCP...", "..PCCCP..PCCCP..", "...PCCCPPCCCP...", "...PPPCCCCPPP...",
    "......PCCP......", ".......PP.......", "................", "................",
};

static const char *const ic_pessoas[] = {
    "................", "................", "................", "....PPP.........",
    "...PMMMP..PPP...", "...PMMMP..PMP...", "...PMMMP..PMP...", "....PPP...PPP...",
    "................", "..PPPPPPPPPPPP..", ".PMMMMMMMMMMMP..", "..PMMMMMMPPMPP..",
    "..PPPMPPP..P....", ".....P..........", "................", "................",
};

static const char *const ic_grupo[] = {
    "................", "................", ".......PP.......", "..PPP.PMMP.PPP..",
    "..PMP.PMMP.PMP..", "..PPP..PP..PPP..", "................", ".PPPPPPPPPPPPPP.",
    ".PMMMMMMMMMMMMP.", ".PMMMMMMMMMMMMP.", ".PMMMMMMMMMMMMP.", ".PMMMMMMMMMMMMP.",
    "..PPPPMMMMPPPP..", "......PPPP......", "................", "................",
};

static const char *const ic_pessoa[] = {
    "................", "................", "......PPPP......", ".....PMMMMP.....",
    ".....PMMMMP.....", "......PMMP......", "......PPPP......", "................",
    "...PPPPPPPPPP...", "...PMMMMMMMMP...", "...PMMMMMMMMP...", "...PMMMMMMMMP...",
    "....PMMMMMMP....", ".....PPPPPP.....", "................", "................",
};

static const char *const ic_pessoa_nova[] = {
    "................", "................", ".......PPPP.....", "......PMMMMP....",
    ".....PMMMMMMP...", ".....PMMMMMMP...", ".....PMMMMMMP...", ".....PMMMMMMP...",
    "......PMMMMP....", ".......PPPP.....", "............PPPP", "...PPPPPPPPPGGGP",
    "..PMMMMMMMCGGGLP", "..PMMMMMMMGGGGBP", "..PMMMMMMMGGLBBP", "...PPPPPPPPPPPPP",
};

static const char *const ic_calendario[] = {
    "................", "................", "...PPPPPPPPPP...", "..PVVVVVVVVVVP..",
    "..PTVVVVVVVVVP..", "..PTVVVVVVVVVP..", "..PLLLLLLLLLLP..", "..PLCTLCCLTCLP..",
    "..PLTVLTTLVTLP..", "..PLLLLLLLLLLP..", "..PLTVLVVLLLLP..", "..PLLTLCCLLLLP..",
    "..PLLLLLLLLLLP..", "...PPPPPPPPPP...", "................", "................",
};

static const char *const ic_relogio[] = {
    "................", "................", "......PPPP......", "....PPTTTTPP....",
    "...PTTTTTTTTP...", "...PTTTLTTTTP...", "..PTTTTBTTTVVP..", "..PTTTTLTVVVVP..",
    "..PTTTTLLLVVVP..", "..PTTVVVVVVVVP..", "...PVVVVVVVVP...", "...PVVVVVVVVP...",
    "....PPVVVVPP....", "......PPPP......", "................", "................",
};

static const char *const ic_despertador[] = {
    "................", "................", "...PPP....PPP...", "..PAACPPPPAAAP..",
    "..PACTLBBLTCAP..", "..PCTBBLBBBTCP..", "...PLBBTBBBLP...", "...PBBBTBLLLP...",
    "..PCBBBVCCLLCP..", "...PBLLCCCLLP...", "...PLLLLLLLLP...", "...PTLLLLLLVP...",
    "...PPPLLLLPPP...", "...P..PPPP..P...", "................", "................",
};

static const char *const ic_coracao[] = {
    "................", "................", "................", "...PPPP..PPP....",
    "..PRRRRPPRRRP...", "..PRRRRRRRRRRP..", "..PRRRRRRRRRRP..", "..PRRRRRRRRRRP..",
    "...PRRRRRRRRP...", "....PRRRRRRP....", ".....PRRRRP.....", "......PRRP......",
    ".......PP.......", "................", "................", "................",
};

static const char *const ic_estrela[] = {
    "................", "................", ".......PP.......", "......PAAP......",
    "......PAAP......", "...PPPAAAAPPP...", "..PAAAAAAAOOOPP.", "..PAAAAAAOOOOP..",
    "...PAAAAOOOOP...", "....PAAOOOOP....", "....PAOOOOOP....", "....POOOOOOP....",
    "....POPPPPOP....", "....PP....PP....", "................", "................",
};

static const char *const ic_luz[] = {
    "................", "................", "......PPPP......", ".....POOOOP.....",
    "....POOOOOOP....", "....POOOOOOP....", "...POOOOOOOOP...", "....POOOOOOP....",
    "....POOOOOOP....", ".....POOOOP.....", ".....POOOOP.....", ".....PYYYYP.....",
    "......POOP......", "......PPPP......", "................", "................",
};

static const char *const ic_presente[] = {
    "................", "................", ".....PPPPP......", "....P..P..P.....",
    "....P..P..P.....", "..PPMPPOPPMPP...", "..PMMMMAMMMMP...", "..PMMMMAMMMMP...",
    "..PMMMMAMMMMP...", "...PMMMOMMMP....", "...PMMMAMMMP....", "...PMMMAMMMP....",
    "...PMMMAMMMP....", "....PPPPPPP.....", "................", "................",
};

static const char *const ic_maleta[] = {
    "................", "................", ".....PPPPPP.....", ".....P....P.....",
    ".....P....P.....", "...PPTPPPPTPP...", "..PTTTTTTTTTTP..", "..PTTTTTTTTTTP..",
    "..PTTTTLLTTTTP..", "..PTTTTLLTTTTP..", "..PVVVVTTVVVVP..", "..PVVVVVVVVMMP..",
    "..PVVVVVMMMMMP..", "...PPPPPPPPPP...", "................", "................",
};

static const char *const ic_ferramentas[] = {
    "................", "................", "................", ".....PPPPPP.....",
    ".....P....P.....", "...PPMPPPPRPP...", "..PRRRRRRRRRRP..", "..PRRLRRRRLRRP..",
    "..PMMLMMMMLMMP..", "..PMMLRRRRLRRP..", "..PRRRRRRRRRRP..", "..PRRRRRRRRRRP..",
    "...PPPPPPPPPP...", "................", "................", "................",
};

static const char *const ic_chave_inglesa[] = {
    "................", ".........PPP....", "........PTP.....", ".......PTP......",
    ".......PTP....P.", ".......PTTP..PP.", ".......PTTTPPTP.", "......PTTTTTTP..",
    ".....PTTTPPPP...", "....PTTTP.......", "...PTTTP........", "..PVVVP.........",
    ".PVVVP..........", "..PVP...........", "...P............", "................",
};

static const char *const ic_bandeira[] = {
    "................", "................", "...PPPPPPPPPP...", "...PRRRRRRRRP...",
    "...PRRRRRRRP....", "...PRRRRRRRP....", "...PRRRRRRRP....", "...PRRRRRRRP....",
    "...PRRRRRRRRP...", "...PPPPPPPPPP...", "...P............", "...P............",
    "...P............", "...P............", "................", "................",
};

static const char *const ic_marcador[] = {
    "................", "................", "....PPPPPPPP....", "...PMMMRRRRRP...",
    "...PMMRRRRRRP...", "...PRRRRRRRRP...", "...PRRRRRRRRP...", "...PRRRRRRRRP...",
    "...PRRRRRRRRP...", "...PRRRRRRRRP...", "...PRRRPPRRRP...", "...PRRP..PRRP...",
    "...PPP....PPP...", "...P........P...", "................", "................",
};

static const char *const ic_nuvem[] = {
    "................", "................", "......PPPP......", ".....PMMMMP.....",
    "....PMMMMMMP....", "....PMMMMMMP....", "..PPMMMMMMMMPP..", ".PMMMMMMMMMMMMP.",
    ".PMMMMMMMMMMMMP.", ".PMMMMMMMMMMMMP.", ".PMMMMMMMMMMMMP.", "..PPPPPPPPPPPP..",
    "................", "................", "................", "................",
};

static const char *const ic_nuvem_erro[] = {
    "................", "................", "......PPPP......", ".....PMMMMP.....",
    "....PMMMMMMP....", "....PMMMMMMP....", "..PPMMMMMRRRPP..", ".PMMMMMRRRRRRRP.",
    ".PMMMMMRRRRRRRP.", ".PMMMMRRRLRLRRP.", ".PMMMMRRRRBRRRP.", "..PPPPRRRLRLRRP.",
    "......PRRRRRRRP.", ".......PRRRRRP..", "........PPPPP...", "................",
};

static const char *const ic_tarefa[] = {
    "................", ".....PPPPPP.....", "...PPAAAAAAPP...", "...PTCAAAACTP...",
    "...PTTTTTTTTP...", "...PTTTTTTTTP...", "...PTTTTTTTTP...", "...PTTTTTTLTP...",
    "...PTTTTTLTTP...", "...PTTLTLTTTP...", "...PTTTLTTTTP...", "...PTTTTTTTTP...",
    "...PTTTTTTTTP...", "...PTTTTTTTTP...", "...PPPPPPPPPP...", "................",
};

static const char *const ic_prancheta[] = {
    "................", ".....PPPPPP.....", "...PPAAAAAAPP...", "...PTCAAAACTP...",
    "...PTTTTTTTTP...", "...PTTTTTTTTP...", "...PTTTTTTTTP...", "...PTTTTTTTTP...",
    "...PTTTTTTTTP...", "...PTTTTTTTTP...", "...PTTTTTTTTP...", "...PTTTTTTTTP...",
    "...PTTTTTTTTP...", "...PTTTTTTTTP...", "...PPPPPPPPPP...", "................",
};

static const char *const ic_cofre[] = {
    "................", "................", "................", "..PPPPPPPPPPPP..",
    ".PGGGGGGGGGGGGP.", ".PGGGGGGGGGGGTP.", ".PGGLGGGLCCLTTP.", ".PGGBGGGCLLCTTP.",
    ".PGGBGTTCLLCTTP.", ".PGTLTTTLCCLTTP.", ".PTTTTTTTTTTVVP.", ".PTTTTTTTVVVVVP.",
    "..PPPPPPPPPPPP..", "................", "................", "................",
};

static const char *const ic_sincronizar[] = {
    ".......P........", ".......PP.......", "......PTTP......", "....PPPTTP.P....",
    "...PP..PP..PP...", "...P........P...", "..PP........PP..", "..PP........PP..",
    "..PP........PP..", "..PP........PP..", "...P........P...", "...PP..PP..PP...",
    "....P.PTTPPP....", "......PTTP......", ".......PP.......", "........P.......",
};

static const char *const ic_historico[] = {
    "................", "................", "...P..PPPP......", "..PTPPPPPPPP....",
    "..PTP......PP...", "..PTTP.P....P...", "...PPP.PP...PP..", ".......PMP..PP..",
    "..P....PPP..PP..", "..PP........PP..", "..PP........P...", "...PP......PP...",
    "....PPPPPPPP....", "......PPPP......", "................", "................",
};

static const char *const ic_trophy[] = {
    "................", ".....PPPPPP.....", "....PAAAAAAP....", "..PPAAAAAAAAPP..",
    "..P.PAAAAAAP.P..", "..P.PAAAAAAP.P..", "..PPAAAAAAAAPP..", "...PAAAAAAAAP...",
    "....PAAAAAAP....", ".....PAAAAP.....", "......PAAP......", ".......PP.......",
    ".....PPOOPP.....", "....POOOOOOP....", "....PPPPPPPP....", "................",
};

static const char *const ic_puzzle[] = {
    "................", "........PP......", ".......PTTP.....", "....PPPTTTTPPP..",
    "....PTTTTTTTTP..", "....PTTTTTTTTP..", "...PTTTTTTTVPP..", "..PTTTTTTVVP....",
    "..PTTTTVVVVP....", "...PTVVVVVVVPP..", "....PVVVVVVVVP..", "....PVVVVVVVVP..",
    "....PPPVVVVPPP..", ".......PMMP.....", "........PP......", "................",
};

static const char *const ic_robo[] = {
    "................", "................", "....PPPPPPPP....", "....PVVVVVVP....",
    "....PMLMMLMP....", "....PMLMMLMP....", "....PVVVVVVP....", "....PPPPPPPP....",
    "................", "....PPPPPPPP....", "...PVVVVVVVMP...", "...PVVVMMMMMP...",
    "...PMMMMMMMMP...", "....PMMMMMMP....", ".....PPPPPP.....", "................",
};

static const char *const ic_medidor[] = {
    "................", "................", ".........PPPPPP.", ".......PPLLLLLLP",
    ".....PPLLLLLLLLP", "....PLLLLRRRRRLP", "....PLLRRRCLLCLP", "...PLLRRCLLLLLLP",
    "...PLLRCLLLLLLVP", "..PLLRRLLLLLCVVP", "..PLLRCLLLLCVVVP", "..PLLRLLLLCVVVVP",
    "..PLLKLLLLVVVVLP", "..PLLDCLLLVVVMLP", "..PLLGCLLLLCCLLP", "...PPPPPPPPPPPPP",
};

static const char *const ic_grafico[] = {
    "................", ".......PPP......", "....PP.PMMP.....", "...PMP.PMMMP....",
    "..PMMP.PMMMMP...", ".PMMMP.PMMMMP...", ".PMMMP.PPPPPPPP.", ".PMMMP.......PP.",
    ".PMMMMPPPPPP.PP.", ".PMMMMMMMMTP.PP.", "..PMMMMTVMTP.PP.", "...PMMMTVPTP.PP.",
    "....PPPTP.PP.PP.", ".......PP.PP.PP.", ".......PP.PP.PP.", "................",
};

static const char *const ic_tendencia[] = {
    "................", "................", "..P.............", "..PP............",
    "..PP......PPPP..", "..PP......PMMP..", "..PP..PP..PMMP..", "..PP.PVMPPMPPP..",
    "..PP.PPPMMP.....", "..PP....PP......", "..PP............", "..PP............",
    "..PCPPPPPPPPP...", "...PPPPPPPPPPP..", "................", "................",
};

static const char *const ic_documento[] = {
    "................", "...PPPPP........", "...PTTTTP.......", "...PTTTTLP......",
    "...PTTTTLLP.....", "...PTTTTLLLP....", "...PTTTTTTTTP...", "...PTTTTTTTTP...",
    "...PTTTTTTTTP...", "...PTTTTTTTTP...", "...PTTTTTTTTP...", "...PTTTTTTTTP...",
    "...PTTTVVVVVP...", "...PVVVVVVVVP...", "...PPPPPPPPPP...", "................",
};

static const char *const ic_documento_texto[] = {
    "................", "...PPPPP........", "...PTTTTP.......", "...PTTTTLP......",
    "...PTTTTLLP.....", "...PTTTTLLLP....", "...PTTTTTTTTP...", "...PTTTTTTTTP...",
    "...PTLLLLLLTP...", "...PTTTTTTTTP...", "...PTLLLLLLTP...", "...PTTTTTTTTP...",
    "...PTLLLLLLVP...", "...PVVVVVVVVP...", "...PPPPPPPPPP...", "................",
};

static const char *const ic_documento_pasta[] = {
    "................", "................", "...PPPPPPPP.....", "...PTTTTTTP.....",
    "...PTTTTTTTPP...", "..PAAACTTTTMP...", "..PAAAACTTTMP...", "..PAAAAACTTMP...",
    "..PAAAAAACTMP...", "..PAAAAAAACMP...", "..PAAAAAAAAAAP..", "..PAAAAAAAAAAP..",
    "..PAAAAAAAAAAP..", "...PPPPPPPPPP...", "................", "................",
};

static const char *const ic_documento_editar[] = {
    "................", "...PPPPP........", "...PTTTTP.......", "...PTTTTLP......",
    "...PTTTTLLP.....", "...PTTTTLLLP....", "...PTTTTTTTTP...", "...PTTTTTTTTP...",
    "...PTTTTTTTTMPP.", "...PTTTTTTTCMMP.", "...PTTTTTTCAORP.", "...PTTTTTCOOOP..",
    "...PTTTVCOOOP...", "...PVVVCOOOP....", "...PPPPYAPP.....", ".......PP.......",
};

static const char *const ic_documento_novo[] = {
    "................", "....PPPPP.......", "....PTTTTP......", "....PTTTTLP.....",
    "....PTTTTLLP....", "....PTTTTLLLP...", "...PGGGTTTTTTP..", "..PGGGGGGTTTTP..",
    ".PGGGLGGGTTTTP..", ".PGGGBGGGTTTTP..", ".PGLBBBLGGTTTP..", ".PGGGLGGGTTTTP..",
    ".PGGGLGGGVVVVP..", "..PGGGGGVVVVVP..", "...PPPPPPPPPPP..", "................",
};

static const char *const ic_documento_bloqueado[] = {
    "................", "....PPPPP.......", "....PTTTTP......", "....PTTTTLP.....",
    "....PTTTTLLP....", "....PTTTTLLLP...", "....PAYTTTTTTP..", "....PTYCTTTTTP..",
    "...PCTCCTTTTTP..", "..PAAAAAOTTTTP..", "..PAAAAOOTTTTP..", "..PAAKOOOTTTTP..",
    "..PAOKOOOVVVVP..", "..PAAOOOOVVVVP..", "..PPPPPPPPPPPP..", "................",
};

static const char *const ic_notas[] = {
    "................", "...........PPP..", ".PPPPPPPPPPMMMP.", "..PTTTTTTYMMMMP.",
    "..PTTTTCYYRMMMP.", ".PPPPPTAAAARMP..", "......PAOOOOP...", ".PPPPPOOOOOP....",
    "..PTOOOOOOP.....", "...POOOOOP......", "..PAOOOOP.......", "..PYOOOP........",
    ".PYYAOP.........", ".PYAPP..........", ".PPP............", "................",
};

static const char *const ic_biblioteca[] = {
    "................", "................", ".PPP.PPP........", ".PTP.PTP........",
    ".PLP.PLP.PPP....", ".PLP.PLP.PTP....", ".PTP.PTP.PTTP...", ".PTP.PTP..PTP...",
    ".PTP.PTP..PTP...", ".PTP.PTP..PTTP..", ".PTP.PTP...PTP..", ".PTP.PTP...PTTP.",
    ".PTP.PVP....PVP.", ".PPP.PPP....PP..", "................", "................",
};

static const char *const ic_livro[] = {
    "................", "....PPPPPPPP....", "...PTTTTTTTTP...", "...PTCLLLLTTP...",
    "...PTLLLLLLTP...", "...PTCLLLLTTP...", "...PTTTTTTTTP...", "...PTTTTTTVVP...",
    "...PVVVVVVVVP...", "...PVVVVVVVVP...", "...PVVVVVVVVP...", "...PVVVVVVVVP...",
    "...PVVVVVVVVP...", "...PTLLLLLLLP...", "....PPPPPPPPP...", "................",
};

static const char *const ic_caderno[] = {
    "................", "...PPPPPPPPP....", "..PMMMMMMMMMP...", "..PMLLLLLLLMP...",
    "..PMLLLLLLLMMPP.", "..PMMMMMMMMMMPP.", "..PMMMMMMMMMP...", "..PMMMMMMMMMMPP.",
    "..PMMMMMMMMMMPP.", "..PMMMMMMMMMP...", "..PMMMMMMMMMMPP.", "..PMMMMMMMMMMPP.",
    "..PMMMMMMMMMP...", "..PMMMMMMMMMP...", "...PPPPPPPPP....", "................",
};

static const char *const ic_lista[] = {
    "................", "................", "...PPPPPPPPPP...", "..PTTTTTTTVVVP..",
    "..PTLTTCCCCCVP..", "..PTLTTCCCCCVP..", "..PTTTTVVVVVVP..", "..PTLTTCCCCCVP..",
    "..PTLTTCCCCMVP..", "..PVVVVVVVVVVP..", "..PVLTVCCCCMVP..", "..PVLTVCCCMMVP..",
    "..PVVVVVVVVVVP..", "...PPPPPPPPPP...", "................", "................",
};

static const char *const ic_texto[] = {
    "................", "................", "....PP..........", "....PP..........",
    "....PP..........", "...P..P....PP...", "...PPPP...PMMP..", "...P..P..PAMMP..",
    "..P....PPOAOP...", "..P....POOOP....", "......POOOP.....", ".....PYOOP......",
    ".....PYOP.......", ".....PPP........", "................", "................",
};

static const char *const ic_tabela[] = {
    "................", "................", "..PPPPPPPPPPPP..", "..PGGCGGGGCGGP..",
    "..PGGCGGGGCGGP..", "..PCCCCCCCCCCP..", "..PGGCGGGGCGGP..", "..PGGCGGGGCGDP..",
    "..PGGCGGGGCDDP..", "..PGGCGGGGCDDP..", "..PCCCCCCCCCCP..", "..PGGCGGDDCDDP..",
    "..PGGCGDDDCDDP..", "..PPPPPPPPPPPP..", "................", "................",
};

static const char *const ic_base[] = {
    "................", ".....PPPPPP.....", "...PPTTTTTTPP...", "...PTTTTTTTTP...",
    "...PTTTTTTTTP...", "...PTTTTTTTTP...", "...PTTTTTTTTP...", "...PTTTTTTTTP...",
    "...PTTTTTTVVP...", "...PTTTTVVVVP...", "...PTTVVVVVVP...", "...PVVVVVVVVP...",
    "...PVVVVVVVVP...", "...PPVVVMMMPP...", ".....PPPPPP.....", "................",
};

static const char *const ic_correio[] = {
    "................", "................", "................", "...PPPPPPPPPP...",
    "..PTTTTTTTTTTP..", "..PTTTTTTTTTTP..", "..PTTTTTTTTTTP..", "..PVVTTTTTTVVP..",
    "..PTVVVTTVVVTP..", "..PTTTVVVVTTTP..", "..PTTTTVVVVVVP..", "..PVVVVVVVVVVP..",
    "...PPPPPPPPPP...", "................", "................", "................",
};

static const char *const ic_correio_alerta[] = {
    "................", ".........PPP....", "........PAAAP...", "........PAAAP...",
    ".......PAAAAOP..", "..PPPPPCAAAOOP..", ".PTTTTTAAAOOOP..", ".PTTTTAAAOOOOPP.",
    ".PTTTTTTCOOOP...", ".PVVTTTTTAAAP...", ".PTVVVTTVVVTP...", ".PTTTVVVVTTTP...",
    ".PTTTTVVVVVVP...", ".PVVVVVVVVVVP...", "..PPPPPPPPPP....", "................",
};

static const char *const ic_correio_muitos[] = {
    "................", "................", "..PPPPPPPPPP....", ".PTTTTTTTTTTP...",
    ".PTTTTTTTTTTTP..", ".PTTTTTTTTTTVVP.", ".PVVTTTTTTVVVVP.", ".PTVVVTTVVVTTVP.",
    ".PTTTVVVVVVVVVP.", ".PTTVVVVVVVVVVP.", ".PVVVVVVVVVVVVP.", "..PVVVVVVMMMVVP.",
    "...PVVVVVVVVVP..", "....PPPPPPPPP...", "................", "................",
};

static const char *const ic_conversa[] = {
    "................", "................", "......PPPP......", "....PPTTTTPP....",
    "...PTTTTTVVVP...", "...PTTTTVVVVP...", "..PTTTCLLCVVVP..", "..PTVVCCCCVVVP..",
    "..PVVVCCCVVVVP..", "..PVVVCCMVVVMP..", "...PVVVVVVMMP...", "...PVVVVMMMMP...",
    "..PVPPVMMMPP....", "..PP..PPPP......", "................", "................",
};

static const char *const ic_conversas[] = {
    "................", "................", ".....PPPP.......", "...PPTTTVPP.....",
    "...PTTTVVVP.....", "..PTTTVVVVVPP...", "..PTTVVVVVVVP...", "..PTVVVVVVVVMP..",
    "..PVVVVVVVVVMP..", "..PVVVVVVVVVMP..", "..PVVVVVVVVMMP..", "..PPPVVVVVMMMP..",
    ".....PPMMMMMMP..", ".......PPPPPPP..", "................", "................",
};

static const char *const ic_telefone[] = {
    "................", ".....PPPPPP.....", "....PVVVVVVP....", "....PVVVVVVP....",
    "....PVVVVVVP....", "....PVVVVVVP....", "....PVVVVVVP....", "....PVVVVVVP....",
    "....PVVVVVVP....", "....PVVVVVVP....", "....PVVVVVVP....", "....PVVVVVVP....",
    "....PVMLLMVP....", "....PVVVVVVP....", ".....PPPPPP.....", "................",
};

static const char *const ic_globo[] = {
    "................", "................", ".........PPPPPP.", ".......PPTTTTTTP",
    ".....PPTTTTTTTTP", "....PTTTTTTTTTTP", "....PTTTTTTTTTTP", "...PTTTTTTTTTTTP",
    "...PTTTTTTTTTTTP", "..PTTTTTTTTTTTTP", "..PTTTTTTTTTTVVP", "..PTTTTTTTTTVVVP",
    "..PTTTTTTTTVVVVP", "..PTTTTTTTVVVVVP", "..PTTTTTTVVVVVVP", "...PPPPPPPPPPPPP",
};

static const char *const ic_globo_escudo[] = {
    "................", "................", ".........PPPPPP.", ".......PPTTTTTTP",
    ".....PPTTTTTTTTP", "....PTTTTTTTTTTP", "....PTTTTTTTTTTP", "...PTTTTTTTTTTTP",
    "...PTTTTTTTTTTTP", "..PTTTTTTTTTTTTP", "..PTTTTTTTTTTVVP", "..PTTTTTTTTTVVVP",
    "..PTTTTTTTTVVVVP", "..PTTTTTTTVVVGGP", "..PTTTTTTVVVVGGP", "...PPPPPPPPPPPPP",
};

static const char *const ic_wifi[] = {
    "................", "................", "................", "................",
    "................", "........PPPPPPPP", "......PPPPPPPPPP", "....PPMP........",
    "...PMPP...PPPP..", "..PMP...PPMPPMPP", "..PP...PMPP..PPP", "......PPP.......",
    ".....PP...PPPP..", ".....PP..PMPPMPP", "........PPP..PPP", ".......PP......P",
};

static const char *const ic_wifi_aviso[] = {
    "................", "................", "................", "................",
    "................", "........PPPPPPPP", "......PPPPPPPPPP", "....PPMP........",
    "...PMPP...PPPP..", "..PMP...PPMPPMPP", "..PP...PMPP..PPP", "......PPP.......",
    ".....PP...PPPP..", ".....PP..PMPPMPP", "........PPP..PPP", ".......PP......P",
};

static const char *const ic_local[] = {
    "................", "......PPPP......", "....PPRRRRPP....", "....PRRRRRRP....",
    "...PRRRCCRRRP...", "...PRRCBBMRRP...", "...PRRCBLMRRP...", "...PRRRMMRRRP...",
    "....PRRRRRRP....", "....PRRRRRRP....", "....PMRRRRMP....", "..PPLLRRRRLLPP..",
    "..PLLLLMMLLLLP..", "..PPLLLLLLLLPP..", "....PPPPPPPP....", "................",
};

static const char *const ic_alfinete[] = {
    "................", "................", "........PP......", ".......PTTP.....",
    ".......PTTTP....", "......PTTTTTP...", "......PTTTTTTP..", "...PPPTTTTTTTP..",
    "...PTTTTTTTPP...", "....PTTTTPP.....", "....PTTTP.......", "...PVPPTP.......",
    "..PMP..PP.......", "..PP............", "................", "................",
};

static const char *const ic_ligacao[] = {
    "................", "................", "................", "................",
    "................", "................", "................", ".....PPPPP....PP",
    "...PPPPPPP....PP", "...PP...........", "..PP............", "..PP..PPPPPPPPPP",
    "..PP..PPPPPPPPPP", "..PP............", "...PP...........", "...PPPPPPP....PP",
};

static const char *const ic_enviar[] = {
    "................", ".PP.............", ".PTPP...........", "..PTTPP.........",
    "..PTTTTPP.......", "..PTTTTTTPP.....", "..PTTTTTTTTPP...", "...PVVVVTTTTTPP.",
    "...PVVVVTTTTTPP.", "..PTTTTTTTMPP...", "..PTTTTTMPP.....", "..PTTMMPP.......",
    "..PMMPP.........", ".PMPP...........", ".PP.............", "................",
};

static const char *const ic_codigo[] = {
    "................", "................", "................", ".........P......",
    "........PP......", "...PP...PP.PP...", "..PP...PP...PP..", ".PP....PP....PP.",
    ".PP....P.....PP.", "..PP..PP....PP..", "...PP.P....PP...", ".....PP.........",
    ".....P..........", "................", "................", "................",
};

static const char *const ic_cartao_contato[] = {
    "................", "................", "................", "..PPPPPPPPPPPP..",
    ".PLLLLLLLLLLLLP.", ".PLLMMCLLLLLLLP.", ".PLLMMMLLVVVVLP.", ".PLLMMCLLLLLLLP.",
    ".PLMMMMCLLLLLLP.", ".PLMMMMMLVVVVLP.", ".PLMMMMLLLLLLLP.", ".PLLLLLLLLLLLLP.",
    "..PPPPPPPPPPPP..", "................", "................", "................",
};

static const char *const ic_camera[] = {
    "................", "................", "......PPPP......", ".....PVVVVP.....",
    "...PPVVVVVVPP...", "..PVVVCLLMVMMP..", "..PVVMCVVCMMMP..", "..PVVLVVVVLVVP..",
    "..PVVLVVVVLVVP..", "..PVVMCVVMMVVP..", "..PVVVMLLMVVVP..", "..PVVVVVVVVVVP..",
    "...PPPPPPPPPP...", "................", "................", "................",
};

static const char *const ic_video[] = {
    "................", "................", "................", "..PPPPPPP.......",
    ".PVVVVVVVP...PP.", ".PVVVVVVVP.PPVP.", ".PVVVVVVVVPVVVP.", ".PVVVVVVVVVVVVP.",
    ".PVVVVVVVVVVVVP.", ".PVMLMVLVVPVVVP.", ".PVVVVVMVP.PPVP.", ".PVVVVVVVP...PP.",
    "..PPPPPPP.......", "................", "................", "................",
};

static const char *const ic_imagem[] = {
    "................", "................", "...PPPPPPPPPP...", "..PVVVVVVVVVVP..",
    "..PVVVVVVVCVVP..", "..PVVVVVVCLCVP..", "..PVVVVVVVCVVP..", "..PVVVVVVVVVVP..",
    "..PVVVVLLVVVVP..", "..PVVVLLLLVVVP..", "..PVVLLLLLLVVP..", "..PVLLLLLLLLVP..",
    "..PLLLLLLLLLCP..", "...PPPPPPPPPP...", "................", "................",
};

static const char *const ic_microfone[] = {
    "................", "................", "......PPPP......", "......PVVP......",
    ".....PVVVVP.....", ".....PVVVVP.....", ".....PVMMMP.....", "...P.PMMMMP.P...",
    "...PP.PMMP.PP...", "...PP.PPPP.PP...", "....PP....PP....", "....PPPPPPPP....",
    "......PCCP......", ".......PP.......", ".......PP.......", "................",
};

static const char *const ic_fones[] = {
    "................", "................", ".........PPPPPP.", ".......PPPPPPPPP",
    ".....PPPP......P", "....PCP.........", "....PP..........", "...PP...........",
    "...PP...........", "..PP............", "..PP............", "..PP............",
    "..PP............", "..PP............", "..PVPPPPP......P", "..PPPPPPP......P",
};

static const char *const ic_sol[] = {
    "................", "................", "...........PP...", "...........PP...",
    ".....P.....PP...", "....PYP.........", ".....PYP...PP...", "......P..PPYYPP.",
    "........PYYYYYAP", ".......PYYYYAAAP", ".......PYYAAAAAP", "......PYAAAAAOOP",
    "......PAAAAOOOOP", "..PPPPTTTTTTTTTP", "..PPPPPPPPPPPPPP", "................",
};

static const char *const ic_neve[] = {
    "................", "................", "...........PP...", "...........PP...",
    "........PP.PP.PP", "........PTPTTPTP", ".........PTTTTP.", "..........PTTP..",
    "....PP.....PP...", "....PTP....PP...", ".....PTP...PP...", "..PPPTTTPPPTTPPP",
    "..PPPTTTPPPTTPPP", ".....PTP...PP...", "....PTP....PP...", "....PP.....PP...",
};

static const char *const ic_jogo[] = {
    "................", "......PPPP......", "....PPTTTTPP....", "...PTTTTTVVVP...",
    "...PTTTTVVVVP...", "..PTTTCLLCVVVP..", "..PTVVCCCCVVVP..", "..PVVVCCCVVVVP..",
    "..PVVVCCMVVVMP..", "...PVVVVVVMMP...", "...PVVVVMCCMMPPP", "..PVPPVMCCCCCCCP",
    "..PP..PCCCCCDCCP", ".......PCCCCDCCP", "......PCCCCCCCCP", "......PPPPPPPPPP",
};

static const char *const ic_desporto[] = {
    "................", ".........PP.....", "........POKP....", "......PPOOKOP...",
    ".....POOOOOKP...", "....POOOOOOOKP..", ".PPPOOOOOKOOOOP.", ".POOOOOOOOOOOOP.",
    ".POOOOOOOOOOOP..", ".POOAAOOOOOPP...", ".POOAYAOOOP.....", "..POOAAOOP......",
    "..POOOOOOP......", "...PPOOOOP......", ".....PPPPP......", "................",
};

static const char *const ic_recibo[] = {
    "................", "................", "..PPPPPPPPP.....", "..PYYYYYYYP.....",
    "..PYYYYYYYP.....", "..PYAOOOAYP.....", "..PYYYYYYYP.....", "..PYAAAOAYP.....",
    "..PYAAOOAAP.....", "..PYAAAAAAP.....", "..PAAOOAAAAPPP..", "..PAAAAAAAAOOP..",
    "..PAAAAAOOOOOP..", "...PPPPPPPPPP...", "................", "................",
};

static const char *const ic_poupanca[] = {
    "................", "................", "....PPP..PPPP...", "....PRRPPAAAAP..",
    "....PRRRAAAAAP..", "...PRRRRAAAAAP..", "..PRDRRRRROOORP.", ".PRRRRRRRRRRRKP.",
    ".PRRRRRRRRRRRRP.", ".PRRRRRRRRRRRRP.", "..PRRRRRRRRRRRP.", "...PRRRRRRRRRP..",
    "....PRRRRRRRP...", ".....PRRPPRRP...", ".....PPP..PPP...", "................",
};

static const char *const ic_formulario[] = {
    "................", "................", "................", "....PPPPPPPPPPPP",
    "...PLLLLLLLLLLLP", "...PLLLLLLLLLLLP", "...PLLTVVVVVVVVP", "...PLLTTTTTTTTTP",
    "...PLLLLLLLLLLLP", "...PLLTVVTLLLLLP", "...PLLTVVVLLVVVP", "...PLLTVVVLLTTTP",
    "...PLLTVVTLLLLLP", "...PLLLLLLLLLLLP", "...PLLTVVTLLLLLP", "...PPPPPPPPPPPPP",
};

static const char *const ic_escudo[] = {
    "................", "................", ".......PP.......", "......PMMP......",
    "...PPPMMMMPPP...", "...PMMMMMMMMP...", "...PMMMMMMMMP...", "...PMMMMMMMMP...",
    "...PMMMMMMMMP...", "...PMMMMMMMMP...", "....PMMMMMMP....", "....PMMMMMMP....",
    ".....PPMMPP.....", ".......PP.......", "................", "................",
};

static const char *const ic_escudo_verificado[] = {
    "................", "................", ".......PP.......", "......PMMP......",
    "...PPPMMMMPPP...", "...PMMMMMMMMP...", "...PMMMMCGGGP...", "...PMMMCGGGGGP..",
    "...PMMCGGGGGCGP.", "...PMMGGGGGLLGP.", "....PMGGCGLCGGP.", "....PMGGLLCGGGP.",
    ".....PPGGCGGGGP.", ".......PGGGGGP..", "........PPPPP...", "................",
};

static const char *const ic_cadeado[] = {
    "................", "......PPPP......", ".....P....P.....", ".....P....P.....",
    "...PPAPPPPAPP...", "..PAAAAAAAAAAP..", "..PAAAAAAAAAAP..", "..PAAAAAAAAAOP..",
    "..PAAAAOOAAOOP..", "..PAAAAOOAOOOP..", "..PAAAAAAOOOOP..", "..PAAAAAOOOOOP..",
    "..PAAAAOOOOOOP..", "...PPPPPPPPPP...", "................", "................",
};

static const char *const ic_alerta[] = {
    "................", "................", "......PPPP......", ".....PAAAAP.....",
    "....PAAAAAAP....", "....PAAAAAAP....", "...PAAAAAAOOP...", "...PAAAAAOOOP...",
    "...PAAAAOOOOP...", "...PAAAOOOOOP...", "...PAAOOOOOOP...", "...PPPOOOOPPP...",
    "......POOP......", ".......PP.......", "................", "................",
};

static const char *const ic_aviso[] = {
    "................", "................", ".......PP.......", "......PAAP......",
    "......PAAP......", ".....PAKKAP.....", ".....PAKKAP.....", "....PAAKKAAP....",
    "....PAAOOAOP....", "...PAAAOOOOOP...", "..PAAAOKKOOOOP..", "..PAOOOOOOOOOP..",
    "..PPPPPPPPPPPP..", "................", "................", "................",
};

static const char *const ic_erro[] = {
    "................", "................", "......PPPP......", "....PPAAAAPP....",
    "...PAAAAAAAAP...", "...PAAAKKAAAP...", "..PAAAAKKAAAAP..", "..PAAAAKKAAAAP..",
    "..PAAAAKKAAAOP..", "..PAAAAAAAOOOP..", "...PAAAKKOOOP...", "...PAAOOOOOOP...",
    "....PPOOOOPP....", "......PPPP......", "................", "................",
};

static const char *const ic_duvida[] = {
    "................", "................", "......PPPP......", "....PPTTTTPP....",
    "...PTTTCCTVVP...", "...PTTLLLLVVP...", "..PTTTLVVBVVVP..", "..PTTTVVCLVVVP..",
    "..PTTVVTLVVVVP..", "..PTVVVTTVVVVP..", "...PVVVTTVVVP...", "...PVVVLLVVVP...",
    "....PPVVVVPP....", "......PPPP......", "................", "................",
};

static const char *const ic_certo[] = {
    "................", "................", "......PPPP......", "....PPGGGGPP....",
    "...PGGGGGGGGP...", "...PGGGGGGGGP...", "..PGGGGGGGLGGP..", "..PGGCGGGLGGGP..",
    "..PGGLLGLGGGGP..", "..PGGGCLGGGGGP..", "...PGGGGGGGGP...", "...PGGGGGGGGP...",
    "....PPGGGGPP....", "......PPPP......", "................", "................",
};

static const char *const ic_fechar[] = {
    "................", "................", "......PPPP......", "....PPRRRRPP....",
    "...PRRRRRRRRP...", "...PRRRRRRRRP...", "..PRRRLRRLRRRP..", "..PRRRRLLRRRRP..",
    "..PRRRRLLRRRRP..", "..PRRRLRRLRRRP..", "...PRRRRRRRRP...", "...PRRRRRRRRP...",
    "....PPRRRRPP....", "......PPPP......", "................", "................",
};

static const char *const ic_pasta[] = {
    "................", "................", "..PPPPP.........", ".PCCCCCP........",
    ".PCCCCP..PPPPP..", ".PPPPP..PCCCCCP.", ".......PCCCCCCP.", ".PPPPPPCCCCCCCP.",
    ".PCCCCCCCCCCCCP.", ".PCCCCCCCCCCCCP.", ".PCCCCCCCCCCCCP.", ".PCCCCCCCCCCCCP.",
    ".PCCCCCCCCCCCCP.", "..PPPPPPPPPPPP..", "................", "................",
};

static const char *const ic_guardar[] = {
    "................", "................", "...PP.PPP.PP....", "..PVP.PVP.PVP...",
    "..PVP.PPP.PVVP..", "..PVVP...PVVVP..", "..PVVVPPPVVVVP..", "..PVVPPPPPPVVP..",
    "..PVP......PVP..", "..PP.PPPPPP.PP..", "..PP.PVVVVP.PP..", "..PP.PVVVVP.PP..",
    "..PP.PVVVVP.PP..", "...P.PPPPPP.P...", "................", "................",
};

static const char *const ic_abrir[] = {
    "................", "................", "...PPPP.PPPPPP..", "..PVPP...PVVVP..",
    "..PP......PVVP..", "..PP.....PVPVP..", "..PP....PVP.PP..", "..PP....PP...P..",
    "..PP............", "..PP.........P..", "..PP........PP..", "..PP........PP..",
    "..PVPPPPPPPPVP..", "...PPPPPPPPPP...", "................", "................",
};

static const char *const ic_procurar[] = {
    "................", "................", ".....PPPPP......", "....PPPPPPP.....",
    "...PP.....PP....", "..PP.......PP...", "..PP.......PP...", "..PP.......PP...",
    "..PP.......PP...", "..PP.......PP...", "...PP.....PP....", "....PPPPPPPCP...",
    ".....PPPPP.PCP..", "............PCP.", ".............PP.", "................",
};

static const char *const ic_tocar[] = {
    "................", "................", "....PPP.........", "....PGGPP.......",
    "....PGGGGP......", "....PGGGGGPP....", "....PGGGGGGGPP..", "....PGGGGGGGGGP.",
    "....PGGGGGGGGGP.", "....PGGGGGGGPP..", "....PGGGGGPP....", "....PGGGGP......",
    "....PGGPP.......", "....PPP.........", "................", "................",
};

static const char *const ic_pausar[] = {
    "................", "................", "...PPP....PPP...", "..PGGGP..PGGGP..",
    "..PGGGP..PGGGP..", "..PGGGP..PGGGP..", "..PGGGP..PGGGP..", "..PGGGP..PGGGP..",
    "..PGGGP..PGGGP..", "..PGGGP..PGGGP..", "..PGGGP..PGGGP..", "..PGGGP..PGGGP..",
    "..PGGGP..PGGGP..", "...PPP....PPP...", "................", "................",
};

static const char *const ic_copiar[] = {
    "................", ".......PPPPPPP..", "......PCCCCCCCP.", "......PCCCCCCCP.",
    "......PCCCCCCCP.", "..PPP.PCCCCCCCP.", ".PCCP.PCCCCCCCP.", ".PCCP.PCCCCCCCP.",
    ".PCCP.PCCCCCCCP.", ".PCCP.PCCCCCCCP.", ".PCCCP.PPPPPPP..", ".PCCCCP.........",
    ".PCCCCCPPP......", ".PCCCCCCCP......", "..PPPPPPP.......", "................",
};

static const char *const ic_colar[] = {
    "................", ".....PPPPPP.....", "...PPP....PPP...", "...PCCPPPPCCP...",
    "...PCCCCCCCCP...", "...PCCCCCCCCP...", "...PCCCCCCCCP...", "...PCCCCCCCCP...",
    "...PCCCCCCCCP...", "...PCCCCCCCCP...", "...PCCCCCCCCP...", "...PCCCCCCCCP...",
    "...PCCCCCCCCP...", "...PCCCCCCCCP...", "...PPPPPPPPPP...", "................",
};

static const char *const ic_excluir[] = {
    "................", ".......PP.......", "......P..P......", "..PPPPRPPRPPPP..",
    "...PRRRRRRRRP...", "...PRRPRRPRRP...", "...PRP.PP.PRP...", "...PRP.PP.PRP...",
    "...PRP.PP.PRP...", "...PRP.PP.PRP...", "...PRP.PP.PRP...", "....PP.PP.PP....",
    "....PRPRRPRP....", "....PRRRRRRP....", ".....PPPPPP.....", "................",
};

static const char *const ic_editar[] = {
    "................", "...........PPP..", "..........PCCCP.", "...........PCCP.",
    "........PP..PCP.", ".......PCCP..P..", "......PCCCCP....", ".....PCCCCCP....",
    "....PCCCCCP.....", "...PCCCCCP......", "..PCCCCCP.......", "..PCCCCP........",
    ".PCCCCP.........", ".PCCPP..........", ".PPP............", "................",
};

static const char *const ic_voltar[] = {
    "................", "................", "................", "......PP........",
    ".....PPP........", "....PP..........", "...PP...........", "..PCCPPPPPPPPP..",
    "..PCCPPPPPPPPP..", "...PP...........", "....PP..........", ".....PPP........",
    "......PP........", "................", "................", "................",
};

static const char *const ic_avancar[] = {
    "................", "................", "................", "........PP......",
    "........PPP.....", "..........PP....", "...........PP...", "..PPPPPPPPPCCP..",
    "..PPPPPPPPPCCP..", "...........PP...", "..........PP....", "........PPP.....",
    "........PP......", "................", "................", "................",
};

static const char *const ic_subir[] = {
    "................", "................", ".......PP.......", "......PCCP......",
    ".....PPCCPP.....", "....PP.PP.PP....", "...PP..PP..PP...", "...PP..PP..PP...",
    ".......PP.......", ".......PP.......", ".......PP.......", ".......PP.......",
    ".......PP.......", ".......PP.......", "................", "................",
};

static const char *const ic_descer[] = {
    "................", "................", ".......PP.......", ".......PP.......",
    ".......PP.......", ".......PP.......", ".......PP.......", ".......PP.......",
    "...PP..PP..PP...", "...PP..PP..PP...", "....PP.PP.PP....", ".....PPCCPP.....",
    "......PCCP......", ".......PP.......", "................", "................",
};

static const char *const ic_janela[] = {
    "................", "................", "...PPPPPPPPPP...", "..PCCCCCCCCCCP..",
    "..PCCCCCCCCCCP..", "..PPPPPPPPPPPP..", "..P..........P..", "..P..........P..",
    "..P..........P..", "..P..........P..", "..P..........P..", "..P..........P..",
    "..PP........PP..", "...PPPPPPPPPP...", "................", "................",
};

static const char *const ic_disco[] = {
    "................", "................", "................", "................",
    "..PPPPPPPPPPPP..", ".PCCCCCCPPCPPCP.", ".PCCCCCP..P..PP.", ".PCCCCCP..P..PP.",
    ".PCCCCCCPPCPPCP.", ".PCCCCCCCCCCCCP.", "..PPPPPPPPPPPP..", "................",
    "................", "................", "................", "................",
};

static const char *const ic_imprimir[] = {
    "................", "................", "....PPPPPPPP....", "....P......P....",
    "..PPCPPPPPPCPP..", ".PCCCCCCCCCCCCP.", ".PCCCCCCCCCCCCP.", ".PCCCCCCCCCCCCP.",
    ".PCCCCCCCCCCCCP.", ".PCCCPPPPPPCCCP.", ".PCCP......PCCP.", ".PPPP......PPPP.",
    "....P......P....", "....PPPPPPPP....", "................", "................",
};

static const char *const ic_olho[] = {
    "................", "................", "................", "................",
    ".....PPPPPP.....", "....PP....PP....", "...P........P...", "..PP..PPPP..PP..",
    "..P...PCCP...P..", ".....PCCCCP.....", "......PCCP......", "......PPPP......",
    "................", "................", "................", "................",
};

static const char *const ic_terminal[] = {
    "................", "..PPPPPPPPPP....", ".PCCCCCCCCCCP...", ".PCCCCCCCCCCP...",
    ".PPPPPPPPPPPP...", ".P..............", ".P...P..P..PP...", ".P..PP....PP....",
    ".P..P....PCP....", ".P..PP...PCP...P", ".P...P...PCCPPPP", ".PP.....PCCCCCP.",
    "..PPPP.PCCCPPP..", "......PCCCP.....", "......PCCP......", "......PPP.......",
};

static const char *const ic_som[] = {
    "................", "................", "........P.......", ".......PP.......",
    "......PVP.......", "..PPPPVVP.......", "..PVVVVVP.......", "..PVVVVVP.......",
    "..PVVVVVP.......", "..PVVVVVP.......", "..PPPPVVP.......", "......PVP.......",
    ".......PP.......", "........P.......", "................", "................",
};

static const char *const ic_musica[] = {
    "................", "...........PP...", "........PPPMP...", ".....PPPMMMMP...",
    ".....PMMMMMPP...", ".....PMMPPP.P...", ".....PPP....P...", ".....P......P...",
    ".....P......P...", ".....P......P...", ".....P....PPP...", "...PPP...PMMP...",
    "..PMMP...PMMP...", "..PMMP....PP....", "...PP...........", "................",
};

static const char *const ic_informacao[] = {
    "................", ".....PPPPPP.....", "....PVVVVVVP....", "...PVVVVVVVVP...",
    "..PVVVVPPVVVVP..", ".PVVVVP..PVVVVP.", ".PVVVVVPPVVVVVP.", ".PVVVVVVVVVVVVP.",
    ".PVVVVVVVVVVVVP.", ".PVVVVVVVVVVVVP.", ".PVVVVVVVVVVVVP.", "..PVVVVVVVVVVP..",
    "...PVVVVVVVVP...", "....PVVVVVVP....", ".....PPPPPP.....", "................",
};

static const char *const ic_ajuda[] = {
    "................", ".....PPPPPP.....", "....PVVVVVVP....", "...PVVVPPVVVP...",
    "..PVVVP..PVVVP..", ".PVVVP.PP.PVVVP.", ".PVVVVPVP.PVVVP.", ".PVVVVVP.PVVVVP.",
    ".PVVVVVP.PVVVVP.", ".PVVVVVVPVVVVVP.", ".PVVVVVPPVVVVVP.", "..PVVVP..PVVVP..",
    "...PVVVPPVVVP...", "....PVVVVVVP....", ".....PPPPPP.....", "................",
};

static const char *const ic_bateria[] = {
    "................", "................", "................", "................",
    "...PPPPPPPPPP...", "..PGPPPPPPPPP...", "..PP........PP..", "..PP........PGP.",
    "..PP........PGP.", "..PP........PP..", "..PGPPPPPPPPP...", "...PPPPPPPPPP...",
    "................", "................", "................", "................",
};

static const char *const ic_ambiente[] = {
    "................", "................", "..PPPPPPPPPPPP..", "..PCCCCCCCCCCP..",
    "..PCCCCCCCCCCP..", "..PCCCCCCCCCCP..", "..PCCCCCCCCCCP..", "..PCCCCCCCCCCP..",
    "..PCCCCCCCCCCP..", "..PCCCCCCCCCCP..", "..PCCCCCCCCCCP..", "..PPPPCPPCPPPP..",
    "......P..P......", "....PPPPPPPP....", "................", "................",
};

static const char *const ic_desenhar[] = {
    "................", "...PPPPP.P.PP...", "...PMMMP.P.PP...", "...PMMMP.P.PP...",
    "...PMMMMPP.PP...", "...PMMMMMMPMP...", "...PPPPPPPPPP...", "................",
    "...PPPPPPPPPP...", "...PMMMMMMMMP...", "....PPMMMMPP....", "......PMMP......",
    "......PMMP......", "......PMMP......", ".......PP.......", "................",
};

/* La tabla: nombre del icono -> dibujo. */
static const icone_t tabela[] = {
    { "inicio", ic_inicio },
    { "casa", ic_casa },
    { "edificio", ic_edificio },
    { "aplicativos", ic_aplicativos },
    { "quadro", ic_quadro },
    { "ajustes", ic_ajustes },
    { "pessoas", ic_pessoas },
    { "grupo", ic_grupo },
    { "pessoa", ic_pessoa },
    { "pessoa-nova", ic_pessoa_nova },
    { "calendario", ic_calendario },
    { "relogio", ic_relogio },
    { "despertador", ic_despertador },
    { "coracao", ic_coracao },
    { "estrela", ic_estrela },
    { "luz", ic_luz },
    { "presente", ic_presente },
    { "maleta", ic_maleta },
    { "ferramentas", ic_ferramentas },
    { "chave-inglesa", ic_chave_inglesa },
    { "bandeira", ic_bandeira },
    { "marcador", ic_marcador },
    { "nuvem", ic_nuvem },
    { "nuvem-erro", ic_nuvem_erro },
    { "tarefa", ic_tarefa },
    { "prancheta", ic_prancheta },
    { "cofre", ic_cofre },
    { "sincronizar", ic_sincronizar },
    { "historico", ic_historico },
    { "trophy", ic_trophy },
    { "puzzle", ic_puzzle },
    { "robo", ic_robo },
    { "medidor", ic_medidor },
    { "grafico", ic_grafico },
    { "tendencia", ic_tendencia },
    { "documento", ic_documento },
    { "documento-texto", ic_documento_texto },
    { "documento-pasta", ic_documento_pasta },
    { "documento-editar", ic_documento_editar },
    { "documento-novo", ic_documento_novo },
    { "documento-bloqueado", ic_documento_bloqueado },
    { "notas", ic_notas },
    { "biblioteca", ic_biblioteca },
    { "livro", ic_livro },
    { "caderno", ic_caderno },
    { "lista", ic_lista },
    { "texto", ic_texto },
    { "tabela", ic_tabela },
    { "base", ic_base },
    { "correio", ic_correio },
    { "correio-alerta", ic_correio_alerta },
    { "correio-muitos", ic_correio_muitos },
    { "conversa", ic_conversa },
    { "conversas", ic_conversas },
    { "telefone", ic_telefone },
    { "globo", ic_globo },
    { "globo-escudo", ic_globo_escudo },
    { "wifi", ic_wifi },
    { "wifi-aviso", ic_wifi_aviso },
    { "local", ic_local },
    { "alfinete", ic_alfinete },
    { "ligacao", ic_ligacao },
    { "enviar", ic_enviar },
    { "codigo", ic_codigo },
    { "cartao-contato", ic_cartao_contato },
    { "camera", ic_camera },
    { "video", ic_video },
    { "imagem", ic_imagem },
    { "microfone", ic_microfone },
    { "fones", ic_fones },
    { "sol", ic_sol },
    { "neve", ic_neve },
    { "jogo", ic_jogo },
    { "desporto", ic_desporto },
    { "recibo", ic_recibo },
    { "poupanca", ic_poupanca },
    { "formulario", ic_formulario },
    { "escudo", ic_escudo },
    { "escudo-verificado", ic_escudo_verificado },
    { "cadeado", ic_cadeado },
    { "alerta", ic_alerta },
    { "aviso", ic_aviso },
    { "erro", ic_erro },
    { "duvida", ic_duvida },
    { "certo", ic_certo },
    { "fechar", ic_fechar },
    { "pasta", ic_pasta },
    { "guardar", ic_guardar },
    { "abrir", ic_abrir },
    { "procurar", ic_procurar },
    { "tocar", ic_tocar },
    { "pausar", ic_pausar },
    { "copiar", ic_copiar },
    { "colar", ic_colar },
    { "excluir", ic_excluir },
    { "editar", ic_editar },
    { "voltar", ic_voltar },
    { "avancar", ic_avancar },
    { "subir", ic_subir },
    { "descer", ic_descer },
    { "janela", ic_janela },
    { "disco", ic_disco },
    { "imprimir", ic_imprimir },
    { "olho", ic_olho },
    { "terminal", ic_terminal },
    { "som", ic_som },
    { "musica", ic_musica },
    { "informacao", ic_informacao },
    { "ajuda", ic_ajuda },
    { "bateria", ic_bateria },
    { "ambiente", ic_ambiente },
    { "desenhar", ic_desenhar },
};

#define QTD_ICONES (int)(sizeof tabela / sizeof tabela[0])

/* La paleta compartida: '.' es transparente, los demás son tonos. */
uint32_t icones_cor(char c)
{
    switch (c) {
    case 'P': return esq_trazo(0x202428);   /* D428: con un esquema oscuro, el trazo oscuro se aclara */
    case 'D': return esq_trazo(0x4A5460);
    case 'B': return 0xFFFFFF;
    case 'C': return 0x9AA3AC;
    case 'L': return 0xE3E8EE;
    case 'V': return 0x2A6FD8;
    case 'T': return 0x22B8D6;
    case 'G': return 0x3CDC6E;
    case 'Y': return 0xFFE070;
    case 'A': return 0xF2C230;
    case 'O': return 0xF2762D;
    case 'R': return 0xE05252;
    case 'M': return 0x9B6BD6;
    case 'K': return 0x8A6239;
    default: return 0xFF000000;   /* transparente */
    }
}

int icones_total(void)
{
    return QTD_ICONES;
}

const icone_t *icones_todos(void)
{
    return tabela;
}

const icone_t *icones_procurar(const char *chave)
{
    if (!chave) return NULL;
    for (int i = 0; i < QTD_ICONES; i++)
        if (compara_texto(tabela[i].chave, chave) == 0) return &tabela[i];
    return NULL;
}

/* Dibuja un icono por su nombre. Si no existe, no dibuja nada. */
void icone_desenhar(jan_t *j, int x, int y, const char *chave, int escala)
{
    const icone_t *ic = icones_procurar(chave);
    if (!ic) return;
    g_desenho(j, x, y, ic->linhas, ICONES_LADO, ICONES_LADO, icones_cor, escala);
}

/* Dibuja un icono por su posición en la tabla. */
void icone_desenhar_indice(jan_t *j, int x, int y, int indice, int escala)
{
    if (indice < 0 || indice >= QTD_ICONES) return;
    g_desenho(j, x, y, tabela[indice].linhas, ICONES_LADO, ICONES_LADO, icones_cor, escala);
}

