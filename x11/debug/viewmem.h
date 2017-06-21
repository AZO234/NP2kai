#if defined(SUPPORT_VIEWER)

G_BEGIN_DECLS

void viewmem_read(VIEWMEM_T *cfg, UINT32 adrs, UINT8 *buf, UINT32 size);
void viewmem_write(VIEWMEM_T *cfg, UINT32 adrs, UINT8 *buf, UINT32 size);

G_END_DECLS

#endif
