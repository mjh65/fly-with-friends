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

#if 0
#include "iconfig.h"
#include "iengine.h"
#include <memory>
#include <vector>

namespace fwf {

// Interface for object providing the plugin control

class IControl
{
public:
    static IControl * New(std::shared_ptr<IConfig> & config, std::shared_ptr<IEngine> & engine);
    virtual ~IControl() {}
    
    virtual void ConfigureHosting() = 0;
    virtual void ConfigureJoining() = 0;
    virtual float UpdateDialogs() = 0;

};

}

#endif
