#include "commng.h"
#include "compiler.h"
#include "cpucore.h"
#include "iocore.h"
#include "pccore.h"

#if 0
#undef TRACEOUT
static void trace_fmt_ex(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "¥n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define TRACEOUT(s) trace_fmt_ex s
static void trace_fmt_exw(const WCHAR* fmt, ...)
{
	WCHAR stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vswprintf(stmp, 2048, fmt, ap);
	wcscat(stmp, L"¥n");
	va_end(ap);
	OutputDebugStringW(stmp);
}
#define TRACEOUTW(s) trace_fmt_exw s
#else
#define TRACEOUTW(s) (void)0
#endif /* 1 */

COMMNG cm_prt;

static REG8 lastData = 0xff;
static REG8 i44Data = 0x00;
static int selectBusyFlag = 0;
static int selectCounter = 0;

// ---- I/O

static void IOOUTCALL prt_o40(UINT port, REG8 dat) {

  COMMNG prt;

  prt = cm_prt;
  if (prt == NULL) {
    prt = commng_create(COMCREATE_PRINTER, FALSE);
    cm_prt = prt;
  }
  prt->write(prt, (UINT8)dat);
  lastData = dat;
  if (lastData == 0x11) { // DC1 Select
    // WORKAROUND: 制御文字でDC1
    // Selectを連続で送ってきているときはBUSYを一時的に立てる。本当はプリンタコマンドのSELECTに応答すべき？
    if (selectCounter > 5) {
      selectBusyFlag = 1;
    } else {
      selectCounter++;
    }
  } else {
    selectCounter = 0;
  }
  TRACEOUT(("prt %.3x - %.2x", port, dat));
  (void)port;
}
static void IOOUTCALL prt_o44(UINT port, REG8 dat) {

  if (!(i44Data & 0x80) && (dat & 0x80)) {
    TRACEOUT(("Strobe"));
  }
  i44Data = dat;
  (void)port;
}

static REG8 IOINPCALL prt_i40(UINT port) {

  (void)port;
  return (lastData);
}
static REG8 IOINPCALL prt_i42(UINT port) {

  REG8 ret;

  ret = 0x84;
  if (pccore.cpumode & CPUMODE_8MHZ) {
    ret |= 0x20;
  }
  if (pccore.dipsw[0] & 4) {
    ret |= 0x10;
  }
  if (pccore.dipsw[0] & 0x80) {
    ret |= 0x08;
  }
  if (!(pccore.model & PCMODEL_EPSON)) {
    if (CPU_TYPE & CPUTYPE_V30) {
      ret |= 0x02;
    }
  } else {
    if (pccore.dipsw[2] & 0x80) {
      ret |= 0x02;
    }
  }
  if (selectBusyFlag) {
    // Selectすると一瞬だけBusyが立つ（負論理なのでビットは0になる）らしい
    ret &= ~0x04;
    selectBusyFlag = 0;
  }
  (void)port;
  return (ret); // 0x04 Low PWR ON
}
static REG8 IOINPCALL prt_i44(UINT port) {

  (void)port;
  return (i44Data);
}

// IEEE標準パラレルポート
#define PARALLEL_IEEE_STATUS_5V 0x02
#define PARALLEL_IEEE_STATUS_NOERROR 0x08
#define PARALLEL_IEEE_STATUS_SELECT 0x10
#define PARALLEL_IEEE_STATUS_PAPEROUT 0x20
#define PARALLEL_IEEE_STATUS_NOACK 0x40
#define PARALLEL_IEEE_STATUS_NOBUSY 0x80

#define PARALLEL_IEEE_CONTROL_STROBE 0x01
#define PARALLEL_IEEE_CONTROL_AUTOFEED 0x02
#define PARALLEL_IEEE_CONTROL_INIT 0x04
#define PARALLEL_IEEE_CONTROL_SELECTIN 0x08
#define PARALLEL_IEEE_CONTROL_ACKINT 0x10
#define PARALLEL_IEEE_CONTROL_DIRECTION 0x20

static REG8 prtIEEE_data = 0xff;
static REG8 prtIEEE_status = 0x00;
static REG8 prtIEEE_control = 0x00;
static REG8 prtIEEE_mode = 0x00;
static REG8 i14bData = 0xff;
static REG8 i14dData = 0x00;
static REG8 i14eData = 0x01;

static void IOOUTCALL prt_o140(UINT port, REG8 dat) {

  prtIEEE_data = dat;
  (void)port;
}
static void IOOUTCALL prt_o141(UINT port, REG8 dat) {
  TRACEOUT(("prt %.3x - %.2x", port, dat));
  (void)port;
}
static void IOOUTCALL prt_o142(UINT port, REG8 dat) {

  if (!(prtIEEE_control & PARALLEL_IEEE_CONTROL_STROBE) && (dat &
                                                            PARALLEL_IEEE_CONTROL_STROBE) /* && (prtIEEE_control & PARALLEL_IEEE_CONTROL_SELECTIN)*/) {
    COMMNG prt;

    prt = cm_prt;
    if (prt == NULL) {
      prt = commng_create(COMCREATE_PRINTER, FALSE);
      cm_prt = prt;
    }
    prt->write(prt, (UINT8)prtIEEE_data);
    prtIEEE_status &= ~PARALLEL_IEEE_STATUS_NOACK;
  }
  if ((prtIEEE_control & PARALLEL_IEEE_CONTROL_INIT) &&
      !(dat & PARALLEL_IEEE_CONTROL_INIT)) {
    COMMNG prt;
    prt = cm_prt;
    if (prt != NULL) {
      prt->msg(prt, COMMSG_REOPEN, NULL);
    }
  }

  prtIEEE_control = dat;
  TRACEOUT(("prt %.3x - %.2x", port, dat));
  (void)port;
}
static void IOOUTCALL prt_o149(UINT port, REG8 dat) {

  prtIEEE_mode = dat;
  TRACEOUT(("prt %.3x - %.2x", port, dat));
  (void)port;
}
static void IOOUTCALL prt_o14b(UINT port, REG8 dat) {

  // if ((i149Data & 0x10)) return;

  i14bData = dat;
  TRACEOUT(("prt %.3x - %.2x", port, dat));
  (void)port;
}
static void IOOUTCALL prt_o14c(UINT port, REG8 dat) {

  COMMNG prt;

  // if ((i149Data & 0x10)) return;

  // if ((i14eData & 0xe0) == 0x40) {
  //	prt = cm_prt;
  //	if (prt == NULL) {
  //		prt = commng_create(COMCREATE_PRINTER, FALSE);
  //		cm_prt = prt;
  //	}
  //	prt->write(prt, (UINT8)dat);
  // }
  TRACEOUT(("prt %.3x - %.2x", port, dat));
  (void)port;
}
static void IOOUTCALL prt_o14d(UINT port, REG8 dat) {

  // if ((i149Data & 0x10)) return;

  i14dData = dat;
  TRACEOUT(("prt %.3x - %.2x", port, dat));
  (void)port;
}
static void IOOUTCALL prt_o14e(UINT port, REG8 dat) {

  // if ((i149Data & 0x10)) return;

  i14eData = (dat & 0xfc) | (i14eData & 0x3);
  TRACEOUT(("prt %.3x - %.2x", port, dat));
  (void)port;
}
static REG8 IOINPCALL prt_i140(UINT port) {

  // if ((i149Data & 0x10)) return 0xff;

  (void)port;
  return (prtIEEE_data);
}
static REG8 IOINPCALL prt_i141(UINT port) {

  COMMNG prt;
  REG8 ret = prtIEEE_status | PARALLEL_IEEE_STATUS_NOBUSY |
             PARALLEL_IEEE_STATUS_NOERROR;
  prtIEEE_status |= PARALLEL_IEEE_STATUS_NOACK; // XXX: ACKは読んだら戻す

  if ((prtIEEE_control & PARALLEL_IEEE_CONTROL_SELECTIN)) {
    ret |= PARALLEL_IEEE_STATUS_SELECT;
  }
  // if ((i149Data & 0x10)) return 0xff;

  prt = cm_prt;
  if (prt == NULL) {
    prt = commng_create(COMCREATE_PRINTER, FALSE);
    cm_prt = prt;
    if (prt == NULL) {
      ret &= ~PARALLEL_IEEE_STATUS_SELECT;
    }
  }
  (void)port;
  return (ret);
}
static REG8 IOINPCALL prt_i142(UINT port) {

  // if ((i149Data & 0x10)) return 0xff;

  (void)port;
  return (prtIEEE_control);
}
static REG8 IOINPCALL prt_i149(UINT port) {

  (void)port;
  return (prtIEEE_mode);
}
static REG8 IOINPCALL prt_i14b(UINT port) {

  // if ((i149Data & 0x10)) return 0xff;

  (void)port;
  return (i14bData);
}
static REG8 IOINPCALL prt_i14c(UINT port) {

  // if ((i149Data & 0x10)) return 0xff;

  (void)port;
  return (0xff);
}
static REG8 IOINPCALL prt_i14d(UINT port) {

  // if ((i149Data & 0x10)) return 0xff;

  (void)port;
  return (i14dData);
}
static REG8 IOINPCALL prt_i14e(UINT port) {

  // if ((i149Data & 0x10)) return 0xff;

  (void)port;
  return (i14eData);
}

// ---- I/F

static const IOOUT prto40[4] = {prt_o40, NULL, prt_o44, NULL};

static const IOINP prti40[4] = {prt_i40, prt_i42, prt_i44, NULL};

void printif_reset(const NP2CFG *pConfig) {

  commng_destroy(cm_prt);
  cm_prt = NULL;

  // 適当な初期値
  selectBusyFlag = 0;
  prtIEEE_data = 0xff;
  prtIEEE_status = 0x00;
  prtIEEE_control = 0x00;
  prtIEEE_mode =
      0x00; // 謎: Undocでbit 4は1=拡張パラレルポートモード,
            // 0=簡易セントロニクスモード（デフォルト）ということになっているが、Win2000は何も設定せずに拡張のポートへアクセスしに来る？？
  i14bData = 0xff;
  i14dData = 0x00;
  i14eData = 0x01;

  (void)pConfig;
}

void printif_bind(void) {

  iocore_attachsysoutex(0x0040, 0x0cf1, prto40, 4);
  iocore_attachsysinpex(0x0040, 0x0cf1, prti40, 4);

  iocore_attachout(0x0140, prt_o140);
  iocore_attachout(0x0141, prt_o141);
  iocore_attachout(0x0142, prt_o142);
  iocore_attachout(0x0149, prt_o149);
  iocore_attachout(0x014b, prt_o14b);
  iocore_attachout(0x014c, prt_o14c);
  iocore_attachout(0x014d, prt_o14d);
  iocore_attachout(0x014e, prt_o14e);
  iocore_attachinp(0x0140, prt_i140);
  iocore_attachinp(0x0141, prt_i141);
  iocore_attachinp(0x0142, prt_i142);
  iocore_attachinp(0x0149, prt_i149);
  iocore_attachinp(0x014b, prt_i14b);
  iocore_attachinp(0x014c, prt_i14c);
  iocore_attachinp(0x014d, prt_i14d);
  iocore_attachinp(0x014e, prt_i14e);
}

void printif_finalize(void) {

  commng_destroy(cm_prt);
  cm_prt = NULL;
}

// Finish current print job
void printif_finishjob(void) {

  if (cm_prt) {
    cm_prt->msg(cm_prt, COMMSG_REOPEN, NULL);
  }
}
