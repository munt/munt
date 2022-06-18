/* Copyright (C) 2011-2022 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtGui>
#include <QMessageBox>
#include <QFileDialog>

#ifdef WITH_WINCONSOLE
#if _WIN32_WINNT < 0x0500
#undef WITH_WINCONSOLE
#else // _WIN32_WINNT < 0x0500
#include <windows.h>
#endif // _WIN32_WINNT < 0x0500
#endif // defined(WITH_WINCONSOLE)

#ifndef BUILD_DATE
#ifdef __DATE__
#define BUILD_DATE __DATE__
#else
#define BUILD_DATE "Unknown"
#endif
#endif

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "Master.h"

#include "AudioFileWriter.h"
#include "mididrv/TestDriver.h"
#include "MidiPlayerDialog.h"
#include "MidiConverterDialog.h"
#include "FloatingDisplay.h"
#include "DemoPlayer.h"

enum FloatingDisplayVisibility {
	FloatingDisplayVisibility_DEFAULT,
	FloatingDisplayVisibility_ALWAYS_HIDDEN,
	FloatingDisplayVisibility_ALWAYS_SHOWN
};

FloatingDisplayVisibility getFloatingDisplayVisibility() {
	return FloatingDisplayVisibility(Master::getInstance()->getSettings()->value("FloatingDisplay/visibility",
		FloatingDisplayVisibility_DEFAULT).toInt());
}

MainWindow::MainWindow(Master *master) :
	QMainWindow(),
	ui(new Ui::MainWindow),
	master(master),
	testMidiDriver(NULL),
	audioFileWriter(NULL),
	midiPlayerDialog(NULL),
	midiConverterDialog(NULL),
	floatingDisplay(NULL),
	demoPlayer(NULL)
{
	ui->setupUi(this);
	connect(master, SIGNAL(synthRouteAdded(SynthRoute *, const AudioDevice *, bool)), SLOT(handleSynthRouteAdded(SynthRoute *, const AudioDevice *, bool)));
	connect(master, SIGNAL(synthRouteRemoved(SynthRoute *)), SLOT(handleSynthRouteRemoved(SynthRoute *)));
	connect(master, SIGNAL(synthRoutePinned()), SLOT(refreshTabNames()));
	connect(master, SIGNAL(romsLoadFailed(bool &)), SLOT(handleROMSLoadFailed(bool &)), Qt::DirectConnection);
	connect(master, SIGNAL(playMidiFiles(const QStringList &)), SLOT(handlePlayMidiFiles(const QStringList &)), Qt::QueuedConnection);
	connect(master, SIGNAL(convertMidiFiles(const QStringList &)), SLOT(handleConvertMidiFiles(const QStringList &)), Qt::QueuedConnection);
	connect(master, SIGNAL(mainWindowTitleUpdated(const QString &)), SLOT(setWindowTitle(const QString &)));
	connect(master, SIGNAL(maxSessionsFinished()), SLOT(on_actionExit_triggered()));

	if (master->getTrayIcon() != NULL) {
		connect(master->getTrayIcon(), SIGNAL(activated(QSystemTrayIcon::ActivationReason)), SLOT(handleTrayIconActivated(QSystemTrayIcon::ActivationReason)));
		trayIconContextMenu();
	}

	setAcceptDrops(true);

	QSettings *settings = master->getSettings();
	QRect rect = settings->value("Master/mainWindowGeometry", geometry()).toRect();
	if (rect != geometry()) {
		setGeometry(rect);
	}

#ifdef WITH_WINCONSOLE
	if (!settings->value("Master/showConsole", false).toBool())
		ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif

#ifdef WITH_JACK_MIDI_DRIVER
	ui->actionNew_JACK_MIDI_port->setVisible(true);
	ui->actionNew_exclusive_JACK_MIDI_port->setVisible(true);
#endif

	QActionGroup *floatingDisplayGroup = new QActionGroup(this);
	ui->actionFloating_display_Default_visibility->setData(FloatingDisplayVisibility_DEFAULT);
	ui->actionFloating_display_Always_shown->setData(FloatingDisplayVisibility_ALWAYS_SHOWN);
	ui->actionFloating_display_Never_shown->setData(FloatingDisplayVisibility_ALWAYS_HIDDEN);
	floatingDisplayGroup->addAction(ui->actionFloating_display_Default_visibility);
	floatingDisplayGroup->addAction(ui->actionFloating_display_Always_shown);
	floatingDisplayGroup->addAction(ui->actionFloating_display_Never_shown);
	connect(floatingDisplayGroup, SIGNAL(triggered(QAction *)), SLOT(handleFloatingDisplayVisibilityChanged(QAction *)));
}

MainWindow::~MainWindow() {
	delete floatingDisplay;
	delete ui;
}

void MainWindow::updateFloatingDisplayVisibility() {
	FloatingDisplayVisibility visibility = getFloatingDisplayVisibility();
	bool visible = visibility == FloatingDisplayVisibility_ALWAYS_SHOWN
		|| !(visibility == FloatingDisplayVisibility_ALWAYS_HIDDEN || isVisible());
	if (floatingDisplay == NULL) {
		if (visible) showFloatingDisplay();
	} else {
		floatingDisplay->setVisible(visible);
	}
}

void MainWindow::showFloatingDisplay() {
	if (floatingDisplay == NULL) {
		floatingDisplay = new FloatingDisplay(this);
		syncFloatingDisplay(ui->synthTabs->currentIndex());
	}
	floatingDisplay->setVisible(true);
}

void MainWindow::syncFloatingDisplay(int currentSynthTabsIndex) {
	if(currentSynthTabsIndex < 0) {
		floatingDisplay->setSynthRoute(NULL);
	} else {
		SynthWidget *synthWidget = (SynthWidget *)ui->synthTabs->widget(currentSynthTabsIndex);
		floatingDisplay->setSynthRoute(synthWidget->getSynthRoute());
	}
}

void MainWindow::showEvent(QShowEvent *) {
	updateFloatingDisplayVisibility();
}

void MainWindow::hideEvent(QHideEvent *) {
	updateFloatingDisplayVisibility();
}

void MainWindow::closeEvent(QCloseEvent *event) {
	QSettings *settings = Master::getInstance()->getSettings();
	if (settings->value("Master/hideToTrayOnClose", false).toBool()) {
		event->ignore();
		hide();
	} else {
		event->accept();
		on_actionExit_triggered();
	}
}

void MainWindow::on_actionExit_triggered() {
	QSettings *settings = Master::getInstance()->getSettings();
	settings->setValue("Master/mainWindowGeometry", geometry());
	if (demoPlayer != NULL) {
		delete demoPlayer;
		demoPlayer = NULL;
	}
	if (floatingDisplay != NULL) {
		floatingDisplay->saveSettings();
	}
	if (testMidiDriver != NULL) {
		delete testMidiDriver;
		testMidiDriver = NULL;
	}
	if (audioFileWriter != NULL) {
		delete audioFileWriter;
		audioFileWriter = NULL;
	}
	if (midiPlayerDialog != NULL) {
		delete midiPlayerDialog;
		midiPlayerDialog = NULL;
	}
	if (midiConverterDialog != NULL) {
		delete midiConverterDialog;
		midiConverterDialog = NULL;
	}
	QApplication::exit();
}

void MainWindow::on_actionAbout_triggered() {
#ifdef BUILD_MT32EMU_VERSION
	QString mt32emuVersion = BUILD_MT32EMU_VERSION;
#else
	QString mt32emuVersion = MT32Emu::Synth::getLibraryVersionString();
#endif
	QMessageBox::about(this, "About",
		"Munt - Roland (R) MT-32 sound module emulator\n"
		"\n"
		"Munt mt32emu_qt GUI Application Version " MT32EMU_QT_VERSION "\n"
		"Munt Library Version " + mt32emuVersion + "\n"
		"Qt Library Version " + qVersion() + "\n"
		"\n"
		"Build Arch: " BUILD_SYSTEM " " + QString::number(QSysInfo::WordSize) + "-bit\n"
		"Build Date: " BUILD_DATE "\n"
		"\n"
		"Copyright (C) 2011-2022 Jerome Fisher, Sergey V. Mikayev\n"
		"\n"
		"Licensed under GPL v3 or any later version."
	);
}

void MainWindow::on_actionCommandLineHelp_triggered() {
	Master::showCommandLineHelp();
}

void MainWindow::refreshTabNames()
{
	QWidget *widget;
	for(int i = 0;; i++) {
		widget = ui->synthTabs->widget(i);
		if (widget == NULL) return;
		QString tabName = QString("Synth &%1").arg(i + 1);
		if (master->isPinned(((SynthWidget *)widget)->getSynthRoute())) tabName = tabName + " *";
		ui->synthTabs->setTabText(i, tabName);
	}
}

void MainWindow::handleSynthRouteAdded(SynthRoute *synthRoute, const AudioDevice *audioDevice, bool pinnable) {
	SynthWidget *synthWidget = new SynthWidget(master, synthRoute, pinnable, audioDevice, this);
	int newTabIx = ui->synthTabs->count();
	ui->synthTabs->addTab(synthWidget, QString("Synth &%1").arg(ui->synthTabs->count() + 1));
	ui->synthTabs->setCurrentIndex(newTabIx);
}

void MainWindow::handleSynthRouteRemoved(SynthRoute *synthRoute) {
	QWidget *widget;
	for(int i = 0;; i++) {
		widget = ui->synthTabs->widget(i);
		if(widget == NULL) {
			qDebug() << "Couldn't find widget for removed synth";
			return;
		}
		SynthWidget *synthWidget = (SynthWidget *)widget;
		if(synthRoute == synthWidget->getSynthRoute()) {
			ui->synthTabs->removeTab(i);
			delete synthWidget;
			refreshTabNames();
			return;
		}
	}
}

void MainWindow::handleROMSLoadFailed(bool &recoveryAttempted) {
	recoveryAttempted = showROMSelectionDialog();
}

void MainWindow::on_menuTools_aboutToShow() {
	ui->actionNew_MIDI_port->setEnabled(master->canCreateMidiPort());
}

void MainWindow::on_actionNew_MIDI_port_triggered() {
	MidiPropertiesDialog mpd(this);
	master->configureMidiPropertiesDialog(mpd);
	master->createMidiPort(mpd);
}

void MainWindow::on_actionTest_MIDI_Driver_toggled(bool checked) {
	bool running = testMidiDriver != NULL;
	if (running != checked) {
		if (running) {
			testMidiDriver->stop();
			delete testMidiDriver;
			testMidiDriver = NULL;
		} else {
			testMidiDriver = new TestMidiDriver(master);
			testMidiDriver->start();
		}
	}
}

void MainWindow::on_actionPlay_MIDI_file_triggered() {
	if (midiPlayerDialog == NULL) {
		midiPlayerDialog = new MidiPlayerDialog(master, this);
	}
	midiPlayerDialog->setVisible(true);
	midiPlayerDialog->activateWindow();
}

void MainWindow::on_actionConvert_MIDI_to_Wave_triggered() {
	if (midiConverterDialog == NULL) {
		midiConverterDialog = new MidiConverterDialog(master, this);
	}
	midiConverterDialog->setVisible(true);
	midiConverterDialog->activateWindow();
}

void MainWindow::on_menuPlay_Demo_Songs_aboutToShow() {
	if (demoPlayer == NULL) {
		const MT32Emu::ROMImage *romImage = DemoPlayer::findSuitableROM(master);
		if (romImage == NULL) return;
		demoPlayer = new DemoPlayer(master, romImage);
	}
	ui->menuPlay_Demo_Songs->clear();
	QActionGroup *group = new QActionGroup(this);
	group->setExclusive(false);
	connect(group, SIGNAL(triggered(QAction *)), SLOT(handleDemoPlay(QAction *)));
	group->addAction(ui->menuPlay_Demo_Songs->addAction("&Chain Play"))->setData(-1);
	group->addAction(ui->menuPlay_Demo_Songs->addAction("&Random Play"))->setData(-2);
	ui->menuPlay_Demo_Songs->addSeparator();
	const QStringList demoSongs = demoPlayer->getDemoSongs();
	for (int demoSongIx = 0; demoSongIx < demoSongs.size(); demoSongIx++) {
		QString actionName = '&' + QString().setNum(demoSongIx + 1) + ": " + demoSongs.at(demoSongIx);
		group->addAction(ui->menuPlay_Demo_Songs->addAction(actionName))->setData(demoSongIx);
	}
	ui->menuPlay_Demo_Songs->addSeparator();
	group->addAction(ui->menuPlay_Demo_Songs->addAction("&Stop Playback"))->setData(-3);
}

void MainWindow::on_actionSuitable_ROMs_unavailable_triggered() {
	QMessageBox::information(this, "Info", "Demo songs are present in the new-gen MT-32 ROMs only.\n"
		"None of the available Synth profiles is configured with such a model.");
}

void MainWindow::handleDemoPlay(QAction *action) {
	if (demoPlayer == NULL) return;
	int demoSongIx = action->data().toInt();
	switch (demoSongIx) {
	case -1:
		demoPlayer->chainPlay();
		break;
	case -2:
		demoPlayer->randomPlay();
		break;
	case -3:
		demoPlayer->stop();
		break;
	default:
		demoPlayer->playSong(demoSongIx);
		break;
	}
}

void MainWindow::on_menuOptions_aboutToShow() {
	QSettings *settings = master->getSettings();
	ui->actionStart_iconized->setChecked(settings->value("Master/startIconized", false).toBool());
	ui->actionHide_to_tray_on_close->setChecked(settings->value("Master/hideToTrayOnClose", false).toBool());
	ui->actionShow_LCD_balloons->setChecked(settings->value("Master/showLCDBalloons", true).toBool());
	ui->actionShow_connection_balloons->setChecked(settings->value("Master/showConnectionBalloons", true).toBool());
	QFileDialog::Options qFileDialogOptions = QFileDialog::Options(settings->value("Master/qFileDialogOptions", 0).toInt());
	ui->actionShow_native_file_dialog->setChecked(!qFileDialogOptions.testFlag(QFileDialog::DontUseNativeDialog));
}

void MainWindow::on_actionStart_iconized_toggled(bool checked) {
	master->getSettings()->setValue("Master/startIconized", checked);
}

void MainWindow::on_actionHide_to_tray_on_close_toggled(bool checked) {
	master->getSettings()->setValue("Master/hideToTrayOnClose", checked);
}

void MainWindow::on_actionShow_LCD_balloons_toggled(bool checked) {
	master->getSettings()->setValue("Master/showLCDBalloons", checked);
}

void MainWindow::on_actionShow_connection_balloons_toggled(bool checked) {
	master->getSettings()->setValue("Master/showConnectionBalloons", checked);
}

void MainWindow::on_actionShow_native_file_dialog_toggled(bool checked) {
	QSettings *settings = master->getSettings();
	QFileDialog::Options qFileDialogOptions = QFileDialog::Options(settings->value("Master/qFileDialogOptions", 0).toInt());
	qFileDialogOptions |= QFileDialog::DontUseNativeDialog;
	if (checked) {
		qFileDialogOptions ^= QFileDialog::DontUseNativeDialog;
	}
	settings->setValue("Master/qFileDialogOptions", int(qFileDialogOptions));
}

void MainWindow::on_menuFloating_Display_aboutToShow() {
	switch (getFloatingDisplayVisibility()) {
	case FloatingDisplayVisibility_ALWAYS_SHOWN:
		ui->actionFloating_display_Always_shown->setChecked(true);
		break;
	case FloatingDisplayVisibility_ALWAYS_HIDDEN:
		ui->actionFloating_display_Never_shown->setChecked(true);
		break;
	default:
		ui->actionFloating_display_Default_visibility->setChecked(true);
		break;
	};
	bool bypassWindowManager = Master::getInstance()->getSettings()->value("FloatingDisplay/bypassWindowManager", true).toBool();
	ui->actionFloating_display_Bypass_window_manager->setChecked(bypassWindowManager);
}

void MainWindow::handleFloatingDisplayVisibilityChanged(QAction *triggeredAction) {
	Master::getInstance()->getSettings()->setValue("FloatingDisplay/visibility", triggeredAction->data());
	updateFloatingDisplayVisibility();
}

void MainWindow::on_actionFloating_display_Bypass_window_manager_toggled(bool checked) {
	Master::getInstance()->getSettings()->setValue("FloatingDisplay/bypassWindowManager", checked);
}

void MainWindow::on_synthTabs_currentChanged(int index) {
	if (floatingDisplay != NULL) syncFloatingDisplay(index);
}

void MainWindow::on_actionROM_Configuration_triggered() {
	showROMSelectionDialog();
}

bool MainWindow::showROMSelectionDialog() {
	Master &master = *Master::getInstance();
	SynthProfile synthProfile;
	master.loadSynthProfile(synthProfile, "");
	ROMSelectionDialog rsd(synthProfile, this);
	rsd.loadROMInfos();
	if (rsd.exec() == QDialog::Accepted) {
		master.storeSynthProfile(synthProfile, "");
		return true;
	}
	return false;
}

void MainWindow::trayIconContextMenu() {
	QMenu *menu = new QMenu(this);

	trayIconMenuShowHideMainWindow = menu->addAction("Show/Hide Main Window", this, SLOT(showHideMainWindow()));
	QFont boldFont(trayIconMenuShowHideMainWindow->font());
	boldFont.setBold(true);
	trayIconMenuShowHideMainWindow->setFont(boldFont);

	trayIconMenuShowHideFloatingDisplay = menu->addAction("Show/Hide Floating Display", this, SLOT(showHideFloatingDisplay()));

	menu->addAction("Show MIDI Player", this, SLOT(on_actionPlay_MIDI_file_triggered()));

	menu->addAction(ui->actionStart_iconized);
	ui->actionStart_iconized->setChecked(master->getSettings()->value("Master/startIconized", false).toBool());

#ifdef WITH_WINCONSOLE
	QAction *a = menu->addAction("Show console", this, SLOT(toggleShowConsole()));
	a->setCheckable(true);
	a->setChecked(master->getSettings()->value("Master/showConsole", false).toBool());
#endif

	menu->addAction("Exit", this, SLOT(on_actionExit_triggered()));

	connect(menu, SIGNAL(aboutToShow()), SLOT(handleTrayIconMenuAboutToShow()));
	master->getTrayIcon()->setContextMenu(menu);
}

void MainWindow::toggleShowConsole() {
#ifdef WITH_WINCONSOLE
	QSettings *s = master->getSettings();
	bool b = !s->value("Master/showConsole", false).toBool();
	s->setValue("Master/showConsole", b);
	ShowWindow(GetConsoleWindow(), b ? SW_NORMAL : SW_HIDE);
#endif
}

void MainWindow::showHideMainWindow() {
	setVisible(!isVisible());
	if (isVisible()) activateWindow();
}

void MainWindow::showHideFloatingDisplay() {
	if (floatingDisplay != NULL) {
		floatingDisplay->setVisible(!floatingDisplay->isVisible());
	} else {
		showFloatingDisplay();
	}
}

void MainWindow::handleTrayIconMenuAboutToShow() {
	trayIconMenuShowHideMainWindow->setText(QString(isVisible() ? "Hide" : "Show") + " Main Window");
	trayIconMenuShowHideFloatingDisplay->setText(
		QString(floatingDisplay != NULL && floatingDisplay->isVisible() ? "Hide" : "Show") + " Floating Display"
	);
}

void MainWindow::handleTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
	switch (reason) {
	case QSystemTrayIcon::Context:
		break;
	case QSystemTrayIcon::DoubleClick:
		// Fall-through until implemented
	case QSystemTrayIcon::Trigger:
		// Fall-through until implemented
	default:
		showHideMainWindow();
		break;
	}
}

void MainWindow::handlePlayMidiFiles(const QStringList &fileList) {
	qDebug() << "Playing:" << fileList;
	on_actionPlay_MIDI_file_triggered();
	midiPlayerDialog->startPlayingFiles(fileList);
}

void MainWindow::handleConvertMidiFiles(const QStringList &fileList) {
	qDebug() << "Converting:" << fileList;
	on_actionConvert_MIDI_to_Wave_triggered();
	connect(midiConverterDialog, SIGNAL(batchConversionFinished()), SLOT(on_actionExit_triggered()));
	midiConverterDialog->startConversion(fileList);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e) {
	Master::isSupportedDropEvent(e);
}

void MainWindow::dragMoveEvent(QDragMoveEvent *e) {
	Master::isSupportedDropEvent(e);
}

void MainWindow::dropEvent(QDropEvent *e) {
	Master::isSupportedDropEvent(e);
	if (e->isAccepted()) {
		on_actionPlay_MIDI_file_triggered();
		midiPlayerDialog->dropEvent(e);
	}
}

#ifdef WITH_JACK_MIDI_DRIVER
void MainWindow::on_actionNew_JACK_MIDI_port_triggered() {
	if (master->createJACKMidiPort(false)) return;
	QMessageBox::warning(this, "Error", "Failed to create JACK MIDI port");
}

void MainWindow::on_actionNew_exclusive_JACK_MIDI_port_triggered() {
	if (master->createJACKMidiPort(true)) return;
	QMessageBox::warning(this, "Error", "Failed to create JACK MIDI port");
}
#endif
