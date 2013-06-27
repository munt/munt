/* Copyright (C) 2011, 2012, 2013 Jerome Fisher, Sergey V. Mikayev
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
#include "font_6x8.h"

static const int SYNTH_MONITOR_UPDATE_MILLIS = 30;
static const MasterClockNanos LCD_MESSAGE_DISPLAYING_NANOS = 2 * MasterClock::NANOS_PER_SECOND;
static const MasterClockNanos LCD_TIMBRE_NAME_DISPLAYING_NANOS = 1 * MasterClock::NANOS_PER_SECOND;

static const QString BANK_NAMES[] = {"Synth-1", "Synth-2", "Memory "};
static const QColor COLOR_GRAY = QColor(100, 100, 100);
static const QColor COLOR_GREEN = Qt::green;
static const QColor lcdBgColor(98, 127, 0);
static const QColor lcdFgColor(232, 254, 0);
static const QColor partialStateColor[] = {COLOR_GRAY, Qt::red, Qt::yellow, Qt::green};

SynthStateMonitor::SynthStateMonitor(Ui::SynthWidget *ui, SynthRoute *useSynthRoute) :
	ui(ui),
	synthRoute(useSynthRoute),
	lcdWidget(*this, ui->synthFrame),
	midiMessageLED(&COLOR_GRAY, ui->midiMessageFrame)
{
	lcdWidget.setMinimumSize(254, 40);
	ui->synthFrameLayout->insertWidget(1, &lcdWidget);
	midiMessageLED.setMinimumSize(10, 2);
	ui->midiMessageLayout->addWidget(&midiMessageLED, 0, Qt::AlignHCenter);

	for (int i = 0; i < MT32EMU_MAX_PARTIALS; i++) {
		partialStateLED[i] = new LEDWidget(&COLOR_GRAY, ui->partialStateGrid->widget());
		partialStateLED[i]->setMinimumSize(16, 16);
		partialStateLED[i]->setMaximumSize(16, 16);
		ui->partialStateGrid->addWidget(partialStateLED[i], i >> 2, i & 3);
	}

	for (int i = 0; i < 9; i++) {
		patchNameLabel[i] = new QLabel(ui->polyStateGrid->widget());
		ui->polyStateGrid->addWidget(patchNameLabel[i], i, 0);

		partStateWidget[i] = new PartStateWidget(i, *this, ui->polyStateGrid->widget());
		partStateWidget[i]->setMinimumSize(480, 16);
		partStateWidget[i]->setMaximumSize(480, 16);
		ui->polyStateGrid->addWidget(partStateWidget[i], i, 1);
	}

	qsynth = synthRoute->findChild<QSynth *>();
	QReportHandler *handler = qsynth->findChild<QReportHandler *>();

	handleReset();
	connect(qsynth, SIGNAL(partStateReset()), SLOT(handleReset()));
	connect(qsynth, SIGNAL(midiMessagePushed()), SLOT(handleMIDIMessagePushed()));
	connect(handler, SIGNAL(programChanged(int, int, QString)), SLOT(handleProgramChanged(int, int, QString)));
	connect(handler, SIGNAL(polyStateChanged(int)), SLOT(handlePolyStateChanged(int)));
	lcdWidget.connect(handler, SIGNAL(lcdMessageDisplayed(const QString)), SLOT(handleLCDMessageDisplayed(const QString)));
	lcdWidget.connect(handler, SIGNAL(masterVolumeChanged(int)), SLOT(handleMasterVolumeChanged(int)));
	connect(&timer, SIGNAL(timeout()), SLOT(handleUpdate()));
}

SynthStateMonitor::~SynthStateMonitor() {
	for (int i = 0; i < 9; i++) {
		delete partStateWidget[i];
		delete patchNameLabel[i];
	}
	for (int i = 0; i < MT32EMU_MAX_PARTIALS; i++) delete partialStateLED[i];
}

void SynthStateMonitor::enableMonitor(bool enable) {
	if (enable) {
		timer.start(SYNTH_MONITOR_UPDATE_MILLIS);
	} else {
		timer.stop();
	}
}

void SynthStateMonitor::handleReset() {
	lcdWidget.reset();
	midiMessageLED.setColor(&COLOR_GRAY);

	for (int i = 0; i < MT32EMU_MAX_PARTIALS; i++) {
		partialStateLED[i]->setColor(&partialStateColor[PartialState_DEAD]);
	}

	for (int i = 0; i < 9; i++) {
		patchNameLabel[i]->setText((i < 8) ? qsynth->getPatchName(i) : "Rhythm Channel");
		partStateWidget[i]->update();
	}
}

void SynthStateMonitor::handleMIDIMessagePushed() {
	if (ui->synthFrame->isVisible() && synthRoute->getState() == SynthRouteState_OPEN) {
		midiMessageLED.setColor(&COLOR_GREEN);
	}
}

void SynthStateMonitor::handlePolyStateChanged(int partNum) {
	partStateWidget[partNum]->update();
}

void SynthStateMonitor::handleProgramChanged(int partNum, int timbreGroup, QString patchName) {
	patchNameLabel[partNum]->setText(patchName);
	lcdWidget.setProgramChangeLCDText(partNum + 1, BANK_NAMES[timbreGroup], patchName);
}

void SynthStateMonitor::handleUpdate() {
	if (synthRoute->getState() != SynthRouteState_OPEN) return;
	bool partActiveNonReleasing[9] = {false};
	bool midiMessageOn = false;
	for (int partialNum = 0; partialNum < MT32EMU_MAX_PARTIALS; partialNum++) {
		const MT32Emu::Partial *partial = qsynth->getPartial(partialNum);
		int partNum = partial->getOwnerPart();
		bool partialActive = partNum > -1;
		PartialState partialState = partialActive ? QSynth::getPartialState(partial->tva->getPhase()) : PartialState_DEAD;
		partialStateLED[partialNum]->setColor(&partialStateColor[partialState]);
		if (partialActive) {
			const MT32Emu::Poly *poly = partial->getPoly();
			bool polyActiveNonReleasing = poly->isActive() && poly->getState() != MT32Emu::POLY_Releasing;
			partActiveNonReleasing[partNum] = partActiveNonReleasing[partNum] || polyActiveNonReleasing;
			midiMessageOn = midiMessageOn || polyActiveNonReleasing;
		}
	}
	if (((lcdWidget.lcdState == LCDWidget::DISPLAYING_MESSAGE) && (MasterClock::getClockNanos() - lcdWidget.lcdStateStartNanos > LCD_MESSAGE_DISPLAYING_NANOS))
		|| ((lcdWidget.lcdState == LCDWidget::DISPLAYING_TIMBRE_NAME) && (MasterClock::getClockNanos() - lcdWidget.lcdStateStartNanos > LCD_TIMBRE_NAME_DISPLAYING_NANOS)))
	{
		lcdWidget.setPartStateLCDText();
	}
	if (lcdWidget.lcdState == LCDWidget::DISPLAYING_PART_STATE) {
		for (int partNum = 0; partNum < 5; partNum++) {
			lcdWidget.maskedChar[partNum << 1] = partActiveNonReleasing[partNum];
		}
		lcdWidget.maskedChar[5 << 1] = partActiveNonReleasing[8]; // mapping for the rhythm channel
		lcdWidget.update();
	}

	midiMessageLED.setColor(midiMessageOn ? &COLOR_GREEN : &COLOR_GRAY);
}

LEDWidget::LEDWidget(const QColor *color, QWidget *parent) : colorProperty(color), QWidget(parent) {}

const QColor *LEDWidget::color() const {
	return colorProperty;
}

void LEDWidget::setColor(const QColor *newColor) {
	if (colorProperty != newColor) {
		colorProperty = newColor;
		update();
	}
}

void LEDWidget::paintEvent(QPaintEvent *paintEvent) {
	QPainter painter(this);
	if (colorProperty != NULL) {
		painter.fillRect(paintEvent->rect(), *colorProperty);
	}
}

PartStateWidget::PartStateWidget(int partNum, const SynthStateMonitor &monitor, QWidget *parent) : partNum(partNum), monitor(monitor), QWidget(parent) {}

void PartStateWidget::paintEvent(QPaintEvent *) {
	QPainter painter(this);
	painter.fillRect(rect(), COLOR_GRAY);
	if (monitor.synthRoute->getState() != SynthRouteState_OPEN) return;
	for (int i = 0; i < MT32EMU_MAX_PARTIALS; i++) {
		const MT32Emu::Partial *partial = monitor.qsynth->getPartial(i);
		if (partial->getOwnerPart() != partNum) continue;
		const MT32Emu::Poly *poly = partial->getPoly();
		uint velocity = poly->getVelocity();
		if (velocity == 0) continue;
		uint key = partial->getPoly()->getKey();
		QColor color(2 * velocity, 255 - 2 * velocity, 0);
		uint x  = 5 * (key - 12);
		painter.fillRect(x, 0, 5, 16, color);
	}
}

LCDWidget::LCDWidget(const SynthStateMonitor &monitor, QWidget *parent) :
	QWidget(parent),
	monitor(monitor),
	lcdOffBackground(":/images/LCDOff.gif"),
	lcdOnBackground(":/images/LCDOn.gif")
{
	reset();
}

void LCDWidget::reset() {
	lcdState = DISPLAYING_PART_STATE;
	lcdStateStartNanos = 0;
	for (int i = 0; i < 20; i++) maskedChar[i] = false;
	masterVolume = 100;
	setPartStateLCDText();
}

void LCDWidget::paintEvent(QPaintEvent *) {
	QPainter lcdPainter(this);
	if (monitor.synthRoute->getState() != SynthRouteState_OPEN) {
		lcdPainter.drawPixmap(0, 0, lcdOffBackground);
		return;
	}
	lcdPainter.drawPixmap(0, 0, lcdOnBackground);
	lcdPainter.translate(7, 9);

	int xat, xstart, yat;
	xstart = 0;
	yat = 0;

	for (int i = 0; i < 20; i++) {
		unsigned char c;
		c = 0x20;
		if (i < lcdText.size()) {
			c = lcdText[i];
		}

		// Don't render characters we don't have mapped
		if (c < 0x20) c = 0x20;
		if (c > 0x7f) c = 0x20;

		c -= 0x20;

		yat = 1;
		for (int t = 0; t < 8; t++) {
			xat = xstart;
			unsigned char fval;
			if (maskedChar[i] && (t != 7) && (lcdState == DISPLAYING_PART_STATE)) {
				fval = 0x1f;
			} else {
				fval = Font_6x8[c][t];
			}
			for (int m = 4; m >= 0; --m) {
				if ((fval >> m) & 1) {
					lcdPainter.fillRect(xat, yat, 2, 2, lcdFgColor);
				} else {
					lcdPainter.fillRect(xat, yat, 2, 2, lcdBgColor);
				}
				xat += 2;
			}
			yat += 2;
			if (t == 6) yat += 2;
		}
		xstart += 12;
	}
}

void LCDWidget::handleLCDMessageDisplayed(const QString useText) {
	lcdState = DISPLAYING_MESSAGE;
	lcdStateStartNanos = MasterClock::getClockNanos();
	lcdText = useText.toAscii();
	update();
}

void LCDWidget::handleMasterVolumeChanged(int volume) {
	masterVolume = volume;
	if (lcdState == DISPLAYING_PART_STATE) {
		setPartStateLCDText();
		update();
	}
}

void LCDWidget::setPartStateLCDText() {
	lcdState = DISPLAYING_PART_STATE;
	lcdText = QString().sprintf("1 2 3 4 5 R |vol:%3d", masterVolume).toAscii();
}

void LCDWidget::setProgramChangeLCDText(int partNum, QString bankName, QString timbreName) {
	if ((lcdState != DISPLAYING_MESSAGE) || (MasterClock::getClockNanos() - lcdStateStartNanos > LCD_MESSAGE_DISPLAYING_NANOS)) {
		lcdState = DISPLAYING_TIMBRE_NAME;
		lcdStateStartNanos = MasterClock::getClockNanos();
		lcdText = (QString().sprintf("%1i|", partNum) + bankName + "|" + timbreName).toAscii();
		update();
	}
}
