#include	"compiler.h"
#include	"strres.h"


const UINT8 str_utf8[3] = {0xef, 0xbb, 0xbf};
const UINT16 str_ucs2[1] = {0xfeff};


const OEMCHAR str_null[] = OEMTEXT("");
const OEMCHAR str_space[] = OEMTEXT(" ");
const OEMCHAR str_dot[] = OEMTEXT(".");

const OEMCHAR str_cr[] = OEMTEXT("\r");
const OEMCHAR str_crlf[] = OEMTEXT("\r\n");

const OEMCHAR str_ini[] = OEMTEXT("ini");
const OEMCHAR str_cfg[] = OEMTEXT("cfg");
const OEMCHAR str_sav[] = OEMTEXT("sav");
const OEMCHAR str_bmp[] = OEMTEXT("bmp");
const OEMCHAR str_bmp_b[] = OEMTEXT("BMP");
const OEMCHAR str_d88[] = OEMTEXT("d88");
const OEMCHAR str_d98[] = OEMTEXT("d98");
const OEMCHAR str_88d[] = OEMTEXT("88d");
const OEMCHAR str_98d[] = OEMTEXT("98d");
const OEMCHAR str_thd[] = OEMTEXT("thd");
const OEMCHAR str_hdi[] = OEMTEXT("hdi");
const OEMCHAR str_fdi[] = OEMTEXT("fdi");
const OEMCHAR str_hdd[] = OEMTEXT("hdd");
const OEMCHAR str_nhd[] = OEMTEXT("nhd");
const OEMCHAR str_vhd[] = OEMTEXT("vhd");
const OEMCHAR str_slh[] = OEMTEXT("slh");
const OEMCHAR str_hdn[] = OEMTEXT("hdn");
const OEMCHAR str_hdm[] = OEMTEXT("hdm");
const OEMCHAR str_hd4[] = OEMTEXT("hd4");

const OEMCHAR str_d[] = OEMTEXT("%d");
const OEMCHAR str_u[] = OEMTEXT("%u");
const OEMCHAR str_x[] = OEMTEXT("%x");
const OEMCHAR str_2d[] = OEMTEXT("%.2d");
const OEMCHAR str_2x[] = OEMTEXT("%.2x");
const OEMCHAR str_4x[] = OEMTEXT("%.4x");
const OEMCHAR str_4X[] = OEMTEXT("%.4X");

const OEMCHAR str_false[] = OEMTEXT("false");
const OEMCHAR str_true[] = OEMTEXT("true");

const OEMCHAR str_posx[] = OEMTEXT("posx");
const OEMCHAR str_posy[] = OEMTEXT("posy");
const OEMCHAR str_width[] = OEMTEXT("width");
const OEMCHAR str_height[] = OEMTEXT("height");

const OEMCHAR str_np2[] = OEMTEXT("Neko Project II kai");
const OEMCHAR str_resume[] = OEMTEXT("Resume");

const OEMCHAR str_VM[] = OEMTEXT("VM");
const OEMCHAR str_VX[] = OEMTEXT("VX");
const OEMCHAR str_EPSON[] = OEMTEXT("EPSON");

const OEMCHAR str_biosrom[] = OEMTEXT("bios.rom");
const OEMCHAR str_sasirom[] = OEMTEXT("sasi.rom");
const OEMCHAR str_scsirom[] = OEMTEXT("scsi.rom");

