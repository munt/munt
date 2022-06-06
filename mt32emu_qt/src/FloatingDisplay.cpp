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

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QDesktopWidget>
#else
#include <QScreen>
#endif

#include <QMenu>
#include <QSlider>
#include <QWidgetAction>

#include "FloatingDisplay.h"

#include "Master.h"
#include "SynthStateMonitor.h"

static const int CONTENT_MARGIN = 2;
static const int LAYOUT_SPACING = 4;
static const QSize MIDI_MESSAGE_LED_SIZE(10, 2);
static const QSize SIZE_GRIP_SIZE(10, 10);

static const int MIN_OPACITY_VALUE = 25;

static Qt::WindowFlags makeWindowFlags() {
	Qt::WindowFlags flags = Qt::WindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
	if (Master::getInstance()->getSettings()->value("FloatingDisplay/bypassWindowManager", true).toBool()) {
		flags |= Qt::X11BypassWindowManagerHint;
	}
	return flags;
}

FloatingDisplay::FloatingDisplay(const QWidget *mainWindow) :
	QFrame(NULL, makeWindowFlags()),
	mainWindow(mainWindow), synthRoute(), lcdWidget(), midiMessageLED(), dragStatus()
{
	setFrameShape(QFrame::Box);
	setFrameShadow(QFrame::Raised);
	setWindowTitle("Munt: Display");

	QSettings *settings = Master::getInstance()->getSettings();
	int storedOpacity = settings->value("FloatingDisplay/opacity", 100).toInt();
	setWindowOpacity(qreal(qBound(MIN_OPACITY_VALUE, storedOpacity, 100) / 100.0));

	int frameWithMargin = frameWidth() + CONTENT_MARGIN;

	lcdWidget = new LCDWidget(this);
	lcdWidget->move(frameWidth() + CONTENT_MARGIN, frameWithMargin);

	midiMessageLED = new MidiMessageLEDWidget(this);

	sizeGrip = new QWidget(this);
	sizeGrip->resize(SIZE_GRIP_SIZE);
	sizeGrip->setCursor(Qt::SizeFDiagCursor);

	baseWindowWidth = lcdWidget->sizeHint().width() + MIDI_MESSAGE_LED_SIZE.width() + 2 * frameWithMargin + LAYOUT_SPACING;
	QRect rect = settings->value("FloatingDisplay/geometry").toRect();
	if (rect.isValid()) {
		layoutWidgets(rect.width());
		rect.setSize(size());
	} else {
		layoutWidgets(baseWindowWidth);
		rect = geometry();
		rect.moveCenter(mainWindow->geometry().center());
	}
	move(rect.topLeft());
	adjustGeometryToScreen();
}

void FloatingDisplay::saveSettings() {
	QSettings *settings = Master::getInstance()->getSettings();
	settings->setValue("FloatingDisplay/geometry", geometry());
	settings->setValue("FloatingDisplay/opacity", qRound(windowOpacity() * 100.0));
}

void FloatingDisplay::setSynthRoute(SynthRoute *useSynthRoute) {
	if (synthRoute != NULL) {
		synthRoute->disconnectSynth(SIGNAL(stateChanged(SynthState)), this, SLOT(handleSynthStateChange(SynthState)));
		synthRoute->disconnectReportHandler(SIGNAL(lcdStateChanged()), lcdWidget, SLOT(handleLCDUpdate()));
		synthRoute->disconnectReportHandler(SIGNAL(midiMessageLEDStateChanged(bool)), this, SLOT(handleMidiMessageLEDUpdate(bool)));
	}
	synthRoute = useSynthRoute;
	lcdWidget->setSynthRoute(useSynthRoute);
	if (synthRoute != NULL) {
		synthRoute->connectSynth(SIGNAL(stateChanged(SynthState)), this, SLOT(handleSynthStateChange(SynthState)));
		synthRoute->connectReportHandler(SIGNAL(lcdStateChanged()), lcdWidget, SLOT(handleLCDUpdate()));
		synthRoute->connectReportHandler(SIGNAL(midiMessageLEDStateChanged(bool)), this, SLOT(handleMidiMessageLEDUpdate(bool)));
	}
	midiMessageLED->setState(lcdWidget->updateDisplayText());
}

void FloatingDisplay::handleSynthStateChange(SynthState state) {
	if (state == SynthState_OPEN) {
		midiMessageLED->setState(lcdWidget->updateDisplayText());
	} else {
		lcdWidget->update();
		midiMessageLED->setState(false);
	}
}

void FloatingDisplay::handleMidiMessageLEDUpdate(bool midiMessageOn) {
	midiMessageLED->setState(midiMessageOn);
}

void FloatingDisplay::handleOpacitySliderValueChanged(int value) {
	setWindowOpacity(qreal(value / 100.0));
}

void FloatingDisplay::closeEvent(QCloseEvent *event) {
	event->ignore();
}

void FloatingDisplay::contextMenuEvent(QContextMenuEvent *event) {
	event->accept();
	QMenu menu(this);
	menu.addAction("Show/Hide Main Window", mainWindow, SLOT(showHideMainWindow()));
	menu.addAction("Hide Floating Display", this, SLOT(hide()));
	menu.addAction("Show MIDI Player", mainWindow, SLOT(on_actionPlay_MIDI_file_triggered()));
	QMenu *menuOpacity = menu.addMenu("Opacity");
	QWidgetAction opacityAction(menuOpacity);
	QSlider opacitySlider;
	opacitySlider.setOrientation(Qt::Horizontal);
	opacitySlider.setRange(MIN_OPACITY_VALUE, 100);
	opacitySlider.setValue(qRound(windowOpacity() * 100));
	connect(&opacitySlider, SIGNAL(valueChanged(int)), SLOT(handleOpacitySliderValueChanged(int)));
	opacityAction.setDefaultWidget(&opacitySlider);
	menuOpacity->addAction(&opacityAction);
	menu.addAction("Exit", mainWindow, SLOT(on_actionExit_triggered()));
	menu.exec(event->globalPos());
}

void FloatingDisplay::mousePressEvent(QMouseEvent *event) {
	if (event->button() != Qt::LeftButton) {
		event->ignore();
		return;
	}
	event->accept();
	if (DRAG_STATUS_OFF == dragStatus) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
		startDrag(event->globalPos());
#else
		startDrag(event->globalPosition().toPoint());
#endif
		dragStatus = DRAG_STATUS_DRAGGING;
	}
}

void FloatingDisplay::mouseMoveEvent(QMouseEvent *event) {
	if (event->buttons() != Qt::LeftButton) {
		event->ignore();
		return;
	}
	event->accept();
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	const QPoint dragPosition = event->globalPos();
#else
	const QPoint dragPosition = event->globalPosition().toPoint();
#endif
	if (DRAG_STATUS_OFF == dragStatus) {
		startDrag(dragPosition);
		return;
	}
	const QPoint dragOffset = dragPosition - dragOrigin;
	if (DRAG_STATUS_INITIATING == dragStatus) {
		MasterClockNanos elapsedNanos = MasterClock::getClockNanos() - startNanos;
		if (!resizing && (elapsedNanos / MasterClock::NANOS_PER_MILLISECOND) < QApplication::startDragTime()
			&& dragOffset.manhattanLength() < QApplication::startDragDistance()) return;
		dragStatus = DRAG_STATUS_DRAGGING;
	}
	if (resizing) {
		layoutWidgets(dragOffset.x() + startWindowWidth);
	} else {
		move(startWindowPosition + dragOffset);
	}
}

void FloatingDisplay::mouseReleaseEvent(QMouseEvent *event) {
	if (event->button() != Qt::LeftButton) {
		event->ignore();
		return;
	}
	event->accept();
	dragStatus = DRAG_STATUS_OFF;
}

void FloatingDisplay::moveEvent(QMoveEvent *event) {
	if (event->spontaneous()) adjustGeometryToScreen();
}

void FloatingDisplay::resizeEvent(QResizeEvent *event) {
	if (event->spontaneous()) layoutWidgets(event->size().width());
}

void FloatingDisplay::showEvent(QShowEvent *) {
	adjustGeometryToScreen();
}

void FloatingDisplay::startDrag(const QPoint &mousePosition) {
	dragStatus = DRAG_STATUS_INITIATING;
	dragOrigin = mousePosition;
	startWindowPosition = pos();
	startWindowWidth = width();
	resizing = sizeGrip->underMouse();
	startNanos = MasterClock::getClockNanos();
}

void FloatingDisplay::layoutWidgets(int targetWidth) {
	targetWidth = qMin(qMax(targetWidth, baseWindowWidth / 2), 8 * baseWindowWidth);

	qreal scale = targetWidth / qreal(baseWindowWidth);
	midiMessageLED->resize(MIDI_MESSAGE_LED_SIZE * scale);

	int frameWithMargin = frameWidth() + CONTENT_MARGIN;
	int lcdWidth = targetWidth - (midiMessageLED->width() + 2 * frameWithMargin + LAYOUT_SPACING);
	int lcdHeight = lcdWidget->heightForWidth(lcdWidth);
	lcdWidget->resize(lcdWidth, lcdHeight);
	int targetHeight = lcdHeight + 2 * frameWithMargin;

	midiMessageLED->move(frameWithMargin + lcdWidth + LAYOUT_SPACING, frameWithMargin + (lcdHeight - midiMessageLED->height()) / 2);

	sizeGrip->move(targetWidth - SIZE_GRIP_SIZE.width(), targetHeight - SIZE_GRIP_SIZE.height());

	resize(targetWidth, targetHeight);
}

void FloatingDisplay::adjustGeometryToScreen() {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	QRect availableGeometry = QApplication::desktop()->availableGeometry(this);
#else
	QRect availableGeometry = screen()->availableGeometry();
#endif
	QRect rect = geometry();
	layoutWidgets(qMin(availableGeometry.width(), rect.width()));
	rect.setSize(size());
	if (availableGeometry.right() < rect.right()) rect.moveRight(availableGeometry.right());
	if (availableGeometry.bottom() < rect.bottom()) rect.moveBottom(availableGeometry.bottom());
	if (rect.left() < availableGeometry.left()) rect.moveLeft(availableGeometry.left());
	if (rect.top() < availableGeometry.top()) rect.moveTop(availableGeometry.top());
	move(rect.topLeft());
}
