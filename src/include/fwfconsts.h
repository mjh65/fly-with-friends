/*
 *   Fly-With-Friends - X-Plane plugin for simple group flying
 *   Copyright (C) 2020 Michael Hasling <michael.hasling@gmail.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Affero General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Affero General Public License for more details.
 *
 *   You should have received a copy of the GNU Affero General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

namespace fwf {

static const unsigned int   MAX_IN_SESSION              = 16;   // could be tuned up maybe?
static const unsigned int   MAX_DATAGRAM_LEN            = 524;
static const unsigned int   MAX_PAYLOAD_LEN             = 512;
static const unsigned int   CLIENT_UPDATE_PERIOD_MS     = 100;
static const unsigned int   SERVER_BROADCAST_PERIOD_MS  = 320;
static const unsigned int   ID_REBROADCAST_PERIOD_MS    = 7500;

}

