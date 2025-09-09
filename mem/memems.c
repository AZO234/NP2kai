#include	<compiler.h>
#include	<cpucore.h>
#include	<pccore.h>
#include	<io/iocore.h>
#include	<mem/memems.h>


REG8 MEMCALL memems_rd8(UINT32 address) {

	return(CPU_EMSPTR[(address >> 14) & 3][LOW14(address)]);
}

REG16 MEMCALL memems_rd16(UINT32 address) {

const UINT8	*ptr;
	REG16	ret;

	if ((address & 0x3fff) != 0x3fff) {
		ptr = CPU_EMSPTR[(address >> 14) & 3] + LOW14(address);
		return(LOADINTELWORD(ptr));
	}
	else {
		ret = CPU_EMSPTR[(address >> 14) & 3][0x3fff];
		ret += CPU_EMSPTR[((address + 1) >> 14) & 3][0] << 8;
		return(ret);
	}
}

UINT32 MEMCALL memems_rd32(UINT32 address){
	UINT32 r = (UINT32)memems_rd16(address);
	r |= (UINT32)memems_rd16(address+2) << 16;
	return r;
}

void MEMCALL memems_wr8(UINT32 address, REG8 value) {

	CPU_EMSPTR[(address >> 14) & 3][LOW14(address)] = (UINT8)value;
}

void MEMCALL memems_wr16(UINT32 address, REG16 value) {

	UINT8	*ptr;

	if ((address & 0x3fff) != 0x3fff) {
		ptr = CPU_EMSPTR[(address >> 14) & 3] + LOW14(address);
		STOREINTELWORD(ptr, value);
	}
	else {
		CPU_EMSPTR[(address >> 14) & 3][0x3fff] = (UINT8)value;
		CPU_EMSPTR[((address + 1) >> 14) & 3][0] = (UINT8)(value >> 8);
	}
}

void MEMCALL memems_wr32(UINT32 address, UINT32 value){
	memems_wr16(address, (REG16)value);
	memems_wr16(address+2, (REG16)(value >> 16));
}

