#include	<compiler.h>
#include	<cpucore.h>
#include	<pccore.h>
#include	<io/iocore.h>
#include	<mem/memtram.h>
#include	<vram/vram.h>
#include	<font/font.h>
#if defined(SUPPORT_TEXTHOOK)
#include	<codecnv/codecnv.h>
#endif


REG8 MEMCALL memtram_rd8(UINT32 address) {

	CPU_REMCLOCK -= MEMWAIT_TRAM;
	if (address < 0xa4000) {
		return(mem[address]);
	}
	else if (address < 0xa5000) {
		if(hf_codeul) {
			hook_fontrom(cgwindow.low);
			hf_codeul = 0;
		}
		if (address & 1) {
			return(fontrom[cgwindow.high + ((address >> 1) & 0x0f)]);
		}
		else {
			return(fontrom[cgwindow.low + ((address >> 1) & 0x0f)]);
		}
	}
	return(mem[address]);
}

REG16 MEMCALL memtram_rd16(UINT32 address) {

	CPU_REMCLOCK -= MEMWAIT_TRAM;
	if(hf_codeul) {
		hook_fontrom(cgwindow.low);
		hf_codeul = 0;
	}
	if (address < (0xa4000 - 1)) {
		return(LOADINTELWORD(mem + address));
	}
	else if (address == 0xa3fff) {
		return(mem[address] + (fontrom[cgwindow.low] << 8));
	}
	else if (address < 0xa4fff) {
		if (address & 1) {
			REG16 ret;
			ret = fontrom[cgwindow.high + ((address >> 1) & 0x0f)];
			ret += fontrom[cgwindow.low + (((address + 1) >> 1) & 0x0f)] << 8;
			return(ret);
		}
		else {
			REG16 ret;
			ret = fontrom[cgwindow.low + ((address >> 1) & 0x0f)];
			ret += fontrom[cgwindow.high + ((address >> 1) & 0x0f)] << 8;
			return(ret);
		}
	}
	else if (address == 0xa4fff) {
		return((mem[0xa5000] << 8) | fontrom[cgwindow.high + 15]);
	}
	return(LOADINTELWORD(mem + address));
}

UINT32 MEMCALL memtram_rd32(UINT32 address){
	UINT32 r = (UINT32)memtram_rd16(address);
	r |= (UINT32)memtram_rd16(address+2) << 16;
	return r;
}

void MEMCALL memtram_wr8(UINT32 address, REG8 value) {
	

	CPU_REMCLOCK -= MEMWAIT_TRAM;
	if (address < 0xa2000) {
		mem[address] = (UINT8)value;
		tramupdate[LOW12(address >> 1)] |= 1;
		gdcs.textdisp |= 1;
	}
	else if (address < 0xa3fe0) {
		if (!(address & 1)) {
			mem[address] = (UINT8)value;
			tramupdate[LOW12(address >> 1)] |= 3;
			gdcs.textdisp |= 1;
		}
	}
	else if (address < 0xa4000) {
		if (!(address & 1)) {
			if ((!(address & 2)) || (gdcs.msw_accessable)) {
				mem[address] = (UINT8)value;
				tramupdate[LOW12(address >> 1)] |= 3;
				gdcs.textdisp |= 1;
			}
		}
	}
	else if (address < 0xa5000) {
		if ((address & 1) && (cgwindow.writable & 1)) {
			cgwindow.writable |= 0x80;
			fontrom[cgwindow.high + ((address >> 1) & 0x0f)] = (UINT8)value;
		}
	}
}

void MEMCALL memtram_wr16(UINT32 address, REG16 value) {

#if defined(SUPPORT_TEXTHOOK)
	if(np2cfg.usetexthook){
		UINT16 SJis;
		UINT8 th[3];
		UINT16 thw[2];
		thw[1]='\0';
		if (address & 2){
			SJis=font_Jis2Sjis(((value + 0x20) << 8) | (value >> 8));
			if(SJis){
				th[0] = SJis >> 8; th[1] = SJis & 0x00ff; th[2] = '\0';
				codecnv_sjistoucs2(thw, 1, (const char*)th, 2);
				font_outhooktest((wchar_t*)thw);
			}
		}else if (!(address & 3)){
			SJis=font_Jis2Sjis2(((value + 0x20) << 8) | (value >> 8));
			if(SJis){
				th[0] = SJis >> 8; th[1] = SJis & 0x00ff; th[2] = '\0';
				codecnv_sjistoucs2(thw, 1, (const char*)th, 2);
				font_outhooktest((wchar_t*)thw);
			}
		}
	}
#endif

	CPU_REMCLOCK -= MEMWAIT_TRAM;
	if (address < 0xa1fff) {
		STOREINTELWORD(mem + address, value);
		tramupdate[LOW12(address >> 1)] |= 1;
		tramupdate[LOW12((address + 1) >> 1)] |= 1;
		gdcs.textdisp |= 1;
	}
	else if (address == 0xa1fff) {
		STOREINTELWORD(mem + address, value);
		tramupdate[0] |= 3;
		tramupdate[0xfff] |= 1;
		gdcs.textdisp |= 1;
	}
	else if (address < 0xa3fe0) {
		if (address & 1) {
			address++;
			value >>= 8;
		}
		mem[address] = (UINT8)value;
		tramupdate[LOW12(address >> 1)] |= 3;
		gdcs.textdisp |= 3;
	}
	else if (address < 0xa3fff) {
		if (address & 1) {
			address++;
			value >>= 8;
		}
		if ((!(address & 2)) || (gdcs.msw_accessable)) {
			mem[address] = (UINT8)value;
			tramupdate[LOW12(address >> 1)] |= 3;
			gdcs.textdisp |= 3;
		}
	}
	else if (address < 0xa5000) {
		if (!(address & 1)) {
			value >>= 8;
		}
		if (cgwindow.writable & 1) {
			cgwindow.writable |= 0x80;
			fontrom[cgwindow.high + ((address >> 1) & 0x0f)] = (UINT8)value;
		}
	}
}

void MEMCALL memtram_wr32(UINT32 address, UINT32 value){
	memtram_wr16(address, (REG16)value);
	memtram_wr16(address+2, (REG16)(value >> 16));
}

