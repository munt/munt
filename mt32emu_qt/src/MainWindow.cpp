/* Copyright (C) 2011-2018 Jerome Fisher, Sergey V. Mikayev
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

MainWindow::MainWindow(Master *master, QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	master(master),
	testMidiDriver(NULL),
	audioFileWriter(NULL),
	midiPlayerDialog(NULL),
	midiConverterDialog(NULL)
{
	ui->setupUi(this);
	connect(master, SIGNAL(synthRouteAdded(SynthRoute *, const AudioDevice *)), SLOT(handleSynthRouteAdded(SynthRoute *, const AudioDevice *)));
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

	QSettings *settings = Master::getInstance()->getSettings();
	QRect rect = settings->value("Master/mainWindowGeometry", geometry()).toRect();
	if (rect != geometry()) {
		setGeometry(rect);
	}

#ifdef WITH_WINCONSOLE
	if (!master->getSettings()->value("Master/showConsole", false).toBool())
		ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
}

MainWindow::~MainWindow()
{
	delete ui;
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
	QApplication::quit();
}

void MainWindow::on_actionAbout_triggered()
{
	QMessageBox::about(this, "About",
		"Munt - Roland (R) MT-32 sound module emulator\n"
		"\n"
		"Munt mt32emu_qt GUI Application Version " APP_VERSION "\n"
		"Munt Library Version " + QString(MT32Emu::Synth::getLibraryVersionString()) + "\n"
		"Qt Library Version " + qVersion() + "\n"
		"\n"
		"Build Arch: " BUILD_SYSTEM " " + QString::number(QSysInfo::WordSize) + "-bit\n"
		"Build Date: " BUILD_DATE "\n"
		"\n"
		"Copyright (C) 2011-2018 Jerome Fisher, Sergey V. Mikayev\n"
		"\n"
		"Licensed under GPL v3 or any later version."
	);
}

void MainWindow::refreshTabNames()
{
	QWidget *widget;
	QString tabName;
	for(int i = 0;; i++) {
		widget = ui->synthTabs->widget(i);
		if (widget == NULL) return;
		tabName.sprintf("Synth &%i", i + 1);
		if (master->isPinned(((SynthWidget *)widget)->getSynthRoute())) tabName = tabName + " *";
		ui->synthTabs->setTabText(i, tabName);
	}
}

void MainWindow::handleSynthRouteAdded(SynthRoute *synthRoute, const AudioDevice *audioDevice) {
	SynthWidget *synthWidget = new SynthWidget(master, synthRoute, audioDevice, this);
	int newTabIx = ui->synthTabs->count();
	ui->synthTabs->addTab(synthWidget, QString().sprintf("Synth &%i", ui->synthTabs->count() + 1));
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

void MainWindow::on_menuMIDI_aboutToShow() {
	ui->actionNew_MIDI_port->setEnabled(master->canCreateMidiPort());
}

void MainWindow::on_actionNew_MIDI_port_triggered() {
	MidiPropertiesDialog mpd(this);
	master->createMidiPort(&mpd);
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

void MainWindow::on_menuOptions_aboutToShow() {
	ui->actionStart_iconized->setChecked(master->getSettings()->value("Master/startIconized", false).toBool());
	ui->actionHide_to_tray_on_close->setChecked(master->getSettings()->value("Master/hideToTrayOnClose", false).toBool());
	ui->actionShow_LCD_balloons->setChecked(master->getSettings()->value("Master/showLCDBalloons", true).toBool());
	ui->actionShow_connection_balloons->setChecked(master->getSettings()->value("Master/showConnectionBalloons", true).toBool());
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
	QFont bold;
	bold.setBold(true);
	menu->addAction("Show/Hide", this, SLOT(showHideMainWindow()))->setFont(bold);
	menu->addAction("Show MIDI Player", this, SLOT(on_actionPlay_MIDI_file_triggered()));
	menu->addAction(ui->actionStart_iconized);
	ui->actionStart_iconized->setChecked(master->getSettings()->value("Master/startIconized", "0").toBool());
#ifdef WITH_WINCONSOLE
	QAction *a = menu->addAction("Show console", this, SLOT(toggleShowConsole()));
	a->setCheckable(true);
	a->setChecked(master->getSettings()->value("Master/showConsole", false).toBool());
#endif
	menu->addAction("Exit", this, SLOT(on_actionExit_triggered()));
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
