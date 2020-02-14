// Copyright (C) 2009 Juan-Carlos Maureira
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef __LINKFAILUREMANAGER_H__
#define __LINKFAILUREMANAGER_H__

#include "inet/common/INETDefs.h"

namespace inet {


enum LinkState
{
    UP,
    DOWN
};

std::ostream& operator << (std::ostream& os, const LinkState& ls);

class LinkFailureManager : public cSimpleModule
{
  public:
    void scheduleLinkStateChange(cGate* gate, simtime_t when, LinkState state);

  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
};

}

#endif
