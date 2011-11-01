#ifndef ROM_SELECTION_DIALOG_H
#define ROM_SELECTION_DIALOG_H

#include <QDialog>
#include <QButtonGroup>
#include <QDir>

class Master;
class QCheckBox;

namespace Ui {
	class ROMSelectionDialog;
}

class ROMSelectionDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ROMSelectionDialog(QWidget *parent = 0);
	~ROMSelectionDialog();
	void loadROMInfos(QString s);

private:
	Ui::ROMSelectionDialog *ui;
	QButtonGroup controlROMGroup;
	QButtonGroup pcmROMGroup;

	Master &master;
	QDir romDir;
	int controlROMRow;
	int pcmROMRow;

	const QString fileFilterToString(const QStringList fileFilter) const;
	void clearButtonGroup(QButtonGroup &group);
	void refreshROMInfos();

private slots:
	void on_fileFilterCombo_currentIndexChanged(int);
	void accept();
	void reject();
};

#endif // ROM_SELECTION_DIALOG_H
