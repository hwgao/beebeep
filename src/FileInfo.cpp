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

#include "FileInfo.h"


FileInfo::FileInfo()
  : m_transferType( FileInfo::Upload ), m_name( "" ), m_path( "" ), m_suffix( ""),
    m_size( 0 ), m_hostAddress(), m_hostPort( 0 ), m_password( "" ), m_id( ID_INVALID ),
    m_fileHash()
{
}

FileInfo::FileInfo( VNumber id, FileInfo::TransferType tt )
  : m_transferType( tt ), m_name( "" ), m_path( "" ), m_suffix( "" ), m_size( 0 ),
    m_hostAddress(), m_hostPort( 0 ), m_password( "" ), m_id( id ), m_fileHash()
{
}

FileInfo& FileInfo::operator=( const FileInfo& fi )
{
  if( this != &fi )
  {
    m_transferType = fi.m_transferType;
    m_name = fi.m_name;
    m_path = fi.m_path;
    m_suffix = fi.m_suffix;
    m_size = fi.m_size;
    m_hostAddress = fi.m_hostAddress;
    m_hostPort = fi.m_hostPort;
    m_password = fi.m_password;
    m_id =  fi.m_id;
    m_fileHash = fi.m_fileHash;
  }
  return *this;
}

