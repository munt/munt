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
#include <QProcessEnvironment>
#include <mt32emu/mt32emu.h>

#include "MainWindow.h"
#include "Master.h"
#include "MasterClock.h"
#include "QSynth.h"

#include "mididrv/TestDriver.h"

using namespace MT32Emu;

int main(int argv, char **args)
{
	QApplication app(argv, args);
	app.setApplicationName("Munt mt32emu-qt");

	QProcessEnvironment::systemEnvironment().insert("PA_ALSA_PLUGHW", "1");

	MasterClock::init();
	Master::init();
	{
		if (QSystemTrayIcon::isSystemTrayAvailable()) {
			QSystemTrayIcon *trayIcon = new QSystemTrayIcon(QIcon(":/images/TrayIcon.bmp"));
			trayIcon->setToolTip("Munt: MT-32 Emulator");
			trayIcon->show();
			Master::getInstance()->setTrayIcon(trayIcon);
		}
		MainWindow mainWindow(Master::getInstance());
		mainWindow.show();
		app.exec();
	}
	Master::deinit();
	MasterClock::deinit();
	return 0;
}
