
// PC-9821 PCIƒuƒŠƒbƒW

#if defined(SUPPORT_PC9821)

typedef struct {
	UINT32	base;

	UINT8	membankd0;
} _PCIDEV, *PCIDEV;


#ifdef __cplusplus
extern "C" {
#endif

void IOOUTCALL pcidev_w32(UINT port, UINT32 value);
UINT32 IOOUTCALL pcidev_r32(UINT port);

void pcidev_reset(const NP2CFG *pConfig);
void pcidev_bind(void);

#ifdef __cplusplus
}
#endif

#endif

