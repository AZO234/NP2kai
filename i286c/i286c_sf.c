#include	"compiler.h"
#include	"cpucore.h"
#include	"i286c.h"
#include	"i286c.mcr"
#include	"i286c_sf.mcr"


// ------------------------------------------------------------------------

I286_SFT _rol_r8_1(UINT8 *p) {

	UINT	src;
	UINT	dst;

	src = *p;
	BYTE_ROL1(dst, src)
	*p = (UINT8)dst;
}

I286_SFT _ror_r8_1(UINT8 *p) {

	UINT	src;
	UINT	dst;

	src = *p;
	BYTE_ROR1(dst, src)
	*p = (UINT8)dst;
}

I286_SFT _rcl_r8_1(UINT8 *p) {

	UINT	src;
	UINT	dst;

	src = *p;
	BYTE_RCL1(dst, src)
	*p = (UINT8)dst;
}

I286_SFT _rcr_r8_1(UINT8 *p) {

	UINT	src;
	UINT	dst;

	src = *p;
	BYTE_RCR1(dst, src)
	*p = (UINT8)dst;
}

I286_SFT _shl_r8_1(UINT8 *p) {

	UINT	src;
	UINT	dst;

	src = *p;
	BYTE_SHL1(dst, src)
	*p = (UINT8)dst;
}

I286_SFT _shr_r8_1(UINT8 *p) {

	UINT	src;
	UINT	dst;

	src = *p;
	BYTE_SHR1(dst, src)
	*p = (UINT8)dst;
}

I286_SFT _sar_r8_1(UINT8 *p) {

	UINT	src;
	UINT	dst;

	src = *p;
	BYTE_SAR1(dst, src)
	*p = (UINT8)dst;
}


I286_SFT _rol_e8_1(UINT32 madr) {

	UINT	src;
	UINT	dst;

	src = i286_memoryread(madr);
	BYTE_ROL1(dst, src)
	i286_memorywrite(madr, (REG8)dst);
}

I286_SFT _ror_e8_1(UINT32 madr) {

	UINT	src;
	UINT	dst;

	src = i286_memoryread(madr);
	BYTE_ROR1(dst, src)
	i286_memorywrite(madr, (REG8)dst);
}

I286_SFT _rcl_e8_1(UINT32 madr) {

	UINT	src;
	UINT	dst;

	src = i286_memoryread(madr);
	BYTE_RCL1(dst, src)
	i286_memorywrite(madr, (REG8)dst);
}

I286_SFT _rcr_e8_1(UINT32 madr) {

	UINT	src;
	UINT	dst;

	src = i286_memoryread(madr);
	BYTE_RCR1(dst, src)
	i286_memorywrite(madr, (REG8)dst);
}

I286_SFT _shl_e8_1(UINT32 madr) {

	UINT	src;
	UINT	dst;

	src = i286_memoryread(madr);
	BYTE_SHL1(dst, src)
	i286_memorywrite(madr, (REG8)dst);
}

I286_SFT _shr_e8_1(UINT32 madr) {

	UINT	src;
	UINT	dst;

	src = i286_memoryread(madr);
	BYTE_SHR1(dst, src)
	i286_memorywrite(madr, (REG8)dst);
}

I286_SFT _sar_e8_1(UINT32 madr) {

	UINT	src;
	UINT	dst;

	src = i286_memoryread(madr);
	BYTE_SAR1(dst, src)
	i286_memorywrite(madr, (REG8)dst);
}


const I286OPSFTR8 sft_r8_table[] = {
		_rol_r8_1,		_ror_r8_1,		_rcl_r8_1,		_rcr_r8_1,
		_shl_r8_1,		_shr_r8_1,		_shl_r8_1,		_sar_r8_1};

const I286OPSFTE8 sft_e8_table[] = {
		_rol_e8_1,		_ror_e8_1,		_rcl_e8_1,		_rcr_e8_1,
		_shl_e8_1,		_shr_e8_1,		_shl_e8_1,		_sar_e8_1};


// ------------------------------------------------------------------------

I286_SFT _rol_r16_1(UINT16 *p) {

	UINT32	src;
	UINT32	dst;

	src = *p;
	WORD_ROL1(dst, src)
	*p = (UINT16)dst;
}

I286_SFT _ror_r16_1(UINT16 *p) {

	UINT32	src;
	UINT32	dst;

	src = *p;
	WORD_ROR1(dst, src)
	*p = (UINT16)dst;
}

I286_SFT _rcl_r16_1(UINT16 *p) {

	UINT32	src;
	UINT32	dst;

	src = *p;
	WORD_RCL1(dst, src)
	*p = (UINT16)dst;
}

I286_SFT _rcr_r16_1(UINT16 *p) {

	UINT32	src;
	UINT32	dst;

	src = *p;
	WORD_RCR1(dst, src)
	*p = (UINT16)dst;
}

I286_SFT _shl_r16_1(UINT16 *p) {

	UINT32	src;
	UINT32	dst;

	src = *p;
	WORD_SHL1(dst, src)
	*p = (UINT16)dst;
}

I286_SFT _shr_r16_1(UINT16 *p) {

	UINT32	src;
	UINT32	dst;

	src = *p;
	WORD_SHR1(dst, src)
	*p = (UINT16)dst;
}

I286_SFT _sar_r16_1(UINT16 *p) {

	UINT32	src;
	UINT32	dst;

	src = *p;
	WORD_SAR1(dst, src)
	*p = (UINT16)dst;
}


I286_SFT _rol_e16_1(UINT32 madr) {

	UINT32	src;
	UINT32	dst;

	src = i286_memoryread_w(madr);
	WORD_ROL1(dst, src)
	i286_memorywrite_w(madr, (REG16)dst);
}

I286_SFT _ror_e16_1(UINT32 madr) {

	UINT32	src;
	UINT32	dst;

	src = i286_memoryread_w(madr);
	WORD_ROR1(dst, src)
	i286_memorywrite_w(madr, (REG16)dst);
}

I286_SFT _rcl_e16_1(UINT32 madr) {

	UINT32	src;
	UINT32	dst;

	src = i286_memoryread_w(madr);
	WORD_RCL1(dst, src)
	i286_memorywrite_w(madr, (REG16)dst);
}

I286_SFT _rcr_e16_1(UINT32 madr) {

	UINT32	src;
	UINT32	dst;

	src = i286_memoryread_w(madr);
	WORD_RCR1(dst, src)
	i286_memorywrite_w(madr, (REG16)dst);
}

I286_SFT _shl_e16_1(UINT32 madr) {

	UINT32	src;
	UINT32	dst;

	src = i286_memoryread_w(madr);
	WORD_SHL1(dst, src)
	i286_memorywrite_w(madr, (REG16)dst);
}

I286_SFT _shr_e16_1(UINT32 madr) {

	UINT32	src;
	UINT32	dst;

	src = i286_memoryread_w(madr);
	WORD_SHR1(dst, src)
	i286_memorywrite_w(madr, (REG16)dst);
}

I286_SFT _sar_e16_1(UINT32 madr) {

	UINT32	src;
	UINT32	dst;

	src = i286_memoryread_w(madr);
	WORD_SAR1(dst, src)
	i286_memorywrite_w(madr, (REG16)dst);
}


const I286OPSFTR16 sft_r16_table[] = {
		_rol_r16_1,		_ror_r16_1,		_rcl_r16_1,		_rcr_r16_1,
		_shl_r16_1,		_shr_r16_1,		_shl_r16_1,		_sar_r16_1};

const I286OPSFTE16 sft_e16_table[] = {
		_rol_e16_1,		_ror_e16_1,		_rcl_e16_1,		_rcr_e16_1,
		_shl_e16_1,		_shr_e16_1,		_shl_e16_1,		_sar_e16_1};

// ------------------------------------------------------------------------

I286_SFT _rol_r8_cl(UINT8 *p, REG8 cl) {

	UINT	src;
	UINT	dst;

	src = *p;
	BYTE_ROLCL(dst, src, cl)
	*p = (UINT8)dst;
}

I286_SFT _ror_r8_cl(UINT8 *p, REG8 cl) {

	UINT	src;
	UINT	dst;

	src = *p;
	BYTE_RORCL(dst, src, cl)
	*p = (UINT8)dst;
}

I286_SFT _rcl_r8_cl(UINT8 *p, REG8 cl) {

	UINT	src;
	UINT	dst;

	src = *p;
	BYTE_RCLCL(dst, src, cl)
	*p = (UINT8)dst;
}

I286_SFT _rcr_r8_cl(UINT8 *p, REG8 cl) {

	UINT	src;
	UINT	dst;

	src = *p;
	BYTE_RCRCL(dst, src, cl)
	*p = (UINT8)dst;
}

I286_SFT _shl_r8_cl(UINT8 *p, REG8 cl) {

	UINT	src;
	UINT	dst;

	src = *p;
	BYTE_SHLCL(dst, src, cl)
	*p = (UINT8)dst;
}

I286_SFT _shr_r8_cl(UINT8 *p, REG8 cl) {

	UINT	src;
	UINT	dst;

	src = *p;
	BYTE_SHRCL(dst, src, cl)
	*p = (UINT8)dst;
}

I286_SFT _sar_r8_cl(UINT8 *p, REG8 cl) {

	UINT	src;
	UINT	dst;

	src = *p;
	BYTE_SARCL(dst, src, cl)
	*p = (UINT8)dst;
}


I286_SFT _rol_e8_cl(UINT32 madr, REG8 cl) {

	UINT	src;
	UINT	dst;

	src = i286_memoryread(madr);
	BYTE_ROLCL(dst, src, cl)
	i286_memorywrite(madr, (REG8)dst);
}

I286_SFT _ror_e8_cl(UINT32 madr, REG8 cl) {

	UINT	src;
	UINT	dst;

	src = i286_memoryread(madr);
	BYTE_RORCL(dst, src, cl)
	i286_memorywrite(madr, (REG8)dst);
}

I286_SFT _rcl_e8_cl(UINT32 madr, REG8 cl) {

	UINT	src;
	UINT	dst;

	src = i286_memoryread(madr);
	BYTE_RCLCL(dst, src, cl)
	i286_memorywrite(madr, (REG8)dst);
}

I286_SFT _rcr_e8_cl(UINT32 madr, REG8 cl) {

	UINT	src;
	UINT	dst;

	src = i286_memoryread(madr);
	BYTE_RCRCL(dst, src, cl)
	i286_memorywrite(madr, (REG8)dst);
}

I286_SFT _shl_e8_cl(UINT32 madr, REG8 cl) {

	UINT	src;
	UINT	dst;

	src = i286_memoryread(madr);
	BYTE_SHLCL(dst, src, cl)
	i286_memorywrite(madr, (REG8)dst);
}

I286_SFT _shr_e8_cl(UINT32 madr, REG8 cl) {

	UINT	src;
	UINT	dst;

	src = i286_memoryread(madr);
	BYTE_SHRCL(dst, src, cl)
	i286_memorywrite(madr, (REG8)dst);
}

I286_SFT _sar_e8_cl(UINT32 madr, REG8 cl) {

	UINT	src;
	UINT	dst;

	src = i286_memoryread(madr);
	BYTE_SARCL(dst, src, cl)
	i286_memorywrite(madr, (REG8)dst);
}


const I286OPSFTR8CL sft_r8cl_table[] = {
		_rol_r8_cl,		_ror_r8_cl,		_rcl_r8_cl,		_rcr_r8_cl,
		_shl_r8_cl,		_shr_r8_cl,		_shl_r8_cl,		_sar_r8_cl};

const I286OPSFTE8CL sft_e8cl_table[] = {
		_rol_e8_cl,		_ror_e8_cl,		_rcl_e8_cl,		_rcr_e8_cl,
		_shl_e8_cl,		_shr_e8_cl,		_shl_e8_cl,		_sar_e8_cl};


// ------------------------------------------------------------------------

I286_SFT _rol_r16_cl(UINT16 *p, REG8 cl) {

	UINT32	src;
	UINT32	dst;

	src = *p;
	WORD_ROLCL(dst, src, cl)
	*p = (UINT16)dst;
}

I286_SFT _ror_r16_cl(UINT16 *p, REG8 cl) {

	UINT32	src;
	UINT32	dst;

	src = *p;
	WORD_RORCL(dst, src, cl)
	*p = (UINT16)dst;
}

I286_SFT _rcl_r16_cl(UINT16 *p, REG8 cl) {

	UINT32	src;
	UINT32	dst;

	src = *p;
	WORD_RCLCL(dst, src, cl)
	*p = (UINT16)dst;
}

I286_SFT _rcr_r16_cl(UINT16 *p, REG8 cl) {

	UINT32	src;
	UINT32	dst;

	src = *p;
	WORD_RCRCL(dst, src, cl)
	*p = (UINT16)dst;
}

I286_SFT _shl_r16_cl(UINT16 *p, REG8 cl) {

	UINT32	src;
	UINT32	dst;

	src = *p;
	WORD_SHLCL(dst, src, cl)
	*p = (UINT16)dst;
}

I286_SFT _shr_r16_cl(UINT16 *p, REG8 cl) {

	UINT32	src;
	UINT32	dst;

	src = *p;
	WORD_SHRCL(dst, src, cl)
	*p = (UINT16)dst;
}

I286_SFT _sar_r16_cl(UINT16 *p, REG8 cl) {

	UINT32	src;
	UINT32	dst;

	src = *p;
	WORD_SARCL(dst, src, cl)
	*p = (UINT16)dst;
}


I286_SFT _rol_e16_cl(UINT32 madr, REG8 cl) {

	UINT32	src;
	UINT32	dst;

	src = i286_memoryread_w(madr);
	WORD_ROLCL(dst, src, cl)
	i286_memorywrite_w(madr, (REG16)dst);
}

I286_SFT _ror_e16_cl(UINT32 madr, REG8 cl) {

	UINT32	src;
	UINT32	dst;

	src = i286_memoryread_w(madr);
	WORD_RORCL(dst, src, cl)
	i286_memorywrite_w(madr, (REG16)dst);
}

I286_SFT _rcl_e16_cl(UINT32 madr, REG8 cl) {

	UINT32	src;
	UINT32	dst;

	src = i286_memoryread_w(madr);
	WORD_RCLCL(dst, src, cl)
	i286_memorywrite_w(madr, (REG16)dst);
}

I286_SFT _rcr_e16_cl(UINT32 madr, REG8 cl) {

	UINT32	src;
	UINT32	dst;

	src = i286_memoryread_w(madr);
	WORD_RCRCL(dst, src, cl)
	i286_memorywrite_w(madr, (REG16)dst);
}

I286_SFT _shl_e16_cl(UINT32 madr, REG8 cl) {

	UINT32	src;
	UINT32	dst;

	src = i286_memoryread_w(madr);
	WORD_SHLCL(dst, src, cl)
	i286_memorywrite_w(madr, (REG16)dst);
}

I286_SFT _shr_e16_cl(UINT32 madr, REG8 cl) {

	UINT32	src;
	UINT32	dst;

	src = i286_memoryread_w(madr);
	WORD_SHRCL(dst, src, cl)
	i286_memorywrite_w(madr, (REG16)dst);
}

I286_SFT _sar_e16_cl(UINT32 madr, REG8 cl) {

	UINT32	src;
	UINT32	dst;

	src = i286_memoryread_w(madr);
	WORD_SARCL(dst, src, cl)
	i286_memorywrite_w(madr, (REG16)dst);
}


const I286OPSFTR16CL sft_r16cl_table[] = {
		_rol_r16_cl,	_ror_r16_cl,	_rcl_r16_cl,	_rcr_r16_cl,
		_shl_r16_cl,	_shr_r16_cl,	_shl_r16_cl,	_sar_r16_cl};

const I286OPSFTE16CL sft_e16cl_table[] = {
		_rol_e16_cl,	_ror_e16_cl,	_rcl_e16_cl,	_rcr_e16_cl,
		_shl_e16_cl,	_shr_e16_cl,	_shl_e16_cl,	_sar_e16_cl};

