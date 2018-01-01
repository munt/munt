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
class MidiConverterDialog;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(Master *master, QWidget *parent = 0);
	~MainWindow();

protected:
	void closeEvent(QCloseEvent *);
	void dragEnterEvent(QDragEnterEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dropEvent(QDropEvent *event);

private:
	Ui::MainWindow *ui;
	Master *master;
	MidiDriver *testMidiDriver;
	AudioFileWriter *audioFileWriter;
	MidiPlayerDialog *midiPlayerDialog;
	MidiConverterDialog *midiConverterDialog;

	bool showROMSelectionDialog();
	void trayIconContextMenu();

private slots:
	void on_actionExit_triggered();
	void on_actionAbout_triggered();
	void handleSynthRouteAdded(SynthRoute *synthRoute, const AudioDevice *audioDevice);
	void handleSynthRouteRemoved(SynthRoute *synthRoute);
	void handleROMSLoadFailed(bool &recoveryAttempted);
	void on_menuMIDI_aboutToShow();
	void on_actionNew_MIDI_port_triggered();
	void on_actionTest_MIDI_Driver_toggled(bool checked);
	void on_actionPlay_MIDI_file_triggered();
	void on_actionConvert_MIDI_to_Wave_triggered();
	void on_menuOptions_aboutToShow();
	void on_actionStart_iconized_toggled(bool checked);
	void on_actionHide_to_tray_on_close_toggled(bool checked);
	void on_actionShow_LCD_balloons_toggled(bool checked);
	void on_actionShow_connection_balloons_toggled(bool checked);
	void on_actionROM_Configuration_triggered();
	void refreshTabNames();
	void showHideMainWindow();
	void handleTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
	void toggleShowConsole();
	void handlePlayMidiFiles(const QStringList &fileList);
	void handleConvertMidiFiles(const QStringList &fileList);
};

#endif // MAINWINDOW_H
