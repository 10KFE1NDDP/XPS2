/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#pragma once

//------------------------------------------------------------------
// Micro VU - Clamp Functions
//------------------------------------------------------------------

alignas(16) const u32 sse4_minvals[2][4] = {
	{0xff7fffff, 0xffffffff, 0xffffffff, 0xffffffff}, //1000
	{0xff7fffff, 0xff7fffff, 0xff7fffff, 0xff7fffff}, //1111
};
alignas(16) const u32 sse4_maxvals[2][4] = {
	{0x7f7fffff, 0x7fffffff, 0x7fffffff, 0x7fffffff}, //1000
	{0x7f7fffff, 0x7f7fffff, 0x7f7fffff, 0x7f7fffff}, //1111
};

// Used for Result Clamping
// Note: This function will not preserve NaN values' sign.
// The theory behind this is that when we compute a result, and we've
// gotten a NaN value, then something went wrong; and the NaN's sign
// is not to be trusted. Games like positive values better usually,
// and its faster... so just always make NaNs into positive infinity.
void mVUclamp1(microVU& mVU, const xmm& reg, const xmm& regT1, int xyzw, bool bClampE = 0)
{
	if (((!clampE && CHECK_VU_OVERFLOW(mVU.index)) || (clampE && bClampE)) && mVU.regAlloc->isClampNeeded(reg.Id, 1))
	{
		switch (xyzw)
		{
			case 1: case 2: case 4: case 8:
				xMIN.SS(reg, ptr32[mVUglob.maxvals]);
				xMAX.SS(reg, ptr32[mVUglob.minvals]);
				break;
			default:
				xMIN.PS(reg, ptr32[mVUglob.maxvals]);
				xMAX.PS(reg, ptr32[mVUglob.minvals]);
				break;
		}
		mVU.regAlloc->setRegClamp(reg.Id, 1);
	}
}

// Used for Operand Clamping
// Note 1: If 'preserve sign' mode is on, it will preserve the sign of NaN values.
// Note 2: Using regalloc here seems to contaminate some regs in certain games.
// Must be some specific case I've overlooked (or I used regalloc improperly on an opcode)
// so we just use a temporary mem location for our backup for now... (non-sse4 version only)
void mVUclamp2(microVU& mVU, const xmm& reg, const xmm& regT1in, int xyzw, bool bClampE = 0)
{
	if (((!clampE && CHECK_VU_SIGN_OVERFLOW(mVU.index)) || (clampE && bClampE && CHECK_VU_SIGN_OVERFLOW(mVU.index))) && mVU.regAlloc->isClampNeeded(reg.Id, 2))
	{
		const int i = (xyzw == 1 || xyzw == 2 || xyzw == 4 || xyzw == 8) ? 0 : 1;
		xPMIN.SD(reg, ptr128[&sse4_maxvals[i][0]]);
		xPMIN.UD(reg, ptr128[&sse4_minvals[i][0]]);
		mVU.regAlloc->setRegClamp(reg.Id, 2);
	}
	else
	{
		mVUclamp1(mVU, reg, regT1in, xyzw, bClampE);
	}
}

// Used for operand clamping on every SSE instruction (add/sub/mul/div)
void mVUclamp3(microVU& mVU, const xmm& reg, const xmm& regT1, int xyzw)
{
	if (clampE && mVU.regAlloc->isClampNeeded(reg.Id, 3))
		mVUclamp2(mVU, reg, regT1, xyzw, 1);
}

// Used for result clamping on every SSE instruction (add/sub/mul/div)
// Note: Disabled in "preserve sign" mode because in certain cases it
// makes too much code-gen, and you get jump8-overflows in certain
// emulated opcodes (causing crashes). Since we're clamping the operands
// with mVUclamp3, we should almost never be getting a NaN result,
// but this clamp is just a precaution just-in-case.
void mVUclamp4(microVU& mVU, const xmm& reg, const xmm& regT1, int xyzw)
{
	if (clampE && !CHECK_VU_SIGN_OVERFLOW(mVU.index) && mVU.regAlloc->isClampNeeded(reg.Id, 4))
		mVUclamp1(mVU, reg, regT1, xyzw, 1);
}
