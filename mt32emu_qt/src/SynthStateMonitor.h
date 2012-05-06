#ifndef SYNTH_STATE_MONITOR_H
#define SYNTH_STATE_MONITOR_H

#include <QtCore>

#include "SynthWidget.h"

class SynthRoute;
namespace Ui {
	class SynthWidget;
}

class SynthStateMonitor : public QObject {
	Q_OBJECT

public:
	SynthStateMonitor(Ui::SynthWidget *ui, SynthRoute *useSynthRoute);
	~SynthStateMonitor();

private:
	SynthRoute *synthRoute;
	QSynth *qsynth;
	Ui::SynthWidget *ui;
	QColor deadPartialLEDColor;
	QColor attackPartialLEDColor;
	QColor sustainPartialLEDColor;
	QColor releasePartialLEDColor;
	int partialState[32];
	QLabel *partialStateLabel[32];
	QLabel *patchNameLabel[9];
	QLabel *polyStateLabel[9];

private slots:
	void handleReset();
	void handlePolyStateChanged(int partNum);
	void handlePartialStateChanged(int partialNum, int partialPhase);
	void handleProgramChanged(int partNum, QString patchName);
};

#endif
