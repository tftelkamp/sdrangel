///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2014 John Greb <hexameron@spam.no>                              //
// Copyright (C) 2015-2020 Edouard Griffiths, F4EXB <f4exb06@gmail.com>          //
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

#include <QDebug>

#include <errno.h>

#include "vrtzmqthread.h"
#include "vrtzmqinput.h"

#include "dsp/devicesamplesource.h"
#include "dsp/samplesinkfifo.h"
#include "dsp/dspcommands.h"

#include "vrt-tools.h"


VRTZMQThread::VRTZMQThread(DeviceAPI* deviceAPI, void* subscriber, SampleSinkFifo* sampleFifo, const VRTZMQSettings& settings, QObject* parent) :
	QThread(parent),
	m_running(false),
    m_subscriber(subscriber),
    m_deviceAPI(deviceAPI),
	m_convertBuffer(VRT_SAMPLES_PER_PACKET),
	m_sampleFifo(sampleFifo)
{
    applySettings(settings, QStringList(), true);
    connect(&m_inputMessageQueue, &MessageQueue::messageEnqueued, this, &VRTZMQThread::handleInputMessages);
    buf = new qint16[VRT_SAMPLES_PER_PACKET*2]; // (I,Q) -> 2 * int16_t
}

VRTZMQThread::~VRTZMQThread()
{
    qDebug() << "VRTZMQThread::~VRTZMQThread";
    if (m_running) {
        stopWork();
    }
    delete[] buf;
}

void VRTZMQThread::startWork()
{
    connect(&m_inputMessageQueue, &MessageQueue::messageEnqueued, this, &VRTZMQThread::handleInputMessages);
    m_startWaitMutex.lock();
    start();
    while (!m_running) {
        m_startWaiter.wait(&m_startWaitMutex, 100);
    }
	m_startWaitMutex.unlock();
}

void VRTZMQThread::stopWork()
{
    if (m_running)
    {
        disconnect(&m_inputMessageQueue, &MessageQueue::messageEnqueued, this, &VRTZMQThread::handleInputMessages);
        m_running = false; // Cause run() to finish
#ifndef __EMSCRIPTEN__
        wait();
#endif
    }
}

void VRTZMQThread::run()
{

	m_running = true;
	m_startWaiter.wakeAll();

    context_type vrt_context;
    packet_type vrt_packet;

    init_context(&vrt_context);
    uint32_t use_channel = m_settings.m_channel;
    vrt_packet.channel_filt = 1<<use_channel;

    // VRT
    uint32_t zmq_buffer[ZMQ_BUFFER_SIZE];

    bool start_rx = false;


    while (m_running)
    {

        int len = zmq_recv(m_subscriber, zmq_buffer, ZMQ_BUFFER_SIZE, ZMQ_DONTWAIT);

        if (m_running && len > 0) {

             if (not vrt_process(zmq_buffer, sizeof(zmq_buffer), &vrt_context, &vrt_packet)) {
                    qWarning("Not a Vita49 packet?\n");
                    continue;
            }

            if (vrt_packet.context) {
                bool changed = false;
                if (current_sample_rate != vrt_context.sample_rate) {
                    current_sample_rate = vrt_context.sample_rate;
                    changed = true;
                }
                if (current_freq != vrt_context.rf_freq) {
                    current_freq = vrt_context.rf_freq;
                    changed = true;
                }
                if (changed) {
                    int sampleRate = current_sample_rate/(1<<m_settings.m_log2Decim);
                    auto *notif = new DSPSignalNotification(sampleRate, current_freq);
                    m_deviceAPI->getDeviceEngineInputMessageQueue()->push(notif);
                    qWarning("VRTZMQThread::  DSPSignalNotification");
                }
                start_rx = true;
            }

            if (vrt_packet.data && start_rx && m_running) {

                for (uint32_t i = 0; i < vrt_packet.num_rx_samps; i++) {

                    int16_t re;
                    memcpy(&re, (char*)&zmq_buffer[vrt_packet.offset+i], 2);
                    int16_t img;
                    memcpy(&img, (char*)&zmq_buffer[vrt_packet.offset+i]+2, 2);

                    buf[2*i] = re;
                    buf[2*i+1] = img;
                    
                }

               
                 callbackIQ(buf, (int32_t)vrt_packet.num_rx_samps*2);

            }

        }

    }

    m_running = false;
}

//  Decimate according to specified log2 (ex: log2=4 => decim=16)
//  Len is total samples (i.e. one I and Q pair will have len=2)
void VRTZMQThread::callbackIQ(const qint16* inBuf, qint32 len)
{
    SampleVector::iterator it = m_convertBuffer.begin();

    const qint16* buf = inBuf;
    qint32 remaining = len;

    while (remaining > 0)
    {
      
        len = remaining;
        remaining -= len; // ??

        if (m_settings.m_log2Decim == 0)
        {
            m_decimatorsIQ.decimate1(&it, buf, len);
        }
        else
        {
            if (m_settings.m_fcPos == 0) // Infradyne
            {
                switch (m_settings.m_log2Decim)
                {
                case 1:
                    m_decimatorsIQ.decimate2_inf(&it, buf, len);
                    break;
                case 2:
                    m_decimatorsIQ.decimate4_inf(&it, buf, len);
                    break;
                case 3:
                    m_decimatorsIQ.decimate8_inf(&it, buf, len);
                    break;
                case 4:
                    m_decimatorsIQ.decimate16_inf(&it, buf, len);
                    break;
                case 5:
                    m_decimatorsIQ.decimate32_inf(&it, buf, len);
                    break;
                case 6:
                    m_decimatorsIQ.decimate64_inf(&it, buf, len);
                    break;
                default:
                    break;
                }
            }
            else if (m_settings.m_fcPos == 1) // Supradyne
            {
                switch (m_settings.m_log2Decim)
                {
                case 1:
                    m_decimatorsIQ.decimate2_sup(&it, buf, len);
                    break;
                case 2:
                    m_decimatorsIQ.decimate4_sup(&it, buf, len);
                    break;
                case 3:
                    m_decimatorsIQ.decimate8_sup(&it, buf, len);
                    break;
                case 4:
                    m_decimatorsIQ.decimate16_sup(&it, buf, len);
                    break;
                case 5:
                    m_decimatorsIQ.decimate32_sup(&it, buf, len);
                    break;
                case 6:
                    m_decimatorsIQ.decimate64_sup(&it, buf, len);
                    break;
                default:
                    break;
                }
            }
            else // Centered
            {
                switch (m_settings.m_log2Decim)
                {
                case 1:
                    m_decimatorsIQ.decimate2_cen(&it, buf, len);
                    break;
                case 2:
                    m_decimatorsIQ.decimate4_cen(&it, buf, len);
                    break;
                case 3:
                    m_decimatorsIQ.decimate8_cen(&it, buf, len);
                    break;
                case 4:
                    m_decimatorsIQ.decimate16_cen(&it, buf, len);
                    break;
                case 5:
                    m_decimatorsIQ.decimate32_cen(&it, buf, len);
                    break;
                case 6:
                    m_decimatorsIQ.decimate64_cen(&it, buf, len);
                    break;
                default:
                    break;
                }
            }
        }
    }

    m_sampleFifo->write(m_convertBuffer.begin(), it);

}

void VRTZMQThread::callbackQI(const qint16* inBuf, qint32 len)
{
    SampleVector::iterator it = m_convertBuffer.begin();

    const qint16* buf = inBuf;
    qint32 remaining = len;

    while (remaining > 0)
    {
       
        len = remaining;
        remaining -= len;

        if (m_settings.m_log2Decim == 0)
        {
            m_decimatorsQI.decimate1(&it, buf, len);
        }
        else
        {
            if (m_settings.m_fcPos == 0) // Infradyne
            {
                switch (m_settings.m_log2Decim)
                {
                case 1:
                    m_decimatorsQI.decimate2_inf(&it, buf, len);
                    break;
                case 2:
                    m_decimatorsQI.decimate4_inf(&it, buf, len);
                    break;
                case 3:
                    m_decimatorsQI.decimate8_inf(&it, buf, len);
                    break;
                case 4:
                    m_decimatorsQI.decimate16_inf(&it, buf, len);
                    break;
                case 5:
                    m_decimatorsQI.decimate32_inf(&it, buf, len);
                    break;
                case 6:
                    m_decimatorsQI.decimate64_inf(&it, buf, len);
                    break;
                default:
                    break;
                }
            }
            else if (m_settings.m_fcPos == 1) // Supradyne
            {
                switch (m_settings.m_log2Decim)
                {
                case 1:
                    m_decimatorsQI.decimate2_sup(&it, buf, len);
                    break;
                case 2:
                    m_decimatorsQI.decimate4_sup(&it, buf, len);
                    break;
                case 3:
                    m_decimatorsQI.decimate8_sup(&it, buf, len);
                    break;
                case 4:
                    m_decimatorsQI.decimate16_sup(&it, buf, len);
                    break;
                case 5:
                    m_decimatorsQI.decimate32_sup(&it, buf, len);
                    break;
                case 6:
                    m_decimatorsQI.decimate64_sup(&it, buf, len);
                    break;
                default:
                    break;
                }
            }
            else // Centered
            {
                switch (m_settings.m_log2Decim)
                {
                case 1:
                    m_decimatorsQI.decimate2_cen(&it, buf, len);
                    break;
                case 2:
                    m_decimatorsQI.decimate4_cen(&it, buf, len);
                    break;
                case 3:
                    m_decimatorsQI.decimate8_cen(&it, buf, len);
                    break;
                case 4:
                    m_decimatorsQI.decimate16_cen(&it, buf, len);
                    break;
                case 5:
                    m_decimatorsQI.decimate32_cen(&it, buf, len);
                    break;
                case 6:
                    m_decimatorsQI.decimate64_cen(&it, buf, len);
                    break;
                default:
                    break;
                }
            }
        }
    }


    m_sampleFifo->write(m_convertBuffer.begin(), it);

}

void VRTZMQThread::handleInputMessages()
{
    Message* message;

    while ((message = m_inputMessageQueue.pop()) != nullptr)
    {
        if (handleMessage(*message)) {
            delete message;
        }
    }
}

bool VRTZMQThread::handleMessage(const Message& cmd)
{
    if (VRTZMQInput::MsgConfigureVRTZMQ::match(cmd))
    {
        auto& conf = (const VRTZMQInput::MsgConfigureVRTZMQ&) cmd;

        applySettings(conf.getSettings(), conf.getSettingsKeys(), conf.getForce());

        return true;
    }
    else
    {
        return false;
    }
}

bool VRTZMQThread::applySettings(const VRTZMQSettings& settings, const QStringList& settingsKeys, bool force)
{
    qDebug() << "VRTZMQThread::applySettings: force: " << force << settings.getDebugString(settingsKeys, force);

    if (force) {
        m_settings = settings;
    } else {
        m_settings.applySettings(settingsKeys, settings);
    }

    return true;
}
