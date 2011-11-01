#ifndef SYNTHPROPERTIESDIALOG_H
#define SYNTHPROPERTIESDIALOG_H

#include <QDialog>

#include "ROMSelectionDialog.h"

namespace Ui {
	class SynthPropertiesDialog;
}

class SynthPropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SynthPropertiesDialog(QWidget *parent = 0);
	~SynthPropertiesDialog();
	int getDACInputMode();

private:
	Ui::SynthPropertiesDialog *ui;
	ROMSelectionDialog rsd;

private slots:
	void on_romDirButton_clicked();
};

#endif // SYNTHPROPERTIESDIALOG_H
