#ifndef SYNTHWIDGET_H
#define SYNTHWIDGET_H

#include <QWidget>

#include "SynthManager.h"
#include "SynthPropertiesDialog.h"

namespace Ui {
    class SynthWidget;
}

class SynthWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SynthWidget(QWidget *parent = 0);
    ~SynthWidget();

private:
    Ui::SynthWidget *ui;
	SynthPropertiesDialog spd;
	SynthManager synthManager;

private slots:
	void on_startButton_clicked();
	void on_synthPropertiesButton_clicked();
	void on_stopButton_clicked();
	void handleSynthManagerState(SynthManagerState state);
};

#endif // SYNTHWIDGET_H
