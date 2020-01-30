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
#include	"haxfunc.h"
#include	"haxcore.h"

#if defined(SUPPORT_IA32_HAXM)

// HAXM IOCTL
UINT8 i386haxfunc_getversion(HAX_MODULE_VERSION *version) {

	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hDevice, HAX_IOCTL_VERSION,
                          NULL, 0,
						  version, sizeof(HAX_MODULE_VERSION), &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hDevice, HAX_IOCTL_VERSION, version);
	if (ret == -1) {
#endif
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_getcapability(HAX_CAPINFO *cap) {
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hDevice, HAX_IOCTL_CAPABILITY,
                          NULL, 0,
						  cap, sizeof(HAX_CAPINFO), &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hDevice, HAX_IOCTL_CAPABILITY, cap);
	if (ret == -1) {
#endif
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_setmemlimit(HAX_SET_MEMLIMIT *memlimit) {
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hDevice, HAX_IOCTL_SET_MEMLIMIT,
                          memlimit, sizeof(HAX_SET_MEMLIMIT),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hDevice, HAX_IOCTL_SET_MEMLIMIT, memlimit);
	if (ret == -1) {
#endif
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_createVM(UINT32 *vm_id) {
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hDevice, HAX_IOCTL_CREATE_VM,
                          NULL, 0,
						  vm_id, sizeof(UINT32), &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hDevice, HAX_IOCTL_CREATE_VM, vm_id);
	if (ret == -1) {
#endif
        return FAILURE;
	}

	return SUCCESS;
}

// HAXM VM IOCTL
UINT8 i386haxfunc_notifyQEMUversion(HAX_QEMU_VERSION *hax_qemu_ver) {
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVMDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hVMDevice, HAX_VM_IOCTL_NOTIFY_QEMU_VERSION,
                          hax_qemu_ver, sizeof(HAX_QEMU_VERSION),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hVMDevice, HAX_VM_IOCTL_NOTIFY_QEMU_VERSION, hax_qemu_ver);
	if (ret == -1) {
#endif
        return FAILURE;
	}

	return SUCCESS;
}

UINT8 i386haxfunc_VCPUcreate(UINT32 vcpu_id) {
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVMDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hVMDevice, HAX_VM_IOCTL_VCPU_CREATE,
                          &vcpu_id, sizeof(UINT32),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hVMDevice, HAX_VM_IOCTL_VCPU_CREATE, &vcpu_id);
	if (ret == -1) {
#endif
        return FAILURE;
	}

	return SUCCESS;
}

UINT8 i386haxfunc_allocRAM(HAX_ALLOC_RAM_INFO *allocraminfo) {
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVMDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hVMDevice, HAX_VM_IOCTL_ALLOC_RAM,
                          allocraminfo, sizeof(HAX_ALLOC_RAM_INFO),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hVMDevice, HAX_VM_IOCTL_ALLOC_RAM, allocraminfo);
	if (ret == -1) {
#endif
#if defined(_WINDOWS)
		ret = GetLastError();
#endif
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_addRAMblock(HAX_RAMBLOCK_INFO *ramblkinfo) {
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVMDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hVMDevice, HAX_VM_IOCTL_ADD_RAMBLOCK,
                          ramblkinfo, sizeof(HAX_RAMBLOCK_INFO),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hVMDevice, HAX_VM_IOCTL_ADD_RAMBLOCK, ramblkinfo);
	if (ret == -1) {
#endif
#if defined(_WINDOWS)
		ret = GetLastError();
#endif
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_setRAM(HAX_SET_RAM_INFO *setraminfo) {
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVMDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hVMDevice, HAX_VM_IOCTL_SET_RAM,
                          setraminfo, sizeof(HAX_SET_RAM_INFO),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hVMDevice, HAX_VM_IOCTL_SET_RAM, setraminfo);
	if (ret == -1) {
#endif
        return FAILURE;
	}

	return SUCCESS;
}

// HAXM VCPU IOCTL
UINT8 i386haxfunc_vcpu_run(){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_RUN,
                          NULL, 0,
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_RUN, NULL);
	if (ret == -1) {
#endif
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_setup_tunnel(HAX_TUNNEL_INFO *info){
	int ret = 0;
	DWORD dwSize;
	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_SETUP_TUNNEL,
                          NULL, 0,
						  info, sizeof(HAX_TUNNEL_INFO), &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_SETUP_TUNNEL, info);
	if (ret == -1) {
#endif
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_setMSRs(HAX_MSR_DATA *inbuf, HAX_MSR_DATA *outbuf){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_SET_MSRS,
                          inbuf, sizeof(HAX_MSR_DATA),
						  outbuf, sizeof(HAX_MSR_DATA), &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_SET_MSRS, inbuf);
	if (ret == -1) {
#endif
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_getMSRs(HAX_MSR_DATA *outbuf){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_GET_MSRS,
                          NULL, 0,
						  outbuf, sizeof(HAX_MSR_DATA), &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_GET_MSRS, outbuf);
	if (ret == -1) {
#endif
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_setFPU(HAX_FX_LAYOUT *inbuf){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_SET_FPU,
                          inbuf, sizeof(HAX_FX_LAYOUT),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_SET_FPU, inbuf);
	if (ret == -1) {
#endif
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_getFPU(HAX_FX_LAYOUT *outbuf){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_GET_FPU,
                          NULL, 0,
						  outbuf, sizeof(HAX_FX_LAYOUT), &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_GET_FPU, outbuf);
	if (ret == -1) {
#endif
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_setREGs(HAX_VCPU_STATE *inbuf){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_SET_REGS,
                          inbuf, sizeof(HAX_VCPU_STATE),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hVCPUDevice, HAX_VCPU_SET_REGS, inbuf);
	if (ret == -1) {
#endif
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_getREGs(HAX_VCPU_STATE *outbuf){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_GET_REGS,
                          NULL, 0,
						  outbuf, sizeof(HAX_VCPU_STATE), &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hVCPUDevice, HAX_VCPU_GET_REGS, outbuf);
	if (ret == -1) {
#endif
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_interrupt(UINT32 vector){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_INTERRUPT,
                          &vector, sizeof(UINT32),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_INTERRUPT, &vector);
	if (ret == -1) {
#endif
        return FAILURE;
	}

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_kickoff(){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_VCPU_IOCTL_KICKOFF,
                          NULL, 0,
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
        return FAILURE;
	}
#endif

	return SUCCESS;
}
UINT8 i386haxfunc_vcpu_debug(HAX_DEBUG *inbuf){
	int ret = 0;
	DWORD dwSize;

	if(!np2hax.hVCPUDevice){
		return FAILURE;
	}

#if defined(_WINDOWS)
	ret = DeviceIoControl(np2hax.hVCPUDevice, HAX_IOCTL_VCPU_DEBUG,
                          inbuf, sizeof(HAX_DEBUG),
						  NULL, 0, &dwSize, 
						  (LPOVERLAPPED)NULL);
	if (!ret) {
#else
  ret = ioctl(np2hax.hVCPUDevice, HAX_IOCTL_VCPU_DEBUG, inbuf);
	if (ret == -1) {
#endif
        return FAILURE;
	}

	return SUCCESS;
}
#endif
