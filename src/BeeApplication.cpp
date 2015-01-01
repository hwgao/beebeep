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
#include <QDebug>
#include <QEvent>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef Q_OS_UNIX
#include <xcb/xcb.h>
#include <xcb/screensaver.h>
// package libxcb-screensaver0-dev
// package libx11-xcb-dev
#endif


BeeApplication::BeeApplication( int& argc, char** argv  )
  : QApplication( argc, argv )
{
  setObjectName( "BeeApplication" );
  m_idleTimeout = 0;
  m_timer.setObjectName( "BeeMainTimer" );
  m_timer.setInterval( 10000 );
  m_isInIdle = false;

#ifdef Q_OS_UNIX
  mp_xcbConnection = xcb_connect( 0, 0 );
  mp_xcbScreen = xcb_setup_roots_iterator( xcb_get_setup( mp_xcbConnection ) ).data;

  if( testAttribute( Qt::AA_DontShowIconsInMenus ) )
    setAttribute( Qt::AA_DontShowIconsInMenus, false );
#endif

  connect( &m_timer, SIGNAL( timeout() ), this, SLOT( checkIdle() ) );
}

void BeeApplication::setIdleTimeout( int new_value )
{
  m_idleTimeout = new_value * 60;
  if( m_timer.isActive() )
    return;
  m_timer.start();
}

void BeeApplication::setIdle()
{
  if( m_isInIdle )
    return;

  m_isInIdle = true;
  emit( enteringInIdle() );
}

void BeeApplication::removeIdle()
{
  if( !m_isInIdle )
    return;

  m_isInIdle = false;
  emit( exitingFromIdle() );
}

bool BeeApplication::notify( QObject* obj_receiver, QEvent* obj_event )
{
  if( obj_event->type() == QEvent::MouseMove || obj_event->type() == QEvent::KeyPress )
  {
    m_lastEventDateTime = QDateTime::currentDateTime();
    if( m_isInIdle )
      removeIdle();
  }

  return QApplication::notify( obj_receiver, obj_event );
}

void BeeApplication::checkIdle()
{
  if( isScreenSaverRunning() || idleTimeFromSystem() > m_idleTimeout )
    setIdle();
  else
    removeIdle();
}

void BeeApplication::cleanUp()
{
  if( m_timer.isActive() )
    m_timer.stop();

#ifdef Q_OS_UNIX
  // mp_xcbScreen not need to free
  free( mp_xcbConnection );
#endif
}

bool BeeApplication::isScreenSaverRunning()
{
  bool screen_saver_is_running = false;

#ifdef Q_OS_WIN
  BOOL is_running = FALSE;
  SystemParametersInfo( SPI_GETSCREENSAVERRUNNING, 0, &is_running, 0 );
  screen_saver_is_running = (bool)is_running;
#endif

#ifdef Q_OS_UNIX
  xcb_screensaver_query_info_cookie_t xcbCookie;
  xcb_screensaver_query_info_reply_t* xcbInfo;

  xcbCookie = xcb_screensaver_query_info( mp_xcbConnection, mp_xcbScreen->root );
  xcbInfo = xcb_screensaver_query_info_reply( mp_xcbConnection, xcbCookie, 0 );

  screen_saver_is_running = xcbInfo->state == XCB_SCREENSAVER_STATE_ON || xcbInfo->state == XCB_SCREENSAVER_STATE_CYCLE;
  free( xcbInfo );
#endif

  return screen_saver_is_running;
}

int BeeApplication::idleTimeFromSystem()
{
  int idle_time = -1;

#ifdef Q_OS_WIN
  LASTINPUTINFO idle_info;
  idle_info.cbSize = sizeof( LASTINPUTINFO );
  if( ::GetLastInputInfo( &idle_info ) )
  {
    idle_time = ::GetTickCount() - idle_info.dwTime;
    idle_time = qMax( 0, idle_time / 1000 );
  }
#endif

#ifdef Q_OS_UNIX
  xcb_screensaver_query_info_cookie_t xcbCookie;
  xcb_screensaver_query_info_reply_t* xcbInfo;

  xcbCookie = xcb_screensaver_query_info( mp_xcbConnection, mp_xcbScreen->root);
  xcbInfo = xcb_screensaver_query_info_reply( mp_xcbConnection, xcbCookie, 0 );

  idle_time = qMax( 0, (int)xcbInfo->ms_since_user_input / 1000 );
  free ( xcbInfo );

#endif

  if( idle_time < 0 )
    idle_time = qMax( 0, m_lastEventDateTime.secsTo( QDateTime::currentDateTime() ) );

  return idle_time;
}


