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

#ifndef BEEBEEP_VERSION_H
#define BEEBEEP_VERSION_H

const char* BEEBEEP_NAME = "BeeBEEP";
const char* BEEBEEP_ORGANIZATION = "MarcoMastroddiSW";
const char* BEEBEEP_ORGANIZATION_DOMAIN = "beebeep.net";
const char* BEEBEEP_DNS_RECORD = "_beebeep._tcp";
#ifdef BEEBEEP_DEBUG
  const char* BEEBEEP_WEBSITE = "http://localhost/beebeep";
#else
  const char* BEEBEEP_WEBSITE = "http://beebeep.sourceforge.net";
#endif
const char* BEEBEEP_DOWNLOAD_WEBSITE = "/download.php";
const char* BEEBEEP_DONATE_WEBSITE = "/donate.php";
const char* BEEBEEP_HELP_WEBSITE = "/help.php";
const char* BEEBEEP_LANGUAGE_WEBSITE = "/language.php";
const char* BEEBEEP_CHECK_VERSION_WEBSITE = "/checkversion.php";
const char* BEEBEEP_PAGE_ON_FACEBOOK = "https://www.facebook.com/beebeeplanmessenger";
const char* BEEBEEP_UPDATER_WEBSITE = "/update/beeupdater.txt";
const char* BEEBEEP_GA_TRACKING_ID = "UA-57878696-1";
const char* BEEBEEP_GA_URL = "http://www.google-analytics.com/collect";
const char* BEEBEEP_GA_EVENT_VERSION = "1";
const char* BEEBEEP_VERSION = "3.0.8";
const int BEEBEEP_PROTO_VERSION = 65;
const int BEEBEEP_SETTINGS_VERSION = 6;
const int BEEBEEP_BUILD = 769;

#endif // BEEBEEP_VERSION_H

