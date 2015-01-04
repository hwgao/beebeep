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

#ifndef BEEBEEP_CHATMANAGER_H
#define BEEBEEP_CHATMANAGER_H

#include "Chat.h"


class ChatManager
{
// Singleton Object
  static ChatManager* mp_instance;
  friend class Core;

public:
  inline Chat defaultChat( bool read_all_messages  );
  Chat chat( VNumber ) const;
  Chat chat( VNumber chat_id, bool read_all_messages );
  Chat privateChatForUser( VNumber user_id ) const;
  Chat groupChat( const QString& ) const;

  void setChat( const Chat& );
  inline const QList<Chat>& constChatList() const;
  QStringList chatNamesToStringList( bool add_default_chat ) const;

  bool hasName( const QString& ) const;
  int unreadMessages() const;
  bool isGroupChat( VNumber ) const;

  QList<Chat> groupChatForUser( VNumber ) const;

  void checkSavedChats(); // after a load completed
  void autoLinkSavedChatWithSameNickname( const Chat& );
  inline QString chatSavedText( const QString& ) const;
  inline bool chatHasSavedText( const QString& ) const;
  inline void removeSavedTextFromChat( const QString& );
  inline void setSavedTextToChat( const QString&, const QString& );
  inline void setLoadHistoryCompleted( bool );
  inline bool isLoadHistoryCompleted() const;
  void updateChatSavedText( const QString& old_chat_name, const QString& new_chat_name, bool add_to_new );
  inline const QMap<QString, QString>& constHistoryMap() const;

  void changePrivateChatNameAfterUserNameChanged( VNumber user_id, const QString& user_new_path );

  static ChatManager& instance()
  {
    if( !mp_instance )
      mp_instance = new ChatManager();
    return *mp_instance;
  }

  static void close()
  {
    if( mp_instance )
    {
      delete mp_instance;
      mp_instance = NULL;
    }
  }

protected:
  ChatManager();

  QString findPrivateChatSavedTextWithSameNickname( const QString& ) const;

  inline QList<Chat>& chatList();

private:
  QList<Chat> m_chats;
  QMap<QString, QString> m_history;
  bool m_isLoadHistoryCompleted;

};


// Inline Function
inline Chat ChatManager::defaultChat( bool read_all_messages ) { return chat( ID_DEFAULT_CHAT, read_all_messages ); }
inline const QList<Chat>& ChatManager::constChatList() const { return m_chats; }
inline QList<Chat>& ChatManager::chatList() { return m_chats; }
inline QString ChatManager::chatSavedText( const QString& chat_name ) const { return m_history.value( chat_name ); }
inline bool ChatManager::chatHasSavedText( const QString& chat_name ) const { return m_history.contains( chat_name ); }
inline void ChatManager::removeSavedTextFromChat( const QString& chat_name ) { m_history.remove( chat_name ); }
inline void ChatManager::setSavedTextToChat( const QString& chat_name, const QString& chat_text ) { m_history.insert( chat_name, chat_text ); }
inline void ChatManager::setLoadHistoryCompleted( bool new_value ) { m_isLoadHistoryCompleted = new_value; }
inline bool ChatManager::isLoadHistoryCompleted() const { return m_isLoadHistoryCompleted; }
inline const QMap<QString, QString>& ChatManager::constHistoryMap() const { return m_history; }

#endif // BEEBEEP_CHATMANAGER_H
