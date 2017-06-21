
#ifdef __cplusplus
extern "C" {
#endif

const OEMCHAR *debugsub_flags(UINT16 flag);
const OEMCHAR *debugsub_regs(void);
void debugsub_status(void);
void debugsub_memorydump(void);
void debugsub_memorydumpall(void);

#ifdef __cplusplus
}
#endif

