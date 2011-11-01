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
	void trayIconContextMenu();

private slots:
	void on_actionAbout_triggered();
	void handleSynthRouteAdded(SynthRoute *synthRoute, const AudioDevice *audioDevice);
	void handleSynthRouteRemoved(SynthRoute *synthRoute);
	void on_actionOptions_triggered();
	void on_actionPlay_MIDI_file_triggered();
	void on_actionTest_MIDI_Driver_toggled(bool );
	void showHideMainWindow();
	void toggleStartIconized();
	void handleTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
	void toggleShowConsole();
	void handleSynthRoutePinned();

signals:
	void synthRoutePinned();
};

#endif // MAINWINDOW_H
