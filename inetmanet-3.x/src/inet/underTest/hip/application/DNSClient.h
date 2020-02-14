//*********************************************************************************
// File:           DNSClient.h
//
// Authors:        Laszlo Tamas Zeke, Levente Mihalyi, Laszlo Bokor
//
// Copyright: (C) 2008-2009 BME-HT (Department of Telecommunications,
// Budapest University of Technology and Economics), Budapest, Hungary
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//**********************************************************************************
// Part of: HIPSim++ Host Identity Protocol Simulation Framework developed by BME-HT
//**********************************************************************************



#ifndef __DNSCLIENT_H__
#define __DNSCLIENT_H__

#include "inet/common/INETDefs.h"
#include "inet/transportlayer/contract/udp/UDPSocket.h"

/**
 simple DNS client module to provide DNS access to tcp apps
 */

namespace inet {


class INET_API DNSClient : public cSimpleModule
{
  protected:
    UDPSocket socket;
    ///@name Overridden cSimpleModule functions
    //@{
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cPacket* msg);
    //@}

};
}

#endif
