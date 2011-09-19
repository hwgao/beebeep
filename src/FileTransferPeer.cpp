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
// $Id$
//
//////////////////////////////////////////////////////////////////////

#include "BeeUtils.h"
#include "FileTransferPeer.h"
#include "Protocol.h"
#include "Settings.h"


FileTransferPeer::FileTransferPeer( QObject *parent )
  : QObject( parent ), m_transferType( FileTransferPeer::Upload ), m_id( ID_INVALID ), m_fileInfo( 0, FileInfo::Upload ), m_file(), m_state( FileTransferPeer::Unknown ),
    m_bytesTransferred( 0 ), m_totalBytesTransferred( 0 ), m_socket( parent ), m_messageAuth()
{
  setObjectName( "FileTransferPeer ");
  qDebug() << "FileTransferPeer created for transfer file";

  m_time = QTime::currentTime();

  connect( &m_socket, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SLOT( socketError( QAbstractSocket::SocketError ) ) );
  connect( &m_socket, SIGNAL( authenticationRequested( const Message& ) ), this, SLOT( checkAuthenticationRequested( const Message& ) ) );
  connect( &m_socket, SIGNAL( dataReceived( const QByteArray& ) ), this, SLOT( checkTransferData( const QByteArray& ) ) );
}

void FileTransferPeer::cancelTransfer()
{
  m_socket.abort();
  qDebug() << "FileTransferPeer" << m_id << "aborted connection due cancel transfer request";
  m_state = FileTransferPeer::Cancelled;
  emit message( id(), userId(), m_fileInfo, tr( "Transfer cancelled" ) );
  closeAll();
}

void FileTransferPeer::closeAll()
{
  qDebug() << "Making cleanup of peer" << m_id;
  if( m_socket.isOpen() )
  {
    qDebug() << "Closing socket with descriptor" << m_socket.socketDescriptor();
    m_socket.flush();
    m_socket.close();
  }
  if( m_file.isOpen() )
  {
    qDebug() << "Closing file" << m_file.fileName();
    m_file.flush();
    m_file.close();
  }
  deleteLater();
}

void FileTransferPeer::setFileInfo( const FileInfo& fi )
{
  m_fileInfo = fi;
  m_file.setFileName( m_fileInfo.path() );
  qDebug() << "Init the file" << m_file.fileName();
}

void FileTransferPeer::setConnectionDescriptor( int socket_descriptor )
{
  qDebug() << "Setup connection with socket" << socket_descriptor;
  m_state = FileTransferPeer::Request;
  m_bytesTransferred = 0;
  m_totalBytesTransferred = 0;

  m_time.start();

  if( socket_descriptor )
  {
    m_socket.setSocketDescriptor( socket_descriptor );
    qDebug() << "Using socket descriptor" << socket_descriptor;
  }
  else
  {
    m_socket.connectToHost( m_fileInfo.hostAddress(), m_fileInfo.hostPort() );
    qDebug() << "Connecting to" << m_fileInfo.hostAddress().toString() << ":" << m_fileInfo.hostPort();
  }

  QTimer::singleShot( Settings::instance().fileTransferConfirmTimeout(), this, SLOT( connectionTimeout() ) );
}

void FileTransferPeer::setTransferCompleted()
{
  qDebug() << m_fileInfo.name() << "transfer completed";
  m_state = FileTransferPeer::Completed;
  emit message( id(), userId(), m_fileInfo, tr( "Transfer completed in %1" ).arg( Bee::timerToString( m_time.elapsed() ) ) );
  closeAll();
}

void FileTransferPeer::socketError( QAbstractSocket::SocketError )
{
  // Make a check to remove the error after a transfer completed
  if( m_state <= FileTransferPeer::Transferring )
    setError( m_socket.errorString() );
}

void FileTransferPeer::setError( const QString& str_err )
{
  m_state = FileTransferPeer::Error;
  qWarning() << m_fileInfo.name() << "transfer error:" << str_err;
  emit message( id(), userId(), m_fileInfo, str_err );
  closeAll();
}

void FileTransferPeer::showProgress()
{
  emit progress( id(), userId(), m_fileInfo, m_totalBytesTransferred );
}

void FileTransferPeer::checkTransferData( const QByteArray& byte_array )
{
  if( isDownload() )
    checkDownloadData( byte_array );
  else
    checkUploadData( byte_array );
}

void FileTransferPeer::sendTransferData()
{
  if( isDownload() )
    sendDownloadData();
  else
    sendUploadData();
}

void FileTransferPeer::checkAuthenticationRequested( const Message& m )
{
  qDebug() << "Store authentication message (fixed a signal bug)";
  m_messageAuth = m;
  emit authenticationRequested();
}

void FileTransferPeer::setUserAuthorized( VNumber user_id )
{
  m_socket.setUserId( user_id );
  sendTransferData();
}

void FileTransferPeer::connectionTimeout()
{
  if( m_state <= FileTransferPeer::Request )
    setError( tr( "Connection timeout" ) );
}