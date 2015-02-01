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

#include "BeeUtils.h"
#include "PluginManager.h"
#include "User.h"


QString Bee::userStatusIconFileName( int user_status )
{
  switch( user_status )
  {
  case User::Offline:
    return QString( ":/images/user-offline.png" );
  case User::Away:
    return QString( ":/images/user-away.png" );
  case User::Busy:
    return QString( ":/images/user-busy.png" );
  default:
    return QString( ":/images/user-online.png" );
  }
}

QString Bee::menuUserStatusIconFileName( int user_status )
{
  switch( user_status )
  {
  case User::Offline:
    return QString( ":/images/menu-user-offline.png" );
  case User::Away:
    return QString( ":/images/menu-user-away.png" );
  case User::Busy:
    return QString( ":/images/menu-user-busy.png" );
  default:
    return QString( ":/images/menu-user-online.png" );
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
    prec = result >= 10 ? 0 : 1;
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

QString Bee::uniqueFilePath( const QString& file_path )
{
  int counter = 1;
  QFileInfo fi( file_path );
  QString dir_path = fi.absoluteDir().absolutePath();
  QString file_base_name = fi.completeBaseName();
  QString file_suffix = fi.suffix();
  QString new_file_name;

  while( fi.exists() )
  {
    new_file_name = QString( "%1 (%2)%3%4" ).arg( file_base_name ).arg( counter ).arg( (file_suffix.isEmpty() ? "" : ".") ).arg( file_suffix );
    fi.setFile( dir_path, new_file_name );
    counter++;

    if( counter > 98 )
    {
      qWarning() << "Unable to find a unique file name from path" << file_path << "(so overwrite the last one)";
      break;
    }
  }

  return fi.absoluteFilePath();
}

bool Bee::isFileTypeAudio( const QString& file_suffix )
{
  QString sx = file_suffix.toLower();
  return sx == "mp3" || sx == "wma" || sx == "flac" || sx == "aiff" || sx == "aac" || sx == "m4a" || sx == "m4p" || sx == "ogg" || sx == "oga" || sx == "ra" || sx == "rm";
}

bool Bee::isFileTypeVideo( const QString& file_suffix )
{
  QString sx = file_suffix.toLower();
  return sx ==  "mpeg" || sx ==  "mpg" || sx == "mp4" || sx == "avi" || sx == "mkv" || sx == "wmv" || sx == "flv" || sx ==  "mov" || sx ==  "3gp" || sx ==  "mpe";
}

bool Bee::isFileTypeImage( const QString& file_suffix )
{
  QString sx = file_suffix.toLower();
  return sx == "jpg" || sx == "jpeg" || sx == "gif" || sx == "bmp" || sx == "png" || sx == "tiff" || sx == "tif" || sx == "psd";
}

bool Bee::isFileTypeDocument( const QString& file_suffix )
{
  QString sx = file_suffix.toLower();
  return sx ==  "pdf" || sx.startsWith( "doc" ) || sx.startsWith( "xls" ) || sx.startsWith( "ppt" ) || sx.startsWith( "pps" )
    || sx ==  "rtf" || sx ==  "txt" || sx ==  "odt" || sx ==  "odp" || sx ==  "ods" || sx ==  "csv" || sx ==  "log" || sx ==  "mobi" || sx ==  "epub";
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

QString Bee::dateTimeStringSuffix( const QDateTime& dt )
{
  QString s = dt.toString( "yyyy mm dd-hhmmss" );
  s.remove( QChar( ' ' ) );
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
