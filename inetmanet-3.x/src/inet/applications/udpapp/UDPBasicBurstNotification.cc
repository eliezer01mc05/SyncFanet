//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2007 Universidad de Malaga
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


#include "inet/applications/udpapp/UDPBasicBurstNotification.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "inet/transportlayer/contract/udp/UDPControlInfo_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/InterfaceTable.h"
#include "inet/common/ModuleAccess.h"

namespace inet {

Define_Module(UDPBasicBurstNotification);

UDPBasicBurstNotification::UDPBasicBurstNotification()
{
    addressModule = nullptr;
}

UDPBasicBurstNotification::~UDPBasicBurstNotification()
{
    if (addressModule)
        delete addressModule;
}

void UDPBasicBurstNotification::initialConfiguration()
{
    socket.setOutputGate(gate("udpOut"));
    socket.bind(localPort);

    if (par("setBroadcast").boolValue())
        socket.setBroadcast(true);
    IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    if (strcmp(par("outputInterface").stringValue(),"") != 0)
    {
        InterfaceEntry *ie = ift->getInterfaceByName(par("outputInterface").stringValue());
        if (ie == nullptr)
            throw cRuntimeError(this, "Invalid output interface name : %s",par("outputInterface").stringValue());
        outputInterface = ie->getInterfaceId();
    }

    outputInterfaceMulticastBroadcast.clear();
    if (strcmp(par("outputInterfaceMulticastBroadcast").stringValue(),"") != 0)
    {
        const char *ports = par("outputInterfaceMulticastBroadcast");
        cStringTokenizer tokenizer(ports);
        const char *token;
        while ((token = tokenizer.nextToken()) != nullptr)
        {
            if (strstr(token, "ALL") != nullptr)
            {
                for (int i = 0; i < ift->getNumInterfaces(); i++)
                {
                    InterfaceEntry *ie = ift->getInterface(i);
                    if (ie->isLoopback())
                        continue;
                    if (ie == nullptr)
                        throw cRuntimeError(this, "Invalid output interface name : %s", token);
                    outputInterfaceMulticastBroadcast.push_back(ie->getInterfaceId());
                }
            }
            else
            {
                InterfaceEntry *ie = ift->getInterfaceByName(token);
                if (ie == nullptr)
                    throw cRuntimeError(this, "Invalid output interface name : %s", token);
                outputInterfaceMulticastBroadcast.push_back(ie->getInterfaceId());
            }
        }
    }

    addressModule = new AddressModule();
    //addressModule->initModule(par("chooseNewIfDeleted").boolValue());
    addressModule->initModule(true);
    std::string destAddresses = par("destAddresses").stdstringValue();
    if (strcmp(destAddresses.c_str(),"") != 0)
        isSource = true;
}


void UDPBasicBurstNotification::processStart()
{
    if (!par("configureInInit").boolValue())
        initialConfiguration();
    if (isSource)
    {
        if (chooseDestAddrMode == ONCE)
            destAddr = chooseDestAddr();
        activeBurst = true;
    }

    nextSleep = simTime();
    nextBurst = simTime();
    nextPkt = simTime();
    activeBurst = false;

    timerNext->setKind(SEND);
    processSend();
}


L3Address UDPBasicBurstNotification::chooseDestAddr()
{
    if (addressModule->isInit())
        return addressModule->choseNewAddress();
    else
    {
        addressModule->initModule(true);
        if (addressModule->isInit())
            return addressModule->choseNewAddress();
        return IPv4Address::UNSPECIFIED_ADDRESS;
    }
}


void UDPBasicBurstNotification::generateBurst()
{
    simtime_t now = simTime();

    if (nextPkt < now)
        nextPkt = now;

    double sendInterval = sendIntervalPar->doubleValue();
    if (sendInterval <= 0.0)
        throw cRuntimeError(this, "The sendInterval parameter must be bigger than 0");
    nextPkt += sendInterval;

    if (activeBurst && nextBurst <= now) // new burst
    {
        double burstDuration = burstDurationPar->doubleValue();
        if (burstDuration < 0.0)
            throw cRuntimeError(this, "The burstDuration parameter mustn't be smaller than 0");
        double sleepDuration = sleepDurationPar->doubleValue();

        if (burstDuration == 0.0)
            activeBurst = false;
        else
        {
            if (sleepDuration < 0.0)
                throw cRuntimeError(this, "The sleepDuration parameter mustn't be smaller than 0");
            nextSleep = now + burstDuration;
            nextBurst = nextSleep + sleepDuration;
        }

        if (chooseDestAddrMode == PER_BURST)
            chooseDestAddr();
    }

    if (chooseDestAddrMode == PER_SEND)
        chooseDestAddr();

    destAddr = addressModule->getAddress();
    if (!destAddr.isUnspecified())
    {
        cPacket *payload = createPacket();
        payload->setTimestamp();
        emit(sentPkSignal, payload);
        // Check address type
        // Check address type
        if (!sendBroadcast(destAddr, payload))
        {
            UDPSocket::SendOptions options;
            options.outInterfaceId = outputInterface;
            socket.sendTo(payload, destAddr, destPort, &options);
        }
        numSent++;
    }
    // Next timer
    if (activeBurst && nextPkt >= nextSleep)
        nextPkt = nextBurst;

    scheduleAt(nextPkt, timerNext);
}

void UDPBasicBurstNotification::receiveSignal(cComponent *source, simsignal_t signalID, long l, cObject *details)
{
    Enter_Method_Silent();
    if (signalID == NF_INTERFACE_IPv4CONFIG_CHANGED || signalID == NF_INTERFACE_IPv6CONFIG_CHANGED)
        addressModule->rebuildAddressList();

}

}

