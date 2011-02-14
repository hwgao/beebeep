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

#ifndef BEEBEEP_GUIUSERLIST_H
#define BEEBEEP_GUIUSERLIST_H

#include <QListWidget>
#include "User.h"
#include "BeeUtils.h"


class GuiUserList : public QListWidget
{
  Q_OBJECT

public:
  enum UserDataType { UserId = Qt::UserRole, Username, UserNickname, UserHostAddress, UserChatName, UnreadMessages, UserStatus, UserStatusDescription };

  GuiUserList( QWidget* parent = 0 );
  virtual QSize sizeHint() const;
  void setUser( const User&, int unread_messages );
  void removeUser( const User& );
  void setUnreadMessages( const QString& chat_name, int );
  void updateUsers();
  bool nextUserWithUnreadMessages();

signals:
  void chatSelected( int, const QString& );
  void stringToShow( const QString&, int );

protected slots:
  void userDoubleClicked( QListWidgetItem* );
  void showUserInfo( QListWidgetItem* );

private:
  inline QIcon userIcon( int unread_messages, int user_status ) const;
  void updateItem( QListWidgetItem* );
  QListWidgetItem* widgetItem( UserDataType, const QString& );

};


// Inline Functions

inline QIcon GuiUserList::userIcon( int unread_messages, int user_status ) const
{ return unread_messages > 0 ?  QIcon( ":/images/chat.png" ) : Bee::userStatusIcon( user_status ); }

#endif // BEEBEEP_GUIUSERLIST_H
