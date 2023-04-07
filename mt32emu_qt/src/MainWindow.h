#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>

namespace Ui {
	class MainWindow;
}

class Master;
class MidiDriver;
class AudioDevice;
class AudioFileWriter;
class SynthRoute;
class MidiPlayerDialog;
class MidiConverterDialog;
class FloatingDisplay;
class DemoPlayer;

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(Master *master);
	~MainWindow();

	void updateFloatingDisplayVisibility();

protected:
	void showEvent(QShowEvent *event);
	void hideEvent(QHideEvent *event);
	void closeEvent(QCloseEvent *event);
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
	FloatingDisplay *floatingDisplay;
	DemoPlayer *demoPlayer;
	QAction *trayIconMenuShowHideMainWindow;
	QAction *trayIconMenuShowHideFloatingDisplay;

	void showFloatingDisplay();
	void syncFloatingDisplay(int currentSynthTabsIndex);
	bool showROMSelectionDialog();
	void trayIconContextMenu();

private slots:
	void on_actionExit_triggered();
	void on_actionAbout_triggered();
	void on_actionCommandLineHelp_triggered();
	void handleSynthRouteAdded(SynthRoute *synthRoute, const AudioDevice *audioDevice, bool pinnable);
	void handleSynthRouteRemoved(SynthRoute *synthRoute);
	void handleROMSLoadFailed(bool &recoveryAttempted);
	void on_menuTools_aboutToShow();
	void on_actionNew_MIDI_port_triggered();
	void on_actionTest_MIDI_Driver_toggled(bool checked);
	void on_actionPlay_MIDI_file_triggered();
	void on_actionConvert_MIDI_to_Wave_triggered();
	void on_menuPlay_Demo_Songs_aboutToShow();
	void on_actionSuitable_ROMs_unavailable_triggered();
	void handleDemoPlay(QAction *);
	void on_menuOptions_aboutToShow();
	void on_actionStart_iconized_toggled(bool checked);
	void on_actionHide_to_tray_on_close_toggled(bool checked);
	void on_actionShow_LCD_balloons_toggled(bool checked);
	void on_actionShow_connection_balloons_toggled(bool checked);
	void on_actionShow_native_file_dialog_toggled(bool checked);
	void on_menuFloating_Display_aboutToShow();
	void handleFloatingDisplayVisibilityChanged(QAction *triggeredAction);
	void on_actionFloating_display_Bypass_window_manager_toggled(bool checked);
	void on_synthTabs_currentChanged(int index);
	void on_actionROM_Configuration_triggered();
	void refreshTabNames();
	void showHideMainWindow();
	void showHideFloatingDisplay();
	void handleTrayIconMenuAboutToShow();
	void handleTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
	void toggleShowConsole();
	void handlePlayMidiFiles(const QStringList &fileList);
	void handleConvertMidiFiles(const QStringList &fileList);

#ifdef WITH_JACK_MIDI_DRIVER
private slots:
	void on_actionNew_JACK_MIDI_port_triggered();
	void on_actionNew_exclusive_JACK_MIDI_port_triggered();
#endif
};

#endif // MAINWINDOW_H
