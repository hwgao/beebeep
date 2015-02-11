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
#include "GuiGroupList.h"
#include "UserManager.h"


GuiGroupList::GuiGroupList( QWidget* parent )
  : QTreeWidget( parent )
{
  setObjectName( "GuiGroupList" );

  setColumnCount( 1 );
  header()->hide();
  setRootIsDecorated( false );
  setSortingEnabled( true );

  setContextMenuPolicy( Qt::CustomContextMenu );
  setMouseTracking( true );

  m_selectedGroupId = ID_INVALID;

  mp_actCreateGroup = new QAction( QIcon( ":/images/group-add.png" ), tr( "Create group" ), this );
  connect( mp_actCreateGroup, SIGNAL( triggered() ), this, SIGNAL( createGroupRequest() ) );

  mp_actEditGroup = new QAction( QIcon( ":/images/group-edit.png" ), tr( "Edit group" ), this );
  connect( mp_actEditGroup, SIGNAL( triggered() ), this, SLOT( editGroupSelected() ) );

  mp_actOpenChat = new QAction( QIcon( ":/images/chat.png" ), tr( "Open chat" ), this );
  connect( mp_actOpenChat, SIGNAL( triggered() ), this, SLOT( openGroupChatSelected() ) );

  connect( this, SIGNAL( itemDoubleClicked( QTreeWidgetItem*, int ) ), this, SLOT( checkItemDoubleClicked( QTreeWidgetItem*, int ) ) );
  connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ), this, SLOT( showGroupMenu( const QPoint& ) ) );
}

QSize GuiGroupList::sizeHint() const
{
  return QSize( 140, 300 );
}

void GuiGroupList::loadGroups()
{
  if( topLevelItemCount() > 0 )
    clear();

  foreach( Group g, UserManager::instance().groups() )
  {
    GuiGroupItem* group_item = new GuiGroupItem( this );
    group_item->init( g.id(), 0, true );
    group_item->updateGroup( g );
  }
}

void GuiGroupList::updateGroup( VNumber group_id )
{
  Group g = UserManager::instance().group( group_id );
  if( !g.isValid() )
    return;

  GuiGroupItem* group_item = itemFromId( group_id );
  if( !group_item )
  {
    group_item = new GuiGroupItem( this );
    group_item->init( g.id(), ID_INVALID, true );
  }
  group_item->updateGroup( g );
}

GuiGroupItem* GuiGroupList::itemFromId( VNumber item_id )
{
  GuiGroupItem* item;
  QTreeWidgetItemIterator it( this );
  while( *it )
  {
    item = (GuiGroupItem*)(*it);
    if( item->itemId() == item_id )
      return item;
    ++it;
  }
  return 0;
}

void GuiGroupList::checkItemDoubleClicked( QTreeWidgetItem* item, int )
{
  if( !item )
    return;

  GuiGroupItem* group_item = (GuiGroupItem*)item;
  if( group_item->isGroup() )
    emit openChatForGroupRequest( group_item->itemId() );
}

void GuiGroupList::showGroupMenu( const QPoint& p )
{
  QTreeWidgetItem* item = itemAt( p );
  if( !item )
  {
    QMenu menu;
    menu.addAction( mp_actCreateGroup );
    menu.exec( QCursor::pos() );
    return;
  }

  GuiGroupItem* group_item = (GuiGroupItem*)item;

  if( group_item->isGroup() )
  {
    m_selectedGroupId = group_item->itemId();
    QMenu menu;
    menu.addAction( mp_actOpenChat );
    menu.setDefaultAction( mp_actOpenChat );
    menu.addSeparator();
    menu.addAction( mp_actEditGroup );
    menu.exec( QCursor::pos() );
  }
  else
  {
    emit showVCardRequest( group_item->itemId() );
  }
}

void GuiGroupList::openGroupChatSelected()
{
  if( m_selectedGroupId != ID_INVALID )
  {
    emit openChatForGroupRequest( m_selectedGroupId );
    m_selectedGroupId = ID_INVALID;
  }
}

void GuiGroupList::editGroupSelected()
{
  if( m_selectedGroupId != ID_INVALID )
  {
    emit editGroupRequest( m_selectedGroupId );
    m_selectedGroupId = ID_INVALID;
  }
}