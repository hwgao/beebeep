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

#ifndef BEE_TWITTERSERVICE_H
#define BEE_TWITTERSERVICE_H

#include "twitterservice_global.h"
#include "Interfaces.h"
#include <QObject>


class BEETWITTERSERVICESHARED_EXPORT TwitterService : public QObject, public ServiceInterface
{
  Q_OBJECT
  Q_INTERFACES( ServiceInterface )

public:
  TwitterService();

  QString name() const;
  QString version() const;
  QString help() const;
  QString author() const;
  QIcon icon() const;
  QString iconFileName() const;
  int priority() const;
  QString coreVersion() const;

  QString protocol() const;
  QString hostAddress() const;
  int hostPort() const;

  bool fileTransferIsEnabled() const;

  QString userStatusOfflineIconPath() const;
  QString userStatusOnlineIconPath() const;
  QString userStatusBusyIconPath() const;
  QString userStatusAwayIconPath() const;

};

#endif // BEE_TWITTERSERVICE_H
