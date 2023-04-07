#ifndef SYNTH_STATE_MONITOR_H
#define SYNTH_STATE_MONITOR_H

#include <QtGui>
#include <QAbstractButton>
#include <QWidget>

#include <mt32emu/mt32emu.h>

#include "QSynth.h"

class SynthRoute;
class SynthStateMonitor;

namespace Ui {
	class SynthWidget;
}

class LEDWidget : public QWidget {
	Q_OBJECT

protected:
	explicit LEDWidget(QWidget *parent, const QColor *initialColor = NULL);

	const QColor *color() const;
	void setColor(const QColor *useColor);
	void paintEvent(QPaintEvent *);

private:
	const QColor *colorProperty;
};

class MidiMessageLEDWidget : public LEDWidget {
	Q_OBJECT

public:
	explicit MidiMessageLEDWidget(QWidget *parent);

	void setState(bool state);
};

class PartialStateLEDWidget : public LEDWidget {
	Q_OBJECT

public:
	explicit PartialStateLEDWidget(QWidget *parent);

	void setState(MT32Emu::PartialState state);
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

	QSize sizeHint() const;
	QSize minimumSizeHint() const;
	int heightForWidth(int) const;

protected:
	void paintEvent(QPaintEvent *);
	void mousePressEvent(QMouseEvent *);

private:
	SynthRoute *synthRoute;
	const QPixmap lcdOffBackground;
	const QPixmap lcdOnBackground;
	char lcdText[21];

private slots:
	void handleLCDUpdate();
};

class PartVolumeButton : public QAbstractButton {
	Q_OBJECT

public:
	explicit PartVolumeButton(QWidget *parent, const SynthStateMonitor &monitor, int partNum);

private:
	const SynthStateMonitor &monitor;
	const int partNum;
	// Non-positive values mean "part muted". Values above 100 imply "no volume override".
	int volume;

	void paintEvent(QPaintEvent *);
	void contextMenuEvent (QContextMenuEvent *);
	void toggleMutePart();
	void mutePart();
	void unmutePart();
	void toggleSoloPart(bool enabled);

private slots:
	void handleClicked();
	void handleVolumeChanged(int);
	void handleResetVolumeTriggered();
	void handleSoloTriggered();
	void handleUnmuteAllTriggered();
	void handleResetAllTriggered();
};

class PatchNameButton : public QAbstractButton {
	Q_OBJECT

public:
	explicit PatchNameButton(QWidget *parent, SynthRoute &synthRoute, int partNum);

private:
	SynthRoute &synthRoute;
	const int partNumber;

	QSize sizeHint() const;
	void paintEvent(QPaintEvent *);

private slots:
	void handleClicked();
};

class SynthStateMonitor : public QObject {
	Q_OBJECT

friend class PartStateWidget;
friend class PartVolumeButton;

public:
	SynthStateMonitor(Ui::SynthWidget *ui, SynthRoute *useSynthRoute);
	~SynthStateMonitor();

	void enableMonitor(bool enable);

private:
	SynthRoute * const synthRoute;

	const Ui::SynthWidget * const ui;
	LCDWidget lcdWidget;
	MidiMessageLEDWidget midiMessageLED;
	PartialStateLEDWidget **partialStateLED;
	PartVolumeButton *partVolumeButton[9];
	PatchNameButton *patchNameButton[9];
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
