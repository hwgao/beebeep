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

#ifndef BEEBEEP_FILETRANSFER_H
#define BEEBEEP_FILETRANSFER_H

#include "Config.h"
#include "FileInfo.h"
#include "User.h"
class FileTransferPeer;


class FileTransfer : public QTcpServer
{
  Q_OBJECT

public:
  explicit FileTransfer( QObject *parent = 0 );

  bool startListener();
  void stopListener();
  bool isWorking() const;

  FileInfo addFile( const QFileInfo& );
  FileInfo fileInfo( VNumber ) const;
  FileInfo fileInfo( const QString& file_absolute_path ) const;
  void downloadFile( const FileInfo& );
  bool cancelTransfer( VNumber peer_id );

  inline void clearFiles();

signals:
  void message( const User&, const FileInfo&, const QString& );
  void progress( const User&, const FileInfo&, FileSizeType );

protected:
  void incomingConnection( int );
  inline VNumber newFileId();
  void resetServerFiles();
  FileTransferPeer* peer( VNumber ) const;

protected slots:
  void stopUpload();
  void stopDownload();
  void checkFileTransferRequest( VNumber, const QByteArray& );
  void peerDestroyed();

private:
  VNumber m_id;
  QList<FileInfo> m_files;
  QList<FileTransferPeer*> m_peers;

};


// Inline Functions
inline bool FileTransfer::isWorking() const { return isListening(); }
inline void FileTransfer::clearFiles() { m_files.clear(); }
inline VNumber FileTransfer::newFileId() { return ++m_id; }


#endif // BEEBEEP_FILETRANSFERSERVER_H