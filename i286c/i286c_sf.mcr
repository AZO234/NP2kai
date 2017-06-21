// wordはかなりノーチェック


#define	BYTE_ROL1(d, s)	{											\
		UINT tmp = ((s) >> 7);										\
		(d) = ((s) << 1) + tmp;										\
		I286_FLAGL &= ~C_FLAG;										\
		I286_FLAGL |= tmp;											\
		I286_OV = ((s) ^ (d)) & 0x80;								\
	}

#define BYTE_ROR1(d, s) {											\
		UINT tmp = ((s) & 1);										\
		(d) = ((tmp << 8) + (s)) >> 1;								\
		I286_FLAGL &= ~C_FLAG;										\
		I286_FLAGL |= tmp;											\
		I286_OV = ((s) ^ (d)) & 0x80;								\
	}

#define BYTE_RCL1(d, s)												\
		(d) = ((s) << 1) | (I286_FLAGL & C_FLAG);					\
		I286_FLAGL &= ~C_FLAG;										\
		I286_FLAGL |= ((s) >> 7);									\
		I286_OV = ((s) ^ (d)) & 0x80;

#define	BYTE_RCR1(d, s)												\
		(d) = (((I286_FLAGL & C_FLAG) << 8) | (s)) >> 1;			\
		I286_FLAGL &= ~C_FLAG;										\
		I286_FLAGL |= ((s) & 1);									\
		I286_OV = ((s) ^ (d)) & 0x80;

#define	BYTE_SHL1(d, s)												\
		(d) = (s) << 1;												\
		I286_OV = ((s) ^ (d)) & 0x80;								\
		I286_FLAGL = BYTESZPCF(d) | A_FLAG;

#define	BYTE_SHR1(d, s)												\
		(d) = (s) >> 1;												\
		I286_OV = (s) & 0x80;										\
		I286_FLAGL = (UINT8)(BYTESZPF(d) | A_FLAG | ((s) & 1));

#if 1
#define	BYTE_SAR1(d, s)												\
		(d) = ((s) & 0x80) + ((s) >> 1);							\
		I286_OV = 0;												\
		I286_FLAGL = (UINT8)(BYTESZPF(d) | A_FLAG | ((s) & 1));
#else	// eVC3/4 compiler bug
#define	BYTE_SAR1(d, s)												\
		(d) = (UINT8)(((SINT8)(s)) >> 1);							\
		I286_OV = 0;												\
		I286_FLAGL = (UINT8)(BYTESZPF(d) | A_FLAG | ((s) & 1));
#endif


#define	WORD_ROL1(d, s)	{											\
		UINT32 tmp = ((s) >> 15);									\
		(d) = ((s) << 1) + tmp;										\
		I286_FLAGL &= ~C_FLAG;										\
		I286_FLAGL |= tmp;											\
		I286_OV = ((s) ^ (d)) & 0x8000;								\
	}

#define WORD_ROR1(d, s) {											\
		UINT32 tmp = ((s) & 1);										\
		(d) = ((tmp << 16) + (s)) >> 1;								\
		I286_FLAGL &= ~C_FLAG;										\
		I286_FLAGL |= tmp;											\
		I286_OV = ((s) ^ (d)) & 0x8000;								\
	}

#define WORD_RCL1(d, s)												\
		(d) = ((s) << 1) | (I286_FLAGL & 1);						\
		I286_FLAGL &= ~C_FLAG;										\
		I286_FLAGL |= ((s) >> 15);									\
		I286_OV = ((s) ^ (d)) & 0x8000;

#define	WORD_RCR1(d, s)												\
		(d) = (((I286_FLAGL & 1) << 16) + (s)) >> 1;				\
		I286_FLAGL &= ~C_FLAG;										\
		I286_FLAGL |= ((s) & 1);									\
		I286_OV = ((s) ^ (d)) & 0x8000;

#define	WORD_SHL1(d, s)												\
		(d) = (s) << 1;												\
		I286_OV = ((s) ^ (d)) & 0x8000;								\
		I286_FLAGL = WORDSZPCF(d) + A_FLAG;

#define	WORD_SHR1(d, s)												\
		(d) = (s) >> 1;												\
		I286_OV = (s) & 0x8000;										\
		I286_FLAGL = (UINT8)(WORDSZPF(d) | A_FLAG | ((s) & 1));

#if 1
#define	WORD_SAR1(d, s)												\
		(d) = ((s) & 0x8000) + ((s) >> 1);							\
		I286_OV = 0;												\
		I286_FLAGL = (UINT8)(WORDSZPF(d) | A_FLAG | ((s) & 1));
#else	// eVC3/4 compiler bug
#define	WORD_SAR1(d, s)												\
		(d) = (UINT16)(((SINT16)(s)) >> 1);							\
		I286_OV = 0;												\
		I286_FLAGL = (UINT8)(WORDSZPF(d) | A_FLAG | ((s) & 1));
#endif



#define	BYTE_ROLCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			(c) = ((c) - 1) & 7;									\
			if (c) {												\
				(s) = ((s) << (c)) | ((s) >> (8 - (c)));			\
				(s) &= 0xff;										\
			}														\
			BYTE_ROL1(d, s)											\
		}															\
		else {														\
			(d) = (s);												\
		}

#define	BYTE_RORCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			(c) = ((c) - 1) & 7;									\
			if (c) {												\
				(s) = ((s) >> (c)) | ((s) << (8 - (c)));			\
				(s) &= 0xff;										\
			}														\
			BYTE_ROR1(d, s)											\
		}															\
		else {														\
			(d) = (s);												\
		}

#define	BYTE_RCLCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			UINT tmp;												\
			tmp = I286_FLAGL & C_FLAG;								\
			I286_FLAGL &= ~C_FLAG;									\
			while((c)--) {											\
				(s) = (((s) << 1) | tmp) & 0x1ff;					\
				tmp = (s) >> 8;										\
			}														\
			I286_OV = ((s) ^ (s >> 1)) & 0x80;						\
			I286_FLAGL |= tmp;										\
		}															\
		(d) = (s);

#define	BYTE_RCRCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			UINT tmp;												\
			tmp = I286_FLAGL & C_FLAG;								\
			I286_FLAGL &= ~C_FLAG;									\
			while((c)--) {											\
				(s) |= tmp << 8;									\
				tmp = (s) & 1;										\
				(s) >>= 1;											\
			}														\
			I286_OV = ((s) ^ (s >> 1)) & 0x40;						\
			I286_FLAGL |= tmp;										\
		}															\
		(d) = (s);

#define	BYTE_SHLCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			if ((c) > 10) {											\
				(c) = 10;											\
			}														\
			(s) <<= (c);											\
			(s) &= 0x1ff;											\
			I286_FLAGL = BYTESZPCF(s) + A_FLAG;						\
			I286_OV = ((s) ^ ((s) >> 1)) & 0x80;					\
		}															\
		(d) = (s);

#define	BYTE_SHRCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			if ((c) >= 10) {										\
				(c) = 10;											\
			}														\
			(s) >>= ((c) - 1);										\
			I286_FLAGL = (UINT8)((s) & 1);							\
			(s) >>= 1;												\
			I286_OV = ((s) ^ ((s) >> 1)) & 0x40;					\
			I286_FLAGL |= BYTESZPF(s) + A_FLAG;						\
		}															\
		(d) = (s);

#if !defined(_WIN32_WCE) || (_WIN32_WCE < 300)
#define	BYTE_SARCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			(s) = ((SINT8)(s)) >> ((c) - 1);						\
			I286_FLAGL = (UINT8)((s) & 1);							\
			(s) = (UINT8)(((SINT8)s) >> 1);							\
			I286_OV = 0;											\
			I286_FLAGL |= BYTESZPF(s) | A_FLAG;						\
		}															\
		(d) = (s);
#else
#define	BYTE_SARCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			SINT32 t;												\
			t = (s) << 24;											\
			t = t >> ((c) - 1);										\
			I286_FLAGL = (UINT8)((t >> 24) & 1);					\
			(s) = (t >> 25) & 0xff;									\
			I286_OV = 0;											\
			I286_FLAGL |= BYTESZPF(s) | A_FLAG;						\
		}															\
		(d) = (s);
#endif

#define	WORD_ROLCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			UINT tmp;												\
			(c)--;													\
			if (c) {												\
				(c) &= 0x0f;										\
				(s) = ((s) << (c)) | ((s) >> (16 - (c)));			\
				(s) &= 0xffff;										\
			}														\
			else {													\
				I286_OV = ((s) + 0x4000) & 0x8000;					\
			}														\
			tmp = ((s) >> 15);										\
			(s) = ((s) << 1) + tmp;									\
			I286_FLAGL &= ~C_FLAG;									\
			I286_FLAGL |= tmp;										\
		}															\
		(d) = (s);

#define	WORD_RORCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			UINT32 tmp;												\
			(c)--;													\
			if (c) {												\
				(c) &= 0x0f;										\
				(s) = ((s) >> (c)) | ((s) << (16 - (c)));			\
				(s) &= 0xffff;										\
			}														\
			else {													\
				I286_OV = ((s) >> 15) ^ ((s) & 1);					\
			}														\
			tmp = (s) & 1;											\
			(s) = ((tmp << 16) + (s)) >> 1;							\
			I286_FLAGL &= ~C_FLAG;									\
			I286_FLAGL |= tmp;										\
		}															\
		(d) = (s);

#define	WORD_RCLCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			UINT tmp;												\
			tmp = I286_FLAGL & C_FLAG;								\
			I286_FLAGL &= ~C_FLAG;									\
			if ((c) == 1) {											\
				I286_OV = ((s) + 0x4000) & 0x8000;					\
			}														\
			while((c)--) {											\
				(s) = (((s) << 1) + tmp) & 0x1ffff;					\
				tmp = (s) >> 16;									\
			}														\
			I286_FLAGL |= tmp;										\
		}															\
		(d) = (s);

#define	WORD_RCRCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			UINT32 tmp;												\
			tmp = I286_FLAGL & C_FLAG;								\
			I286_FLAGL &= ~C_FLAG;									\
			if ((c) == 1) {											\
				I286_OV = ((s) >> 15) ^ tmp;						\
			}														\
			while((c)--) {											\
				(s) |= tmp << 16;									\
				tmp = (s) & 1;										\
				(s) >>= 1;											\
			}														\
			I286_FLAGL |= tmp;										\
		}															\
		(d) = (s);

#define	WORD_SHLCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			I286_OV = 0;											\
			if ((c) == 1) {											\
				I286_OV = ((s) + 0x4000) & 0x8000;					\
			}														\
			(s) <<= (c);											\
			(s) &= 0x1ffff;											\
			I286_FLAGL = WORDSZPCF(s);								\
		}															\
		(d) = (s);

#define	WORD_SHRCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			(c)--;													\
			if (c) {												\
				(s) >>= (c);										\
				I286_OV = 0;										\
			}														\
			else {													\
				I286_OV = (s) & 0x8000;								\
			}														\
			I286_FLAGL = (UINT8)((s) & 1);							\
			(s) >>= 1;												\
			I286_FLAGL |= WORDSZPF(s);								\
		}															\
		(d) = (s);

#if !defined(_WIN32_WCE) || (_WIN32_WCE < 300)
#define	WORD_SARCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			(s) = ((SINT16)(s)) >> ((c) - 1);						\
			I286_FLAGL = (UINT8)((s) & 1);							\
			(s) = (UINT16)(((SINT16)s) >> 1);						\
			I286_OV = 0;											\
			I286_FLAGL |= WORDSZPF(s);								\
		}															\
		(d) = (s);
#else	// eVC～
#define	WORD_SARCL(d, s, c)											\
		(c) &= 0x1f;												\
		if (c) {													\
			SINT32 tmp;												\
			tmp = (s) << 16;										\
			tmp = tmp >> (16 + (c) - 1);							\
			I286_FLAGL = (UINT8)(tmp & 1);							\
			(s) = (UINT16)(tmp >> 1);								\
			I286_OV = 0;											\
			I286_FLAGL |= WORDSZPF(s);								\
		}															\
		(d) = (s);
#endif

