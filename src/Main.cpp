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

#include "BeeApplication.h"
#include "ChatManager.h"
#include "ColorManager.h"
#include "FileShare.h"
#include "GuiIconProvider.h"
#include "GuiMain.h"
#include "Log.h"
#include "PluginManager.h"
#include "UserManager.h"
#include "Protocol.h"
#include "Random.h"
#include "Settings.h"

bool SetTranslator( QTranslator* translator, QString language_folder, QString lang )
{
  QString language_file_path = Settings::instance().languageFilePath( language_folder, lang );
  if( !translator->load( language_file_path ) )
  {
    qDebug() << language_file_path << "file not found. Language" << lang << "will not be installed";
    return false;
  }
  else
    qDebug() << "Language file" << language_file_path << "found and" << lang << "installed";
  qApp->installTranslator( translator );
  return true;
}

int main( int argc, char *argv[] )
{
  BeeApplication bee_app( argc, argv );
  Q_INIT_RESOURCE( beebeep );

  /* Randomize */
  Random::init();

  /* Enable internal logs */
  Log::installMessageHandler();

    /* Load Settings */
  (void)Settings::instance();
  qDebug() << "Starting BeeBEEP" << Settings::instance().version( true );
  if( !Settings::instance().createDefaultFolder() )
    qDebug() << "Default folder cannot be created. Check" << Settings::instance().defaultFolder();
  Settings::instance().loadPreConf();
  Settings::instance().load();

  bee_app.setApplicationName( Settings::instance().programName() );
  bee_app.setOrganizationName( Settings::instance().organizationName() );
  bee_app.setApplicationVersion( Settings::instance().version( false ) );

  /* Starting File Logs */
  if( Settings::instance().logToFile() )
    Log::instance().bootFileStream();

  /* Apply system language */
  QTranslator translator;
  SetTranslator( &translator, Settings::instance().languagePath(), Settings::instance().language() );

  /* Init Protocol */
  (void)Protocol::instance();

  /* Init Color Manager */
  (void)ColorManager::instance();

  /* Init User Manager */
  (void)UserManager::instance();

  /* Init Chat Manager */
  (void)ChatManager::instance();

  /* Init File Sharing */
  (void)FileShare::instance();

  /* Init Icon Provider */
  (void)GuiIconProvider::instance();

  /* Init Plugins */
  PluginManager::instance().loadPlugins();

  /* Init BeeApp */
  bee_app.init();

  /* Init Main Window */
  GuiMain mw;
  QObject::connect( &bee_app, SIGNAL( enteringInIdle() ), &mw, SLOT( setInIdle() ) );
  QObject::connect( &bee_app, SIGNAL( exitingFromIdle() ), &mw, SLOT( exitFromIdle() ) );

  QByteArray ba = Settings::instance().guiGeometry();
  if( !ba.isEmpty() )
  {
    mw.restoreGeometry( Settings::instance().guiGeometry() );
    ba = Settings::instance().guiState();
    if( !ba.isEmpty() )
      mw.restoreState( Settings::instance().guiState() );
  }
  else
    mw.resize( QSize( 740, 420 ) );

  mw.checkWindowFlagsAndShow();

  /* Load saved session */
  mw.loadSession();

  if( Settings::instance().loadOnTrayAtStartup() && QSystemTrayIcon::isSystemTrayAvailable() )
    QTimer::singleShot( 100, &mw, SLOT( hideToTrayIcon() ) );

  /* Starting connection to BeeBEEP Network */
  QTimer::singleShot( 500, &mw, SLOT( startStopCore() ) );

  if( Settings::instance().autoUserAway() )
    bee_app.setIdleTimeout( Settings::instance().userAwayTimeout() );

  /* Event Loop */
  int iRet = bee_app.exec();

  /* Check Icon Provider */
  qDebug() << "IconProvider has load in cache" << GuiIconProvider::instance().cacheSize() << "icons";

  /* Save session */
  mw.saveSession();

  /* CleanUp */
  bee_app.cleanUp();
  Settings::instance().clearTemporaryFile();
  GuiIconProvider::close();
  FileShare::close();
  ChatManager::close();
  UserManager::close();
  ColorManager::close();
  Protocol::close();
  PluginManager::close();
  Settings::instance().loadPreConf();
  Settings::instance().save();
  Log::instance().closeFileStream();
  Log::instance().close();
  Settings::close();

  /* Exit */
  return iRet;
}
