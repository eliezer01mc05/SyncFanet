//
// Copyright (C) 2004 Andras Varga
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2011 Zoltan Bojthe
// Copyright (C) 2011 Alfonso Ariza, Universidad de Malaga
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


#ifndef __INET_UDPBASICBURSTNOTIFICATION_H
#define __INET_UDPBASICBURSTNOTIFICATION_H

#include "UDPBasicBurst.h"
#include "inet/applications/base/AddressModule.h"

namespace inet {

/**
 * UDP application. See NED for more info.
 */
class INET_API UDPBasicBurstNotification : public UDPBasicBurst, protected cListener
{
  protected:
    AddressModule * addressModule;
  protected:
    // chooses random destination address
    virtual L3Address chooseDestAddr() override;
    virtual void generateBurst() override;
    virtual void processStart() override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, long l, cObject *details) override;
    virtual void initialConfiguration() override;
  public:
    UDPBasicBurstNotification();
    virtual ~UDPBasicBurstNotification();
};

}

#endif

