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

#include <QCheckBox>
#include <QFileDialog>

#include <mt32emu/mt32emu.h>

#include "Master.h"
#include "ROMSelectionDialog.h"
#include "ui_ROMSelectionDialog.h"

using namespace MT32Emu;

enum ROMSubType {
	ROMSubType_UNSUPPORTED,
	ROMSubType_CONTROL_FULL,
	ROMSubType_CONTROL_MUX0,
	ROMSubType_CONTROL_MUX1,
	ROMSubType_PCM_FULL,
	ROMSubType_PCM_FIRST_HALF,
	ROMSubType_PCM_SECOND_HALF
};

static const char CONTROL_ROM_FULL[] = "Control Full";
static const char CONTROL_ROM_MUX0[] = "Control Mux0";
static const char CONTROL_ROM_MUX1[] = "Control Mux1";
static const char PCM_ROM_FULL[] = "PCM Full";
static const char PCM_ROM_FIRST_HALF[] = "PCM First Half";
static const char PCM_ROM_SECOND_HALF[] = "PCM Second Half";

static const int CHECKBOX_COLUMN = 0;
static const int FILENAME_COLUMN = 1;
static const int ROM_SUB_TYPE_COLUMN = 4;
static const int ROM_SHA1_DIGEST_COLUMN = 5;

static const int MACHINE_COMBO_IX_ANY = 0;
static const int MACHINE_COMBO_IX_ANY_MT32 = 1;
static const int MACHINE_COMBO_IX_ANY_CM32L = 2;
static const int MACHINE_COMBO_IX_OFFSET = 3;

static const char ALIASED_PCM_ROM_SHA1_DIGEST[] = "f6b1eebc4b2d200ec6d3d21d51325d5b48c60252";

static void addCompatibleROMInfos(QVarLengthArray<const ROMInfo *> &tmpROMInfos, const char *machineSeries) {
	for (const ROMInfo * const *romInfos = ROMInfo::getAllROMInfos(); *romInfos != NULL; romInfos++) {
		if (QByteArray((*romInfos)->shortName).contains(machineSeries)) tmpROMInfos.append(*romInfos);
	}
	tmpROMInfos.append(NULL);
}

static const ROMInfo * const *getCompatibleROMInfos(const int machineComboIx, QVarLengthArray<const ROMInfo *> &tmpROMInfos) {
	switch (machineComboIx) {
	case MACHINE_COMBO_IX_ANY:
		return ROMInfo::getAllROMInfos();
	case MACHINE_COMBO_IX_ANY_MT32:
		addCompatibleROMInfos(tmpROMInfos, "mt32");
		return tmpROMInfos.constData();
	case MACHINE_COMBO_IX_ANY_CM32L:
		addCompatibleROMInfos(tmpROMInfos, "cm32l");
		return tmpROMInfos.constData();
	default:
		break;
	}
	return MachineConfiguration::getAllMachineConfigurations()[machineComboIx - MACHINE_COMBO_IX_OFFSET]->getCompatibleROMInfos();
}

static ROMSubType getROMSubType(const ROMInfo &romInfo) {
	switch (romInfo.type) {
	case ROMInfo::Control:
		switch (romInfo.pairType) {
		case ROMInfo::Full:
			return ROMSubType_CONTROL_FULL;
			break;
		case ROMInfo::Mux0:
			return ROMSubType_CONTROL_MUX0;
			break;
		case ROMInfo::Mux1:
			return ROMSubType_CONTROL_MUX1;
		default:
			break;
		}
		break;
	case ROMInfo::PCM:
		switch (romInfo.pairType) {
		case ROMInfo::Full:
			return ROMSubType_PCM_FULL;
			break;
		case ROMInfo::FirstHalf:
			return ROMSubType_PCM_FIRST_HALF;
			break;
		case ROMInfo::SecondHalf:
			return ROMSubType_PCM_SECOND_HALF;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return ROMSubType_UNSUPPORTED;
}

static const char *getROMSubTypeName(const ROMSubType romSubType) {
	switch (romSubType) {
	case ROMSubType_CONTROL_FULL:
		return CONTROL_ROM_FULL;
	case ROMSubType_CONTROL_MUX0:
		return CONTROL_ROM_MUX0;
	case ROMSubType_CONTROL_MUX1:
		return CONTROL_ROM_MUX1;
	case ROMSubType_PCM_FULL:
		return PCM_ROM_FULL;
	case ROMSubType_PCM_FIRST_HALF:
		return PCM_ROM_FIRST_HALF;
	case ROMSubType_PCM_SECOND_HALF:
		return PCM_ROM_SECOND_HALF;
	default:
		break;
	}
	return NULL;
}

static bool isROMChecked(const QTableWidget *table, int row) {
	return table->item(row, CHECKBOX_COLUMN)->checkState() == Qt::Checked;
}

static void setROMChecked(const QTableWidget *table, int row, bool checked) {
	table->item(row, CHECKBOX_COLUMN)->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}

static const QString getFileName(const QTableWidget *table, int row) {
	return table->item(row, FILENAME_COLUMN)->text();
}

static ROMSubType getROMSubType(const QTableWidget *table, int row) {
	return (ROMSubType)table->item(row, ROM_SUB_TYPE_COLUMN)->data(Qt::UserRole).toUInt();
}

static bool isROMAliasedPCM(Ui::ROMSelectionDialog *ui, int row) {
	if (ui->machineCombo->currentIndex() != MACHINE_COMBO_IX_ANY) return false;
	return ui->romInfoTable->item(row, ROM_SHA1_DIGEST_COLUMN)->text() == ALIASED_PCM_ROM_SHA1_DIGEST;
}

static void resetCheckStates(Ui::ROMSelectionDialog *ui, const ROMSubType romSubType) {
	const QTableWidget *table = ui->romInfoTable;
	for (int row = 0; row < table->rowCount(); row++) {
		ROMSubType rowROMSubType = getROMSubType(table, row);
		if (romSubType != rowROMSubType) {
			switch (romSubType) {
			case ROMSubType_CONTROL_FULL:
				if (rowROMSubType == ROMSubType_CONTROL_MUX0 || rowROMSubType == ROMSubType_CONTROL_MUX1) break;
				continue;
			case ROMSubType_CONTROL_MUX0:
				if (rowROMSubType == ROMSubType_CONTROL_FULL) break;
				continue;
			case ROMSubType_CONTROL_MUX1:
				if (rowROMSubType == ROMSubType_CONTROL_FULL) break;
				continue;
			case ROMSubType_PCM_FULL:
				if (rowROMSubType == ROMSubType_PCM_FIRST_HALF || rowROMSubType == ROMSubType_PCM_SECOND_HALF) break;
				continue;
			case ROMSubType_PCM_FIRST_HALF:
				if (rowROMSubType == ROMSubType_PCM_FULL) break;
				continue;
			case ROMSubType_PCM_SECOND_HALF:
				if (rowROMSubType == ROMSubType_PCM_FULL && !isROMAliasedPCM(ui, row)) break;
				continue;
			default:
				break;
			}
		}
		setROMChecked(table, row, false);
	}
}

static void validateCheckedROMs(Ui::ROMSelectionDialog *ui) {
	uint controlROMSetChecked = 0;
	uint pcmROMSetChecked = 0;
	const QTableWidget *table = ui->romInfoTable;
	for (int row = 0; row < table->rowCount(); row++) {
		if (!isROMChecked(table, row)) continue;
		switch (getROMSubType(table, row)) {
		case ROMSubType_CONTROL_FULL:
			controlROMSetChecked = 3;
			break;
		case ROMSubType_CONTROL_MUX0:
			controlROMSetChecked |= 1;
			break;
		case ROMSubType_CONTROL_MUX1:
			controlROMSetChecked |= 2;
			break;
		case ROMSubType_PCM_FULL:
			pcmROMSetChecked = 3;
			break;
		case ROMSubType_PCM_FIRST_HALF:
			pcmROMSetChecked |= 1;
			break;
		case ROMSubType_PCM_SECOND_HALF:
			pcmROMSetChecked |= 2;
			break;
		default:
			break;
		}
	}
	QPushButton *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
	okButton->setEnabled(controlROMSetChecked == 3 && pcmROMSetChecked == 3);
}

ROMSelectionDialog::ROMSelectionDialog(SynthProfile &useSynthProfile, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ROMSelectionDialog),
	synthProfile(useSynthProfile),
	romInfoTableCellChangedGuard()
{
	ui->setupUi(this);

	ui->machineCombo->addItems(QStringList("any") << "any_mt32" << "any_cm32l");
	const MachineConfiguration * const *knownMachines = MachineConfiguration::getAllMachineConfigurations();
	while (*knownMachines != NULL) ui->machineCombo->addItem((*(knownMachines++))->getMachineID());

	ui->fileFilterCombo->addItems(QStringList("*.ROM;*.BIN") << "*.ROM" << "*.BIN" << "*.*");
}

ROMSelectionDialog::~ROMSelectionDialog() {
	delete ui;
}

void ROMSelectionDialog::loadROMInfos() {
	if (ui->romDirLineEdit->text() != synthProfile.romDir.absolutePath()) {
		ui->romDirLineEdit->setText(synthProfile.romDir.absolutePath());
		refreshROMInfos();
	}
}

void ROMSelectionDialog::refreshROMInfos() {
	const QString &controlROMFileName = synthProfile.controlROMFileName;
	const QString &controlROMFileName2 = synthProfile.controlROMFileName2;
	const QString &pcmROMFileName = synthProfile.pcmROMFileName;
	const QString &pcmROMFileName2 = synthProfile.pcmROMFileName2;

	QStringList fileFilter = ui->fileFilterCombo->currentText().split(';');
	QStringList dirEntries = synthProfile.romDir.entryList(fileFilter);
	ui->romInfoTable->clearContents();
	ui->romInfoTable->setRowCount(dirEntries.size());

	QVarLengthArray<const ROMInfo *> tmpROMInfos;
	const ROMInfo * const * const romInfos = getCompatibleROMInfos(ui->machineCombo->currentIndex(), tmpROMInfos);

	romInfoTableCellChangedGuard = true;
	int row = 0;
	for (int dirEntryIx = 0; dirEntryIx < dirEntries.size(); dirEntryIx++) {
		const QString &fileName = dirEntries.at(dirEntryIx);
		FileStream file;
		if (!file.open(Master::getROMPathNameLocal(synthProfile.romDir, fileName))) continue;
		const ROMInfo *romInfoPtr = ROMInfo::getROMInfo(&file, romInfos);
		if (romInfoPtr == NULL) continue;
		const ROMInfo &romInfo = *romInfoPtr;
		const ROMSubType romSubType = getROMSubType(romInfo);

		if (romSubType == ROMSubType_UNSUPPORTED) {
			MT32Emu::ROMInfo::freeROMInfo(romInfoPtr);
			continue;
		}

		bool fileNameHit = fileName == controlROMFileName || fileName == controlROMFileName2
			|| fileName == pcmROMFileName || fileName == pcmROMFileName2;

		int column = CHECKBOX_COLUMN;
		QTableWidgetItem *item = new QTableWidgetItem();
		item->setCheckState(fileNameHit ? Qt::Checked : Qt::Unchecked);
		ui->romInfoTable->setItem(row, column++, item);

		item = new QTableWidgetItem(fileName);
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		ui->romInfoTable->setItem(row, column++, item);

		item = new QTableWidgetItem(romInfo.shortName);
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		ui->romInfoTable->setItem(row, column++, item);

		item = new QTableWidgetItem(romInfo.description);
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		ui->romInfoTable->setItem(row, column++, item);

		item = new QTableWidgetItem(getROMSubTypeName(romSubType));
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		item->setData(Qt::UserRole, uint(romSubType));
		ui->romInfoTable->setItem(row, column++, item);

		item = new QTableWidgetItem(romInfo.sha1Digest);
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		ui->romInfoTable->setItem(row, column++, item);

		MT32Emu::ROMInfo::freeROMInfo(romInfoPtr);
		row++;
	}
	romInfoTableCellChangedGuard = false;
	ui->romInfoTable->setRowCount(row);
	ui->romInfoTable->resizeColumnsToContents();
	validateCheckedROMs(ui);
}

void ROMSelectionDialog::on_romDirButton_clicked() {
	QFileDialog::Options qFileDialogOptions = QFileDialog::Options(Master::getInstance()->getSettings()->value("Master/qFileDialogOptions", 0).toInt());
	QString s = QFileDialog::getExistingDirectory(this, "Choose ROM directory", synthProfile.romDir.absolutePath(),
		qFileDialogOptions | QFileDialog::ShowDirsOnly);
	if (s.isEmpty()) return;
	if (s != synthProfile.romDir.absolutePath()) {
		synthProfile.romDir.setPath(s);
		ui->romDirLineEdit->setText(s);
		refreshROMInfos();
	}
}

void ROMSelectionDialog::on_refreshButton_clicked() {
	refreshROMInfos();
}

void ROMSelectionDialog::on_fileFilterCombo_currentIndexChanged(int) {
	refreshROMInfos();
}

void ROMSelectionDialog::on_machineCombo_currentIndexChanged(int index) {
	if (index >= 0) refreshROMInfos();
}

void ROMSelectionDialog::on_romInfoTable_cellChanged(int row, int) {
	if (romInfoTableCellChangedGuard) return;
	romInfoTableCellChangedGuard = true;
	resetCheckStates(ui, getROMSubType(ui->romInfoTable, row));
	setROMChecked(ui->romInfoTable, row, true);
	romInfoTableCellChangedGuard = false;
	validateCheckedROMs(ui);
}

void ROMSelectionDialog::accept() {
	QDialog::accept();
	synthProfile.controlROMFileName2 = "";
	synthProfile.pcmROMFileName2 = "";
	const QTableWidget *table = ui->romInfoTable;
	for (int row = 0; row < table->rowCount(); row++) {
		if (!isROMChecked(table, row)) continue;
		switch (getROMSubType(table, row)) {
		case ROMSubType_CONTROL_FULL:
		case ROMSubType_CONTROL_MUX0:
			synthProfile.controlROMFileName = getFileName(table, row);
			break;
		case ROMSubType_CONTROL_MUX1:
			synthProfile.controlROMFileName2 = getFileName(table, row);
			break;
		case ROMSubType_PCM_FULL:
		case ROMSubType_PCM_FIRST_HALF:
			synthProfile.pcmROMFileName = getFileName(table, row);
			break;
		case ROMSubType_PCM_SECOND_HALF:
			synthProfile.pcmROMFileName2 = getFileName(table, row);
			break;
		default:
			break;
		}
	}
}

void ROMSelectionDialog::reject() {
	QDialog::reject();
	const QString &controlROMFileName = synthProfile.controlROMFileName;
	const QString &controlROMFileName2 = synthProfile.controlROMFileName2;
	const QString &pcmROMFileName = synthProfile.pcmROMFileName;
	const QString &pcmROMFileName2 = synthProfile.pcmROMFileName2;
	const QTableWidget *table = ui->romInfoTable;
	romInfoTableCellChangedGuard = true;
	for (int row = 0; row < table->rowCount(); row++) {
		QString fileName = getFileName(table, row);
		bool fileNameHit = fileName == controlROMFileName || fileName == controlROMFileName2
			|| fileName == pcmROMFileName || fileName == pcmROMFileName2;
		setROMChecked(table, row, fileNameHit);
	}
	romInfoTableCellChangedGuard = false;
	validateCheckedROMs(ui);
}
