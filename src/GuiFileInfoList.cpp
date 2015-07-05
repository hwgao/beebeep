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
// $Id: GuiFileInfoItem.cpp 346 2015-04-05 16:12:37Z mastroddi $
//
//////////////////////////////////////////////////////////////////////

#include "GuiFileInfoList.h"
#include "FileShare.h"
#include "User.h"


GuiFileInfoList::GuiFileInfoList()
 : QObject( 0 ), mp_tree( 0 ), m_selectedFileInfoList(), m_lastFolderItem( 0 ), m_lastUserItem( 0 )
{
}

void GuiFileInfoList::initTree( QTreeWidget* tree_widget )
{
  mp_tree = tree_widget;
  mp_tree->setColumnCount( 3 );

  QStringList labels;
  labels << tr( "Shared folders and files" ) << tr( "Size" ) << tr( "Status" );
  mp_tree->setHeaderLabels( labels );

  mp_tree->sortItems( GuiFileInfoItem::ColumnFile, Qt::AscendingOrder );

  mp_tree->setAlternatingRowColors( true );
  mp_tree->setRootIsDecorated( true );
  mp_tree->setContextMenuPolicy( Qt::CustomContextMenu );
  mp_tree->setSelectionMode( QAbstractItemView::ExtendedSelection );

  QHeaderView* hv = mp_tree->header();
#if QT_VERSION >= 0x050000
  hv->setSectionResizeMode( GuiFileInfoItem::ColumnFile, QHeaderView::Stretch );
  hv->setSectionResizeMode( GuiFileInfoItem::ColumnSize, QHeaderView::ResizeToContents );
  hv->setSectionResizeMode( GuiFileInfoItem::ColumnStatus, QHeaderView::ResizeToContents );
#else
  hv->setResizeMode( GuiFileInfoItem::ColumnFile, QHeaderView::Stretch );
  hv->setResizeMode( GuiFileInfoItem::ColumnSize, QHeaderView::ResizeToContents );
  hv->setResizeMode( GuiFileInfoItem::ColumnStatus, QHeaderView::ResizeToContents );
#endif

}

void GuiFileInfoList::clearTree()
{
  if( mp_tree->topLevelItemCount() > 0 )
    mp_tree->clear();
  m_selectedFileInfoList.clear();
  m_lastFolderItem = 0;
  m_lastUserItem = 0;
}

void GuiFileInfoList::clearTreeSelection()
{
  mp_tree->clearSelection();
  m_selectedFileInfoList.clear();
}

GuiFileInfoItem* GuiFileInfoList::userItem( VNumber user_id )
{
  if( m_lastUserItem && m_lastUserItem->userId() == user_id )
    return m_lastUserItem;

  GuiFileInfoItem* item;
  QTreeWidgetItemIterator it( mp_tree );
  while( *it )
  {
    item = (GuiFileInfoItem*)(*it);
    if( item->isObjectUser() && item->userId() == user_id )
    {
      m_lastUserItem = 0;
      return item;
    }
    ++it;
  }
  return 0;
}

GuiFileInfoItem* GuiFileInfoList::createUserItem( const User& u )
{
#ifdef BEEBEEP_DEBUG
  qDebug() << "GuiFileInfoList::createUserItem for user" << u.id() << u.name();
#endif
  GuiFileInfoItem* item = new GuiFileInfoItem( mp_tree );
  item->initUser( u.id(), u.name() );
  return item;
}

GuiFileInfoItem* GuiFileInfoList::folderItem( VNumber user_id, const QString& folder_name )
{
  if( m_lastFolderItem && m_lastFolderItem->userId() == user_id && m_lastFolderItem->folder() == folder_name )
    return m_lastFolderItem;

  GuiFileInfoItem* item;
  QTreeWidgetItemIterator it( mp_tree );
  while( *it )
  {
    item = (GuiFileInfoItem*)(*it);
    if( item->isObjectFolder() && item->userId() == user_id && item->folder() == folder_name )
    {
      m_lastFolderItem = item;
      return item;
    }
    ++it;
  }
  return 0;
}

GuiFileInfoItem* GuiFileInfoList::createFolderItem( const User& u, const QString& folder_name )
{
#ifdef BEEBEEP_DEBUG
  qDebug() << "GuiFileInfoList::createFolderItem for user" << u.id() << folder_name;
#endif
  GuiFileInfoItem* item;
  if( u.isLocal() )
  {
    item = new GuiFileInfoItem( mp_tree );
  }
  else
  {
    GuiFileInfoItem* parent_item = userItem( u.id() );
    if( !parent_item )
      parent_item = createUserItem( u );
    item = new GuiFileInfoItem( parent_item );
  }

  item->initFolder( u.id(), folder_name );
  return item;
}

GuiFileInfoItem* GuiFileInfoList::fileItem( VNumber user_id, VNumber file_info_id )
{
  GuiFileInfoItem* item;
  QTreeWidgetItemIterator it( mp_tree );
  while( *it )
  {
    item = (GuiFileInfoItem*)(*it);
    if( item->isObjectFile() && item->userId() == user_id && item->fileInfoId() == file_info_id )
      return item;
    ++it;
  }
  return 0;
}

GuiFileInfoItem* GuiFileInfoList::createFileItem( const User& u, const FileInfo& file_info )
{
  GuiFileInfoItem* parent_item;
  if( file_info.shareFolder().isEmpty() )
  {
    parent_item = userItem( u.id() );
    if( !parent_item )
      parent_item = createUserItem( u );
  }
  else
  {
    parent_item = folderItem( u.id(), file_info.shareFolder() );
    if( !parent_item )
      parent_item = createFolderItem( u, file_info.shareFolder() );
  }

#ifdef BEEBEEP_DEBUG
  qDebug() << "GuiFileInfoList::createFileItem for user" << u.id() << "in folder" << parent_item->folder() << "with id" << file_info.id();
#endif
  GuiFileInfoItem* item = new GuiFileInfoItem( parent_item );
  item->initFile( u.id(), file_info );


  return item;
}

void GuiFileInfoList::addFileInfoToList( VNumber user_id, const FileInfo& fi )
{
  SharedFileInfo sfi( user_id, fi );
  if( !m_selectedFileInfoList.contains( sfi ) )
    m_selectedFileInfoList.append( sfi );
}

void GuiFileInfoList::addFileInfoListToList( VNumber user_id, const QList<FileInfo>& file_info_list )
{
  foreach( FileInfo fi, file_info_list )
    addFileInfoToList( user_id, fi );
}

void GuiFileInfoList::addItemToFileInfoList( GuiFileInfoItem* fi_item )
{
  if( !fi_item->isObjectFile() )
  {
#ifdef BEEBEEP_DEBUG
    qWarning() << "Unable to add a not file item in list:" << fi_item->text( GuiFileInfoItem::ColumnFile );
#endif
    return;
  }

  FileInfo fi = FileShare::instance().networkFileInfo( fi_item->userId(), fi_item->fileInfoId() );
  if( !fi.isValid() )
  {
#ifdef BEEBEEP_DEBUG
    qWarning() << "Unable to add an invalid file info in list:" << fi_item->text( GuiFileInfoItem::ColumnFile );
#endif
    return;
  }

  addFileInfoToList( fi_item->userId(), fi );
}

void GuiFileInfoList::parseItem( QTreeWidgetItem* tw_item )
{
  GuiFileInfoItem* item = (GuiFileInfoItem*)(tw_item);

  if( item->isObjectFile() )
  {
    addItemToFileInfoList( item );
  }
  else if( item->isObjectFolder() && item->childCount() > 0  )
  {
    QList<FileInfo> folder_file_info_list = FileShare::instance().networkFolder( item->userId(), item->folder() );
    addFileInfoListToList( item->userId(), folder_file_info_list );
  }
  else if( item->isObjectUser() )
  {
    QList<FileInfo> user_file_info_list = FileShare::instance().fileSharedFromUser( item->userId() );
    addFileInfoListToList( item->userId(), user_file_info_list );
  }
}

int GuiFileInfoList::parseSelectedItems()
{
  m_selectedFileInfoList.clear();
  QList<QTreeWidgetItem*> selected_items = mp_tree->selectedItems();
  if( selected_items.isEmpty() )
    return 0;

  foreach( QTreeWidgetItem* item, selected_items )
    parseItem( item );

  return m_selectedFileInfoList.size();
}

int GuiFileInfoList::countFileItems() const
{
  int count_file_items = 0;
  GuiFileInfoItem* item;
  QTreeWidgetItemIterator it( mp_tree );
  while( *it )
  {
    item = (GuiFileInfoItem*)(*it);
    if( item->isObjectFile() )
      count_file_items++;
    ++it;
  }
  return count_file_items;
}