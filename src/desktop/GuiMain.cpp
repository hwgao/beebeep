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

#include "AudioManager.h"
#include "Core.h"
#include "BeeApplication.h"
#include "BeeUtils.h"
#include "ChatManager.h"
#include "FileDialog.h"
#include "FileShare.h"
#include "GuiAddUser.h"
#include "GuiAskPassword.h"
#include "GuiChat.h"
#include "GuiChatList.h"
#include "GuiCreateGroup.h"
#include "GuiConfig.h"
#include "GuiEditVCard.h"
#include "GuiFileSharing.h"
#include "GuiFloatingChat.h"
#include "GuiGroupList.h"
#include "GuiHome.h"
#include "GuiLanguage.h"
#include "GuiLog.h"
#include "GuiPluginManager.h"
#include "GuiSavedChat.h"
#include "GuiSavedChatList.h"
#include "GuiScreenShot.h"
#include "GuiNetwork.h"
#include "GuiShareBox.h"
#ifdef BEEBEEP_USE_SHAREDESKTOP
  #include "GuiShareDesktop.h"
#endif
#include "GuiShareLocal.h"
#include "GuiShareNetwork.h"
#include "GuiShortcut.h"
#include "GuiSystemTray.h"
#include "GuiTransferFile.h"
#include "GuiUserList.h"
#include "GuiMain.h"
#include "GuiVCard.h"
#include "GuiWizard.h"
#include "GuiWorkgroups.h"
#include "PluginManager.h"
#include "Protocol.h"
#include "SaveChatList.h"
#include "Settings.h"
#include "ShortcutManager.h"
#include "SpellChecker.h"
#include "UserManager.h"
#ifdef Q_OS_WIN
  #include <windows.h>
#endif


GuiMain::GuiMain( QWidget *parent )
 : QMainWindow( parent ), m_floatingChats()
{
  setObjectName( "GuiMainWindow" );
  setWindowIcon( QIcon( ":/images/beebeep.png" ) );
  mp_core = new Core( this );

  // Create a status bar before the actions and the menu
  (void) statusBar();

  mp_tabMain = new QTabWidget( this );
  mp_tabMain->setObjectName( "GuiTabMain" );
  mp_tabMain->setTabPosition( QTabWidget::South );
  setCentralWidget( mp_tabMain );

  mp_fileSharing = 0;
  mp_screenShot = 0;
  mp_log = 0;
  m_unreadActivities = 0;

  mp_barMain = addToolBar( tr( "Show the main tool bar" ) );
  mp_barMain->setObjectName( "GuiMainToolBar" );
  mp_barMain->setIconSize( Settings::instance().mainBarIconSize() );
  mp_barMain->toggleViewAction()->setVisible( false );

  mp_trayIcon = new GuiSystemTray( this );

  m_lastUserStatus = User::Online;
  m_forceShutdown = false;
  m_autoConnectOnInterfaceUp = false;
  m_prevActivatedState = true;
  m_coreIsConnecting = false;

  createActions();
  createMainWidgets();
  createMenus();
  createToolAndMenuBars();
  updadePluginMenu();

  connect( mp_tabMain, SIGNAL( currentChanged( int ) ), this, SLOT( onMainTabChanged( int ) ) );

  connect( mp_core, SIGNAL( newChatMessage( const Chat&, const ChatMessage& ) ), this, SLOT( onNewChatMessage( const Chat&, const ChatMessage& ) ) );
  connect( mp_core, SIGNAL( fileDownloadRequest( const User&, const FileInfo& ) ), this, SLOT( downloadFile( const User&, const FileInfo& ) ) );
  connect( mp_core, SIGNAL( folderDownloadRequest( const User&, const QString&, const QList<FileInfo>& ) ), this, SLOT( downloadFolder( const User&, const QString&, const QList<FileInfo>& ) ) );
  connect( mp_core, SIGNAL( userChanged( const User& ) ), this, SLOT( onUserChanged( const User& ) ) );
  connect( mp_core, SIGNAL( userIsWriting( const User&, VNumber ) ), this, SLOT( showWritingUser( const User&, VNumber ) ) );
  connect( mp_core, SIGNAL( fileTransferProgress( VNumber, const User&, const FileInfo&, FileSizeType ) ), this, SLOT( onFileTransferProgress( VNumber, const User&, const FileInfo&, FileSizeType ) ) );
  connect( mp_core, SIGNAL( fileTransferMessage( VNumber, const User&, const FileInfo&, const QString& ) ), this, SLOT( onFileTransferMessage( VNumber, const User&, const FileInfo&, const QString& ) ) );
  connect( mp_core, SIGNAL( fileTransferCompleted( VNumber, const User&, const FileInfo& ) ), this, SLOT( onFileTransferCompleted( VNumber, const User&, const FileInfo& ) ) );
  connect( mp_core, SIGNAL( fileShareAvailable( const User& ) ), this, SLOT( showSharesForUser( const User& ) ) );
  connect( mp_core, SIGNAL( chatChanged( const Chat& ) ), this, SLOT( onChatChanged( const Chat& ) ) );
  connect( mp_core, SIGNAL( savedChatListAvailable() ), this, SLOT( loadSavedChatsCompleted() ) );
  connect( mp_core, SIGNAL( updateGroup( VNumber ) ), this, SLOT( checkGroup( VNumber ) ) );
  connect( mp_core, SIGNAL( userConnectionStatusChanged( const User& ) ), this, SLOT( showConnectionStatusChanged( const User& ) ) );
  connect( mp_core, SIGNAL( networkInterfaceIsDown() ), this, SLOT( onNetworkInterfaceDown() ) );
  connect( mp_core, SIGNAL( networkInterfaceIsUp() ), this, SLOT( onNetworkInterfaceUp() ) );
  connect( mp_core, SIGNAL( chatReadByUser( const Chat&, const User& ) ), this, SLOT( onChatReadByUser( const Chat&, const User& ) ) );
  connect( mp_core, SIGNAL( localUserIsBuzzedBy( const User& ) ), this, SLOT( showBuzzFromUser( const User& ) ) );

#ifdef BEEBEEP_USE_SHAREDESKTOP
  connect( mp_core, SIGNAL( shareDesktopImageAvailable( const User&, const QPixmap& ) ), this, SLOT( onShareDesktopImageAvailable( const User&, const QPixmap& ) ) );
#endif

  connect( mp_fileTransfer, SIGNAL( transferCancelled( VNumber ) ), mp_core, SLOT( cancelFileTransfer( VNumber ) ) );
  connect( mp_fileTransfer, SIGNAL( openFileCompleted( const QUrl& ) ), this, SLOT( openUrl( const QUrl& ) ) );

  connect( mp_userList, SIGNAL( chatSelected( VNumber ) ), this, SLOT( showChat( VNumber ) ) );
  connect( mp_userList, SIGNAL( userSelected( VNumber ) ), this, SLOT( checkUserSelected( VNumber ) ) );
  connect( mp_userList, SIGNAL( showVCardRequest( VNumber ) ), this, SLOT( showVCard( VNumber ) ) );

  connect( mp_groupList, SIGNAL( openChatForGroupRequest( VNumber ) ), this, SLOT( showChatForGroup( VNumber ) ) );
  connect( mp_groupList, SIGNAL( createGroupRequest() ), this, SLOT( createGroup() ) );
  connect( mp_groupList, SIGNAL( editGroupRequest( VNumber ) ), this, SLOT( editGroup( VNumber ) ) );
  connect( mp_groupList, SIGNAL( showVCardRequest( VNumber ) ), this, SLOT( showVCard( VNumber ) ) );
  connect( mp_groupList, SIGNAL( removeGroupRequest( VNumber ) ), this, SLOT( removeGroup( VNumber ) ) );

  connect( mp_chatList, SIGNAL( chatSelected( VNumber ) ), this, SLOT( showChat( VNumber ) ) );
  connect( mp_chatList, SIGNAL( chatToClear( VNumber ) ), this, SLOT( clearChat( VNumber ) ) );
  connect( mp_chatList, SIGNAL( chatToRemove( VNumber ) ), this, SLOT( removeChat( VNumber ) ) );
  connect( mp_chatList, SIGNAL( createNewChatRequest() ), this, SLOT( createChat() ) );

  connect( mp_trayIcon, SIGNAL( activated( QSystemTrayIcon::ActivationReason ) ), this, SLOT( trayIconClicked( QSystemTrayIcon::ActivationReason ) ) );
  connect( mp_trayIcon, SIGNAL( messageClicked() ), this, SLOT( trayMessageClicked() ) );

  connect( mp_savedChatList, SIGNAL( savedChatSelected( const QString& ) ), this, SLOT( showSavedChatSelected( const QString& ) ) );
  connect( mp_savedChatList, SIGNAL( savedChatRemoved( const QString& ) ), this, SLOT( removeSavedChat( const QString& ) ) );
  connect( mp_savedChatList, SIGNAL( savedChatLinkRequest( const QString& ) ), this, SLOT( linkSavedChat( const QString& ) ) );

  initShortcuts();
  initGuiItems();
  updateShortcuts();
}

void GuiMain::initShortcuts()
{
  mp_scMinimizeAllChats = new QShortcut( this );
  mp_scMinimizeAllChats->setContext( Qt::ApplicationShortcut );
  connect( mp_scMinimizeAllChats, SIGNAL( activated() ), this, SLOT( minimizeAllChats() ) );

#ifdef BEEBEEP_USE_QXT
  mp_scShowAllChats = new QxtGlobalShortcut( this );
  connect( mp_scShowAllChats, SIGNAL( activated() ), this, SLOT( showAllChats() ) );
#endif

  mp_scShowNextUnreadMessage = new QShortcut( this );
  mp_scShowNextUnreadMessage->setContext( Qt::ApplicationShortcut );
  connect( mp_scShowNextUnreadMessage, SIGNAL( activated() ), this, SLOT( showNextChat() ) );
}

void GuiMain::setupChatConnections( GuiChat* gui_chat )
{
  connect( gui_chat, SIGNAL( newMessage( VNumber, const QString& ) ), this, SLOT( sendMessage( VNumber, const QString& ) ) );
  connect( gui_chat, SIGNAL( writing( VNumber ) ), mp_core, SLOT( sendWritingMessage( VNumber ) ) );
  connect( gui_chat, SIGNAL( nextChat() ), this, SLOT( showNextChat() ) );
  connect( gui_chat, SIGNAL( openUrl( const QUrl& ) ), this, SLOT( openUrl( const QUrl& ) ) );
  connect( gui_chat, SIGNAL( sendFileFromChatRequest( VNumber, const QString& ) ), this, SLOT( sendFileFromChat( VNumber, const QString& ) ) );
  connect( gui_chat, SIGNAL( editGroupRequestFromChat( VNumber ) ), this, SLOT( editChat( VNumber ) ) );
  connect( gui_chat, SIGNAL( chatToClear( VNumber ) ), this, SLOT( clearChat( VNumber ) ) );
  connect( gui_chat, SIGNAL( leaveThisChat( VNumber ) ), this, SLOT( leaveGroupChat( VNumber ) ) );
  connect( gui_chat, SIGNAL( showChatMenuRequest() ), this, SLOT( showChatSettingsMenu() ) );
  connect( gui_chat, SIGNAL( createGroupFromChatRequest( VNumber ) ), this, SLOT( createGroupFromChat( VNumber ) ) );
}

void GuiMain::checkWindowFlagsAndShow()
{
  if( !mp_trayIcon->isVisible() )
  {
    mp_trayIcon->show();
#ifdef Q_OS_LINUX
    qApp->processEvents();
    mp_trayIcon->hide();
    qApp->processEvents();
    mp_trayIcon->show();
#endif
  }

  Bee::setWindowStaysOnTop( this, Settings::instance().stayOnTop() );

  if( !isVisible() )
    show();

  if( Settings::instance().resetGeometryAtStartup() || Settings::instance().guiGeometry().isEmpty() )
  {
    resize( width(), qMin( 520, qMax( QApplication::desktop()->availableGeometry().height() - 120, 460 ) ) );
    int diff_w = qMax( 0, QApplication::desktop()->screenGeometry().width() - QApplication::desktop()->availableGeometry().width() );
    int diff_h = qMax( 0, QApplication::desktop()->screenGeometry().height() - QApplication::desktop()->availableGeometry().height() );

#ifdef Q_OS_WIN
    diff_w += qMax( 20, diff_w );
    diff_h += qMax( 20, diff_h );
    move( QApplication::desktop()->availableGeometry().width() - width() - diff_w,
          QApplication::desktop()->availableGeometry().height() - height() - diff_h );
#elif defined Q_OS_MAC
    diff_h = 0; // skip only macosx top bar
    move( QApplication::desktop()->availableGeometry().width() - width() - diff_w, diff_h );
#else
    diff_w += qMax( 15, diff_w );
    diff_h += qMax( 15, diff_h );
    move( QApplication::desktop()->availableGeometry().width() - width() - diff_w,
          diff_h );
#endif
    mp_dockFileTransfers->setVisible( false );
  }
  else
  {
    if( !Settings::instance().guiState().isEmpty() )
      restoreState( Settings::instance().guiState() );
    restoreGeometry( Settings::instance().guiGeometry() );
  }

  checkViewActions();

  if( Settings::instance().loadOnTrayAtStartup() && QSystemTrayIcon::isSystemTrayAvailable() )
  {
    QMetaObject::invokeMethod( this, "hideToTrayIcon", Qt::QueuedConnection );
    return;
  }

  if( Settings::instance().showMinimizedAtStartup() )
    QMetaObject::invokeMethod( this, "showMinimized", Qt::QueuedConnection );
}

void GuiMain::showUp()
{
  bool on_top_flag_added = false;
  if( !(windowFlags() & Qt::WindowStaysOnTopHint) )
  {
    Bee::setWindowStaysOnTop( this, true );
    on_top_flag_added = true;
  }

  if( isMinimized() )
    showNormal();

  if( !isVisible() )
    show();

  raise();

  if( on_top_flag_added )
    Bee::setWindowStaysOnTop( this, false );
}

void GuiMain::updateWindowTitle()
{
  setWindowTitle( Settings::instance().localUser().name() );
}

void GuiMain::updateTabTitles()
{
  int tab_index = mp_tabMain->indexOf( mp_home );
  int current_value = m_unreadActivities;
  mp_tabMain->setTabText( tab_index, current_value > 0 ? QString::number( current_value ) : "" );
  if( current_value > 0 )
    mp_tabMain->setTabToolTip( tab_index, QString( "%1: %2 %3" ).arg( mp_home->toolTip() ).arg( current_value ).arg( tr( "news" ) ) );
  else
    mp_tabMain->setTabToolTip( tab_index, mp_home->toolTip() );

  tab_index = mp_tabMain->indexOf( mp_userList );
  current_value = mp_core->connectedUsers();
  int other_value = UserManager::instance().userList().size();
  mp_tabMain->setTabText( tab_index, current_value > 0 ? QString::number( current_value ) : (other_value > 0 ? QString::number( other_value ) : "" ) );
  if( current_value > 0 )
    mp_tabMain->setTabToolTip( tab_index, QString( "%1: %2 %3" ).arg( mp_userList->toolTip() ).arg( current_value ).arg( tr( "connected" ) ) );
  else
    mp_tabMain->setTabToolTip( tab_index, mp_userList->toolTip() );

  tab_index = mp_tabMain->indexOf( mp_chatList );
  current_value = ChatManager::instance().constChatList().size();
  mp_tabMain->setTabText( tab_index, current_value > 0 ? QString::number( current_value ) : "" );

  tab_index = mp_tabMain->indexOf( mp_groupList );
  current_value = UserManager::instance().groups().size();
  mp_tabMain->setTabText( tab_index, current_value > 0 ? QString::number( current_value ) : "" );

  tab_index = mp_tabMain->indexOf( mp_savedChatList );
  current_value = ChatManager::instance().constHistoryMap().size();
  mp_tabMain->setTabText( tab_index, current_value > 0 ? QString::number( current_value ) : "" );
}

void GuiMain::keyPressEvent( QKeyEvent* e )
{
  if( e->key() == Qt::Key_Escape )
  {
    if( Settings::instance().keyEscapeMinimizeInTray() )
      QTimer::singleShot( 0, this, SLOT( hideToTrayIcon() ) );
    else
      QTimer::singleShot( 0, this, SLOT( showMinimized() ) );
    e->accept();
    return;
  }

  QMainWindow::keyPressEvent( e );
}

void GuiMain::changeEvent( QEvent* e )
{
  QMainWindow::changeEvent( e );
}

void GuiMain::closeEvent( QCloseEvent* e )
{
  if( mp_core->isConnected() )
  {
    if( !m_forceShutdown )
    {
      if( Settings::instance().minimizeInTray() && QSystemTrayIcon::isSystemTrayAvailable() )
      {
        QTimer::singleShot( 0, this, SLOT( hideToTrayIcon() ) );
        e->ignore();
        return;
      }

      if( Settings::instance().promptOnCloseEvent() )
      {
        if( QMessageBox::question( this, Settings::instance().programName(), tr( "Do you really want to quit %1?" ).arg( Settings::instance().programName() ),
                               tr( "Yes" ), tr( "No" ), QString::null, 1, 1 ) == 1 )
        {
          e->ignore();
          return;
        }
      }
    }

    mp_core->stop();
  }

  QSettings* sets = Settings::instance().objectSettings();
  sets->deleteLater();

  if( !m_forceShutdown )
  {
    if( !sets->isWritable() )
    {
      if( QMessageBox::warning( this, Settings::instance().programName(),
                              QString( "%1<br />%2<br />%3<br />%4<br />%5" ).arg( tr( "<b>Settings can not be saved</b>. Path:" ) )
                                                                     .arg( sets->fileName() )
                                                                     .arg( tr( "<b>is not writable</b> by user:" ) )
                                                                     .arg( Settings::instance().localUser().accountName() )
                                                                     .arg( tr( "Do you want to close anyway?" ) ),
                              tr( "Yes" ), tr( "No" ), QString::null, 1, 1 ) == 1 )
      {
        e->ignore();
        return;
      }
    }

    if( Settings::instance().chatAutoSave() )
    {
      if( !SaveChatList::canBeSaved() )
      {
        if( QMessageBox::warning( this, Settings::instance().programName(),
                              QString( "%1<br />%2<br />%3<br />%4<br />%5" ).arg( tr( "<b>Chat messages can not be saved</b>. Path:" ) )
                                                                     .arg( Settings::instance().savedChatsFilePath() )
                                                                     .arg( tr( "<b>is not writable</b> by user:" ) )
                                                                     .arg( Settings::instance().localUser().accountName() )
                                                                     .arg( tr( "Do you want to close anyway?" ) ),
                              tr( "Yes" ), tr( "No" ), QString::null, 1, 1 ) == 1 )
        {
          e->ignore();
          return;
        }
      }
    }
  }

  if( mp_fileSharing )
    mp_fileSharing->close();

  if( mp_screenShot )
    mp_screenShot->close();

  if( mp_log )
    mp_log->close();

  foreach( GuiFloatingChat* fl_chat, m_floatingChats )
    fl_chat->close();

  mp_trayIcon->hide();

  if( mp_dockFileTransfers->isFloating() && mp_dockFileTransfers->isVisible() )
    mp_dockFileTransfers->hide();

  // quit now on last window closed
  qApp->setQuitOnLastWindowClosed( true );

  e->accept();
}

void GuiMain::showNextChat()
{
#ifdef BEEBEEP_DEBUG
  qDebug() << "Show next chat in list with unread messages";
#endif
  Chat c = ChatManager::instance().firstChatWithUnreadMessages();
  if( c.isValid() )
    showChat( c.id() );
  else
    showMessage( tr( "No new message available" ), 5000 );
}

void GuiMain::startStopCore()
{
  if( mp_core->isConnected() )
    stopCore();
  else
    startCore();
}

void GuiMain::forceShutdown()
{
  qDebug() << "Shutdown...";
  m_forceShutdown = true;
  if( mp_core->isConnected() )
    mp_core->stop();
  close();
}


void GuiMain::startCore()
{
  m_autoConnectOnInterfaceUp = false;

  if( mp_core->isConnected() )
    return;

  mp_tabMain->setCurrentWidget( mp_home );

  if( Settings::instance().firstTime() )
  {
    Settings::instance().setFirstTime( false );
    if( Settings::instance().askChangeUserAtStartup() )
    {
      if( !showWizard() )
        return;
      Settings::instance().setAskChangeUserAtStartup( false );
    }
  }
  else
  {
    if( Settings::instance().askChangeUserAtStartup() )
    {
      if( !showWizard() )
        return;
    }
  }

  if( Settings::instance().askPassword() )
  {
    if( !promptConnectionPassword() )
      return;
  }

  showMessage( tr( "Connecting" ), 2000 );
  m_coreIsConnecting = true;
  mp_core->start();
  initGuiItems();
}

bool GuiMain::promptConnectionPassword()
{
  GuiAskPassword gap( this );
  gap.setModal( true );
  gap.loadData();
  gap.show();
  gap.setFixedSize( gap.size() );
  if( gap.exec() == QDialog::Rejected )
    return false;
  mp_actPromptPassword->setChecked( Settings::instance().askPasswordAtStartup() );
  return true;
}

void GuiMain::stopCore()
{
  mp_core->stop();
  initGuiItems();
}

void GuiMain::initGuiItems()
{
  bool enable = mp_core->isConnected();

  if( enable )
  {
    mp_actStartStopCore->setIcon( QIcon( ":/images/disconnect.png") );
    mp_actStartStopCore->setText( tr( "Disconnect" ) );
  }
  else
  {
    mp_actStartStopCore->setIcon( QIcon( ":/images/connect.png") );
    mp_actStartStopCore->setText( tr( "Connect" ) );
  }

  mp_actBroadcast->setEnabled( enable );
  refreshUserList();

  updateStatusIcon();
  updateNewMessageAction();
  checkViewActions();
}

void GuiMain::checkViewActions()
{
  bool is_connected = mp_core->isConnected();
  int connected_users = mp_core->connectedUsers();

  mp_actCreateGroup->setEnabled( is_connected && UserManager::instance().userList().size() >= 2 );
  mp_actCreateGroupChat->setEnabled( is_connected && connected_users > 1 );
  mp_actViewFileSharing->setEnabled( Settings::instance().enableFileTransfer() && Settings::instance().enableFileSharing() );
  mp_actEnableFileSharing->setEnabled( Settings::instance().enableFileTransfer() && !Settings::instance().disableFileSharing() );
  mp_menuExistingFile->setEnabled( Settings::instance().enableFileTransfer() );
  mp_actConfirmDownload->setEnabled( Settings::instance().enableFileTransfer() );
  mp_actSelectDownloadFolder->setEnabled( Settings::instance().enableFileTransfer() );

  showDefaultServerPortInMenu();

  if( !m_floatingChats.isEmpty() )
  {
    foreach( GuiFloatingChat* fl_chat, m_floatingChats )
      fl_chat->updateActions( is_connected, connected_users );
  }

  if( mp_fileSharing )
  {
    if( mp_actViewFileSharing->isEnabled() )
      mp_fileSharing->checkViewActions();
    else
      mp_fileSharing->close();
  }

  updateWindowTitle();
  updateTabTitles();
}

void GuiMain::showAbout()
{
  QMessageBox::about( this, Settings::instance().programName(),
                      QString( "<b>%1</b> - %2<br /><br />%3 %4 %5 %6<br />%7 %8<br />%9<br />" )
                      .arg( Settings::instance().programName() )
                      .arg( tr( "Secure Lan Messenger" ) )
                      .arg( tr( "Version" ) )
                      .arg( Settings::instance().version( true, true ) )
                      .arg( tr( "for" ) )
                      .arg( Settings::instance().operatingSystem( true ) )
                      .arg( tr( "developed by" ) )
                      .arg( QString( "<a href='http://it.linkedin.com/pub/marco-mastroddi/20/5a7/191'>Marco Mastroddi</a>" ) )
                      .arg( QString( "e-mail: <a href='mailto://marco.mastroddi@gmail.com'>marco.mastroddi@gmail.com</a><br />web: <a href='http://www.beebeep.net'>www.beebeep.net</a>" ) )
                      );

}

void GuiMain::showLicense()
{
  QString license_txt = tr( "BeeBEEP is free software: you can redistribute it and/or modify<br />"
  "it under the terms of the GNU General Public License as published<br />"
  "by the Free Software Foundation, either version 3 of the License<br />"
  "or (at your option) any later version.<br /><br />"
  "BeeBEEP is distributed in the hope that it will be useful,<br />"
  "but WITHOUT ANY WARRANTY; without even the implied warranty<br />"
  "of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.<br />"
  "See the GNU General Public License for more details." );
  QMessageBox::about( this, Settings::instance().programName(), license_txt );
}

void GuiMain::createActions()
{
  mp_actStartStopCore = new QAction( this );
  connect( mp_actStartStopCore, SIGNAL( triggered() ), this, SLOT( startStopCore() ) );

  mp_actBroadcast = new QAction( QIcon( ":/images/broadcast.png" ), tr( "Broadcast to network" ), this );
  connect( mp_actBroadcast, SIGNAL( triggered() ), this, SLOT( sendBroadcastMessage() ) );

  mp_actConfigureNetwork = new QAction( QIcon( ":/images/network.png"), tr( "Network..."), this );
  connect( mp_actConfigureNetwork, SIGNAL( triggered() ), this, SLOT( searchUsers() ) );

  mp_actQuit = new QAction( QIcon( ":/images/quit.png" ), tr( "Quit" ), this );
  mp_actQuit->setShortcuts( QKeySequence::Quit );
  mp_actQuit->setMenuRole( QAction::QuitRole );
  connect( mp_actQuit, SIGNAL( triggered() ), this, SLOT( forceShutdown() ) );

  mp_actVCard = new QAction( QIcon( ":/images/profile-edit.png"), tr( "Edit your profile..." ), this );
  connect( mp_actVCard, SIGNAL( triggered() ), this, SLOT( changeVCard() ) );

  mp_actAbout = new QAction( QIcon( ":/images/beebeep.png" ), tr( "About %1..." ).arg( Settings::instance().programName() ), this );
  mp_actAbout->setMenuRole( QAction::AboutRole );
  connect( mp_actAbout, SIGNAL( triggered() ), this, SLOT( showAbout() ) );

  mp_actCreateGroupChat = new QAction( QIcon( ":/images/chat-create.png" ), tr( "Create chat" ), this );
  connect( mp_actCreateGroupChat, SIGNAL( triggered() ), this, SLOT( createChat() ) );

  mp_actCreateGroup = new QAction(  QIcon( ":/images/group-add.png" ), tr( "Create group" ), this );
  connect( mp_actCreateGroup, SIGNAL( triggered() ), this, SLOT( createGroup() ) );

  mp_actViewNewMessage = new QAction( QIcon( ":/images/beebeep-message.png" ), tr( "Show new message" ), this );
  connect( mp_actViewNewMessage, SIGNAL( triggered() ), this, SLOT( showNextChat() ) );

  mp_actViewFileSharing = new QAction( QIcon( ":/images/file-sharing.png" ), tr( "Show file sharing window" ), this );
  connect( mp_actViewFileSharing, SIGNAL( triggered() ), this, SLOT( showFileSharingWindow() ) );

  mp_actViewLog = new QAction( QIcon( ":/images/log.png" ), tr( "Show the %1 log" ).arg( Settings::instance().programName() ), this );
  connect( mp_actViewLog, SIGNAL( triggered() ), this, SLOT( showLogWindow() ) );

  mp_actViewScreenShot = new QAction( QIcon( ":/images/screenshot.png" ), tr( "Make a screenshot" ), this );
  connect( mp_actViewScreenShot, SIGNAL( triggered() ), this, SLOT( showScreenShotWindow() ) );
}

void GuiMain::createMenus()
{
  /* Plugins Menu */
  mp_menuPlugins = new QMenu( tr( "Plugins" ) + QString( "..." ), this );
  mp_menuPlugins->setIcon( QIcon( ":/images/plugin.png" ) );

  /* Main Menu */
  mp_menuMain = new QMenu( tr( "Main" ), this );
  mp_menuMain->addAction( mp_actStartStopCore );
  mp_menuMain->addSeparator();
  mp_menuMain->addAction( mp_actVCard );
  mp_actAddUsers = mp_menuMain->addAction( QIcon( ":/images/user-add.png" ), tr( "Add users" ) + QString( "..." ), this, SLOT( showAddUser() ) );
  mp_menuMain->addSeparator();
  mp_menuMain->addMenu( mp_menuPlugins );
  mp_menuMain->addSeparator();
  if( Settings::instance().resourceFolder() != Settings::instance().dataFolder() )
    mp_menuMain->addAction( QIcon( ":/images/resource-folder.png" ), tr( "Open your resource folder" ), this, SLOT( openResourceFolder() ) );
  mp_menuMain->addAction( QIcon( ":/images/data-folder.png" ), tr( "Open your data folder" ), this, SLOT( openDataFolder() ) );
  mp_menuMain->addAction( mp_actViewLog );
  mp_menuMain->addSeparator();
  mp_menuMain->addAction( mp_actQuit );

  QAction* act;
  /* Chat Menu */
  mp_menuChat = new QMenu( tr( "Chat" ), this );

  /* System Menu */
  mp_menuSettings = new QMenu( tr( "Settings" ), this );

  mp_menuStartupSettings = new QMenu( tr( "On start" ), this );
  mp_menuStartupSettings->setIcon( QIcon( ":/images/settings-start.png" ) );
  mp_menuSettings->addMenu( mp_menuStartupSettings );
  act = mp_menuStartupSettings->addAction( tr( "Prompts for change user" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().askChangeUserAtStartup() );
  act->setData( 45 );
  mp_actPromptPassword = mp_menuStartupSettings->addAction( tr( "Prompts for network password" ), this, SLOT( settingsChanged() ) );
  mp_actPromptPassword->setCheckable( true );
  mp_actPromptPassword->setChecked( Settings::instance().askPasswordAtStartup() );
  mp_actPromptPassword->setData( 17 );
  act = mp_menuStartupSettings->addAction( tr( "Show minimized" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().showMinimizedAtStartup() );
  act->setData( 35 );
  act = mp_menuStartupSettings->addAction( tr( "Show only on system tray" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().loadOnTrayAtStartup() );
  act->setData( 24 );
  act = mp_menuStartupSettings->addAction( tr( "Reset window geometry" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().resetGeometryAtStartup() );
  act->setData( 26 );
  act = mp_menuStartupSettings->addAction( tr( "Check for new version" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().checkNewVersionAtStartup() );
  act->setData( 43 );

  mp_menuCloseSettings = new QMenu( tr( "On close" ), this );
  mp_menuCloseSettings->setIcon( QIcon( ":/images/settings-close.png" ) );
  mp_menuSettings->addMenu( mp_menuCloseSettings );
  act = mp_menuCloseSettings->addAction( tr( "Prompt on quit when connected" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().promptOnCloseEvent() );
  act->setData( 36 );
  act = mp_menuCloseSettings->addAction( tr( "Close button minimize to tray icon" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().minimizeInTray() );
  act->setData( 11 );
  act = mp_menuCloseSettings->addAction( tr( "Escape key minimize to tray icon" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().keyEscapeMinimizeInTray() );
  act->setData( 29 );

  mp_menuUsersSettings = new QMenu( tr( "Users" ), this );
  mp_menuUsersSettings->setIcon( QIcon( ":/images/user-list.png" ) );
  mp_menuSettings->addMenu( mp_menuUsersSettings );
  if( Settings::instance().trustSystemAccount() )
  {
    act = mp_menuUsersSettings->addAction( tr( "Recognize users by system or domain account" ) );
    act->setCheckable( true );
    act->setChecked( true );
    act->setDisabled( true );
  }
  else
  {
    act = mp_menuUsersSettings->addAction( tr( "Recognize users only by nickname" ), this, SLOT( settingsChanged() ) );
    act->setCheckable( true );
    act->setChecked( !Settings::instance().trustUserHash() );
    act->setData( 2 );
  }
  mp_menuUsersSettings->addSeparator();
  act = mp_menuUsersSettings->addAction( tr( "Save users" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().saveUserList() );
  act->setData( 32 );
  mp_menuUsersSettings->addSeparator();
  mp_menuUsersSettings->addAction( QIcon( ":/images/workgroup.png" ), tr( "Workgroups" ) + QString( "..." ), this, SLOT( showWorkgroups() ) );

  mp_menuChatSettings = new QMenu( tr( "Chat" ), this );
  mp_menuChatSettings->setIcon( QIcon( ":/images/chat.png" ) );
  mp_menuSettings->addMenu( mp_menuChatSettings );
  act = mp_menuChatSettings->addAction( tr( "Save messages" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().chatAutoSave() );
  act->setData( 18 );
  mp_menuChatSettings->addSeparator();
  act = mp_menuChatSettings->addAction( tr( "Open chats in a single window" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().showChatsInOneWindow() );
  act->setData( 7 );
  act = mp_menuChatSettings->addAction( tr( "Raise chat window on new message" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().raiseOnNewMessageArrived() );
  act->setData( 15 );
  act = mp_menuChatSettings->addAction( tr( "Clear all read messages on closing window" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().chatClearAllReadMessages() );
  act->setData( 47 );
  act = mp_menuChatSettings->addAction( tr( "Set your status to away automatically" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().autoUserAway() );
  act->setData( 20 );

  mp_menuFileTransferSettings = new QMenu( tr( "File transfer" ), this );
  mp_menuFileTransferSettings->setIcon( QIcon( ":/images/file-transfer.png" ) );
  mp_menuFileTransferSettings->setDisabled( Settings::instance().disableFileTransfer() );
  mp_menuSettings->addMenu( mp_menuFileTransferSettings );
  act = mp_menuFileTransferSettings->addAction( tr( "Enable file transfer" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().enableFileTransfer() );
  act->setData( 12 );
  mp_actEnableFileSharing = mp_menuFileTransferSettings->addAction( tr( "Enable file sharing" ), this, SLOT( settingsChanged() ) );
  mp_actEnableFileSharing->setCheckable( true );
  mp_actEnableFileSharing->setChecked( Settings::instance().enableFileSharing() );
  mp_actEnableFileSharing->setData( 5 );
  mp_actEnableFileSharing->setDisabled( Settings::instance().disableFileSharing() );

  mp_menuFileTransferSettings->addSeparator();
  mp_menuExistingFile = mp_menuFileTransferSettings->addMenu( tr( "If a file already exists" ) + QString( "..." ) );
  mp_actGroupExistingFile = new QActionGroup( this );
  mp_actGroupExistingFile->setExclusive( true );
  mp_actOverwriteExistingFile = mp_menuExistingFile->addAction( tr( "Overwrite" ), this, SLOT( settingsChanged() ) );
  mp_actOverwriteExistingFile->setCheckable( true );
  mp_actOverwriteExistingFile->setChecked( Settings::instance().overwriteExistingFiles() );
  mp_actOverwriteExistingFile->setData( 99 );
  mp_actGenerateAutomaticFilename = mp_menuExistingFile->addAction( tr( "Generate automatic filename" ), this, SLOT( settingsChanged() ) );
  mp_actGenerateAutomaticFilename->setCheckable( true );
  mp_actGenerateAutomaticFilename->setChecked( Settings::instance().automaticFileName() );
  mp_actGenerateAutomaticFilename->setData( 99 );
  mp_actAskToDoOnExistingFile = mp_menuExistingFile->addAction( tr( "Ask me" ), this, SLOT( settingsChanged() ) );
  mp_actAskToDoOnExistingFile->setCheckable( true );
  mp_actAskToDoOnExistingFile->setChecked( !Settings::instance().automaticFileName() && !Settings::instance().overwriteExistingFiles() );
  mp_actAskToDoOnExistingFile->setData( 99 );
  mp_actGroupExistingFile->addAction( mp_actOverwriteExistingFile );
  mp_actGroupExistingFile->addAction( mp_actGenerateAutomaticFilename );
  mp_actGroupExistingFile->addAction( mp_actAskToDoOnExistingFile );
  connect( mp_actGroupExistingFile, SIGNAL( triggered( QAction* ) ), this, SLOT( onChangeSettingOnExistingFile( QAction* ) ) );
  mp_actConfirmDownload = mp_menuFileTransferSettings->addAction( tr( "Prompt before downloading file" ), this, SLOT( settingsChanged() ) );
  mp_actConfirmDownload->setCheckable( true );
  mp_actConfirmDownload->setChecked( Settings::instance().confirmOnDownloadFile() );
  mp_actConfirmDownload->setData( 30 );
  mp_menuFileTransferSettings->addSeparator();
  mp_actSelectDownloadFolder = mp_menuFileTransferSettings->addAction( QIcon( ":/images/download-folder.png" ), tr( "Download folder..."), this, SLOT( selectDownloadDirectory() ) );

  mp_menuSoundSettings = new QMenu( tr( "Sound" ), this );
  mp_menuSoundSettings->setIcon( QIcon( ":/images/bell.png" ) );
  mp_menuSettings->addMenu( mp_menuSoundSettings );
  mp_actBeepOnNewMessage = mp_menuSoundSettings->addAction( tr( "Enable BEEP alert" ), this, SLOT( settingsChanged() ) );
  mp_actBeepOnNewMessage->setCheckable( true );
  mp_actBeepOnNewMessage->setChecked( Settings::instance().beepOnNewMessageArrived() );
  mp_actBeepOnNewMessage->setData( 34 );
  act = mp_menuSoundSettings->addAction( tr( "Enable Buzz sound" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().playBuzzSound() );
  act->setData( 56 );
  mp_menuSoundSettings->addSeparator();
  mp_menuSoundSettings->addAction( QIcon( ":/images/file-beep.png" ), tr( "Select beep file..." ), this, SLOT( selectBeepFile() ) );
  mp_menuSoundSettings->addAction( QIcon( ":/images/play.png" ), tr( "Play beep" ), this, SLOT( testBeepFile() ) );

  mp_menuTrayIconSettings = new QMenu( tr( "System tray icon" ), this );
  mp_menuTrayIconSettings->setIcon( QIcon( ":/images/settings-tray-icon.png" ) );
  mp_menuSettings->addMenu( mp_menuTrayIconSettings );
  act = mp_menuTrayIconSettings->addAction( tr( "Enable tray icon notification" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().showNotificationOnTray()  );
  act->setData( 19 );
  act = mp_menuTrayIconSettings->addAction( tr( "Show only message notifications" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().showOnlyMessageNotificationOnTray()  );
  act->setData( 40 );
  act = mp_menuTrayIconSettings->addAction( tr( "Show chat message preview" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().showChatMessageOnTray() );
  act->setData( 46 );
  act = mp_menuTrayIconSettings->addAction( tr( "Show file notification" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().showFileTransferCompletedOnTray() );
  act->setData( 48 );

  mp_menuSettings->addSeparator();
  mp_menuSettings->addAction( mp_actConfigureNetwork );
  mp_menuSettings->addAction( QIcon( ":/images/shortcut.png" ), tr( "Shortcuts" ) + QString( "..." ), this, SLOT( editShortcuts() ) );
  mp_menuSettings->addAction( QIcon( ":/images/language.png" ), tr( "Select language") + QString( "..." ), this, SLOT( selectLanguage() ) );
  mp_menuSettings->addAction( QIcon( ":/images/dictionary.png" ), tr( "Dictionary" ) + QString( "..." ), this, SLOT( selectDictionatyPath() ) );
  mp_menuSettings->addSeparator();

  act = mp_menuSettings->addAction( tr( "Always stay on top" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().stayOnTop() );
  act->setData( 14 );
  act = mp_menuSettings->addAction( tr( "Use native file dialogs" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().useNativeDialogs() );
  act->setData( 4 );
#ifdef Q_OS_WIN
  act = mp_menuSettings->addAction( tr( "Start %1 on Windows startup" ).arg( Settings::instance().programName() ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().hasStartOnSystemBoot() );
  act->setData( 16 );
#endif

  mp_menuSettings->addSeparator();
  mp_menuSettings->addAction( QIcon( ":/images/save-window.png" ), tr( "Save window's geometry" ), this, SLOT( saveGeometryAndState() ) );

  /* User List Menu */
  mp_menuUserList = new QMenu( tr( "Options" ), this );

  act = mp_menuUserList->addAction( tr( "Sort users in ascending order" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().sortUsersAscending() );
  act->setData( 49 );

  QMenu* sorting_users_menu = mp_menuUserList->addMenu( tr( "Sorting mode" ) + QString( "..." ) );
  QActionGroup* sorting_users_action_group = new QActionGroup( this );
  sorting_users_action_group->setExclusive( true );

  act = sorting_users_menu->addAction( tr( "Default mode" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().userSortingMode() < 1 || Settings::instance().userSortingMode() > 3 );
  act->setData( 50 );
  sorting_users_action_group->addAction( act );

  act = sorting_users_menu->addAction( tr( "By user name" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().userSortingMode() == 1 );
  act->setData( 51 );
  sorting_users_action_group->addAction( act );

  act = sorting_users_menu->addAction( tr( "By user status" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().userSortingMode() == 2 );
  act->setData( 52 );
  sorting_users_action_group->addAction( act );

  act = sorting_users_menu->addAction( tr( "By unread messages" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().userSortingMode() == 3 );
  act->setData( 53 );
  sorting_users_action_group->addAction( act );

  mp_menuUserList->addSeparator();

  act = mp_menuUserList->addAction( tr( "Show only the online users" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().showOnlyOnlineUsers() );
  act->setData( 6 );

  act = mp_menuUserList->addAction( tr( "Show the user's picture" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().showUserPhoto() );
  act->setData( 21 );

  act = mp_menuUserList->addAction( tr( "Show the user's vCard on right click" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().showVCardOnRightClick() );
  act->setData( 25 );

  act = mp_menuUserList->addAction( tr( "Show status color in background" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().showUserStatusBackgroundColor() );
  act->setData( 38 );

  act = mp_menuUserList->addAction( tr( "Show the status description" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().showUserStatusDescription() );
  act->setData( 37 );

  mp_menuUserList->addSeparator();

  mp_menuUserList->addAction( tr( "Change size of the user's picture" ), this, SLOT( changeAvatarSizeInList() ) );

  mp_userList->setMenuSettings( mp_menuUserList );

  /* Context Menu for user list view */
  QMenu* context_menu_users = new QMenu( "Menu", this );
  context_menu_users->addAction( mp_actVCard );
  context_menu_users->addSeparator();
  context_menu_users->addAction( mp_actConfigureNetwork );
  context_menu_users->addAction( mp_actAddUsers );
  mp_userList->setContextMenuUsers( context_menu_users );

  /* Status Menu */
  mp_menuStatus = new QMenu( tr( "Status" ), this );
  mp_menuStatus->setIcon( QIcon( ":/images/user-status.png" ) );
  for( int i = User::Online; i < User::NumStatus; i++ )
  {
    act = mp_menuStatus->addAction( QIcon( Bee::menuUserStatusIconFileName( i ) ), Bee::userStatusToString( i ), this, SLOT( statusSelected() ) );
    act->setData( i );
    act->setIconVisibleInMenu( true );
  }
  mp_menuStatus->addSeparator();
  mp_menuStatus->addAction( mp_actVCard );
  mp_menuStatus->addSeparator();
  mp_menuUserStatusList = new QMenu( tr( "Recently used" ), this );
  act = mp_menuStatus->addMenu( mp_menuUserStatusList );
  act->setIcon( QIcon( ":/images/recent.png" ) );
  loadUserStatusRecentlyUsed();
  act = mp_menuStatus->addAction( QIcon( ":/images/user-status.png" ), tr( "Change your status description..." ), this, SLOT( changeStatusDescription() ) );
  act = mp_menuStatus->addAction( QIcon( ":/images/clear.png" ), tr( "Clear all status descriptions" ), this, SLOT( clearRecentlyUsedUserStatus() ) );
  mp_menuStatus->addSeparator();
  act = mp_menuStatus->addAction( QIcon( Bee::menuUserStatusIconFileName( User::Offline ) ), Bee::userStatusToString( User::Offline ), this, SLOT( statusSelected() ) );
  act->setData( User::Offline );
  act->setIconVisibleInMenu( true );
  act = mp_menuStatus->menuAction();
  connect( act, SIGNAL( triggered() ), this, SLOT( showLocalUserVCard() ) );

  /* Help Menu */
  mp_menuInfo = new QMenu( tr("?" ), this );
  mp_menuInfo->addAction( QIcon( ":/images/tip.png" ), tr( "Tip of the day" ), this, SLOT( showTipOfTheDay() ) );
  mp_menuInfo->addAction( QIcon( ":/images/fact.png" ), tr( "Fact of the day" ), this, SLOT( showFactOfTheDay() ) );
  mp_menuInfo->addSeparator();
  mp_menuInfo->addAction( mp_actAbout );
  mp_menuInfo->addAction( QIcon( ":/images/license.png" ), tr( "Show %1's license..." ).arg( Settings::instance().programName() ), this, SLOT( showLicense() ) );
  act = mp_menuInfo->addAction( QIcon( ":/images/qt.png" ), tr( "Qt Library..." ), qApp, SLOT( aboutQt() ) );
  act->setMenuRole( QAction::AboutQtRole );
  mp_menuInfo->addSeparator();
  mp_menuInfo->addAction( QIcon( ":/images/beebeep.png" ), tr( "Open %1 official website..." ).arg( Settings::instance().programName() ), this, SLOT( openWebSite() ) );
  mp_menuInfo->addAction( QIcon( ":/images/update.png" ), tr( "Check for new version..." ), this, SLOT( checkNewVersion() ) );
  mp_menuInfo->addAction( QIcon( ":/images/plugin.png" ), tr( "Download plugins..." ), this, SLOT( openDownloadPluginPage() ) );
  mp_menuInfo->addAction( QIcon( ":/images/info.png" ), tr( "Help online..." ), this, SLOT( openHelpPage() ) );
  mp_menuInfo->addSeparator();
  mp_menuInfo->addAction( QIcon( ":/images/thumbup.png" ), tr( "Like %1 on Facebook" ).arg( Settings::instance().programName() ), this, SLOT( openFacebookPage() ) );
#ifdef BEEBEEP_DEBUG
  act = mp_menuInfo->addAction( tr( "Add +1 user to anonymous usage statistics" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().postUsageStatistics() );
  act->setData( 44 );
#endif
  mp_menuInfo->addAction( QIcon( ":/images/donate.png" ), tr( "Donate for %1" ).arg( Settings::instance().programName() ), this, SLOT( openDonationPage() ) );

  /* Tray icon menu */
  mp_menuTrayIcon = new QMenu( this );
  act = mp_menuTrayIcon->addAction( QIcon( ":/images/beebeep.png" ), tr( "Show" ), this, SLOT( showUp() ) );
  mp_menuTrayIcon->setDefaultAction( act );

  mp_menuTrayIcon->addSeparator();
  mp_menuTrayIcon->addAction( mp_menuStatus->menuAction() );
  mp_menuTrayIcon->addSeparator();

  mp_menuNetworkStatus = mp_menuTrayIcon->addMenu( QIcon( ":/images/connect.png" ), tr( "Network" ) );
  mp_actHostAddress = mp_menuNetworkStatus->addAction( QString( "ip" ) );
  mp_actPortBroadcast = mp_menuNetworkStatus->addAction( QString( "udp1" ) );
  mp_actPortListener = mp_menuNetworkStatus->addAction( QString( "tcp1" ) );
  mp_actPortFileTransfer = mp_menuNetworkStatus->addAction( QString( "tcp2" ) );
#ifdef BEEBEEP_USE_MULTICAST_DNS
  mp_actMulticastDns = mp_menuNetworkStatus->addAction( QString( "mdns" ) );
#endif
  mp_menuTrayIcon->addSeparator();
  mp_menuTrayIcon->addAction( QIcon( ":/images/quit.png" ), tr( "Quit" ), this, SLOT( forceShutdown() ) );

  mp_trayIcon->setContextMenu( mp_menuTrayIcon );
}

void GuiMain::createToolAndMenuBars()
{
  menuBar()->addMenu( mp_menuMain );
  menuBar()->addMenu( mp_menuSettings );
  menuBar()->addMenu( mp_menuInfo );
  QLabel *label_version = new QLabel( this );
  label_version->setTextFormat( Qt::RichText );
  label_version->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
  QString label_version_text = QString( "&nbsp;&nbsp;<b>%1</b> %2&nbsp;" )
                                .arg( Settings::instance().version( false, false ) )
                                .arg( Bee::iconToHtml( Settings::instance().operatingSystemIconPath(), "*", 12, 12 ) );
  label_version->setText( label_version_text );
  label_version->setToolTip( QString( "BeeBEEP %1 %2" ).arg( Settings::instance().version( true, true ), Settings::instance().operatingSystem( true ) ) );
  menuBar()->setCornerWidget( label_version );

  mp_barMain->addAction( mp_menuStatus->menuAction() );
  mp_barMain->addAction( mp_actBroadcast );
  mp_barMain->addAction( mp_actViewNewMessage );
  mp_barMain->addAction( mp_actCreateGroupChat );
  mp_barMain->addAction( mp_actCreateGroup );
  mp_barMain->addAction( mp_actViewFileTransfer );
  mp_barMain->addAction( mp_actViewFileSharing );
}

void GuiMain::createMainWidgets()
{
  int tab_index;

  mp_home = new GuiHome( this );
  mp_home->setToolTip( tr( "Activities" ) );
  connect( mp_home, SIGNAL( openUrlRequest( const QUrl& ) ), this, SLOT( openUrl( const QUrl& ) ) );
  tab_index = mp_tabMain->addTab( mp_home, QIcon( ":/images/activities.png" ), "" );
  mp_tabMain->setTabToolTip( tab_index, mp_home->toolTip() );
  mp_userList = new GuiUserList( this );
  mp_userList->setToolTip( tr( "Users" ) );
  tab_index = mp_tabMain->addTab( mp_userList, QIcon( ":/images/user-list.png" ), "" );
  mp_tabMain->setTabToolTip( tab_index, mp_userList->toolTip() );
  mp_chatList = new GuiChatList( this );
  mp_chatList->setToolTip( tr( "Chats" ) );
  tab_index = mp_tabMain->addTab( mp_chatList, QIcon( ":/images/chat-list.png" ), "" );
  mp_tabMain->setTabToolTip( tab_index, mp_chatList->toolTip() );
  mp_groupList = new GuiGroupList( this );
  mp_groupList->setToolTip( tr( "Groups" ) );
  tab_index = mp_tabMain->addTab( mp_groupList, QIcon( ":/images/group.png" ), "" );
  mp_tabMain->setTabToolTip( tab_index, mp_groupList->toolTip() );
  mp_savedChatList = new GuiSavedChatList( this );
  mp_savedChatList->setToolTip( tr( "Chat histories" ) );
  tab_index = mp_tabMain->addTab( mp_savedChatList, QIcon( ":/images/saved-chat-list.png" ), "" );
  mp_tabMain->setTabToolTip( tab_index, mp_savedChatList->toolTip() );

  mp_dockFileTransfers = new QDockWidget( tr( "File Transfers" ), this );
  mp_dockFileTransfers->setObjectName( "GuiFileTransferDock" );
  mp_fileTransfer = new GuiTransferFile( this );
  mp_dockFileTransfers->setWidget( mp_fileTransfer );
  mp_dockFileTransfers->setAllowedAreas( Qt::AllDockWidgetAreas );
  addDockWidget( Qt::BottomDockWidgetArea, mp_dockFileTransfers );
  mp_actViewFileTransfer = mp_dockFileTransfers->toggleViewAction();
  mp_actViewFileTransfer->setIcon( QIcon( ":/images/file-transfer.png" ) );
  mp_actViewFileTransfer->setText( tr( "Show the file transfer panel" ) );
  mp_actViewFileTransfer->setData( 99 );
}

void GuiMain::startExternalApplicationFromActionData()
{
  QAction* act = qobject_cast<QAction*>( sender() );
  if( !act )
    return;

  QString application_path = act->data().toString();
  qDebug() << "Starting external application:" << qPrintable( application_path );
  if( !QDesktopServices::openUrl( QUrl::fromLocalFile( application_path ) ) )
    QMessageBox::information( this, Settings::instance().programName(), tr( "Unable to open %1" ).arg( application_path ), tr( "Ok" ) );
}

void GuiMain::onUserChanged( const User& u )
{
  mp_userList->setUser( u, true );
  mp_groupList->updateUser( u );
  mp_chatList->updateUser( u );
  if( mp_fileSharing )
    mp_fileSharing->onUserChanged( u );
 foreach( GuiFloatingChat* fl_chat, m_floatingChats )
    fl_chat->updateUser( u );
  checkViewActions();
  if( u.isLocal() )
  {
    updateWindowTitle();
  }
  else
  {
    if( m_coreIsConnecting && u.isStatusConnected() && mp_tabMain->currentWidget() == mp_home )
    {
      m_coreIsConnecting = false;
      mp_tabMain->setCurrentWidget( mp_userList );
    }
  }
}

void GuiMain::refreshUserList()
{
  mp_userList->updateUsers( mp_core->isConnected() );
}

void GuiMain::settingsChanged()
{
  QAction* act = qobject_cast<QAction*>( sender() );
  if( !act )
    return;

  bool refresh_users = false;
  bool refresh_chat = false;
  int settings_data_id = act->data().toInt();
  bool ok = false;

#ifdef BEEBEEP_DEBUG
  qDebug() << "Settings changed for action id" << settings_data_id << "to" << (bool)(act->isChecked());
#endif

  switch( settings_data_id )
  {
  case 1:
    Settings::instance().setChatCompact( act->isChecked() );
    refresh_chat = true;
    break;
  case 2:
    Settings::instance().setTrustUserHash( !act->isChecked() );
    break;
  case 3:
    Settings::instance().setChatShowMessageTimestamp( act->isChecked() );
    refresh_chat = true;
    break;
  case 4:
    Settings::instance().setUseNativeDialogs( act->isChecked() );
    break;
  case 5:
    setFileSharingEnabled( act->isChecked() );
    break;
  case 6:
    Settings::instance().setShowOnlyOnlineUsers( act->isChecked() );
    refresh_users = true;
    refresh_chat = true;
  case 7:
    {
      Settings::instance().setShowChatsInOneWindow( act->isChecked() );
      if( Settings::instance().showChatsInOneWindow() )
      {
        foreach( GuiFloatingChat* fl_chat, m_floatingChats )
          fl_chat->close();
      }
    }
    break;
  case 8:
    Settings::instance().setChatUseHtmlTags( act->isChecked() );
    refresh_chat = true;
    break;
  case 9:
    Settings::instance().setChatUseClickableLinks( act->isChecked() );
    refresh_chat = true;
    break;
  case 10:
    Settings::instance().setShowEmoticons( act->isChecked() );
    refresh_chat = true;
    break;
  case 11:
    Settings::instance().setMinimizeInTray( act->isChecked() );
    break;
  case 12:
    setFileTransferEnabled( act->isChecked() );
    break;
  case 13:
    Settings::instance().setShowMessagesGroupByUser( act->isChecked() );
    refresh_chat = true;
    break;
  case 14:
    {
      Settings::instance().setStayOnTop( act->isChecked() );
      Bee::setWindowStaysOnTop( this, act->isChecked() );
      foreach( GuiFloatingChat* fl_chat, m_floatingChats )
        Bee::setWindowStaysOnTop( fl_chat, act->isChecked() );
      if( mp_fileSharing )
        Bee::setWindowStaysOnTop( mp_fileSharing, act->isChecked() );
      if( mp_log )
        Bee::setWindowStaysOnTop( mp_log, act->isChecked() );
      if( mp_screenShot )
        Bee::setWindowStaysOnTop( mp_screenShot, act->isChecked() );
    }
    break;
  case 15:
    Settings::instance().setRaiseOnNewMessageArrived( act->isChecked() );
    break;
  case 16:
    checkAutoStartOnBoot( act->isChecked() );
    break;
  case 17:
    {
      if( !act->isChecked() )
      {
        Settings::instance().setAskPasswordAtStartup( false );
        if( Settings::instance().askPassword() )
        {
          QMessageBox::information( this, Settings::instance().programName(), tr( "Please save the network password in the next dialog." ) );
          promptConnectionPassword();
          return;
        }
      }
      else
        Settings::instance().setAskPasswordAtStartup( true );
    }
    break;
  case 18:
    Settings::instance().setChatAutoSave( act->isChecked() );
    break;
  case 19:
    Settings::instance().setShowNotificationOnTray( act->isChecked() );
    break;
  case 20:
    {
      Settings::instance().setAutoUserAway( act->isChecked() );
      if( act->isChecked() )
      {
        BeeApplication* bee_app = (BeeApplication*)qApp;
        int away_timeout = QInputDialog::getInt( this, Settings::instance().programName(),
                              tr( "How many minutes of idle %1 can wait before changing status to away?" ).arg( Settings::instance().programName() ),
                              Settings::instance().userAwayTimeout(), 1, 30, 1, &ok );
        if( ok && away_timeout > 0 )
          Settings::instance().setUserAwayTimeout( away_timeout );

        bee_app->setIdleTimeout( Settings::instance().userAwayTimeout() );
      }
    }
    break;
  case 21:
    Settings::instance().setShowUserPhoto( act->isChecked() );
    refresh_users = true;
    break;
  case 23:
    {
      Settings::instance().setChatFont( QApplication::font() );
      foreach( GuiFloatingChat* fl_chat, m_floatingChats )
        fl_chat->guiChat()->setChatFont( Settings::instance().chatFont() );
    }
    break;
  case 24:
    Settings::instance().setLoadOnTrayAtStartup( act->isChecked() );
    break;
  case 25:
    Settings::instance().setShowVCardOnRightClick( act->isChecked() );
    break;
  case 26:
    Settings::instance().setResetGeometryAtStartup( act->isChecked() );
    break;
  case 27:
    {
      Settings::instance().setChatMaxMessagesToShow( act->isChecked() );
      if( act->isChecked() )
      {
#if QT_VERSION >= 0x050000
        int num_messages = QInputDialog::getInt( qApp->activeWindow(), Settings::instance().programName(),
#else
        int num_messages = QInputDialog::getInteger( qApp->activeWindow(), Settings::instance().programName(),
#endif
                                                     tr( "Please select the maximum number of messages to be showed" ),
                                                     Settings::instance().chatMessagesToShow(),
                                                     10, 1000, 5, &ok );
        if( ok )
        {
          Settings::instance().setChatMessagesToShow( num_messages );
          setChatMessagesToShowInAction( act );
        }
      }
      refresh_chat = true;
    }
    break;
  case 28:
    Settings::instance().setShowEmoticonMenu( act->isChecked() );
    break;
  case 29:
    Settings::instance().setKeyEscapeMinimizeInTray( act->isChecked() );
    break;
  case 30:
    Settings::instance().setConfirmOnDownloadFile( act->isChecked() );
    break;
  case 31:
    Settings::instance().setUseNativeEmoticons( act->isChecked() );
    updateEmoticons();
    refresh_chat = true;
  case 32:
    Settings::instance().setSaveUserList( act->isChecked() );
    break;
  case 33:
    Settings::instance().setShowImagePreview( act->isChecked() );
    break;
  case 34:
    Settings::instance().setBeepOnNewMessageArrived( act->isChecked() );
    break;
  case 35:
    Settings::instance().setShowMinimizedAtStartup( act->isChecked() );
    break;
  case 36:
    Settings::instance().setPromptOnCloseEvent( act->isChecked() );
    break;
  case 37:
    Settings::instance().setShowUserStatusDescription( act->isChecked() );
    refresh_users = true;
    break;
  case 38:
    Settings::instance().setShowUserStatusBackgroundColor( act->isChecked() );
    refresh_users = true;
    break;
  case 39:
    Settings::instance().setUseShortcuts( act->isChecked() );
    break;
  case 40:
    Settings::instance().setShowOnlyMessageNotificationOnTray( act->isChecked() );
    break;
  case 41:
    Settings::instance().setChatUseYourNameInsteadOfYou( act->isChecked() );
    refresh_chat = true;
    break;
  case 42:
    // free
    break;
  case 43:
    Settings::instance().setCheckNewVersionAtStartup( act->isChecked() );
    break;
  case 44:
    Settings::instance().setPostUsageStatistics( act->isChecked() );
    break;
  case 45:
    Settings::instance().setAskChangeUserAtStartup( act->isChecked() );
    break;
  case 46:
    Settings::instance().setShowChatMessageOnTray( act->isChecked() );
    break;
  case 47:
    Settings::instance().setChatClearAllReadMessages( act->isChecked() );
    break;
  case 48:
    Settings::instance().setShowFileTransferCompletedOnTray( act->isChecked() );
    break;
  case 49:
    Settings::instance().setSortUsersAscending( act->isChecked() );
    refresh_users = true;
    break;
  case 50:
    Settings::instance().setUserSortingMode( 0 );
    refresh_users = true;
    break;
  case 51:
    Settings::instance().setUserSortingMode( 1 );
    refresh_users = true;
    break;
  case 52:
    Settings::instance().setUserSortingMode( 2 );
    refresh_users = true;
    break;
  case 53:
    Settings::instance().setUserSortingMode( 3 );
    refresh_users = true;
    break;
  case 54:
    Settings::instance().setShowPresetMessages( act->isChecked() );
    break;
  case 55:
    Settings::instance().setShowTextInModeRTL( act->isChecked() );
    refresh_chat = true;
    break;
  case 56:
    Settings::instance().setPlayBuzzSound( act->isChecked() );
    break;
  case 99:
    break;
  default:
    qWarning() << "GuiMain::settingsChanged(): error in setting id" << act->data().toInt();
  }

  if( refresh_users )
    refreshUserList();

  if( refresh_chat )
  {
    QApplication::setOverrideCursor( Qt::WaitCursor );
    QApplication::processEvents();
    foreach( GuiFloatingChat* fl_chat, m_floatingChats )
    {
      Chat c = ChatManager::instance().chat( fl_chat->guiChat()->chatId() );
      if( c.isValid() )
        fl_chat->setChat( c );
    }
    QApplication::restoreOverrideCursor();
  }

  if( settings_data_id > 0 && settings_data_id < 99 )
    Settings::instance().save();
}

void GuiMain::setChatMessagesToShowInAction( QAction* act )
{
  act->setText( tr( "Show only last %1 messages" ).arg( Settings::instance().chatMessagesToShow() ) );
}

void GuiMain::sendMessage( VNumber chat_id, const QString& msg )
{
#ifdef BEEBEEP_DEBUG
  int num_messages = mp_core->sendChatMessage( chat_id, msg );
  qDebug() << num_messages << "messages sent";
#else
  mp_core->sendChatMessage( chat_id, msg );
#endif
}

void GuiMain::showAlertForMessage( const Chat& c, const ChatMessage& cm )
{
  if( c.isValid() && c.isGroup() && Settings::instance().isNotificationDisabledForGroup( c.privateId() ) )
  {
#ifdef BEEBEEP_DEBUG
    qDebug() << "Notification disabled for group:" << c.privateId() << c.name();
#endif
    return;
  }

  if( Settings::instance().beepOnNewMessageArrived() )
    playBeep();

  bool show_message_in_tray = true;

  GuiFloatingChat* fl_chat = floatingChat( c.id() );

  if( fl_chat )
  {
    if( fl_chat->chatIsVisible() )
      return;

    if( Settings::instance().raiseOnNewMessageArrived() )
    {
      fl_chat->showUp();
      show_message_in_tray = false;
    }

    fl_chat->setMainIcon( true );
    QApplication::alert( fl_chat, 0 );
  }
  else
  {
    if( Settings::instance().raiseOnNewMessageArrived() )
      showUp();
    QApplication::alert( this, 2000 );
  }

  if( show_message_in_tray )
  {
    User u = UserManager::instance().findUser( cm.userId() );
    QString msg;
    bool long_time_show = false;

    if( u.isValid() )
    {
      if( Settings::instance().showChatMessageOnTray() )
      {
        QString txt = Bee::removeHtmlTags( cm.message() );
        if( txt.size() > Settings::instance().textSizeInChatMessagePreviewOnTray() )
        {
          txt.truncate( Settings::instance().textSizeInChatMessagePreviewOnTray() );
          txt.append( "..." );
        }

        if( c.isDefault() )
          msg = QString( "%1 %2: %3" ).arg( u.name(), tr( "to all" ), txt );
        else if( c.isGroup() )
          msg = QString( "%1 %2 %3: %4" ).arg( u.name(), tr( "to" ), c.name(), txt );
        else
          msg = QString( "%1 %2: %4" ).arg( u.name(), tr( "to you" ), txt );

        long_time_show = true;
      }
      else
      {
        QString pre_msg = tr( "New message from" );
        if( c.isDefault() )
          msg = QString( "%1 %2 %3" ).arg( pre_msg, u.name(), tr( "to all" ) );
        else if( c.isGroup() )
          msg = QString( "%1 %2 %3 %4" ).arg( pre_msg, u.name(), tr( "to" ), c.name() );
        else
          msg = QString( "%1 %2 %3" ).arg( pre_msg, u.name(), tr( "to you" ) );
      }
    }
    else
      msg = tr( "New message arrived" );

    mp_trayIcon->showNewMessageArrived( c.id(), msg, long_time_show );
  }
  else
    mp_trayIcon->setUnreadMessages( c.id(), 0 );
}

void GuiMain::onNewChatMessage( const Chat& c, const ChatMessage& cm )
{
  if( c.isDefault() )
  {
    if( mp_home->addSystemMessage( cm ) && mp_tabMain->currentWidget() != mp_home )
    {
      m_unreadActivities++;
      updateTabTitles();
    }
  }

  GuiFloatingChat* fl_chat = floatingChat( c.id() );
  if( fl_chat )
    fl_chat->showChatMessage( c, cm );

  if( cm.isFromSystem() || cm.isFromLocalUser() )
    return;

  bool chat_is_visible = fl_chat && fl_chat->isActiveWindow();

  if( chat_is_visible )
  {
    readAllMessagesInChat( c.id() );
    if( !cm.isFromSystem() && !cm.isFromLocalUser() )
      fl_chat->statusBar()->showMessage( "" ); // reset writing message
  }
  else
  {
    showAlertForMessage( c, cm );
    mp_userList->updateChat( c );
    mp_chatList->updateChat( c );
    mp_groupList->updateChat( c );
  }

  updateNewMessageAction();
}

void GuiMain::searchUsers()
{
  GuiNetwork gn;
  gn.setModal( true );
  gn.loadSettings();
  gn.setSizeGripEnabled( true );
  gn.show();

  if( gn.exec() != QDialog::Accepted )
    return;

  if( !mp_core->isConnected() )
    return;

#ifdef BEEBEEP_USE_MULTICAST_DNS
  if( Settings::instance().useMulticastDns() )
    mp_core->startDnsMulticasting();
  else
    mp_core->stopDnsMulticasting();
#endif

  QMetaObject::invokeMethod( this, "sendBroadcastMessage", Qt::QueuedConnection );
}

void GuiMain::showWritingUser( const User& u, VNumber chat_id )
{
  QString msg = tr( "%1 is writing..." ).arg( u.name() );
  GuiFloatingChat* fl_chat = floatingChat( chat_id );
  if( fl_chat )
    fl_chat->statusBar()->showMessage( msg, Settings::instance().writingTimeout() );
}

void GuiMain::setUserStatusSelected( int user_status )
{
  if( user_status == User::Offline && mp_core->isConnected() )
  {
    if( !Settings::instance().promptOnCloseEvent() || QMessageBox::question( this, Settings::instance().programName(),
                               tr( "Do you want to disconnect from %1 network?" ).arg( Settings::instance().programName() ),
                               QMessageBox::Yes, QMessageBox::No ) == QMessageBox::Yes )
      stopCore();
    return;
  }

  mp_core->setLocalUserStatus( user_status );

  if( !mp_core->isConnected() )
    startCore();
  else
    updateStatusIcon();
}

void GuiMain::statusSelected()
{
  QAction* act = qobject_cast<QAction*>( sender() );
  if( !act )
    return;

  int user_status = act->data().toInt();
  setUserStatusSelected( user_status );
}

void GuiMain::updateStatusIcon()
{
  int status_type;
  if( !mp_core->isConnected() )
    status_type = User::Offline;
  else
    status_type = Settings::instance().localUser().status();

  mp_menuStatus->setIcon( Bee::avatarForUser( Settings::instance().localUser(), Settings::instance().avatarIconSize(), true ) );
  QString tip = tr( "You are %1%2" ).arg( Bee::userStatusToString( status_type ) )
      .arg( (Settings::instance().localUser().statusDescription().isEmpty() ? QString( "" ) : QString( ": %1" ).arg( Settings::instance().localUser().statusDescription() ) ) );
  QAction* act = mp_menuStatus->menuAction();
  act->setToolTip( tip );
  act->setText( Bee::capitalizeFirstLetter( Bee::userStatusToString( status_type ), true ) );
  updateWindowTitle();
}

void GuiMain::changeStatusDescription()
{
  bool ok = false;
  QString status_description = QInputDialog::getText( this, Settings::instance().programName(),
                           tr( "Please insert the new status description" ), QLineEdit::Normal, Settings::instance().localUser().statusDescription(), &ok );
  if( !ok || status_description.isNull() )
    return;
  mp_core->setLocalUserStatusDescription( Settings::instance().localUser().status(), status_description, true );
  loadUserStatusRecentlyUsed();
  updateStatusIcon();
}

void GuiMain::sendFileFromChat( VNumber chat_id, const QString& file_path )
{
  Chat c = ChatManager::instance().chat( chat_id );
  if( !c.isValid() )
    return;

  if( c.isDefault() && !Settings::instance().chatWithAllUsersIsEnabled() )
    return;

  QStringList files_path_selected = checkFilePath( file_path );
  if( files_path_selected.isEmpty() )
    return;

  UserList chat_members = UserManager::instance().userList().fromUsersId( c.usersId() );
  foreach( User u, chat_members.toList() )
    sendFiles( u, files_path_selected, chat_id );
}

void GuiMain::sendFile( VNumber user_id )
{
  User u = UserManager::instance().findUser( user_id );
  QStringList files_path_selected = checkFilePath( "" );
  if( files_path_selected.isEmpty() )
    return;
  sendFiles( u, files_path_selected, ID_INVALID );
}

void GuiMain::sendFiles( const User& u, const QStringList& file_list, VNumber chat_id )
{
  foreach( QString file_path, file_list )
  {
    if( !sendFile( u, file_path, chat_id ) )
      return;
  }
}

QStringList GuiMain::checkFilePath( const QString& file_path )
{
  QStringList files_path_selected;
  if( file_path.isEmpty() || !QFile::exists( file_path ) )
  {
    files_path_selected = FileDialog::getOpenFileNames( true, activeWindow(), tr( "%1 - Select a file" ).arg( Settings::instance().programName() ) + QString( " %1" ).arg( tr( "or more" ) ),
                                                       Settings::instance().lastDirectorySelected() );
    if( files_path_selected.isEmpty() )
      return files_path_selected;

    Settings::instance().setLastDirectorySelectedFromFile( files_path_selected.last() );
  }
  else
  {
    files_path_selected.append( file_path );
  }

  return files_path_selected;
}

bool GuiMain::sendFile( const User& u, const QString& file_path, VNumber chat_id )
{
  if( !Settings::instance().enableFileTransfer() )
  {
    QMessageBox::information( activeWindow(), Settings::instance().programName(), tr( "File transfer is not enabled." ) );
    return false;
  }

  if( !mp_core->isConnected() )
  {
    QMessageBox::information( activeWindow(), Settings::instance().programName(), tr( "You are not connected." ) );
    return false;
  }

  User user_selected;

  if( !u.isValid() )
  {
    QStringList user_string_list = UserManager::instance().userList().toStringList( false, true );
    if( user_string_list.isEmpty() )
    {
      QMessageBox::information( activeWindow(), Settings::instance().programName(), tr( "There is no user connected." ) );
      return false;
    }

    bool ok = false;
    QString user_path = QInputDialog::getItem( activeWindow(), Settings::instance().programName(),
                                        tr( "Please select the user to whom you would like to send a file."),
                                        user_string_list, 0, false, &ok );
    if( !ok )
      return false;

    user_selected = UserManager::instance().findUserByPath( user_path );

    if( !user_selected.isValid() )
    {
      QMessageBox::warning( activeWindow(), Settings::instance().programName(), tr( "User not found." ) );
      return false;
    }

    Chat c = ChatManager::instance().privateChatForUser( user_selected.id() );
    chat_id = c.id();
  }
  else
    user_selected = u;

  return mp_core->sendFile( user_selected.id(), file_path, "", false, chat_id );
}

void GuiMain::sendFile( const QString& file_path )
{
  sendFile( User(), file_path, ID_INVALID );
}

bool GuiMain::askToDownloadFile( const User& u, const FileInfo& fi, const QString& download_path, bool make_questions )
{
  if( !Settings::instance().enableFileTransfer() )
  {
    QMessageBox::warning( activeWindow(), Settings::instance().programName(), tr( "File transfer is disabled. You cannot download %1." ).arg( fi.name() ) );
    return false;
  }

  int msg_result = make_questions ? 0 : 1;
  bool auto_file_name = Settings::instance().automaticFileName();
  if( !make_questions )
    auto_file_name = true;

  if( msg_result == 0 )
  {
    if( Settings::instance().confirmOnDownloadFile() )
    {
      if( isMinimized() || !isActiveWindow() )
      {
        if( Settings::instance().raiseOnNewMessageArrived() )
          showUp();
        QApplication::alert( this, 0 );
      }
      QString msg = tr( "Do you want to download %1 (%2) from %3?" ).arg( fi.name(), Bee::bytesToString( fi.size() ), u.name() );
      msg_result = QMessageBox::question( this, Settings::instance().programName(), msg, tr( "No" ), tr( "Yes" ), tr( "Yes, and don't ask anymore" ), 0, 0 );
    }
    else
      msg_result = 1;
  }

  if( msg_result == 2 )
  {
    qDebug() << "Prompt on download file disabled by user request";
    Settings::instance().setConfirmOnDownloadFile( false );
    mp_actConfirmDownload->setChecked( false );
  }

  if( msg_result > 0 )
  {
    // Accepted
    qDebug() << "You accept to download" << fi.name() << "from" << u.path();

    QFileInfo qfile_info( download_path, fi.name() );
    if( qfile_info.exists() )
    {
      if( !Settings::instance().overwriteExistingFiles() )
      {
        QString file_name;
        if( auto_file_name )
        {
          file_name = Bee::uniqueFilePath( qfile_info.absoluteFilePath(), true );
          qDebug() << "File" << qfile_info.absoluteFilePath() << "exists. Save with" << file_name;
        }
        else
        {
          file_name = FileDialog::getSaveFileName( this,
                            tr( "%1 already exists. Please select a new filename." ).arg( qfile_info.fileName() ),
                            qfile_info.absoluteFilePath() );
          if( file_name.isNull() || file_name.isEmpty() )
            return false;
        }
        qfile_info = QFileInfo( file_name );
      }
    }
    FileInfo file_info = fi;
    file_info.setName( qfile_info.fileName() );
    file_info.setPath( qfile_info.absoluteFilePath() );
    file_info.setSuffix( qfile_info.suffix() );
    return mp_core->downloadFile( u.id(), file_info, make_questions );
  }
  else
  {
    qDebug() << "You refuse to download" << fi.name() << "from" << u.path();
    return false;
  }
}

void GuiMain::downloadFile( const User& u, const FileInfo& fi )
{
  if( !askToDownloadFile( u, fi, Settings::instance().downloadDirectory(), true ) )
    mp_core->refuseToDownloadFile( u.id(), fi );
}

void GuiMain::downloadSharedFiles( const QList<SharedFileInfo>& share_file_info_list )
{
  if( share_file_info_list.isEmpty() )
    return;

  QString download_folder;
  User u;
  int files_to_download = 0;

  if( share_file_info_list.size() > Settings::instance().maxQueuedDownloads() )
  {
    if( QMessageBox::question( activeWindow(), Settings::instance().programName(),
                               tr( "You cannot download all these files at once. Do you want to download the first %1 files of the list?" )
                               .arg( Settings::instance().maxQueuedDownloads() ),
                               tr( "Yes" ), tr( "No" ), QString::null, 1, 1 ) != 0 )
      return;
  }
  else if( share_file_info_list.size() > 100 )
  {
    if( QMessageBox::question( activeWindow(), Settings::instance().programName(),
                           tr( "Downloading %1 files is a hard duty. Maybe you have to wait a lot of minutes. Do yo want to continue?" ).arg( share_file_info_list.size() ),
                           tr( "Yes" ), tr( "No" ), QString::null, 1, 1 ) != 0 )
      return;
  }

  foreach( SharedFileInfo sfi, share_file_info_list )
  {
    download_folder = Bee::convertToNativeFolderSeparator( QString( "%1/%2" ).arg( Settings::instance().downloadDirectory(), sfi.second.shareFolder() ) );
    u = UserManager::instance().findUser( sfi.first );
    if( !askToDownloadFile( u, sfi.second, download_folder, false ) )
      return;

    files_to_download++;

    if( files_to_download > Settings::instance().maxQueuedDownloads() )
      break;
  }

  showMessage( tr( "Downloading %1 files" ).arg( files_to_download ), 5000 );
}

void GuiMain::downloadSharedFile( VNumber user_id, VNumber file_id )
{
  User u = UserManager::instance().findUser( user_id );
  FileInfo file_info = FileShare::instance().networkFileInfo( user_id, file_id );

  if( u.isStatusConnected() && file_info.isValid() )
  {
    askToDownloadFile( u, file_info, Settings::instance().downloadDirectory(), true );
    return;
  }

  qWarning() << "Unable to download shared file" << file_id << "from user" << user_id;
  QString info_msg = tr( "File is not available for download." );
  if( u.isValid() && !u.isStatusConnected() )
    info_msg += QLatin1String( "\n" ) + tr( "%1 is not connected." ).arg( u.name() );
  info_msg += QLatin1String( "\n" ) + tr( "Please reload the list of shared files." );

  if( QMessageBox::information( activeWindow(), Settings::instance().programName(), info_msg,
                              tr( "Reload file list" ), tr( "Cancel" ), QString::null, 1, 1 ) == 0 )
  {
    if( mp_fileSharing )
      mp_fileSharing->updateNetworkFileList();
  }
}

void GuiMain::downloadFolder( const User& u, const QString& folder_name, const QList<FileInfo>& file_info_list )
{
  if( !Settings::instance().enableFileTransfer() )
  {
    QMessageBox::warning( activeWindow(), Settings::instance().programName(), tr( "File transfer is disabled. You cannot download %1." ).arg( folder_name ) );
    return;
  }

  if( file_info_list.isEmpty() )
  {
    qWarning() << "Unable to download folder" << folder_name << "from user" << qPrintable( u.path() ) << "with empty file list";
    return;
  }

  int msg_result = Settings::instance().confirmOnDownloadFile() ? 0 : 1;

  if( msg_result == 0 )
  {
    QString msg = tr( "Do you want to download folder %1 (%2 files) from %3?" ).arg( folder_name ).arg( file_info_list.size() ).arg( u.name() );
    msg_result = QMessageBox::question( activeWindow(), Settings::instance().programName(), msg, tr( "No" ), tr( "Yes" ), tr( "Yes, and don't ask anymore" ), 0, 0 );
  }

  if( msg_result == 2 )
  {
    Settings::instance().setConfirmOnDownloadFile( false );
    mp_actConfirmDownload->setChecked( false );
  }

  if( msg_result > 0 )
  {
    // Accepted
    qDebug() << "You accept to download folder" << folder_name << "from" << u.path();
    QString download_folder;
    int files_to_download = 0;
    foreach( FileInfo fi, file_info_list )
    {
      download_folder = Bee::convertToNativeFolderSeparator( QString( "%1/%2" ).arg( Settings::instance().downloadDirectory(), fi.shareFolder() ) );
      if( !askToDownloadFile( u, fi, download_folder, false ) )
        return;

      files_to_download++;

      if( files_to_download > Settings::instance().maxQueuedDownloads() )
      {
        qWarning() << "Unable to download all the" << file_info_list.size() << "files because max queued reached" << Settings::instance().maxQueuedDownloads();
        break;
      }
    }
  }
  else
  {
    qDebug() << "You refuse to download folder" << folder_name << "from" << u.path();
    mp_core->refuseToDownloadFolder( u.id(), folder_name, file_info_list.first().chatPrivateId() );
  }
}

void GuiMain::selectDownloadDirectory()
{
  QString download_directory_path = FileDialog::getExistingDirectory( activeWindow(),
                                                                       tr( "%1 - Select the download folder" )
                                                                       .arg( Settings::instance().programName() ),
                                                                       Settings::instance().downloadDirectory() );
  if( download_directory_path.isEmpty() )
    return;

  Settings::instance().setDownloadDirectory( download_directory_path );
}

void GuiMain::showTipOfTheDay()
{
  mp_tabMain->setCurrentWidget( mp_home );
  mp_core->showTipOfTheDay();
}

void GuiMain::showFactOfTheDay()
{
  mp_tabMain->setCurrentWidget( mp_home );
  mp_core->showFactOfTheDay();
}

void GuiMain::showChat( VNumber chat_id )
{
  Chat c = ChatManager::instance().chat( chat_id );
  if( !c.isValid() )
  {
    qWarning() << "Invalid chat" << chat_id << "found in GuiMain::showChat(...)";
    return;
  }

  GuiFloatingChat* fl_chat = createFloatingChat( c );
  if( !fl_chat )
    return;

  if( !fl_chat->chatIsVisible() )
    fl_chat->showUp();

  fl_chat->setFocusInChat();
}

void GuiMain::changeVCard()
{
  GuiEditVCard gvc;
  gvc.setModal( true );
  gvc.setUser( Settings::instance().localUser() );
  gvc.setSizeGripEnabled( true );
  gvc.show();

  if( gvc.exec() == QDialog::Accepted )
  {
    mp_core->setLocalUserVCard( gvc.userColor(), gvc.vCard() );
    updateStatusIcon();
  }
}

void GuiMain::showLocalUserVCard()
{
  showVCard( ID_LOCAL_USER );
}

void GuiMain::showVCard( VNumber user_id )
{
  User u = UserManager::instance().findUser( user_id );
  if( !u.isValid() )
    return;

  GuiVCard* gvc = new GuiVCard( this );
  connect( gvc, SIGNAL( showChat( VNumber ) ), this, SLOT( showChat( VNumber ) ) );
  connect( gvc, SIGNAL( sendFile( VNumber ) ), this, SLOT( sendFile( VNumber ) ) );
  connect( gvc, SIGNAL( changeUserColor( VNumber, const QString& ) ), this, SLOT( changeUserColor( VNumber, const QString& ) ) );
  connect( gvc, SIGNAL( toggleFavorite( VNumber ) ), this, SLOT( toggleUserFavorite( VNumber ) ) );
  connect( gvc, SIGNAL( removeUser( VNumber ) ), this, SLOT( removeUserFromList( VNumber ) ) );
  connect( gvc, SIGNAL( buzzUser( VNumber ) ), this, SLOT( sendBuzzToUser( VNumber ) ) );
  gvc->setVCard( u, ChatManager::instance().privateChatForUser( u.id() ).id(), mp_core->isConnected() );

  QPoint cursor_pos = QCursor::pos();
  QRect screen_rect = qApp->desktop()->availableGeometry( cursor_pos );
  int diff_margin =  (cursor_pos.x() + gvc->size().width()+5) - screen_rect.width();
  if( diff_margin > 0 )
    cursor_pos.setX( cursor_pos.x() - gvc->size().width() );
  gvc->move( cursor_pos );
  gvc->show();
  gvc->setFixedSize( gvc->size() );
}

void GuiMain::updadePluginMenu()
{
  mp_menuPlugins->clear();
  QAction* act = mp_menuPlugins->addAction( QIcon( ":/images/plugin.png" ), tr( "Plugin Manager..." ), this, SLOT( showPluginManager() ) );

  QString help_data_ts = tr( "is a plugin developed by" );
  QString help_data_format = QString( "<p>%1 <b>%2</b> %3 <b>%4</b>.<br /><i>%5</i></p><br />" );

  mp_menuPlugins->addAction( mp_actViewScreenShot );

  bool copymastro_available = false;
  QString copy_mastro_path = "";
#ifdef Q_OS_WIN
  copy_mastro_path = QString( "%1\\%2" ).arg( Settings::instance().pluginPath(), QString( "CopyMastro.exe" ) );
  copymastro_available = QFile::exists( copy_mastro_path );
  if( !copymastro_available )
  {
    copy_mastro_path = QString( "C:\\Program Files (x86)\\CopyMastro\\CopyMastro.exe" );
    copymastro_available = QFile::exists( copy_mastro_path );
  }
#endif

  if( copymastro_available )
  {
    qDebug() << "CopyMastro is found:" << qPrintable( copy_mastro_path );
    act = mp_menuPlugins->addAction( QIcon( ":/images/CopyMastro.png" ), "CopyMastro", this, SLOT( startExternalApplicationFromActionData() ) );
    act->setToolTip( tr( "Start the new application to copy file and folders by Marco Mastroddi" ) );
    act->setData( copy_mastro_path );
  }

  if( PluginManager::instance().textMarkers().size() > 0 )
  {
    mp_menuPlugins->addSeparator();

    foreach( TextMarkerInterface* text_marker, PluginManager::instance().textMarkers() )
    {
      act = mp_menuPlugins->addAction( text_marker->name(), this, SLOT( showPluginHelp() ) );

      act->setData( help_data_format
                  .arg( Bee::iconToHtml( (text_marker->icon().isNull() ? ":/images/plugin.png" : text_marker->iconFileName()), "*P*" ),
                        text_marker->name(), help_data_ts, text_marker->author(), text_marker->help() ) );
      act->setIcon( text_marker->icon() );
      act->setEnabled( text_marker->isEnabled() );
    }
  }
}

void GuiMain::showPluginHelp()
{
  QAction* act = qobject_cast<QAction*>(sender());
  if( !act )
    return;

  QMessageBox::information( this, act->text(), act->data().toString() );
}

void GuiMain::showPluginManager()
{
  GuiPluginManager gpm;
  gpm.setModal( true );
  gpm.setSizeGripEnabled( true );
  gpm.updatePlugins();
  gpm.show();
  gpm.exec();
  if( gpm.isChanged() )
    updadePluginMenu();
}

bool GuiMain::showWizard()
{
  GuiWizard gw( this );
  gw.setModal( true );
  gw.loadSettings();
  gw.show();
  gw.setFixedSize( gw.size() );
  if( gw.exec() == QDialog::Accepted )
    return mp_core->changeLocalUser( gw.userName() );
  else
    return false;
}

void GuiMain::hideToTrayIcon()
{
  Chat c = ChatManager::instance().firstChatWithUnreadMessages();
  if( c.isValid() )
    mp_trayIcon->setUnreadMessages( c.id(), c.unreadMessages() );
  else
    mp_trayIcon->setUnreadMessages( ID_INVALID, 0 );
  hide();
}

void GuiMain::trayIconClicked( QSystemTrayIcon::ActivationReason ar )
{
#ifdef Q_OS_MAC

  // In Mac that is the expected behavior, there is no distinction
  // between left and right buttons for the systray icons.
  // They will always show the context menu, that's the Mac behavior.
  Q_UNUSED( ar );

#else

  // Other OS

  if( ar == QSystemTrayIcon::Context )
  {
#ifdef BEEBEEP_DEBUG
    qDebug() << "TrayIcon is activated with context click and menu is showed";
#endif
    return;
  }

  if( ar == QSystemTrayIcon::Trigger )
  {
    if( mp_menuTrayIcon->isVisible() )
    {
#ifdef BEEBEEP_DEBUG
      qDebug() << "TrayIcon is activated with trigger click and menu will be hided";
#endif
      mp_menuTrayIcon->hide();
      return;
    }

    if( !isActiveWindow() || isMinimized() )
    {
#ifdef BEEBEEP_DEBUG
      qDebug() << "TrayIcon is activated with trigger click and main window will be showed";
#endif
      trayMessageClicked();
    }
    else
    {
#ifdef BEEBEEP_DEBUG
      qDebug() << "TrayIcon is activated with trigger click and menu will be showed";
#endif
      mp_menuTrayIcon->popup( QCursor::pos() );
    }
  }
  else
  {
#ifdef BEEBEEP_DEBUG
    qDebug() << "TrayIcon is activated with unknown click";
#endif
  }
#endif
}

void GuiMain::trayMessageClicked()
{
  // QT 2017-04-24: Currently this signal is not sent on macOS.

  if( mp_trayIcon->chatId() != ID_INVALID && ChatManager::instance().chat( mp_trayIcon->chatId() ).isValid() )
  {
    VNumber chat_id = mp_trayIcon->chatId();
    showChat( chat_id );
  }
  else
    QMetaObject::invokeMethod( this, "showUp", Qt::QueuedConnection );
}

void GuiMain::addToShare( const QString& share_path )
{
  mp_core->addPathToShare( share_path );
}

void GuiMain::removeFromShare( const QString& share_path )
{
  mp_core->removePathFromShare( share_path );
}

void GuiMain::openUrl( const QUrl& file_url )
{
#ifdef BEEBEEP_DEBUG
  qDebug() << "Opening url (not encoded):" << file_url.toString();
#endif

  if( file_url.scheme() == QLatin1String( "beeshowfileinfolder" ) )
  {
    QUrl adj_file_url = file_url;
    adj_file_url.setScheme( QLatin1String( "file" ) );
    if( !Bee::showFileInGraphicalShell( Bee::convertToNativeFolderSeparator( adj_file_url.toLocalFile() ) ) )
    {
      QFileInfo file_info_url( adj_file_url.toLocalFile() );
      adj_file_url = QUrl::fromLocalFile( Bee::convertToNativeFolderSeparator( file_info_url.absoluteDir().absolutePath() ) );
      openUrl( adj_file_url );
      return;
    }
  }
#if QT_VERSION >= 0x040800
  else if( file_url.isLocalFile() )
#else
  else if( file_url.scheme() == QLatin1String( "file" ) )
#endif
  {
    QString file_path = Bee::convertToNativeFolderSeparator( file_url.toLocalFile() );
    if( file_path.isEmpty() )
    {
      qWarning() << "Unable to open an empty file path";
      return;
    }

    QFileInfo fi( file_path );
#ifdef Q_OS_MAC
    bool is_exe_file = fi.isBundle();
#else
    bool is_exe_file = fi.isExecutable() && !fi.isDir();
#endif
    if( is_exe_file && QMessageBox::question( this, Settings::instance().programName(),
                             tr( "Do you really want to open the file %1?" ).arg( file_path ),
                             tr( "Yes" ), tr( "No" ), QString(), 1, 1 ) != 0 )
      return;

    qDebug() << "Open file:" << file_path;
    if( !QDesktopServices::openUrl( QUrl::fromLocalFile( file_path ) ) )
      QMessageBox::information( this, Settings::instance().programName(),
                              tr( "Unable to open %1" ).arg( file_path.isEmpty() ? file_url.toString() : file_path ), tr( "Ok" ) );
  }
  else
  {
    QString url_txt = file_url.toString();
    qDebug() << "Open url:" << url_txt;
    if( QDesktopServices::openUrl( file_url ) )
    {
      if( url_txt == Settings::instance().facebookPage() )
        Settings::instance().setIsFacebookPageLinkClicked( true );
    }
    else
      qWarning() << "Unable to open link url:" << url_txt;
  }
}

void GuiMain::selectBeepFile()
{
  QString file_path = FileDialog::getOpenFileName( false, this, Settings::instance().programName(), Settings::instance().beepFilePath(), tr( "Sound files (*.wav)" ) );
  if( file_path.isNull() || file_path.isEmpty() )
    return;

  Settings::instance().setBeepFilePath( file_path );
  qDebug() << "New sound file selected:" << file_path;

  AudioManager::instance().clearBeep();

  if( !Settings::instance().beepOnNewMessageArrived() )
  {
    if( QMessageBox::question( this, Settings::instance().programName(), tr( "Sound is not enabled on a new message. Do you want to enable it?" ), tr( "Yes" ), tr( "No" ) ) == 0 )
    {
      Settings::instance().setBeepOnNewMessageArrived( true );
      mp_actBeepOnNewMessage->setChecked( true );
    }
  }
}

void GuiMain::testBeepFile()
{
  QString s_default_beep = tr( "The default BEEP will be used" );
  if( !AudioManager::instance().isAudioDeviceAvailable() )
  {
    qWarning() << "Sound device is not available";
    QMessageBox::warning( this, Settings::instance().programName(), QString( "%1. %2." ).arg( tr( "Sound module is not working" ), s_default_beep  ) );
  }
  else if( !QFile::exists( Settings::instance().beepFilePath() ) )
  {
    QString warn_text = QString( "%1\n%2. %3." ).arg( Settings::instance().beepFilePath() )
                                                  .arg( tr( "Sound file not found" ) )
                                                  .arg( s_default_beep );
    QMessageBox::warning( this, Settings::instance().programName(), warn_text );
  }

  playBeep();
}

void GuiMain::playBeep()
{
  AudioManager::instance().playBeep();
}

void GuiMain::createGroup()
{
  GuiCreateGroup gcg;
  gcg.loadData( true );
  gcg.setModal( true );
  gcg.show();
  gcg.setFixedSize( gcg.size() );
  if( gcg.exec() == QDialog::Accepted )
  {
    Group g = mp_core->createGroup( gcg.selectedName(), gcg.selectedUsersId() );
    if( g.isValid() )
      showChatForGroup( g.id() );
  }
}

void GuiMain::editGroup( VNumber group_id )
{
  Group g = UserManager::instance().group( group_id );
  if( !g.isValid() )
    return;

  GuiCreateGroup gcg( activeWindow() );
  gcg.init( g.name(), g.usersId() );
  gcg.loadData( true );
  gcg.setModal( true );
  gcg.show();
  gcg.setFixedSize( gcg.size() );
  if( gcg.exec() == QDialog::Accepted )
    mp_core->changeGroup( group_id, gcg.selectedName(), gcg.selectedUsersId() );
}

void GuiMain::createChat()
{
  switch( QMessageBox::question( this, Settings::instance().programName(),
                 tr( "Group chat will be deleted when all members goes offline." ) + QString( " " ) +
                 tr( "If you want a persistent chat please consider to make a Group instead." ) + QString( " " ) +
                 tr( "Do you wish to continue or create group?" ),
                 tr( "Continue" ), tr( "Create Group" ), tr( "Cancel" ), 0, 2 ) )
  {
  case 0:
    // do nothing
    break;
  case 1:
    createGroup();
    return;
  default:
    return;
  }

  GuiCreateGroup gcg;
  gcg.loadData( false );
  gcg.setModal( true );
  gcg.show();
  gcg.setFixedSize( gcg.size() );
  if( gcg.exec() == QDialog::Accepted )
  {
    Chat c = mp_core->createGroupChat( gcg.selectedName(), gcg.selectedUsersId(), "", true );
    if( c.isValid() )
      showChat( c.id() );
  }
}

void GuiMain::editChat( VNumber chat_id )
{
  Chat group_chat_tmp = ChatManager::instance().chat( chat_id );
  if( !group_chat_tmp.isValid() )
    return;

  if( !group_chat_tmp.isGroup() )
  {
    qWarning() << "Unable to edit chat" << chat_id << qPrintable( group_chat_tmp.name() ) << "because it is not a group";
    return;
  }

  Group g = UserManager::instance().findGroupByPrivateId( group_chat_tmp.privateId() );
  if( g.isValid() )
  {
    editGroup( g.id() );
    return;
  }

  GuiCreateGroup gcg( activeWindow() );
  gcg.init( group_chat_tmp.name(), group_chat_tmp.usersId() );
  gcg.loadData( false );
  gcg.setModal( true );
  gcg.show();
  gcg.setFixedSize( gcg.size() );
  if( gcg.exec() == QDialog::Accepted )
    mp_core->changeGroupChat( Settings::instance().localUser(), group_chat_tmp.id(), gcg.selectedName(), gcg.selectedUsersId() );
}

void GuiMain::checkAutoStartOnBoot( bool add_service )
{
  if( add_service )
  {
    if( Settings::instance().addStartOnSystemBoot() )
      QMessageBox::information( this, Settings::instance().programName(), tr( "Now %1 will start on windows boot." ).arg( Settings::instance().programName() ) );
    else
      QMessageBox::warning( this, Settings::instance().programName(), tr( "Unable to add this key in the registry: permission denied." ) );
  }
  else
  {
    if( Settings::instance().removeStartOnSystemBoot() )
      QMessageBox::information( this, Settings::instance().programName(), tr( "%1 will not start on windows boot." ).arg( Settings::instance().programName() ) );
    else
      QMessageBox::warning( this, Settings::instance().programName(), tr( "Unable to add this key in the registry: permission denied." ) );
  }
}

void GuiMain::loadSession()
{
  showMessage( tr( "Starting" ), 3000 );
  QTimer::singleShot( 200, mp_core, SLOT( buildSavedChatList() ) );
  mp_tabMain->setCurrentWidget( mp_home );
  mp_home->loadSystemMessages();
  mp_groupList->loadGroups();
}

void GuiMain::showSavedChatSelected( const QString& chat_name )
{
  if( chat_name.isEmpty() )
    return;

  foreach( QWidget* w, qApp->allWidgets() )
  {
    GuiSavedChat* gsv = qobject_cast<GuiSavedChat*>( w );
    if( gsv && gsv->savedChatName() == chat_name )
    {
      gsv->raise();
      return;
    }
  }

  GuiSavedChat* saved_chat = new GuiSavedChat( this );
  saved_chat->setAttribute( Qt::WA_DeleteOnClose, true );
  connect( saved_chat, SIGNAL( deleteSavedChatRequest( const QString& ) ), this, SLOT( removeSavedChat( const QString& ) ) );
  connect( saved_chat, SIGNAL( openUrl( const QUrl& ) ), this, SLOT( openUrl( const QUrl& ) ) );
  saved_chat->showSavedChat( chat_name );
  saved_chat->show();
}

void GuiMain::removeSavedChat( const QString& chat_name )
{
  mp_core->removeSavedChat( chat_name );
  mp_savedChatList->updateSavedChats();
  updateTabTitles();
}

void GuiMain::linkSavedChat( const QString& chat_name )
{
  bool ok = false;
  QStringList chat_names_string_list = ChatManager::instance().chatNamesToStringList( true );
  chat_names_string_list.removeOne( chat_name );

  QString chat_name_selected = QInputDialog::getItem( this, Settings::instance().programName(),
                                        tr( "Please select a chat you would like to link the saved text."),
                                        chat_names_string_list, 0, false, &ok );
  if( !ok )
    return;

  bool add_to_existing_saved_text = false;
  if( ChatManager::instance().chatHasSavedText( chat_name_selected ) )
  {
     switch( QMessageBox::question( this, Settings::instance().programName(),
               tr( "The chat '%1' selected has already a saved text.<br />"
                   "What do you want to do with the selected saved text?" ).arg( chat_name_selected ),
                   tr( "Overwrite" ), tr( "Add in the head" ), tr( "Cancel" ), 2, 2 ) )
     {
     case 0:
       break;
     case 1:
       add_to_existing_saved_text = true;
       break;
     default:
       return;
     }

  }

  ChatManager::instance().updateChatSavedText( chat_name, chat_name_selected, add_to_existing_saved_text );
  mp_savedChatList->updateSavedChats();

  Chat c = ChatManager::instance().findChatByName( chat_name );
  if( c.isValid() )
  {
    GuiFloatingChat* fl_chat = floatingChat( c.id() );
    if( fl_chat )
      fl_chat->setChat( c );
  }

  c = ChatManager::instance().findChatByName( chat_name_selected );
  if( c.isValid() )
  {
    GuiFloatingChat* fl_chat = floatingChat( c.id() );
    if( fl_chat )
      fl_chat->setChat( c );
  }
}

bool GuiMain::openWebUrl( const QString& web_url )
{
  QUrl url( web_url );

  if( !QDesktopServices::openUrl( url ) )
  {
    QMessageBox::information( this, Settings::instance().programName(), tr( "Unable to open %1" ).arg( web_url ), tr( "Ok" ) );
    return false;
  }
  else
    return true;
}

void GuiMain::checkNewVersion()
{
  QString url_and_arguments = Settings::instance().checkVersionWebSite();
  openWebUrl( url_and_arguments );
}

void GuiMain::openWebSite()
{
  openWebUrl( Settings::instance().officialWebSite() );
}

void GuiMain::openDownloadPluginPage()
{
  openWebUrl( Settings::instance().pluginWebSite() );
}

void GuiMain::openDonationPage()
{
  openWebUrl( Settings::instance().donationWebSite() );
}

void GuiMain::openHelpPage()
{
  openWebUrl( Settings::instance().helpWebSite() );
}

void GuiMain::openFacebookPage()
{
  if( openWebUrl( Settings::instance().facebookPage() ) )
    Settings::instance().setIsFacebookPageLinkClicked( true );
}

void GuiMain::setInIdle()
{
  if( !mp_core->isConnected() )
    return;

  if( !Settings::instance().autoUserAway() )
    return;

  if( Settings::instance().localUser().status() == User::Away )
    return;

  m_lastUserStatus = Settings::instance().localUser().status();
  mp_core->setLocalUserStatus( User::Away );
  updateStatusIcon();
}

void GuiMain::exitFromIdle()
{
  if( !mp_core->isConnected() )
    return;

  if( !Settings::instance().autoUserAway() )
    return;

  if( Settings::instance().localUser().status() != User::Away )
    return;

  mp_core->setLocalUserStatus( m_lastUserStatus );
  updateStatusIcon();
}

void GuiMain::showMessage( const QString& status_msg, int time_out )
{
  statusBar()->showMessage( status_msg, time_out );
}

void GuiMain::changeUserColor( VNumber user_id, const QString& user_color )
{
  QColor c = QColorDialog::getColor( QColor( user_color ), qApp->activeWindow() );
  if( c.isValid() )
    mp_core->changeUserColor( user_id, c.name() );
}

void GuiMain::checkGroup( VNumber group_id )
{
  if( UserManager::instance().group( group_id ).isValid() )
    mp_groupList->updateGroup( group_id );
  else
    mp_groupList->loadGroups();
}

void GuiMain::onChatChanged( const Chat& c )
{
  mp_userList->updateChat( c );
  mp_chatList->updateChat( c );
  mp_savedChatList->updateSavedChats();
  GuiFloatingChat* fl_chat = floatingChat( c.id() );
  if( fl_chat )
    fl_chat->setChat( c );
  updateTabTitles();
}

bool GuiMain::checkAllChatMembersAreConnected( const QList<VNumber>& users_id )
{
  if( !mp_core->areUsersConnected( users_id ) )
  {
    if( QMessageBox::question( activeWindow(), Settings::instance().programName(),
                               tr( "All the members of this chat are not online. The changes may not be permanent. Do you wish to continue?" ),
                               tr( "Yes" ), tr( "No" ), QString::null, 1, 1 ) != 0 )
       return false;
  }
  return true;
}

void GuiMain::leaveGroupChat( VNumber chat_id )
{
  Chat c = ChatManager::instance().chat( chat_id );
  if( !c.isValid() )
    return;
  if( !c.hasUser( ID_LOCAL_USER ) )
    return;

  Group g = UserManager::instance().findGroupByPrivateId( c.privateId() );
  if( g.isValid() )
  {
    if( QMessageBox::warning( activeWindow(), Settings::instance().programName(),
                              tr( "%1 is a your group. You cannot leave the chat until the group exists." ).arg( g.name() ),
                              tr( "Delete this group" ), tr( "Cancel" ), QString(), 1, 1 ) == 1 )
        return;

    if( !mp_core->removeGroup( g.id() ) )
    {
      QMessageBox::warning( activeWindow(), Settings::instance().programName(), tr( "You cannot delete %1." ).arg( g.name() ) );
      return;
    }

    mp_groupList->loadGroups();
    updateTabTitles();
  }

  if( !checkAllChatMembersAreConnected( c.usersId() ) )
    return;

  if( !mp_core->removeUserFromChat( Settings::instance().localUser(), c.privateId() ) )
    QMessageBox::warning( activeWindow(), Settings::instance().programName(), tr( "You cannot leave %1." ).arg( c.name() ) );
}

void GuiMain::removeGroup( VNumber group_id )
{
  Group g = UserManager::instance().group( group_id );
  if( g.isValid() )
  {
    if( QMessageBox::question( activeWindow(), Settings::instance().programName(),
                               tr( "Do you really want to delete group '%1'?" ).arg( g.name() ),
                               tr( "Yes" ), tr( "No" ), QString::null, 1, 1 ) == 0 )
    {
      if( mp_core->removeGroup( group_id ) )
      {
        mp_groupList->loadGroups();
        updateTabTitles();
      }
      else
        QMessageBox::warning( activeWindow(), Settings::instance().programName(), tr( "You cannot delete %1." ).arg( g.name() ) );
    }
  }
}

void GuiMain::clearChat( VNumber chat_id )
{
  Chat c = ChatManager::instance().chat( chat_id );
  if( !c.isValid() )
    return;
  QString chat_name = c.isDefault() ? QObject::tr( "All Lan Users" ).toLower() : c.name();
  if( c.isEmpty() && !ChatManager::instance().chatHasSavedText( c.name() ) )
  {
    QMessageBox::information( activeWindow(), Settings::instance().programName(), tr( "Chat with %1 is empty." ).arg( chat_name ) );
    return;
  }

  QString question_txt = tr( "Do you really want to clear messages with %1?" ).arg( chat_name );
  QString button_2_text;
  if( ChatManager::instance().chatHasSavedText( c.name() ) )
    button_2_text = QString( "  " ) + tr( "Yes and delete history" ) + QString( "  " );

  switch( QMessageBox::information( activeWindow(), Settings::instance().programName(), question_txt, tr( "Yes" ), tr( "No" ), button_2_text, 1, 1 ) )
  {
  case 0:
    mp_core->clearMessagesInChat( chat_id, false );
    break;
  case 2:
    mp_core->clearMessagesInChat( chat_id, true );
    break;
  default:
    return;
  }

  updateTabTitles();
}

void GuiMain::removeChat( VNumber chat_id )
{
  Chat c = ChatManager::instance().chat( chat_id );
  if( !c.isValid() )
  {
    qWarning() << "Invalid chat" << chat_id << "found in removeChat function";
    return;
  }

  QString question_txt = tr( "Do you really want to delete chat with %1?" ).arg( c.name() );
  if( QMessageBox::information( activeWindow(), Settings::instance().programName(), question_txt, tr( "Yes" ), tr( "No" ), QString::null, 1, 1 ) != 0 )
    return;

  if( mp_core->removeChat( chat_id ) )
  {
    mp_chatList->reloadChatList();
    updateTabTitles();
    GuiFloatingChat* fl_chat = floatingChat( chat_id );
    if( fl_chat )
      fl_chat->close();
  }
  else
    QMessageBox::warning( activeWindow(), Settings::instance().programName(), tr( "Unable to delete %1." ).arg( c.name() ) );
}

void GuiMain::showChatForGroup( VNumber group_id )
{
  Group g = UserManager::instance().group( group_id );
  if( !g.isValid() )
    return;

  Chat c = ChatManager::instance().findChatByPrivateId( g.privateId(), true, ID_INVALID );
  if( !c.isValid() )
    c = mp_core->createGroupChat( g.name(), g.usersId(), g.privateId(), true );

  showChat( c.id() );
}

void GuiMain::showSharesForUser( const User& u )
{
  if( mp_fileSharing )
    mp_fileSharing->showUserFileList( u );
}

void GuiMain::selectLanguage()
{
  GuiLanguage gl;
  gl.setModal( true );
  gl.loadLanguages();
  gl.setSizeGripEnabled( true );
  gl.show();

  if( gl.exec() == QDialog::Rejected )
    return;

  QString old_language_path = Settings::instance().languageFilePath( Settings::instance().languagePath(), Settings::instance().language() );
  QString new_language_path = Settings::instance().languageFilePath( gl.folderSelected(), gl.languageSelected() );

  if( old_language_path != new_language_path )
  {
    QString language_message;
    if( gl.languageSelected().isEmpty() )
      language_message = tr( "Default language is restored." );
    else
      language_message = tr( "New language '%1' is selected." ).arg( gl.languageSelected() );

    QMessageBox::information( this, Settings::instance().programName(),
                              QString( "%1<br />%2" ).arg( language_message ).arg( tr( "You must restart %1 to apply these changes." )
                                                                                    .arg( Settings::instance().programName() ) ) );

    Settings::instance().setLanguage( gl.languageSelected() );
    Settings::instance().setLanguagePath( gl.folderSelected() );
  }
}

void GuiMain::showAddUser()
{
  GuiAddUser gad;
  gad.loadUsers();
  gad.setModal( true );
  gad.setSizeGripEnabled( true );
  gad.show();

  if( gad.exec() == QDialog::Accepted )
  {
    if( !Settings::instance().userPathList().isEmpty() )
      QMetaObject::invokeMethod( this, "sendBroadcastMessage", Qt::QueuedConnection );
  }
}

void GuiMain::showChatSettingsMenu()
{
  mp_menuChat->clear();

  QAction* act = mp_menuChat->addAction( tr( "Use RTL mode to show text" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().showTextInModeRTL() );
  act->setData( 55 );

  mp_menuChat->addSeparator();

  act = mp_menuChat->addAction( tr( "Show the chat in compact view mode" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().chatCompact() );
  act->setData( 1 );

  act = mp_menuChat->addAction( tr( "Show the timestamp" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().chatShowMessageTimestamp() );
  act->setData( 3 );

  act = mp_menuChat->addAction( tr( "Show preview of the images" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().showImagePreview() );
  act->setData( 33 );

  act = mp_menuChat->addAction( tr( "Show messages grouped by user" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().showMessagesGroupByUser() );
  act->setData( 13 );

  act = mp_menuChat->addAction( "", this, SLOT( settingsChanged() ) );
  setChatMessagesToShowInAction( act );
  act->setCheckable( true );
  act->setChecked( Settings::instance().chatMaxMessagesToShow() );
  act->setData( 27 );

  act = mp_menuChat->addAction( tr( "Show your name instead of 'You'" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().chatUseYourNameInsteadOfYou() );
  act->setData( 41 );

  mp_menuChat->addSeparator();

  act = mp_menuChat->addAction( tr( "Show emoticons" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().showEmoticons() );
  act->setData( 10 );

  act = mp_menuChat->addAction( tr( "Use font emoticons" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().useNativeEmoticons() );
  act->setData( 31 );

  mp_menuChat->addSeparator();

  act = mp_menuChat->addAction( tr( "Use HTML tags" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().chatUseHtmlTags() );
  act->setData( 8 );

  act = mp_menuChat->addAction( tr( "Use clickable links" ), this, SLOT( settingsChanged() ) );
  act->setCheckable( true );
  act->setChecked( Settings::instance().chatUseClickableLinks() );
  act->setData( 9 );

  mp_menuChat->addSeparator();

  act = mp_menuChat->addAction( tr( "Restore default font" ), this, SLOT( settingsChanged() ) );
  act->setIcon( QIcon( ":/images/font.png" ) );
  act->setData( 23 );

  mp_menuChat->exec( QCursor::pos() );
}

void GuiMain::showDefaultServerPortInMenu()
{
  QString host_address = tr( "offline" );
  QString broadcast_port = tr( "offline" );
  QString listener_port = tr( "offline" );
  QString file_transfer_port = tr( "offline" );
#ifdef BEEBEEP_USE_MULTICAST_DNS
  QString multicast_dns = tr( "inactive" );
#endif

  if( mp_core->isConnected() )
  {
    mp_menuNetworkStatus->setIcon( QIcon( ":/images/network-connected.png" ) );
    mp_actHostAddress->setIcon( QIcon( ":/images/connect.png" ) );
    mp_actPortBroadcast->setIcon( QIcon( ":/images/broadcast.png" ) );
    mp_actPortListener->setIcon( QIcon( ":/images/default-chat-online.png" ) );

    host_address = Settings::instance().localUser().networkAddress().hostAddress().toString();
    broadcast_port = QString::number( Settings::instance().defaultBroadcastPort() );
    listener_port = QString::number( Settings::instance().localUser().networkAddress().hostPort() );
    if( Settings::instance().enableFileTransfer() )
    {
      file_transfer_port = QString::number( mp_core->fileTransferPort() );
      mp_actPortFileTransfer->setIcon( QIcon( ":/images/network-scan.png" ) );
    }
    else
    {
      file_transfer_port = tr( "disabled" );
      mp_actPortFileTransfer->setIcon( QIcon() );
    }

#ifdef BEEBEEP_USE_MULTICAST_DNS
    if( mp_core->dnsMulticastingIsActive() )
    {
      multicast_dns = tr( "active" );
      mp_actMulticastDns->setIcon( QIcon( ":/images/mdns.png" ) );
    }
#endif
  }
  else
  {
    mp_menuNetworkStatus->setIcon( QIcon( ":/images/network-disconnected.png" ) );
    mp_actHostAddress->setIcon( QIcon() );
    mp_actPortBroadcast->setIcon( QIcon() );
    mp_actPortListener->setIcon( QIcon() );
    mp_actPortFileTransfer->setIcon( QIcon() );
#ifdef BEEBEEP_USE_MULTICAST_DNS
    mp_actMulticastDns->setIcon( QIcon() );
#endif
  }

  mp_actHostAddress->setText( QString( "ip: %1" ).arg( host_address ) );
  mp_actPortBroadcast->setText( QString( "udp: %1" ).arg( broadcast_port ) );
  mp_actPortListener->setText( QString( "tcp1: %1" ).arg( listener_port ) );
  mp_actPortFileTransfer->setText( QString( "tcp2: %1" ).arg( file_transfer_port ) );
#ifdef BEEBEEP_USE_MULTICAST_DNS
  mp_actMulticastDns->setText( QString( "mdns: %1" ).arg( multicast_dns ) );
#endif
}

void GuiMain::sendBroadcastMessage()
{
  mp_actBroadcast->setDisabled( true );
  mp_core->sendBroadcastMessage();
  mp_core->sendMulticastingMessage();
  QTimer::singleShot( 3 * 61 * 1000, this, SLOT( enableBroadcastAction() ) );
}

void GuiMain::enableBroadcastAction()
{
  mp_actBroadcast->setEnabled( true );
}

void GuiMain::checkUserSelected( VNumber user_id )
{
  User u = UserManager::instance().findUser( user_id );
  if( !u.isValid() )
  {
    qWarning() << "Invalid user id" << user_id << "found in check user selected";
    return;
  }

  Chat c = ChatManager::instance().privateChatForUser( user_id );
  if( !c.isValid() )
  {
    mp_core->createPrivateChat( u );
    c = ChatManager::instance().privateChatForUser( user_id );
    if( !c.isValid() )
    {
      qWarning() << "Unable to create private chat for user" << user_id;
      return;
    }
  }

  showChat( c.id() );
}

void GuiMain::showConnectionStatusChanged( const User& u )
{
  if( !mp_core->isConnected() )
    return;

  if( Settings::instance().showOnlyMessageNotificationOnTray() )
    return;

  QString msg;
  if( u.isStatusConnected() )
    msg = tr( "%1 is online" ).arg( u.name() );
  else
    msg = tr( "%1 is offline" ).arg( u.name() );

  Chat c = ChatManager::instance().privateChatForUser( u.id() );
  if( c.isValid() && u.isStatusConnected() )
    mp_trayIcon->showUserStatusChanged( c.id(), msg );
  else
    mp_trayIcon->showUserStatusChanged( ID_DEFAULT_CHAT, msg );

  if( mp_fileSharing )
    mp_fileSharing->onUserChanged( u );
}

void GuiMain::changeAvatarSizeInList()
{
  bool ok = false;
  int avatar_size = QInputDialog::getInt( this, Settings::instance().programName(), tr( "Please select the new size of the user picture" ),
                                          Settings::instance().avatarIconSize().height(), 16, 96, 4, &ok );
  if( !ok )
    return;

  Settings::instance().setAvatarIconSize( QSize( avatar_size, avatar_size ) );
  refreshUserList();
  mp_chatList->reloadChatList();
}

void GuiMain::toggleUserFavorite( VNumber user_id )
{
  mp_core->toggleUserFavorite( user_id );
  refreshUserList();
}

void GuiMain::createGroupFromChat( VNumber chat_id )
{
  mp_core->createGroupFromChat( chat_id );
}

void GuiMain::removeUserFromList( VNumber user_id )
{
  if( mp_core->removeOfflineUser( user_id ) )
  {
    Chat c = ChatManager::instance().privateChatForUser( user_id );
    if( c.isValid() )
      closeFloatingChat( c.id() );
    refreshUserList();
    mp_chatList->reloadChatList();
    updateTabTitles();
  }
}

void GuiMain::openResourceFolder()
{
  QUrl data_folder_url = QUrl::fromLocalFile( Settings::instance().resourceFolder() );
  openUrl( data_folder_url );
}

void GuiMain::openDataFolder()
{
  QUrl data_folder_url = QUrl::fromLocalFile( Settings::instance().dataFolder() );
  openUrl( data_folder_url );
}

GuiFloatingChat* GuiMain::floatingChat( VNumber chat_id ) const
{
  foreach( GuiFloatingChat* fl_chat, m_floatingChats )
  {
    if( fl_chat->guiChat()->chatId() == chat_id )
      return fl_chat;
  }
  return 0;
}

void GuiMain::closeFloatingChat( VNumber chat_id )
{
  GuiFloatingChat* fl_chat = floatingChat( chat_id );
  if( !fl_chat )
  {
#ifdef BEEBEEP_DEBUG
    qWarning() << "Unable to find floating chat" << chat_id << "in GuiMain::closeFloatingChat(...)";
#endif
    return;
  }
  else
    fl_chat->close();
}

void GuiMain::removeFloatingChatFromList( VNumber chat_id )
{
  GuiFloatingChat* fl_chat = floatingChat( chat_id );
  if( !fl_chat )
    return;

  if( mp_menuChat->isVisible() )
    mp_menuChat->close();

  m_floatingChats.removeOne( fl_chat );
  fl_chat->deleteLater();
#ifdef BEEBEEP_DEBUG
  qDebug() << "Floating chat" << chat_id << "closed and deleted";
#endif
}

GuiFloatingChat* GuiMain::createFloatingChat( const Chat& c )
{
  GuiFloatingChat* fl_chat = floatingChat( c.id() );
  if( fl_chat )
    return fl_chat;

  if( Settings::instance().showChatsInOneWindow() && !m_floatingChats.isEmpty() )
    fl_chat = m_floatingChats.first();

  bool window_is_created = true;
  if( !fl_chat )
  {
    fl_chat = new GuiFloatingChat( mp_core );
    setupChatConnections( fl_chat->guiChat() );
    connect( fl_chat, SIGNAL( chatIsAboutToClose( VNumber ) ), this, SLOT( removeFloatingChatFromList( VNumber ) ) );
    connect( fl_chat, SIGNAL( readAllMessages( VNumber ) ), this, SLOT( readAllMessagesInChat( VNumber ) ) );
    connect( fl_chat, SIGNAL( showVCardRequest( VNumber ) ), this, SLOT( showVCard( VNumber ) ) );
    m_floatingChats.append( fl_chat );
  }
  else
    window_is_created = false;

  if( !fl_chat->setChat( c ) )
  {
    qWarning() << "Unable to create floating window for not existing chat" << c.id() << c.name();
    fl_chat->deleteLater();
    return 0;
  }

  if( window_is_created )
  {
    fl_chat->checkWindowFlagsAndShow();
    QApplication::setActiveWindow( fl_chat );
  }
  else
    fl_chat->showUp();

  return fl_chat;
}

QWidget* GuiMain::activeWindow() const
{
  QWidget* active_window = QApplication::activeWindow();
  if( active_window )
    return active_window;
  else
    return (QWidget*)this;
}

void GuiMain::loadUserStatusRecentlyUsed()
{
  if( !mp_menuUserStatusList->isEmpty() )
    mp_menuUserStatusList->clear();

  QList<UserStatusRecord> user_status_list;
  foreach( QString s, Settings::instance().userStatusList() )
  {
    UserStatusRecord usr = Protocol::instance().loadUserStatusRecord( s );
    if( usr.isValid() )
      user_status_list.append( usr );
  }

  if( user_status_list.size() < Settings::instance().maxUserStatusDescriptionInList() )
  {
    UserStatusRecord usr1;
    usr1.setStatus( User::Away );
    usr1.setStatusDescription( tr( "at lunch" ) );
    if( !user_status_list.contains( usr1 ) )
      user_status_list.append( usr1 );
  }

  if( user_status_list.size() < Settings::instance().maxUserStatusDescriptionInList() )
  {
    UserStatusRecord usr2;
    usr2.setStatus( User::Busy );
    usr2.setStatusDescription( tr( "in a meeting" ) );
    if( !user_status_list.contains( usr2 ) )
      user_status_list.append( usr2 );
  }

  qSort( user_status_list );

  QAction* act;
  foreach( UserStatusRecord usr, user_status_list )
  {
    if( usr.isValid() )
    {
      act = mp_menuUserStatusList->addAction( Bee::userStatusIcon( usr.status() ), usr.statusDescription(), this, SLOT( recentlyUsedUserStatusSelected() ) );
      act->setData( Protocol::instance().saveUserStatusRecord( usr ) );
    }
  }
}

void GuiMain::recentlyUsedUserStatusSelected()
{
  QAction* act = qobject_cast<QAction*>( sender() );
  if( !act )
    return;

  UserStatusRecord usr = Protocol::instance().loadUserStatusRecord( act->data().toString() );
  if( usr.isValid() )
  {
    mp_core->setLocalUserStatusDescription( usr.status(), usr.statusDescription(), false );
    setUserStatusSelected( usr.status() );
    loadUserStatusRecentlyUsed();
  }
}

void GuiMain::clearRecentlyUsedUserStatus()
{
  if( QMessageBox::question( this, Settings::instance().programName(),
                             tr( "Do you really want to clear all saved status descriptions?" ),
                             tr( "Yes" ), tr( "No" ), QString::null, 1, 1 ) != 0 )
    return;

  qDebug() << "User status description list is cleared";
  QStringList sl = Settings::instance().userStatusList();
  sl.clear();
  Settings::instance().setUserStatusList( sl );
  loadUserStatusRecentlyUsed();
}

void GuiMain::loadSavedChatsCompleted()
{
  mp_chatList->reloadChatList();
  mp_savedChatList->updateSavedChats();
  foreach( GuiFloatingChat* fl_chat, m_floatingChats )
    fl_chat->setChat( ChatManager::instance().chat( fl_chat->guiChat()->chatId() ) );
  updateTabTitles();
}

void GuiMain::editShortcuts()
{
  GuiShortcut gs;
  gs.setModal( true );
  gs.loadShortcuts();
  gs.setSizeGripEnabled( true );
  gs.show();

  if( gs.exec() == QDialog::Rejected )
    return;

  Settings::instance().setShortcuts( ShortcutManager::instance().saveToStringList() );
  Settings::instance().save();
  updateShortcuts();
}

void GuiMain::updateShortcuts()
{
  foreach( GuiFloatingChat* fl_chat, m_floatingChats )
    fl_chat->guiChat()->updateShortcuts();

  QKeySequence ks = ShortcutManager::instance().shortcut( ShortcutManager::ShowFileTransfers );
  if( !ks.isEmpty() && Settings::instance().useShortcuts() )
    mp_actViewFileTransfer->setShortcut( ks );
  else
    mp_actViewFileTransfer->setShortcut( QKeySequence() );

  ks = ShortcutManager::instance().shortcut( ShortcutManager::MinimizeAllChats );
  if( !ks.isEmpty() )
  {
    mp_scMinimizeAllChats->setKey( ks );
    mp_scMinimizeAllChats->setEnabled( Settings::instance().useShortcuts() );
  }
  else
    mp_scMinimizeAllChats->setEnabled( false );

#ifdef BEEBEEP_USE_QXT
  ks = ShortcutManager::instance().shortcut( ShortcutManager::ShowAllChats );
  if( !ks.isEmpty() )
  {
    mp_scShowAllChats->setShortcut( ks );
    mp_scShowAllChats->setEnabled( Settings::instance().useShortcuts() );
  }
  else
    mp_scShowAllChats->setEnabled( false );
#endif
  ks = ShortcutManager::instance().shortcut( ShortcutManager::ShowNextUnreadMessage );
  if( !ks.isEmpty() )
  {
    mp_scShowNextUnreadMessage->setKey( ks );
    mp_scShowNextUnreadMessage->setEnabled( Settings::instance().useShortcuts() );
  }
  else
    mp_scShowNextUnreadMessage->setEnabled( false );

  ks = ShortcutManager::instance().shortcut( ShortcutManager::Broadcast );
  if( !ks.isEmpty() && Settings::instance().useShortcuts() )
    mp_actBroadcast->setShortcut( ks );
  else
    mp_actBroadcast->setShortcut( QKeySequence() );

}

void GuiMain::minimizeAllChats()
{
  QWidget* w = qApp->activeWindow();

  bool last_active_window_exists = w == this ? true : false;

  if( !m_floatingChats.isEmpty() )
  {
    foreach( GuiFloatingChat* fl_chat, m_floatingChats )
    {
      if( !fl_chat->isMinimized() )
        fl_chat->showMinimized();
      if( fl_chat == mp_lastActiveWindow )
        last_active_window_exists = true;
    }
  }

  if( !isMinimized() )
    showMinimized();

  if( last_active_window_exists )
    mp_lastActiveWindow = w;
}

void GuiMain::showAllChats()
{
  bool last_active_window_exists = false;

  if( !m_floatingChats.isEmpty() )
  {
    foreach( GuiFloatingChat* fl_chat, m_floatingChats )
    {
      if( fl_chat->isMinimized() )
        fl_chat->showNormal();
      else
        show();

      if( fl_chat == mp_lastActiveWindow )
        last_active_window_exists = true;
    }
  }

  if( isMinimized() )
    showNormal();
  else
    show();

  if( this == mp_lastActiveWindow )
    last_active_window_exists = true;

  if( last_active_window_exists )
  {
    mp_lastActiveWindow->raise();
    qApp->setActiveWindow( mp_lastActiveWindow );
  }
}

void GuiMain::selectDictionatyPath()
{
  QString dictionary_path = FileDialog::getOpenFileName( false, this, tr( "Select your dictionary path" ), Settings::instance().dictionaryPath(), QString( "*.dic" ) );

  if( dictionary_path.isEmpty() )
    return;

  Settings::instance().setDictionaryPath( dictionary_path );
#ifdef BEEBEEP_USE_HUNSPELL
  if( SpellChecker::instance().setDictionary( dictionary_path ) )
    QMessageBox::information( this, Settings::instance().programName(), tr( "Dictionary selected: %1" ).arg( dictionary_path ) );
  else
    QMessageBox::warning( this, Settings::instance().programName(), tr( "Unable to set dictionary: %1" ).arg( dictionary_path ) );
#endif

  // update spellchecker and wordcompleter actions
  foreach( GuiFloatingChat* fl_chat, m_floatingChats )
    fl_chat->guiChat()->updateActionsOnFocusChanged();
}

void GuiMain::onNetworkInterfaceDown()
{
  if( mp_core->isConnected() )
  {
    //raiseHomeView();
    m_autoConnectOnInterfaceUp = true;
    QTimer::singleShot( 1000, this, SLOT( stopCore() ) );
  }
}

void GuiMain::onNetworkInterfaceUp()
{
  if( m_autoConnectOnInterfaceUp && !mp_core->isConnected() )
    QTimer::singleShot( 5000, this, SLOT( startCore() ) );
}

static bool IsTimeToCheck( int ticks, int tick_for_check ) { return ticks % tick_for_check == 0; }

void GuiMain::onTickEvent( int ticks )
{
  if( IsTimeToCheck( ticks, Settings::instance().tickIntervalCheckNetwork() ) )
    QMetaObject::invokeMethod( mp_core, "checkNetworkInterface", Qt::QueuedConnection );

  if( IsTimeToCheck( ticks, Settings::instance().tickIntervalCheckIdle() ) )
  {
    BeeApplication* bee_app = (BeeApplication*)qApp;
    if( bee_app->idleTimeout() > 0 )
      QMetaObject::invokeMethod( bee_app, "checkIdle", Qt::QueuedConnection );
  }

  if( mp_actViewNewMessage->isEnabled() )
  {
    QIcon new_message_blinking_icon( ":/images/beebeep-message.png" );
    mp_actViewNewMessage->setIcon( ticks % 2 == 0 ? new_message_blinking_icon : Bee::convertToGrayScale( new_message_blinking_icon.pixmap( Settings::instance().mainBarIconSize() ) ) );
  }

  mp_trayIcon->onTickEvent( ticks );
  mp_chatList->onTickEvent( ticks );
  mp_userList->onTickEvent( ticks );
  mp_core->onTickEvent( ticks );
  mp_fileSharing->onTickEvent( ticks );

  if( mp_core->hasFileTransferInProgress() )
    mp_actViewFileTransfer->setIcon( ticks % 2 == 0 ? QIcon( ":/images/file-transfer-progress.png" ) : QIcon( ":/images/file-transfer.png" ) );

  if( mp_actViewNewMessage->isEnabled() )
    QApplication::alert( this, 1000 );
}

void GuiMain::onChatReadByUser( const Chat& c, const User& u )
{
  GuiFloatingChat* fl_chat = floatingChat( c.id() );
  if( fl_chat )
    fl_chat->setChatReadByUser( c, u );
}

void GuiMain::readAllMessagesInChat( VNumber chat_id )
{
  if( mp_core->readAllMessagesInChat( chat_id ) )
  {
    Chat c = ChatManager::instance().chat( chat_id );
    mp_userList->setUnreadMessages( c.id(), 0 );
    mp_chatList->updateChat( c );
    mp_groupList->updateChat( c );
  }

  GuiFloatingChat *fl_chat = floatingChat( chat_id );
  if( fl_chat )
    fl_chat->setMainIcon( false );

  Chat c = ChatManager::instance().firstChatWithUnreadMessages();
  if( c.isValid() )
  {
    mp_trayIcon->setUnreadMessages( c.id(), c.unreadMessages() );
  }
  else
  {
    mp_trayIcon->resetChatId();
    mp_trayIcon->setDefaultIcon();
  }

  updateNewMessageAction();
}

void GuiMain::saveSession( QSessionManager& )
{
  qDebug() << "Session manager ask to save and close session";
  forceShutdown();
}

void GuiMain::updateEmoticons()
{
  if( m_floatingChats.isEmpty() )
    return;

  foreach( GuiFloatingChat* fl_chat, m_floatingChats )
    fl_chat->updateEmoticon();
}

void GuiMain::updateNewMessageAction()
{
  mp_actViewNewMessage->setEnabled( ChatManager::instance().hasUnreadMessages() );
}

void GuiMain::saveGeometryAndState()
{
  if( isVisible() )
  {
    Settings::instance().setGuiGeometry( saveGeometry() );
    Settings::instance().setGuiState( saveState() );
    Settings::instance().save();
    showMessage( tr( "Window geometry and state saved" ), 3000 );
  }
}

void GuiMain::onChangeSettingOnExistingFile( QAction* act )
{
  if( !act )
    return;

  Settings::instance().setOverwriteExistingFiles( mp_actOverwriteExistingFile->isChecked() );
  Settings::instance().setAutomaticFileName( mp_actGenerateAutomaticFilename->isChecked() );
  Settings::instance().save();
}

void GuiMain::onShareBoxRequest( VNumber user_id, const QString& share_box_path )
{
  mp_core->sendShareBoxRequest( user_id, share_box_path );
}

void GuiMain::onShareBoxDownloadRequest( VNumber user_id, const FileInfo& fi, const QString& to_path )
{
  mp_core->downloadFromShareBox( user_id, fi, to_path );
}

void GuiMain::onShareBoxUploadRequest( VNumber user_id, const FileInfo& fi, const QString& to_path )
{
  mp_core->uploadToShareBox( user_id, fi, to_path );
}

void GuiMain::onFileTransferProgress( VNumber peer_id, const User& u, const FileInfo& fi, FileSizeType bytes )
{
  mp_fileTransfer->setProgress( peer_id, u, fi, bytes );
}

void GuiMain::onFileTransferMessage( VNumber peer_id, const User& u, const FileInfo& fi, const QString& msg )
{
  mp_fileTransfer->setMessage( peer_id, u, fi, msg );
}

void GuiMain::onFileTransferCompleted( VNumber peer_id, const User& u, const FileInfo& fi )
{
  Q_UNUSED( peer_id );
  if( fi.isDownload() && Settings::instance().showFileTransferCompletedOnTray() )
  {
    VNumber chat_id = ID_DEFAULT_CHAT;
    Chat c = ChatManager::instance().privateChatForUser( u.id() );
    if( c.isValid() )
      chat_id = c.id();
    mp_trayIcon->showNewFileArrived( chat_id, tr( "New file from %1" ).arg( u.name() ), false );
  }
}

void GuiMain::sendBuzzToUser( VNumber user_id )
{
  if( mp_core->isConnected() )
    mp_core->sendBuzzToUser( user_id );
}

void GuiMain::showBuzzFromUser( const User& u )
{
  if( Settings::instance().playBuzzSound() )
    playBeep();

  Chat c = ChatManager::instance().privateChatForUser( u.id() );
  if( c.isValid() )
    mp_trayIcon->showNewMessageArrived( c.id(), tr( "%1 is buzzing you!" ).arg( u.name() ), true );
}

void GuiMain::showFileSharingWindow()
{
  if( Settings::instance().disableFileSharing() )
    return;

  if( !mp_fileSharing )
  {
    mp_fileSharing = new GuiFileSharing( mp_core, 0 );
    mp_fileSharing->setAttribute( Qt::WA_DeleteOnClose, true );
    Bee::setWindowStaysOnTop( mp_fileSharing, Settings::instance().stayOnTop() );
    mp_fileSharing->resize( qMin( (QApplication::desktop()->availableGeometry().width()-20), 760 ), 460 );
    mp_fileSharing->updateLocalFileList();
    connect( mp_fileSharing, SIGNAL( destroyed() ), this, SLOT( onFileSharingWindowClosed() ) );
    connect( mp_fileSharing, SIGNAL( openUrlRequest( const QUrl& ) ), this, SLOT( openUrl( const QUrl& ) ) );
    connect( mp_fileSharing, SIGNAL( sendFileRequest( const QString& ) ), this, SLOT( sendFile( const QString& ) ) );
    connect( mp_fileSharing, SIGNAL( downloadSharedFileRequest( VNumber, VNumber ) ), this, SLOT( downloadSharedFile( VNumber, VNumber ) ) );
    connect( mp_fileSharing, SIGNAL( downloadSharedFilesRequest( const QList<SharedFileInfo>& ) ), this, SLOT( downloadSharedFiles( QList<SharedFileInfo> ) ) );
  }

  mp_fileSharing->showUp();
}

void GuiMain::onFileSharingWindowClosed()
{
  if( mp_fileSharing )
    mp_fileSharing = 0;
}

void GuiMain::showScreenShotWindow()
{
  if( !mp_screenShot )
  {
    mp_screenShot = new GuiScreenShot;
    mp_screenShot->setAttribute( Qt::WA_DeleteOnClose, true );
    Bee::setWindowStaysOnTop( mp_screenShot, Settings::instance().stayOnTop() );
    mp_screenShot->resize( 620,  460 );
    connect( mp_screenShot, SIGNAL( screenShotToSend( const QString& ) ), this, SLOT( sendFile( const QString& ) ) );
    connect( mp_screenShot, SIGNAL( destroyed() ), this, SLOT( onScreenShotWindowClosed() ) );
  }

  mp_screenShot->showUp();
}

void GuiMain::onScreenShotWindowClosed()
{
  if( mp_screenShot )
    mp_screenShot = 0;
}

void GuiMain::showLogWindow()
{
  if( !mp_log )
  {
    mp_log = new GuiLog;
    Bee::setWindowStaysOnTop( mp_log, Settings::instance().stayOnTop() );
    mp_log->resize( qMin( (QApplication::desktop()->availableGeometry().width()-20), 760 ), 460 );
    connect( mp_log, SIGNAL( destroyed() ), this, SLOT( onLogWindowClosed() ) );
  }

  mp_log->showUp();
}

void GuiMain::onLogWindowClosed()
{
  if( mp_log )
    mp_log = 0;
}

void GuiMain::onMainTabChanged( int tab_index )
{
  if( mp_tabMain->widget( tab_index ) == mp_home )
  {
    m_unreadActivities = 0;
    updateTabTitles();
  }
}

void GuiMain::setFileTransferEnabled( bool enable )
{
  if( Settings::instance().disableFileTransfer() )
    return;

 Settings::instance().setEnableFileTransfer( enable );
 if( !enable )
 {
   Settings::instance().setEnableFileSharing( false );
   Settings::instance().setUseShareBox( false );
   mp_core->stopFileTransferServer();
   QMetaObject::invokeMethod( mp_core, "buildLocalShareList", Qt::QueuedConnection );
 }
 else
   mp_core->startFileTransferServer();

 checkViewActions();
}

void GuiMain::setFileSharingEnabled( bool enable )
{
  if( Settings::instance().disableFileTransfer() || Settings::instance().disableFileSharing() )
    return;

  Settings::instance().setEnableFileSharing( enable );
  QMetaObject::invokeMethod( mp_core, "buildLocalShareList", Qt::QueuedConnection );
  checkViewActions();
}

void GuiMain::showWorkgroups()
{
  GuiWorkgroups gw;
  gw.loadWorkgroups();
  gw.setModal( true );
  gw.setSizeGripEnabled( true );
  gw.show();
  if( gw.exec() != QDialog::Accepted )
    return;

  if( Settings::instance().acceptConnectionsOnlyFromWorkgroups() && !Settings::instance().workgroups().isEmpty() )
    qDebug() << "Protocol now accepts connections only from these workgroups:" << qPrintable( Settings::instance().workgroups().join( ", " ) );
}

#ifdef BEEBEEP_USE_SHAREDESKTOP
void GuiMain::onShareDesktopImageAvailable( const User& u, const QPixmap& pix )
{

  foreach( GuiShareDesktop* gsd, m_desktops )
  {
    if( gsd->ownerId() == u.id() )
    {
      gsd->updatePixmap( pix );
      return;
    }
  }

  GuiShareDesktop* new_gui = new GuiShareDesktop;
  connect( new_gui, SIGNAL( shareDesktopClosed( VNumber ) ), this, SLOT( onShareDesktopCloseEvent( VNumber ) ) );
  new_gui->setOwner( u );
  new_gui->setGeometry( 30, 30, 800, 600 );
  new_gui->show();
  new_gui->updatePixmap( pix );

  m_desktops.append( new_gui );
}

void GuiMain::onShareDesktopCloseEvent( VNumber user_id )
{
  QList<GuiShareDesktop*>::iterator it = m_desktops.begin();
  while( it != m_desktops.end() )
  {
    if( (*it)->ownerId() == user_id )
    {
      (*it)->disconnect();
      (*it)->deleteLater();
      m_desktops.erase( it );
      return;
    }
  }
}
#endif
