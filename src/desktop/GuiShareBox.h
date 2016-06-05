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

#ifndef BEEBEEP_GUISHAREBOX_H
#define BEEBEEP_GUISHAREBOX_H

#include "ui_GuiShareBox.h"
#include "GuiShareBoxFileInfoList.h"
class User;


class GuiShareBox : public QWidget, private Ui::GuiShareBoxWidget
{
  Q_OBJECT

public:
  explicit GuiShareBox( QWidget *parent = 0 );
  void updateShareBoxes();
  void updateUser( const User& );

signals:
  void shareBoxRequest( VNumber, const QString& );
  void openUrlRequest( const QUrl& );
  void shareBoxDownloadRequest( VNumber, const FileInfo&, const QString& );

public slots:
  void updateBox( const User&, const QString&, const QList<FileInfo>& );
  void onShareFolderUnavailable(  const User&, const QString& );

protected slots:
  void updateMyBox();
  void updateOutBox();
  void enableMyUpdateButton();
  void enableOutUpdateButton();

  void onMyItemDoubleClicked( QTreeWidgetItem*, int );
  void onOutItemDoubleClicked( QTreeWidgetItem*, int );
  void onEnableMyShareBoxClicked();
  void onShareBoxSelected( int );

  void selectMyShareBoxFolder();

  void dropInMyBox( const QString& );
  void dropInOutBox( const QString& );

protected:
  void updateMyBox( const QString&, const QList<FileInfo>& );
  void updateOutBox( const User&, const QString&, const QList<FileInfo>& );

private:
  QAction* mp_actUpdate;
  QAction* mp_actDownload;
  QAction* mp_actUpload;

  VNumber m_userId;
  QString m_myCurrentFolder;
  QString m_outCurrentFolder;

  QSplitter* mp_splitter;

};

#endif // BEEBEEP_GUISHAREBOX_H
