//////////////////////////////////////////////////////////////////////
//
// This file is part of BeeBEEP.
//
// BeeBEEP is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// BeeBEEP is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with BeeBEEP.  If not, see <http://www.gnu.org/licenses/>.
//
// Author: Marco Mastroddi <marco.mastroddi(AT)gmail.com>
//
// $Id$
//
//////////////////////////////////////////////////////////////////////

#ifndef BEEBEEP_CONFIG_H
#define BEEBEEP_CONFIG_H

#include <QtCore>
#include <QtGui>
#include <QtNetwork>

#if QT_VERSION >= 0x050000
  #include <QtWidgets>
  #include <QtMultimedia>
#else
  typedef int qintptr;
#endif


// Type definition
typedef quint64 VNumber;
typedef quint64 FileSizeType;

// General
#define DATASTREAM_VERSION_1 QDataStream::Qt_4_0
#define DATASTREAM_VERSION_2 QDataStream::Qt_4_8
#define LAST_DATASTREAM_VERSION DATASTREAM_VERSION_2

// Connection I/O
#define DATA_BLOCK_SIZE_16 quint16
// set the limit to 65535 - sizeof( quint16 ) ... or a value near it
const DATA_BLOCK_SIZE_16 DATA_BLOCK_SIZE_16_LIMIT = 65519;
#define DATA_BLOCK_SIZE_32 quint32
//Due to compiler warning (ISO C90) the value 4294967295 of quint32 is not possibile... use qint32 - sizeof( qint32 )
const qint32 DATA_BLOCK_SIZE_32_LIMIT = 2147483615;
#define ENCRYPTED_DATA_BLOCK_SIZE 16
#define ENCRYPTION_KEYBITS 256
#define MAX_NUM_OF_LOOP_IN_CONNECTON_SOCKECT 20

// Protocol
#define ID_INVALID         0
#define ID_LOCAL_USER      1
#define ID_DEFAULT_CHAT    2
#define ID_START           1000
#define ID_SYSTEM_MESSAGE  10
#define ID_BEEP_MESSAGE    11
#define ID_WRITING_MESSAGE 12
#define ID_PING_MESSAGE    13
#define ID_PONG_MESSAGE    14
#define ID_HELLO_MESSAGE   15
#define ID_USER_MESSAGE    16
#define ID_VCARD_MESSAGE   17
#define ID_SHARE_MESSAGE   18

// FILE_SHARING
#define MAX_NUM_FILE_SHARED 32700

#endif // BEEBEEP_CONFIG_H
