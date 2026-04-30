///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2014 John Greb <hexameron@spam.no>                              //
// Copyright (C) 2015-2016, 2018-2020 Edouard Griffiths, F4EXB <f4exb06@gmail.com> //
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

#ifndef INCLUDE_VRTZMQTHREAD_H
#define INCLUDE_VRTZMQTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "device/deviceapi.h"

#include "vrtzmqsettings.h"

#include "dsp/replaybuffer.h"
#include "dsp/samplesinkfifo.h"
#include "dsp/decimators.h"

class VRTZMQThread : public QThread {
	Q_OBJECT

public:
	VRTZMQThread(DeviceAPI* deviceAPI, void* subscriber, SampleSinkFifo* sampleFifo, const VRTZMQSettings& settings, QObject* parent = nullptr);
	~VRTZMQThread();

	void startWork();
	void stopWork();
	MessageQueue *getInputMessageQueue() { return &m_inputMessageQueue; }

private:
	QMutex m_startWaitMutex;
	QWaitCondition m_startWaiter;
	bool m_running;

	int16_t *buf; //!< holds I+Q values of each sample from device

	void* m_subscriber;
	DeviceAPI* m_deviceAPI;

	// VRT
    int64_t current_freq;
    uint32_t current_sample_rate;

	SampleVector m_convertBuffer;
	SampleSinkFifo* m_sampleFifo;

	VRTZMQSettings m_settings;
	MessageQueue m_inputMessageQueue;  //!< Queue for asynchronous inbound communication

	Decimators<qint32, qint16, SDR_RX_SAMP_SZ, 16, true> m_decimatorsIQ;
    Decimators<qint32, qint16, SDR_RX_SAMP_SZ, 16, false> m_decimatorsQI;

	void run();
	void callbackIQ(const qint16* buf, qint32 len);
	void callbackQI(const qint16* buf, qint32 len);

	bool handleMessage(const Message& cmd);
	bool applySettings(const VRTZMQSettings& settings, const QStringList& settingsKeys, bool force);

private slots:
	void handleInputMessages();
};

#endif // INCLUDE_VRTZMQThread_H
