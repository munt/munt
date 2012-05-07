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
	void connectSignals(bool enable);

private:
	SynthRoute *synthRoute;
	QSynth *qsynth;
	Ui::SynthWidget *ui;
	QLabel *partialStateLabel[32];
	QLabel *patchNameLabel[9];
	QLabel *polyStateLabel[9];

private slots:
	void handleReset();
	void handlePolyStateChanged(int partNum);
	void handlePartialStateChanged(int partialNum, int partialState);
	void handleProgramChanged(int partNum, QString patchName);
};

#endif
