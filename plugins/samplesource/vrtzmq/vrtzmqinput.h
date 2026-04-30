///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2014 John Greb <hexameron@spam.no>                              //
// Copyright (C) 2015-2020, 2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com>    //
// Copyright (C) 2023 Jon Beniston, M7RCE <jon@beniston.com>                     //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
// (at your option) any later version.                                           //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDE_VRTZMQINPUT_H
#define INCLUDE_VRTZMQINPUT_H

#include <QString>
#include <QByteArray>
#include <QNetworkRequest>

#include "dsp/devicesamplesource.h"
#include "vrtzmqsettings.h"

#include <zmq.h>

extern "C" {
#include "vrt-tools.h"
}

class DeviceAPI;
class VRTZMQThread;
class QNetworkAccessManager;

class VRTZMQInput : public DeviceSampleSource {
    Q_OBJECT
public:
	class MsgConfigureVRTZMQ : public Message {
		MESSAGE_CLASS_DECLARATION

	public:
		const VRTZMQSettings& getSettings() const { return m_settings; }
        const QList<QString>& getSettingsKeys() const { return m_settingsKeys; }
		bool getForce() const { return m_force; }

		static MsgConfigureVRTZMQ* create(const VRTZMQSettings& settings, const QList<QString>& settingsKeys, bool force)
		{
			return new MsgConfigureVRTZMQ(settings, settingsKeys, force);
		}

	private:
		VRTZMQSettings m_settings;
        QList<QString> m_settingsKeys;
		bool m_force;

		MsgConfigureVRTZMQ(const VRTZMQSettings& settings, const QList<QString>& settingsKeys, bool force) :
			Message(),
			m_settings(settings),
            m_settingsKeys(settingsKeys),
			m_force(force)
		{ }
	};

    class MsgStartStop : public Message {
        MESSAGE_CLASS_DECLARATION

    public:
        bool getStartStop() const { return m_startStop; }

        static MsgStartStop* create(bool startStop) {
            return new MsgStartStop(startStop);
        }

    private:
        bool m_startStop;

        explicit MsgStartStop(bool startStop) :
            Message(),
            m_startStop(startStop)
        { }
    };

	explicit VRTZMQInput(DeviceAPI *deviceAPI);
	~VRTZMQInput() final;
	void destroy() final;

	void init() final;
	bool start() final;
	void stop() final;

    QByteArray serialize() const final;
    bool deserialize(const QByteArray& data) final;

    void setMessageQueueToGUI(MessageQueue *queue) final { m_guiMessageQueue = queue; }
	const QString& getDeviceDescription() const final;
	int getSampleRate() const final;
    void setSampleRate(int sampleRate) final { (void) sampleRate; }
	quint64 getCenterFrequency() const final;
	void setCenterFrequency(qint64 centerFrequency) final;

	bool handleMessage(const Message& message) final;

private:

    // VRT
    qint64 current_freq;
    quint32 current_sample_rate;

    // VRT ZMQ
    void *context = NULL;
    void *subscriber = NULL;

	DeviceAPI *m_deviceAPI;
	QMutex m_mutex;
	VRTZMQSettings m_settings;
	VRTZMQThread* m_VRTZMQThread;
	QString m_deviceDescription;
	bool m_running;
    QNetworkAccessManager *m_networkManager;
    QNetworkRequest m_networkRequest;

	bool openDevice();
	void closeDevice();
	bool applySettings(const VRTZMQSettings& settings, const QList<QString>& settingsKeys, bool force);
   
private slots:
    // void networkManagerFinished(QNetworkReply *reply) const;
};

#endif // INCLUDE_VRTZMQINPUT_H
