
struct _gdcpset;
typedef struct _gdcpset		_GDCPSET;
typedef struct _gdcpset		*GDCPSET;

typedef void (MEMCALL * GDCPFN)(GDCPSET pen, UINT addr, UINT bit);

struct _gdcpset {
	GDCPFN	func[2];
	union {
		UINT8	*ptr;			// raw access / grcg
		UINT32	addr;			// egc
	}		base;
	UINT16	pattern;
	PAIR16	update;
	UINT16	x;
	UINT16	y;
	UINT	dots;
};


#ifdef __cplusplus
extern "C" {
#endif

void MEMCALL gdcpset_prepare(GDCPSET pset, UINT32 csrw, REG16 pat, REG8 op);
void MEMCALL gdcpset(GDCPSET pset, REG16 x, REG16 y);

#ifdef __cplusplus
}
#endif

