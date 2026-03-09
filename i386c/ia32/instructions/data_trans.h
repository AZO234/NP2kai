/*
 * Copyright (c) 2003 NONAKA Kimihiro
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

#ifndef	IA32_CPU_INSTRUCTION_DATA_TRANS_H__
#define	IA32_CPU_INSTRUCTION_DATA_TRANS_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * MOV
 */
void MOV_EbGb(void);
void MOV_EwGw(void);
void MOV_EdGd(void);
void MOV_GbEb(void);
void MOV_GwEw(void);
void MOV_GdEd(void);
void MOV_EwSw(void);
void MOV_EdSw(void);
void MOV_SwEw(void);
void MOV_ALOb(void);
void MOV_AXOw(void);
void MOV_EAXOd(void);
void MOV_ObAL(void);
void MOV_OwAX(void);
void MOV_OdEAX(void);
void MOV_EbIb(void);
void MOV_EwIw(void);
void MOV_EdId(void);
void MOV_ALIb(void);
void MOV_CLIb(void);
void MOV_DLIb(void);
void MOV_BLIb(void);
void MOV_AHIb(void);
void MOV_CHIb(void);
void MOV_DHIb(void);
void MOV_BHIb(void);
void MOV_AXIw(void);
void MOV_CXIw(void);
void MOV_DXIw(void);
void MOV_BXIw(void);
void MOV_SPIw(void);
void MOV_BPIw(void);
void MOV_SIIw(void);
void MOV_DIIw(void);
void MOV_EAXId(void);
void MOV_ECXId(void);
void MOV_EDXId(void);
void MOV_EBXId(void);
void MOV_ESPId(void);
void MOV_EBPId(void);
void MOV_ESIId(void);
void MOV_EDIId(void);

/*
 * CMOVcc
 */
void CMOVO_GwEw(void);
void CMOVO_GdEd(void);
void CMOVNO_GwEw(void);
void CMOVNO_GdEd(void);
void CMOVC_GwEw(void);
void CMOVC_GdEd(void);
void CMOVNC_GwEw(void);
void CMOVNC_GdEd(void);
void CMOVZ_GwEw(void);
void CMOVZ_GdEd(void);
void CMOVNZ_GwEw(void);
void CMOVNZ_GdEd(void);
void CMOVA_GwEw(void);
void CMOVA_GdEd(void);
void CMOVNA_GwEw(void);
void CMOVNA_GdEd(void);
void CMOVS_GwEw(void);
void CMOVS_GdEd(void);
void CMOVNS_GwEw(void);
void CMOVNS_GdEd(void);
void CMOVP_GwEw(void);
void CMOVP_GdEd(void);
void CMOVNP_GwEw(void);
void CMOVNP_GdEd(void);
void CMOVL_GwEw(void);
void CMOVL_GdEd(void);
void CMOVNL_GwEw(void);
void CMOVNL_GdEd(void);
void CMOVLE_GwEw(void);
void CMOVLE_GdEd(void);
void CMOVNLE_GwEw(void);
void CMOVNLE_GdEd(void);

/*
 * XCHG
 */
void XCHG_EbGb(void);
void XCHG_EwGw(void);
void XCHG_EdGd(void);
void XCHG_CXAX(void);
void XCHG_DXAX(void);
void XCHG_BXAX(void);
void XCHG_SPAX(void);
void XCHG_BPAX(void);
void XCHG_SIAX(void);
void XCHG_DIAX(void);
void XCHG_ECXEAX(void);
void XCHG_EDXEAX(void);
void XCHG_EBXEAX(void);
void XCHG_ESPEAX(void);
void XCHG_EBPEAX(void);
void XCHG_ESIEAX(void);
void XCHG_EDIEAX(void);

/*
 * BSWAP
 */
void BSWAP_EAX(void);
void BSWAP_ECX(void);
void BSWAP_EDX(void);
void BSWAP_EBX(void);
void BSWAP_ESP(void);
void BSWAP_EBP(void);
void BSWAP_ESI(void);
void BSWAP_EDI(void);

/*
 * XADD
 */
void XADD_EbGb(void);
void XADD_EwGw(void);
void XADD_EdGd(void);

/*
 * CMPXCHG
 */
void CMPXCHG_EbGb(void);
void CMPXCHG_EwGw(void);
void CMPXCHG_EdGd(void);
void CPUCALL CMPXCHG8B(UINT32);

/*
 * PUSH
 */
void PUSH_AX(void);
void PUSH_CX(void);
void PUSH_DX(void);
void PUSH_BX(void);
void PUSH_SP(void);
void PUSH_BP(void);
void PUSH_SI(void);
void PUSH_DI(void);
void PUSH_EAX(void);
void PUSH_ECX(void);
void PUSH_EDX(void);
void PUSH_EBX(void);
void PUSH_ESP(void);
void PUSH_EBP(void);
void PUSH_ESI(void);
void PUSH_EDI(void);
void PUSH_Ib(void);
void PUSH_Iw(void);
void PUSH_Id(void);
void PUSH16_ES(void);
void PUSH16_CS(void);
void PUSH16_SS(void);
void PUSH16_DS(void);
void PUSH16_FS(void);
void PUSH16_GS(void);
void PUSH32_ES(void);
void PUSH32_CS(void);
void PUSH32_SS(void);
void PUSH32_DS(void);
void PUSH32_FS(void);
void PUSH32_GS(void);

void CPUCALL PUSH_Ew(UINT32);
void CPUCALL PUSH_Ed(UINT32);

/*
 * POP
 */
void POP_AX(void);
void POP_CX(void);
void POP_DX(void);
void POP_BX(void);
void POP_SP(void);
void POP_BP(void);
void POP_SI(void);
void POP_DI(void);
void POP_EAX(void);
void POP_ECX(void);
void POP_EDX(void);
void POP_EBX(void);
void POP_ESP(void);
void POP_EBP(void);
void POP_ESI(void);
void POP_EDI(void);
void POP_Ew(void);
void POP_Ed(void);
void POP16_ES(void);
void POP32_ES(void);
void POP16_SS(void);
void POP32_SS(void);
void POP16_DS(void);
void POP32_DS(void);
void POP16_FS(void);
void POP32_FS(void);
void POP16_GS(void);
void POP32_GS(void);

void CPUCALL POP_Ew_G5(UINT32);
void CPUCALL POP_Ed_G5(UINT32);

/*
 * PUSHA/POPA
 */
void PUSHA(void);
void PUSHAD(void);
void POPA(void);
void POPAD(void);

/*
 * in/out
 */
void IN_ALDX(void);
void IN_AXDX(void);
void IN_EAXDX(void);
void IN_ALIb(void);
void IN_AXIb(void);
void IN_EAXIb(void);
void OUT_DXAL(void);
void OUT_DXAX(void);
void OUT_DXEAX(void);
void OUT_IbAL(void);
void OUT_IbAX(void);
void OUT_IbEAX(void);

/*
 * convert
 */
void CWD(void);
void CDQ(void);
void CBW(void);
void CWDE(void);

/*
 * MOVSx/MOVZx
 */
void MOVSX_GwEb(void);
void MOVSX_GwEw(void);
void MOVSX_GdEb(void);
void MOVSX_GdEw(void);
void MOVZX_GwEb(void);
void MOVZX_GwEw(void);
void MOVZX_GdEb(void);
void MOVZX_GdEw(void);

#ifdef __cplusplus
}
#endif

#endif	/* IA32_CPU_INSTRUCTION_DATA_TRANS_H__ */
