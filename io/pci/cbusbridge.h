
// PC-9821 PCI-CBusƒuƒŠƒbƒW

#if defined(SUPPORT_PC9821)
#if defined(SUPPORT_PCI)

#ifdef __cplusplus
extern "C" {
#endif

extern int pcidev_cbusbridge_deviceid;
	
void pcidev_cbusbridge_reset(const NP2CFG *pConfig);
void pcidev_cbusbridge_bind(void);

#ifdef __cplusplus
}
#endif

#endif
#endif

