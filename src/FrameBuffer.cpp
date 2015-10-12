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

#include "stdafx.h"
#include <malloc.h>
#include "FrameBuffer.h"
#include "MediaTypes.h"


static inline int RowBytes(int Width) { return (Width + 31) & ~31; }


CFrameBuffer::CFrameBuffer()
	: m_Width(0)
	, m_Height(0)
	, m_PitchY(0)
	, m_PitchC(0)
	, m_pBuffer(nullptr)
	, m_pSurface(nullptr)
	, m_Subtype(GUID_NULL)
	, m_AspectX(0)
	, m_AspectY(0)
	, m_rtStart(0)
	, m_rtStop(0)
	, m_Flags(0)
{
	for (int i = 0; i < _countof(m_Buffer); i++)
		m_Buffer[i] = nullptr;
}

CFrameBuffer::~CFrameBuffer()
{
	Free();
}

bool CFrameBuffer::Allocate(int Width, int Height)
{
	if (Width <= 0 || Height <= 0 || (Width & 1) || (Height & 1))
		return false;
	if (Width == m_Width && Height == m_Height)
		return true;

	Free();

	const int PitchY = RowBytes(Width);
	const int PitchC = RowBytes(Width / 2);
	const int SizeY = PitchY * Height;
	const int SizeC = PitchC * (Height / 2);

	m_pBuffer = (uint8_t*)::_aligned_malloc(SizeY + SizeC * 2, 32);
	if (!m_pBuffer)
		return false;

	m_Width = Width;
	m_Height = Height;
	m_PitchY = PitchY;
	m_PitchC = PitchC;

	uint8_t *p = m_pBuffer;
	m_Buffer[0] = p;
	p += SizeY;
	m_Buffer[1] = p;
	p += SizeC;
	m_Buffer[2] = p;

	m_Subtype = MEDIASUBTYPE_I420;

	return true;
}

void CFrameBuffer::Free()
{
	if (m_pBuffer) {
		::_aligned_free(m_pBuffer);
		m_pBuffer = nullptr;
	}

	for (int i = 0; i < _countof(m_Buffer); i++)
		m_Buffer[i] = nullptr;

	m_Width = 0;
	m_Height = 0;
	m_PitchY = 0;
	m_PitchC = 0;
	m_Subtype = GUID_NULL;
}

bool CFrameBuffer::CopyAttributesFrom(const CFrameBuffer *pBuffer)
{
	if (!pBuffer)
		return false;

	m_AspectX = pBuffer->m_AspectX;
	m_AspectY = pBuffer->m_AspectY;
	m_rtStart = pBuffer->m_rtStart;
	m_rtStop = pBuffer->m_rtStop;
	m_Flags = pBuffer->m_Flags;
	m_Deinterlace = pBuffer->m_Deinterlace;

	return true;
}

bool CFrameBuffer::CopyReferenceTo(CFrameBuffer *pBuffer) const
{
	if (!pBuffer)
		return false;

	pBuffer->Free();

	pBuffer->m_Width = m_Width;
	pBuffer->m_Height = m_Height;
	pBuffer->m_PitchY = m_PitchY;
	pBuffer->m_PitchC = m_PitchC;
	for (int i = 0; i < _countof(m_Buffer); i++)
		pBuffer->m_Buffer[i] = m_Buffer[i];
	pBuffer->m_pSurface = m_pSurface;
	pBuffer->m_Subtype = m_Subtype;

	pBuffer->CopyAttributesFrom(this);

	return true;
}
