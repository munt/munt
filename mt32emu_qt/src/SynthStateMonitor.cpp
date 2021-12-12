/* Copyright (C) 2011-2021 Jerome Fisher, Sergey V. Mikayev
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

#include "SynthRoute.h"
#include "ui_SynthWidget.h"
#include "font_6x8.h"

static const QColor COLOR_GRAY(100, 100, 100);
static const QColor COLOR_GREEN(Qt::green);
static const QColor lcdUnlitColor = QColor::fromRgba(0x32F2FEEDU);
static const QColor lcdLitColor = QColor::fromRgb(0xCAFB10U);
static const QColor partialStateColor[] = {COLOR_GRAY, Qt::red, Qt::yellow, Qt::green};

static const uint LCD_BOTTOM_ROW_INDEX = 6;
static const uint LCD_ROW_COUNT = 8;

static const qreal LCD_SCALE_FACTOR = 1.0 / 4.0;
static const uint LCD_PIXEL_SIZE = 7;
static const uint LCD_PIXEL_SIZE_WITH_SPACING = 8;
static const uint LCD_UNDERLINE_GAP = LCD_PIXEL_SIZE_WITH_SPACING;
static const uint LCD_COLUMN_SIZE_WITH_SPACING = 6 * LCD_PIXEL_SIZE_WITH_SPACING;

static const QPoint LCD_CONTENT_INSETS(8, 10);

using namespace MT32Emu;

SynthStateMonitor::SynthStateMonitor(Ui::SynthWidget *ui, SynthRoute *useSynthRoute) :
	synthRoute(useSynthRoute),
	ui(ui),
	lcdWidget(*this, ui->synthFrame),
	midiMessageLED(&COLOR_GRAY, ui->midiMessageFrame)
{
	partialCount = useSynthRoute->getPartialCount();
	allocatePartialsData();

	QSizePolicy lcdWidgetPolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	lcdWidgetPolicy.setHeightForWidth(true);
	lcdWidget.setSizePolicy(lcdWidgetPolicy);
	ui->synthFrameLayout->insertWidget(1, &lcdWidget);
	midiMessageLED.setMinimumSize(10, 2);
	ui->midiMessageLayout->addWidget(&midiMessageLED, 0, Qt::AlignHCenter);

	for (int i = 0; i < 9; i++) {
		patchNameLabel[i] = new QLabel(ui->polyStateGrid->widget());
		ui->polyStateGrid->addWidget(patchNameLabel[i], i, 0);

		partStateWidget[i] = new PartStateWidget(i, *this, ui->polyStateGrid->widget());
		partStateWidget[i]->setMinimumSize(480, 16);
		partStateWidget[i]->setMaximumSize(480, 16);
		ui->polyStateGrid->addWidget(partStateWidget[i], i, 1);
	}

	handleSynthStateChange(synthRoute->getState() == SynthRouteState_OPEN ? SynthState_OPEN : SynthState_CLOSED);
	synthRoute->connectSynth(SIGNAL(stateChanged(SynthState)), this, SLOT(handleSynthStateChange(SynthState)));
	synthRoute->connectSynth(SIGNAL(audioBlockRendered()), this, SLOT(handleAudioBlockRendered()));
	synthRoute->connectReportHandler(SIGNAL(programChanged(int, QString, QString)), this, SLOT(handleProgramChanged(int, QString, QString)));
	synthRoute->connectReportHandler(SIGNAL(polyStateChanged(int)), this, SLOT(handlePolyStateChanged(int)));
	synthRoute->connectReportHandler(SIGNAL(lcdStateChanged()), this, SLOT(handleLCDUpdate()));
	synthRoute->connectReportHandler(SIGNAL(midiMessageLEDStateChanged(bool)), this, SLOT(handleMidiMessageLEDUpdate(bool)));
}

SynthStateMonitor::~SynthStateMonitor() {
	for (int i = 0; i < 9; i++) {
		delete partStateWidget[i];
		delete patchNameLabel[i];
	}
	freePartialsData();
}

void SynthStateMonitor::enableMonitor(bool enable) {
	enabled = enable;
}

void SynthStateMonitor::handleSynthStateChange(SynthState state) {
	enableMonitor(state == SynthState_OPEN);
	midiMessageLED.setColor(&COLOR_GRAY);
	handleLCDUpdate();

	uint newPartialCount = synthRoute->getPartialCount();
	if (partialCount == newPartialCount || state != SynthState_OPEN) {
		for (unsigned int i = 0; i < partialCount; i++) {
			partialStateLED[i]->setColor(&partialStateColor[PartialState_INACTIVE]);
		}
	} else {
		freePartialsData();
		partialCount = newPartialCount;
		allocatePartialsData();
	}

	for (int i = 0; i < 9; i++) {
		patchNameLabel[i]->setText((i < 8) ? synthRoute->getPatchName(i) : "Rhythm Channel");
		partStateWidget[i]->update();
	}
}

void SynthStateMonitor::handlePolyStateChanged(int partNum) {
	partStateWidget[partNum]->update();
}

void SynthStateMonitor::handleProgramChanged(int partNum, QString, QString patchName) {
	patchNameLabel[partNum]->setText(patchName);
}

void SynthStateMonitor::handleLCDUpdate() {
	if (!enabled) return;
	synthRoute->getDisplayState(lcdWidget.lcdText);
	lcdWidget.update();
}

void SynthStateMonitor::handleMidiMessageLEDUpdate(bool midiMessageOn) {
	if (enabled) midiMessageLED.setColor(midiMessageOn ? &COLOR_GREEN : &COLOR_GRAY);
}

void SynthStateMonitor::handleAudioBlockRendered() {
	if (!enabled) return;
	synthRoute->getPartialStates(partialStates);
	for (unsigned int partialNum = 0; partialNum < partialCount; partialNum++) {
		partialStateLED[partialNum]->setColor(&partialStateColor[partialStates[partialNum]]);
	}
}

void SynthStateMonitor::allocatePartialsData() {
	partialStates = new PartialState[partialCount];
	keysOfPlayingNotes = new Bit8u[partialCount];
	velocitiesOfPlayingNotes = new Bit8u[partialCount];

	partialStateLED = new LEDWidget*[partialCount];
	unsigned int partialColumnWidth;
	if (partialCount < 64) {
		partialColumnWidth = 4;
	} else if (partialCount < 128) {
		partialColumnWidth = 8;
	} else {
		partialColumnWidth = 16;
	}
	for (unsigned int i = 0; i < partialCount; i++) {
		partialStateLED[i] = new LEDWidget(&COLOR_GRAY, ui->partialStateGrid->widget());
		partialStateLED[i]->setMinimumSize(16, 16);
		partialStateLED[i]->setMaximumSize(16, 16);
		ui->partialStateGrid->addWidget(partialStateLED[i], i / partialColumnWidth, i % partialColumnWidth);
	}
}

void SynthStateMonitor::freePartialsData() {
	if (partialStateLED != NULL) {
		for (unsigned int i = 0; i < partialCount; i++) delete partialStateLED[i];
	}
	delete[] partialStateLED;
	partialStateLED = NULL;
	delete[] velocitiesOfPlayingNotes;
	velocitiesOfPlayingNotes = NULL;
	delete[] keysOfPlayingNotes;
	keysOfPlayingNotes = NULL;
	delete[] partialStates;
	partialStates = NULL;
}

LEDWidget::LEDWidget(const QColor *color, QWidget *parent) : QWidget(parent), colorProperty(color) {}

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

PartStateWidget::PartStateWidget(int partNum, const SynthStateMonitor &monitor, QWidget *parent) : QWidget(parent), partNum(partNum), monitor(monitor) {}

void PartStateWidget::paintEvent(QPaintEvent *) {
	QPainter painter(this);
	painter.fillRect(rect(), COLOR_GRAY);
	if (monitor.synthRoute->getState() != SynthRouteState_OPEN) return;
	uint playingNotes = monitor.synthRoute->getPlayingNotes(partNum, monitor.keysOfPlayingNotes, monitor.velocitiesOfPlayingNotes);
	while (playingNotes-- > 0) {
		uint velocity = monitor.velocitiesOfPlayingNotes[playingNotes];
		if (velocity == 0) continue;
		QColor color(2 * velocity, 255 - 2 * velocity, 0);
		uint x  = 5 * (monitor.keysOfPlayingNotes[playingNotes] - 12);
		painter.fillRect(x, 0, 5, 16, color);
	}
}

LCDWidget::LCDWidget(const SynthStateMonitor &monitor, QWidget *parent) :
	QWidget(parent),
	monitor(monitor),
	lcdOffBackground(":/images/LCDOff.gif"),
	lcdOnBackground(":/images/LCDOn.gif")
{}

int LCDWidget::heightForWidth (int useWidth) const {
	return useWidth * lcdOnBackground.height() / lcdOnBackground.width();
}

void LCDWidget::paintEvent(QPaintEvent *) {
	QPainter lcdPainter(this);
	lcdPainter.setRenderHint(QPainter::Antialiasing, true);
	qreal scaleFactor = qreal(width()) / lcdOnBackground.width();
	lcdPainter.scale(scaleFactor, scaleFactor);
	bool open = monitor.synthRoute->getState() == SynthRouteState_OPEN;
	lcdPainter.drawPixmap(0, 0, open ? lcdOnBackground : lcdOffBackground);
	if (!open) return;
	lcdPainter.translate(LCD_CONTENT_INSETS);
	lcdPainter.scale(LCD_SCALE_FACTOR, LCD_SCALE_FACTOR);

	bool endOfText = false;
	QRect rect(0, 0, LCD_PIXEL_SIZE, LCD_PIXEL_SIZE);
	for (int position = 0; position < 20; position++) {
		uchar charCode = endOfText ? 0x20 : lcdText[position];

		// Map special characters to what we have defined in the font.
		if (charCode < 0x20) {
			switch (charCode) {
			case 0x02:
				charCode = 0x7C;
				break;
			case 0x01:
				charCode = 0x80;
				break;
			case 0x00:
				endOfText = true;
				// Fall-through
			default:
				charCode = 0x20;
				break;
			}
		} else if (charCode > 0x7f) charCode = 0x20;

		charCode -= 0x20;

		for (uint rowIndex = 0; rowIndex < LCD_ROW_COUNT; rowIndex++) {
			uchar row = Font_6x8[charCode][rowIndex];
			for (uint mask = 0x10; mask > 0; mask >>= 1) {
				const QColor &color = (row & mask) ? lcdLitColor : lcdUnlitColor;
				lcdPainter.fillRect(rect, color);
				rect.moveLeft(rect.x() + LCD_PIXEL_SIZE_WITH_SPACING);
			}
			rect.moveLeft(0);
			rect.moveTop(rect.y() + LCD_PIXEL_SIZE_WITH_SPACING);
			if (rowIndex == LCD_BOTTOM_ROW_INDEX) rect.moveTop(rect.y() + LCD_UNDERLINE_GAP);
		}
		rect.moveTop(0);
		lcdPainter.translate(LCD_COLUMN_SIZE_WITH_SPACING, 0);
	}
}

void LCDWidget::mousePressEvent(QMouseEvent *) {
	monitor.synthRoute->setMainDisplayMode();
}
