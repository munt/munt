#ifndef FRONT_PANEL_H
#define FRONT_PANEL_H

#include <QDialog>

#include "SynthRoute.h"

namespace Ui {
	class FrontPanel;
}

class FrontPanel : public QDialog
{
	Q_OBJECT

public:
	explicit FrontPanel(QWidget *parent = 0, SynthRoute *useSynthRoute = NULL);
	~FrontPanel();

protected:
	void FrontPanel::paintEvent(QPaintEvent *);

private:
	Ui::FrontPanel *ui;
	SynthRoute *synthRoute;
	char lcdText[21];
	bool drawMaskedChars, maskedChar[20];
};

#endif // FRONT_PANEL_H
