#ifdef __cplusplus
extern "C" {
#endif

void ct1741io_reset();
void ct1741io_bind(void);
void ct1741io_unbind(void);
REG8 DMACCALL ct1741dmafunc(REG8 func);
void ct1741_set_dma_irq(UINT8 irq);
void ct1741_set_dma_ch(UINT8 dmach);
UINT8 ct1741_get_dma_irq();
UINT8 ct1741_get_dma_ch();

#ifdef __cplusplus
}
#endif
