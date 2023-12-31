/* Central processor. */

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

#define IR0 (IR & 020)
#define IR1 (IR & 010)
#define IR2 (IR & 004)
#define IR3 (IR & 002)
#define IR4 (IR & 001)

#define MB07 (MB & 002000)
#define MB12 (MB & 000040)
#define MB13 (MB & 000020)
#define MB14 (MB & 000010)
#define MB15 (MB & 000004)

#define CM_SUB    0400000000000ULL
#define CM_EAE    0200000000000ULL
#define CM_EAE_R  0100000000000ULL
#define CM_MBO    0040000000000ULL
#define CM_MBI    0020000000000ULL
#define CM_SKPI   0010000000000ULL
#define CM_ACO    0004000000000ULL
#define CM_ACI    0002000000000ULL
#define CM_IRI    0001000000000ULL
#define CM_ARO    0000400000000ULL
#define CM_ARI    0000200000000ULL
#define CM_CJIT   0000100000000ULL
#define CM_PCO    0000040000000ULL
#define CM_PCI    0000020000000ULL
#define CM_EAE_P  0000010000000ULL
#define CM_MQO    0000004000000ULL
#define CM_MQI    0000002000000ULL
#define CM_DEI    0000001000000ULL
#define CM_LI     0000000400000ULL
#define CM_AND    0000000200000ULL
#define CM_TI     0000000100000ULL
#define CM_DONE   0000000040000ULL
#define CM_AXS    0000000020000ULL
#define CM_CMA0   0000000010000ULL
#define CM_CMA1   0000000004000ULL
#define CM_CMA2   0000000002000ULL
#define CM_CMA3   0000000001000ULL
#define CM_CMA4   0000000000400ULL
#define CM_CMA5   0000000000200ULL
#define CM_CMA    (CM_CMA0|CM_CMA1|CM_CMA2|CM_CMA3|CM_CMA4|CM_CMA5)
#define CM_EXT    0000000000100ULL
#define CM_CONT   0000000000040ULL
#define CM_PLUS1  0000000000020ULL
#define CM_SM     0000000000010ULL
#define CM_ADSO   0000000000004ULL
#define CM_KEY    0000000000002ULL
#define CM_DCH    0000000000001ULL

word_t IR;
word_t MB;
word_t AR;
word_t AC;
word_t MQ;
word_t PC;
unsigned LINK;

word_t ADDR_SW;
word_t DATA_SW;

word_t sig_A_BUS;
word_t sig_B_BUS;
word_t sig_O_BUS;

word_t sig_ADR;
unsigned sig_ADRis0, sig_CMPL, sig_ADRL;

cm_t ff_CMA;
cm_t sig_CMSL;
cm_t ff_DCH;
cm_t ff_EAE;
cm_t ff_CJIT;
cm_t ff_EAE_P;
cm_t ff_ADSO;
cm_t ff_MBO;
cm_t ff_SUB;
cm_t ff_LI;
cm_t ff_IRI;
cm_t ff_AND;
cm_t ff_DEI;
cm_t ff_SM;
cm_t ff_AXS;
cm_t ff_KEY;
cm_t ff_PCO;
cm_t ff_MQO;
cm_t ff_ACO;
cm_t ff_ARO;
cm_t ff_EAE;
cm_t ff_EAE;
cm_t ff_TI;
cm_t ff_SKPI;
cm_t ff_DONE;
cm_t ff_CONT;
cm_t ff_EAE_R;
cm_t ff_PLUS1;
cm_t ff_MBI;
cm_t ff_ACI;
cm_t ff_ARI;
cm_t ff_PCI;
cm_t ff_MQI;
cm_t ff_EXT;

unsigned C, ff_SEN, ff_PCOS, ff_AROS, PV, LOT, sig_ISZ, ff_CAL, REP, IOT, OP;
unsigned sig_CJIT_CAL_V_JMS, sig_CI17, sig_CO00;
unsigned ff_SKIP;
unsigned ff_SAO;
unsigned sig_KIOA3, sig_KIOA4, sig_KIOA5, sig_DASO, ff_RUN;
unsigned INC_V_DCH, API_BK_RQ_B, PROG_SY, R12B;
unsigned ff_AUT_INX, BK_SYNC, ff_BK, ODD_ADDR, sig_ADDR10;
volatile unsigned sig_KST, sig_KSP, sig_KCT, sig_KMT, sig_KIO, sig_KRI;
volatile unsigned sig_KDP, sig_KDN, sig_KEX, sig_KEN;
volatile unsigned sig_SW_SGL_INST, sig_SW_SGL_STP, sig_REPT;
volatile unsigned sig_KDPDN_RI, sig_KEY_KPDN;
unsigned ff_UM;
unsigned sig_NOSH, sig_SHL1, sig_SHL2, sig_SHR1, sig_SHR2;
unsigned sig_deltaMB, ff_MEM_STROBE;

static void pwr_clr_pos(void);
static void key_init_pos(void);
static void pk_clr(void);
static void zero_to_cma(void);
static void cm_clk(void);
static void cm_current(void);
static void cm_strobe_a(void);
static void cm_strobe_b(void);
static void cm_strobe_c(void);
static void cm_strobe_d(void);

static int power = 0;

void power_on(void)
{
  VCD(IR, IR = rand() & 017);
  VCD(MB, MB = rand() & 0777777);
  VCD(AC, AC = rand() & 0777777);
  VCD(AR, AR = rand() & 0777777);
          MQ = rand() & 0777777;
  VCD(MA, MA = rand() & 0017777);
  VCD(PC, PC = rand() & 0017777);
  sig_CMSL = 0;
  power = 0;
  pwr_clr_pos();
  power = 1;
}

void power_off(void)
{
  VCD(RUN, ff_RUN = 0);
  MB = AC = AR = PC = MQ = 0;
  MA = IR = 0;
  power = 0;
  VAR(IR);
  VAR(MB);
  VAR(AC);
  VAR(AR);
  VAR(PC);
}

unsigned cp_clk(void)
{
  unsigned sig_key_init_pos = 0;
  static unsigned prev_KST = 0, prev_KCT = 0;
  static unsigned prev_KEX = 0, prev_KEN = 0;
  static unsigned prev_KDP = 0, prev_KDN = 0;
  static unsigned this_KST, this_KCT;
  static unsigned this_KEX, this_KEN;
  static unsigned this_KDP, this_KDN;

  if (!power)
    return 0;

  VCD(KST, this_KST = sig_KST);
  VCD(KSP, sig_KSP);
  VCD(KCT, this_KCT = sig_KCT);
  VCD(KDP, this_KDP = sig_KDP);
  VCD(KDN, this_KDN = sig_KDN);
  VCD(KEX, this_KEX = sig_KEX);
  VCD(KEN, this_KEN = sig_KEN);
  VCD(KRI, sig_KRI);

  VCD(KIOA3, sig_KIOA3 = this_KST || sig_KMT || sig_KIO);
  VCD(KIOA4, sig_KIOA4 = this_KST || sig_KMT || this_KEN || this_KDN);
  VCD(KIOA5, sig_KIOA5 = sig_KMT || this_KEN || this_KDN ||
      this_KEX || this_KDP || sig_KIO);

  if ((!prev_KST && this_KST) ||
      (!prev_KEX && this_KEX) ||
      (!prev_KEN && this_KEN) ||
      (!prev_KDP && this_KDP) ||
      (!prev_KDN && this_KDN)) {
    if (sig_KIOA3 || sig_KIOA4 || sig_KIOA5) {
      key_init_pos();
      sig_key_init_pos = 1;
    }
  }

  if (!prev_KCT && this_KCT) {
    ff_PCO = ff_PCOS;
    ff_ARO = ff_AROS;
    ff_RUN = 1;
    sig_key_init_pos = 1;
  }

  prev_KST = this_KST;
  prev_KCT = this_KCT;
  prev_KEX = this_KEX;
  prev_KEN = this_KEN;
  prev_KDP = this_KDP;
  prev_KDN = this_KDN;

  return sig_key_init_pos;
}

static void ADR(void)
{
  sig_ADR = sig_A_BUS + sig_B_BUS + !!sig_CI17;
  sig_CO00 = sig_ADR >> 18;
  sig_ADR &= 0777777;
  if (sig_CMPL)
    sig_ADR ^= 0777777;
  sig_ADRis0 = sig_ADR == 0;
}

void clr(void)
{
  VCD(CLR, 1);

  VCD(MBO, ff_MBO = 0);
  VCD(PLUS1, ff_PLUS1 = 0);
  VCD(PCI, ff_PCI = 0);
  VCD(ACO, ff_ACO = 0);
  VCD(ACI, ff_ACI = 0);
  sig_KEY_KPDN =
    ((sig_KDP || sig_KDN) && ff_KEY) ||
    (OP && MB15) ||
    (ff_MBI && sig_KDPDN_RI);
  sig_DASO = sig_KEY_KPDN;
  VCD(SAO, ff_SAO = !sig_KEY_KPDN);
  VCD(MBI, ff_MBI = 1);

  sig_A_BUS = 0;
  sig_B_BUS = 0;
  if (ff_SAO)
    sig_B_BUS = SA;
  sig_CI17 = ff_PLUS1 ||
    (ff_PCO && ff_SKIP) ||
    (ff_CJIT && sig_ISZ) ||
    (ff_SAO && ff_AUT_INX);
  VCD(CI17, sig_CI17);
  ADR();
  
  sig_O_BUS = sig_ADR;
  if (sig_DASO)
    sig_O_BUS |= DATA_SW;
  if (ff_MBI)
    VCD(MB, MB = sig_O_BUS);
}

void cm_clk_pos(void)
{
  cm_clk();
}

static void cm_clk(void)
{
  VCD(CM_STROBE, 1);
  cm_current();
  cm_strobe_a();
  cm_strobe_b();
  cm_strobe_c();
  cm_strobe_d();

  if (sig_CJIT_CAL_V_JMS)
    VCD(PCI, ff_PCI = 1);
  if (ff_PCI)
    VCD(SKIP, ff_SKIP = 0);
  sig_deltaMB = ff_PCO || ff_ARO || ff_CAL || ff_EXT || PV;
  if (sig_deltaMB && ff_SM && ff_RUN)
    VCD(MBI, ff_MBI = 1);

  sig_A_BUS = 0;
  if (ff_ADSO)
    sig_A_BUS = ADDR_SW;
  if (ff_ACO)
    sig_A_BUS = AC;
  if (ff_ARO)
    sig_A_BUS = AR;
  if (ff_PCO)
    sig_A_BUS = PC;
  if (ff_MQO)
    sig_A_BUS = MQ;

  sig_B_BUS = 0;
  if (ff_MBO)
    sig_B_BUS |= MB;
  if (ff_SUB)
    sig_B_BUS |= ~MB & 0777777;
  if (ff_AND)
    sig_B_BUS |= ~sig_A_BUS & 0777777;
  if (ff_SAO)
    sig_B_BUS |= SA;

  ADR();

  if (sig_NOSH) {
    sig_O_BUS = sig_ADR;
    sig_ADRL = sig_CO00;
  }
  if (sig_SHL1) {
    sig_O_BUS = ((sig_ADR << 1) & 0777777) | sig_CO00;
    sig_ADRL = sig_ADR >> 17;
  }
  if (sig_SHL2) {
    sig_O_BUS = ((sig_ADR << 2) & 0777777) | (sig_CO00 << 1) | (sig_ADR >> 17);
    sig_ADRL = (sig_ADR >> 16) & 1;
  }
  if (sig_SHR1) {
    sig_O_BUS = (sig_ADR >> 1) | (sig_CO00 << 17);
    sig_ADRL = sig_ADR & 1;
  }
  if (sig_SHR2) {
    sig_O_BUS = (sig_ADR >> 2) | (sig_CO00 << 16) | ((sig_ADR & 1) << 17);
    sig_ADRL = (sig_ADR >> 1) & 1;
  }
  if (ff_TI && ff_CAL)
    sig_O_BUS |= 020;

  if (ff_MBI)
    VCD(MB, MB = sig_O_BUS);
  if (ff_ACI)
    VCD(AC, AC = sig_O_BUS);
  if (ff_ARI)
    VCD(AR, AR = sig_O_BUS);
  if (ff_PCI)
    VCD(PC, PC = sig_O_BUS & 017777);
  if (ff_PCI)
    VCD(SKIP, ff_SKIP = 0);
  if (ff_MQI)
    MQ = sig_O_BUS;
  if (ff_LI)
    LINK = sig_ADRL;
  //printf("MB = %06o\nAC = %06o\nAR = %06o\nPC = %06o\n", MB, AC, AR, PC);
  //printf("IR = %02o\n", IR << 1);
}

static void ind_clk(void)
{
  cm_strobe_b();
}

static void pwr_clr_pos(void)
{
  C = 0;
  ff_SEN = 0;
  ind_clk();
  pk_clr();
  cm_strobe_a();
  cm_strobe_c();
  cm_strobe_d();
  zero_to_cma();
}

static void key_init_pos(void)
{
  zero_to_cma();
}

static void sen(void)
{
  ff_PCOS = ff_PCO;
  ff_AROS = ff_ARO;
}

static void pk_clr(void)
{
  PV = 0;
  ff_CMA &= ~073;
}

static void cm_current(void)
{
  unsigned ff_CMA0 = ff_CMA & 040;
  unsigned ff_CMA1 = ff_CMA & 020;
  unsigned ff_CMA2 = ff_CMA & 010;
  unsigned ff_CMA3 = ff_CMA & 004;
  unsigned ff_CMA4 = ff_CMA & 002;
  unsigned ff_CMA5 = ff_CMA & 001;
  unsigned address = ff_CMA0 | ff_CMA1;
  if (!ff_CMA0 && !ff_CMA1 && !ff_CMA2)
    address |= (sig_KIOA3 << 2) | (sig_KIOA4 << 1) | sig_KIOA5;
  if (!REP)
    address |= ff_CMA2 | ff_CMA3 | ff_CMA4 | ff_CMA5;
  if (sig_ADDR10)
    address |= 010;
  if (ff_EXT)
    address |= (INC_V_DCH ? 4 : 0) | (API_BK_RQ_B ? 3 : 0);
  if (REP || (ff_CMA0 && ff_CMA1))
    address |= IR >> 1;
  if (REP)
    address |= 040;
  if ((PROG_SY && ff_EXT) || (ff_TI && !IR4 && !IR0 && !IR1))
    address |= 2;
  if ((ff_TI && IR4) || (ff_KEY && R12B) || (ff_DONE && BK_SYNC) || ODD_ADDR)
    address |= 1;
  //printf("CMA/%02o\r\n", address);
  VCD(CMA, address);
  sig_CMSL = cm[address];
}

static void cm_strobe_a(void)
{
  ff_DCH = sig_CMSL & CM_DCH;
  if (ff_EAE || ff_LI)
    ff_LI = sig_CMSL & CM_LI;
  VCD(IRI, ff_IRI = sig_CMSL & CM_IRI);
  if (ff_IRI) {
    LOT = (SA >> 15) == 7;
    VCD(IR, IR = SA >> 13);
    //PV = ...;
    VCD(ISZ, sig_ISZ = IR0 && !IR1 && !IR2 && IR3);
  }
  if (ff_IRI)
    ff_CAL = !IR0 && !IR1 && !IR2 && !IR3 && !ff_EXT;
  ff_AND = sig_CMSL & CM_AND;
  ff_DEI = sig_CMSL & CM_DEI;
  if (ff_DEI) {
    IR &= ~1;
    VAR(IR);
    ff_CAL = 0;
  }
  REP = (ff_IRI || ff_DEI) &&
    ((IR0 && IR1 && IR2) ||
     (IR0 && IR1 && IR3) ||
     (IR0 && !IR2 && !IR3 && !IR4));
  VAR(REP);

  VCD(SM, ff_SM = sig_CMSL & CM_SM);
  VCD(AXS, ff_AXS = sig_CMSL & CM_AXS);
  VCD(KEY, ff_KEY = sig_CMSL & CM_KEY);
}

static void cm_strobe_b(void)
{
  VCD(PCO, ff_PCO = sig_CMSL & CM_PCO);
  VCD(ACO, ff_ACO = sig_CMSL & CM_ACO);
  VCD(ARO, ff_ARO = sig_CMSL & CM_ARO);
  ff_MQO = sig_CMSL & CM_MQO;
  //IO_BUS_ON =
}

static void cm_strobe_c(void)
{
  ff_EAE = sig_CMSL & CM_EAE;
  ff_EAE_P = sig_CMSL & CM_EAE_P;
  ff_TI = sig_CMSL & CM_TI;
  ff_EAE = sig_CMSL & CM_EAE;
  ff_CMA = (sig_CMSL & CM_CMA) >> 7;

  VCD(CJIT, ff_CJIT = sig_CMSL & CM_CJIT);
  sig_CJIT_CAL_V_JMS = ff_CJIT && !IR0 && !IR1 && !IR3;
  ff_ADSO = sig_CMSL & CM_ADSO;

  VCD(MBO, ff_MBO = sig_CMSL & CM_MBO);
  ff_SUB = sig_CMSL & CM_SUB;

  IOT = LOT && !IR3;
  OP = LOT && IR3 && !IR4;
  if (OP && (MB & 040) && !ff_UM)
    VCD(RUN, ff_RUN = 0);
  sig_SHR1 = OP && !MB07 && MB13;
  sig_SHL1 = OP && !MB07 && MB14;
  sig_SHR2 = OP && MB07 && MB13;
  sig_SHL2 = OP && MB07 && MB14;
  sig_NOSH = !sig_SHR1 && !sig_SHL1 && !sig_SHR2 && !sig_SHL2;

  ff_AUT_INX = ff_TI && IR4 && (MB & 017770) == 010;
}

static void zero_to_cma(void)
{
  VCD(CMA, ff_CMA &= 010);
}

static void thirteen_to_cma(void)
{
  VCD(CMA, ff_CMA |= 013);
}

static void cm_strobe_d(void)
{
  VCD(SAO, ff_SAO = 0);
  VCD(SKPI, ff_SKPI = sig_CMSL & CM_SKPI);
  if (ff_SKPI)
    ff_SKIP = (sig_ISZ && sig_ADRis0); //More inputs!
  FF(SKIP);
  ff_DONE = sig_CMSL & CM_DONE;
  if (ff_DONE && (sig_KSP || sig_SW_SGL_INST)) {
    VCD(RUN, ff_RUN = 0);
    sen();
  }
  VCD(CONT, ff_CONT = sig_CMSL & CM_CONT);
  ff_EAE_R = sig_CMSL & CM_EAE_R;

  VCD(PLUS1, ff_PLUS1 = sig_CMSL & CM_PLUS1);
  sig_CI17 = ff_PLUS1 ||
    (ff_PCO && ff_SKIP) ||
    (ff_CJIT && sig_ISZ) ||
    (ff_SAO && ff_AUT_INX);
  VCD(CI17, sig_CI17);
  VCD(MBI, ff_MBI = sig_CMSL & CM_MBI);
  VCD(ACI, ff_ACI = sig_CMSL & CM_ACI);
  VCD(ARI, ff_ARI = sig_CMSL & CM_ARI);
  VCD(PCI, ff_PCI = sig_CMSL & CM_PCI);
  if (ff_PCI)
    VCD(SKIP, ff_SKIP = 0);
  ff_MQI = sig_CMSL & CM_MQI;

  ff_EXT = sig_CMSL & CM_EXT;
  if (ff_EXT)
    ff_BK = 1;
}

static void zero_to_mbi(void)
{
  VCD(MBI, ff_MBI = 0);
}
