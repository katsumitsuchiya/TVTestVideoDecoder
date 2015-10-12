/*
 *  TVTest DTV Video Decoder
 *  Copyright (C) 2015 DBCTRADO
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once


#include <d3d9.h>
#include <dxva2api.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <evr.h>
#include <vector>
#include "FrameBuffer.h"


class CBaseVideoFilter : public CTransformFilter
{
	friend class CBaseVideoOutputPin;
	friend class CDXVA2Allocator;

public:
	CBaseVideoFilter(PCWSTR pName, LPUNKNOWN lpunk, HRESULT *phr, REFCLSID clsid, long cBuffers = 1);
	virtual ~CBaseVideoFilter();

	virtual HRESULT CheckOutputType(const CMediaType *mtOut);

// CTransformFilter

	int GetPinCount() override;
	CBasePin *GetPin(int n) override;
	HRESULT CheckInputType(const CMediaType *mtIn) override;
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut) override;
	HRESULT DecideBufferSize(IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pProperties) override;
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType) override;
	HRESULT SetMediaType(PIN_DIRECTION dir, const CMediaType *pmt) override;
	HRESULT CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin) override;
	HRESULT Receive(IMediaSample *pIn) override;

protected:
	struct VideoDimensions
	{
		int Width   = 0;
		int Height  = 0;
		int AspectX = 0;
		int AspectY = 0;

		bool operator==(const VideoDimensions &o) const {
			return Width   == o.Width
				&& Height  == o.Height
				&& AspectX == o.AspectX
				&& AspectY == o.AspectY;
		}
		bool operator!=(const VideoDimensions &o) const {
			return !(*this == o);
		}
	};

	struct OutputFormatInfo
	{
		const GUID *subtype;
		WORD Planes;
		WORD BitCount;
		DWORD Compression;
	};

	struct OutputFormatList
	{
		const OutputFormatInfo *pFormats;
		int FormatCount;
	};

	CCritSec m_csReceive;
	long m_cBuffers;

	VideoDimensions m_Dimensions;
	VideoDimensions m_InDimensions;
	VideoDimensions m_OutDimensions;

	IDirect3DDeviceManager9 *m_pD3DDeviceManager;
	IDirectXVideoDecoderService *m_pDecoderService;
	HANDLE m_hDXVADevice;
	bool m_fDXVAConnect;
	bool m_fDXVAOutput;

	HRESULT GetDeliveryBuffer(
		IMediaSample **ppOut, int Width, int Height,
		int AspectX = 0, int AspectY = 0,
		REFERENCE_TIME AvgTimePerFrame = 0, bool fInterlaced = false);
	HRESULT GetDeliveryBuffer(IMediaSample **ppOut);
	void SetupMediaType(CMediaType *pmt);
	HRESULT CopySampleBuffer(BYTE *pOut, const CFrameBuffer *pSrc, bool fInterlaced = false);
	HRESULT ReconnectOutput(
		int Width, int Height, int AspectX = 0, int AspectY = 0,
		bool fSendSample = true, bool fForce = false,
		REFERENCE_TIME AvgTimePerFrame = 0, bool fInterlaced = false, int RealWidth = 0, int RealHeight = 0);
	static bool GetDimensions(const AM_MEDIA_TYPE &mt, VideoDimensions *pDimensions);

	virtual void GetOutputFormatList(OutputFormatList *pFormatList) const = 0;
	virtual HRESULT Transform(IMediaSample *pIn) = 0;
	virtual void GetOutputSize(VideoDimensions *pDimensions, int *pRealWidth, int *pRealHeight) {}
	virtual bool IsVideoInterlaced() { return false; }
	virtual DWORD GetVideoInfoControlFlags() const { return 0; }
	virtual HRESULT OnDXVAConnect(IPin *pPin) { return S_OK; }
};

class CBaseVideoInputAllocator : public CMemAllocator
{
public:
	CBaseVideoInputAllocator(HRESULT *phr);
	void SetMediaType(const CMediaType &mt);

// CBaseAllocator

	STDMETHODIMP GetBuffer(IMediaSample **ppBuffer, REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime, DWORD dwFlags) override;

private:
	CMediaType m_mt;
};

class CBaseVideoInputPin : public CTransformInputPin
{
public:
	CBaseVideoInputPin(PCWSTR pObjectName, CBaseVideoFilter *pFilter, HRESULT *phr, PCWSTR pName);
	~CBaseVideoInputPin();

	STDMETHODIMP GetAllocator(IMemAllocator **ppAllocator);
	STDMETHODIMP ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt);

private:
	CBaseVideoInputAllocator *m_pAllocator;
};

class CBaseVideoOutputPin : public CTransformOutputPin
{
public:
	CBaseVideoOutputPin(PCWSTR pObjectName, CBaseVideoFilter *pFilter, HRESULT *phr, PCWSTR pName);

// CBasePin

	HRESULT CheckMediaType(const CMediaType *mtOut) override;
	HRESULT InitAllocator(IMemAllocator **ppAllocator) override;

private:
	CBaseVideoFilter *m_pFilter;
};

class CDXVA2Allocator : public CBaseAllocator
{
public:
	CDXVA2Allocator(CBaseVideoFilter *pFilter, HRESULT *phr);
	~CDXVA2Allocator();

// CBaseAllocator

	HRESULT Alloc() override;
	void Free() override;

private:
	CBaseVideoFilter *m_pFilter;
	std::vector<IDirect3DSurface9*> m_SurfaceList;
};

class CDXVA2MediaSample
	: public CMediaSample
	, public IMFGetService
{
public:
	CDXVA2MediaSample(CDXVA2Allocator *pAllocator, HRESULT *phr);
	~CDXVA2MediaSample();

	void SetSurface(DWORD SurfaceID, IDirect3DSurface9 *pSurface);

// IUnknown

	STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override;
	STDMETHODIMP_(ULONG) AddRef() override;
	STDMETHODIMP_(ULONG) Release() override;

// IMediaSample

	STDMETHODIMP GetPointer(BYTE **ppBuffer) override;

// IMFGetService

	STDMETHODIMP GetService(REFGUID guidSerivce, REFIID riid, LPVOID *ppv) override;

private:
	IDirect3DSurface9 *m_pSurface;
	DWORD m_SurfaceID;
};
