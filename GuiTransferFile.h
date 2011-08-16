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

#ifndef BEEBEEP_GUITRANSFERFILE_H
#define BEEBEEP_GUITRANSFERFILE_H


#include "Config.h"
class FileInfo;
class FileTransfer;
class User;


class GuiTransferFile : public QTreeWidget
{
  Q_OBJECT

public:
  enum ColumnType { ColumnFile = 0, ColumnUser, ColumnProgress };
  enum FileDataType { FileId = Qt::UserRole, FilePath };

  GuiTransferFile( QWidget* parent = 0 );

public slots:
  void setProgress( const User&, const FileInfo&, FileSizeType );
  void setMessage( const User&, const FileInfo&, const QString& );

signals:
  void transferCancelled( VNumber );

protected:
  QTreeWidgetItem* findItem( VNumber );
  void showProgress( QTreeWidgetItem*, const FileInfo&, FileSizeType );

private slots:
  void cancelTransfer();


};


#endif // BEEBEEP_GUITRANSFERFILE_H
