#include	"compiler.h"
#include	"textcnv.h"


void textcnv_swapendian16(void *buf, UINT leng) {

	UINT8	*p;
	UINT8	tmp;

	p = (UINT8 *)buf;
	while(leng) {
		tmp = p[0];
		p[0] = p[1];
		p[1] = tmp;
		p += 2;
		leng--;
	}
}

