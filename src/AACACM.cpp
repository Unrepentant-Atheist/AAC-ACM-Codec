/*
 * AACACM.cpp
 * Copyright (C) 2011-2012 fccHandler <fcchandler@comcast.net>
 * Copyright (C) 2003-2005 M. Bakker, Nero AG, http://www.nero.com
 *
 * AACACM is an open source Advanced Audio Codec for Windows Audio
 * Compression Manager. The AAC decoder was taken from faad2.
 *
 * See http://fcchandler.home.comcast.net/ for AACACM updates.
 * See http://www.audiocoding.com/faad2.html for faad2 updates.
 *
 * AACACM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * AACACM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include <windows.h>
#include <mmreg.h>
#include <mmsystem.h>
#include <msacm.h>
#include <ks.h>
#include <ksmedia.h>
#include "WinDDK.h"
#include "resource.h"

#include "..\faad2-2.7\include\neaacdec.h"

// Make sure this matches faad2-2.7\libfaad\structs.h:
#define MAX_SYNTAX_ELEMENTS 48


#ifdef _DEBUG
	#define AACACM_LOGFILE
	#define AACACM_LOGMODE "w"
#endif

#include <stdio.h>

typedef struct {
	FOURCC          fccType;    // fccType from ACMDRVOPENDESC
	HDRVR           hdrvr;      // hdrvr (as sent by the caller)
	HMODULE         hmod;       // driver module handle
	DWORD           dwVersion;  // dwVersion from ACMDRVOPENDESC
	DWORD           dwFlags;    // codec flags (see below)
#ifdef AACACM_LOGFILE
	ACMDRVOPENDESCW *pdod;
	FILE            *fLogFile;  // "%HOMEDRIVE%:\AACACM.log" (for debugging)
#endif
} MyData;

// Flags for MyData.dwFlags
#define	AACACM_MULTICHANNEL		1	// enable > 2 channel output
#define AACACM_706D             2	// support format tag 0x706D

// Support WAVEFORMATEXTENSIBLE?
//#define AACACM_EXTENSIBLE


// Registry key
static const char MyRegKey[] = "Software\\fccHandler\\AACACM";


typedef struct {
	NeAACDecHandle  aacHandle;

	unsigned char*  inbuf;
	unsigned char*  inptr;

	unsigned char*  outbuf;
	unsigned char*  outptr;

	unsigned long   inbytes;
	unsigned long   outbytes;
	int             flags;
} MyStreamData;

// Flags for MyStreamData.flags

#define	AACACM_FIRST_TIME   1	// first time decoding this stream
#define AACACM_HAS_ADTS     2	// stream has ADTS headers


// AAC format tags from wmcodecdsp.h

#define WAVE_FORMAT_RAW_AAC1       0x00FF
#define WAVE_FORMAT_MPEG_ADTS_AAC  0x1600
#define WAVE_FORMAT_FAAC51         0x706D


// We support 12 sampling rates and 7 channel configurations

#define AACACM_NFREQS 12
#define AACACM_NCHANS 7

static const unsigned long freq_map[AACACM_NFREQS] = {
	96000, 88200, 64000, 48000, 44100, 32000,
	24000, 22050, 16000, 12000, 11025,  8000
};

static const unsigned char chan_map[AACACM_NCHANS] = {
	1, 2, 3, 4, 5, 6, 8
};


BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}


static int LoadResString(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, DWORD cchBuffer)
{
	int i = 0;

	// Assume the OS has no Unicode support
	if (cchBuffer > 0)
	{
		HGLOBAL h = GlobalAlloc(GPTR, cchBuffer+1);
		if (h != NULL)
		{
			i = LoadStringA(hInstance, uID, (LPSTR)h, cchBuffer);

			if (i > 0)
			{
				if ((DWORD)i >= cchBuffer) i = cchBuffer - 1;

				MultiByteToWideChar(CP_ACP,
					MB_PRECOMPOSED, (LPSTR)h, i, lpBuffer, i);
			}
			else
			{
				i = 0;
			}

			GlobalFree(h);
		}

		if (lpBuffer) lpBuffer[i] = L'\0';
	}

	return i;
}


static bool IsValidPCM(const WAVEFORMATEX *wfex)
{
	// Does "wfex" describe a PCM format we can decompress to?

	if (wfex != NULL)
	{
		const WORD  ch  = wfex->nChannels;
		const DWORD ba  = wfex->nBlockAlign;
		const DWORD sps = wfex->nSamplesPerSec;

		int i;

		if (wfex->wFormatTag != WAVE_FORMAT_PCM) goto Abort;
		
		for (i = 0; i < AACACM_NCHANS; ++i)
		{
			if (ch == chan_map[i]) break;
		}
		if (i == AACACM_NCHANS) goto Abort;

		for (i = 0; i < AACACM_NFREQS; ++i)
		{
			if (sps == freq_map[i]) break;
		}
		if (i == AACACM_NFREQS) goto Abort;

		if (wfex->wBitsPerSample != 16) goto Abort;
		if (ba != (ch * 2U)) goto Abort;
		if (wfex->nAvgBytesPerSec != ba * sps) goto Abort;
		
		return true;
	}
Abort:
	return false;
}


#ifdef AACACM_EXTENSIBLE

// These are the channel configurations we support:
static const DWORD channel_masks[8] = {
	KSAUDIO_SPEAKER_MONO,
	KSAUDIO_SPEAKER_STEREO,
	KSAUDIO_SPEAKER_MONO | KSAUDIO_SPEAKER_STEREO,
	KSAUDIO_SPEAKER_QUAD,
	KSAUDIO_SPEAKER_MONO | KSAUDIO_SPEAKER_QUAD,
	KSAUDIO_SPEAKER_5POINT1,
	0,
	KSAUDIO_SPEAKER_7POINT1_SURROUND
};


static bool IsValidPCMEX(const WAVEFORMATPCMEX *wfex)
{
	// Does "wfex" describe an extensible format we can decompress to?

	if (wfex != NULL)
	{
		const WORD  ch  = wfex->Format.nChannels;
		const DWORD ba  = wfex->Format.nBlockAlign;
		const DWORD sps = wfex->Format.nSamplesPerSec;
		const WORD  bps = wfex->Format.wBitsPerSample;

		int i;

		if (wfex->Format.wFormatTag != WAVE_FORMAT_EXTENSIBLE) goto Abort;

		for (i = 0; i < AACACM_NCHANS; ++i)
		{
			if (ch == chan_map[i]) break;
		}
		if (i == AACACM_NCHANS) goto Abort;

		for (i = 0; i < AACACM_NFREQS; ++i)
		{
			if (sps == freq_map[i]) break;
		}
		if (i == AACACM_NFREQS) goto Abort;

		if (bps != 16) goto Abort;
		if (ba != (ch * 2U)) goto Abort;
		if (wfex->Format.nAvgBytesPerSec != ba * sps) goto Abort;

		if (wfex->Format.cbSize < (sizeof(wfex) - sizeof(wfex->Format))) goto Abort;
		if (wfex->Samples.wValidBitsPerSample != bps) goto Abort;
		if (wfex->dwChannelMask != channel_masks[ch - 1]) goto Abort;
		if (wfex->SubFormat != KSDATAFORMAT_SUBTYPE_PCM) goto Abort;

		return true;
	}
Abort:
	return false;
}

#endif	// AACACM_EXTENSIBLE


static bool IsValidAAC(const WAVEFORMATEX *wfex, const MyData *md)
{
	// Does "wfex" describe an AAC format we can decode?

	if (wfex != NULL)
	{
		int i;

		if (wfex->wFormatTag != WAVE_FORMAT_RAW_AAC1)
		{
			if (!(md->dwFlags & AACACM_706D)) goto Abort;
			if (wfex->wFormatTag != WAVE_FORMAT_FAAC51) goto Abort;
		}

		for (i = 0; i < AACACM_NCHANS; ++i)
		{
			if (wfex->nChannels == chan_map[i]) break;
		}
		if (i == AACACM_NCHANS) goto Abort;

		for (i = 0; i < AACACM_NFREQS; ++i)
		{
			if (wfex->nSamplesPerSec == freq_map[i]) break;
		}
		if (i == AACACM_NFREQS) goto Abort;

		if (wfex->nAvgBytesPerSec == 0) goto Abort;
		if (wfex->nBlockAlign == 0) goto Abort;
		if (wfex->cbSize < 2) goto Abort;
		
		return true;
	}
Abort:
	return false;
}


#ifdef AACACM_LOGFILE
static void log_format(DWORD_PTR dwDriverId, const WAVEFORMATEX *wfex)
{
	FILE *fh = ((MyData *)dwDriverId)->fLogFile;

	if (fh != NULL)
	{
		if (wfex != NULL)
		{
			fprintf(fh, "  wFormatTag          = 0x%04X\n", wfex->wFormatTag);
			fprintf(fh, "  nChannels           = %hu\n", wfex->nChannels);
			fprintf(fh, "  nSamplesPerSec      = %lu\n", wfex->nSamplesPerSec);
			fprintf(fh, "  nAvgBytesPerSec     = %lu\n", wfex->nAvgBytesPerSec);
			fprintf(fh, "  nBlockAlign         = %hu\n", wfex->nBlockAlign);
			fprintf(fh, "  wBitsPerSample      = %hu\n", wfex->wBitsPerSample);
			fprintf(fh, "  cbSize              = %hu\n", wfex->cbSize);

#ifdef AACACM_EXTENSIBLE
			if (IsValidPCMEX((WAVEFORMATPCMEX *)wfex))
			{
				const WAVEFORMATPCMEX *pcmex = (WAVEFORMATPCMEX *)wfex;

				fprintf(fh, "  wValidBitsPerSample = %hu\n", pcmex->Samples.wValidBitsPerSample);
				fprintf(fh, "  dwChannelMask       = %lu\n", pcmex->dwChannelMask);
				fprintf(fh, "  SubFormat           = KSDATAFORMAT_SUBTYPE_PCM\n");
			}
#endif
		}
		else
		{
			fprintf(fh, "  log_format(): wfex is NULL!");
		}
	}
}
#endif


static void WriteReg(const MyData *md)
{
	// Store configuration options in Registry

	if (md != NULL)
	{
		HKEY hKey;

		if (RegCreateKeyEx(HKEY_CURRENT_USER, MyRegKey, 0,
			NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
			NULL, &hKey, NULL) == ERROR_SUCCESS)
		{
			DWORD dwData = md->dwFlags;

			RegSetValueExA(hKey, "Flags", 0, REG_DWORD,
				(CONST BYTE *)&dwData, sizeof(dwData));

			RegCloseKey(hKey);
		}
	}
}


static void ReadReg(MyData *md)
{
	// Read configuration options from Registry

	if (md != NULL)
	{
		HKEY hKey;

		// Default
		md->dwFlags = AACACM_MULTICHANNEL;

		if (RegOpenKeyExA(HKEY_CURRENT_USER, MyRegKey,
			0, KEY_READ, &hKey) == ERROR_SUCCESS)
		{
			DWORD dwData;
			DWORD dwType;

			DWORD cbData = sizeof(dwData);

			if (RegQueryValueExA(hKey, "Flags", NULL, &dwType,
				(LPBYTE)&dwData, &cbData) == ERROR_SUCCESS)
			{
				if (dwType == REG_DWORD && cbData == sizeof(dwData))
				{
					md->dwFlags = dwData;
				}
			}

			RegCloseKey(hKey);
		}
	}
}


#ifdef AACACM_LOGFILE
static inline char GetHomeDrive()
{
	char pszHome[4];
	if (ExpandEnvironmentStringsA("%HOMEDRIVE%", pszHome, sizeof(pszHome)) > 1)
	{
		return pszHome[0];
	}
	return 'C';
}
#endif


static LRESULT drv_open(HDRVR hdrvr, LPARAM lParam2)
{
	ACMDRVOPENDESCW *dod = (ACMDRVOPENDESCW *)lParam2;
	MyData *md;

	if (dod != NULL)
	{
		if (dod->fccType != ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC)
		{
			return NULL;
		}
	}

	md = (MyData *)LocalAlloc(LPTR, sizeof(MyData));

	if (md != NULL)
	{
		md->hdrvr	= hdrvr;
		md->hmod	= GetDriverModuleHandle(hdrvr);
		md->fccType	= ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;

		// Fetch options from Registry
		ReadReg(md);

#ifdef AACACM_LOGFILE
		md->fLogFile = NULL;
		if (md->pdod = dod)
#else
		if (dod != NULL)
#endif
		{
			md->dwVersion = dod->dwVersion;
			dod->dwError = MMSYSERR_NOERROR;
		}

#ifdef AACACM_LOGFILE
		{
			char logname[16];
			sprintf(logname, "%c:\\AACACM.log", GetHomeDrive());

			if (logname[0] != '\0')
			{
				md->fLogFile = fopen(logname, AACACM_LOGMODE);
				if (md->fLogFile != NULL)
				{
					fprintf(md->fLogFile, "DRV_OPEN: successful\n");
					fprintf(md->fLogFile, " ACMDRVOPENDESCW = 0x%p\n", dod);
					fflush(md->fLogFile);
				}
			}
		}
#endif
	}
	else if (dod != NULL)
	{
		dod->dwError = MMSYSERR_NOMEM;
	}

	return (LRESULT)md;
}


static LRESULT drv_close(DWORD_PTR dwDriverId)
{
    if (dwDriverId != NULL)
	{

#ifdef AACACM_LOGFILE
		const MyData *md = (MyData *)dwDriverId;

		if (md->fLogFile != NULL && md->pdod != NULL)
		{
			fprintf(md->fLogFile, "DRV_CLOSE\n");
			fclose(md->fLogFile);
		}
#endif

		LocalFree((HLOCAL)dwDriverId);
	}

	return (LRESULT)TRUE;
}


static LRESULT driver_details(DWORD_PTR dwDriverId, LPARAM lParam1)
{
	const MyData *md = (MyData *)dwDriverId;
	ACMDRIVERDETAILSW tmp;

	int cbStruct = ((ACMDRIVERDETAILSW *)lParam1)->cbStruct;

	if (cbStruct > sizeof(tmp)) cbStruct = sizeof(tmp);

	tmp.cbStruct    = cbStruct;
	tmp.fccType     = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
	tmp.fccComp     = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
	tmp.wMid        = 0;
	tmp.wPid        = 0;
	tmp.vdwACM      = 0x03320000;	// 3.50
	tmp.vdwDriver   = 0x01090000;	// 1.09
	tmp.fdwSupport  = ACMDRIVERDETAILS_SUPPORTF_CODEC;
	tmp.cFormatTags = 2;	// PCM, AAC
	tmp.cFilterTags = 0;

	if (md->dwFlags & AACACM_706D) ++tmp.cFormatTags;

#ifdef AACACM_EXTENSIBLE
	++tmp.cFormatTags;
#endif

	if (cbStruct > FIELD_OFFSET(ACMDRIVERDETAILSW, hicon))
	{
		tmp.hicon = NULL;

		LoadResString(md->hmod, IDS_SHORTNAME, tmp.szShortName, ACMDRIVERDETAILS_SHORTNAME_CHARS);
		LoadResString(md->hmod, IDS_LONGNAME, tmp.szLongName, ACMDRIVERDETAILS_LONGNAME_CHARS);

		if (cbStruct > FIELD_OFFSET(ACMDRIVERDETAILSW, szCopyright))
		{
			LoadResString(md->hmod, IDS_COPYRIGHT, tmp.szCopyright, ACMDRIVERDETAILS_COPYRIGHT_CHARS);
			LoadResString(md->hmod, IDS_LICENSING, tmp.szLicensing, ACMDRIVERDETAILS_LICENSING_CHARS);
			LoadResString(md->hmod, IDS_FEATURES, tmp.szFeatures, ACMDRIVERDETAILS_FEATURES_CHARS);
		}
	}

	memcpy((void *)lParam1, &tmp, cbStruct);

#ifdef AACACM_LOGFILE
	if (md->fLogFile != NULL)
	{
		fprintf(md->fLogFile, "ACMDM_DRIVERDETAILS\n");
		fflush(md->fLogFile);
	}
#endif

	return MMSYSERR_NOERROR;
}


static LRESULT format_suggest(DWORD_PTR dwDriverId, LPARAM lParam1)
{
	const MyData *md = (MyData *)dwDriverId;
	ACMDRVFORMATSUGGEST *fs = (ACMDRVFORMATSUGGEST *)lParam1;

	WAVEFORMATEX *src = fs->pwfxSrc;
	WAVEFORMATEX *dst = fs->pwfxDst;

	const DWORD flags = fs->fdwSuggest & (
		ACM_FORMATSUGGESTF_WFORMATTAG |
		ACM_FORMATSUGGESTF_NCHANNELS |
		ACM_FORMATSUGGESTF_NSAMPLESPERSEC |
		ACM_FORMATSUGGESTF_WBITSPERSAMPLE
	);

	if ((~flags) & fs->fdwSuggest)
	{

#ifdef AACACM_LOGFILE
		if (md->fLogFile != NULL)
		{
			fprintf(md->fLogFile, "ACMDM_FORMAT_SUGGEST: unsupported fdwSuggest (0x%08X)\n", fs->fdwSuggest);
			fflush(md->fLogFile);
		}
#endif
		return MMSYSERR_NOTSUPPORTED;
	}

	if (IsValidAAC(src, md))
	{
		// Number of channels must be the same
		if (flags & ACM_FORMATSUGGESTF_NCHANNELS)
		{
			if (dst->nChannels != src->nChannels) goto Abort;
		}
		else
		{
			dst->nChannels = src->nChannels;
		}

		// Are we allowed to return multichannel PCM?
		if (!(md->dwFlags & AACACM_MULTICHANNEL))
		{
			if (dst->nChannels != 1 && dst->nChannels != 2) goto Abort;
		}

		if (flags & ACM_FORMATSUGGESTF_WFORMATTAG)
		{
			if	(  dst->wFormatTag != WAVE_FORMAT_PCM
#ifdef AACACM_EXTENSIBLE
				&& dst->wFormatTag != WAVE_FORMAT_EXTENSIBLE
#endif
				)
			{
				goto Abort;
			}
		}
		else
		{
#ifdef AACACM_EXTENSIBLE
			if (dst->nChannels > 2)
			{
				dst->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
			}
			else
#endif
			dst->wFormatTag = WAVE_FORMAT_PCM;
		}

		// We don't convert the sample rate
		if (flags & ACM_FORMATSUGGESTF_NSAMPLESPERSEC)
		{
			if (dst->nSamplesPerSec != src->nSamplesPerSec) goto Abort;
		}
		else
		{
			dst->nSamplesPerSec = src->nSamplesPerSec;
		}

		// We only support 16-bit PCM
		if (flags & ACM_FORMATSUGGESTF_WBITSPERSAMPLE)
		{
			if (dst->wBitsPerSample != 16) goto Abort;
		}
		else
		{
			dst->wBitsPerSample = 16;
		}

		dst->nBlockAlign = (dst->wBitsPerSample / 8) * dst->nChannels;
		dst->nAvgBytesPerSec = dst->nBlockAlign * dst->nSamplesPerSec;

		if (fs->cbwfxDst >= sizeof(WAVEFORMATEX))
		{
#ifdef AACACM_EXTENSIBLE
			if (dst->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
			{
				WAVEFORMATPCMEX *pcmex = (WAVEFORMATPCMEX *)dst;

				if (fs->cbwfxDst < sizeof(WAVEFORMATPCMEX)) goto Abort;
				
				dst->cbSize = sizeof(WAVEFORMATPCMEX) - sizeof(WAVEFORMATEX);

				pcmex->Samples.wValidBitsPerSample = dst->wBitsPerSample;
				pcmex->dwChannelMask = channel_masks[dst->nChannels - 1];
				pcmex->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
			}
			else
#endif
			dst->cbSize = 0;
		}

	}
	else
	{
		goto Abort;
	}

#ifdef AACACM_LOGFILE
	if (md->fLogFile != NULL)
	{
		fprintf(md->fLogFile, "ACMDM_FORMAT_SUGGEST:\n");
		fprintf(md->fLogFile, " fdwSuggest = 0x%08X\n", fs->fdwSuggest);

		fprintf(md->fLogFile, " source:\n");
		log_format(dwDriverId, src);

		fprintf(md->fLogFile, " destination:\n");
		log_format(dwDriverId, dst);

		fflush(md->fLogFile);
	}
#endif

	return MMSYSERR_NOERROR;

Abort:
#ifdef AACACM_LOGFILE
	if (md->fLogFile != NULL)
	{
		fprintf(md->fLogFile, "ACMDM_FORMAT_SUGGEST: not possible\n");
		fprintf(md->fLogFile, " fdwSuggest = 0x%08X\n", fs->fdwSuggest);

		fprintf(md->fLogFile, " source:\n");
		log_format(dwDriverId, src);

		fprintf(md->fLogFile, " destination:\n");
		log_format(dwDriverId, dst);

		fflush(md->fLogFile);
	}
#endif

	return ACMERR_NOTPOSSIBLE;
}


static bool IsSupportedTag(DWORD dwFormatTag, const MyData *md)
{
	if (dwFormatTag == WAVE_FORMAT_PCM) goto Done;

	if (dwFormatTag == WAVE_FORMAT_RAW_AAC1) goto Done;

	if (md->dwFlags & AACACM_706D)
	{
		if (dwFormatTag == WAVE_FORMAT_FAAC51) goto Done;
	}

#ifdef AACACM_EXTENSIBLE
	if (dwFormatTag == WAVE_FORMAT_EXTENSIBLE) goto Done;
#endif

	return false;

Done:
	return true;
}


static bool IndexToTag(DWORD dwIndex, DWORD& dwFormatTag, const MyData *md)
{
	if (dwIndex == 0)
	{
		dwFormatTag = WAVE_FORMAT_PCM;
		goto Done;
	}

	if (dwIndex == 1)
	{
		dwFormatTag = WAVE_FORMAT_RAW_AAC1;
		goto Done;
	}

	if (dwIndex == 2)
	{
		if (md->dwFlags & AACACM_706D)
		{
			dwFormatTag = WAVE_FORMAT_FAAC51;
			goto Done;
		}
#ifdef AACACM_EXTENSIBLE
		dwFormatTag = WAVE_FORMAT_EXTENSIBLE;
		goto Done;
#endif
	}

#ifdef AACACM_EXTENSIBLE
	if (dwIndex == 3)
	{
		if (md->dwFlags & AACACM_706D)
		{
			dwFormatTag = WAVE_FORMAT_EXTENSIBLE;
			goto Done;
		}
	}
#endif

	return false;

Done:
	return true;
}


static LRESULT formattag_details(DWORD_PTR dwDriverId, LPARAM lParam1, LPARAM lParam2)
{
	ACMFORMATTAGDETAILSW *fmtd = (ACMFORMATTAGDETAILSW *)lParam1;
	const MyData *md = (MyData *)dwDriverId;
	DWORD format;

#ifdef AACACM_LOGFILE
	if (md->fLogFile != NULL)
	{
		fprintf(md->fLogFile, "ACMDM_FORMATTAG_DETAILS\n");
		fflush(md->fLogFile);
	}
#endif

	switch (lParam2 & ACM_FORMATTAGDETAILSF_QUERYMASK) {

	case ACM_FORMATTAGDETAILSF_INDEX:

#ifdef AACACM_LOGFILE
		if (md->fLogFile != NULL)
		{
			fprintf(md->fLogFile, " ACM_FORMATTAGDETAILSF_INDEX\n");
			fprintf(md->fLogFile, " dwFormatTagIndex = %lu\n", fmtd->dwFormatTagIndex);
			fflush(md->fLogFile);
		}
#endif
		if (!IndexToTag(fmtd->dwFormatTagIndex, format, md)) goto Abort;
		break;

	case ACM_FORMATTAGDETAILSF_FORMATTAG:

#ifdef AACACM_LOGFILE
		if (md->fLogFile != NULL)
		{
			fprintf(md->fLogFile, " ACM_FORMATTAGDETAILSF_FORMATTAG\n");
			fprintf(md->fLogFile, " dwFormatTag = 0x%08X\n", fmtd->dwFormatTag);
			fflush(md->fLogFile);
		}
#endif
		format = fmtd->dwFormatTag;
		if (!IsSupportedTag(format, md)) goto Abort;
		break;

	case ACM_FORMATTAGDETAILSF_LARGESTSIZE:

#ifdef AACACM_LOGFILE
		if (md->fLogFile != NULL)
		{
			fprintf(md->fLogFile, " ACM_FORMATTAGDETAILSF_LARGESTSIZE\n");
			fprintf(md->fLogFile, " dwFormatTag = 0x%08X\n", fmtd->dwFormatTag);
			fflush(md->fLogFile);
		}
#endif

		if (fmtd->dwFormatTag == WAVE_FORMAT_UNKNOWN)
		{
#ifdef AACACM_EXTENSIBLE
			format = WAVE_FORMAT_EXTENSIBLE;
#else
			format = WAVE_FORMAT_RAW_AAC1;
#endif
		}
		else
		{
			format = fmtd->dwFormatTag;
			if (!IsSupportedTag(format, md)) goto Abort;
		}
		break;

	default:
#ifdef AACACM_LOGFILE
		if (md->fLogFile != NULL)
		{
			fprintf(md->fLogFile, " Unsupported query: 0x%08X\n", (DWORD)lParam2);
			fflush(md->fLogFile);
		}
#endif
		goto Abort;
	}

	// "format" is the format to return details about

	switch (format) {
	
	case WAVE_FORMAT_PCM:

		fmtd->dwFormatTag       = WAVE_FORMAT_PCM;
		fmtd->dwFormatTagIndex  = 0;
        fmtd->cbFormatSize      = sizeof(PCMWAVEFORMAT);
		fmtd->fdwSupport        = ACMDRIVERDETAILS_SUPPORTF_CODEC;

		// Are we allowed to return multichannel PCM?
		if (md->dwFlags & AACACM_MULTICHANNEL)
		{
			fmtd->cStandardFormats = AACACM_NFREQS * AACACM_NCHANS;
		} else {
			fmtd->cStandardFormats = AACACM_NFREQS * 2;
		}
		fmtd->szFormatTag[0] = L'\0';
		break;

	case WAVE_FORMAT_RAW_AAC1:
	case WAVE_FORMAT_FAAC51:

		fmtd->dwFormatTag       = format;
		fmtd->dwFormatTagIndex  = (format == WAVE_FORMAT_RAW_AAC1)? 1: 2;
		fmtd->cbFormatSize      = sizeof(WAVEFORMATEX) + 2;
		fmtd->fdwSupport        = ACMDRIVERDETAILS_SUPPORTF_CODEC;

        fmtd->cStandardFormats	= AACACM_NFREQS * AACACM_NCHANS;

		LoadResString(md->hmod, IDS_LONGNAME,
			fmtd->szFormatTag, ACMFORMATTAGDETAILS_FORMATTAG_CHARS);
		break;

#ifdef AACACM_EXTENSIBLE
	case WAVE_FORMAT_EXTENSIBLE:
		fmtd->dwFormatTag       = WAVE_FORMAT_EXTENSIBLE;
		fmtd->dwFormatTagIndex  = (md->dwFlags & AACACM_706D)? 3: 2;
		fmtd->cbFormatSize      = sizeof(WAVEFORMATPCMEX);
		fmtd->fdwSupport        = ACMDRIVERDETAILS_SUPPORTF_CODEC;

        fmtd->cStandardFormats	= AACACM_NFREQS * AACACM_NCHANS;
		fmtd->szFormatTag[0]    = L'\0';
		break;
#endif

	default:
		goto Abort;
	}

	if (fmtd->cbStruct > sizeof(ACMFORMATTAGDETAILSW))
	{
		fmtd->cbStruct = sizeof(ACMFORMATTAGDETAILSW);
	}

	return MMSYSERR_NOERROR;

Abort:
	return MMSYSERR_NOTSUPPORTED;
}


static inline unsigned short make_aac_config(DWORD srate, WORD chans)
{
	// Create two-byte AAC AudioSpecificConfig

	unsigned short asc = 0;

	// AudioSpecificConfig, 2 bytes

	//   LSB      MSB
	// xxxxx... ........  object type (00010 = AAC LC)
	// .....xxx x.......  sampling frequency index (0 to 11)
	// ........ .xxxx...  channels configuration (1 to 7)
	// ........ .....x..  frame length flag (0 = 1024, 1 = 960)
	// ........ ......x.  depends on core coder (0)
	// ........ .......x  extensions flag (0)
		
	//   LSB      MSB
	// .....000 0....... = 96000
	// .....000 1....... = 88200
	// .....001 0....... = 64000
	// .....001 1....... = 48000
	// .....010 0....... = 44100
	// .....010 1....... = 32000
	// .....011 0....... = 24000
	// .....011 1....... = 22050
	// .....100 0....... = 16000
	// .....100 1....... = 12000
	// .....101 0....... = 11025
	// .....101 1....... =  8000

	switch (srate) {
	case 96000: break;
	case 88200: asc |= 0x8000; break;
	case 64000: asc |= 0x0001; break;
	case 48000: asc |= 0x8001; break;
	case 44100: asc |= 0x0002; break;
	case 32000: asc |= 0x8002; break;
	case 24000: asc |= 0x0003; break;
	case 22050: asc |= 0x8003; break;
	case 16000: asc |= 0x0004; break;
	case 12000: asc |= 0x8004; break;
	case 11025: asc |= 0x0005; break;
	case  8000: asc |= 0x8005; break;
	default:
		return 0;
	}

	//   LSB      MSB
	// ........ .0001... = 1 channel
	// ........ .0010... = 2 channels
	// ........ .0011... = 3 channels
	// ........ .0100... = 4 channels
	// ........ .0101... = 5 channels
	// ........ .0110... = 6 channels
	// ........ .0111... = 8 channels

	switch (chans) {
	case 1: asc |= 0x0800; break;
	case 2: asc |= 0x1000; break;
	case 3: asc |= 0x1800; break;
	case 4: asc |= 0x2000; break;
	case 5: asc |= 0x2800; break;
	case 6: asc |= 0x3000; break;
	case 8: asc |= 0x3800; break;
	default:
		return 0;
	}

	return (asc | 0x0010);
}


static LRESULT format_details(DWORD_PTR dwDriverId, LPARAM lParam1, LPARAM lParam2)
{
	ACMFORMATDETAILSW *fmtd = (ACMFORMATDETAILSW *)lParam1;
	const MyData *md = (MyData *)dwDriverId;

	WAVEFORMATPCMEX outmem = { 0 };

	unsigned int idiv;
	unsigned int imod;

#ifdef AACACM_LOGFILE
	if (md->fLogFile != NULL)
	{
		fprintf(md->fLogFile, "ACMDM_FORMAT_DETAILS\n");
		fflush(md->fLogFile);
	}
#endif

	fmtd->fdwSupport  = ACMDRIVERDETAILS_SUPPORTF_CODEC;
	fmtd->szFormat[0] = L'\0';

	switch (lParam2 & ACM_FORMATDETAILSF_QUERYMASK) {
	
	case ACM_FORMATDETAILSF_INDEX:

#ifdef AACACM_LOGFILE
		if (md->fLogFile != NULL)
		{
			fprintf(md->fLogFile, " ACM_FORMATDETAILSF_INDEX\n");
			fprintf(md->fLogFile, " dwFormatIndex = %lu\n", fmtd->dwFormatIndex);
			fprintf(md->fLogFile, " dwFormatTag = 0x%08X\n", fmtd->dwFormatTag);
			fflush(md->fLogFile);
		}
#endif
		if (!IsSupportedTag(fmtd->dwFormatTag, md)) goto Abort;

		// If the ACM_FORMATDETAILSF_INDEX flag is set, the client
		// has specified an index value in the dwFormatIndex member
		// of ACMFORMATDETAILS. The driver fills in WAVEFORMATEX
		// for the format associated with the specified index value.
		// It also fills in the szFormat, fdwSupport, and cbStruct
		// members of ACMFORMATDETAILS.

		switch (fmtd->dwFormatTag) {
		
		case WAVE_FORMAT_PCM:
		{
			WAVEFORMATEX *wfex = (WAVEFORMATEX *)&outmem;

			wfex->wFormatTag = WAVE_FORMAT_PCM;

			// Are we allowed to return multichannel PCM?
			if (md->dwFlags & AACACM_MULTICHANNEL)
			{
				if (fmtd->dwFormatIndex >= AACACM_NFREQS * AACACM_NCHANS) goto Abort;
			}
			else	// 2 channels max
			{
				if (fmtd->dwFormatIndex >= AACACM_NFREQS * 2) goto Abort;
			}

			idiv = fmtd->dwFormatIndex / AACACM_NFREQS;
			imod = fmtd->dwFormatIndex % AACACM_NFREQS;

			wfex->nChannels       = chan_map[idiv];
			wfex->nSamplesPerSec  = freq_map[(AACACM_NFREQS - 1) - imod];
			wfex->nBlockAlign     = 2 * wfex->nChannels;
			wfex->nAvgBytesPerSec = wfex->nBlockAlign * wfex->nSamplesPerSec;
			wfex->wBitsPerSample  = 16;
			wfex->cbSize          = 0;

			break;
		}

		case WAVE_FORMAT_RAW_AAC1:
		case WAVE_FORMAT_FAAC51:
		{
			WAVEFORMATEX *wfex = (WAVEFORMATEX *)&outmem;

			wfex->wFormatTag = (WORD)fmtd->dwFormatTag;

			if (fmtd->dwFormatIndex >= AACACM_NFREQS * AACACM_NCHANS) goto Abort;

			idiv = fmtd->dwFormatIndex / AACACM_NFREQS;
			imod = fmtd->dwFormatIndex % AACACM_NFREQS;

			wfex->nChannels       = chan_map[idiv];
			wfex->nSamplesPerSec  = freq_map[(AACACM_NFREQS - 1) - imod];

			// Guesswork...
			wfex->nAvgBytesPerSec = 8000;    // 64 kbps
			wfex->nBlockAlign     = 16;      // smallest possible

			wfex->wBitsPerSample  = 0;
			wfex->cbSize          = 2;

			*((WORD *)((char *)wfex + sizeof(WAVEFORMATEX))) =
				make_aac_config(wfex->nSamplesPerSec, wfex->nChannels);

			break;
		}

#ifdef AACACM_EXTENSIBLE
		case WAVE_FORMAT_EXTENSIBLE:
		{
			WAVEFORMATPCMEX *pcmex = (WAVEFORMATPCMEX *)&outmem;

			pcmex->wFormatTag = WAVE_FORMAT_EXTENSIBLE;

			if (fmtd->dwFormatIndex >= AACACM_NFREQS * AACACM_NCHANS) goto Abort;

			idiv = fmtd->dwFormatIndex / AACACM_NFREQS;
			imod = fmtd->dwFormatIndex % AACACM_NFREQS;

			pcmex->Format.nChannels       = chan_map[idiv];
			pcmex->Format.nSamplesPerSec  = freq_map[(AACACM_NFREQS - 1) - imod];
			pcmex->Format.nBlockAlign     = 2 * pcmex->Format.nChannels;
			pcmex->Format.nAvgBytesPerSec = pcmex->Format.nBlockAlign * pcmex->Format.nSamplesPerSec;
			pcmex->Format.wBitsPerSample  = 16;
			pcmex->Format.cbSize          = sizeof(WAVEFORMATPCMEX) - sizeof(WAVEFORMATEX);

			pcmex->Samples.wValidBitsPerSample = pcmex->Format.wBitsPerSample;
			pcmex->dwChannelMask = channel_masks[pcmex->Format.nChannels - 1];
			pcmex->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

			break;
		}
#endif

		default:
			// Unsupported dwFormatTag
			goto Abort;
		}

		idiv = fmtd->cbwfx;		
		if (idiv != 0)
		{
			if (idiv > sizeof(outmem))
			{
				idiv = sizeof(outmem);
			}
			memcpy(fmtd->pwfx, &outmem, idiv);
		}
		break;

	case ACM_FORMATDETAILSF_FORMAT:

#ifdef AACACM_LOGFILE
		if (md->fLogFile != NULL)
		{
			fprintf(md->fLogFile, " ACM_FORMATDETAILSF_FORMAT\n");
			fprintf(md->fLogFile, " dwFormatTag = 0x%08X\n", fmtd->dwFormatTag);
			fflush(md->fLogFile);
		}
#endif
		if (!IsSupportedTag(fmtd->dwFormatTag, md)) goto Abort;

		// If the ACM_FORMATDETAILSF_FORMAT flag is set, the client
		// has filled in WAVEFORMATEX. The driver validates the
		// structure contents and, if the contents are valid, fills
		// in the szFormat, fdwSupport, and cbStruct members of
		// ACMFORMATDETAILS.

		switch (fmtd->dwFormatTag) {
		
		case WAVE_FORMAT_PCM:
			if (!IsValidPCM(fmtd->pwfx)) goto Abort;

			// Are we allowed to return multichannel PCM?
			if ((md->dwFlags & AACACM_MULTICHANNEL) == 0)
			{
				if (fmtd->pwfx->nChannels > 2) goto Abort;
			}
			break;

		case WAVE_FORMAT_RAW_AAC1:
		case WAVE_FORMAT_FAAC51:
			if (!IsValidAAC(fmtd->pwfx, md)) goto Abort;
			break;

#ifdef AACACM_EXTENSIBLE
		case WAVE_FORMAT_EXTENSIBLE:
			if (!IsValidPCMEX((WAVEFORMATPCMEX *)fmtd->pwfx)) goto Abort;
			break;
#endif

		default:
			// Unsupported dwFormatTag
			goto Abort;
		}
		break;

	default:
		// Unsupported query
#ifdef AACACM_LOGFILE
		if (md->fLogFile != NULL)
		{
			fprintf(md->fLogFile, " Unsupported query: 0x%08X\n", (DWORD)lParam2);
			fflush(md->fLogFile);
		}
#endif
		goto Abort;
	}

	if (fmtd->cbStruct > sizeof(ACMFORMATDETAILSW))
	{
		fmtd->cbStruct = sizeof(ACMFORMATDETAILSW);
	}

	return MMSYSERR_NOERROR;

Abort:
	return MMSYSERR_NOTSUPPORTED;
}


#define IN_BUFSIZE  (FAAD_MIN_STREAMSIZE * 8)


static inline LRESULT stream_convert_aac(DWORD_PTR dwDriverId, LPARAM lParam1, LPARAM lParam2)
{
	const MyData *md = (MyData *)dwDriverId;
	const ACMDRVSTREAMINSTANCE *si;

	NeAACDecFrameInfo hInfo;

	ACMDRVSTREAMHEADER  *sh;
	MyStreamData        *msd;
	const unsigned char *src;
	unsigned char       *dst;

	void *out;
	unsigned long outcount;
	unsigned long framesize;

	unsigned long srcLen;
	unsigned long dstLen;
	
	sh = (ACMDRVSTREAMHEADER *)lParam2;
	sh->cbSrcLengthUsed = 0;
	sh->cbDstLengthUsed = 0;

	src = (unsigned char *)sh->pbSrc;
	dst = (unsigned char *)sh->pbDst;
	srcLen = sh->cbSrcLength;
	dstLen = sh->cbDstLength;

	si = (ACMDRVSTREAMINSTANCE *)lParam1;
	msd = (MyStreamData *)si->dwDriver;

#ifdef AACACM_LOGFILE
	if (md->fLogFile != NULL)
	{
		fprintf(md->fLogFile, "ACMDM_STREAM_CONVERT:\n");
		fprintf(md->fLogFile, " pbSrc = 0x%p, cbSrcLength = %ld\n", src, srcLen);
		fprintf(md->fLogFile, " pbDst = 0x%p, cbDstLength = %ld\n", dst, dstLen);
		fflush(md->fLogFile);
	}
#endif

	if (sh->fdwConvert & ACM_STREAMCONVERTF_START)
	{
		msd->inptr    = msd->inbuf;
		msd->inbytes  = 0;
		LocalFree(msd->outbuf);
		msd->outbuf   = NULL;
		msd->outbytes = 0;
		msd->flags    = AACACM_FIRST_TIME;
	}
	else if (msd->outbytes != 0)
	{
		// Data left over from the last call

		unsigned long tc = msd->outbytes;

		if (tc > dstLen) tc = dstLen;

		memcpy(dst, msd->outbuf, tc);

		msd->outbytes       -= tc;
		dst                 += tc;
		dstLen              -= tc;
		sh->cbDstLengthUsed += tc;

		if (msd->outbytes == 0)
		{
			// Finished uploading this data
			LocalFree(msd->outbuf);
			msd->outbuf = NULL;
		}
		else
		{
			// Not finished; client buffer is full
			memmove(msd->outbuf, msd->outbuf + tc, msd->outbytes);
			goto Done;
		}
	}

	while (srcLen != 0)
	{
		// Do we have enough source to invoke the decoder?

		if (msd->inbytes < 7)
		{
			unsigned long tc = srcLen;

			if (tc > (IN_BUFSIZE - msd->inbytes))
			{
				tc = (IN_BUFSIZE - msd->inbytes);
			}

			memcpy(msd->inptr, src, tc);

			msd->inptr          += tc;
			msd->inbytes        += tc;
			src                 += tc;
			srcLen              -= tc;
			sh->cbSrcLengthUsed += tc;

			// If buffer is still insufficient, we need more source
			if (msd->inbytes < 7) break;
		}

		// There are at least 7 bytes in the buffer.
		// Check for ADTS header now.

		if (msd->inbuf[0] == 0xFF && (msd->inbuf[1] & 0xF6) == 0xF0)
		{
			msd->flags |= AACACM_HAS_ADTS;

			// We have a 7-byte ADTS header in the buffer.
			// From this we can determine the frame size.

			// 00: 11111111   syncword
			// 01: 1111....   syncword (continued)
			//     ....x...   id (1 = MPEG-2, 0 = MPEG-4)
			//     .....xx.   layer
			//     .......x   protection absent
			// 02: xx......   profile
			//     ..xxxx..   sf_index
			//     ......x.   private bit
			//     .......x   channel configuration
			// 03: xx......   channel configuration (continued)
			//     ..x.....   original
			//     ...x....   home
			//     ....x...   copyright identification bit
			//     .....x..   copyright identification start
			//     ......xx   frame length
			// 04: xxxxxxxx   frame length (continued)
			// 05: xxx.....   frame length (continued)
			//     ...xxxxx   buffer fullness
			// 06: xxxxxx..   buffer fullness (continued)
			//     ......xx   raw data blocks in frame

			framesize = ((msd->inbuf[3] & 3) << 11)
					  + (msd->inbuf[4] << 3)
					  + (msd->inbuf[5] >> 5)
					  + 7;
		}
		else
		{
			msd->flags &= ~AACACM_HAS_ADTS;

			// No ADTS and no idea of the framesize. Guess...

			framesize = si->pwfxSrc->nBlockAlign;
			if (framesize < 7 || framesize > IN_BUFSIZE)
			{
				framesize = IN_BUFSIZE;
			}
		}

#ifdef AACACM_LOGFILE
		if (md->fLogFile != NULL)
		{
			fprintf(md->fLogFile, " framesize = %lu\n", framesize);
			fflush(md->fLogFile);
		}
#endif

		// Read framesize bytes into the input buffer.

		if (msd->inbytes < framesize)
		{
			unsigned long tc = srcLen;

			if (tc > (IN_BUFSIZE - msd->inbytes))
			{
				tc = (IN_BUFSIZE - msd->inbytes);
			}

			memcpy(msd->inptr, src, tc);

			msd->inptr          += tc;
			msd->inbytes        += tc;
			src                 += tc;
			srcLen              -= tc;
			sh->cbSrcLengthUsed += tc;

			// If buffer is still insufficient, we need more source
			if (msd->inbytes < framesize) break;
		}

		// Now we have enough data to invoke the decoder.

		if (msd->flags & AACACM_FIRST_TIME)
		{
			// First time initialization.
			unsigned long srate;
			unsigned char chans;
			int result;

			if (msd->flags & AACACM_HAS_ADTS)
			{
				// Initialize from ADTS header
				result = NeAACDecInit(
					msd->aacHandle,
					msd->inbuf,
					msd->inbytes,
					&srate,
					&chans
				);
			}
			else
			{
				// Initialize from AudioSpecificConfig
				result = NeAACDecInit2(
					msd->aacHandle,
					(unsigned char *)si->pwfxSrc + sizeof(WAVEFORMATEX),
					si->pwfxSrc->cbSize,
					&srate,
					&chans
				);
			}

			msd->flags -= AACACM_FIRST_TIME;
			if (result < 0)
			{
#ifdef AACACM_LOGFILE
				if (md->fLogFile != NULL)
				{
					fprintf(md->fLogFile, " stream_convert_aac: NeAACDecInit%s failed (%i)\n",
						(msd->flags & AACACM_HAS_ADTS)? "": "2",
						result
					);
					fflush(md->fLogFile);
				}
#endif
				goto Fatality;
			}
		}

		memset(&hInfo, 0, sizeof(hInfo));
		out = NeAACDecDecode(msd->aacHandle, &hInfo, msd->inbuf, msd->inbytes);

		// Ignore errors as long as the decoder is consuming.
		if (   hInfo.bytesconsumed == 0
		    || hInfo.bytesconsumed > msd->inbytes)
		{
			// Fatal error
#ifdef AACACM_LOGFILE
			if (md->fLogFile != NULL)
			{
				fprintf(md->fLogFile, " stream_convert_aac fatality...\n");
				fprintf(md->fLogFile, "    NeAACDecDecode: 0x%p\n", out);
				fprintf(md->fLogFile, "    error:          %lu\n", (unsigned long)hInfo.error);
				fprintf(md->fLogFile, "    bytesconsumed:  %lu\n", hInfo.bytesconsumed);
				fprintf(md->fLogFile, "    samples:        %lu\n", hInfo.samples);
				fflush(md->fLogFile);
			}
#endif
Fatality:
			msd->inptr    = msd->inbuf;
			msd->inbytes  = 0;
			LocalFree(msd->outbuf);
			msd->outbuf   = NULL;
			msd->outbytes = 0;
			msd->flags    = AACACM_FIRST_TIME;

			sh->cbSrcLengthUsed = 0;
			sh->cbDstLengthUsed = 0;

			goto Abort;
		}
		
		// Discard the consumed data
		msd->inbytes -= hInfo.bytesconsumed;
		if (msd->inbytes != 0)
		{
			memmove(msd->inbuf, msd->inbuf + hInfo.bytesconsumed, msd->inbytes);
		}
		msd->inptr -= hInfo.bytesconsumed;

		// Upload samples to the client
		if (out != NULL && hInfo.samples > 0)
		{
			outcount = hInfo.samples * sizeof(short);

			// Due to implicit PS faad2 seems to always return stereo.
			// If the client has requested mono, downmix it now.

			if (si->pwfxDst->nChannels == 1 && hInfo.channels == 2)
			{
				unsigned long i = hInfo.samples;
				short *src      = (short *)out;
				short *dst      = src;

				while (i >= 2)
				{
					*dst++ = (short)(((int)src[0] + (int)src[1]) / 2);
					src += 2;
					i   -= 2;
				}

				outcount /= 2;
			}

			if (outcount > dstLen)
			{
				// Too much! Upload what we can and hold the rest.
				unsigned long tc = outcount - dstLen;

				memcpy(dst, out, dstLen);
				sh->cbDstLengthUsed += dstLen;

				msd->outbuf = (unsigned char *)LocalAlloc(LPTR, tc);
				if (msd->outbuf == NULL)
				{
#ifdef AACACM_LOGFILE
					if (md->fLogFile != NULL)
					{
						DWORD dwLastError = GetLastError();
						fprintf(md->fLogFile, " stream_convert_aac: LocalAlloc failed (%lu)\n", dwLastError);
						fflush(md->fLogFile);
					}
#endif
					goto Fatality;
				}

				memcpy(msd->outbuf, (char *)out + dstLen, tc);
				msd->outbytes = tc;
				break;
			}

			memcpy(dst, out, outcount);

			dst += outcount;
			sh->cbDstLengthUsed += outcount;
			dstLen -= outcount;
		}

		if (dstLen == 0) break;
	}

Done:
#ifdef AACACM_LOGFILE
	if (md->fLogFile != NULL)
	{
		fprintf(md->fLogFile, " cbSrcLengthUsed = %lu\n", sh->cbSrcLengthUsed);
		fprintf(md->fLogFile, " cbDstLengthUsed = %lu\n", sh->cbDstLengthUsed);
		fflush(md->fLogFile);
	}
#endif
	return MMSYSERR_NOERROR;

Abort:
	return MMSYSERR_ERROR;
}


static LRESULT stream_open(DWORD_PTR dwDriverId, LPARAM lParam1)
{
	const MyData *md = (MyData *)dwDriverId;
	ACMDRVSTREAMINSTANCE *si = (ACMDRVSTREAMINSTANCE *)lParam1;

	si->dwDriver = NULL;

	if (!IsValidAAC(si->pwfxSrc, md)) goto Abort;

	if (!IsValidPCM(si->pwfxDst))
	{
#ifdef AACACM_EXTENSIBLE
		if (!IsValidPCMEX((WAVEFORMATPCMEX *)si->pwfxDst))
#endif
		goto Abort;
	}

	// Are we allowed to return multichannel PCM?
	if (!(md->dwFlags & AACACM_MULTICHANNEL))
	{
		if (si->pwfxDst->nChannels > 2)
		{
			return MMSYSERR_NOTSUPPORTED;
		}
	}

	// We can't convert the sampling rate
	if (si->pwfxDst->nSamplesPerSec != si->pwfxSrc->nSamplesPerSec)
	{
		goto Abort;
	}

	if ((si->fdwOpen & ACM_STREAMOPENF_QUERY) == 0)
	{
		NeAACDecConfiguration conf;

		// Allocate all memory as a big block
		MyStreamData *msd = (MyStreamData *)VirtualAlloc(
			NULL,
			sizeof(MyStreamData) +
			IN_BUFSIZE + 16,
			MEM_COMMIT, PAGE_READWRITE
		);

		if (msd == NULL) goto MemError;

		msd->aacHandle = NeAACDecOpen();
		if (msd->aacHandle == NULL)
		{
			VirtualFree((LPVOID)msd, 0, MEM_RELEASE);
			goto MemError;
		}

		conf.defObjectType           = LC;
		conf.defSampleRate           = si->pwfxDst->nSamplesPerSec;
		conf.outputFormat            = FAAD_FMT_16BIT;
		conf.downMatrix              = (si->pwfxSrc->nChannels > 2 && si->pwfxDst->nChannels <= 2)? 1: 0;
		conf.useOldADTSFormat        = 0;
		conf.dontUpSampleImplicitSBR = 0;

		NeAACDecSetConfiguration(msd->aacHandle, &conf);

		// Assign memory blocks
		msd->inbuf = (unsigned char *)msd + sizeof(MyStreamData);

		// Align buffer to 16-byte boundary
		msd->inbuf = (unsigned char *)
			(((DWORD_PTR)msd->inbuf + 15) & (LONG_PTR)-16);

		msd->inptr    = msd->inbuf;
		msd->inbytes  = 0;
		msd->outbuf   = NULL;
		msd->outbytes = 0;
		msd->flags    = AACACM_FIRST_TIME;

		si->dwDriver = (DWORD_PTR)msd;
	}

#ifdef AACACM_LOGFILE
	if (md->fLogFile != NULL)
	{
		if (si->fdwOpen & ACM_STREAMOPENF_QUERY)
		{
			fprintf(md->fLogFile, "ACMDM_STREAM_OPEN: query succeeded\n");
		}
		else
		{
			fprintf(md->fLogFile, "ACMDM_STREAM_OPEN: succeeded\n");
		}

		fprintf(md->fLogFile, " source:\n");
		log_format(dwDriverId, si->pwfxSrc);

		fprintf(md->fLogFile, " destination:\n");
		log_format(dwDriverId, si->pwfxDst);

		fflush(md->fLogFile);
	}
#endif

	return MMSYSERR_NOERROR;

Abort:
#ifdef AACACM_LOGFILE
	if (md->fLogFile != NULL)
	{
		fprintf(md->fLogFile, "ACMDM_STREAM_OPEN: not possible\n");

		fprintf(md->fLogFile, " source:\n");
		log_format(dwDriverId, si->pwfxSrc);

		fprintf(md->fLogFile, " destination:\n");
		log_format(dwDriverId, si->pwfxDst);

		fflush(md->fLogFile);
	}
#endif

	return ACMERR_NOTPOSSIBLE;

MemError:
#ifdef AACACM_LOGFILE
	if (md->fLogFile != NULL)
	{
		fprintf(md->fLogFile, "ACMDM_STREAM_OPEN: out of memory\n");
		fflush(md->fLogFile);
	}
#endif

	return MMSYSERR_NOMEM;
}


static LRESULT stream_close(DWORD_PTR dwDriverId, LPARAM lParam1)
{
	const MyData *md = (MyData *)dwDriverId;
	ACMDRVSTREAMINSTANCE *si = (ACMDRVSTREAMINSTANCE *)lParam1;

	if (md->fccType != (FOURCC)0 && si != NULL)
	{
		if (si->dwDriver != NULL)
		{
			MyStreamData *msd = (MyStreamData *)si->dwDriver;

			LocalFree(msd->outbuf);
			NeAACDecClose(msd->aacHandle);

			VirtualFree((LPVOID)si->dwDriver, 0, MEM_RELEASE);
			si->dwDriver = NULL;
		}
	}

#ifdef AACACM_LOGFILE
	if (md->fLogFile != NULL)
	{
		fprintf(md->fLogFile, "ACMDM_STREAM_CLOSE\n");
		fflush(md->fLogFile);
	}
#endif

	return MMSYSERR_NOERROR;
}


static LRESULT stream_size(DWORD_PTR dwDriverId, LPARAM lParam1, LPARAM lParam2)
{
	const MyData *md = (MyData *)dwDriverId;
	const ACMDRVSTREAMINSTANCE *si = (ACMDRVSTREAMINSTANCE *)lParam1;

	ACMDRVSTREAMSIZE *ss = (ACMDRVSTREAMSIZE *)lParam2;

	unsigned long i;

	// It seems that many clients are going to require us to
	// be able to use buffers of any size, so we mustn't fail
	// this message if at all possible...

	// For maximum compatibility, we will use ALL of the source
	// data during stream_convert, but we will not necessarily
	// fill all of the destination buffer.

	switch (ss->fdwSize & ACM_STREAMSIZEF_QUERYMASK) {

	case ACM_STREAMSIZEF_SOURCE:
		// Given a specified source buffer size, how large
		// does a destination buffer need to be in order
		// to hold all of the converted data?

		// The smallest possible AAC frame is 16 bytes (?)
		// Each AAC frame can produce up to 1024 samples
		// per channel.

		i = (ss->cbSrcLength + 15) / 16;
		if (i == 0) ++i;
		ss->cbDstLength = i * 1024 * si->pwfxSrc->nChannels * sizeof(short);
		break;

	case ACM_STREAMSIZEF_DESTINATION:
		// Given a specified destination buffer size, what
		// is the largest amount of source data that can be
		// specified without overflowing the destination buffer?

		i = 1024 * si->pwfxSrc->nChannels * sizeof(short);
		i = (ss->cbDstLength + (i - 1)) / i;
		if (i == 0) ++i;
		ss->cbSrcLength = i * 16;
		break;

	default:

#ifdef AACACM_LOGFILE
		if (md->fLogFile != NULL)
		{
			fprintf(md->fLogFile, "ACMDM_STREAM_SIZE: unsupported query\n");
			fprintf(md->fLogFile, " fdwSize = 0x%08X\n", ss->fdwSize);
			fflush(md->fLogFile);
		}
#endif
		return MMSYSERR_NOTSUPPORTED;
	}

#ifdef AACACM_LOGFILE
	if (md->fLogFile != NULL)
	{
		switch (ss->fdwSize & ACM_STREAMSIZEF_QUERYMASK) {

			case ACM_STREAMSIZEF_SOURCE:
				fprintf(md->fLogFile, "ACMDM_STREAM_SIZE: destination size query OK\n");
				break;

			case ACM_STREAMSIZEF_DESTINATION:
				fprintf(md->fLogFile, "ACMDM_STREAM_SIZE: source size query OK\n");
				break;
		}

		fprintf(md->fLogFile, " cbSrcLength = %lu\n", ss->cbSrcLength);
		fprintf(md->fLogFile, " cbDstLength = %lu\n", ss->cbDstLength);

		fflush(md->fLogFile);
	}
#endif

	return MMSYSERR_NOERROR;
}


INT_PTR CALLBACK MyDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {

	case WM_INITDIALOG:
		{
			const MyData *md = (MyData *)lParam;
			SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);

			CheckDlgButton(hwndDlg, IDC_MULTICHANNEL,
				(md->dwFlags & AACACM_MULTICHANNEL)?
				BST_CHECKED: BST_UNCHECKED);

		}
		return TRUE;

	case WM_COMMAND:

		switch (LOWORD(wParam)) {

			case IDOK:
			{
				MyData *md = (MyData *)GetWindowLongPtr(hwndDlg, DWLP_USER);

				if (IsDlgButtonChecked(hwndDlg, IDC_MULTICHANNEL) == BST_CHECKED) {
					md->dwFlags |= AACACM_MULTICHANNEL;
				} else {
					md->dwFlags &= (~AACACM_MULTICHANNEL);
				}

				WriteReg(md);

				EndDialog(hwndDlg, DRVCNF_OK);
				return TRUE;
			}

			case IDCANCEL:
				EndDialog(hwndDlg, DRVCNF_CANCEL);
				return TRUE;
		}
		break;

	}

	return FALSE;
}


static LRESULT drv_configure(DWORD_PTR dwDriverId, LPARAM lParam1, LPARAM lParam2)
{
	// Show configuration dialog
	const MyData *md = (MyData *)dwDriverId;

#ifdef AACACM_LOGFILE
	if (md->fLogFile != NULL)
	{
		fprintf(md->fLogFile, "DRV_CONFIGURE\n");
		fflush(md->fLogFile);
	}
#endif

	if (lParam1 == -1) return (LRESULT)TRUE;

	return (LRESULT)DialogBoxParam(
		md->hmod,
		MAKEINTRESOURCE(IDD_CONFIG),
		(HWND)lParam1,
		MyDialogProc,
		(LPARAM)dwDriverId
	);
}


LRESULT CALLBACK DriverProc(DWORD_PTR dwDriverId, HDRVR hdrvr, UINT msg, LPARAM lParam1, LPARAM lParam2)
{
	switch (msg) {

	case DRV_LOAD:
		return TRUE;

	case DRV_OPEN:
		return drv_open(hdrvr, lParam2);

	case DRV_CLOSE:
		return drv_close(dwDriverId);

	case DRV_FREE:
		return TRUE;

	case DRV_CONFIGURE:
		return drv_configure(dwDriverId, lParam1, lParam2);

	case DRV_QUERYCONFIGURE:
		return TRUE;

	case DRV_INSTALL:
		return DRVCNF_RESTART;

	case DRV_REMOVE:
		return DRVCNF_RESTART;

	case ACMDM_DRIVER_DETAILS:
		return driver_details(dwDriverId, lParam1);

	case ACMDM_DRIVER_ABOUT:
		return MMSYSERR_NOTSUPPORTED;

	case ACMDM_FORMATTAG_DETAILS:
		return formattag_details(dwDriverId, lParam1, lParam2);

	case ACMDM_FORMAT_DETAILS:
		return format_details(dwDriverId, lParam1, lParam2);

	case ACMDM_FORMAT_SUGGEST:
		return format_suggest(dwDriverId, lParam1);

	case ACMDM_STREAM_OPEN:
		return stream_open(dwDriverId, lParam1);

	case ACMDM_STREAM_CLOSE:
		return stream_close(dwDriverId, lParam1);

	case ACMDM_STREAM_SIZE:
		return stream_size(dwDriverId, lParam1, lParam2);

	case ACMDM_STREAM_CONVERT:
		return stream_convert_aac(dwDriverId, lParam1, lParam2);

	default:
		if (msg < DRV_USER)
			return DefDriverProc(dwDriverId, hdrvr, msg, lParam1, lParam2);
	}

	return MMSYSERR_NOTSUPPORTED;
}
