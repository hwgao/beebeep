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

#include "Avatar.h"
#include "BeeUtils.h"
#include "ChatManager.h"
#include "ColorManager.h"
#include "Connection.h"
#include "Core.h"
#include "FileShare.h"
#include "NetworkManager.h"
#include "Protocol.h"
#include "Settings.h"
#include "UserManager.h"


Connection* Core::connection( VNumber user_id ) const
{
  foreach( Connection* c, m_connections )
  {
    if( c->userId() == user_id )
      return c;
  }
  return 0;
}

bool Core::hasConnection( const QHostAddress& sender_ip, int sender_port ) const
{
  foreach( Connection* c, m_connections )
  {
    if( (sender_port == -1 || c->peerPort() == sender_port) && c->peerAddress() == sender_ip )
    {
      if( c->isConnected() || c->isConnecting() )
      {
#ifdef BEEBEEP_DEBUG
        qDebug() << "Connection from" << qPrintable( sender_ip.toString() ) << sender_port << "is already opened";
#endif
        return true;
      }
    }
  }

  return false;
}

void Core::checkNetworkAddress( const NetworkAddress& na )
{
  if( na == Settings::instance().localUser().networkAddress() )
    return;

  newPeerFound( na.hostAddress(), na.hostPort() );
}

void Core::newPeerFound( const QHostAddress& sender_ip, int sender_port )
{
  if( !isConnected() )
    return;

  NetworkAddress na( sender_ip, sender_port );
  if( isUserConnected( na ) )
    return;

  qDebug() << "Connecting to new peer" << qPrintable( sender_ip.toString() ) << sender_port;

  Connection *c = new Connection( this );
  setupNewConnection( c );
  c->connectToNetworkAddress( NetworkAddress( sender_ip, sender_port ) );
}

void Core::checkNewConnection( qintptr socket_descriptor )
{
  // Has connection never return true because peer port is always different.
  // It comes from Listener. If I want to prevent multiple users from single
  // ip, I can pass -1 to peer_port and check only host address

  Connection *c = new Connection( this );
  c->initSocket( socket_descriptor );

  qDebug() << "New connection from" << qPrintable( c->networkAddress().toString() );

  if( Settings::instance().preventMultipleConnectionsFromSingleHostAddress() )
  {
    if( hasConnection( c->peerAddress(), -1 ) )
    {
      qWarning() << qPrintable( c->networkAddress().toString() ) << "is already connected and blocked by prevent multiple connections";
      closeConnection( c );
      return;
    }
  }

  setupNewConnection( c );
}

void Core::setupNewConnection( Connection *c )
{
#ifdef BEEBEEP_DEBUG
  if( !c->peerAddress().isNull() )
    qDebug() << "Connecting SIGNAL/SLOT to connection from" << qPrintable( c->networkAddress().toString() );
#endif
  connect( c, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SLOT( setConnectionError( QAbstractSocket::SocketError ) ) );
  connect( c, SIGNAL( disconnected() ), this, SLOT( setConnectionClosed() ) );
  connect( c, SIGNAL( abortRequest() ), this, SLOT( setConnectionClosed() ) );
  connect( c, SIGNAL( authenticationRequested( const QByteArray& ) ), this, SLOT( checkUserAuthentication( const QByteArray& ) ) );
}

void Core::addConnectionReadyForUse( Connection* c )
{
#ifdef BEEBEEP_DEBUG
  qDebug() << "Connection from" << qPrintable( c->networkAddress().toString() ) << "is ready for use";
#endif
  connect( c, SIGNAL( newMessage( VNumber, const Message& ) ), this, SLOT( parseMessage( VNumber, const Message& ) ) );
  m_connections.append( c );
}

void Core::setConnectionError( QAbstractSocket::SocketError se )
{
  Connection* c = qobject_cast<Connection*>( sender() );
  if( c )
  {
    qWarning() << "Connection from" << qPrintable( c->networkAddress().toString() ) << "has an error:" << c->errorString() << "-" << (int)se;
    closeConnection( c );
  }
  else
    qWarning() << "Connection error" << se << "occurred but the object caller is invalid";
}

void Core::setConnectionClosed()
{
  Connection* c = qobject_cast<Connection*>( sender() );
  if( c )
    closeConnection( c );
  else
    qWarning() << "Connection closed but the object caller is invalid";
}

void Core::closeConnection( Connection *c )
{
  m_connections.removeOne( c );

  if( c->userId() != ID_INVALID )
  {
    User u = UserManager::instance().findUser( c->userId() );
    if( u.isValid() )
    {
      qDebug() << "User" << qPrintable( u.path() ) << "goes offline";
      u.setStatus( User::Offline );
      UserManager::instance().setUser( u );

      emit userChanged( u );
      emit userConnectionStatusChanged( u );

      Chat default_chat = ChatManager::instance().defaultChat();
      if( default_chat.removeUser( u.id() ) )
        ChatManager::instance().setChat( default_chat );

      FileShare::instance().removeFromNetwork( c->userId() );
      emit fileShareAvailable( u );
    }
    else
      qWarning() << "User" << c->userId() << "not found while closing connection";
  }

  c->disconnect();
  c->abortConnection();
  // do not delete later connection... socket notifier in qabstractsocket.cpp can crash
}

void Core::checkUserAuthentication( const QByteArray& auth_byte_array )
{
  Connection* c = qobject_cast<Connection*>( sender() );
  if( !c )
  {
    qWarning() << "Unable to check authentication for an invalid connection object";
    return;
  }

  Message m = Protocol::instance().toMessage( auth_byte_array, c->protoVersion() );
  if( !m.isValid() )
  {
    qWarning() << "Core has received an invalid HELLO from" << qPrintable( c->networkAddress().toString() );
    closeConnection( c );
    return;
  }

  if( m.type() != Message::Hello )
  {
    qWarning() << "Core is waiting for HELLO, but another message type" << m.type() << "is arrived from" << qPrintable( c->networkAddress().toString() );
    closeConnection( c );
    return;
  }

  User u = Protocol::instance().createUser( m, c->peerAddress() );
  if( !u.isValid() )
  {
    qWarning() << "Unable to create a new user (invalid protocol or password) from the message arrived from:" << qPrintable( c->networkAddress().toString() );
    closeConnection( c );
    return;
  }
  else
    qDebug() << qPrintable( u.path() ) << "has completed the authentication";

  User user_found;
  bool user_has_same_account_name = false;

  if( Settings::instance().trustSystemAccount() )
  {
    user_found = UserManager::instance().findUserByAccountName( u.accountName(), u.domainName() );
    if( user_found.isValid() )
    {
      user_has_same_account_name = true;
      qDebug() << "User found in list with account name:" << qPrintable( u.accountName() );
      if( !u.domainName().isEmpty() )
        qDebug() << "User found in list also with domain name:" << qPrintable( u.domainName() );
    }
  }
  else
  {
    user_found = UserManager::instance().findUserByNickname( u.name() );
    if( !user_found.isValid() )
    {
      if( Settings::instance().trustUserHash() )
      {
        user_found = UserManager::instance().findUserByHash( u.hash() );
        if( user_found.isValid() )
          qDebug() << "User found in list with hash:" << qPrintable( u.hash() );
      }
    }
    else
      qDebug() << "User found in list with name:" << qPrintable( u.name() );
  }

  bool user_path_changed = false;

  if( user_found.isValid() )
  {
    QString sAlertMsg;
    user_path_changed = (u.path() != user_found.path());
    if( user_found.isLocal() )
    {
      if( user_path_changed )
      {
        if( user_has_same_account_name )
        {
          sAlertMsg = tr( "%1 Connection closed to user %2 because it uses your account name: %3." )
                          .arg( Bee::iconToHtml( ":/images/warning.png", "*E*" ), u.path(), u.accountName() );
        }
        else
        {
          sAlertMsg = tr( "%1 Connection closed to user %2 because it uses your nickname: %3." )
                          .arg( Bee::iconToHtml( ":/images/warning.png", "*E*" ), u.path(), u.name() );
        }

        dispatchSystemMessage( ID_DEFAULT_CHAT, user_found.id(), sAlertMsg, DispatchToDefaultAndPrivateChat, ChatMessage::Connection );
      }

      qWarning() << "User with account" << qPrintable( u.accountName() ) << "and path" << qPrintable( u.path() ) << "is recognized to be Local";
      closeConnection( c );
      return;
    }

    if( isUserConnected( user_found.id() ) )
    {
      if( user_path_changed )
      {
        if( user_has_same_account_name )
        {
          sAlertMsg = tr( "%1 Connection closed to user %2 because it uses same account name of the already connected user %3: %4." )
                        .arg( Bee::iconToHtml( ":/images/warning.png", "*E*" ), u.path(), user_found.path(), u.accountName() );
        }
        else
        {
          sAlertMsg = tr( "%1 Connection closed to user %2 because it uses same nickname of the already connected user %3: %4." )
                        .arg( Bee::iconToHtml( ":/images/warning.png", "*E*" ), u.path(), user_found.path(), u.name() );
        }
      }

      dispatchSystemMessage( ID_DEFAULT_CHAT, user_found.id(), sAlertMsg, DispatchToDefaultAndPrivateChat, ChatMessage::Connection );
      qDebug() << "User with account" << qPrintable( u.accountName() ) << "and path" << qPrintable( u.path() ) << "is already connected with account name" << user_found.accountName() << "path" << user_found.path();
      c->setUserId( ID_INVALID );
      closeConnection( c );
      return;
    }

    if( user_path_changed )
      qDebug() << "On connection old user found" << qPrintable( user_found.path() ) << "and associated to" << qPrintable( u.path() );
    else
      qDebug() << "User" << qPrintable( u.path() ) << "reconnected";

    u.setId( user_found.id() );
    u.setIsFavorite( user_found.isFavorite() );
    u.setColor( user_found.color() );
  }
  else
    qDebug() << "New user is connected from" << qPrintable( u.path() );

  if( !ColorManager::instance().isValidColor( u.color() ) )
    u.setColor( ColorManager::instance().unselectedQString() );
  u.setProtocolVersion( c->protoVersion() );
  UserManager::instance().setUser( u );

#ifdef BEEBEEP_DEBUG
  qDebug() << "User" << qPrintable( u.path() ) << "added with id" << u.id() << "and color" << qPrintable( u.color() );
#endif

  Chat default_chat = ChatManager::instance().defaultChat();
  if( default_chat.addUser( u.id() ) )
    ChatManager::instance().setChat( default_chat );

  Chat private_chat = ChatManager::instance().privateChatForUser( u.id() );
  if( private_chat.isValid() )
  {
    if( user_found.isValid() && u.name() != user_found.name() )
    {
      ChatManager::instance().changePrivateChatNameAfterUserNameChanged( u.id(), u.name() );
      showUserNameChanged( u, user_found.name() );
    }
  }
  else
    createPrivateChat( u );

  c->setReadyForUse( u.id() );
  addConnectionReadyForUse( c );

  emit userChanged( u );
  emit userConnectionStatusChanged( u );
  showMessage( tr( "%1 users connected" ).arg( connectedUsers() ), 3000 );

  if( !Settings::instance().localUser().vCard().hasOnlyNickName() )
  {
    if( c->protoVersion() > 1 )
    {
#ifdef BEEBEEP_DEBUG
      qDebug() << "Sending my VCard to" << qPrintable( u.path() );
#endif
      c->sendMessage( Protocol::instance().localVCardMessage() );
    }
  }

  if( user_found.isValid() )
    checkGroupChatAfterUserReconnect( u );

  checkOfflineMessagesForUser( u );
  if( Settings::instance().useHive() && u.protocolVersion() >= HIVE_PROTO_VERSION )
    sendLocalConnectedUsersTo( u );
}
