/* Copyright (C) 2011, 2012 Jerome Fisher, Sergey V. Mikayev
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

#include "SynthStateMonitor.h"

#include <mt32emu/mt32emu.h>

#include "SynthRoute.h"
#include "ui_SynthWidget.h"

static QColor colorGrey = QColor(100, 100, 100);
static QColor partialStateColor[] = {colorGrey, Qt::red, Qt::yellow, Qt::green};

SynthStateMonitor::SynthStateMonitor(Ui::SynthWidget *ui, SynthRoute *useSynthRoute) {
	this->ui = ui;
	synthRoute = useSynthRoute;

	for (int i = 0; i < 32; i++) {
		partialStateLabel[i] = new QLabel();
		partialStateLabel[i]->setMaximumSize(16, 16);
		ui->partialStateGrid->addWidget(partialStateLabel[i], i >> 2, i & 3);
	}

	for (int i = 0; i < 9; i++) {
		patchNameLabel[i] = new QLabel();
		polyStateLabel[i] = new QLabel();
		ui->polyStateGrid->addWidget(patchNameLabel[i], i, 0);
		ui->polyStateGrid->addWidget(polyStateLabel[i], i, 1);
	}

	qsynth = synthRoute->findChild<QSynth *>();

#ifdef SYNTH_MONITOR_UPDATE_MILLIS
	handleReset();
	connect(qsynth, SIGNAL(partStateReset()), SLOT(handleReset()));
	connect(qsynth->findChild<QReportHandler *>(), SIGNAL(programChanged(int, QString)), SLOT(handleProgramChanged(int, QString)));
	connect(&timer, SIGNAL(timeout()), SLOT(handleUpdate()));
	timer.start(SYNTH_MONITOR_UPDATE_MILLIS);
#endif
}

SynthStateMonitor::~SynthStateMonitor() {
	for (int i = 0; i < 9; i++) {
		delete polyStateLabel[i];
		delete patchNameLabel[i];
	}
	for (int i = 0; i < 32; i++) delete partialStateLabel[i];
}

void SynthStateMonitor::connectSignals(bool enable) {
#ifdef SYNTH_MONITOR_UPDATE_MILLIS
	if (enable) {
		timer.start();
	} else {
		timer.stop();
	}
#else
	QReportHandler *handler = qsynth->findChild<QReportHandler *>();
	if (enable) {
		connect(qsynth, SIGNAL(partStateReset()), SLOT(handleReset()));
		connect(handler, SIGNAL(polyStateChanged(int)), SLOT(handlePolyStateChanged(int)));
		connect(handler, SIGNAL(partialStateChanged(int, int)), SLOT(handlePartialStateChanged(int, int)));
		connect(handler, SIGNAL(programChanged(int, QString)), SLOT(handleProgramChanged(int, QString)));
		handleReset();
	} else {
		disconnect(qsynth, 0, this, 0);
		disconnect(handler, 0, this, 0);
	}
#endif
}

void SynthStateMonitor::handleReset() {
	QPixmap partialLED(15, 15);
	partialLED.fill(partialStateColor[PartialState_DEAD]);
	for (int i = 0; i < 32; i++) {
		partialStateLabel[i]->setPixmap(partialLED);
#ifdef SYNTH_MONITOR_UPDATE_MILLIS
		partialState[i] = 0;
		polyState[i] = 0;
#endif
	}
	QPixmap polyBar(480, 16);
	polyBar.fill(colorGrey);
	for (int i = 0; i < 9; i++) {
		patchNameLabel[i]->setText((i < 8) ? qsynth->getPatchName(i) : "Rhythm Channel");
		polyStateLabel[i]->setPixmap(polyBar);
	}
}

void SynthStateMonitor::handlePolyStateChanged(int partNum) {
	if (synthRoute->getState() != SynthRouteState_OPEN) return;
	QPixmap &polyBar = const_cast <QPixmap &> (*polyStateLabel[partNum]->pixmap());
	polyBar.fill(colorGrey);
	QPainter painter(&polyBar);
	for (int i = 0; i < MT32EMU_MAX_PARTIALS; i++) {
		const MT32Emu::Partial *partial = qsynth->getPartial(i);
		if (partial == NULL || partial->getOwnerPart() != partNum) continue;
		const MT32Emu::Poly *poly = partial->getPoly();
		if (poly == NULL) continue;
		uint velocity = poly->getVelocity();
		if (velocity == 0) continue;
		uint key = partial->getPoly()->getKey();
		QColor color(2 * velocity, 255 - 2 * velocity, 0);
		uint x  = 5 * (key - 12);
		painter.fillRect(x, 0, 5, 16, color);
	}
	polyStateLabel[partNum]->update();
}

void SynthStateMonitor::handlePartialStateChanged(int partialNum, int partialState) {
	QPixmap &partialLED = const_cast <QPixmap &> (*partialStateLabel[partialNum]->pixmap());
	partialLED.fill(partialStateColor[partialState]);
	partialStateLabel[partialNum]->update();
}

void SynthStateMonitor::handleProgramChanged(int partNum, QString patchName) {
	patchNameLabel[partNum]->setText(patchName);
}

#ifdef SYNTH_MONITOR_UPDATE_MILLIS
void SynthStateMonitor::handleUpdate() {
	static const int partialPhaseToState[8] = {
		PartialState_ATTACK, PartialState_ATTACK, PartialState_ATTACK, PartialState_ATTACK,
		PartialState_SUSTAINED, PartialState_SUSTAINED, PartialState_RELEASED, PartialState_DEAD
	};

	if (!ui->detailsFrame->isVisible() || synthRoute->getState() != SynthRouteState_OPEN) return;
	bool partUpdated[9] = {false};
	for (int i = 0; i < 32; i++) {
		const MT32Emu::Partial *partial = qsynth->getPartial(i);
		int newState = partial->isActive() ? partialPhaseToState[partial->tva->getPhase()] : PartialState_DEAD;
		if (partialState[i] != newState) {
			handlePartialStateChanged(i, newState);
			partialState[i] = newState;
		}
		uchar part;
		uchar key;
		uchar velocity;
		if (partial->isActive()) {
			part = partial->getOwnerPart();
			key = partial->getPoly()->getKey();
			velocity = partial->getPoly()->getVelocity();
		} else {
			part = polyState[i] & 0xFF;
			key = (polyState[i] >> 8) & 0xFF;
			velocity = 0;
		}
		newState = part | (key << 8) | (velocity << 16);
		if (polyState[i] != newState) {
			partUpdated[part] = true;
			polyState[i] = newState;
		}
	}
	for (int i = 0; i < 9; i++) {
		if (partUpdated[i]) handlePolyStateChanged(i);
	}
}
#endif
