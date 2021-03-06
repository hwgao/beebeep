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

#include "ConnectionSocket.h"
#include "Protocol.h"
#include "Settings.h"

#undef CONNECTION_SOCKET_IO_DEBUG
#undef CONNECTION_SOCKET_IO_DEBUG_VERBOSE


ConnectionSocket::ConnectionSocket( QObject* parent )
  : QTcpSocket( parent ), m_blockSize( 0 ), m_isHelloSent( false ), m_userId( ID_INVALID ), m_protoVersion( 1 ),
    m_cipherKey( "" ), m_publicKey1( "" ), m_publicKey2( "" ), m_networkAddress(), m_latestActivityDateTime(),
    m_checkConnectionTimeout( false ), m_tickCounter( 0 ), m_isAborted( false ), m_datastreamVersion( 0 )
{
  if( Settings::instance().useLowDelayOptionOnSocket() )
    setSocketOption( QAbstractSocket::LowDelayOption, 1 );
  if( Settings::instance().disableSystemProxyForConnections() )
    setProxy( QNetworkProxy::NoProxy );
  m_pingByteArraySize = Protocol::instance().pingMessage().size() + 10;

  connect( this, SIGNAL( connected() ), this, SLOT( sendQuestionHello() ) );
  connect( this, SIGNAL( readyRead() ), this, SLOT( readBlock() ) );
  connect( this, SIGNAL( bytesWritten( qint64 ) ), this, SLOT( onBytesWritten( qint64 ) ) );
}

void ConnectionSocket::initSocket( qintptr socket_descriptor )
{
  m_isAborted = false;
  setSocketDescriptor( socket_descriptor );
  m_networkAddress.setHostAddress( peerAddress() );
  m_networkAddress.setHostPort( peerPort() );
  m_tickCounter = 0;
  m_checkConnectionTimeout = false;
}

void ConnectionSocket::connectToNetworkAddress( const NetworkAddress& network_address )
{
  m_isAborted = false;
  m_networkAddress = network_address;
  m_tickCounter = 0;
  m_checkConnectionTimeout = true;
  connectToHost( network_address.hostAddress(), network_address.hostPort() );
}

void ConnectionSocket::abortConnection()
{
  m_isAborted = true;
  m_userId = ID_INVALID;
  abort();
}

void ConnectionSocket::closeConnection()
{
  if( isOpen() )
  {
    flushAll();
    close();
  }
  m_userId = ID_INVALID;
  m_isAborted = true;
}

const QByteArray& ConnectionSocket::cipherKey() const
{
  return m_cipherKey.isEmpty() ? Settings::instance().password() : m_cipherKey;
}

bool ConnectionSocket::createCipherKey( const QString& public_key )
{
  if( m_publicKey1.isEmpty() )
  {
#ifdef BEEBEEP_DEBUG
    qDebug() << "Encryption handshake key 1";
#endif
    m_publicKey1 = public_key;
  }
  else if( m_publicKey2.isEmpty() )
  {
    m_publicKey2 = public_key;
#ifdef BEEBEEP_DEBUG
    qDebug() << "Encryption handshake key 2";
#endif
  }
  else
  {
    qWarning() << "Encryption handshake error. Too many public key arrived from" << qPrintable( m_networkAddress.toString() );
    return false;
  }

  if( m_publicKey1.isEmpty() || m_publicKey2.isEmpty() )
    return false;

#ifdef BEEBEEP_DEBUG
  qDebug() << "Encryption handshake completed with" << qPrintable( m_networkAddress.toString() );
#endif

  m_cipherKey = Protocol::instance().createCipherKey( m_publicKey1, m_publicKey2, m_datastreamVersion );
  m_publicKey1 = "";
  m_publicKey2 = "";
  return true;
}

qint64 ConnectionSocket::readBlock()
{
  if( m_isAborted )
    return 0;

  m_latestActivityDateTime = QDateTime::currentDateTime();
  qint64 bytes_available = bytesAvailable();

  if( bytes_available == 0 )
  {
#ifdef BEEBEEP_DEBUG
    qDebug() << "ConnectionSocket from" << qPrintable( m_networkAddress.toString() ) << "is empty... wait for more bytes";
#endif
    return 0;
  }

  // QByteArray
  // If the byte array is null: 0xFFFFFFFF (quint32)
  // Otherwise: the array size (quint32) followed by the array bytes, i.e. size bytes
  QDataStream data_stream( this );
  data_stream.setByteOrder( QDataStream::BigEndian );

  if( m_datastreamVersion > 0 )
  {
    data_stream.setVersion( m_datastreamVersion );
  }
  else
  {
    if( m_protoVersion > SECURE_LEVEL_2_PROTO_VERSION )
      data_stream.setVersion( DATASTREAM_VERSION_2 );
    else
      data_stream.setVersion( DATASTREAM_VERSION_1 );
  }

#ifdef BEEBEEP_DEBUG
  if( isConnecting() )
    qDebug() << "Connection" << qPrintable( m_networkAddress.toString() ) << "use datastream version:" << data_stream.version();
#endif

  if( m_blockSize == 0 )
  {
    if( m_protoVersion > SECURE_LEVEL_2_PROTO_VERSION )
    {
      if( bytes_available < (int)sizeof(DATA_BLOCK_SIZE_32))
      {
#ifdef BEEBEEP_DEBUG
        qDebug() << "ConnectionSocket from" << qPrintable( m_networkAddress.toString() ) << "has only" << bytes_available << "bytes... wait for more";
#endif
        return 0;
      }
      DATA_BLOCK_SIZE_32 block_size_32;
      data_stream >> block_size_32;
      m_blockSize = block_size_32 - sizeof(DATA_BLOCK_SIZE_32); // bytearray serialize format must be 32 bytes
    }
    else
    {
      if( bytes_available < (int)sizeof(DATA_BLOCK_SIZE_16))
      {
#ifdef BEEBEEP_DEBUG
        qDebug() << "ConnectionSocket from" << qPrintable( m_networkAddress.toString() ) << "has only" << bytes_available << "bytes... wait for more";
#endif
        return 0;
      }
      DATA_BLOCK_SIZE_16 block_size_16;
      data_stream >> block_size_16;
      m_blockSize = block_size_16 - sizeof(DATA_BLOCK_SIZE_32); // bytearray serialize format must be 32 bytes (not 16!!!)
    }
  }

#if defined( CONNECTION_SOCKET_IO_DEBUG )
  qDebug() << "ConnectionSocket read from" << qPrintable( m_networkAddress.toString() ) << "the block size:" << m_blockSize;
#endif

  if( bytes_available < m_blockSize )
  {
#ifdef BEEBEEP_DEBUG
    qDebug() << "ConnectionSocket from" << qPrintable( m_networkAddress.toString() ) << "has" << bytes_available << "and wait for" << (m_blockSize-bytes_available) << "more bytes, total" << m_blockSize;
#endif
    return 0;
  }

  QByteArray byte_array_read;
  data_stream >> byte_array_read;

  if( byte_array_read.size() != (int)m_blockSize )
  {
    qWarning() << "ConnectionSocket read an invalid block size from" << qPrintable( m_networkAddress.toString() ) << ":"
               << byte_array_read.size() << "bytes read and" << m_blockSize << "bytes aspected";
  }

  m_blockSize = 0;

  QByteArray decrypted_byte_array = Protocol::instance().decryptByteArray( byte_array_read, cipherKey(), m_protoVersion );

#if defined( CONNECTION_SOCKET_IO_DEBUG_VERBOSE )
  qDebug() << "ConnectionSocket read from" << qPrintable( m_networkAddress.toString() ) << "the byte array:" << decrypted_byte_array;
#endif

  if( m_userId == ID_INVALID )
  {
#ifdef BEEBEEP_DEBUG
    qDebug() << "HELLO received from" << qPrintable( m_networkAddress.toString() );
#endif
    checkHelloMessage( decrypted_byte_array );
  }
  else
    emit dataReceived( decrypted_byte_array );

  return byte_array_read.size();
}

void ConnectionSocket::flushAll()
{
  int prevent_loop = 0;
  while( bytesAvailable() && prevent_loop < (MAX_NUM_OF_LOOP_IN_CONNECTON_SOCKECT * 10) )
  {
    prevent_loop++;
    readBlock();
  }
  flush();
}

QByteArray ConnectionSocket::serializeData( const QByteArray& bytes_to_send )
{
  QByteArray data_block;
  QDataStream data_stream( &data_block, QIODevice::WriteOnly );
  data_stream.setByteOrder( QDataStream::BigEndian );

  if( m_protoVersion > SECURE_LEVEL_2_PROTO_VERSION )
  {
    if( m_datastreamVersion > 0 )
      data_stream.setVersion( m_datastreamVersion );
    else
      data_stream.setVersion( DATASTREAM_VERSION_2 );

    data_stream << (DATA_BLOCK_SIZE_32)0;

    if( bytes_to_send.size() > DATA_BLOCK_SIZE_32_LIMIT )
    {
      qWarning() << "Unable to send a message to" << qPrintable( m_networkAddress.toString() ) << "because exceeded the limit of 32bit block data... truncated to max size";
      QByteArray bytes_to_send_truncated = bytes_to_send;
      bytes_to_send_truncated.truncate( DATA_BLOCK_SIZE_32_LIMIT );
      data_stream << bytes_to_send_truncated;
    }
    else
      data_stream << bytes_to_send;

    data_stream.device()->seek( 0 );
    data_stream << (DATA_BLOCK_SIZE_32)(data_block.size() - sizeof(DATA_BLOCK_SIZE_32));
  }
  else
  {
    data_stream.setVersion( DATASTREAM_VERSION_1 );
    data_stream << (DATA_BLOCK_SIZE_16)0;

    if( bytes_to_send.size() > DATA_BLOCK_SIZE_16_LIMIT )
    {
      qWarning() << "Unable to send a message to" << qPrintable( m_networkAddress.toString() ) << "because exceeded the limit of 16bit block data... truncated to max size";
      QByteArray bytes_to_send_truncated = bytes_to_send;
      bytes_to_send_truncated.truncate( DATA_BLOCK_SIZE_16_LIMIT );
      data_stream << bytes_to_send_truncated;
    }
    else
      data_stream << bytes_to_send;

    data_stream.device()->seek( 0 );
    data_stream << (DATA_BLOCK_SIZE_16)(data_block.size() - sizeof(DATA_BLOCK_SIZE_16));
  }

  return data_block;
}

void ConnectionSocket::onBytesWritten( qint64 bytes_written )
{
  // This function is useful for large byte array data to prevent connection timeout.
  // To recognize disconnection, ping message must be skipped
  if( bytes_written > m_pingByteArraySize )
    m_latestActivityDateTime = QDateTime::currentDateTime();
}

bool ConnectionSocket::sendData( const QByteArray& byte_array )
{
#if defined( CONNECTION_SOCKET_IO_DEBUG_VERBOSE )
  qDebug() << "ConnectionSocket is sending to" << qPrintable( m_networkAddress.toString() ) << "the following data:" << byte_array;
#endif
  QByteArray byte_array_to_send = Protocol::instance().encryptByteArray( byte_array, cipherKey(), m_protoVersion );

  QByteArray data_serialized = serializeData( byte_array_to_send );

  if( write( data_serialized ) == data_serialized.size() )
  {
#ifdef CONNECTION_SOCKET_IO_DEBUG
    qDebug() << "ConnectionSocket sends" << data_serialized.size() << "bytes to" << qPrintable( m_networkAddress.toString() );
#endif
    flush();
    return true;
  }
  else
  {
    qWarning() << "ConnectionSocket to"  << qPrintable( m_networkAddress.toString() ) << "has an I/O error";
    return false;
  }
}

void ConnectionSocket::sendQuestionHello()
{
  m_checkConnectionTimeout = false;
  m_publicKey1 = Protocol::instance().newMd5Id();
#ifdef CONNECTION_SOCKET_IO_DEBUG
  qDebug() << "ConnectionSocket is sending pkey1 with shared-key:" << qPrintable( m_publicKey1 );
#endif
  if( sendData( Protocol::instance().helloMessage( m_publicKey1 ) ) )
  {
#ifdef BEEBEEP_DEBUG
    qDebug() << "ConnectionSocket has sent question HELLO to" << qPrintable( m_networkAddress.toString() );
#endif
    m_isHelloSent = true;
  }
  else
  {
    qWarning() << "ConnectionSocket is unable to send question HELLO to" << qPrintable( m_networkAddress.toString() );
    emit abortRequest();
  }
}

void ConnectionSocket::sendAnswerHello()
{
  m_publicKey2 = Protocol::instance().newMd5Id();
#ifdef CONNECTION_SOCKET_IO_DEBUG
  qDebug() << "ConnectionSocket is sending pkey2 with shared-key:" << qPrintable( m_publicKey2 );
#endif
  if( sendData( Protocol::instance().helloMessage( m_publicKey2 ) ) )
  {
#ifdef BEEBEEP_DEBUG
    qDebug() << "ConnectionSocket has sent answer HELLO to" << qPrintable( m_networkAddress.toString() );
#endif
    m_isHelloSent = true;
  }
  else
  {
    qWarning() << "ConnectionSocket is unable to send answer HELLO to" << qPrintable( m_networkAddress.toString() );
    emit abortRequest();
  }
}

void ConnectionSocket::checkHelloMessage( const QByteArray& array_data )
{
  Message m = Protocol::instance().toMessage( array_data, m_protoVersion );
  if( !m.isValid() )
  {
    qWarning() << "ConnectionSocket has received an invalid HELLO from" << qPrintable( m_networkAddress.toString() );
    emit abortRequest();
    return;
  }

  if( m.type() != Message::Hello )
  {
    qWarning() << "ConnectionSocket is waiting for HELLO, but another message type" << m.type() << "is arrived from" << qPrintable( m_networkAddress.toString() );
    emit abortRequest();
    return;
  }

  if( Settings::instance().acceptConnectionsOnlyFromWorkgroups() )
  {
    if( !Protocol::instance().acceptConnectionFromWorkgroup( m ) )
    {
#ifdef BEEBEEP_DEBUG
      qDebug() << "ConnectionSocket drops user of external workgroup from" << qPrintable( m_networkAddress.toString() );
#endif
      emit abortRequest();
      return;
    }
    else
      qDebug() << "ConnectionSocket has accepted user of your workgroup from" << qPrintable( m_networkAddress.toString() );
  }

  if( !m_isHelloSent )
    sendAnswerHello();

  // After sending HELLO to ensure low protocol version compatibility
  m_protoVersion = Protocol::instance().protoVersion( m );
  int peer_datastream_version = Protocol::instance().datastreamVersion( m );
  if( peer_datastream_version > 0 )
  {
    m_datastreamVersion = qMax( 0, (int)qMin( peer_datastream_version, Protocol::instance().datastreamMaxVersion() ) );
#ifdef BEEBEEP_DEBUG
    qDebug() << "ConnectionSocket uses handshaked datastream version" << m_datastreamVersion;
#endif
  }
  else
    m_datastreamVersion = 0;

  if( m_protoVersion != Settings::instance().protoVersion() )
  {
    qWarning() << "Protocol version from" << qPrintable( m_networkAddress.toString() ) << "is"
               << (m_protoVersion > Settings::instance().protoVersion() ? "newer" : "older") << "than yours";
    if( m_protoVersion > Settings::instance().protoVersion() )
    {
      m_protoVersion = Settings::instance().protoVersion();
      qWarning() << "Your old protocol version" << m_protoVersion << "is used with" << qPrintable( m_networkAddress.toString() );
    }
    else
      qWarning() << "Old protocol version" << m_protoVersion << "is used with" << qPrintable( m_networkAddress.toString() );
  }

  if( m_protoVersion > SECURE_LEVEL_2_PROTO_VERSION )
  {
    QString public_key = Protocol::instance().publicKey( m );
    if( !public_key.isEmpty() )
    {
      if( !createCipherKey( public_key ) )
      {
        qWarning() << "You have not shared a public key for encryption";
        emit abortRequest();
        return;
      }
      else
      {
        if( m_protoVersion < SECURE_LEVEL_3_PROTO_VERSION )
          qWarning() << "Old encryption level 2 is activated with" << qPrintable( m_networkAddress.toString() );
        else
          qDebug() << "Encryption level 3 is activated with" << qPrintable( m_networkAddress.toString() );
      }
    }
    else
      qWarning() << "Remote host" << qPrintable( m_networkAddress.toString() ) << "has not shared a public key for encryption";
  }

  qDebug() << "ConnectionSocket request an authentication for" << qPrintable( m_networkAddress.toString() );
  emit authenticationRequested( array_data );
}

int ConnectionSocket::fileTransferBufferSize() const
{
  return m_protoVersion > SECURE_LEVEL_2_PROTO_VERSION ? Settings::instance().fileTransferBufferSize() : qMin( (int)65456, Settings::instance().fileTransferBufferSize() );
}

void ConnectionSocket::checkConnectionTimeout( int ticks )
{
  if( !m_checkConnectionTimeout )
    return;

  if( ticks < Settings::instance().tickIntervalConnectionTimeout() )
    return;

  qDebug() << "Connection timeout for" << qPrintable( m_networkAddress.toString() ) << ":" << ticks << "ticks";
  disconnectFromHost();
  emit disconnected();
}

int ConnectionSocket::activityIdle() const
{
  if( m_isAborted )
    return 0;

  if( !m_latestActivityDateTime.isValid() )
    return 0;

  if( bytesAvailable() > 0 )
    return TICK_INTERVAL; // force a read block

#if QT_VERSION < 0x040700
  quint64 idle_time = m_latestActivityDateTime.time().msecsTo( QDateTime::currentDateTime().time() );
#else
  quint64 idle_time = m_latestActivityDateTime.msecsTo( QDateTime::currentDateTime() );
#endif

  if( idle_time < 2147483647 )
    return (int)idle_time;
  else
    return 2147483647;
}

void ConnectionSocket::onTickEvent( int  )
{
  if( m_isAborted )
    return;

  m_tickCounter++;

  if( isConnecting() )
  {
    checkConnectionTimeout( m_tickCounter );
    return;
  }

  if( m_tickCounter > 31536000 )
  {
    // 1 year is passed ... it is time to close!
    qWarning() << "A year in uptime is passed. It is time to close and restart connection" << qPrintable( m_networkAddress.toString() );
    emit abortRequest();
    return;
  }

  qint64 bytes_available = bytesAvailable();
  if( bytes_available > 0 )
  {
    if( m_blockSize == 0 || bytes_available >= m_blockSize )
    {
#ifdef BEEBEEP_DEBUG
      qDebug() << qPrintable( m_networkAddress.toString() ) << "has" << (int)bytesAvailable() << "bytes available: read forced";
#endif
      if( readBlock() > 0 )
        return;
    }
  }

  if( m_tickCounter % PING_INTERVAL_TICK == 0 )
    emit pingRequest();
}
