#include	"compiler.h"
#include	"textcnv.h"


void textcnv_swapendian32(void *buf, UINT leng) {

	UINT8	*p;
	UINT8	tmp0;
	UINT8	tmp1;

	p = (UINT8 *)buf;
	while(leng) {
		tmp0 = p[0];
		tmp1 = p[1];
		p[0] = p[3];
		p[1] = p[2];
		p[2] = tmp1;
		p[3] = tmp0;
		p += 4;
		leng--;
	}
}

