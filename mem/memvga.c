#include	"compiler.h"

// PEGC 256 color mode 

// �ڂ������Ȃ��̂ɍ�����̂ł��Ȃ肢�������ł��B
// ���ǂ���̂ł���ΑS���̂Ăč�蒼���������ǂ���������܂���

#if defined(SUPPORT_PC9821)

#include	"cpucore.h"
#include	"pccore.h"
#include	<io/iocore.h>
#include	"memvga.h"
#include	<vram/vram.h>
#if defined(SUPPORT_IA32_HAXM)
#include	"i386hax/haxfunc.h"
#include	"i386hax/haxcore.h"
#endif

#if 0
#undef  TRACEOUT
static void trace_fmt_ex(const char* fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define TRACEOUT(s) trace_fmt_ex s
#endif


// ---- macros

#define	VGARD8(p, a) {													\
	UINT32	addr;														\
	addr = (vramop.mio1[(p) * 2] & 15) << 15;							\
	addr += (a);														\
	addr -= (0xa8000 + ((p) * 0x8000));									\
	return(vramex[addr]);												\
}

#define	VGAWR8(p, a, v) {												\
	UINT32	addr;														\
	UINT8	bit;														\
	addr = (vramop.mio1[(p) * 2] & 15) << 15;							\
	addr += (a);														\
	addr -= (0xa8000 + ((p) * 0x8000));									\
	vramex[addr] = (v);													\
	bit = (addr & 0x40000)?2:1;											\
	vramupdate[LOW15(addr >> 3)] |= bit;								\
	gdcs.grphdisp |= bit;												\
}

#define	VGARD16(p, a) {													\
	UINT32	addr;														\
	addr = (vramop.mio1[(p) * 2] & 15) << 15;							\
	addr += (a);														\
	addr -= (0xa8000 + ((p) * 0x8000));									\
	return(LOADINTELWORD(vramex + addr));								\
}

#define	VGAWR16(p, a, v) {												\
	UINT32	addr;														\
	UINT8	bit;														\
	addr = (vramop.mio1[(p) * 2] & 15) << 15;							\
	addr += (a);														\
	addr -= (0xa8000 + ((p) * 0x8000));									\
	STOREINTELWORD(vramex + addr, (v));									\
	bit = (addr & 0x40000)?2:1;											\
	vramupdate[LOW15((addr + 0) >> 3)] |= bit;							\
	vramupdate[LOW15((addr + 1) >> 3)] |= bit;							\
	gdcs.grphdisp |= bit;												\
}

// ---- flat (PEGC 0F00000h-00F80000h Memory Access ?)

REG8 MEMCALL memvgaf_rd8(UINT32 address) {
	
	if(!(vramop.mio2[PEGC_REG_VRAM_ENABLE] & 0x1)){
		return 0xff;
	}
	return(vramex[address & 0x7ffff]);
}

void MEMCALL memvgaf_wr8(UINT32 address, REG8 value) {

	UINT8	bit;
	
	if(!(vramop.mio2[PEGC_REG_VRAM_ENABLE] & 0x1)){
		return;
	}
	address = address & 0x7ffff;
	vramex[address] = value;
	bit = (address & 0x40000)?2:1;
	vramupdate[LOW15(address >> 3)] |= bit;
	gdcs.grphdisp |= bit;
}

REG16 MEMCALL memvgaf_rd16(UINT32 address) {
	
	if(!(vramop.mio2[PEGC_REG_VRAM_ENABLE] & 0x1)){
		return 0xffff;
	}
	address = address & 0x7ffff;
	return(LOADINTELWORD(vramex + address));
}

void MEMCALL memvgaf_wr16(UINT32 address, REG16 value) {

	UINT8	bit;
	
	if(!(vramop.mio2[PEGC_REG_VRAM_ENABLE] & 0x1)){
		return;
	}
	address = address & 0x7ffff;
	STOREINTELWORD(vramex + address, value);
	bit = (address & 0x40000)?2:1;
	vramupdate[LOW15((address + 0) >> 3)] |= bit;
	vramupdate[LOW15((address + 1) >> 3)] |= bit;
	gdcs.grphdisp |= bit;
}

UINT32 MEMCALL memvgaf_rd32(UINT32 address){
	UINT32 r = (UINT32)memvgaf_rd16(address);
	r |= (UINT32)memvgaf_rd16(address+2) << 16;
	return r;
}
void MEMCALL memvgaf_wr32(UINT32 address, UINT32 value){
	memvgaf_wr16(address, (REG16)value);
	memvgaf_wr16(address+2, (REG16)(value >> 16));
}


// ---- 8086 bank memory (PEGC memvga0:A8000h-AFFFFh, memvga1:B0000h-B7FFFh Bank(Packed-pixel Mode) or Plane Access(Plane Mode))

REG8 MEMCALL memvga0_rd8(UINT32 address){
#ifdef SUPPORT_PEGC
	if(pegc.enable && (vramop.mio2[PEGC_REG_MODE] & 0x1)){
		// Plane Mode
		return 0;
	}else
#endif
	{
		// Packed-pixel Mode
		VGARD8(0, address)
	}
}
REG8 MEMCALL memvga1_rd8(UINT32 address){
#ifdef SUPPORT_PEGC
	if(pegc.enable && (vramop.mio2[PEGC_REG_MODE] & 0x1)){
		// Plane Mode
		return 0;
	}else
#endif
	{
		// Packed-pixel Mode
		VGARD8(1, address)
	}
}
void MEMCALL memvga0_wr8(UINT32 address, REG8 value){
#ifdef SUPPORT_PEGC
	if(pegc.enable && (vramop.mio2[PEGC_REG_MODE] & 0x1)){
		// Plane Mode
		// Nothing to do
	}else
#endif
	{
		// Packed-pixel Mode
		VGAWR8(0, address, value)
	}
}
void MEMCALL memvga1_wr8(UINT32 address, REG8 value){
#ifdef SUPPORT_PEGC
	if(pegc.enable && (vramop.mio2[PEGC_REG_MODE] & 0x1)){
		// Plane Mode
		// Nothing to do
	}else
#endif
	{
		// Packed-pixel Mode
		VGAWR8(1, address, value)
	}
}

REG16 MEMCALL memvga0_rd16(UINT32 address){
#ifdef SUPPORT_PEGC
	if(pegc.enable && (vramop.mio2[PEGC_REG_MODE] & 0x1)){
		// Plane Mode
		return pegc_memvgaplane_rd16(address);
	}else
#endif
	{
		// Packed-pixel Mode
		VGARD16(0, address)
	}
}
REG16 MEMCALL memvga1_rd16(UINT32 address){
#ifdef SUPPORT_PEGC
	if(pegc.enable && (vramop.mio2[PEGC_REG_MODE] & 0x1)){
		// Plane Mode
		return pegc_memvgaplane_rd16(address);
	}else
#endif
	{
		// Packed-pixel Mode
		VGARD16(1, address)
	}
}

void MEMCALL memvga0_wr16(UINT32 address, REG16 value){
#ifdef SUPPORT_PEGC
	if(pegc.enable && (vramop.mio2[PEGC_REG_MODE] & 0x1)){
		// Plane Mode
		pegc_memvgaplane_wr16(address, value);
	}else
#endif
	{
		// Packed-pixel Mode
		VGAWR16(0, address, value)
	}
}
void MEMCALL memvga1_wr16(UINT32 address, REG16 value){
#ifdef SUPPORT_PEGC
	if(pegc.enable && (vramop.mio2[PEGC_REG_MODE] & 0x1)){
		// Plane Mode
		pegc_memvgaplane_wr16(address, value);
	}else
#endif
	{
		// Packed-pixel Mode
		VGAWR16(1, address, value)
	}
}

UINT32 MEMCALL memvga0_rd32(UINT32 address){
#ifdef SUPPORT_PEGC
	if(pegc.enable && (vramop.mio2[PEGC_REG_MODE] & 0x1)){
		// Plane Mode
		return pegc_memvgaplane_rd32(address);
	}else
#endif
	{
		// Packed-pixel Mode
		return (UINT32)memvga0_rd16(address)|(memvga0_rd16(address+2)<<16);
	}
}
UINT32 MEMCALL memvga1_rd32(UINT32 address){
#ifdef SUPPORT_PEGC
	if(pegc.enable && (vramop.mio2[PEGC_REG_MODE] & 0x1)){
		// Plane Mode
		return pegc_memvgaplane_rd32(address);
	}else
#endif
	{
		// Packed-pixel Mode
		return (UINT32)memvga1_rd16(address)|(memvga1_rd16(address+2)<<16);
	}
}
void MEMCALL memvga0_wr32(UINT32 address, UINT32 value){
#ifdef SUPPORT_PEGC
	if(pegc.enable && (vramop.mio2[PEGC_REG_MODE] & 0x1)){
		// Plane Mode
		pegc_memvgaplane_wr32(address, value);
	}else
#endif
	{
		// Packed-pixel Mode
		memvga0_wr16(address, (REG16)value);
		memvga0_wr16(address+2, (REG16)(value >> 16));
	}
}
void MEMCALL memvga1_wr32(UINT32 address, UINT32 value){
#ifdef SUPPORT_PEGC
	if(pegc.enable && (vramop.mio2[PEGC_REG_MODE] & 0x1)){
		// Plane Mode
		pegc_memvgaplane_wr32(address, value);
	}else
#endif
	{
		// Packed-pixel Mode
		memvga1_wr16(address, (REG16)value);
		memvga1_wr16(address+2, (REG16)(value >> 16));
	}
}


// ---- 8086 bank I/O (PEGC E0000h-E7FFFh MMIO)

REG8 MEMCALL memvgaio_rd8(UINT32 address) {
	UINT pos;

	// PEGC pattern register has two logical views.  Do not expose the backing
	// mio2[] bytes directly; the pixel view is a transpose of the plane view.
	if ((address >= 0xe0120) && (address < 0xe0200)) {
		pos = address - 0xe0100;
		return pegc_pattern_rd8(pos);
	}

	pos = address - 0xe0004;
	if (pos < 4) {
		return vramop.mio1[pos];
	}
	pos = address - 0xe0100;
	if (pos < 0x20) {
		return vramop.mio2[pos];
	}
	return 0x00;
}

void MEMCALL memvgaio_wr8(UINT32 address, REG8 value) {
	UINT pos;

	if ((address >= 0xe0120) && (address < 0xe0200)) {
		pos = address - 0xe0100;
		pegc_pattern_wr8(pos, value);
		return;
	}

	pos = address - 0xe0004;
	if (pos < 4) {
		vramop.mio1[pos] = value;
#if defined(SUPPORT_IA32_HAXM)
		i386hax_vm_setmemoryarea(vramex + ((vramop.mio1[0] & 15) << 15), 0xA8000, 0x8000);
		i386hax_vm_setmemoryarea(vramex + ((vramop.mio1[2] & 15) << 15), 0xB0000, 0x8000);
#endif
		return;
	}

	pos = address - 0xe0100;
	if (pos < 0x20) {
		if (pos == PEGC_REG_MODE) {
#ifdef SUPPORT_PEGC
			if (pegc.enable) {
				value &= 0x1;
			}
			else
#endif
			{
				value = 0x0;
			}
		}
		vramop.mio2[pos] = value;

#ifdef SUPPORT_PEGC
		TRACEOUT(("PEGC CTRLRESET pos=%02x val=%02x "
			"ROP=%04x LEN=%04x SHIFT=%04x remain=%08x lastlen=%08x",
			pos, value,
			LOADINTELWORD(vramop.mio2 + PEGC_REG_PLANE_ROP),
			LOADINTELWORD(vramop.mio2 + PEGC_REG_LENGTH),
			LOADINTELWORD(vramop.mio2 + PEGC_REG_SHIFT),
			pegc.remain, pegc.lastdatalen));

		if ((pos == PEGC_REG_PLANE_ROP) ||
		    (pos == PEGC_REG_PLANE_ROP + 1) ||
		    (pos == PEGC_REG_LENGTH) ||
		    (pos == PEGC_REG_LENGTH + 1) ||
		    (pos == PEGC_REG_SHIFT) ||
		    (pos == PEGC_REG_SHIFT + 1)) {
			pegc_transfer_reset();
		}
#endif
	}
}

REG16 MEMCALL memvgaio_rd16(UINT32 address) {
	REG16 ret;

	ret = memvgaio_rd8(address);
	ret |= (REG16)memvgaio_rd8(address + 1) << 8;
	return ret;
}

void MEMCALL memvgaio_wr16(UINT32 address, REG16 value) {
	UINT pos;

	if ((address >= 0xe0120) && (address < 0xe0200)) {
		pos = address - 0xe0100;
		pegc_pattern_wr16(pos, value);
		return;
	}
	memvgaio_wr8(address, (REG8)value);
	memvgaio_wr8(address + 1, (REG8)(value >> 8));
}

UINT32 MEMCALL memvgaio_rd32(UINT32 address) {
	UINT32 ret;

	ret = (UINT32)memvgaio_rd8(address);
	ret |= (UINT32)memvgaio_rd8(address + 1) << 8;
	ret |= (UINT32)memvgaio_rd8(address + 2) << 16;
	ret |= (UINT32)memvgaio_rd8(address + 3) << 24;
	return ret;
}

void MEMCALL memvgaio_wr32(UINT32 address, UINT32 value) {
	UINT pos;

	if ((address >= 0xe0120) && (address < 0xe0200)) {
		pos = address - 0xe0100;
		pegc_pattern_wr32(pos, value);
		return;
	}
	memvgaio_wr16(address, (REG16)value);
	memvgaio_wr16(address + 2, (REG16)(value >> 16));
}


#endif

