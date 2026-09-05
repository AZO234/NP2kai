#pragma once


#ifdef __cplusplus
extern "C" {
#endif

void pcm86io_setopt(REG8 cDipSw);
void pcm86io_bind(void);
void pcm86io_unbind(void);

/* Shared PC-9801-86 compatible register handlers.  Composite boards may
 * reuse these handlers while owning their board-specific port muxing. */
void IOOUTCALL pcm86_oa460(UINT port, REG8 val);
void IOOUTCALL pcm86_oa466(UINT port, REG8 val);
void IOOUTCALL pcm86_oa468(UINT port, REG8 val);
void IOOUTCALL pcm86_oa46a(UINT port, REG8 val);
void IOOUTCALL pcm86_oa46c(UINT port, REG8 val);
REG8 IOINPCALL pcm86_ia460(UINT port);
REG8 IOINPCALL pcm86_ia466(UINT port);
REG8 IOINPCALL pcm86_ia468(UINT port);
REG8 IOINPCALL pcm86_ia46a(UINT port);

#ifdef __cplusplus
}
#endif
