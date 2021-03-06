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

#ifndef BEEBEEP_FILETRANSFERPEER_H
#define BEEBEEP_FILETRANSFERPEER_H

#include "ConnectionSocket.h"
#include "FileInfo.h"


class FileTransferPeer : public QObject
{
  Q_OBJECT

public:
  enum TransferState { Unknown, Queue, Starting, Request, FileHeader, Transferring, Completed, Error, Cancelled };

  explicit FileTransferPeer( QObject *parent = Q_NULLPTR );

  inline QString name() const;

  inline void setInQueue();
  inline bool isInQueue() const;
  inline void removeFromQueue();

  inline void setTransferType( FileInfo::TransferType );
  inline void setRemoteUserId( VNumber );
  inline VNumber remoteUserId() const;
  inline bool isDownload() const;
  inline void setId( VNumber );
  inline VNumber id() const;
  inline void setConnectionDescriptor( int ); // if descriptor = 0 socket tries to connect to remote host (client side)
  void setFileInfo( FileInfo::TransferType, const FileInfo& );
  inline const FileInfo& fileInfo() const;

  inline const Message& messageAuth() const; // Read below...
  inline QHostAddress peerAddress() const;

  inline bool isActive() const;
  inline bool isTransferCompleted() const;
  void startUpload( const FileInfo& );
  void cancelTransfer();

  void onTickEvent( int );

signals:
  void message( VNumber peer_id, VNumber user_id, const FileInfo&, const QString& );
  void progress( VNumber peer_id, VNumber user_id, const FileInfo&, FileSizeType );
  void completed( VNumber peer_id, VNumber user_id, const FileInfo& );
  void fileUploadRequest( const FileInfo& );
  void userValidationRequested( VNumber peer_id, VNumber user_id );

public slots:
  void startConnection();

protected slots:
  void socketError( QAbstractSocket::SocketError );
  void checkTransferData( const QByteArray& );
  void connectionTimeout();
  void checkUserAuthentication( const QByteArray& );

protected:
  void setUserAuthorized( VNumber );
  void showProgress();
  void setError( const QString& );
  void setTransferCompleted();
  void closeAll();
  void sendTransferData();

  /* FileTransferUpload */
  void sendUploadData();
  void checkUploadData( const QByteArray& );
  void checkUploadRequest( const QByteArray& );
  void checkUploading( const QByteArray& );
  void sendFileHeader();

  /* FileTransferDownload */
  void sendDownloadData();
  void checkDownloadData( const QByteArray& );
  void sendDownloadRequest();
  void sendDownloadDataConfirmation();

protected:
  FileInfo::TransferType m_transferType;
  VNumber m_id;
  FileInfo m_fileInfo;
  QFile m_file;
  TransferState m_state;
  int m_bytesTransferred;
  FileSizeType m_totalBytesTransferred;
  ConnectionSocket m_socket;
  QTime m_time;
  int m_socketDescriptor;
  VNumber m_remoteUserId;

};


// Inline Functions
inline QString FileTransferPeer::name() const { return QString( "%1 Peer #%2" ).arg( isDownload() ? "Download" : "Upload" ).arg( m_id ); }
inline void FileTransferPeer::setConnectionDescriptor( int new_value ) { m_socketDescriptor = new_value; }
inline void FileTransferPeer::setInQueue() { m_state = FileTransferPeer::Queue; }
inline bool FileTransferPeer::isInQueue() const { return m_state == FileTransferPeer::Queue; }
inline void FileTransferPeer::removeFromQueue() { m_state = FileTransferPeer::Starting; }
inline void FileTransferPeer::setTransferType( FileInfo::TransferType new_value ) { m_transferType = new_value; }
inline bool FileTransferPeer::isDownload() const { return m_transferType == FileInfo::Download; }
inline void FileTransferPeer::setId( VNumber new_value ) { m_id = new_value; }
inline VNumber FileTransferPeer::id() const { return m_id; }
inline const FileInfo& FileTransferPeer::fileInfo() const { return m_fileInfo; }
inline QHostAddress FileTransferPeer::peerAddress() const { return m_socket.peerAddress(); }
inline bool FileTransferPeer::isActive() const { return m_state == FileTransferPeer::Starting || m_state == FileTransferPeer::Request || m_socket.isConnected(); }
inline bool FileTransferPeer::isTransferCompleted() const { return m_state == FileTransferPeer::Completed; }
inline void FileTransferPeer::setRemoteUserId( VNumber new_value ) { m_remoteUserId = new_value; }
inline VNumber FileTransferPeer::remoteUserId() const { return m_socket.userId() != ID_INVALID ? m_socket.userId() : m_remoteUserId; }

#endif // BEEBEEP_FILETRANSFERSERVERPEER_H
