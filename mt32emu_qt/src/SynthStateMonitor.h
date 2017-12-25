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

friend class SynthStateMonitor;

public:
	explicit LCDWidget(const SynthStateMonitor &monitor, QWidget *parent = 0);
	void reset();

protected:
	void paintEvent(QPaintEvent *);

private:
	enum LCDState {
		DISPLAYING_PART_STATE,
		DISPLAYING_TIMBRE_NAME,
		DISPLAYING_MESSAGE
	};

	const SynthStateMonitor &monitor;
	const QPixmap lcdOffBackground;
	const QPixmap lcdOnBackground;

	QByteArray lcdText;
	LCDState lcdState;
	MasterClockNanos lcdStateStartNanos;
	bool maskedChar[20];
	int masterVolume;

	void setPartStateLCDText();
	void setProgramChangeLCDText(int partNum, QString bankName, QString timbreName);

private slots:
	void handleLCDMessageDisplayed(const QString text);
	void handleMasterVolumeChanged(int volume);
};

class SynthStateMonitor : public QObject {
	Q_OBJECT

friend class PartStateWidget;
friend class LCDWidget;

public:
	SynthStateMonitor(Ui::SynthWidget *ui, SynthRoute *useSynthRoute);
	~SynthStateMonitor();
	void enableMonitor(bool enable);

private:
	const SynthRoute * const synthRoute;

	const Ui::SynthWidget * const ui;
	LCDWidget lcdWidget;
	LEDWidget midiMessageLED;
	LEDWidget **partialStateLED;
	QLabel *patchNameLabel[9];
	PartStateWidget *partStateWidget[9];

	MT32Emu::PartialState *partialStates;
	MT32Emu::Bit8u *keysOfPlayingNotes;
	MT32Emu::Bit8u *velocitiesOfPlayingNotes;

	MasterClockNanos midiMessageLEDStartNanos;
	MasterClockNanos previousUpdateNanos;
	bool enabled;
	uint partialCount;

	void allocatePartialsData();
	void freePartialsData();

private slots:
	void handleUpdate();
	void handleSynthStateChange(SynthState);
	void handleMIDIMessagePlayed();
	void handlePolyStateChanged(int partNum);
	void handleProgramChanged(int partNum, QString soundGroupName, QString patchName);
};

#endif
