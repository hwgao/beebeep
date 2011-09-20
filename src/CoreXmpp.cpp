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

#include "ColorManager.h"
#include "Core.h"
#include "Protocol.h"
#include "XmppManager.h"


void Core::parseXmppMessage( const QString& user_path, const Message& m )
{
  User u = m_users.find( user_path );
  if( !u.isValid() )
  {
    qWarning() << "XMPP> invalid user" << user_path << "found while parsing message";
    return;
  }

  switch( m.type() )
  {
  case Message::System:
    dispatchSystemMessage( ID_DEFAULT_CHAT, u.id(), m.text(), DispatchToAllChatsWithUser );
    break;
  case Message::User:
  case Message::Chat:
    parseMessage( u, m );
    break;
  default:
    qWarning() << "XMPP> core cannot parse the message with type" << m.type();
    break;
  }
}

void Core::checkXmppUser( const User& user_to_check )
{
  User u = m_users.find( user_to_check.path() );
  if( u.isValid() )
  {
    qDebug() << "XMPP>" << u.path() << "already exists";
    return;
  }

  u = user_to_check;
  u.setId( Protocol::instance().newId() );
  u.setColor( ColorManager::instance().unselectedQString() );
  qDebug() << "XMPP> new user connected:" << u.path() << "with color" << u.color();
  createPrivateChat( u );
  m_users.setUser( u );
}


void Core::sendXmppChatMessage( const QString& user_path, const Message& msg )
{
  mp_xmppManager->sendMessage( user_path, msg );
}

