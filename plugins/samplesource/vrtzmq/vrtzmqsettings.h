///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
// Copyright (C) 2014 John Greb <hexameron@spam.no>                              //
// Copyright (C) 2015, 2017-2020, 2022 Edouard Griffiths, F4EXB <f4exb06@gmail.com> //
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

#ifndef _VRTZMQ_SETTINGS_H_
#define _VRTZMQ_SETTINGS_H_

#include <QString>

struct VRTZMQSettings {
	typedef enum {
		FC_POS_INFRA = 0,
		FC_POS_SUPRA,
		FC_POS_CENTER
	} fcPos_t;

	quint32 m_log2Decim;
	fcPos_t m_fcPos;
	QString m_title;
	QString m_host;
	quint32 m_instance;
	quint32 m_channel;
	quint32 m_port;
	bool m_useport;

	bool m_dcBlock;
	bool m_iqImbalance;

	VRTZMQSettings();
	void resetToDefaults();
	QByteArray serialize() const;
	bool deserialize(const QByteArray& data);
    void applySettings(const QStringList& settingsKeys, const VRTZMQSettings& settings);
    QString getDebugString(const QStringList& settingsKeys, bool force=false) const;
};


#endif /* _VRTZMQ_SETTINGS_H_ */
