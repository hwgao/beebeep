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

#include "Connection.h"
#include "Core.h"
#include "BeeUtils.h"
#include "Protocol.h"
#include "Random.h"
#include "Settings.h"
#include "Tips.h"


void Core::createDefaultChat()
{
  qDebug() << "Creating default chat";
  Chat c;
  c.setId( ID_DEFAULT_CHAT );
  c.addUser( ID_LOCAL_USER );
  QString sHtmlMsg = tr( "%1 Chat with all people." ).arg( Bee::iconToHtml( ":/images/chat.png", "*C*" ) );
  ChatMessage cm( ID_LOCAL_USER, Protocol::instance().systemMessage( sHtmlMsg ) );
  c.addMessage( cm );
  setChat( c );
}

void Core::createPrivateChat( const User& u )
{
  qDebug() << "Creating private chat room for user" << u.path();
  QList<VNumber> user_list;
  user_list.append( u.id() );
  Chat c = Protocol::instance().createChat( user_list );
  QString sHtmlMsg = tr( "%1 Chat with %2." ).arg( Bee::iconToHtml( ":/images/chat.png", "*C*" ), u.path() );
  ChatMessage cm( u.id(), Protocol::instance().systemMessage( sHtmlMsg ) );
  c.addMessage( cm );
  setChat( c );
}

Chat Core::chat( VNumber chat_id, bool read_all_messages )
{
  QList<Chat>::iterator it = m_chats.begin();
  while( it != m_chats.end() )
  {
    if( chat_id == (*it).id() )
    {
      if( read_all_messages )
        (*it).readAllMessages();
      return *it;
    }
    ++it;
  }
  return Chat();
}

Chat Core::privateChatForUser( VNumber user_id ) const
{
  if( user_id == ID_LOCAL_USER )
    return chat( ID_DEFAULT_CHAT );
  QList<Chat>::const_iterator it = m_chats.begin();
  while( it != m_chats.end() )
  {
    if( (*it).isPrivateForUser( user_id ) )
      return *it;
    ++it;
  }
  return Chat();
}

Chat Core::chat( VNumber chat_id ) const
{
  QList<Chat>::const_iterator it = m_chats.begin();
  while( it != m_chats.end() )
  {
    if( chat_id == (*it).id() )
      return *it;
    ++it;
  }
  return Chat();
}

void Core::setChat( const Chat& c )
{
  QList<Chat>::iterator it = m_chats.begin();
  while( it != m_chats.end() )
  {
    if( (*it).id() == c.id() )
    {
      (*it) = c;
      return;
    }
    ++it;
  }
  m_chats.append( c );
}

int Core::sendChatMessage( VNumber chat_id, const QString& msg )
{
  if( !isConnected() )
  {
    dispatchSystemMessage( chat_id, ID_LOCAL_USER, tr( "%1 Unable to send the message: you are not connected." )
                           .arg( Bee::iconToHtml( ":/images/red-ball.png", "*X*" ) ), DispatchToChat );
    return 0;
  }

  if( msg.isEmpty() )
    return 0;

  Message m = Protocol::instance().chatMessage( msg );
  m.setData( Settings::instance().chatFontColor() );

  int messages_sent = 0;

  if( chat_id == ID_DEFAULT_CHAT )
  {
    foreach( Connection *c, m_connections )
    {
      if( !c->sendMessage( m ) )
        dispatchSystemMessage( ID_DEFAULT_CHAT, ID_LOCAL_USER, tr( "%1 Unable to send the message to %2." )
                               .arg( Bee::iconToHtml( ":/images/red-ball.png", "*X*" ), user( c->userId() ).path() ), DispatchToChat );
      else
        messages_sent += 1;
    }
  }
  else
  {
    m.addFlag( Message::Private );
    Chat from_chat = chat( chat_id );
    QList<VNumber> user_list = from_chat.usersId();
    foreach( VNumber user_id, user_list )
    {
      if( user_id == ID_LOCAL_USER )
        continue;
      Connection* c = connection( user_id );
      if( c && c->sendMessage( m ) )
        messages_sent += 1;
      else
        dispatchSystemMessage( chat_id, ID_LOCAL_USER, tr( "%1 Unable to send the message to %2." )
                               .arg( Bee::iconToHtml( ":/images/red-ball.png", "*X*" ), user( user_id ).path() ), DispatchToChat );
    }
  }

  ChatMessage cm( ID_LOCAL_USER, m );
  dispatchToChat( cm, chat_id );
  return messages_sent;
}

void Core::sendWritingMessage( VNumber chat_id )
{
  if( !isConnected() )
    return;

  Chat from_chat = chat( chat_id );
  QList<VNumber> user_list = from_chat.usersId();
  foreach( VNumber user_id, user_list )
  {
    if( user_id == ID_LOCAL_USER )
      continue;
    Connection* c = connection( user_id );
    if( c )
    {
      qDebug() << "Sending Writing Message to" << c->peerAddress() << c->peerPort();
      c->sendData( Protocol::instance().writingMessage() );
    }
  }
}

void Core::showTipOfTheDay()
{
  QString tip_of_the_day = QString( "%1 %2" ).arg( Bee::iconToHtml( ":/images/tip.png", "*T*" ),
                                                   qApp->translate( "Tips", BeeBeepTips[ Random::number( 0, (BeeBeepTipsSize-1) ) ] ) );
  dispatchSystemMessage( ID_DEFAULT_CHAT, ID_LOCAL_USER, tip_of_the_day, DispatchToChat );
}



