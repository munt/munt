/* Copyright (C) 2011-2022 Jerome Fisher, Sergey V. Mikayev
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

#include <QApplication>
#include <QMenu>
#include <QSlider>
#include <QStylePainter>
#include <QWidgetAction>

#include "SynthStateMonitor.h"

#include "SynthRoute.h"
#include "ui_SynthWidget.h"
#include "font_6x8.h"

static const QColor COLOR_GRAY(100, 100, 100);
static const QColor COLOR_GREEN(Qt::green);
static const QColor LCD_UNLIT_COLOR = QColor::fromRgba(0x32F2FEEDU);
static const QColor LCD_LIT_COLOR = QColor::fromRgb(0xCAFB10U);
static const QColor PARTIAL_STATE_COLORS[] = {COLOR_GRAY, Qt::red, Qt::yellow, Qt::green};

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
	lcdWidget(ui->synthFrame),
	midiMessageLED(ui->midiMessageFrame)
{
	partialCount = useSynthRoute->getPartialCount();
	allocatePartialsData();
	lcdWidget.setSynthRoute(useSynthRoute);

	ui->synthFrameLayout->insertWidget(1, &lcdWidget);
	midiMessageLED.setMinimumSize(10, 2);
	ui->midiMessageLayout->addWidget(&midiMessageLED, 0, Qt::AlignHCenter);

	for (int i = 0; i < 9; i++) {
		int column = 1;
		partVolumeButton[i] = new PartVolumeButton(ui->polyStateGrid->widget(), *this, i);
		partVolumeButton[i]->setFixedSize(16, 16);
		ui->polyStateGrid->addWidget(partVolumeButton[i], i, column++);

		patchNameButton[i] = new PatchNameButton(ui->polyStateGrid->widget(), *useSynthRoute, i);
		ui->polyStateGrid->addWidget(patchNameButton[i], i, column++);

		partStateWidget[i] = new PartStateWidget(i, *this, ui->polyStateGrid->widget());
		partStateWidget[i]->setFixedSize(480, 16);
		ui->polyStateGrid->addWidget(partStateWidget[i], i, column++);
	}

	handleSynthStateChange(synthRoute->getState() == SynthRouteState_OPEN ? SynthState_OPEN : SynthState_CLOSED);
	synthRoute->connectSynth(SIGNAL(stateChanged(SynthState)), this, SLOT(handleSynthStateChange(SynthState)));
	synthRoute->connectSynth(SIGNAL(stateChanged(SynthState)), partVolumeButton[0], SLOT(handleResetAllTriggered()));
	synthRoute->connectReportHandler(SIGNAL(programChanged(int, QString, QString)), this, SLOT(handleProgramChanged(int, QString, QString)));
	synthRoute->connectReportHandler(SIGNAL(polyStateChanged(int)), this, SLOT(handlePolyStateChanged(int)));
}

SynthStateMonitor::~SynthStateMonitor() {
	for (int i = 0; i < 9; i++) {
		delete partStateWidget[i];
		delete patchNameButton[i];
	}
	freePartialsData();
}

void SynthStateMonitor::enableMonitor(bool enabled) {
	if (enabled) {
		synthRoute->connectReportHandler(SIGNAL(lcdStateChanged()), &lcdWidget, SLOT(handleLCDUpdate()));
		synthRoute->connectReportHandler(SIGNAL(midiMessageLEDStateChanged(bool)), this, SLOT(handleMidiMessageLEDUpdate(bool)));
		synthRoute->connectSynth(SIGNAL(audioBlockRendered()), this, SLOT(handleAudioBlockRendered()));
		midiMessageLED.setState(lcdWidget.updateDisplayText());
		handleAudioBlockRendered();
	} else {
		synthRoute->disconnectReportHandler(SIGNAL(lcdStateChanged()), &lcdWidget, SLOT(handleLCDUpdate()));
		synthRoute->disconnectReportHandler(SIGNAL(midiMessageLEDStateChanged(bool)), this, SLOT(handleMidiMessageLEDUpdate(bool)));
		synthRoute->disconnectSynth(SIGNAL(audioBlockRendered()), this, SLOT(handleAudioBlockRendered()));
	}
}

void SynthStateMonitor::handleSynthStateChange(SynthState state) {
	enableMonitor(state == SynthState_OPEN);
	if (state != SynthState_OPEN) {
		lcdWidget.update();
		midiMessageLED.setState(false);
	}

	uint newPartialCount = synthRoute->getPartialCount();
	if (partialCount == newPartialCount || state != SynthState_OPEN) {
		for (unsigned int i = 0; i < partialCount; i++) {
			partialStateLED[i]->setState(PartialState_INACTIVE);
		}
	} else {
		freePartialsData();
		partialCount = newPartialCount;
		allocatePartialsData();
	}

	for (int i = 0; i < 9; i++) {
		partVolumeButton[i]->setEnabled(state == SynthState_OPEN);
		patchNameButton[i]->setEnabled(state == SynthState_OPEN);
		patchNameButton[i]->setText((i < 8) ? synthRoute->getPatchName(i) : "Rhythm Channel");
		partStateWidget[i]->update();
	}
}

void SynthStateMonitor::handlePolyStateChanged(int partNum) {
	partStateWidget[partNum]->update();
}

void SynthStateMonitor::handleProgramChanged(int partNum, QString, QString patchName) {
	if (synthRoute->getState() != SynthRouteState_OPEN) return;
	patchNameButton[partNum]->setText(patchName);
}

void SynthStateMonitor::handleMidiMessageLEDUpdate(bool midiMessageOn) {
	if (synthRoute->getState() != SynthRouteState_OPEN) return;
	midiMessageLED.setState(midiMessageOn);
}

void SynthStateMonitor::handleAudioBlockRendered() {
	if (synthRoute->getState() != SynthRouteState_OPEN) return;
	synthRoute->getPartialStates(partialStates);
	for (unsigned int partialNum = 0; partialNum < partialCount; partialNum++) {
		partialStateLED[partialNum]->setState(partialStates[partialNum]);
	}
}

void SynthStateMonitor::allocatePartialsData() {
	partialStates = new PartialState[partialCount];
	keysOfPlayingNotes = new Bit8u[partialCount];
	velocitiesOfPlayingNotes = new Bit8u[partialCount];

	partialStateLED = new PartialStateLEDWidget*[partialCount];
	uint partialColumnWidth = (partialCount + 7) / 8;
	for (uint i = 0; i < partialCount; i++) {
		partialStateLED[i] = new PartialStateLEDWidget(ui->partialStateGrid->widget());
		partialStateLED[i]->setFixedSize(16, 16);
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

LEDWidget::LEDWidget(QWidget *parent, const QColor *initialColor) : QWidget(parent), colorProperty(initialColor) {}

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

MidiMessageLEDWidget::MidiMessageLEDWidget(QWidget *parent) : LEDWidget(parent, &COLOR_GRAY) {}

void MidiMessageLEDWidget::setState(bool state) {
	setColor(state ? &COLOR_GREEN : &COLOR_GRAY);
}

PartialStateLEDWidget::PartialStateLEDWidget(QWidget *parent) : LEDWidget(parent, &COLOR_GRAY) {}

void PartialStateLEDWidget::setState(PartialState state) {
	setColor(&PARTIAL_STATE_COLORS[state]);
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
		uint x = 5 * (monitor.keysOfPlayingNotes[playingNotes] - 12);
		painter.fillRect(x, 0, 5, 16, color);
	}
}

LCDWidget::LCDWidget(QWidget *parent) :
	QWidget(parent),
	synthRoute(),
	lcdOffBackground(":/images/LCDOff.gif"),
	lcdOnBackground(":/images/LCDOn.gif")
{
	QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
	sizePolicy.setHeightForWidth(true);
	setSizePolicy(sizePolicy);
}

void LCDWidget::setSynthRoute(SynthRoute *useSynthRoute) {
	synthRoute = useSynthRoute;
}

bool LCDWidget::updateDisplayText() {
	bool ledState = synthRoute != NULL && synthRoute->getDisplayState(lcdText);
	update();
	return ledState;
}

QSize LCDWidget::sizeHint() const {
	return lcdOnBackground.size();
}

QSize LCDWidget::minimumSizeHint() const {
	return lcdOnBackground.size() / 2;
}

int LCDWidget::heightForWidth(int useWidth) const {
	return useWidth * lcdOnBackground.height() / lcdOnBackground.width();
}

void LCDWidget::paintEvent(QPaintEvent *) {
	QPainter lcdPainter(this);
	lcdPainter.setRenderHint(QPainter::Antialiasing, true);
	qreal scaleFactor = qreal(width()) / lcdOnBackground.width();
	lcdPainter.scale(scaleFactor, scaleFactor);
	bool open = synthRoute != NULL && synthRoute->getState() == SynthRouteState_OPEN;
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
				const QColor &color = (row & mask) ? LCD_LIT_COLOR : LCD_UNLIT_COLOR;
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

void LCDWidget::mousePressEvent(QMouseEvent *event) {
	if (synthRoute != NULL) synthRoute->setMainDisplayMode();
	event->accept();
}

void LCDWidget::handleLCDUpdate() {
	updateDisplayText();
}

PartVolumeButton::PartVolumeButton(QWidget *parent, const SynthStateMonitor &monitor, int partNum) :
	QAbstractButton(parent), monitor(monitor), partNum(partNum), volume(101)
{
	connect(this, SIGNAL(clicked()), SLOT(handleClicked()));
}

void PartVolumeButton::paintEvent(QPaintEvent*) {
	QStylePainter painter(this);
	QIcon icon = style()->standardIcon(volume > 0 ? QStyle::SP_MediaVolume : QStyle::SP_MediaVolumeMuted);
	painter.drawItemPixmap(rect(), Qt::AlignCenter, icon.pixmap(size()));
	if (hasFocus()) {
		QStyleOptionFocusRect opt;
		opt.initFrom(this);
		painter.drawPrimitive(QStyle::PE_FrameFocusRect, opt);
	}
}

void PartVolumeButton::contextMenuEvent(QContextMenuEvent *event) {
	event->accept();
	QMenu menu(this);
	QMenu *volumeMenu = menu.addMenu("Volume");
	QWidgetAction volumeControlAction(volumeMenu);
	QSlider volumeControlSlider(volumeMenu);
	volumeControlSlider.setRange(0, 100);
	volumeControlSlider.setValue(qBound(0, volume, 100));
	connect(&volumeControlSlider, SIGNAL(valueChanged(int)), SLOT(handleVolumeChanged(int)));
	volumeControlAction.setDefaultWidget(&volumeControlSlider);
	volumeMenu->addAction(&volumeControlAction);
	menu.addAction("Reset Volume", this, SLOT(handleResetVolumeTriggered()));
	menu.addAction("Solo", this, SLOT(handleSoloTriggered()));
	menu.addAction("Unmute All", this, SLOT(handleUnmuteAllTriggered()));
	menu.addAction("Reset All", this, SLOT(handleResetAllTriggered()));
	menu.exec(event->globalPos());
}

void PartVolumeButton::toggleMutePart() {
	handleVolumeChanged(volume == 0 ? 101 : -volume);
}

void PartVolumeButton::mutePart() {
	if (volume > 0) toggleMutePart();
}

void PartVolumeButton::unmutePart() {
	if (volume <= 0) toggleMutePart();
}

void PartVolumeButton::toggleSoloPart(bool enabled) {
	for (int i = 0; i < 9; i++) {
		if (enabled && i != partNum) {
			monitor.partVolumeButton[i]->mutePart();
		} else {
			monitor.partVolumeButton[i]->unmutePart();
		}
	}
}

void PartVolumeButton::handleClicked() {
	Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
	if (modifiers & Qt::ControlModifier) {
		toggleSoloPart(true);
	} else if (modifiers & Qt::AltModifier) {
		toggleSoloPart(false);
	} else {
		toggleMutePart();
	}
}

void PartVolumeButton::handleVolumeChanged(int newVolume) {
	volume = newVolume;
	monitor.synthRoute->setPartVolumeOverride(partNum, qBound(0, volume, 101));
	update();
}

void PartVolumeButton::handleResetVolumeTriggered() {
	handleVolumeChanged(101);
}

void PartVolumeButton::handleSoloTriggered() {
	toggleSoloPart(true);
}

void PartVolumeButton::handleUnmuteAllTriggered() {
	toggleSoloPart(false);
}

void PartVolumeButton::handleResetAllTriggered() {
	for (int i = 0; i < 9; i++) {
		monitor.partVolumeButton[i]->handleResetVolumeTriggered();
	}
}

PatchNameButton::PatchNameButton(QWidget *parent, SynthRoute &synthRoute, int partNumber) :
	QAbstractButton(parent), synthRoute(synthRoute), partNumber(partNumber)
{
	if (partNumber < 8) {
		connect(this, SIGNAL(clicked()), SLOT(handleClicked()));
	} else {
		setFocusPolicy(Qt::NoFocus);
	}
}

QSize PatchNameButton::sizeHint() const {
	int height = 16;
	int textWidth;
	if (text().isEmpty()) {
		textWidth = 64;
	} else {
		QSize textSize(fontMetrics().size(0, text()));
		textWidth = textSize.width();
		if (height < textSize.height()) height = textSize.height();
	}
	QSize sizeHint(textWidth, height);
	if (partNumber < 8) {
		QStyleOptionButton option;
		option.initFrom(this);
		option.rect.setSize(sizeHint);
		sizeHint.rwidth() += style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &option, this);
	}
	return sizeHint;
}

void PatchNameButton::paintEvent(QPaintEvent *) {
	QStylePainter painter(this);
	painter.drawItemText(rect(), Qt::AlignVCenter | Qt::AlignLeft, palette(), true, text());
	if (partNumber < 8) {
		QStyleOptionButton option;
		option.initFrom(this);
		int arrowWidth = style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &option, this);
		option.rect.setRect(width() - arrowWidth - 1, 0, arrowWidth, height());
		painter.drawPrimitive(QStyle::PE_IndicatorArrowDown, option);
	}
	if (hasFocus()) {
		QStyleOptionFocusRect option;
		option.initFrom(this);
		painter.drawPrimitive(QStyle::PE_FrameFocusRect, option);
	}
}

void PatchNameButton::handleClicked() {
	QVector<SoundGroup> groups;
	synthRoute.getSoundGroups(groups);

	QMenu menu(this);
	QMenu *groupMenu = NULL;
	for (int groupIx = 0; groupIx < groups.size(); groupIx++) {
		const SoundGroup &group = groups.at(groupIx);
		groupMenu = menu.addMenu(group.name);
		const QVector<SoundGroup::Item> &items = group.constituents;
		for (int itemIx = 0; itemIx < items.size(); itemIx++) {
			const SoundGroup::Item &item = items.at(itemIx);
			QString timbreName = item.timbreName.trimmed();
			if (timbreName.isEmpty()) {
				timbreName = "Memory Timbre #" + QString::number(item.timbreNumber);
			}
			groupMenu->addAction(timbreName)->setData(QVariant::fromValue(item));
		}
	}
	QAction *triggeredAction = menu.exec(mapToGlobal(mapFromParent(geometry().bottomLeft())));
	if (triggeredAction != NULL) {
		SoundGroup::Item item = triggeredAction->data().value<SoundGroup::Item>();
		synthRoute.setTimbreOnPart(partNumber, item.timbreGroup, item.timbreNumber);
	}
}
