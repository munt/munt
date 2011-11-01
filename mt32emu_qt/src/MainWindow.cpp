/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
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

#ifdef _WINDOWS
#include <windows.h>
#endif

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "Master.h"

#include "mididrv/TestDriver.h"

MainWindow::MainWindow(Master *master, QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	master(master),
	testMidiDriver(NULL)
{
	ui->setupUi(this);
	QObject::connect(master, SIGNAL(synthRouteAdded(SynthRoute *, const AudioDevice *)), SLOT(handleSynthRouteAdded(SynthRoute *, const AudioDevice *)));
	QObject::connect(master, SIGNAL(synthRouteRemoved(SynthRoute *)), SLOT(handleSynthRouteRemoved(SynthRoute *)));
	if (master->getTrayIcon() != NULL) {
		connect(master->getTrayIcon(), SIGNAL(activated(QSystemTrayIcon::ActivationReason)), SLOT(handleTrayIconActivated(QSystemTrayIcon::ActivationReason)));
		trayIconContextMenu();
	}
#ifdef _WINDOWS
	if (!master->getSettings()->value("Master/showConsole", "0").toBool())
		ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
	if (testMidiDriver != NULL) {
		testMidiDriver->stop();
	}
	event->accept();
}

void MainWindow::on_actionAbout_triggered()
{
	QMessageBox::about(this, "About",
					   "Munt mt32emu-qt\n"
					   "\n"
					   "Version wibble"
					   "\n"
					   "Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev\n"
					   "\n"
					   "Blah blah GPL etc."
					   );
}

void MainWindow::handleSynthRouteAdded(SynthRoute *synthRoute, const AudioDevice *audioDevice) {
	SynthWidget *synthWidget = new SynthWidget(master, synthRoute, audioDevice, this);
	ui->synthTabs->addTab(synthWidget, "Synth");
	connect(synthWidget, SIGNAL(synthRoutePinned()), SLOT(handleSynthRoutePinned()));
}

void MainWindow::handleSynthRoutePinned() {
	emit synthRoutePinned();
}

void MainWindow::handleSynthRouteRemoved(SynthRoute *synthRoute) {
	QWidget *widget;
	for(int i = 0;;i++) {
		widget = ui->synthTabs->widget(i);
		if(widget == NULL) {
			qDebug() << "Couldn't find widget for removed synth";
			return;
		}
		SynthWidget *synthWidget = (SynthWidget *)widget;
		if(synthRoute == synthWidget->getSynthRoute()) {
			ui->synthTabs->removeTab(i);
			return;
		}
	}
}

void MainWindow::on_actionOptions_triggered()
{

}

void MainWindow::on_actionPlay_MIDI_file_triggered()
{

}

void MainWindow::on_actionTest_MIDI_Driver_toggled(bool checked)
{
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

void MainWindow::trayIconContextMenu() {
	QMenu *menu = new QMenu(this);
	QFont bold;
	bold.setBold(true);
	menu->addAction("Show/Hide", this, SLOT(showHideMainWindow()))->setFont(bold);
	QAction *a = menu->addAction("Start iconized", this, SLOT(toggleStartIconized()));
	a->setCheckable(true);
	a->setChecked(master->getSettings()->value("Master/startIconized", "1").toBool());
#ifdef _WINDOWS
	a = menu->addAction("Show console", this, SLOT(toggleShowConsole()));
	a->setCheckable(true);
	a->setChecked(master->getSettings()->value("Master/showConsole", "0").toBool());
#endif
	menu->addAction("Exit", this, SLOT(close()));
	master->getTrayIcon()->setContextMenu(menu);
}

void MainWindow::toggleShowConsole() {
#ifdef _WINDOWS
	QSettings *s = master->getSettings();
	bool b = !s->value("Master/showConsole", "0").toBool();
	s->setValue("Master/showConsole", QString().setNum(b));
	if (b)
		ShowWindow(GetConsoleWindow(), SW_NORMAL);
	else
		ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
}

void MainWindow::toggleStartIconized() {
	QSettings *s = master->getSettings();
	bool b = !s->value("Master/startIconized", "1").toBool();
	s->setValue("Master/startIconized", QString().setNum(b));
}

void MainWindow::showHideMainWindow() {
	setVisible(!isVisible());
}

void MainWindow::handleTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
	switch (reason) {
	case QSystemTrayIcon::Context:
		break;
	case QSystemTrayIcon::DoubleClick:
		// Fall-through until implemented
	case QSystemTrayIcon::Trigger:
		// Fall-through untill implemented
	default:
		showHideMainWindow();
		break;
	}
}
