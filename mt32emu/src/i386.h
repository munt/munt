/* Copyright (C) 2003-2009 Dean Beeler, Jerome Fisher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MT32EMU_I386_H
#define MT32EMU_I386_H

namespace MT32Emu {
#ifdef MT32EMU_HAVE_X86

// Function that detects the availablity of SSE SIMD instructions
bool DetectSIMD();
// Function that detects the availablity of 3DNow instructions
bool Detect3DNow();

float iir_filter_sse(float input,float *hist1_ptr, float *coef_ptr);
float iir_filter_3dnow(float input,float *hist1_ptr, float *coef_ptr);
float iir_filter_normal(float input,float *hist1_ptr, float *coef_ptr);

#if MT32EMU_USE_MMX > 0
int i386_partialProductOutput(int len, Bit16s leftvol, Bit16s rightvol, Bit16s *partialBuf, Bit16s *mixedBuf);
int i386_mixBuffers(Bit16s * buf1, Bit16s *buf2, int len);
int i386_mixBuffersRingMix(Bit16s * buf1, Bit16s *buf2, int len);
int i386_mixBuffersRing(Bit16s * buf1, Bit16s *buf2, int len);
int i386_produceOutput1(Bit16s *useBuf, Bit16s *stream, Bit32u len, Bit16s volume);
#endif

#endif

}

#endif
