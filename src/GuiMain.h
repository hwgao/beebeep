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

#ifndef BEEBEEP_GUIMAIN_H
#define BEEBEEP_GUIMAIN_H


#include "Config.h"
class Core;
class Chat;
class ChatMessage;
class FileInfo;
class GuiChat;
class GuiChatList;
class GuiShareLocal;
class GuiShareNetwork;
class GuiTransferFile;
class GuiUserList;
class User;


class GuiMain : public QMainWindow
{
  Q_OBJECT

public:
  GuiMain( QWidget* parent = 0 );

public slots:
  void startStopCore();

private slots:
  void showAbout();
  void showLicense();
  void checkUser( const User& );
  void showWritingUser( const User& );
  void showChatMessage( VNumber, const ChatMessage& );
  void sendMessage( VNumber, const QString& );
  void showTipOfTheDay();
  void selectFontColor();
  void selectFont();
  void searchUsers();
  void settingsChanged();
  void emoticonSelected();
  void saveChat();
  void showNextChat();
  void statusSelected();
  void changeStatusDescription();
  void sendFile();
  void downloadFile( const User&, const FileInfo& );
  void downloadSharedFile( VNumber, VNumber );
  void selectDownloadDirectory();
  void changeVCard();
  void showUserMenu( VNumber );
  void changeUserColor( VNumber );
  void sendFile( VNumber );
  void showPluginHelp();
  void showPluginManager();
  void showNetworkManager();
  void showNetworkAccount();
  void showUserSubscriptionRequest( const QString&, const QString& );
  void removeUser( VNumber );
  void showChat( VNumber );
  void serviceConnected( const QString& );
  void serviceDisconnected( const QString& );
  void sendBroadcastMessage();
  void showWizard();
  void hideToTrayIcon();
  void showFromTrayIcon();
  void forceExit();
  void trayIconClicked( QSystemTrayIcon::ActivationReason );
  void raiseChatView();
  void raiseLocalShareView();
  void raiseNetworkShareView();
  void addToShare( const QString& );
  void removeFromShare( const QString& );

protected:
  void closeEvent( QCloseEvent* );
  void changeEvent( QEvent* );

private:
  void createActions();
  void createMenus();
  void createToolAndMenuBars();
  void createStatusBar();
  void createDockWindows();
  void createStackedWidgets();
  void refreshUserList();
  void refreshChat();
  void refreshTitle();
  void startCore();
  void stopCore();
  void initGuiItems();
  void updadePluginMenu();
  void updateStatusIcon();
  void updateAccountMenu();
  void sendFile( const User& );
  bool askToDownloadFile( const User&, const FileInfo& );

private:
  QStackedWidget* mp_stackedWidget;
  GuiChat* mp_defaultChat;
  GuiTransferFile* mp_fileTransfer;
  GuiUserList* mp_userList;
  GuiChatList* mp_chatList;
  GuiShareLocal* mp_shareLocal;
  GuiShareNetwork* mp_shareNetwork;
  Core *mp_core;

  QMenu *mp_menuMain;
  QMenu *mp_menuInfo;
  QMenu *mp_menuSettings;
  QMenu *mp_menuEmoticons;
  QMenu *mp_menuStatus;
  QMenu *mp_menuPlugins;
  QMenu *mp_menuAccounts;
  QMenu *mp_menuView;
  QMenu *mp_menuTray;

  QToolBar *mp_barMain;

  QAction* mp_actStartStopCore;
  QAction* mp_actSaveChat;
  QAction* mp_actSearch;
  QAction* mp_actQuit;
  QAction* mp_actVCard;
  QAction* mp_actFont;
  QAction* mp_actFontColor;
  QAction* mp_actToolBar;
  QAction* mp_actAbout;
  QAction* mp_actViewUsers;
  QAction* mp_actSendFile;
  QAction* mp_actViewFileTransfer;
  QAction* mp_actViewChats;
  QAction* mp_actViewShareLocal;
  QAction* mp_actViewShareNetwork;
  QAction* mp_actViewDefaultChat;

  QDockWidget* mp_dockUserList;

  QSystemTrayIcon* mp_trayIcon;

};

#endif // BEEBEEP_GUIMAIN_H
