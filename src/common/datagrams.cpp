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

#include "datagrams.h"

namespace fwf {

const char* xdigits = "0123456789abcdef";

std::string AddressedDatagram::LogString()
{
    std::string str;
    str += address.GetAsString();
    str += ':';
    size_t n = str.size();
    str.resize(n + (2 * datagram->Length()));
    char* s = datagram->Buffer();
    char* d = const_cast<char *>(str.c_str() + n);
    for (unsigned int i = 0; i < datagram->Length(); ++i)
    {
        int c = *s++;
        *d++ = xdigits[(c & 0xf0) >> 4];
        *d++ = xdigits[c & 0x0f];
    }
    return str;
}

}

