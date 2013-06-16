/* Copyright (C) 2011, 2012, 2013 Jerome Fisher, Sergey V. Mikayev
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

#ifdef WITH_WINCONSOLE
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#include <windows.h>
#endif

#ifndef GIT_HASH
#define GIT_HASH "Unknown"
#endif

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
	connect(master, SIGNAL(romsNotSet()), SLOT(on_actionROM_Configuration_triggered()));
	connect(master, SIGNAL(playMidiFiles(const QStringList &)), SLOT(handlePlayMidiFiles(const QStringList &)), Qt::QueuedConnection);
	connect(master, SIGNAL(convertMidiFiles(const QStringList &)), SLOT(handleConvertMidiFiles(const QStringList &)), Qt::QueuedConnection);

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
	if (!master->getSettings()->value("Master/showConsole", "0").toBool())
		ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
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
	event->accept();
	QApplication::quit();
}

void MainWindow::on_actionAbout_triggered()
{
	QMessageBox::about(this, "About",
					   "Munt - Roland (R) MT-32 sound module emulator\n"
					   "\n"
					   "Version " VERSION "\n"
					   "\n"
					   "Build " GIT_HASH "\n"
					   "Date " BUILD_DATE "\n"
					   "\n"
					   "Copyright (C) 2011, 2012, 2013 Jerome Fisher, Sergey V. Mikayev\n"
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
	ui->actionStart_iconized->setChecked(master->getSettings()->value("Master/startIconized", "0").toBool());
	ui->actionShow_LCD_balloons->setChecked(master->getSettings()->value("Master/showLCDBalloons", "1").toBool());
	ui->actionShow_connection_balloons->setChecked(master->getSettings()->value("Master/showConnectionBalloons", "1").toBool());
}

void MainWindow::on_actionStart_iconized_toggled(bool checked) {
	master->getSettings()->setValue("Master/startIconized", QString().setNum(checked));
}

void MainWindow::on_actionShow_LCD_balloons_toggled(bool checked) {
	master->getSettings()->setValue("Master/showLCDBalloons", QString().setNum(checked));
}

void MainWindow::on_actionShow_connection_balloons_toggled(bool checked) {
	master->getSettings()->setValue("Master/showConnectionBalloons", QString().setNum(checked));
}

void MainWindow::on_actionROM_Configuration_triggered() {
	Master &master = *Master::getInstance();
	SynthProfile synthProfile;
	synthProfile.controlROMImage = synthProfile.pcmROMImage = NULL;
	master.disconnect(this, SLOT(on_actionROM_Configuration_triggered()));
	master.loadSynthProfile(synthProfile, "");
	connect(&master, SIGNAL(romsNotSet()), SLOT(on_actionROM_Configuration_triggered()));
	ROMSelectionDialog rsd(synthProfile, this);
	rsd.loadROMInfos();
	if (rsd.exec() == QDialog::Accepted) {
		master.storeSynthProfile(synthProfile, "");
	}
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
	a->setChecked(master->getSettings()->value("Master/showConsole", "0").toBool());
#endif
	menu->addAction("Exit", this, SLOT(close()));
	master->getTrayIcon()->setContextMenu(menu);
}

void MainWindow::toggleShowConsole() {
#ifdef WITH_WINCONSOLE
	QSettings *s = master->getSettings();
	bool b = !s->value("Master/showConsole", "0").toBool();
	s->setValue("Master/showConsole", QString().setNum(b));
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
	midiConverterDialog->startConversion(fileList);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e) {
	dragMoveEvent(e);
}

void MainWindow::dragMoveEvent(QDragMoveEvent *e) {
	if (!e->mimeData()->hasUrls()) {
		e->ignore();
		return;
	}
	if ((e->possibleActions() & Qt::CopyAction) == 0) {
		e->ignore();
		return;
	}
	if (e->proposedAction() != Qt::CopyAction) {
		e->setDropAction(Qt::CopyAction);
	}
	e->accept();
}

void MainWindow::dropEvent(QDropEvent *e) {
	if (!e->mimeData()->hasUrls()) return;
	if ((e->possibleActions() & Qt::CopyAction) == 0) return;
	on_actionPlay_MIDI_file_triggered();
	midiPlayerDialog->dropEvent(e);
}
