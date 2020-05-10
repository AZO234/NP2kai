/*
 * Copyright (c) 2019 SimK
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"timing.h"
#include	"dmax86.h"
#include	"bios/bios.h"
#include	"vram/vram.h"
#include	"wab/cirrus_vga_extern.h"

#if defined(SUPPORT_IA32_HAXM)

#if 1
#undef	TRACEOUT
//#define USE_TRACEOUT_VS
#ifdef USE_TRACEOUT_VS
static void trace_fmt_ex(const char *fmt, ...)
{
	char stmp[2048];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(stmp, fmt, ap);
	strcat(stmp, "\n");
	va_end(ap);
	OutputDebugStringA(stmp);
}
#define	TRACEOUT(s)	trace_fmt_ex s
#else
#define	TRACEOUT(s)	(void)(s)
#endif
#endif	/* 1 */
#include	"haxfunc.h"
#include	"haxcore.h"
#include	"np2_tickcount.h"

#if defined(_WINDOWS)
#include	<process.h>
#else
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#endif

NP2_HAX			np2hax = {0};
NP2_HAX_STAT	np2haxstat = {0};
NP2_HAX_CORE	np2haxcore = {0};

static void make_vm_str(OEMCHAR* buf, UINT32 vm_id){
	_stprintf(buf, OEMTEXT("\\\\.\\hax_vm%02d"), vm_id);
}
static void make_vcpu_str(OEMCHAR* buf, UINT32 vm_id, UINT32 vcpu_id){
	_stprintf(buf, OEMTEXT("\\\\.\\hax_vm%02d_vcpu%02d"), vm_id, vcpu_id);
}

// HAXMが使えそうかチェック
UINT8 i386hax_check(void) {
	
	HAX_MODULE_VERSION haxver = {0};
	HAX_CAPINFO haxcap = {0};

#if defined(_WINDOWS)
	// HAXMカーネルモードドライバを開く
	np2hax.hDevice = CreateFile(OEMTEXT("\\\\.\\HAX"), 
		GENERIC_READ|GENERIC_WRITE, 0, NULL, 
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (np2hax.hDevice == INVALID_HANDLE_VALUE) {
        msgbox("HAXM check", "Failed to initialize the HAX device.");
#else
  np2hax.hDevice = open("/dev/HAX", O_RDWR); 
	if (np2hax.hDevice == -1) {
#endif
		TRACEOUT(("HAXM: Failed to initialize the HAX device."));
#if defined(_WINDOWS)
        msgbox("HAXM check", "Failed to initialize the HAX device.");
#endif
		np2hax.available = 0;
		np2hax.hDevice = NULL;
        return FAILURE;
	}
	
	// HAXMバージョンを取得
	if(i386haxfunc_getversion(&haxver)==SUCCESS){
		char buf[256] = {0};
		TRACEOUT(("HAXM: HAX getversion: compatible version %d, current version %d", haxver.compat_version, haxver.current_version));
		//sprintf(buf, "HAXM: HAX getversion: compatible version %d, current version %d", haxver.compat_version, haxver.current_version);
		//msgbox("HAXM check", buf);
		//msgbox("HAXM init", "HAX getversion succeeded.");
	}else{
		TRACEOUT(("HAXM: HAX getversion failed."));
		msgbox("HAXM check", "HAX getversion failed.");
		goto initfailure;
	}
	
	// HAXMで使える機能を調査
	if(i386haxfunc_getcapability(&haxcap)==SUCCESS){
		char buf[256] = {0};
		TRACEOUT(("HAXM: HAX getcapability: wstatus 0x%.4x, winfo 0x%.4x, win_refcount %d, mem_quota %d", haxcap.wstatus, haxcap.winfo, haxcap.win_refcount, haxcap.mem_quota));
		//sprintf(buf, "HAXM: HAX getcapability: wstatus 0x%.4x, winfo 0x%.4x, win_refcount %d, mem_quota %d", haxcap.wstatus, haxcap.winfo, haxcap.win_refcount, haxcap.mem_quota);
		//msgbox("HAXM check", buf);
		if(!(haxcap.wstatus & HAX_CAP_WORKSTATUS_MASK)){
			if(haxcap.winfo & HAX_CAP_FAILREASON_VT){
				msgbox("HAXM check", "Error: Intel VT-x is not supported by the host CPU.");
			}else{
				msgbox("HAXM check", "Error: Intel Execute Disable Bit is not supported by the host CPU.");
			}
			goto initfailure;
		}
		if(!(haxcap.winfo & HAX_CAP_FASTMMIO)){
			msgbox("HAXM check", "Error: Fast MMIO is not supported. Please update HAXM.");
			goto initfailure;
		}
		if(!(haxcap.winfo & HAX_CAP_DEBUG)){
			msgbox("HAXM check", "Error: Debugging function is not supported. Please update HAXM.");
			goto initfailure;
		}
		//msgbox("HAXM init", "HAX getversion succeeded.");
	}else{
		TRACEOUT(("HAXM: HAX getcapability failed."));
		msgbox("HAXM check", "HAX getcapability failed.");
		goto initfailure;
	}
	
#if defined(_WINDOWS)
	CloseHandle(np2hax.hDevice);
#else
	close(np2hax.hDevice);
#endif
	np2hax.hDevice = NULL;

	TRACEOUT(("HAXM: HAX available."));
    //msgbox("HAXM check", "HAX available.");

	np2hax.available = 1;

	return SUCCESS;

initfailure:
#if defined(_WINDOWS)
	CloseHandle(np2hax.hDevice);
#else
	close(np2hax.hDevice);
#endif
	np2hax.available = 0;
	np2hax.enable = 0;
	np2hax.hDevice = NULL;
	return FAILURE;
}

void i386hax_initialize(void) {
	
	if(!np2hax.available) return;
	if(!np2hax.enable) return;

	i386hax_deinitialize();
	
#if defined(_WINDOWS)
	// HAXMカーネルモードドライバを開く
	np2hax.hDevice = CreateFile(OEMTEXT("\\\\.\\HAX"), 
		GENERIC_READ|GENERIC_WRITE, 0, NULL, 
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (np2hax.hDevice == INVALID_HANDLE_VALUE) {
#else
  np2hax.hDevice = open("/dev/HAX", O_RDWR); 
	if (np2hax.hDevice == -1) {
#endif
		TRACEOUT(("HAXM: Failed to initialize the HAX device."));
        return;
	}

	TRACEOUT(("HAXM: HAX initialized."));
    //msgbox("HAXM init", "HAX initialized.");
	
	return;
	
error1:
	CloseHandle(np2hax.hDevice);
	np2hax.hDevice = NULL;
	return;
}

#if !defined(_WINDOWS)
static char *hax_vm_devfs_string(int vm_id)
{
    char *name;

    if (vm_id > MAX_VM_ID) {
        fprintf(stderr, "Too big VM id\n");
        return NULL;
    }

#define HAX_VM_DEVFS "/dev/hax_vm/vmxx"
    name = strdup(HAX_VM_DEVFS);
    if (!name) {
        return NULL;
    }

    snprintf(name, sizeof HAX_VM_DEVFS, "/dev/hax_vm/vm%02d", vm_id);
    return name;
}

static char *hax_vcpu_devfs_string(int vm_id, int vcpu_id)
{
    char *name;

    if (vm_id > MAX_VM_ID || vcpu_id > MAX_VCPU_ID) {
        fprintf(stderr, "Too big vm id %x or vcpu id %x\n", vm_id, vcpu_id);
        return NULL;
    }

#define HAX_VCPU_DEVFS "/dev/hax_vmxx/vcpuxx"
    name = strdup(HAX_VCPU_DEVFS);
    if (!name) {
        return NULL;
    }

    snprintf(name, sizeof HAX_VCPU_DEVFS, "/dev/hax_vm%02d/vcpu%02d",
             vm_id, vcpu_id);
    return name;
}
#endif

void i386hax_createVM(void) {
	
#if defined(_WINDOWS)
	OEMCHAR vm_str[64] = {0};
	OEMCHAR vcpu_str[64] = {0};
#else
	char* vm_str;
	char* vcpu_str;
#endif
	HAX_DEBUG hax_dbg = {0};
	HAX_QEMU_VERSION hax_qemu_ver = {0};

	if(!np2hax.available) return;
	if(!np2hax.enable) return;
	if(!np2hax.hDevice) return;

	i386hax_disposeVM();
	
	// HAXM仮想マシンを作成
	if(i386haxfunc_createVM(&np2hax.vm_id)==FAILURE){
		TRACEOUT(("HAXM: HAX create VM failed."));
		msgbox("HAXM VM", "HAX create VM failed.");
		return;
	}

#if defined(_WINDOWS)
	make_vm_str(vm_str, np2hax.vm_id);
	
	// HAXM仮想マシンを開く
	np2hax.hVMDevice = CreateFile(vm_str, 
		GENERIC_READ|GENERIC_WRITE, 0, NULL, 
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (np2hax.hVMDevice == INVALID_HANDLE_VALUE) {
#else
	vm_str = hax_vm_devfs_string(np2hax.vm_id);

  np2hax.hVMDevice = open(vm_str, O_RDWR); 
	if (np2hax.hVMDevice == -1) {
#endif
		TRACEOUT(("HAXM: Failed to initialize the HAX VM device."));
		return;
	}
	
	// あまり意味なし？
	//hax_qemu_ver.cur_version = 0x0;
	//if(i386haxfunc_notifyQEMUversion(&hax_qemu_ver)==FAILURE){
	//	TRACEOUT(("HAXM: HAX notify QEMU version failed."));
	//	msgbox("HAXM VM", "HAX notify QEMU version failed.");
	//	goto error2;
	//}
	
	// 仮想CPUを作成
	if(i386haxfunc_VCPUcreate(0)==FAILURE){
		TRACEOUT(("HAXM: HAX create VCPU failed."));
		msgbox("HAXM VM", "HAX create VCPU failed.");
		goto error2;
	}
	
#if defined(_WINDOWS)
	make_vcpu_str(vcpu_str, np2hax.vm_id, 0);
	
	// 仮想CPUを開く
	np2hax.hVCPUDevice = CreateFile(vcpu_str, 
		GENERIC_READ|GENERIC_WRITE, 0, NULL, 
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (np2hax.hVCPUDevice == INVALID_HANDLE_VALUE) {
#else
	vcpu_str = hax_vcpu_devfs_string(np2hax.vm_id, 0);

  np2hax.hVCPUDevice = open(vcpu_str, O_RDWR); 
	if (np2hax.hVCPUDevice == -1) {
#endif
		TRACEOUT(("HAXM: Failed to initialize the HAX VCPU device."));
		goto error2;
	}
	
	// 仮想CPUとデータをやりとりするのための場所を確保
	if(i386haxfunc_vcpu_setup_tunnel(&np2hax.tunnel)==FAILURE){
		TRACEOUT(("HAXM: HAX VCPU setup tunnel failed."));
		msgbox("HAXM VM", "HAX VCPU setup tunnel failed.");
		goto error2;
	}
	
	// デバッグ機能を有効にする（HAX_DEBUG_USE_SW_BPは仮想マシンがINT3 CChを呼ぶと無条件で仮想マシンモニタに移行できる）
	np2hax.bioshookenable = 1;
	hax_dbg.control = HAX_DEBUG_ENABLE|HAX_DEBUG_USE_SW_BP;//|HAX_DEBUG_USE_HW_BP|HAX_DEBUG_STEP;
	if(i386haxfunc_vcpu_debug(&hax_dbg)==FAILURE){
		TRACEOUT(("HAXM: HAX VCPU debug setting failed."));
		msgbox("HAXM VM", "HAX VCPU debug setting failed.");
		goto error2;
	}
	
	TRACEOUT(("HAXM: HAX VM initialized."));
    //msgbox("HAXM VM", "HAX VM initialized.");
	
	memset(&np2haxcore, 0, sizeof(np2haxcore));
	
	// デフォルトのレジスタ設定を覚えておく
	i386haxfunc_vcpu_getREGs(&np2haxstat.state);
	i386haxfunc_vcpu_getFPU(&np2haxstat.fpustate);
	memcpy(&np2haxstat.default_state, &np2haxstat.state, sizeof(np2haxstat.state));
	memcpy(&np2haxstat.default_fpustate, &np2haxstat.fpustate, sizeof(np2haxstat.fpustate));

	return;
	
error3:
#if defined(_WINDOWS)
	CloseHandle(np2hax.hVCPUDevice);
#else
	close(np2hax.hVCPUDevice);
#endif
	np2hax.hVCPUDevice = NULL;
error2:
#if defined(_WINDOWS)
	CloseHandle(np2hax.hVMDevice);
#else
	close(np2hax.hVMDevice);
#endif
	np2hax.hVMDevice = NULL;
error1:
	return;
}

void i386hax_resetVMMem(void) {
	
	// メモリ周りの初期設定
	np2haxcore.lastA20en = (CPU_ADRSMASK != 0x000fffff);
	i386hax_vm_sethmemory(np2haxcore.lastA20en);
	np2haxcore.lastITFbank = CPU_ITFBANK;
	i386hax_vm_setitfmemory(np2haxcore.lastITFbank);
	np2haxcore.lastVGA256linear = (vramop.mio2[0x2]==0x1);
	i386hax_vm_setvga256linearmemory();
	np2haxcore.lastVRAMMMIO = 1;
	
	i386hax_vm_setmemoryarea(mem+0xA5000, 0xA5000, 0x3000);
	//i386hax_vm_setmemoryarea(mem+0xA8000, 0xA8000, 0x8000);
	//i386hax_vm_setmemoryarea(mem+0xB0000, 0xB0000, 0x10000);
	//i386hax_vm_setmemoryarea(mem+0xC0000, 0xC0000, 0x10000);
	i386hax_vm_setmemoryarea(mem+0xD0000, 0xD0000, 0x10000);
	//i386hax_vm_setmemoryarea(mem+0xE0000, 0xE0000, 0x8000);
	i386hax_vm_setmemoryarea(mem+0xE8000, 0xE8000, 0x8000);
	i386hax_vm_setmemoryarea(mem+0xF0000, 0xF0000, 0x8000);
	
	//i386hax_vm_setmemoryarea(mem+0xA5000, 0xA5000, 0x3000);
	//i386hax_vm_setmemoryarea(mem+0xA8000, 0xA8000, 0x8000);
	//i386hax_vm_setmemoryarea(mem+0xB0000, 0xB0000, 0x10000);
	//i386hax_vm_setmemoryarea(mem+0xC0000, 0xC0000, 0x10000);
	//i386hax_vm_setmemoryarea(mem+0xD0000, 0xD0000, 0x10000);
	//i386hax_vm_setmemoryarea(mem+0xE0000, 0xE0000, 0x8000);
	//i386hax_vm_setmemoryarea(mem+0xE8000, 0xE8000, 0x8000);
	//i386hax_vm_setmemoryarea(mem+0xF0000, 0xF0000, 0x8000);
}

void i386hax_resetVMCPU(void) {
	
	// CPU周りの初期設定
	np2haxstat.irq_reqidx_cur = np2haxstat.irq_reqidx_end = 0;
	
	memcpy(&np2haxstat.state, &np2haxstat.default_state, sizeof(np2haxstat.state));
	memcpy(&np2haxstat.fpustate, &np2haxstat.default_fpustate, sizeof(np2haxstat.fpustate));
	
	np2haxstat.update_regs = np2haxstat.update_fpu = 1;
	np2haxstat.update_segment_regs = 1;
	
	np2haxcore.clockpersec = NP2_TickCount_GetFrequency();
	np2haxcore.lastclock = NP2_TickCount_GetCount();
	np2haxcore.clockcount = NP2_TickCount_GetCount();
}

/*
 * bios call interface (HAXM)
 */
static int
ia32hax_bioscall(void)
{
	UINT32 adrs;
	int ret = 0;

	if (!CPU_STAT_PM || CPU_STAT_VM86) {
#if 1
		adrs = CPU_PREV_EIP + (CPU_CS << 4);
#else
		adrs = CPU_PREV_EIP + CPU_STAT_CS_BASE;
#endif
		if ((adrs >= 0xf8000) && (adrs < 0x100000)) {
			if (biosfunc(adrs)) {
				/* Nothing to do */
				ret = 1;
			}
			LOAD_SEGREG(CPU_ES_INDEX, CPU_ES);
			LOAD_SEGREG(CPU_CS_INDEX, CPU_CS);
			LOAD_SEGREG(CPU_SS_INDEX, CPU_SS);
			LOAD_SEGREG(CPU_DS_INDEX, CPU_DS);
		}
	}
	return ret;
}

#define	TSS_16_SIZE	44
#define	TSS_16_LIMIT	(TSS_16_SIZE - 1)
#define	TSS_32_SIZE	104
#define	TSS_32_LIMIT	(TSS_32_SIZE - 1)

static void CPUCALL
ia32hax_setHAXtoNP2IOADDR()
{
	UINT16 iobase;
	
	if (CPU_STAT_PM && !CPU_STAT_VM86) {
		/* check descriptor type & stack room size */
		switch (CPU_TR_DESC.type) {
		case CPU_SYSDESC_TYPE_TSS_16:
		case CPU_SYSDESC_TYPE_TSS_BUSY_16:
			if (CPU_TR_DESC.u.seg.limit < TSS_16_LIMIT) {
				return;
			}
			iobase = 0;
			break;

		case CPU_SYSDESC_TYPE_TSS_32:
		case CPU_SYSDESC_TYPE_TSS_BUSY_32:
			if (CPU_TR_DESC.u.seg.limit < TSS_32_LIMIT) {
				return;
			}
			iobase = cpu_kmemoryread_w(CPU_TR_DESC.u.seg.segbase + 102);
				//return;
			break;

		default:
			return;
		}

		/* I/O deny bitmap */
		CPU_STAT_IOLIMIT = 0;
		if (CPU_TR_DESC.type == CPU_SYSDESC_TYPE_TSS_BUSY_32) {
			if (iobase < CPU_TR_LIMIT) {
				CPU_STAT_IOLIMIT = (UINT16)(CPU_TR_LIMIT - iobase);
				CPU_STAT_IOADDR = CPU_TR_BASE + iobase;
			}
		}
	}
}

// HAXレジスタ → NP2 IA-32 レジスタ
void
ia32hax_copyregHAXtoNP2(void)
{
	static UINT32 lasteflags = 0;

	CPU_EAX = np2haxstat.state._eax;
	CPU_EBX = np2haxstat.state._ebx;
	CPU_ECX = np2haxstat.state._ecx;
	CPU_EDX = np2haxstat.state._edx;

	CPU_ESI = np2haxstat.state._esi;
	CPU_EDI = np2haxstat.state._edi;

	CPU_CS = np2haxstat.state._cs.selector;
	CPU_DS = np2haxstat.state._ds.selector;
	CPU_ES = np2haxstat.state._es.selector;
	CPU_SS = np2haxstat.state._ss.selector;
	CPU_FS = np2haxstat.state._fs.selector;
	CPU_GS = np2haxstat.state._gs.selector;
	CS_BASE = np2haxstat.state._cs.base;
	DS_BASE = np2haxstat.state._ds.base;
	ES_BASE = np2haxstat.state._es.base;
	SS_BASE = np2haxstat.state._ss.base;
	FS_BASE = np2haxstat.state._fs.base;
	GS_BASE = np2haxstat.state._gs.base;

	CPU_CS_DESC.u.seg.limit = np2haxstat.state._cs.limit;
	CPU_CS_DESC.type = np2haxstat.state._cs.type;
	CPU_CS_DESC.s = np2haxstat.state._cs.desc;
	CPU_CS_DESC.dpl = np2haxstat.state._cs.dpl;
	CPU_CS_DESC.rpl = np2haxstat.state._cs.selector & 0x3;
	CPU_CS_DESC.p = np2haxstat.state._cs.present;
	CPU_CS_DESC.valid = np2haxstat.state._cs.available;
	CPU_CS_DESC.d = np2haxstat.state._cs.operand_size;
	CPU_CS_DESC.u.seg.g = np2haxstat.state._cs.granularity;
	
	CPU_DS_DESC.u.seg.limit = np2haxstat.state._ds.limit;
	CPU_DS_DESC.type = np2haxstat.state._ds.type;
	CPU_DS_DESC.s = np2haxstat.state._ds.desc;
	CPU_DS_DESC.dpl = np2haxstat.state._ds.dpl;
	CPU_DS_DESC.rpl = np2haxstat.state._ds.selector & 0x3;
	CPU_DS_DESC.p = np2haxstat.state._ds.present;
	CPU_DS_DESC.valid = np2haxstat.state._ds.available;
	CPU_DS_DESC.d = np2haxstat.state._ds.operand_size;
	CPU_DS_DESC.u.seg.g = np2haxstat.state._ds.granularity;
	
	CPU_ES_DESC.u.seg.limit = np2haxstat.state._es.limit;
	CPU_ES_DESC.type = np2haxstat.state._es.type;
	CPU_ES_DESC.s = np2haxstat.state._es.desc;
	CPU_ES_DESC.dpl = np2haxstat.state._es.dpl;
	CPU_ES_DESC.rpl = np2haxstat.state._es.selector & 0x3;
	CPU_ES_DESC.p = np2haxstat.state._es.present;
	CPU_ES_DESC.valid = np2haxstat.state._es.available;
	CPU_ES_DESC.d = np2haxstat.state._es.operand_size;
	CPU_ES_DESC.u.seg.g = np2haxstat.state._es.granularity;
	
	CPU_SS_DESC.u.seg.limit = np2haxstat.state._ss.limit;
	CPU_SS_DESC.type = np2haxstat.state._ss.type;
	CPU_SS_DESC.s = np2haxstat.state._ss.desc;
	CPU_SS_DESC.dpl = np2haxstat.state._ss.dpl;
	CPU_SS_DESC.rpl = np2haxstat.state._ss.selector & 0x3;
	CPU_SS_DESC.p = np2haxstat.state._ss.present;
	CPU_SS_DESC.valid = np2haxstat.state._ss.available;
	CPU_SS_DESC.d = np2haxstat.state._ss.operand_size;
	CPU_SS_DESC.u.seg.g = np2haxstat.state._ss.granularity;
	
	CPU_FS_DESC.u.seg.limit = np2haxstat.state._fs.limit;
	CPU_FS_DESC.type = np2haxstat.state._fs.type;
	CPU_FS_DESC.s = np2haxstat.state._fs.desc;
	CPU_FS_DESC.dpl = np2haxstat.state._fs.dpl;
	CPU_FS_DESC.rpl = np2haxstat.state._fs.selector & 0x3;
	CPU_FS_DESC.p = np2haxstat.state._fs.present;
	CPU_FS_DESC.valid = np2haxstat.state._fs.available;
	CPU_FS_DESC.d = np2haxstat.state._fs.operand_size;
	CPU_FS_DESC.u.seg.g = np2haxstat.state._fs.granularity;
	
	CPU_GS_DESC.u.seg.limit = np2haxstat.state._gs.limit;
	CPU_GS_DESC.type = np2haxstat.state._gs.type;
	CPU_GS_DESC.s = np2haxstat.state._gs.desc;
	CPU_GS_DESC.dpl = np2haxstat.state._gs.dpl;
	CPU_GS_DESC.rpl = np2haxstat.state._gs.selector & 0x3;
	CPU_GS_DESC.p = np2haxstat.state._gs.present;
	CPU_GS_DESC.valid = np2haxstat.state._gs.available;
	CPU_GS_DESC.d = np2haxstat.state._gs.operand_size;
	CPU_GS_DESC.u.seg.g = np2haxstat.state._gs.granularity;
	
	CPU_EBP = np2haxstat.state._ebp;
	CPU_ESP = np2haxstat.state._esp;
	CPU_EIP = np2haxstat.state._eip;
	CPU_PREV_EIP = np2haxstat.state._eip; // XXX: あんまり良くない
	
	//if(!(lasteflags & I_FLAG) && (np2haxstat.state._eflags & I_FLAG)){
	//	TRACEOUT(("I_FLAG on"));
	//}else if((lasteflags & I_FLAG) && !(np2haxstat.state._eflags & I_FLAG)){
	//	TRACEOUT(("I_FLAG off"));
	//}
	CPU_EFLAG = np2haxstat.state._eflags;
	lasteflags = CPU_EFLAG;
	
	CPU_CR0 = np2haxstat.state._cr0;
	//CPU_CR1 = np2haxstat.state._cr1;
	CPU_CR2 = np2haxstat.state._cr2;
	CPU_CR3 = np2haxstat.state._cr3;
	CPU_CR4 = np2haxstat.state._cr4;
	
	CPU_GDTR_BASE = np2haxstat.state._gdt.base;
	CPU_GDTR_LIMIT = np2haxstat.state._gdt.limit;
	CPU_IDTR_BASE = np2haxstat.state._idt.base;
	CPU_IDTR_LIMIT = np2haxstat.state._idt.limit;
	CPU_LDTR = np2haxstat.state._ldt.selector;
	CPU_LDTR_BASE = np2haxstat.state._ldt.base;
	CPU_LDTR_LIMIT = np2haxstat.state._ldt.limit;
	CPU_LDTR_DESC.type = np2haxstat.state._ldt.type;
	CPU_LDTR_DESC.s = np2haxstat.state._ldt.desc;
	CPU_LDTR_DESC.dpl = np2haxstat.state._ldt.dpl;
	CPU_LDTR_DESC.p = np2haxstat.state._ldt.present;
	CPU_LDTR_DESC.valid = np2haxstat.state._ldt.available;
	CPU_LDTR_DESC.d = np2haxstat.state._ldt.operand_size;
	CPU_LDTR_DESC.u.seg.g = np2haxstat.state._ldt.granularity;
	CPU_TR = np2haxstat.state._tr.selector;
	CPU_TR_BASE = np2haxstat.state._tr.base;
	CPU_TR_LIMIT = np2haxstat.state._tr.limit;
	CPU_TR_DESC.type = np2haxstat.state._tr.type;
	CPU_TR_DESC.s = np2haxstat.state._tr.desc;
	CPU_TR_DESC.dpl = np2haxstat.state._tr.dpl;
	CPU_TR_DESC.p = np2haxstat.state._tr.present;
	CPU_TR_DESC.valid = np2haxstat.state._tr.available;
	CPU_TR_DESC.d = np2haxstat.state._tr.operand_size;
	CPU_TR_DESC.u.seg.g = np2haxstat.state._tr.granularity;

	CPU_DR(0) = np2haxstat.state._dr0;
	CPU_DR(1) = np2haxstat.state._dr1;
	CPU_DR(2) = np2haxstat.state._dr2;
	CPU_DR(3) = np2haxstat.state._dr3;
	//CPU_DR(4) = np2haxstat.state._dr4;
	//CPU_DR(5) = np2haxstat.state._dr5;
	CPU_DR(6) = np2haxstat.state._dr6;
	CPU_DR(7) = np2haxstat.state._dr7;
	
	CPU_STAT_PM = (np2haxstat.state._cr0 & 0x1)!=0;
	CPU_STAT_PAGING = (CPU_CR0 & CPU_CR0_PG)!=0;
	CPU_STAT_VM86 = (np2haxstat.state._eflags & VM_FLAG)!=0;
	CPU_STAT_WP = (CPU_CR0 & CPU_CR0_WP) ? 0x10 : 0;
	CPU_STAT_CPL = (UINT8)CPU_CS_DESC.rpl;
	CPU_STAT_USER_MODE = (CPU_CS_DESC.rpl == 3) ? CPU_MODE_USER : CPU_MODE_SUPERVISER;
	//CPU_STAT_PDE_BASE = CPU_CR3 & CPU_CR3_PD_MASK;
	CPU_STAT_PDE_BASE = np2haxstat.state._pde;
	
	//ia32hax_setHAXtoNP2IOADDR();
}
// NP2 IA-32 レジスタ → HAXレジスタ
void
ia32hax_copyregNP2toHAX(void)
{
	static int wacounter = 0;

	np2haxstat.state._eax = CPU_EAX;
	np2haxstat.state._ebx = CPU_EBX;
	np2haxstat.state._ecx = CPU_ECX;
	np2haxstat.state._edx = CPU_EDX;
	
	np2haxstat.state._esi = CPU_ESI;
	np2haxstat.state._edi = CPU_EDI;
	
	if(1||np2haxstat.update_segment_regs){
		np2haxstat.state._cs.selector = CPU_CS;
		np2haxstat.state._ds.selector = CPU_DS;
		np2haxstat.state._es.selector = CPU_ES;
		np2haxstat.state._ss.selector = CPU_SS;
		np2haxstat.state._fs.selector = CPU_FS;
		np2haxstat.state._gs.selector = CPU_GS;
		np2haxstat.state._cs.base = CS_BASE;
		np2haxstat.state._ds.base = DS_BASE;
		np2haxstat.state._es.base = ES_BASE;
		np2haxstat.state._ss.base = SS_BASE;
		np2haxstat.state._fs.base = FS_BASE;
		np2haxstat.state._gs.base = GS_BASE;

		np2haxstat.state._cs.base = CPU_CS_DESC.u.seg.segbase;
		np2haxstat.state._cs.limit = CPU_CS_DESC.u.seg.limit;
		//if(wacounter > 10)np2haxstat.state._cs.type = CPU_CS_DESC.type;
		np2haxstat.state._cs.desc = CPU_CS_DESC.s;
		np2haxstat.state._cs.dpl = CPU_CS_DESC.dpl;
		//if(wacounter > 10)np2haxstat.state._cs.selector = (np2haxstat.state._cs.selector & ~0x3) | CPU_CS_DESC.rpl;
		np2haxstat.state._cs.present = CPU_CS_DESC.p;
		np2haxstat.state._cs.available = CPU_CS_DESC.valid;
		np2haxstat.state._cs.operand_size = CPU_CS_DESC.d;
		np2haxstat.state._cs.granularity = CPU_CS_DESC.u.seg.g;
	
		np2haxstat.state._ds.base = CPU_DS_DESC.u.seg.segbase;
		np2haxstat.state._ds.limit = CPU_DS_DESC.u.seg.limit;
		//if(wacounter > 10)np2haxstat.state._ds.type = CPU_DS_DESC.type;
		np2haxstat.state._ds.desc = CPU_DS_DESC.s;
		np2haxstat.state._ds.dpl = CPU_DS_DESC.dpl;
		//if(wacounter > 10)np2haxstat.state._ds.selector = (np2haxstat.state._ds.selector & ~0x3) | CPU_CS_DESC.rpl;
		np2haxstat.state._ds.present = CPU_DS_DESC.p;
		np2haxstat.state._ds.available = CPU_DS_DESC.valid;
		np2haxstat.state._ds.operand_size = CPU_DS_DESC.d;
		np2haxstat.state._ds.granularity = CPU_DS_DESC.u.seg.g;
	
		np2haxstat.state._es.base = CPU_ES_DESC.u.seg.segbase;
		np2haxstat.state._es.limit = CPU_ES_DESC.u.seg.limit;
		//if(wacounter > 10)np2haxstat.state._es.type = CPU_ES_DESC.type;
		np2haxstat.state._es.desc = CPU_ES_DESC.s;
		np2haxstat.state._es.dpl = CPU_ES_DESC.dpl;
		//if(wacounter > 10)np2haxstat.state._es.selector = (np2haxstat.state._es.selector & ~0x3) | CPU_CS_DESC.rpl;
		np2haxstat.state._es.present = CPU_ES_DESC.p;
		np2haxstat.state._es.available = CPU_ES_DESC.valid;
		np2haxstat.state._es.operand_size = CPU_ES_DESC.d;
		np2haxstat.state._es.granularity = CPU_ES_DESC.u.seg.g;
	
		np2haxstat.state._ss.base = CPU_SS_DESC.u.seg.segbase;
		np2haxstat.state._ss.limit = CPU_SS_DESC.u.seg.limit;
		//if(wacounter > 10)np2haxstat.state._ss.type = CPU_SS_DESC.type;
		np2haxstat.state._ss.desc = CPU_SS_DESC.s;
		np2haxstat.state._ss.dpl = CPU_SS_DESC.dpl;
		//if(wacounter > 10)np2haxstat.state._ss.selector = (np2haxstat.state._ss.selector & ~0x3) | CPU_CS_DESC.rpl;
		np2haxstat.state._ss.present = CPU_SS_DESC.p;
		np2haxstat.state._ss.available = CPU_SS_DESC.valid;
		np2haxstat.state._ss.operand_size = CPU_SS_DESC.d;
		np2haxstat.state._ss.granularity = CPU_SS_DESC.u.seg.g;
	
		np2haxstat.state._fs.base = CPU_FS_DESC.u.seg.segbase;
		np2haxstat.state._fs.limit = CPU_FS_DESC.u.seg.limit;
		//if(wacounter > 10)np2haxstat.state._fs.type = CPU_FS_DESC.type;
		np2haxstat.state._fs.desc = CPU_FS_DESC.s;
		np2haxstat.state._fs.dpl = CPU_FS_DESC.dpl;
		//if(wacounter > 10)np2haxstat.state._fs.selector = (np2haxstat.state._fs.selector & ~0x3) | CPU_CS_DESC.rpl;
		np2haxstat.state._fs.present = CPU_FS_DESC.p;
		np2haxstat.state._fs.available = CPU_FS_DESC.valid;
		np2haxstat.state._fs.operand_size = CPU_FS_DESC.d;
		np2haxstat.state._fs.granularity = CPU_FS_DESC.u.seg.g;
	
		np2haxstat.state._gs.base = CPU_GS_DESC.u.seg.segbase;
		np2haxstat.state._gs.limit = CPU_GS_DESC.u.seg.limit;
		//if(wacounter > 10)np2haxstat.state._gs.type = CPU_GS_DESC.type;
		np2haxstat.state._gs.desc = CPU_GS_DESC.s;
		np2haxstat.state._gs.dpl = CPU_GS_DESC.dpl;
		//if(wacounter > 10)np2haxstat.state._gs.selector = (np2haxstat.state._gs.selector & ~0x3) | CPU_CS_DESC.rpl;
		np2haxstat.state._gs.present = CPU_GS_DESC.p;
		np2haxstat.state._gs.available = CPU_GS_DESC.valid;
		np2haxstat.state._gs.operand_size = CPU_GS_DESC.d;
		np2haxstat.state._gs.granularity = CPU_GS_DESC.u.seg.g;

		np2haxstat.update_segment_regs = 0;
	}
	
	np2haxstat.state._ebp = CPU_EBP;
	np2haxstat.state._esp = CPU_ESP;
	np2haxstat.state._eip = CPU_EIP;
	
	np2haxstat.state._eflags = CPU_EFLAG;
	
	np2haxstat.state._cr0 = CPU_CR0;
	//np2haxstat.state._cr1 = CPU_CR1;
	np2haxstat.state._cr2 = CPU_CR2;
	np2haxstat.state._cr3 = CPU_CR3;
	np2haxstat.state._cr4 = CPU_CR4;
	
	//if(wacounter > 10){
		np2haxstat.state._gdt.base = CPU_GDTR_BASE;
		np2haxstat.state._gdt.limit = CPU_GDTR_LIMIT;
		np2haxstat.state._idt.base = CPU_IDTR_BASE;
		np2haxstat.state._idt.limit = CPU_IDTR_LIMIT;
		np2haxstat.state._ldt.selector = CPU_LDTR;
		np2haxstat.state._ldt.base = CPU_LDTR_BASE;
		np2haxstat.state._ldt.limit = CPU_LDTR_LIMIT;
		np2haxstat.state._ldt.type = CPU_LDTR_DESC.type;
		np2haxstat.state._ldt.desc = CPU_LDTR_DESC.s;
		np2haxstat.state._ldt.dpl = CPU_LDTR_DESC.dpl;
		np2haxstat.state._ldt.present = CPU_LDTR_DESC.p;
		np2haxstat.state._ldt.available = CPU_LDTR_DESC.valid;
		np2haxstat.state._ldt.operand_size = CPU_LDTR_DESC.d;
		np2haxstat.state._ldt.granularity = CPU_LDTR_DESC.u.seg.g;
		//np2haxstat.state._tr.selector = CPU_TR;
		//np2haxstat.state._tr.base = CPU_TR_BASE;
		//np2haxstat.state._tr.limit = CPU_TR_LIMIT;
		//np2haxstat.state._tr.type = CPU_TR_DESC.type;
		//np2haxstat.state._tr.desc = CPU_TR_DESC.s;
		//np2haxstat.state._tr.dpl = CPU_TR_DESC.dpl;
		//np2haxstat.state._tr.present = CPU_TR_DESC.p;
		//np2haxstat.state._tr.available = CPU_TR_DESC.valid;
		//np2haxstat.state._tr.operand_size = CPU_TR_DESC.d;
		//np2haxstat.state._tr.granularity = CPU_TR_DESC.u.seg.g;
	//}else{
	//	wacounter++;
	//}

	//np2haxstat.state._dr0 = CPU_DR(0);
	//np2haxstat.state._dr1 = CPU_DR(1);
	//np2haxstat.state._dr2 = CPU_DR(2);
	//np2haxstat.state._dr3 = CPU_DR(3);
	////np2haxstat.state._dr4 = CPU_DR(4);
	////np2haxstat.state._dr5 = CPU_DR(5);
	//np2haxstat.state._dr6 = CPU_DR(6);
	//np2haxstat.state._dr7 = CPU_DR(7);
	
	//np2haxstat.state._pde = CPU_STAT_PDE_BASE;
}

// 仮想マシン実行
void i386hax_vm_exec_part(void) {
	HAX_TUNNEL *tunnel;
	UINT8 *iobuf;
	UINT32 exitstatus;
	UINT32 addr;
	SINT32 oldremclock;
#define PMCOUNTER_THRESHOLD	200000
	static UINT32 pmcounter = 0;
	static int restoreCounter = 0;

	np2haxcore.running = 1;

	if(np2hax.emumode){
		OEMCHAR vm_str[64] = {0};
		OEMCHAR vcpu_str[64] = {0};
		NP2_HAX_CORE tmphax;
		
		if(restoreCounter > 0){
			restoreCounter--;
			if (!(CPU_TYPE & CPUTYPE_V30)) {
				CPU_EXEC();
			}
			else {
				CPU_EXECV30();
			}
			np2haxstat.update_regs = 1;
			np2haxstat.update_fpu = 1;
		
			np2haxcore.running = 0;
			return;
		}

		memcpy(&tmphax, &np2haxcore, sizeof(np2haxcore));

		i386hax_createVM();

		memcpy(&np2haxcore, &tmphax, sizeof(np2haxcore));

		i386hax_resetVMCPU();
		
		memcpy(&np2haxstat.state, &np2haxstat.default_state, sizeof(np2haxstat.state));
		memcpy(&np2haxstat.fpustate, &np2haxstat.default_fpustate, sizeof(np2haxstat.fpustate));
	
		// HAXレジスタを更新
		ia32hax_copyregNP2toHAX();
		i386haxfunc_vcpu_setREGs(&np2haxstat.state);
		i386haxfunc_vcpu_setFPU(&np2haxstat.fpustate);
		//i386hax_disposeVM();
		//
		//// HAXM仮想マシンを開く
		//make_vm_str(vm_str, np2hax.vm_id);
		//np2hax.hVMDevice = CreateFile(vm_str, 
		//	GENERIC_READ|GENERIC_WRITE, 0, NULL, 
		//	CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		//if (np2hax.hVMDevice == INVALID_HANDLE_VALUE) {
		//	TRACEOUT(("HAXM: Failed to initialize the HAX VM device."));
		//	return;
		//}

		//// 仮想CPUを開く
		//make_vcpu_str(vcpu_str, np2hax.vm_id, 0);
		//np2hax.hVCPUDevice = CreateFile(vcpu_str, 
		//	GENERIC_READ|GENERIC_WRITE, 0, NULL, 
		//	CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		//if (np2hax.hVCPUDevice == INVALID_HANDLE_VALUE) {
		//	TRACEOUT(("HAXM: Failed to initialize the HAX VCPU device."));
		//}
	
		// 仮想CPUとデータをやりとりするのための場所を確保
		if(i386haxfunc_vcpu_setup_tunnel(&np2hax.tunnel)==FAILURE){
			TRACEOUT(("HAXM: HAX VCPU setup tunnel failed."));
		}
		i386hax_vm_allocmemory();
		if(np2hax.hVMDevice){
			i386hax_vm_setmemory();
			i386hax_vm_setbankmemory();
			i386hax_vm_setextmemory();
			i386haxfunc_vcpu_setREGs(&np2haxstat.state);
			i386haxfunc_vcpu_setFPU(&np2haxstat.fpustate);
			{
				HAX_MSR_DATA	msrstate_set = {0};
				i386haxfunc_vcpu_setMSRs(&np2haxstat.msrstate, &msrstate_set);
			}
			i386hax_vm_sethmemory(CPU_ADRSMASK != 0x000fffff);
			i386hax_vm_setitfmemory(CPU_ITFBANK);
			i386hax_vm_setvga256linearmemory();
	
			i386hax_vm_setmemoryarea(mem+0xA5000, 0xA5000, 0x3000);
			//i386hax_vm_setmemoryarea(mem+0xA8000, 0xA8000, 0x8000);
			//i386hax_vm_setmemoryarea(mem+0xB0000, 0xB0000, 0x10000);
			//i386hax_vm_setmemoryarea(mem+0xC0000, 0xC0000, 0x10000);
			i386hax_vm_setmemoryarea(mem+0xD0000, 0xD0000, 0x10000);
			//i386hax_vm_setmemoryarea(mem+0xE0000, 0xE0000, 0x8000);
			i386hax_vm_setmemoryarea(mem+0xE8000, 0xE8000, 0x8000);
			i386hax_vm_setmemoryarea(mem+0xF0000, 0xF0000, 0x8000);

			np2hax.emumode = 0;
		}
	}

	// HAXMとのデータやりとり用
	tunnel = (HAX_TUNNEL*)np2hax.tunnel.va;
	iobuf = (UINT8*)np2hax.tunnel.io_va;
	
	// 猫レジスタをHAXレジスタにコピー
	ia32hax_copyregNP2toHAX();
	
	// 必要ならHAXレジスタを更新
	if(np2haxstat.update_regs) i386haxfunc_vcpu_setREGs(&np2haxstat.state);
	if(np2haxstat.update_fpu) i386haxfunc_vcpu_setFPU(&np2haxstat.fpustate);
	
	np2haxcore.hltflag = 0;
	
coutinue_cpu:
	// メモリ絡みの構成が変わっていたら変える
	if(np2haxcore.lastA20en != (CPU_ADRSMASK != 0x000fffff)){
		i386hax_vm_sethmemory(CPU_ADRSMASK != 0x000fffff);
		np2haxcore.lastA20en = (CPU_ADRSMASK != 0x000fffff);
	}
	if(np2haxcore.lastITFbank != CPU_ITFBANK){
		i386hax_vm_sethmemory(CPU_ADRSMASK != 0x000fffff);
		i386hax_vm_setitfmemory(CPU_ITFBANK);
		np2haxcore.lastITFbank = CPU_ITFBANK;
	}
	if(np2haxcore.lastVGA256linear != (vramop.mio2[0x2]==0x1)){
		i386hax_vm_setvga256linearmemory();
		np2haxcore.lastVGA256linear = (vramop.mio2[0x2]==0x1);
	}
	
	// プロテクトモードが続いたらBIOSエミュレーション用のデバッグ設定を無効にする（デバッグレジスタを使うソフト用）
	if(CPU_STAT_PM && CPU_STAT_PAGING && !CPU_STAT_VM86){
		if(pmcounter < PMCOUNTER_THRESHOLD){
			pmcounter++;
		}else{
			if(np2hax.bioshookenable){
				HAX_DEBUG hax_dbg = {0};
				hax_dbg.control = 0;
				i386haxfunc_vcpu_debug(&hax_dbg);
				np2hax.bioshookenable = 0;
			}
		}
	}else{
		if(pmcounter > 0){
			if(!CPU_STAT_PM && !CPU_STAT_PAGING && !CPU_STAT_VM86){
				// リアルモードっぽいのでカウンタリセット
				pmcounter = 0;
			}else{
				pmcounter--;
			}
		}
		if(!np2hax.bioshookenable){
			HAX_DEBUG hax_dbg = {0};
			hax_dbg.control = HAX_DEBUG_ENABLE|HAX_DEBUG_USE_SW_BP;//|HAX_DEBUG_USE_HW_BP|HAX_DEBUG_STEP;
			i386haxfunc_vcpu_debug(&hax_dbg);
			np2hax.bioshookenable = 1;
		}
	}

	// 割り込みを処理
	if(np2haxstat.irq_reqidx_cur != np2haxstat.irq_reqidx_end){
		if (tunnel->ready_for_interrupt_injection) { // 割り込み準備OK？
			i386haxfunc_vcpu_interrupt(np2haxstat.irq_req[np2haxstat.irq_reqidx_cur]);
			np2haxstat.irq_reqidx_cur++;
		}
	}
	if(np2haxstat.irq_reqidx_cur != np2haxstat.irq_reqidx_end){ // 割り込みが必要ならrequest_interrupt_windowを1にする
		tunnel->request_interrupt_window = 1;
	}else{
		tunnel->request_interrupt_window = 0;
	}

	// リセット可能フラグをクリア
	np2haxcore.ready_for_reset = 0;
	
coutinue_cpu_imm:
	// HAXM CPU実行
	if(np2hax.emumode){
		tunnel->_exit_status = HAX_EXIT_STATECHANGE;
	}else{
		i386haxfunc_vcpu_run();
	}

	// 実行停止理由を取得
	exitstatus = tunnel->_exit_status;

	// CPU実行再開のための処理（高速処理が要るもの）
	switch (exitstatus) {
	case HAX_EXIT_IO: // I/Oポートへのアクセス
		//printf("HAX_EXIT_IO\n");
		oldremclock = CPU_REMCLOCK;
		switch(tunnel->io._direction){ // 入力 or 出力
		case HAX_IO_OUT: // 出力
			if(tunnel->io._df==0){ // 連続でデータを出力する場合の方向？
				int i;
				UINT8 *bufptr = iobuf;
				for(i=0;i<tunnel->io._count;i++){
					if(tunnel->io._size==1){ // 1byte
						iocore_out8(tunnel->io._port, *bufptr);
					}else if(tunnel->io._size==2){ // 2yte
						iocore_out16(tunnel->io._port, LOADINTELWORD(bufptr));
					}else{ // tunnel->io._size==4 // それ以外（実質4byte）
						iocore_out32(tunnel->io._port, LOADINTELDWORD(bufptr));
					}
					bufptr += tunnel->io._size;
				}
			}else{
				int i;
				UINT8 *bufptr = iobuf;
				bufptr += tunnel->io._size * (tunnel->io._count-1);
				for(i=tunnel->io._count-1;i>=0;i--){
					if(tunnel->io._size==1){ // 1byte
						iocore_out8(tunnel->io._port, *bufptr);
					}else if(tunnel->io._size==2){ // 2yte
						iocore_out16(tunnel->io._port, LOADINTELWORD(bufptr));
					}else{ // tunnel->io._size==4 // それ以外（実質4byte）
						iocore_out32(tunnel->io._port, LOADINTELDWORD(bufptr));
					}
					bufptr -= tunnel->io._size;
				}
			}
			break;
		case HAX_IO_IN: // 入力
			if(tunnel->io._df==0){ // 連続でデータを入力する場合の方向？
				int i;
				UINT8 *bufptr = iobuf;
				for(i=0;i<tunnel->io._count;i++){
					if(tunnel->io._size==1){
						*bufptr = iocore_inp8(tunnel->io._port); // 1byte
					}else if(tunnel->io._size==2){
						STOREINTELWORD(bufptr, iocore_inp16(tunnel->io._port)); // 2yte
					}else{ // tunnel->io._size==4
						STOREINTELDWORD(bufptr, iocore_inp32(tunnel->io._port)); // それ以外（実質4byte）
					}
					bufptr += tunnel->io._size;
				}
			}else{
				int i;
				UINT8 *bufptr = iobuf;
				bufptr += tunnel->io._size * (tunnel->io._count-1);
				for(i=tunnel->io._count-1;i>=0;i--){
					if(tunnel->io._size==1){
						*bufptr = iocore_inp8(tunnel->io._port); // 1byte
					}else if(tunnel->io._size==2){
						STOREINTELWORD(bufptr, iocore_inp16(tunnel->io._port)); // 2yte
					}else{ // tunnel->io._size==4
						STOREINTELDWORD(bufptr, iocore_inp32(tunnel->io._port)); // それ以外（実質4byte）
					}
					bufptr -= tunnel->io._size;
				}
			}
			break;
		}
		if(tunnel->io._port==0x1480){
			// ゲームポート読み取りは即座に返す
			goto coutinue_cpu_imm;
		}
		if(CPU_REMCLOCK==-1){
			break;
		}
		//CPU_REMCLOCK = oldremclock;
		
		// 高速化のために一部のI/Oポートの処理を簡略化
		//if(tunnel->io._port != 0x90 && tunnel->io._port != 0x92 && tunnel->io._port != 0x94 && 
		//	tunnel->io._port != 0xc8 && tunnel->io._port != 0xca && tunnel->io._port != 0xcc && tunnel->io._port != 0xbe && tunnel->io._port != 0x4be) {
		//	(0x430 <= tunnel->io._port && tunnel->io._port <= 0x64e && !(g_nevent.item[NEVENT_SASIIO].flag & NEVENT_ENABLE)) || 
		//	(0x7FD9 <= tunnel->io._port && tunnel->io._port <= 0x7FDF)){
			if(tunnel->io._port==0x640){
				// 厳しめにする
				if (np2haxcore.hurryup) {
					np2haxcore.hurryup = 0;
					break;
				}
			}
			{
				SINT32 diff; 
				np2haxcore.clockcount = NP2_TickCount_GetCount();
				diff = (np2haxcore.clockcount - np2haxcore.lastclock) * pccore.realclock / np2haxcore.clockpersec;
				if(CPU_REMCLOCK > 0){
					CPU_REMCLOCK -= diff;
				}
				np2haxcore.lastclock = np2haxcore.clockcount;
			}
	
			// 時間切れなら抜ける
			if (CPU_REMCLOCK <= 0) {
				break;
			}
			if (CPU_REMCLOCK > 0 && timing_getcount_baseclock()!=0) {
				CPU_REMCLOCK = 0;
				break;
			}
			i386haxfunc_vcpu_getREGs(&np2haxstat.state);
			i386haxfunc_vcpu_getFPU(&np2haxstat.fpustate);
			np2haxstat.update_regs = np2haxstat.update_fpu = 0;
	
			ia32hax_copyregHAXtoNP2();
			pic_irq();

			goto coutinue_cpu;
		//}
		break;
	case HAX_EXIT_FAST_MMIO: // メモリマップドI/Oへのアクセス
		//printf("HAX_EXIT_FAST_MMIO\n");
		{
			HAX_FASTMMIO *mmio = (HAX_FASTMMIO*)np2hax.tunnel.io_va;
				
			switch(mmio->direction){ // データREAD/WRITEの判定
			case 0: // データREAD
				if(mmio->size==1){
					mmio->value = memp_read8((UINT32)mmio->gpa);
				}else if(mmio->size==2){
					mmio->value = memp_read16((UINT32)mmio->gpa);
				}else{ // mmio->size==4
					mmio->value = memp_read32((UINT32)mmio->gpa);
				}
				break;
			case 1: // データWRITE
				if(mmio->size==1){
					memp_write8((UINT32)mmio->gpa, (UINT8)mmio->value);
				}else if(mmio->size==2){
					memp_write16((UINT32)mmio->gpa, (UINT16)mmio->value);
				}else{ // mmio->size==4
					memp_write32((UINT32)mmio->gpa, (UINT32)mmio->value);
				}
				break;
			default: // データREAD→データWRITE
				if(mmio->size==1){
					memp_write8((UINT32)mmio->gpa2, memp_read8((UINT32)mmio->gpa));
				}else if(mmio->size==2){
					memp_write16((UINT32)mmio->gpa2, memp_read16((UINT32)mmio->gpa));
				}else{ // mmio->size==4
					memp_write32((UINT32)mmio->gpa2, memp_read32((UINT32)mmio->gpa));
				}
				break;
			}
		}
		{
			SINT32 diff; 
			np2haxcore.clockcount = NP2_TickCount_GetCount();
			diff = (np2haxcore.clockcount - np2haxcore.lastclock) * pccore.realclock / np2haxcore.clockpersec;
			if(CPU_REMCLOCK > 0){
				CPU_REMCLOCK -= diff;
			}
			np2haxcore.lastclock = np2haxcore.clockcount;
		}
	
		// 時間切れなら抜ける
		if (CPU_REMCLOCK <= 0) {
			break;
		}
		if (CPU_REMCLOCK > 0 && timing_getcount_baseclock()!=0) {
			CPU_REMCLOCK = 0;
			break;
		}
		i386haxfunc_vcpu_getREGs(&np2haxstat.state);
		i386haxfunc_vcpu_getFPU(&np2haxstat.fpustate);
		np2haxstat.update_regs = np2haxstat.update_fpu = 0;
	
		ia32hax_copyregHAXtoNP2();
		pic_irq();
		goto coutinue_cpu;
		break;
	}
	
	// HAXMレジスタを読み取り
	i386haxfunc_vcpu_getREGs(&np2haxstat.state);
	i386haxfunc_vcpu_getFPU(&np2haxstat.fpustate);
	np2haxstat.update_regs = np2haxstat.update_fpu = 0;
	
	// HAXMレジスタ→猫レジスタにコピー
	ia32hax_copyregHAXtoNP2();
	
	// CPU実行再開のための処理（処理速度はどうでもいい物）
	switch (exitstatus) {
	case HAX_EXIT_MMIO: // メモリマップドI/Oへのアクセス（旧式・もう呼ばれないと思う）
		//printf("HAX_EXIT_MMIO\n");
		break;
	case HAX_EXIT_REALMODE: // 事前設定をするとリアルモードの時に呼ばれるらしい
		//printf("HAX_EXIT_REALMODE\n");
		break;
	case HAX_EXIT_INTERRUPT: // 割り込み･･･？
		//printf("HAX_EXIT_INTERRUPT\n");
		// リセット可能フラグを立てる
		np2haxcore.ready_for_reset = 1;
		break;
	case HAX_EXIT_UNKNOWN: // 謎
		//printf("HAX_EXIT_UNKNOWN\n");
		//if(!CPU_RESETREQ){
		//	np2hax.emumode = 1;
		//	CPU_REMCLOCK = 0;
		//	if (!(CPU_TYPE & CPUTYPE_V30)) {
		//		CPU_EXEC();
		//	}
		//	else {
		//		CPU_EXECV30();
		//	}
		//	np2haxstat.update_regs = 1;
		//	np2haxstat.update_fpu = 1;
		//}
		break;
	case HAX_EXIT_HLT: // HLT命令が実行されたとき
		np2haxcore.hltflag = 1;
		//printf("HAX_EXIT_HLT\n");
		// リセット可能フラグを立てる
		np2haxcore.ready_for_reset = 1;
		break;
	case HAX_EXIT_STATECHANGE: // CPU状態が変わったとき･･･と言いつつ、事実上CPUが実行不能(panic)になったときしか呼ばれない
		//printf("HAX_EXIT_STATECHANGE\n");
		// リセット可能フラグを立てる
		np2haxcore.ready_for_reset = 1;
		//if(!CPU_RESETREQ){
		//	np2hax.emumode = 1;
		//	//CPU_REMCLOCK = 0;
		//	if (!(CPU_TYPE & CPUTYPE_V30)) {
		//		CPU_EXEC();
		//	}
		//	else {
		//		CPU_EXECV30();
		//	}
		//	np2haxstat.update_regs = 1;
		//	np2haxstat.update_fpu = 1;
		//	restoreCounter = 100000;
		//}
		//{
		//	int i;
		//	UINT32 memdumpa[0x100];
		//	UINT8 memdump[0x100];
		//	UINT32 baseaddr;
		//	addr = CPU_EIP + (CPU_CS << 4);
		//	baseaddr = addr & ~0xff;
		//	for(i=0;i<0x100;i++){
		//		memdumpa[i] = baseaddr + i;
		//		memdump[i] = memp_read8(memdumpa[i]);
		//	}
		//	//msgbox("HAXM_ia32_panic", "HAXM CPU panic");
		//	//pcstat.hardwarereset = TRUE;
		//	return;
		//}
		break;
	case HAX_EXIT_PAUSED: // 一時停止？
		//printf("HAX_EXIT_PAUSED\n");
		// リセット可能フラグを立てる
		np2haxcore.ready_for_reset = 1;
		break;
	case HAX_EXIT_PAGEFAULT: // ページフォールト？
		//printf("HAX_EXIT_PAGEFAULT\n");
		break;
	case HAX_EXIT_DEBUG: // デバッグ命令(INT3 CCh)が呼ばれたとき
		//printf("HAX_EXIT_DEBUG\n");
		// リセット可能フラグを立てる
		np2haxcore.ready_for_reset = 1;
		// リアルモード or 仮想86
		if(!CPU_STAT_PM || CPU_STAT_VM86){
			addr = CPU_EIP + (CPU_CS << 4);
			// BIOSコール
			if(memp_read8(addr)==bioshookinfo.hookinst){
				CPU_EIP++;
				if(ia32hax_bioscall()){
				}
				np2haxstat.update_regs = 1;
				np2haxstat.update_segment_regs = 1;
				// カウンタリセット
				pmcounter = 0;
			}
		}
		break;
	default:
		break;
	}
	
	np2haxcore.running = 0;
}
void i386hax_vm_exec(void) {
	static int skipcounter = 0;
	static SINT32 remain_clk = 0;
	SINT32 remclktmp = CPU_REMCLOCK;
	int timing;
	if(pcstat.hardwarereset){
		CPU_REMCLOCK = 0;
		return;
	}
	if(!np2haxcore.hltflag || !CPU_isEI){
		CPU_REMCLOCK += remain_clk;
		np2haxcore.lastclock = NP2_TickCount_GetCount();
		while(CPU_REMCLOCK > 0){
			i386hax_vm_exec_part();
			if(dmac.working) {
				dmax86();
			}
			np2haxcore.clockcount = NP2_TickCount_GetCount();
			if(CPU_REMCLOCK > 0){
				CPU_REMCLOCK -= (np2haxcore.clockcount - np2haxcore.lastclock) * pccore.realclock / np2haxcore.clockpersec;
			}
			np2haxcore.lastclock  = np2haxcore.clockcount;
			if(CPU_RESETREQ) break;
			if(pcstat.hardwarereset) break;
			if (CPU_REMCLOCK > 0 && (timing = timing_getcount_baseclock())!=0) {
				CPU_REMCLOCK = 0;
				break;
			}
			if(np2haxcore.hltflag){
				CPU_REMCLOCK = 0;
				remain_clk = 0;
				break;
			}
		}
		if(CPU_REMCLOCK <= 0){
			remain_clk = CPU_REMCLOCK;
		}else{
			remain_clk = 0;
		}
		if (CPU_REMCLOCK < -(SINT32)(CPU_BASECLOCK*3)) {
			np2haxcore.hurryup = 1;
		}
		CPU_REMCLOCK = 0;
	}else{
		if(dmac.working) {
			dmax86();
		}
		CPU_REMCLOCK = 0;
		remain_clk = 0;
	}
}

void i386hax_disposeVM(void) {
	
	if(!np2hax.hDevice) return;
	if(!np2hax.hVCPUDevice) return;
	if(!np2hax.hVMDevice) return;
	
	// 仮想マシンを閉じる
#if defined(_WINDOWS)
	CloseHandle(np2hax.hVCPUDevice);
#else
	close(np2hax.hVCPUDevice);
#endif
	np2hax.hVCPUDevice = NULL;
#if defined(_WINDOWS)
	CloseHandle(np2hax.hVMDevice);
#else
	close(np2hax.hVMDevice);
#endif
	np2hax.hVMDevice = NULL;

	TRACEOUT(("HAXM: HAX VM disposed."));
    //msgbox("HAXM deinit", "HAX VM disposed.");
}

void i386hax_deinitialize(void) {
	
	if(!np2hax.hDevice) return;

	i386hax_disposeVM();
	
	// HAXMを閉じる
#if defined(_WINDOWS)
	CloseHandle(np2hax.hDevice);
#else
	close(np2hax.hDevice);
#endif
	np2hax.hDevice = NULL;
	
	TRACEOUT(("HAXM: HAX deinitialized."));
    //msgbox("HAXM deinit", "HAX deinitialized.");
}

// HAXM仮想マシンで使用するホストのメモリ領域を登録（登録済みメモリのみ仮想マシンに割り当て可能）
void i386hax_vm_allocmemory(void) {
	
	HAX_ALLOC_RAM_INFO info = {0};

	if(!np2hax.hVMDevice) return;
	
	info.size = 0x200000;
	info.va = (UINT64)mem;
	if(i386haxfunc_allocRAM(&info)==FAILURE){
		TRACEOUT(("HAXM: HAX VM alloc memory failed."));
		msgbox("HAXM VM", "HAX VM alloc memory failed.");
		np2hax.enable = 0;
		i386hax_disposeVM();
		return;
	}
	
	info.size = CPU_EXTMEMSIZE + 4096;
	info.va = (UINT64)CPU_EXTMEM;
	if(i386haxfunc_allocRAM(&info)==FAILURE){
		TRACEOUT(("HAXM: HAX VM alloc memory failed."));
		msgbox("HAXM VM", "HAX VM alloc memory failed.");
		np2hax.enable = 0;
		i386hax_disposeVM();
		return;
	}
	
	info.size = sizeof(vramex_base);
	info.va = (UINT64)vramex;
	if(i386haxfunc_allocRAM(&info)==FAILURE){
		TRACEOUT(("HAXM: HAX VM alloc memory failed."));
		msgbox("HAXM VM", "HAX VM alloc memory failed.");
		np2hax.enable = 0;
		i386hax_disposeVM();
		return;
	}

	TRACEOUT(("HAXM: HAX VM alloc memory."));
    //msgbox("HAXM VM", "HAX VM alloc memory.");
}
void i386hax_vm_allocmemoryex(UINT8 *vramptr, UINT32 size) {
	
	HAX_ALLOC_RAM_INFO info = {0};

	if(!np2hax.hVMDevice) return;
	
	info.size = size;
	info.va = (UINT64)vramptr;
	if(i386haxfunc_allocRAM(&info)==FAILURE){
		TRACEOUT(("HAXM: HAX VM alloc memory failed."));
		msgbox("HAXM VM", "HAX VM alloc memory failed.");
		return;
	}

	TRACEOUT(("HAXM: HAX VM alloc memory."));
    //msgbox("HAXM VM", "HAX VM alloc memory.");
}

// HAXM仮想マシンのゲスト物理アドレス(Guest Physical Address; GPA)にホストの仮想アドレス(Host Virtual Address; HVA)を割り当て
void i386hax_vm_setmemory(void) {
	
	HAX_SET_RAM_INFO info = {0};

	if(!np2hax.enable) return;
	if(!np2hax.hVMDevice) return;
	
	info.pa_start = 0;
	info.size = 0xA0000;
	info.flags = 0;
	info.va = (UINT64)mem;
	if(i386haxfunc_setRAM(&info)==FAILURE){
		TRACEOUT(("HAXM: HAX VM set memory failed."));
		msgbox("HAXM VM", "HAX VM set memory failed.");
		np2hax.enable = 0;
		return;
	}
	
	//info.pa_start = 0xFFF00000;
	//info.size = 0xF8000;
	//info.flags = 0;
	//info.va = (UINT64)mem;
	//if(i386haxfunc_setRAM(&info)==FAILURE){
	//	TRACEOUT(("HAXM: HAX VM set memory failed."));
	//	msgbox("HAXM VM", "HAX VM set memory failed.");
	//	np2hax.enable = 0;
	//	return;
	//}
	
	TRACEOUT(("HAXM: HAX VM set memory."));
    //msgbox("HAXM VM", "HAX VM set memory.");
}
void i386hax_vm_setbankmemory() {

}
void i386hax_vm_setitfmemory(UINT8 isitfbank) {
	
	HAX_SET_RAM_INFO info = {0};

	if(!np2hax.hVMDevice) 
	if(!np2hax.enable) return;
	
	info.pa_start = 0xF8000;
	info.size = 0x8000;
	info.flags = HAX_RAM_INFO_INVALID;
	if(i386haxfunc_setRAM(&info)==FAILURE){
	}
	if(isitfbank){
		info.flags = HAX_RAM_INFO_ROM;
		info.va = (UINT64)(mem + ITF_ADRS);
	}else{
		info.flags = HAX_RAM_INFO_ROM;
		info.va = (UINT64)(mem + info.pa_start);
	}
	if(i386haxfunc_setRAM(&info)==FAILURE){
		TRACEOUT(("HAXM: HAX VM set ITF bank memory failed."));
		msgbox("HAXM VM", "HAX VM set ITF bank memory failed.");
		np2hax.enable = 0;
		return;
	}
	
	info.pa_start = 0xFFFF8000;
	info.size = 0x8000;
	info.flags = HAX_RAM_INFO_INVALID;
	if(i386haxfunc_setRAM(&info)==FAILURE){
	}
	if(isitfbank){
		info.flags = HAX_RAM_INFO_ROM;
		info.va = (UINT64)(mem + ITF_ADRS);
	}else{
		info.flags = HAX_RAM_INFO_ROM;
		info.va = (UINT64)(mem + 0xF8000);
	}
	if(i386haxfunc_setRAM(&info)==FAILURE){
		TRACEOUT(("HAXM: HAX VM set ITF bank memory failed."));
		msgbox("HAXM VM", "HAX VM set ITF bank memory failed.");
		np2hax.enable = 0;
		return;
	}

	if(isitfbank){
		TRACEOUT(("HAXM: HAX VM set ITF bank memory. (ITF)"));
		//msgbox("HAXM VM", "HAX VM set ITF bank memory. (ITF)");
	}else{
		TRACEOUT(("HAXM: HAX VM set ITF bank memory. (OFF)"));
		//msgbox("HAXM VM", "HAX VM set ITF bank memory. (OFF)");
	}
}
void i386hax_vm_sethmemory(UINT8 a20en) {
	
	HAX_SET_RAM_INFO info = {0};
	
	if(!np2hax.enable) return;
	if(!np2hax.hVMDevice) return;
	
	info.pa_start = 0x100000;
	info.size = 0x10000;
	info.flags = HAX_RAM_INFO_INVALID;
	if(i386haxfunc_setRAM(&info)==FAILURE){
	}

	if(a20en){
		info.flags = 0;
		info.va = (UINT64)(mem + info.pa_start);
	}else{
		info.flags = 0;
		info.va = (UINT64)(mem);
	}
	if(i386haxfunc_setRAM(&info)==FAILURE){
		TRACEOUT(("HAXM: HAX VM set high memory failed."));
		msgbox("HAXM VM", "HAX VM set high memory failed.");
		np2hax.enable = 0;
		return;
	}
	
	if(a20en){
		TRACEOUT(("HAXM: HAX VM set high memory. (A20 ON)"));
		//msgbox("HAXM VM", "HAX VM set high memory. (A20 ON)");
	}else{
		TRACEOUT(("HAXM: HAX VM set high memory. (A20 OFF)"));
		//msgbox("HAXM VM", "HAX VM set ITF bank memory. (A20 OFF)");
	}
}
void i386hax_vm_setextmemory(void) {
	
	HAX_SET_RAM_INFO info = {0};
	
	if(!np2hax.enable) return;
	if(!np2hax.hVMDevice) return;
	
	info.pa_start = 0x110000;
	info.size = CPU_EXTLIMIT16 - 0x110000;
	info.flags = 0;
	info.va = (UINT64)(CPU_EXTMEMBASE + info.pa_start);
	if(i386haxfunc_setRAM(&info)==FAILURE){
		TRACEOUT(("HAXM: HAX VM set ext16 memory failed."));
		msgbox("HAXM VM", "HAX VM set ext16 memory failed.");
		np2hax.enable = 0;
		return;
	}

	if(CPU_EXTLIMIT > 0x01000000){
		info.pa_start = 0x01000000;
		info.size = CPU_EXTLIMIT - 0x01000000;
		info.flags = 0;
		info.va = (UINT64)(CPU_EXTMEMBASE + info.pa_start);
		if(i386haxfunc_setRAM(&info)==FAILURE){
			TRACEOUT(("HAXM: HAX VM set ext memory failed."));
			msgbox("HAXM VM", "HAX VM set ext memory failed.");
			np2hax.enable = 0;
			return;
		}
	}
	
	TRACEOUT(("HAXM: HAX VM set ext memory."));
    //msgbox("HAXM VM", "HAX VM set ext memory.");
}

void i386hax_vm_setvga256linearmemory(void) {
	
	HAX_SET_RAM_INFO info = {0};
	
	if(!np2hax.enable) return;
	if(!np2hax.hVMDevice) return;
	
	info.pa_start = 0xF00000;
	info.size =  0x80000;
	info.flags = HAX_RAM_INFO_INVALID;
	i386haxfunc_setRAM(&info);
	if(vramop.mio2[0x2]==0x1){
		info.flags = 0;
		info.va = (UINT64)vramex;
		if(i386haxfunc_setRAM(&info)==FAILURE){
			TRACEOUT(("HAXM: HAX VM set vga256 linear memory failed."));
			msgbox("HAXM VM", "HAX VM set vga256 linear memory failed.");
			return;
		}
	
		TRACEOUT(("HAXM: HAX VM set vga256 linear memory."));
	}
	
	info.pa_start = 0xfff00000;
	info.size =  0x80000;
	info.flags = HAX_RAM_INFO_INVALID;
	info.va = NULL;
	i386haxfunc_setRAM(&info);
	info.flags = 0;
	info.va = (UINT64)vramex;
	if(i386haxfunc_setRAM(&info)==FAILURE){
		TRACEOUT(("HAXM: HAX VM set vga256 0xfff00000 linear memory failed."));
		msgbox("HAXM VM", "HAX VM set vga256 0xfff00000 linear memory failed.");
		return;
	}
    //msgbox("HAXM VM", "HAX VM set ext memory.");
}

void i386hax_vm_setmemoryarea(UINT8 *vramptr, UINT32 addr, UINT32 size) {
	
	HAX_SET_RAM_INFO info = {0};
	
	if(!np2hax.enable) return;
	if(!np2hax.hVMDevice) return;
	
	info.pa_start = addr;
	info.size =  size;
	info.flags = HAX_RAM_INFO_INVALID;
	i386haxfunc_setRAM(&info);
	if(vramptr){
		info.flags = 0;
		info.va = (UINT64)vramptr;
		if(i386haxfunc_setRAM(&info)==FAILURE){
			TRACEOUT(("HAXM: HAX VM set memory area failed."));
			//msgbox("HAXM VM", "HAX VM set memory area failed.");
			return;
		}
	
		TRACEOUT(("HAXM: HAX VM set memory area."));
	}
}
void i386hax_vm_removememoryarea(UINT8 *vramptr, UINT32 addr, UINT32 size) {
	
	HAX_SET_RAM_INFO info = {0};
	
	if(!np2hax.enable) return;
	if(!np2hax.hVMDevice) return;
	
	info.pa_start = addr;
	info.size =  size;
	info.flags = HAX_RAM_INFO_INVALID;
	i386haxfunc_setRAM(&info);
}

#endif
