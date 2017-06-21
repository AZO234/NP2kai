/**
 * @file	pcm86g.c
 * @brief	Implementation of the 86-PCM
 */

#include "compiler.h"
#include "pcm86.h"

#define PCM86GET8(p, a)													\
	do																	\
	{																	\
		(a) = (SINT8)((p)->buffer[(p)->readpos & PCM86_BUFMSK]) << 8;	\
		(p)->readpos++;													\
	} while (0 /*CONSTCOND*/)

#define PCM86GET16(p, a)												\
	do																	\
	{																	\
		(a) = (SINT8)((p)->buffer[(p)->readpos & PCM86_BUFMSK]) << 8;	\
		(a) |= (p)->buffer[((p)->readpos + 1) & PCM86_BUFMSK];			\
		(p)->readpos += 2;												\
	} while (0 /*CONSTCOND*/)

#define	BYVOLUME(p, s)	((((s) >> 6) * (p)->volume) >> (PCM86_DIVBIT + 4))


static void pcm86mono16(PCM86 pcm86, SINT32 *lpBuffer, UINT nCount)
{
	if (pcm86->div < PCM86_DIVENV)					/* アップさんぷる */
	{
		do
		{
			SINT32 smp;
			if (pcm86->divremain < 0)
			{
				SINT32 dat;
				pcm86->divremain += PCM86_DIVENV;
				pcm86->realbuf -= 2;
				if (pcm86->realbuf < 0)
				{
					goto pm16_bufempty;
				}
				PCM86GET16(pcm86, dat);
				pcm86->lastsmp = pcm86->smp;
				pcm86->smp = dat;
			}
			smp = (pcm86->lastsmp * pcm86->divremain) - (pcm86->smp * (pcm86->divremain - PCM86_DIVENV));
			lpBuffer[0] += BYVOLUME(pcm86, smp);
			lpBuffer += 2;
			pcm86->divremain -= pcm86->div;
		} while (--nCount);
	}
	else
	{
		do
		{
			SINT32 smp;
			smp = pcm86->smp * (pcm86->divremain * -1);
			pcm86->divremain += PCM86_DIVENV;
			while (1)
			{
				SINT32 dat;
				pcm86->realbuf -= 2;
				if (pcm86->realbuf < 0)
				{
					goto pm16_bufempty;
				}
				PCM86GET16(pcm86, dat);
				pcm86->lastsmp = pcm86->smp;
				pcm86->smp = dat;
				if (pcm86->divremain > pcm86->div2)
				{
					pcm86->divremain -= pcm86->div2;
					smp += pcm86->smp * pcm86->div2;
				}
				else
				{
					break;
				}
			}
			smp += pcm86->smp * pcm86->divremain;
			lpBuffer[0] += BYVOLUME(pcm86, smp);
			lpBuffer += 2;
			pcm86->divremain -= pcm86->div2;
		} while (--nCount);
	}
	return;

pm16_bufempty:
	pcm86->realbuf += 2;
	pcm86->divremain = 0;
	pcm86->smp = 0;
	pcm86->lastsmp = 0;
}

static void pcm86stereo16(PCM86 pcm86, SINT32 *lpBuffer, UINT nCount)
{
	if (pcm86->div < PCM86_DIVENV)					/* アップさんぷる */
	{
		do
		{
			SINT32 smp;
			if (pcm86->divremain < 0)
			{
				SINT32 dat;
				pcm86->divremain += PCM86_DIVENV;
				pcm86->realbuf -= 4;
				if (pcm86->realbuf < 0)
				{
					goto ps16_bufempty;
				}
				PCM86GET16(pcm86, dat);
				pcm86->lastsmp_l = pcm86->smp_l;
				pcm86->smp_l = dat;
				PCM86GET16(pcm86, dat);
				pcm86->lastsmp_r = pcm86->smp_r;
				pcm86->smp_r = dat;
			}
			smp = (pcm86->lastsmp_l * pcm86->divremain) - (pcm86->smp_l * (pcm86->divremain - PCM86_DIVENV));
			lpBuffer[0] += BYVOLUME(pcm86, smp);
			smp = (pcm86->lastsmp_r * pcm86->divremain) - (pcm86->smp_r * (pcm86->divremain - PCM86_DIVENV));
			lpBuffer[1] += BYVOLUME(pcm86, smp);
			lpBuffer += 2;
			pcm86->divremain -= pcm86->div;
		} while (--nCount);
	}
	else
	{
		do
		{
			SINT32 smp_l;
			SINT32 smp_r;
			smp_l = pcm86->smp_l * (pcm86->divremain * -1);
			smp_r = pcm86->smp_r * (pcm86->divremain * -1);
			pcm86->divremain += PCM86_DIVENV;
			while (1 /*CONSTCOND*/)
			{
				SINT32 dat;
				pcm86->realbuf -= 4;
				if (pcm86->realbuf < 4)
				{
					goto ps16_bufempty;
				}
				PCM86GET16(pcm86, dat);
				pcm86->lastsmp_l = pcm86->smp_l;
				pcm86->smp_l = dat;
				PCM86GET16(pcm86, dat);
				pcm86->lastsmp_r = pcm86->smp_r;
				pcm86->smp_r = dat;
				if (pcm86->divremain > pcm86->div2)
				{
					pcm86->divremain -= pcm86->div2;
					smp_l += pcm86->smp_l * pcm86->div2;
					smp_r += pcm86->smp_r * pcm86->div2;
				}
				else
				{
					break;
				}
			}
			smp_l += pcm86->smp_l * pcm86->divremain;
			smp_r += pcm86->smp_r * pcm86->divremain;
			lpBuffer[0] += BYVOLUME(pcm86, smp_l);
			lpBuffer[1] += BYVOLUME(pcm86, smp_r);
			lpBuffer += 2;
			pcm86->divremain -= pcm86->div2;
		} while (--nCount);
	}
	return;

ps16_bufempty:
	pcm86->realbuf += 4;
	pcm86->divremain = 0;
	pcm86->smp_l = 0;
	pcm86->smp_r = 0;
	pcm86->lastsmp_l = 0;
	pcm86->lastsmp_r = 0;
}

static void pcm86mono8(PCM86 pcm86, SINT32 *lpBuffer, UINT nCount)
{
	if (pcm86->div < PCM86_DIVENV)					/* アップさんぷる */
	{
		do
		{
			SINT32 smp;
			if (pcm86->divremain < 0)
			{
				SINT32 dat;
				pcm86->divremain += PCM86_DIVENV;
				pcm86->realbuf--;
				if (pcm86->realbuf < 0)
				{
					goto pm8_bufempty;
				}
				PCM86GET8(pcm86, dat);
				pcm86->lastsmp = pcm86->smp;
				pcm86->smp = dat;
			}
			smp = (pcm86->lastsmp * pcm86->divremain) - (pcm86->smp * (pcm86->divremain - PCM86_DIVENV));
			lpBuffer[0] += BYVOLUME(pcm86, smp);
			lpBuffer += 2;
			pcm86->divremain -= pcm86->div;
		} while (--nCount);
	}
	else
	{
		do
		{
			SINT32 smp;
			smp = pcm86->smp * (pcm86->divremain * -1);
			pcm86->divremain += PCM86_DIVENV;
			while (1 /*CONSTCOND*/)
			{
				SINT32 dat;
				pcm86->realbuf--;
				if (pcm86->realbuf < 0)
				{
					goto pm8_bufempty;
				}
				PCM86GET8(pcm86, dat);
				pcm86->lastsmp = pcm86->smp;
				pcm86->smp = dat;
				if (pcm86->divremain > pcm86->div2)
				{
					pcm86->divremain -= pcm86->div2;
					smp += pcm86->smp * pcm86->div2;
				}
				else
				{
					break;
				}
			}
			smp += pcm86->smp * pcm86->divremain;
			lpBuffer[0] += BYVOLUME(pcm86, smp);
			lpBuffer += 2;
			pcm86->divremain -= pcm86->div2;
		} while (--nCount);
	}
	return;

pm8_bufempty:
	pcm86->realbuf += 1;
	pcm86->divremain = 0;
	pcm86->smp = 0;
	pcm86->lastsmp = 0;
}

static void pcm86stereo8(PCM86 pcm86, SINT32 *lpBuffer, UINT nCount)
{
	if (pcm86->div < PCM86_DIVENV)					/* アップさんぷる */
	{
		do
		{
			SINT32 smp;
			if (pcm86->divremain < 0)
			{
				SINT32 dat;
				pcm86->divremain += PCM86_DIVENV;
				pcm86->realbuf -= 2;
				if (pcm86->realbuf < 0)
				{
					goto pm8_bufempty;
				}
				PCM86GET8(pcm86, dat);
				pcm86->lastsmp_l = pcm86->smp_l;
				pcm86->smp_l = dat;
				PCM86GET8(pcm86, dat);
				pcm86->lastsmp_r = pcm86->smp_r;
				pcm86->smp_r = dat;
			}
			smp = (pcm86->lastsmp_l * pcm86->divremain) - (pcm86->smp_l * (pcm86->divremain - PCM86_DIVENV));
			lpBuffer[0] += BYVOLUME(pcm86, smp);
			smp = (pcm86->lastsmp_r * pcm86->divremain) - (pcm86->smp_r * (pcm86->divremain - PCM86_DIVENV));
			lpBuffer[1] += BYVOLUME(pcm86, smp);
			lpBuffer += 2;
			pcm86->divremain -= pcm86->div;
		} while (--nCount);
	}
	else
	{
		do
		{
			SINT32 smp_l;
			SINT32 smp_r;
			smp_l = pcm86->smp_l * (pcm86->divremain * -1);
			smp_r = pcm86->smp_r * (pcm86->divremain * -1);
			pcm86->divremain += PCM86_DIVENV;
			while (1 /*CONSTCOND*/)
			{
				SINT32 dat;
				pcm86->realbuf -= 2;
				if (pcm86->realbuf < 0)
				{
					goto pm8_bufempty;
				}
				PCM86GET8(pcm86, dat);
				pcm86->lastsmp_l = pcm86->smp_l;
				pcm86->smp_l = dat;
				PCM86GET8(pcm86, dat);
				pcm86->lastsmp_r = pcm86->smp_r;
				pcm86->smp_r = dat;
				if (pcm86->divremain > pcm86->div2)
				{
					pcm86->divremain -= pcm86->div2;
					smp_l += pcm86->smp_l * pcm86->div2;
					smp_r += pcm86->smp_r * pcm86->div2;
				}
				else
				{
					break;
				}
			}
			smp_l += pcm86->smp_l * pcm86->divremain;
			smp_r += pcm86->smp_r * pcm86->divremain;
			lpBuffer[0] += BYVOLUME(pcm86, smp_l);
			lpBuffer[1] += BYVOLUME(pcm86, smp_r);
			lpBuffer += 2;
			pcm86->divremain -= pcm86->div2;
		} while (--nCount);
	}
	return;

pm8_bufempty:
	pcm86->realbuf += 2;
	pcm86->divremain = 0;
	pcm86->smp_l = 0;
	pcm86->smp_r = 0;
	pcm86->lastsmp_l = 0;
	pcm86->lastsmp_r = 0;
}

void SOUNDCALL pcm86gen_getpcm(PCM86 pcm86, SINT32 *lpBuffer, UINT nCount)
{
	if ((nCount) && (pcm86->fifo & 0x80) && (pcm86->div))
	{
		switch (pcm86->dactrl & 0x70)
		{
			case 0x00:						/* 16bit-none */
				break;

			case 0x10:						/* 16bit-right */
				pcm86mono16(pcm86, lpBuffer + 1, nCount);
				break;

			case 0x20:						/* 16bit-left */
				pcm86mono16(pcm86, lpBuffer, nCount);
				break;

			case 0x30:						/* 16bit-stereo */
				pcm86stereo16(pcm86, lpBuffer, nCount);
				break;

			case 0x40:						/* 8bit-none */
				break;

			case 0x50:						/* 8bit-right */
				pcm86mono8(pcm86, lpBuffer + 1, nCount);
				break;

			case 0x60:						/* 8bit-left */
				pcm86mono8(pcm86, lpBuffer, nCount);
				break;

			case 0x70:						/* 8bit-stereo */
				pcm86stereo8(pcm86, lpBuffer, nCount);
				break;
		}
		pcm86gen_checkbuf(pcm86);
	}
}
