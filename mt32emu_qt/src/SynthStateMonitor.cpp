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

SynthStateMonitor::SynthStateMonitor(Ui::SynthWidget *ui, SynthRoute *useSynthRoute) {
	this->ui = ui;
	synthRoute = useSynthRoute;

	deadPartialLEDColor = QColor(100, 100, 100);
	attackPartialLEDColor = Qt::red;
	sustainPartialLEDColor = Qt::yellow;
	releasePartialLEDColor = Qt::green;

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
	QReportHandler *handler = qsynth->findChild<QReportHandler *>();
	handleReset();
	connect(qsynth, SIGNAL(partStateReset()), SLOT(handleReset()));
	connect(handler, SIGNAL(polyStateChanged(int)), SLOT(handlePolyStateChanged(int)));
	connect(handler, SIGNAL(partialStateChanged(int, int)), SLOT(handlePartialStateChanged(int, int)));
	connect(handler, SIGNAL(programChanged(int, QString)), SLOT(handleProgramChanged(int, QString)));
}

SynthStateMonitor::~SynthStateMonitor() {
	for (int i = 0; i < 9; i++) {
		delete polyStateLabel[i];
		delete patchNameLabel[i];
	}
	for (int i = 0; i < 32; i++) delete partialStateLabel[i];
}

void SynthStateMonitor::handleReset() {
	QPixmap partialLED(15, 15);
	partialLED.fill(deadPartialLEDColor);
	for (int i = 0; i < 32; i++) {
		partialStateLabel[i]->setPixmap(partialLED);
		partialState[i] = 0;
	}
	QPixmap polyBar(480, 16);
	polyBar.fill(deadPartialLEDColor);
	for (int i = 0; i < 9; i++) {
		patchNameLabel[i]->setText((i < 8) ? qsynth->getPatchName(i) : "Rhythm Channel");
		polyStateLabel[i]->setPixmap(polyBar);
	}
}

void SynthStateMonitor::handlePolyStateChanged(int partNum) {
	QPixmap &polyBar = const_cast <QPixmap &> (*polyStateLabel[partNum]->pixmap());
	polyBar.fill(deadPartialLEDColor);
	QPainter painter(&polyBar);
	for (int i = 0; i < MT32EMU_MAX_POLY; i++) {
		const MT32Emu::Partial *partial = qsynth->getPartial(i);
		if (partial->getOwnerPart() != partNum) continue;
		uint velocity = partial->getPoly()->getVelocity();
		if (velocity == 0) continue;
		uint key = partial->getPoly()->getKey();
		QColor color(2 * velocity, 255 - 2 * velocity, 0);
		uint x  = 5 * (key - 12);
		painter.fillRect(x, 0, 5, 16, color);
	}
	polyStateLabel[partNum]->update();
}

void SynthStateMonitor::handlePartialStateChanged(int partialNum, int partialPhase) {
	QColor *color;
	int state;
	switch (partialPhase) {
		case MT32Emu::TVA_PHASE_4:
		case MT32Emu::TVA_PHASE_SUSTAIN:
			state = 2;
			color = &sustainPartialLEDColor;
			break;
		case MT32Emu::TVA_PHASE_RELEASE:
			state = 3;
			color = &releasePartialLEDColor;
			break;
		case MT32Emu::TVA_PHASE_DEAD:
			state = 0;
			color = &deadPartialLEDColor;
			break;
		default:
			state = 1;
			color = &attackPartialLEDColor;
			break;
	}
	if (partialState[partialNum] == state) return;
	partialState[partialNum] = state;
	QPixmap &partialLED = const_cast <QPixmap &> (*partialStateLabel[partialNum]->pixmap());
	partialLED.fill(*color);
	partialStateLabel[partialNum]->update();
}

void SynthStateMonitor::handleProgramChanged(int partNum, QString patchName) {
	patchNameLabel[partNum]->setText(patchName);
}
