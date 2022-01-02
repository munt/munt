#ifndef SYNTH_STATE_MONITOR_H
#define SYNTH_STATE_MONITOR_H

#include <QtCore>

#include <mt32emu/mt32emu.h>

#include "SynthWidget.h"

class SynthRoute;
class QLabel;

namespace Ui {
	class SynthWidget;
}

class LEDWidget : public QWidget {
	Q_OBJECT

public:
	explicit LEDWidget(const QColor *, QWidget *parent = 0);
	const QColor *color() const;
	void setColor(const QColor *);

protected:
	void paintEvent(QPaintEvent *);

private:
	const QColor *colorProperty;
};

class PartStateWidget : public QWidget {
	Q_OBJECT

public:
	explicit PartStateWidget(int partNum, const SynthStateMonitor &monitor, QWidget *parent = 0);

protected:
	void paintEvent(QPaintEvent *);

private:
	const int partNum;
	const SynthStateMonitor &monitor;
};

class LCDWidget : public QWidget {
	Q_OBJECT

public:
	explicit LCDWidget(QWidget *parent = NULL);

	void setSynthRoute(SynthRoute *synthRoute);
	bool updateDisplayText();

private:
	SynthRoute *synthRoute;
	const QPixmap lcdOffBackground;
	const QPixmap lcdOnBackground;
	char lcdText[21];

	int heightForWidth (int) const;
	void paintEvent(QPaintEvent *);
	void mousePressEvent(QMouseEvent *);

private slots:
	void handleLCDUpdate();
};

class SynthStateMonitor : public QObject {
	Q_OBJECT

friend class PartStateWidget;

public:
	SynthStateMonitor(Ui::SynthWidget *ui, SynthRoute *useSynthRoute);
	~SynthStateMonitor();
	void enableMonitor(bool enable);

private:
	SynthRoute * const synthRoute;

	const Ui::SynthWidget * const ui;
	LCDWidget lcdWidget;
	LEDWidget midiMessageLED;
	LEDWidget **partialStateLED;
	QLabel *patchNameLabel[9];
	PartStateWidget *partStateWidget[9];

	MT32Emu::PartialState *partialStates;
	MT32Emu::Bit8u *keysOfPlayingNotes;
	MT32Emu::Bit8u *velocitiesOfPlayingNotes;

	uint partialCount;

	void allocatePartialsData();
	void freePartialsData();

private slots:
	void handleSynthStateChange(SynthState);
	void handlePolyStateChanged(int partNum);
	void handleProgramChanged(int partNum, QString soundGroupName, QString patchName);
	void handleMidiMessageLEDUpdate(bool);
	void handleAudioBlockRendered();
};

#endif
