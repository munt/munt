#ifndef FLOATING_DISPLAY_H
#define FLOATING_DISPLAY_H

#include <QtGui>
#include <QFrame>

#include "MasterClock.h"
#include "SynthRoute.h"

class SynthRoute;
class LCDWidget;
class MidiMessageLEDWidget;

class FloatingDisplay : public QFrame {
	Q_OBJECT

public:
	explicit FloatingDisplay(const QWidget *mainWindow);

	void saveSettings();
	void setSynthRoute(SynthRoute *synthRoute);

private:
	const QWidget * const mainWindow;
	SynthRoute *synthRoute;
	LCDWidget *lcdWidget;
	MidiMessageLEDWidget *midiMessageLED;
	QWidget *sizeGrip;
	int baseWindowWidth;

	enum {
		DRAG_STATUS_OFF,
		DRAG_STATUS_INITIATING,
		DRAG_STATUS_DRAGGING
	} dragStatus;

	QPoint dragOrigin;
	QPoint startWindowPosition;
	int startWindowWidth;
	bool resizing;
	MasterClockNanos startNanos;

	void closeEvent(QCloseEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void moveEvent(QMoveEvent *event);
	void resizeEvent(QResizeEvent *event);
	void showEvent(QShowEvent *event);

	void startDrag(const QPoint &mousePosition);
	void layoutWidgets(int targetWidth);
	void adjustGeometryToScreen();

private slots:
	void handleSynthStateChange(SynthState);
	void handleMidiMessageLEDUpdate(bool);
	void handleOpacitySliderValueChanged(int);
};

#endif
