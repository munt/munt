/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtGui>

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "Master.h"

#include "mididrv/TestDriver.h"
#ifdef WITH_WIN32_MIDI_DRIVER
#include "mididrv/Win32Driver.h"
#elif defined(WITH_ALSA_MIDI_DRIVER)
#include "mididrv/ALSADriver.h"
#endif


#ifdef USE_WINMM_AUDIO_DRIVER
#include "audiodrv/WinMMAudioDriver.h"
#endif
#ifdef USE_ALSA_AUDIO_DRIVER
#include "audiodrv/AlsaAudioDriver.h"
#endif
#ifdef USE_PULSE_AUDIO_DRIVER
#include "audiodrv/PulseAudioDriver.h"
#endif
#ifdef USE_PORT_AUDIO_DRIVER
#include "audiodrv/PortAudioDriver.h"
#endif

MainWindow::MainWindow(Master *master, QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	master(master),
	testMidiDriver(NULL)
{
	ui->setupUi(this);
	QObject::connect(master, SIGNAL(synthRouteAdded(SynthRoute *)), this, SLOT(handleSynthRouteAdded(SynthRoute *)));
	QObject::connect(master, SIGNAL(synthRouteRemoved(SynthRoute *)), this, SLOT(handleSynthRouteRemoved(SynthRoute *)));
#ifdef WITH_WIN32_MIDI_DRIVER
	midiDriver = new Win32MidiDriver(master);
#elif defined(WITH_ALSA_MIDI_DRIVER)
	midiDriver = new ALSAMidiDriver(master);
#else
	midiDriver = NULL;
#endif
	if (midiDriver != NULL) {
		midiDriver->start();
	}

#ifdef USE_WINMM_AUDIO_DRIVER
	//audioDrivers.append(new WinMMAudioDriver(master));
#endif
#ifdef USE_ALSA_AUDIO_DRIVER
	//audioDrivers.append(new AlsaAudioDriver(master));
#endif
#ifdef USE_PULSE_AUDIO_DRIVER
	audioDrivers.append(new PulseAudioDriver(master));
#endif
#ifdef USE_PORT_AUDIO_DRIVER
	audioDrivers.append(new PortAudioDriver(master));
#endif
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
	if (midiDriver != NULL) {
		midiDriver->stop();
		midiDriver = NULL;
	}
	if (testMidiDriver != NULL) {
		testMidiDriver->stop();
	}
	QMutableListIterator<AudioDriver *> audioDriverIt(audioDrivers);
	while(audioDriverIt.hasNext()) {
		AudioDriver *audioDriver = audioDriverIt.next();
		delete audioDriver;
		audioDriverIt.remove();
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

void MainWindow::handleSynthRouteAdded(SynthRoute *synthRoute) {
	SynthWidget *synthWidget = new SynthWidget(master, synthRoute, this);
	ui->synthTabs->addTab(synthWidget, "Synth");
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
