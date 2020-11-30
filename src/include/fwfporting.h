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

#if defined(TARGET_WINDOWS)
#define SPRINTF sprintf_s
#define SSCANF sscanf_s
#define STRCAT strcat_s
#define STRCPY strcpy_s
#define STRNCPY strncpy_s
#define LOCALTIME(t,s) localtime_s(s,t)
#elif defined(TARGET_MACOSX)
#define SPRINTF sprintf
#define SSCANF sscanf
#define STRCAT strcat
#define STRCPY(t,n,s) strcpy(t,s)
#define STRNCPY strncpy
#define LOCALTIME localtime_r
#elif defined(TARGET_LINUX)
#endif

