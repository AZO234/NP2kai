
// PC-9821 PCI-CBusブリッジ

#if defined(SUPPORT_PC9821)
#if defined(SUPPORT_PCI)

#ifdef __cplusplus
extern "C" {
#endif
	
extern int pcidev_98graphbridge_deviceid;
	
void pcidev_98graphbridge_reset(const NP2CFG *pConfig);
void pcidev_98graphbridge_bind(void);

#ifdef __cplusplus
}
#endif

#endif
#endif

