/**
 * @file	wavefile.c
 * @brief	Implementation of wave file
 */

#include "compiler.h"
#include "wavefile.h"
#include "dosio.h"

#if !defined(WAVE_FORMAT_PCM)
#define WAVE_FORMAT_PCM			1		/*!< PCM */
#endif	/* !defined(WAVE_FORMAT_PCM) */

#if defined(BYTESEX_BIG)
#define MAKEID(a, b, c, d)	(UINT32)((d) | ((c) << 8) | ((b) << 16) | ((a) << 24))		/*!< FOURCCs */
#else
#define MAKEID(a, b, c, d)	(UINT32)((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))		/*!< FOURCCs */
#endif

#pragma pack(push, 1)

/**
 * @brief RIFF chunk
 */
struct TagRiffChunk
{
	UINT32 riff;				/*!< 'RIFF' */
	UINT8 fileSize[4];			/*!< fileSize */
	UINT32 fileType;			/*!< fileType */
};

/**
 * @brief chunk
 */
struct tagChunk
{
	UINT32 id;					/*!< chunkID */
	UINT8 size[4];				/*!< chunkSize */
};

/**
 * @brief WAVEFORMAT structure
 */
struct TagWaveFormat
{
	UINT8 formatTag[2];			/*!< Format type */
	UINT8 channels[2];			/*!< Number of channels */
	UINT8 samplePerSec[4];		/*!< Sample rate */
	UINT8 avgBytesPerSec[4];	/*!< Required average data transfer rate */
	UINT8 blockAlign[2];		/*!< Block alignment */
	UINT8 bitsPerSample[2];		/*!< Bits per sample */
};

#pragma pack(pop)

/**
 * @brief The informations of wave
 */
struct TagWaveFile
{
	FILEH fh;				/*!< The handle of file */
	UINT nRate;				/*!< The sampling rate */
	UINT nBits;				/*!< The bits of the sample */
	UINT nChannels;			/*!< The number of the channel */

	UINT nDataSize;			/*!< The size of the file */
	UINT8* lpCurrent;		/*!< The current */
	UINT nRemain;			/*!< The remain of the buffer */
	UINT8 buffer[4096];		/*!< The buffer */
};

/**
 * Write the header
 * @param[in] hWave The handle of wave
 * @retval SUCCESS If succeeded
 * @retval FAILURE If failed
 */
static BRESULT WriteHeader(WAVEFILEH hWave)
{
	struct TagRiffChunk riff;
	struct tagChunk chunk;
	struct TagWaveFormat format;
	UINT nFileSize;
	UINT nBlockAlign;
	UINT nAvgBytesPerSec;

	nFileSize = hWave->nDataSize;
	nFileSize += 4 + sizeof(chunk) + sizeof(format) + sizeof(chunk);

	riff.riff = MAKEID('R', 'I', 'F', 'F');
	STOREINTELDWORD(riff.fileSize, nFileSize);
	riff.fileType = MAKEID('W', 'A', 'V', 'E');
	if (file_write(hWave->fh, &riff, sizeof(riff)) != sizeof(riff))
	{
		return FAILURE;
	}

	chunk.id = MAKEID('f', 'm', 't', ' ');
	STOREINTELDWORD(chunk.size, sizeof(format));
	if (file_write(hWave->fh, &chunk, sizeof(chunk)) != sizeof(chunk))
	{
		return FAILURE;
	}

	nBlockAlign = hWave->nChannels * (hWave->nBits / 8);
	nAvgBytesPerSec = nBlockAlign * hWave->nRate;
	STOREINTELWORD(format.formatTag, WAVE_FORMAT_PCM);
	STOREINTELWORD(format.channels, hWave->nChannels);
	STOREINTELDWORD(format.samplePerSec, hWave->nRate);
	STOREINTELDWORD(format.avgBytesPerSec, nAvgBytesPerSec);
	STOREINTELWORD(format.blockAlign, nBlockAlign);
	STOREINTELWORD(format.bitsPerSample, hWave->nBits);
	if (file_write(hWave->fh, &format, sizeof(format)) != sizeof(format))
	{
		return FAILURE;
	}

	chunk.id = MAKEID('d', 'a', 't', 'a');
	STOREINTELDWORD(chunk.size, hWave->nDataSize);
	if (file_write(hWave->fh, &chunk, sizeof(chunk)) != sizeof(chunk))
	{
		return FAILURE;
	}
	return SUCCESS;
}

/**
 * Flash
 * @param[in] hWave The handle of wave
 */
static void FlashBuffer(WAVEFILEH hWave)
{
	UINT nSize;

	nSize = (UINT)(hWave->lpCurrent - hWave->buffer);
	if (nSize)
	{
		hWave->nDataSize += file_write(hWave->fh, hWave->buffer, nSize);
	}
	hWave->lpCurrent = hWave->buffer;
	hWave->nRemain = sizeof(hWave->buffer);
}

/**
 * Creates
 * @param[in] lpFilename The filename
 * @param[in] nRate The sampling rate
 * @param[in] nBits The bits of the sample
 * @param[in] nChannels The number of the channels
 * @return The handle of wave
 */
WAVEFILEH wavefile_create(const OEMCHAR *lpFilename, UINT nRate, UINT nBits, UINT nChannels)
{
	FILEH fh = FILEH_INVALID;
	WAVEFILEH hWave = NULL;

	do
	{
		if (lpFilename == NULL)
		{
			break;
		}
		if (nRate == 0)
		{
			break;
		}
		if ((nBits != 8) && (nBits != 16))
		{
			break;
		}
		if ((nChannels != 1) && (nChannels != 2))
		{
			break;
		}

		fh = file_create(lpFilename);
		if (fh == FILEH_INVALID)
		{
			break;
		}

		hWave = (WAVEFILEH)_MALLOC(sizeof(*hWave), "WAVEFILEH");
		if (hWave == NULL)
		{
			break;
		}

		memset(hWave, 0, sizeof(*hWave));
		hWave->fh = fh;
		hWave->nRate = nRate;
		hWave->nBits = nBits;
		hWave->nChannels = nChannels;
		if (WriteHeader(hWave) != SUCCESS)
		{
			break;
		}
		hWave->lpCurrent = hWave->buffer;
		hWave->nRemain = sizeof(hWave->buffer);
		return hWave;
	} while (FALSE /*CONSTCOND*/);

	if (hWave)
	{
		_MFREE(hWave);
	}
	if (fh != FILEH_INVALID)
	{
		file_close(fh);
	}
	return NULL;
}

/**
 * Write
 * @param[in] hWave The handle of wave
 * @param[in] lpBuffer The buffer
 * @param[in] cbBuffer The size of the buffer
 * @return The written size
 */
UINT wavefile_write(WAVEFILEH hWave, const void *lpBuffer, UINT cbBuffer)
{
	if (hWave == NULL)
	{
		return 0;
	}
	while (cbBuffer)
	{
		UINT nSize = min(hWave->nRemain, cbBuffer);
		memcpy(hWave->lpCurrent, lpBuffer, nSize);
		lpBuffer = ((UINT8 *)lpBuffer) + nSize;
		cbBuffer -= nSize;
		hWave->lpCurrent += nSize;
		hWave->nRemain -= nSize;
		if (hWave->nRemain == 0)
		{
			FlashBuffer(hWave);
		}
	}
	return 0;
}

/**
 * Close
 * @param[in] hWave The handle of wave
 */
void wavefile_close(WAVEFILEH hWave)
{
	if (hWave)
	{
		FlashBuffer(hWave);
		file_seek(hWave->fh, 0, FSEEK_SET);
		WriteHeader(hWave);
		file_close(hWave->fh);
		_MFREE(hWave);
	}
}
