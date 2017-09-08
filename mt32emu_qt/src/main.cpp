/* Copyright (C) 2011-2017 Jerome Fisher, Sergey V. Mikayev
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

#include <locale>
#include <QApplication>

#include "MainWindow.h"
#include "Master.h"

int main(int argv, char **args) {
	QApplication app(argv, args);
	app.setApplicationName("Munt mt32emu-qt");
	app.setQuitOnLastWindowClosed(false);
	{
		std::locale::global(std::locale(""));
		Master master;
		QSystemTrayIcon *trayIcon = NULL;
		if (QSystemTrayIcon::isSystemTrayAvailable()) {
			trayIcon = new QSystemTrayIcon(QIcon(":/images/Icon.gif"));
			trayIcon->setToolTip("Munt: MT-32 Emulator");
			trayIcon->show();
			master.setTrayIcon(trayIcon);
		}
		MainWindow mainWindow(&master);
		if (trayIcon == NULL || !master.getSettings()->value("Master/startIconized", false).toBool()) mainWindow.show();
		if (argv > 1) master.processCommandLine(app.arguments());
		master.startPinnedSynthRoute();
		master.startMidiProcessing();
		app.exec();
		master.setTrayIcon(NULL);
		delete trayIcon;
	}
	return 0;
}
