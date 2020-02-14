//
// Copyright (C) OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "inet/linklayer/base/MACBase.h"
#include "inet/linklayer/base/MACProtocolBase.h"
#include "inet/visualizer/linklayer/DataLinkOsgVisualizer.h"

namespace inet {

namespace visualizer {

Define_Module(DataLinkOsgVisualizer);

bool DataLinkOsgVisualizer::isLinkEnd(cModule *module) const
{
    return dynamic_cast<MACProtocolBase *>(module) != nullptr || dynamic_cast<MACBase *>(module) != nullptr;
}

} // namespace visualizer

} // namespace inet

