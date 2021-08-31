/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2021 PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"
#include "GS.h"
#include "GSVector.h"

CONSTINIT const GSVector4i GSVector4i::m_xff[17] =
{
	cxpr(0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x000000ff, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0000ffff, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x00ffffff, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0x000000ff, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0x0000ffff, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0x00ffffff, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0x000000ff, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0x0000ffff, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0x00ffffff, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0x000000ff),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0x0000ffff),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0x00ffffff),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff),
};

CONSTINIT const GSVector4i GSVector4i::m_x0f[17] =
{
	cxpr(0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0000000f, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x00000f0f, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x000f0f0f, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0000000f, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x00000f0f, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x000f0f0f, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0000000f, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x00000f0f, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x000f0f0f, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0000000f),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x00000f0f),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x000f0f0f),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f),
};

CONSTINIT const GSVector4 GSVector4::m_ps0123 = cxpr(0.0f, 1.0f, 2.0f, 3.0f);
CONSTINIT const GSVector4 GSVector4::m_ps4567 = cxpr(4.0f, 5.0f, 6.0f, 7.0f);
CONSTINIT const GSVector4 GSVector4::m_half = cxpr(0.5f);
CONSTINIT const GSVector4 GSVector4::m_one = cxpr(1.0f);
CONSTINIT const GSVector4 GSVector4::m_two = cxpr(2.0f);
CONSTINIT const GSVector4 GSVector4::m_four = cxpr(4.0f);
CONSTINIT const GSVector4 GSVector4::m_x4b000000 = cxpr(0x4b000000);
CONSTINIT const GSVector4 GSVector4::m_x4f800000 = cxpr(0x4f800000);
CONSTINIT const GSVector4 GSVector4::m_max = cxpr(FLT_MAX);
CONSTINIT const GSVector4 GSVector4::m_min = cxpr(FLT_MIN);

#if _M_SSE >= 0x500

CONSTINIT const GSVector8 GSVector8::m_half = cxpr(0.5f);
CONSTINIT const GSVector8 GSVector8::m_one = cxpr(1.0f);
CONSTINIT const GSVector8 GSVector8::m_x7fffffff = cxpr(0x7fffffff);
CONSTINIT const GSVector8 GSVector8::m_x80000000 = cxpr(0x80000000);
CONSTINIT const GSVector8 GSVector8::m_x4b000000 = cxpr(0x4b000000);
CONSTINIT const GSVector8 GSVector8::m_x4f800000 = cxpr(0x4f800000);
CONSTINIT const GSVector8 GSVector8::m_max = cxpr(FLT_MAX);
CONSTINIT const GSVector8 GSVector8::m_min = cxpr(FLT_MAX);

#endif

#if _M_SSE >= 0x501
CONSTINIT const GSVector8i GSVector8i::m_xff[33] =
{
	cxpr(0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x000000ff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0000ffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x00ffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0x000000ff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0x0000ffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0x00ffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0x000000ff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0x0000ffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0x00ffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0x000000ff, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0x0000ffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0x00ffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000000ff, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0000ffff, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00ffffff, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000000ff, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0000ffff, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00ffffff, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000000ff, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0000ffff, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00ffffff, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000000ff),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0000ffff),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00ffffff),
	cxpr(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff),
};

CONSTINIT const GSVector8i GSVector8i::m_x0f[33] =
{
	cxpr(0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0000000f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x00000f0f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x000f0f0f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0000000f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x00000f0f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x000f0f0f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0000000f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x00000f0f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x000f0f0f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0000000f, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x00000f0f, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x000f0f0f, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0000000f, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x00000f0f, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x000f0f0f, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x00000000, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0000000f, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x00000f0f, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x000f0f0f, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x00000000, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0000000f, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x00000f0f, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x000f0f0f, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x00000000),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0000000f),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x00000f0f),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x000f0f0f),
	cxpr(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f),
};
#endif

GSVector4i GSVector4i::fit(int arx, int ary) const
{
	GSVector4i r = *this;

	if (arx > 0 && ary > 0)
	{
		int w = width();
		int h = height();

		if (w * ary > h * arx)
		{
			w = h * arx / ary;
			r.left = (r.left + r.right - w) >> 1;
			if (r.left & 1)
				r.left++;
			r.right = r.left + w;
		}
		else
		{
			h = w * ary / arx;
			r.top = (r.top + r.bottom - h) >> 1;
			if (r.top & 1)
				r.top++;
			r.bottom = r.top + h;
		}

		r = r.rintersect(*this);
	}
	else
	{
		r = *this;
	}

	return r;
}

static const int s_ar[][2] = {{0, 0}, {4, 3}, {16, 9}};

GSVector4i GSVector4i::fit(int preset) const
{
	GSVector4i r;

	if (preset > 0 && preset < (int)std::size(s_ar))
	{
		r = fit(s_ar[preset][0], s_ar[preset][1]);
	}
	else
	{
		r = *this;
	}

	return r;
}
