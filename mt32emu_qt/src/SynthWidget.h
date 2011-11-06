#ifndef SYNTHWIDGET_H
#define SYNTHWIDGET_H

#include <QWidget>

#include "SynthRoute.h"
#include "SynthPropertiesDialog.h"
#include "AudioPropertiesDialog.h"

class Master;
class AudioDevice;
class QListWidgetItem;

namespace Ui {
	class SynthWidget;
}

class SynthWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SynthWidget(Master *master, SynthRoute *synthRoute, const AudioDevice *audioDevice, QWidget *parent = 0);
	~SynthWidget();
	SynthRoute *getSynthRoute();

private:
	SynthRoute *synthRoute;
	Ui::SynthWidget *ui;
	SynthPropertiesDialog spd;
	AudioPropertiesDialog apd;

	void refreshAudioDeviceList(Master *master, const AudioDevice *useAudioDevice);
	int findMIDISession(MidiSession *midiSession);

private slots:
	void on_startButton_clicked();
	void on_synthPropertiesButton_clicked();
	void on_audioPropertiesButton_clicked();
	void on_stopButton_clicked();
	void on_refreshButton_clicked();
	void on_pinCheckBox_stateChanged(int state);
	void on_audioDeviceComboBox_currentIndexChanged(int audioDeviceIndex);
	void handleSynthRouteState(SynthRouteState state);
	void handleSynthRoutePinned();
	void handleMIDISessionAdded(MidiSession *midiSession);
	void handleMIDISessionRemoved(MidiSession *midiSession);
};

#endif // SYNTHWIDGET_H
