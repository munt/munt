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
	QButtonGroup controlROMGroup;
	QButtonGroup pcmROMGroup;

	SynthProfile &synthProfile;
	int controlROMRow;
	int pcmROMRow;

	const QString fileFilterToString(const QStringList fileFilter) const;
	void clearButtonGroup(QButtonGroup &group);
	void refreshROMInfos();

private slots:
	void on_romDirButton_clicked();
	void on_refreshButton_clicked();
	void on_fileFilterCombo_currentIndexChanged(int);
	void accept();
	void reject();
};

#endif // ROM_SELECTION_DIALOG_H
