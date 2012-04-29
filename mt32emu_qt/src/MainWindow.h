#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>

#include "SynthWidget.h"

namespace Ui {
	class MainWindow;
}

class Master;
class MidiDriver;
class AudioFileWriter;
class MidiPlayerDialog;

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
	MidiDriver *testMidiDriver;
	AudioFileWriter *audioFileWriter;
	MidiPlayerDialog *midiPlayerDialog;

	void trayIconContextMenu();

private slots:
	void on_actionAbout_triggered();
	void handleSynthRouteAdded(SynthRoute *synthRoute, const AudioDevice *audioDevice);
	void handleSynthRouteRemoved(SynthRoute *synthRoute);
	void on_menuMIDI_aboutToShow();
	void on_actionNew_MIDI_port_triggered();
	void on_actionTest_MIDI_Driver_toggled(bool checked);
	void on_actionPlay_MIDI_file_triggered();
	void on_actionConvert_MIDI_to_Wave_triggered();
	void on_menuOptions_aboutToShow();
	void on_actionStart_iconized_toggled(bool checked);
	void on_actionShow_LCD_balloons_toggled(bool checked);
	void on_actionShow_connection_balloons_toggled(bool checked);
	void on_actionROM_Configuration_triggered();
	void refreshTabNames();
	void showHideMainWindow();
	void handleTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
	void toggleShowConsole();
};

#endif // MAINWINDOW_H
