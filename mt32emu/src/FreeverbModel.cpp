/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
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

#include "mt32emu.h"
#include "FreeverbModel.h"

#include "freeverb/revmodel.h"

using namespace MT32Emu;

FreeverbModel::FreeverbModel(float useScaleTuning, float useFiltVal, float useWet, float useRoom, float useDamp) {
	freeverb = NULL;
	scaleTuning = useScaleTuning;
	filtVal = useFiltVal;
	wet = useWet;
	room = useRoom;
	damp = useDamp;
}

FreeverbModel::~FreeverbModel() {
	delete freeverb;
}

void FreeverbModel::open(unsigned int /*sampleRate*/) {
	// FIXME: scaleTuning must be multiplied by sample rate to 32000Hz ratio
	// IIR filter values depend on sample rate as well
	if (freeverb == NULL) {
		freeverb = new revmodel(scaleTuning);
	}
}

void FreeverbModel::close() {
	delete freeverb;
	freeverb = NULL;
}

void FreeverbModel::process(const float *inLeft, const float *inRight, float *outLeft, float *outRight, unsigned long numSamples) {
	freeverb->process(inLeft, inRight, outLeft, outRight, numSamples);
}

void FreeverbModel::setParameters(Bit8u time, Bit8u level) {
	freeverb->mute();

	freeverb->setfiltval(filtVal);

	// wet signal level
	freeverb->setwet((float)level / 7.0f * wet);

	// wet signal decay speed
	freeverb->setroomsize((0.5f + 0.5f * (float)time / 7.0f) * room);

	// decay speed of high frequencies in the wet signal
	freeverb->setdamp(damp);
}

bool FreeverbModel::isActive() const {
	// FIXME: Not bothering to do this properly since we'll be replacing Freeverb soon...
	return false;
}
