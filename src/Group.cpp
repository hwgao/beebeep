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

#include "Group.h"


Group::Group()
  : m_id( ID_INVALID ), m_name( "" ), m_usersId(), m_privateId( "" )
{
}

Group::Group( const Group& g )
{
  (void)operator=( g );
}

Group& Group::operator=( const Group& g )
{
  if( this != &g )
  {
    m_id = g.m_id;
    m_name = g.m_name;
    m_usersId = g.m_usersId;
    m_privateId = g.privateId();
  }
  return *this;
}

bool Group::addUser( VNumber user_id )
{
  if( hasUser( user_id ) )
    return false;
  m_usersId.append( user_id );
  return true;
}

int Group::addUsers( const QList<VNumber>& user_list )
{
  int num = 0;
  foreach( VNumber user_id, user_list )
  {
    if( addUser( user_id ) )
      num++;
  }
  return num;
}