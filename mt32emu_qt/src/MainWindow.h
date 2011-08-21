#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "SynthWidget.h"

#include "mididrv/MidiDriver.h"
#include "audiodrv/AudioDriver.h"

namespace Ui {
	class MainWindow;
}

class Master;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(Master *master, QWidget *parent = 0);
	~MainWindow();

protected:
	void closeEvent(QCloseEvent *);

private:
	Ui::MainWindow *ui;
	Master *master;
	MidiDriver *midiDriver;
	MidiDriver *testMidiDriver;
	QList<AudioDriver *> audioDrivers;

private slots:
	void on_actionAbout_triggered();
	void handleSynthRouteAdded(SynthRoute *synthRoute);
	void handleSynthRouteRemoved(SynthRoute *synthRoute);
	void on_actionOptions_triggered();
	void on_actionPlay_MIDI_file_triggered();
	void on_actionTest_MIDI_Driver_toggled(bool );
};

#endif // MAINWINDOW_H
