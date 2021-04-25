#ifndef ROM_SELECTION_DIALOG_H
#define ROM_SELECTION_DIALOG_H

#include <QDialog>
#include <QButtonGroup>
#include <QDir>

namespace Ui {
	class ROMSelectionDialog;
}

struct SynthProfile;

class ROMSelectionDialog : public QDialog
{
	Q_OBJECT

public:
	ROMSelectionDialog(SynthProfile &synthProfile, QWidget *parent);
	~ROMSelectionDialog();
	void loadROMInfos();

private:
	Ui::ROMSelectionDialog *ui;

	SynthProfile &synthProfile;
	bool romInfoTableCellChangedGuard;

	void refreshROMInfos();

private slots:
	void on_romDirButton_clicked();
	void on_refreshButton_clicked();
	void on_fileFilterCombo_currentIndexChanged(int);
	void on_machineCombo_currentIndexChanged(int);
	void on_romInfoTable_cellChanged(int, int);
	void accept();
	void reject();
};

#endif // ROM_SELECTION_DIALOG_H
