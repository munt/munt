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

#include <QtGui>

#include "MidiPropertiesDialog.h"
#include "ui_MidiPropertiesDialog.h"

MidiPropertiesDialog::MidiPropertiesDialog(QWidget *parent) : QDialog(parent), ui(new Ui::MidiPropertiesDialog) {
	ui->setupUi(this);
}

MidiPropertiesDialog::~MidiPropertiesDialog() {
	delete ui;
}

void MidiPropertiesDialog::on_midiPortList_itemSelectionChanged() {
	ui->midiPortName->setText(ui->midiPortList->currentItem()->text());
}

int MidiPropertiesDialog::getCurrentMidiPortIndex() {
	return ui->midiPortList->currentRow();
}

QString MidiPropertiesDialog::getMidiPortName() {
	return ui->midiPortName->text();
}

void MidiPropertiesDialog::setMidiList(QList<QString> useMidiPortList, int selectedIndex) {
	ui->midiPortList->clear();
	for (int i = 0; i < useMidiPortList.size(); i++) {
		ui->midiPortList->addItem(useMidiPortList[i]);
	}
	if (selectedIndex > -1) {
		ui->midiPortList->setCurrentRow(selectedIndex);
	} else {
		ui->midiPortName->setText("");
	}
}

void MidiPropertiesDialog::setMidiPortListEnabled(bool enabled) {
	ui->midiPortList->setEnabled(enabled);
}

void MidiPropertiesDialog::setMidiPortName(QString name) {
	ui->midiPortName->setText(name);
}

void MidiPropertiesDialog::setMidiPortNameEditorEnabled(bool enabled) {
	ui->midiPortName->setEnabled(enabled);
}
