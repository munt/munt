#ifndef SYNTHWIDGET_H
#define SYNTHWIDGET_H

#include <QWidget>

#include "SynthRoute.h"
#include "SynthPropertiesDialog.h"

namespace Ui {
    class SynthWidget;
}

class SynthWidget : public QWidget
{
    Q_OBJECT

public:
	explicit SynthWidget(SynthRoute *synthRoute, QWidget *parent = 0);
    ~SynthWidget();
	SynthRoute *getSynthRoute();

private:
	SynthRoute *synthRoute;
	Ui::SynthWidget *ui;
	SynthPropertiesDialog spd;

private slots:
	void on_startButton_clicked();
	void on_synthPropertiesButton_clicked();
	void on_stopButton_clicked();
	void handleSynthRouteState(SynthRouteState state);
};

#endif // SYNTHWIDGET_H
