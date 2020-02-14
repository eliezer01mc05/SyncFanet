//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2007 Universidad de Malaga
// Copyright (C) 2011 Zoltan Bojthe
// Copyright (C) 2013 Universidad de Malaga
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


#include "UDPBasicFlooding.h"

#include "inet/transportlayer/contract/udp/UDPControlInfo_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/InterfaceTable.h"
#include "inet/common/ModuleAccess.h"

namespace inet {

Define_Module(UDPBasicFlooding);

int UDPBasicFlooding::counter;

simsignal_t UDPBasicFlooding::sentPkSignal = registerSignal("sentPk");
simsignal_t UDPBasicFlooding::rcvdPkSignal = registerSignal("rcvdPk");
simsignal_t UDPBasicFlooding::outOfOrderPkSignal = registerSignal("outOfOrderPk");
simsignal_t UDPBasicFlooding::dropPkSignal = registerSignal("dropPk");
simsignal_t UDPBasicFlooding::floodPkSignal = registerSignal("floodPk");


UDPBasicFlooding::UDPBasicFlooding()
{
    messageLengthPar = nullptr;
    burstDurationPar = nullptr;
    sleepDurationPar = nullptr;
    sendIntervalPar = nullptr;
    timerNext = nullptr;
    addressModule = nullptr;
    outputInterfaceMulticastBroadcast.clear();
}

UDPBasicFlooding::~UDPBasicFlooding()
{
    cancelAndDelete(timerNext);
}

void UDPBasicFlooding::initialize(int stage)
{
    // because of AddressResolver, we need to wait until interfaces are registered,
    // address auto-assignment takes place etc.
    ApplicationBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
    {
        counter = 0;
        numSent = 0;
        numReceived = 0;
        numDeleted = 0;
        numDuplicated = 0;
        numFlood = 0;

        delayLimit = par("delayLimit");
        startTime = par("startTime");
        stopTime = par("stopTime");

        messageLengthPar = &par("messageLength");
        burstDurationPar = &par("burstDuration");
        sleepDurationPar = &par("sleepDuration");
        sendIntervalPar = &par("sendInterval");
        nextSleep = startTime;
        nextBurst = startTime;
        nextPkt = startTime;
        isSource = par("isSource");

        WATCH(numSent);
        WATCH(numReceived);
        WATCH(numDeleted);
        WATCH(numDuplicated);
        WATCH(numFlood);
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
        processStart();
}

void UDPBasicFlooding::processStart()
{
    localPort = par("localPort");
    destPort = par("destPort");

    socket.setOutputGate(gate("udpOut"));
    socket.bind(localPort);
    socket.setBroadcast(true);

    outputInterfaceMulticastBroadcast.clear();
    if (strcmp(par("outputInterfaceMulticastBroadcast").stringValue(),"") != 0)
    {
        IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
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
    myId = this->getParentModule()->getId();
}



cPacket *UDPBasicFlooding::createPacket()
{
    char msgName[32];
    sprintf(msgName, "UDPBasicAppData-%d", counter++);
    long msgByteLength = messageLengthPar->longValue();
    cPacket *payload = new cPacket(msgName);
    payload->setByteLength(msgByteLength);
    payload->addPar("sourceId") = getId();
    payload->addPar("msgId") = numSent;
    if (addressModule)
        payload->addPar("destAddr") = addressModule->choseNewModule();

    return payload;
}

void UDPBasicFlooding::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (dynamic_cast<cPacket*>(msg))
        {
            L3Address destAddr(IPv4Address::ALLONES_ADDRESS);
            sendBroadcast(destAddr, PK(msg));
        }
        else
        {
            if (stopTime <= 0 || simTime() < stopTime)
            {
                // send and reschedule next sending
                if (isSource) // if the node is a sink, don't generate messages
                    generateBurst();
            }
        }
    }
    else if (msg->getKind() == UDP_I_DATA)
    {
        // process incoming packet
        processPacket(PK(msg));
    }
    else if (msg->getKind() == UDP_I_ERROR)
    {
        EV << "Ignoring UDP error report\n";
        delete msg;
    }
    else
    {
        error("Unrecognized message (%s)%s", msg->getClassName(), msg->getName());
    }

    if (hasGUI())
    {
        char buf[40];
        sprintf(buf, "rcvd: %d pks\nsent: %d pks", numReceived, numSent);
        getDisplayString().setTagArg("t", 0, buf);
    }
}

void UDPBasicFlooding::processPacket(cPacket *pk)
{
    if (pk->getKind() == UDP_I_ERROR)
    {
        EV << "UDP error received\n";
        delete pk;
        return;
    }

    if (pk->hasPar("sourceId") && pk->hasPar("msgId"))
    {
        // duplicate control
        int moduleId = (int)pk->par("sourceId");
        int msgId = (int)pk->par("msgId");
        // check if this message has like origin this node
        if (moduleId == getId())
        {
            delete pk;
            return;
        }
        auto it = sourceSequence.find(moduleId);
        if (it != sourceSequence.end())
        {
            if (it->second >= msgId)
            {
                EV << "Out of order packet: " << UDPSocket::getReceivedPacketInfo(pk) << endl;
                emit(outOfOrderPkSignal, pk);
                delete pk;
                numDuplicated++;
                return;
            }
            else
                it->second = msgId;
        }
        else
            sourceSequence[moduleId] = msgId;
    }

    if (delayLimit > 0)
    {
        if (simTime() - pk->getTimestamp() > delayLimit)
        {
            EV << "Old packet: " << UDPSocket::getReceivedPacketInfo(pk) << endl;
            emit(dropPkSignal, pk);
            delete pk;
            numDeleted++;
            return;
        }
    }

    if (pk->hasPar("destAddr"))
    {
        int moduleId = (int)pk->par("destAddr");
        if (moduleId == myId)
        {
            EV << "Received packet: " << UDPSocket::getReceivedPacketInfo(pk) << endl;
            emit(rcvdPkSignal, pk);
            numReceived++;
            delete pk;
            return;
        }
    }
    else
    {
        EV << "Received packet: " << UDPSocket::getReceivedPacketInfo(pk) << endl;
        emit(rcvdPkSignal, pk);
        numReceived++;
    }

    UDPDataIndication *ctrl = check_and_cast<UDPDataIndication *>(pk->removeControlInfo());
    if (ctrl->getDestAddr().toIPv4() == IPv4Address::ALLONES_ADDRESS && par("flooding").boolValue())
    {
        numFlood++;
        emit(floodPkSignal, pk);
        scheduleAt(simTime()+par("delay").doubleValue(),pk);
    }
    else
        delete pk;
    delete ctrl;

}

void UDPBasicFlooding::generateBurst()
{
    simtime_t now = simTime();

    if (nextPkt < now)
        nextPkt = now;

    double sendInterval = sendIntervalPar->doubleValue();
    if (sendInterval <= 0.0)
        throw cRuntimeError("The sendInterval parameter must be bigger than 0");
    nextPkt += sendInterval;

    if (activeBurst && nextBurst <= now) // new burst
    {
        double burstDuration = burstDurationPar->doubleValue();
        if (burstDuration < 0.0)
            throw cRuntimeError("The burstDuration parameter mustn't be smaller than 0");
        double sleepDuration = sleepDurationPar->doubleValue();

        if (burstDuration == 0.0)
            activeBurst = false;
        else
        {
            if (sleepDuration < 0.0)
                throw cRuntimeError("The sleepDuration parameter mustn't be smaller than 0");
            nextSleep = now + burstDuration;
            nextBurst = nextSleep + sleepDuration;
        }
    }

    L3Address destAddr(IPv4Address::ALLONES_ADDRESS);

    cPacket *payload = createPacket();
    payload->setTimestamp();
    emit(sentPkSignal, payload);

    // Check address type
    sendBroadcast(destAddr, payload);

    numSent++;

    // Next timer
    if (activeBurst && nextPkt >= nextSleep)
        nextPkt = nextBurst;

    scheduleAt(nextPkt, timerNext);
}

void UDPBasicFlooding::finish()
{
    recordScalar("Total sent", numSent);
    recordScalar("Total received", numReceived);
    recordScalar("Total deleted", numDeleted);
}

bool UDPBasicFlooding::sendBroadcast(const L3Address &dest, cPacket *pkt)
{
    if (!outputInterfaceMulticastBroadcast.empty() && (dest.isMulticast() || (dest.getType() != L3Address::IPv6 && dest.toIPv4() == IPv4Address::ALLONES_ADDRESS)))
    {
        for (unsigned int i = 0; i < outputInterfaceMulticastBroadcast.size(); i++)
        {
            UDPSocket::SendOptions options;
            options.outInterfaceId = outputInterfaceMulticastBroadcast[i];
            if (outputInterfaceMulticastBroadcast.size() - i > 1)
                socket.sendTo(pkt->dup(), dest, destPort, &options);
            else
                socket.sendTo(pkt, dest, destPort, &options);
        }
        return true;
    }
    return false;
}


bool UDPBasicFlooding::handleNodeStart(IDoneCallback *doneCallback)
{
    simtime_t start = std::max(startTime, simTime());

    if ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime))
    {
        if (isSource)
        {
            activeBurst = true;
            timerNext = new cMessage("UDPBasicFloodingTimer");
            scheduleAt(startTime, timerNext);
        }

        if (strcmp(par("destAddresses").stringValue(),"") != 0)
        {
            if (addressModule == nullptr)
            {
                addressModule = new AddressModule();
                addressModule->initModule(true);
            }
        }
    }

    return true;
}

bool UDPBasicFlooding::handleNodeShutdown(IDoneCallback *doneCallback)
{
    if (timerNext)
        cancelEvent(timerNext);
    activeBurst = false;
    //TODO if(socket.isOpened()) socket.close();
    return true;
}

void UDPBasicFlooding::handleNodeCrash()
{
    if (timerNext)
        cancelEvent(timerNext);
    activeBurst = false;
}

}

