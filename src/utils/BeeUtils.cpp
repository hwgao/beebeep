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
#include "ChatMessage.h"
#include "IconManager.h"
#include "PluginManager.h"
#include "User.h"
#if QT_VERSION < 0x050000
  #ifdef Q_OS_WIN
    #include <Windows.h>
  #endif
#endif
#ifndef Q_OS_WIN
  #include <utime.h>
  #include <errno.h>
#endif


QString Bee::userStatusIconFileName( int user_status )
{
  switch( user_status )
  {
  case User::Offline:
    return IconManager::instance().iconPath( "user-offline.png" );
  case User::Away:
    return IconManager::instance().iconPath( "user-away.png" );
  case User::Busy:
    return IconManager::instance().iconPath( "user-busy.png" );
  default:
    return IconManager::instance().iconPath( "user-online.png" );
  }
}

QString Bee::menuUserStatusIconFileName( int user_status )
{
  switch( user_status )
  {
  case User::Offline:
    return IconManager::instance().iconPath( "menu-user-offline.png" );
  case User::Away:
    return IconManager::instance().iconPath( "menu-user-away.png" );
  case User::Busy:
    return IconManager::instance().iconPath( "menu-user-busy.png" );
  default:
    return IconManager::instance().iconPath( "menu-user-online.png" );
  }
}

static const char* UserStatusToString[] =
{
  QT_TRANSLATE_NOOP( "User", "offline" ),
  QT_TRANSLATE_NOOP( "User", "available" ),
  QT_TRANSLATE_NOOP( "User", "busy" ),
  QT_TRANSLATE_NOOP( "User", "away" ),
  QT_TRANSLATE_NOOP( "User", "status error" ),
};

QString Bee::userStatusToString( int user_status )
{
  if( user_status < 0 || user_status > User::NumStatus )
    user_status = User::NumStatus;
  return qApp->translate( "User", UserStatusToString[ user_status ] );
}

QColor Bee::userStatusColor( int user_status )
{
  switch( user_status )
  {
  case User::Online:
    return QColor( Qt::green );
  case User::Away:
    return QColor( Qt::yellow );
  case User::Busy:
    return QColor( Qt::red );
  case User::Offline:
    return QColor( Qt::gray );
  default:
    return QColor( Qt::black );
  }
}

QColor Bee::userStatusBackgroundColor( int user_status )
{
  switch( user_status )
  {
  case User::Online:
    return defaultBackgroundBrush().color();
  case User::Away:
    return QColor( Qt::darkYellow );
  case User::Busy:
    return QColor( Qt::darkRed );
  case User::Offline:
    return QColor( Qt::gray );
  default:
    return defaultBackgroundBrush().color();
  }
}

QColor Bee::userStatusForegroundColor( int user_status )
{
  switch( user_status )
  {
  case User::Online:
    return defaultBackgroundBrush().color();
  case User::Away:
    return QColor( Qt::white );
  case User::Busy:
    return QColor( Qt::white );
  case User::Offline:
    return QColor( Qt::black );
  default:
    return defaultBackgroundBrush().color();
  }
}

QString Bee::bytesToString( FileSizeType bytes, int precision )
{
  QString suffix;
  double result = 0;
  int prec = 1;
  if( bytes > 1000000000 )
  {
    suffix = "Gb";
    result = bytes / 1000000000.0;
    prec = 3;
  }
  else if( bytes > 1000000 )
  {
    suffix = "Mb";
    result = bytes / 1000000.0;
    prec = 2;
  }
  else if( bytes > 1000 )
  {
    suffix = "kb";
    result = bytes / 1000.0;
    prec = result >= 10 ? 0 : 1;
  }
  else
  {
    suffix = "b";
    result = bytes;
    prec = 0;
  }
  return QString( "%1 %2").arg( result, 0, 'f', prec > 0 ? (precision >= 0 ? precision : prec) : 0 ).arg( suffix );
}

QString Bee::elapsedTimeToString( int time_elapsed )
{
  QTime t( 0, 0 );
  t = t.addMSecs( time_elapsed );
  QString s = "";
  if( t.hour() == 0 && t.minute() == 0 && t.second() == 0 )
    s = QString( "%1 ms" ).arg( t.msec() );
  else if( t.hour() == 0 && t.minute() == 0 )
    s = QString( "%1 s" ).arg( t.second() );
  else if( t.hour() == 0 )
    s = QString( "%1 m, %2 s" ).arg( t.minute() ).arg( t.second() );
  else
    s = QString( "%1 h, %2 m, %3 s" ).arg( t.hour() ).arg( t.minute() ).arg( t.second() );
  return s;
}

QString Bee::uniqueFilePath( const QString& file_path, bool add_date_time )
{
  int counter = 1;
  QFileInfo fi( file_path );
  QString dir_path = fi.absoluteDir().absolutePath();
  QString file_base_name = fi.completeBaseName();
  QString file_suffix = fi.suffix();
  QString new_file_name;

  while( fi.exists() )
  {
    new_file_name = QString( "%1 (%2)%3" )
                      .arg( file_base_name )
                      .arg( add_date_time ? QString( "%1 %2" ).arg( QDateTime::currentDateTime().toString( Qt::ISODate ).replace( ":", "." ), QString::number( counter ) ) : QString::number( counter ) )
                      .arg( file_suffix.isEmpty() ? QString( "" ) : QString( ".%1" ) ).arg( file_suffix );
    fi.setFile( dir_path, new_file_name );
    counter++;

    if( counter > 98 )
    {
      qWarning() << "Unable to find a unique file name from path" << file_path << "(so overwrite the last one)";
      break;
    }
  }

  return QDir::toNativeSeparators( fi.absoluteFilePath() );
}

QString Bee::suffixFromFile( const QString& file_path )
{
  if( file_path.isEmpty() )
    return "";
  QStringList sl = file_path.split( "." );
  if( sl.size() > 1 )
    return sl.last();
  else
    return "";
}

bool Bee::isFileTypeAudio( const QString& file_suffix )
{
  QString sx = file_suffix.toLower();
  return sx == "mp3" || sx == "wav" || sx == "wma" || sx == "flac" || sx == "aiff" || sx == "aac" || sx == "m4a" || sx == "m4p" ||
         sx == "ogg" || sx == "oga" || sx == "ra" || sx == "rm";
}

bool Bee::isFileTypeVideo( const QString& file_suffix )
{
  QString sx = file_suffix.toLower();
  return sx ==  "mpeg" || sx ==  "mpg" || sx == "mp4" || sx == "avi" || sx == "mkv" || sx == "wmv" || sx == "flv" || sx ==  "mov" ||
         sx ==  "3gp" || sx ==  "mpe";
}

bool Bee::isFileTypeImage( const QString& file_suffix )
{
  QString sx = file_suffix.toLower();
  return sx == "jpg" || sx == "jpeg" || sx == "gif" || sx == "bmp" || sx == "png" || sx == "tiff" || sx == "tif" || sx == "psd" ||
         sx == "nef" || sx == "cr2"  || sx == "dng" || sx == "dcr" || sx == "3fr" || sx == "raf" || sx == "orf" || sx == "pef" ||
         sx == "arw" || sx == "svg" || sx == "ico" || sx == "ppm" || sx == "pgm" || sx == "pbm" || sx == "pnm" || sx == "webp";
}

bool Bee::isFileTypeDocument( const QString& file_suffix )
{
  QString sx = file_suffix.toLower();
  return sx ==  "pdf" || sx.startsWith( "doc" ) || sx.startsWith( "xls" ) || sx.startsWith( "ppt" ) || sx.startsWith( "pps" ) ||
         sx ==  "rtf" || sx ==  "txt" || sx ==  "odt" || sx ==  "odp" || sx ==  "ods" || sx ==  "csv" || sx ==  "log" ||
         sx ==  "mobi" || sx ==  "epub";
}

bool Bee::isFileTypeExe( const QString& file_suffix )
{
  QString sx = file_suffix.toLower();
  return sx == "exe" || sx == "bat" || sx == "inf" || sx == "com" || sx == "sh" || sx == "cab" || sx == "cmd" || sx == "bin";
}

bool Bee::isFileTypeBundle( const QString& file_suffix )
{
  QString sx = file_suffix.toLower();
  return sx == "app" || sx == "dmg";
}

Bee::FileType Bee::fileTypeFromSuffix( const QString& file_suffix )
{
  if( isFileTypeDocument( file_suffix ) )
    return Bee::FileDocument;

  if( isFileTypeImage( file_suffix ) )
    return Bee::FileImage;

  if( isFileTypeAudio( file_suffix ) )
    return Bee::FileAudio;

  if( isFileTypeVideo( file_suffix ) )
    return Bee::FileVideo;

  if( isFileTypeExe( file_suffix ) )
    return Bee::FileExe;

  if( isFileTypeBundle( file_suffix ) )
    return Bee::FileBundle;

  return Bee::FileOther;
}

static const char* FileTypeToString[] =
{
  QT_TRANSLATE_NOOP( "File", "Audio" ),
  QT_TRANSLATE_NOOP( "File", "Video" ),
  QT_TRANSLATE_NOOP( "File", "Image" ),
  QT_TRANSLATE_NOOP( "File", "Document" ),
  QT_TRANSLATE_NOOP( "File", "Other" ),
  QT_TRANSLATE_NOOP( "File", "Executable" ),
  QT_TRANSLATE_NOOP( "File", "MacOSX" )
};

QString Bee::fileTypeToString( Bee::FileType ft )
{
  if( ft < 0 || ft > Bee::NumFileType )
    ft = Bee::FileOther;
  return qApp->translate( "File", FileTypeToString[ ft ] );
}

static const char* ChatMessageTypeToString[] =
{
  QT_TRANSLATE_NOOP( "ChatMessage", "Header" ),
  QT_TRANSLATE_NOOP( "ChatMessage", "System" ),
  QT_TRANSLATE_NOOP( "ChatMessage", "Chat" ),
  QT_TRANSLATE_NOOP( "ChatMessage", "Connection" ),
  QT_TRANSLATE_NOOP( "ChatMessage", "User Information" ),
  QT_TRANSLATE_NOOP( "ChatMessage", "File Transfer" ),
  QT_TRANSLATE_NOOP( "ChatMessage", "History" ),
  QT_TRANSLATE_NOOP( "ChatMessage", "Other" ),
  QT_TRANSLATE_NOOP( "ChatMessage", "Image Preview" ),
  QT_TRANSLATE_NOOP( "ChatMessage", "Autoresponder" )
};

QString Bee::chatMessageTypeToString( int cmt )
{
  if( cmt < 0 || cmt > ChatMessage::NumTypes )
    cmt = ChatMessage::Other;
  return qApp->translate( "ChatMessage", ChatMessageTypeToString[ cmt ] );
}

QString Bee::dateTimeStringSuffix( const QDateTime& dt )
{
  QString s = dt.toString( "yyyyMMdd-hhmmss" );
  return s;
}

QString Bee::capitalizeFirstLetter( const QString& txt, bool all_chars_after_space )
{
  if( txt.isEmpty() )
    return txt;
  QString tmp = txt.toLower();
  QString capitalized = "";
  bool apply_title_case = true;
  QChar c;
  for( int i = 0; i < tmp.size(); i++ )
  {
    c = tmp.at( i );

    if( c.isSpace() )
    {
      capitalized += c;
      if( all_chars_after_space )
        apply_title_case = true;
    }
    else if( apply_title_case )
    {
      capitalized += c.toTitleCase();
      apply_title_case = false;
    }
    else
      capitalized += c;
  }

  return capitalized;
}

QColor Bee::invertColor( const QColor& c )
{
  int r, g, b;
  c.getRgb( &r, &g, &b );
  int i_r = 255 - r;
  int i_g = 255 - g;
  int i_b = 255 - b;

  int s_r = r - i_r;
  int s_g = r - i_g;
  int s_b = r - i_b;

  if( qAbs( s_r ) < 30 && qAbs( s_g ) < 30 && qAbs( s_b ) < 30 ) // gray on gray
    return QColor( 0, 0, 0 );
  else
    return QColor( i_r, i_g, i_b );
}

bool Bee::isColorNear( const QColor& c1, const QColor& c2 )
{
  int r_diff = c1.red() - c2.red();
  int g_diff = c1.green() - c2.green();
  int b_diff = c1.blue() - c2.blue();

  return qAbs( r_diff ) < 30 && qAbs( g_diff ) < 30 && qAbs( b_diff ) < 30;
}

QString Bee::removeHtmlTags( const QString& s )
{
  QTextDocument text_document;
  text_document.setHtml( s );
  return text_document.toPlainText();
}

QBrush Bee::defaultTextBrush()
{
  return qApp->palette().text();
}

QBrush Bee::defaultBackgroundBrush()
{
  return QBrush( Qt::transparent );
}

QBrush Bee::defaultHighlightedTextBrush()
{
  return qApp->palette().highlightedText();
}

QBrush Bee::defaultHighlightBrush()
{
  return qApp->palette().highlight();
}

QBrush Bee::userStatusBackgroundBrush( int user_status )
{
  if( user_status == User::Away || user_status == User::Busy )
    return QBrush( userStatusColor( user_status ) );
  else
    return defaultBackgroundBrush();
}

QPixmap Bee::convertToGrayScale( const QIcon& icon_to_convert, const QSize& pixmap_size )
{
  return convertToGrayScale( icon_to_convert.pixmap( pixmap_size ) );
}

QPixmap Bee::convertToGrayScale( const QPixmap& pix )
{
  QImage img = pix.toImage();
  if( img.isNull() )
    return QPixmap();

  int pixels = img.width() * img.height();
  if( pixels*(int)sizeof(QRgb) <= img.byteCount() )
  {
    QRgb *data = (QRgb*)img.bits();
    for (int i = 0; i < pixels; i++)
    {
      int val = qGray(data[i]);
      data[i] = qRgba(val, val, val, qAlpha(data[i]));
    }
  }

  QPixmap ret_pix;

#if QT_VERSION < 0x040700
  ret_pix = QPixmap::fromImage( img );
#else
  ret_pix.convertFromImage( img );
#endif
  return ret_pix;
}

QChar Bee::naviveFolderSeparator()
{
  return QDir::separator();
}

QString Bee::convertToNativeFolderSeparator( const QString& raw_path )
{
  QString path_converted( raw_path );
  QChar from_char;
#if defined( Q_OS_WIN ) || defined( Q_OS_OS2 ) || defined( Q_OS_OS2EMX ) || defined( Q_OS_SYMBIAN )
  from_char = QLatin1Char( '/' );
#else
  from_char = QLatin1Char( '\\' );
#endif
  QChar to_char = QDir::separator();

  for( int i = 0; i < path_converted.length(); i++ )
  {
    if( path_converted[ i ] == from_char )
      path_converted[ i ] = to_char;
  }

  return path_converted;
}

QString Bee::folderCdUp( const QString& folder_path )
{
  QStringList sl = Bee::convertToNativeFolderSeparator( folder_path ).split( naviveFolderSeparator() );
  if( sl.isEmpty() )
    return folder_path;

  sl.removeLast();
  return sl.join( naviveFolderSeparator() );
}

bool Bee::setLastModifiedToFile( const QString& to_path, const QDateTime& dt_last_modified )
{
  QFileInfo file_info_to( to_path );
  if( !file_info_to.exists() )
  {
    qWarning() << "Unable to set last modidified time to not existing file" << qPrintable( to_path );
    return false;
  }

  uint mod_time = dt_last_modified.toTime_t();
  uint ac_time = dt_last_modified.toTime_t();

  if( mod_time == (uint)-1 || ac_time == (uint)-1 )
  {
    qWarning() << "Unable to set invalid last modidified time to file" << qPrintable( to_path );
    return false;
  }

  bool ok = false;

#ifdef Q_OS_WIN

  uint cr_time = mod_time;

  FILETIME ft_modified, ft_creation, ft_access;

  LPCWSTR to_file_name = (const WCHAR*)to_path.utf16();

  HANDLE h_file = ::CreateFileW( to_file_name, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );

  if( h_file != INVALID_HANDLE_VALUE )
  {
    LONGLONG ll = Int32x32To64( cr_time, 10000000) + 116444736000000000;
    ft_creation.dwLowDateTime = (DWORD) ll;
    ft_creation.dwHighDateTime = ll >> 32;
    LONGLONG ll2 = Int32x32To64( mod_time, 10000000) + 116444736000000000;
    ft_modified.dwLowDateTime = (DWORD) ll2;
    ft_modified.dwHighDateTime = ll2 >> 32;
    LONGLONG ll3 = Int32x32To64( ac_time, 10000000) + 116444736000000000;
    ft_access.dwLowDateTime = (DWORD) ll3;
    ft_access.dwHighDateTime = ll3 >> 32;

    if( !::SetFileTime( h_file, &ft_creation, &ft_access, &ft_modified ) )
    {
      QString s_error = QString( "0x%1" ).arg( (unsigned long)GetLastError() );
      qWarning() << "Function SetFileTime has error" << s_error << "for file" << qPrintable( to_path );
    }
    else
      ok = true;
  }
  else
    qWarning() << "Unable to get the HANDLE (CreateFile) of file" << qPrintable( to_path );

  CloseHandle( h_file );

#else

  struct utimbuf from_time_buffer;
  from_time_buffer.modtime = mod_time;
  from_time_buffer.actime = ac_time;

  const char *to_file_name = to_path.toLatin1().constData();
  ok = utime( to_file_name, &from_time_buffer ) == 0;
  if( !ok )
    qWarning() << "Function utime error" << errno << ":" << qPrintable( QString::fromLatin1( strerror( errno ) ) ) << "for file" << qPrintable( to_path );

#endif

  return ok;
}

bool Bee::showFileInGraphicalShell( const QString& file_path )
{
  QFileInfo file_info( file_path );
  if( !file_info.exists() )
    return false;

#ifdef Q_OS_WIN
  QString explorer_path = QLatin1String( "c:\\windows\\explorer.exe" );

  if( QFile::exists( explorer_path ) )
  {
    QStringList explorer_args;
    if( !file_info.isDir() )
      explorer_args += QLatin1String("/select,");
    explorer_args += Bee::convertToNativeFolderSeparator( file_info.canonicalFilePath() );
    if( QProcess::startDetached( explorer_path, explorer_args ) )
      return true;
    else
      qWarning() << "Unable to start process:" << qPrintable( explorer_path ) << qPrintable( explorer_args.join( " " ) );
  }
#endif

#ifdef Q_OS_MAC

  QStringList script_args;
  script_args << QLatin1String( "-e" )
              << QString::fromLatin1( "tell application \"Finder\" to reveal POSIX file \"%1\"" ).arg( file_info.canonicalFilePath() );
  QProcess::execute( QLatin1String( "/usr/bin/osascript" ), script_args );
  script_args.clear();
  script_args << QLatin1String( "-e" )
             << QLatin1String( "tell application \"Finder\" to activate" );
  QProcess::execute( QLatin1String( "/usr/bin/osascript" ), script_args );
  return true;

#endif

  return false;
}

bool Bee::folderIsWriteable( const QString& folder_path )
{
  QDir folder_to_test( folder_path );
  if( !folder_to_test.exists() )
    return folder_to_test.mkpath( "." );

  QFile test_file( QString( "%1/%2" ).arg( folder_path ).arg( "beetestfile.txt" ) );
  if( test_file.open( QFile::WriteOnly ) )
  {
    test_file.close();
    test_file.remove();
    return true;
  }
  else
    return false;
}

static int GetBoxSize( int pix_size )
{
  int box_size = pix_size > 10 ? pix_size / 10 : 1;
  if( box_size % 2 > 7 )
    box_size++;
  box_size = qMax( 1, box_size );
  return box_size;
}

QPixmap Bee::avatarForUser( const User& u, const QSize& avatar_size, bool use_available_user_image )
{
  QPixmap user_avatar;
  bool default_avatar_used = false;
  if( u.vCard().photo().isNull() || !use_available_user_image )
  {
    default_avatar_used = true;
    Avatar av;
    av.setName( u.isValid() ? u.name() : "??" );
    if( u.isStatusConnected() )
      av.setColor( u.color() );
    else
      av.setColor( QColor( Qt::gray ).name() );
    av.setSize( avatar_size );
    if( !av.create() )
    {
      user_avatar = QIcon( Bee::menuUserStatusIconFileName( u.status() ) ).pixmap( avatar_size );
      return user_avatar;
    }
    else
      user_avatar = av.pixmap();
  }
  else
  {
    user_avatar = u.vCard().photo().scaled( avatar_size );
    if( !u.isStatusConnected() )
    {
      user_avatar = convertToGrayScale(user_avatar );
      return user_avatar;
    }
  }

  int pix_height = user_avatar.height();
  int pix_width = user_avatar.width();
  int box_height = GetBoxSize( pix_height );
  int box_width = GetBoxSize( pix_width );
  int box_start_height = qMax( 1, box_height / 2 );
  int box_start_width = qMax( 1, box_width / 2 );

  QPixmap pix( pix_width, pix_height );
  QPainter p( &pix );
  if( !default_avatar_used )
  {
    pix.fill( Bee::userStatusColor( u.status() ) );
    p.drawPixmap( box_start_width, box_start_height, pix_width - box_width, pix_height - box_height, user_avatar.scaled( pix_width - box_width, pix_height - box_height ) );
  }
  else
  {
    p.drawPixmap( 0, 0, pix_width, pix_height, user_avatar );
    p.setPen( Bee::userStatusColor( u.status() ) );
    for( int i = 0; i < box_height; i++ )
    {
      p.drawLine( 0, i, pix_width, i );
      p.drawLine( 0, pix_height-box_height+i, pix_width, pix_height-box_height+i );
    }

    for( int i = 0; i < box_width; i++ )
    {
      p.drawLine( i, 0, i, pix_height );
      p.drawLine( pix_width-box_width+i, 0, pix_width-box_width+i, pix_height );
    }
  }

  return pix;
}

QString Bee::toolTipForUser( const User& u, bool only_status )
{
  QString tool_tip = u.isLocal() ? QObject::tr( "You are %1" ).arg( Bee::userStatusToString( u.status() ) ) : QObject::tr( "%1 is %2" ).arg( u.name(), Bee::userStatusToString( u.status() ) );

  if( only_status )
    return tool_tip;

  if( u.isStatusConnected() )
  {
    if( u.statusDescription().isEmpty() )
      tool_tip += QString( "\n" );
    else
      tool_tip += QString( ": %1\n" ).arg( u.statusDescription() );

    if( !u.vCard().info().isEmpty() )
    {
      tool_tip += QString( "~~~\n" );
      tool_tip += u.vCard().info();
      tool_tip += QString( "\n~~~\n" );
    }

    if( u.statusChangedIn().isValid() )
      tool_tip += QString( "(%1 %2)" ).arg( QObject::tr( "last update" ) ).arg( Bee::dateTimeToString( u.statusChangedIn() ) );
  }
  else
  {
    if( u.lastConnection().isValid() )
      tool_tip += QString( "\n(%1 %2)" ).arg( QObject::tr( "last connection" ) ).arg( Bee::dateTimeToString( u.lastConnection() ) );
  }

  if( !u.vCard().birthday().isNull() )
  {
    QString text = userBirthdayToText( u );
    if( !text.isEmpty() )
      tool_tip +=  QString( "\n* %1 *" ).arg( text );
  }

  return tool_tip;
}

QString Bee::userBirthdayToText( const User& u )
{
  QString birthday_text;
  if( !u.isLocal() )
  {
    int days_to = u.daysToBirthDay();
    if( days_to == 0 )
      birthday_text = QObject::tr( "Today is %1's birthday" ).arg( u.name() );
    else if( days_to == 1 )
      birthday_text =  QObject::tr( "Tomorrow is %1's birthday" ).arg( u.name() );
    else if( days_to > 1 && days_to < 10 )
      birthday_text= QObject::tr( "%1's birthday is in %2 days" ).arg( u.name() ).arg( days_to );
    else if( days_to == -1 )
      birthday_text = QObject::tr( "Yesterday was %1's birthday" ).arg( u.name() );
    else
      birthday_text = "";
  }
  else
  {
    if( u.isBirthDay() )
      birthday_text = QObject::tr( "Happy Birthday to you!" );
    else
      birthday_text = "";
  }

  return birthday_text;
}

void Bee::setWindowStaysOnTop( QWidget* w, bool enable )
{
  bool w_was_visible = w->isVisible();
  if( w_was_visible )
    w->hide();

  Qt::WindowFlags w_flags = w->windowFlags();
  if( enable )
    w_flags |= Qt::WindowStaysOnTopHint;
  else
    w_flags &= ~Qt::WindowStaysOnTopHint;
  w->setWindowFlags( w_flags );

  if( w_was_visible )
    QMetaObject::invokeMethod( w, "show", Qt::QueuedConnection );
}

QString Bee::stringListToTextString( const QStringList& sl, int max_items )
{
  if( sl.isEmpty() )
    return "";
  if( sl.size() == 1 )
    return sl.first();
  if( sl.size() == 2 )
    return sl.join( QString( " %1 " ).arg( QObject::tr( "and" ) ) );

  QStringList sl_to_join;
  if( max_items < 1 || max_items > (sl.size()-1))
    max_items = sl.size()-1;
  int num_items = 0;

  foreach( QString s, sl )
  {
    if( num_items >= max_items )
      break;
    num_items++;
    sl_to_join << s;
  }

  QString s_joined = sl_to_join.join( ", " );
  int diff_items = sl.size() - sl_to_join.size();
  if( diff_items == 1 )
  {
    if( !sl.last().isEmpty() )
    {
      s_joined.append( QString( " %1 " ).arg( QObject::tr( "and" ) ) );
      s_joined.append( sl.last() );
    }
  }
  else
    s_joined.append( QString( " %1" ).arg( QObject::tr( "and %1 others" ).arg( diff_items ) ) );

  return s_joined;
}

void Bee::removeContextHelpButton( QWidget* w )
{
  Qt::WindowFlags w_flags = w->windowFlags();
  w_flags &= ~Qt::WindowContextHelpButtonHint;
  w->setWindowFlags( w_flags );
}

void Bee::showUp( QWidget* w )
{
  if( !w->isVisible() )
    w->show();

  if( w->isMinimized() )
    w->showNormal();

  w->raise();
}

void Bee::raiseOnTop( QWidget* w )
{
  bool on_top_flag_added = false;
  if( !(w->windowFlags() & Qt::WindowStaysOnTopHint) )
  {
#if defined Q_OS_WIN && QT_VERSION < 0x050000
    ::SetWindowPos( (HWND)w->winId(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );
#else
    Bee::setWindowStaysOnTop( w, true );
#endif
    on_top_flag_added = true;
  }

  Bee::showUp( w );

  if( on_top_flag_added )
  {
#if defined Q_OS_WIN && QT_VERSION < 0x050000
    ::SetWindowPos( (HWND)w->winId(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );
#else
    Bee::setWindowStaysOnTop( w, false );
#endif
  }
}

bool Bee::areStringListEqual( const QStringList& sl1, const QStringList& sl2, Qt::CaseSensitivity cs )
{
  if( sl1.size() != sl2.size() )
    return false;

  foreach( QString s, sl1 )
  {
    if( !sl2.contains( s, cs ) )
      return false;
  }
  return true;
}

QString Bee::dateTimeToString( const QDateTime& dt )
{
  return dt.date() == QDate::currentDate() ? dt.time().toString( Qt::SystemLocaleShortDate ) : dt.toString( Qt::SystemLocaleShortDate );
}

QString Bee::beeColorsToHtmlText( const QString& txt )
{
  QString bee_txt = "";
  for( int i = 0; i < txt.size(); i++ )
    bee_txt.append( QString( "<font color=%1>%2</font>" ).arg( (i % 2 == 0) ? "#000000" : "#dede00" ).arg( txt.at( i ) ) );
  return bee_txt;
}

void Bee::setBackgroundColor( QWidget* w, const QColor& c )
{
  QPalette pal = w->palette();
  pal.setBrush( QPalette::Base, QBrush( c ) );
  w->setPalette( pal );
}

QColor Bee::selectColor( QWidget* w, const QColor& default_color )
{
  return QColorDialog::getColor( default_color, w );
}
