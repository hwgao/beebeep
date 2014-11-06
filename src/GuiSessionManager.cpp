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

#include "ChatManager.h"
#include "GuiChatMessage.h"
#include "GuiSessionManager.h"
#include "Settings.h"

GuiSessionManager* GuiSessionManager::mp_instance = NULL;


GuiSessionManager::GuiSessionManager()
 : m_chatMap()
{
}

QString GuiSessionManager::chatStoredText( const QString& chat_name )
{
  return m_chatMap.value( chat_name );
}

bool GuiSessionManager::save()
{
  QString file_name = QString( "%1/%2" ).arg( Settings::instance().chatSaveDirectory() ).arg( "beebeep.dat" );
  QFile file( file_name );
  if( !file.open( QIODevice::WriteOnly ) )
  {
    qWarning() << "Unable to open file" << file.fileName() << ": saving session aborted";
    return false;
  }

  QDataStream stream( &file );

  QStringList file_header;
  file_header << Settings::instance().programName();
  file_header << Settings::instance().version( false );
  file_header << QString::number( Settings::instance().protoVersion() );

  stream << file_header;

  saveChats( &stream );

  file.close();

  return true;
}

void GuiSessionManager::saveChats( QDataStream* stream )
{
  int num_of_chats = ChatManager::instance().constChatList().count();
 (*stream) << num_of_chats;

  QString chat_footer = QString( "<font color=gray><b>*** %1 %2 ***</b></font><br />" ).arg( QObject::tr( "Saved in" ) ).arg( QDateTime::currentDateTime().toString( Qt::SystemLocaleShortDate ) );

  foreach( Chat c, ChatManager::instance().constChatList() )
  {
    qDebug() << "Saving chat" << c.name();
    (*stream) << (QString)c.name();
    QString html_text = GuiChatMessage::chatToHtml( c );
    html_text.prepend( QString( "<font color=gray><b>*** %1 %2 ***</b></font><br />" ).arg( QObject::tr( "Started in" ) ).arg( c.dateTimeStarted().toString( Qt::SystemLocaleShortDate ) ) );
    html_text.append( chat_footer );
    if( chatHasStoredText( c.name() ) )
      html_text.prepend( chatStoredText( c.name() ) );
    (*stream) << html_text;
  }
}

bool GuiSessionManager::load()
{
  QString file_name = QString( "%1/%2" ).arg( Settings::instance().chatSaveDirectory() ).arg( "beebeep.dat" );
  QFile file( file_name );

  if( !file.open( QIODevice::ReadOnly ) )
  {
    qWarning() << "Unable to open file" << file.fileName() << ": loading session aborted";
    return false;
  }

  QDataStream stream( &file );

  QStringList file_header;

  stream >> file_header;

  loadChats( &stream );

  file.close();

  return true;
}

void GuiSessionManager::loadChats( QDataStream* stream )
{
  int num_of_chats = 0;
  (*stream) >> num_of_chats;

  // Save chats

  if( num_of_chats <= 0 )
    return;

  QString chat_name;
  QString chat_text;
  for( int i = 0; i < num_of_chats; i++ )
  {
    (*stream) >> chat_name;
    (*stream) >> chat_text;

    qDebug() << "Loading chat" << chat_name;
    m_chatMap.insert( chat_name, chat_text );
  }
}

