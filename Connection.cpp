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
// Author: Marco Mastroddi (marco.mastroddi(AT)gmail.com)
//
// $Id: Connection.cpp 37 2010-07-20 17:57:07Z mastroddi $
//
//////////////////////////////////////////////////////////////////////

#include "Connection.h"
#include "Protocol.h"
#include "Settings.h"


Connection::Connection( QObject *parent )
  : QTcpSocket( parent ), m_user(), m_state( WaitingForHello ), m_isHelloMessageSent( false )
{
  m_pingTimer.setInterval( PING_INTERVAL );
  connect( this, SIGNAL( readyRead() ), this, SLOT( readData() ) );
  connect( this, SIGNAL( disconnected() ), &m_pingTimer, SLOT( stop() ) );
  connect( &m_pingTimer, SIGNAL( timeout() ), this, SLOT( sendPing() ) );
  connect( this, SIGNAL( connected() ), this, SLOT( sendHello() ) );
}

bool Connection::sendMessage( const Message& m )
{
  if( !m.isValid() )
    return false;
  QString message_data = Protocol::instance().fromMessage( m );
  if( message_data.isEmpty() )
    return false;
#if defined( BEEBEEP_DEBUG )
  qDebug() << "Sending:" << message_data;
#endif
  return writeMessageData( message_data );
}

void Connection::readData()
{
  if( !canReadLine() )
    return;
#if defined( BEEBEEP_DEBUG )
  qDebug() << "New data available";
#endif
  QString line;
  if( Settings::instance().useEncryption() )
  {
    QTextStream ts( this );
    ts.setCodec( "UTF-16");
    QString encrypted_data = ts.readLine( MAX_BUFFER_SIZE );
#if defined( BEEBEEP_DEBUG )
  qDebug() << "Decrypt data:" << encrypted_data;
#endif
    encrypted_data.replace( "-NEWLINE-", "\n" );
    line = Protocol::instance().decrypt( encrypted_data );
  }
  else
  {
    line = QString::fromUtf8( readLine( MAX_BUFFER_SIZE ) );
    line.replace( "-NEWLINE-", "\n" );
    line.chop( 1 ); // remove last '\n'
  }

  if( line.size() <= Protocol::instance().messageMinimumSize() )
  {
    qWarning() << "Invalid message data size:" << line;
    return;
  }

#if defined( BEEBEEP_DEBUG )
  qDebug() << "Message data:" << line;
#endif
  Message m = Protocol::instance().toMessage( line );
  if( !m.isValid() )
  {
    qWarning() << "Skip message cause error occcurred:" << line;
    return;
  }

  if( m_state == WaitingForHello )
  {
    if( m.type() != Message::Hello )
    {
      qWarning() << "Hello message not arrived from connection:" << peerAddress().toString();
      abort();
      return;
    }

    m_user = Protocol::instance().createUser( m );
    if( !m_user.isValid() )
    {
      qDebug() << "Invalid user from connection:" << peerAddress().toString();
      abort();
      return;
    }
    qDebug() << "New user:" << m_user.nickname();
    m_user.setHostAddress( peerAddress() );
    if( !m_isHelloMessageSent )
      sendHello();

    m_pingTimer.start();
    m_pongTime.start();
    m_state = ReadyForUse;
    emit readyForUse();
    return;
  }

  switch( m.type() )
  {
  case Message::Chat:
#if defined( BEEBEEP_DEBUG )
    qDebug() << "New chat message:" << m.text();
#endif
    emit newMessage( m_user, m );
    break;

  case Message::Ping:
#if defined( BEEBEEP_DEBUG )
    qDebug() << "Ping arrived";
#endif
    sendPong();
#if defined( BEEBEEP_DEBUG )
    qDebug() << "Pong sent";
#endif
    break;

  case Message::Pong:
#if defined( BEEBEEP_DEBUG )
    qDebug() << "Pong arrived";
#endif
    m_pongTime.restart();
    break;

  case Message::File:
#if defined( BEEBEEP_DEBUG )
    qDebug() << "New File Request:" << m.text();
#endif
    emit newFileMessage( m_user, m );
    break;

  default:
    qWarning() << "Invalid message type (in Connection):" << m.type();
    break;
  }
}

void Connection::sendPing()
{
  if( m_pongTime.elapsed() > PONG_TIMEOUT )
  {
    abort();
    return;
  }
  writeMessageData( Protocol::instance().pingMessage() );
}

void Connection::sendPong()
{
  writeMessageData( Protocol::instance().pongMessage() );
}

void Connection::sendHello()
{
  if( writeMessageData( Protocol::instance().helloMessage() ) )
    m_isHelloMessageSent = true;
}

bool Connection::writeMessageData( const QString& message_data )
{
  if( Settings::instance().useEncryption() )
  {
    QTextStream ts( this );
    ts.setCodec( "UTF-16");
    QString encrypted_data = Protocol::instance().encrypt( message_data );
    encrypted_data.replace( "\n", "-NEWLINE-" );
    ts << encrypted_data;
    ts << "\n";
    ts.flush();
    return true;
  }
  else
  {
    QByteArray byte_array = message_data.toUtf8();
    byte_array.replace( "\n", "-NEWLINE-" );
    byte_array += '\n';
    return write( byte_array ) == byte_array.size();
  }
}
