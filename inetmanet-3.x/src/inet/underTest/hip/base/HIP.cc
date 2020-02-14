//*********************************************************************************
// File:           HIP.cc
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

#include "inet/underTest/hip/base/HIP.h"
#include "inet/networklayer/contract/ipv6/IPv6ControlInfo.h"
#include "inet/networklayer/ipv6/IPv6ExtensionHeaders_m.h"
#include "inet/networklayer/ipv6/IPv6Datagram.h"
#include "inet/underTest/hip/application/DNSBaseMsg_m.h"
#include "inet/underTest/hip/application/DNSRegRvsMsg_m.h"
#include "inet/transportlayer/udp/UDPPacket.h"
#include "inet/transportlayer/contract/udp/UDPControlInfo_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/InterfaceTable.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/networklayer/ipv6/IPv6InterfaceData.h"
#include "inet/networklayer/common/IPSocket.h"
#include "inet/common/ModuleAccess.h"

namespace inet {

Define_Module(HIP)

HIP::HIP()
{
}

// Default destructor
HIP::~HIP()
{
    if (!listHITtoTriggerDNS.empty())
        for (listHITtoTriggerDNSIt = listHITtoTriggerDNS.begin(); listHITtoTriggerDNSIt != listHITtoTriggerDNS.end();
                listHITtoTriggerDNSIt++)
            delete listHITtoTriggerDNSIt->second;
}


// Initializing the module's basic parameters
void HIP::initialize(int stage)
{
    if (stage==INITSTAGE_LOCAL)
    {
        EV << "Initializing HIP...\n";
        expectingDnsResp = false;
        expectingRvsDnsResp = false;
        fsmType = cModuleType::get("inet.underTest.hip.base.HipFsm");
        hipMsgSent = 0;
        hipVector.setName("HIP_DNS msgs");
        currentIfId = -1;
        udpPresent = false;
        tcpPresent = false;

        WATCH_MAP(mapIfaceToIP);
        WATCH_MAP(mapIfaceToConnected);
        WATCH_MAP(hitToIpMap);
        WATCH(firstUpdate);
        WATCH(expectingDnsResp);
        WATCH(expectingRvsDnsResp);
        WATCH(currentIfId);

        specInitialize();
    }
    else if (stage==INITSTAGE_LINK_LAYER_2)
    {
        cModule *host = getContainingNode(this);
        host->subscribe(NF_L2_DISASSOCIATED, this); // NF_L2_BEACON_LOST
        host->subscribe(NF_L2_ASSOCIATED_OLDAP, this);
        host->subscribe(NF_L2_ASSOCIATED_NEWAP, this);
        host->subscribe(NF_IPv6_HANDOVER_OCCURRED,this);
        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    }
}

void HIP::specInitialize()
{
    _isRvs = false;
    //partnerHIT = -1; //will get from DNS
    if ((int) this->par("registerAtRVS") == 1)
    {
        EV << "hip init3\n";
        EV << "k" << this->par("RVSAddr").getName() << "k" << endl;
        //if rvsipaddress.. get rvsip from dns... scheduleAt
        scheduleAt(this->par("REG_StartTime"),new cPacket("HIP_REGISTER_AT_RVS"));
    }

    firstUpdate = true;
}

// Changes in interface states and addresses handled here with the help of the NotificationBoard object
//void HIP::receiveChangeNotification(int category, const cObject * details)
void HIP::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    Enter_Method_Silent();

    // printNotificationBanner(category, details);

    // OLD AP, or disassociating, update is needed
    if (signalID == NF_L2_DISASSOCIATED || signalID == NF_L2_ASSOCIATED_OLDAP)
    {
        InterfaceEntry *ie = dynamic_cast<InterfaceEntry *>(const_cast<cObject*>(obj));
        if (!(ie->isLoopback()) && (ie->isUp()))
            return;
        if (signalID == NF_L2_DISASSOCIATED)
        {
            mapIfaceToConnected[ie] = false;
            for (auto it = mapIfaceToConnected.begin(); it != mapIfaceToConnected.end(); ++it)
            {
                InterfaceEntry *ie2 = it->first;
                if (!(ie2->isLoopback()) && (ie2->isUp()) && it->second == true)
                {
                    handleAddressChange();
                    return;
                }
            }
        }
        else
        {
            mapIfaceToConnected[ie] = true;
            handleAddressChange();
            return;
        }
    }
    // Iface is connected to a new AP, wait for address change and then update
    else if (signalID == NF_L2_ASSOCIATED_NEWAP)
    {
        InterfaceEntry *ie = dynamic_cast<InterfaceEntry *>(const_cast<cObject*>(obj));
        if (!(ie->isLoopback()) && (ie->isUp()))
            mapIfaceToConnected[ie] = true;
    }
    else if (signalID == NF_IPv6_HANDOVER_OCCURRED)
    {

        for (auto it = mapIfaceToConnected.begin(); it != mapIfaceToConnected.end(); ++it)
        {
            InterfaceEntry *ie = it->first;
            if (!(ie->isLoopback()) && (ie->isUp()) && it->second == true)
            {
                if (mapIfaceToIP.find(ie) == mapIfaceToIP.end()
                        || mapIfaceToIP[ie] != ie->ipv6Data()->getPreferredAddress())
                {
                    mapIfaceToIP[ie] = ie->ipv6Data()->getPreferredAddress();
                    handleAddressChange();
                    return;
                }
            }
        }
    }
}

// RVS registration's first DNS lookup handled here
void HIP::handleRvsRegistration(cMessage *msg)
{
    if (msg->isName("HIP_REGISTER_AT_RVS"))
    {
        delete msg;
        EV << "rvs selfmsg arrived\n";
        if (rvsIPaddress.isUnspecified())
        {

            EV << "Starting dns request for " << par("RVSAddr").getName() << endl;

            // Generating the DNS message
            DNSBaseMsg* dnsReqMsg = new DNSBaseMsg("DNS Request");//the request
            dnsReqMsg->setId(this->getId());
            char reqstring[50] = "hip-";
            strcat(reqstring, par("RVSAddr"));
            dnsReqMsg->setData(reqstring);
            int tempIfId = currentIfId;
            if (currentIfId == -1)
            {
                tempIfId = getTempId();
                if (tempIfId == -1)
                    tempIfId = currentIfId;
                //throw cRuntimeError("currentIfId == -1");
            }
#if 0
            //UDP controlinfo
            UDPDataIndication *ctrl = new UDPDataIndication();
            ctrl->setSrcPort(10500);

            ctrl->setSrcAddr(ift->getInterfaceById(tempIfId)->ipv6Data()->getPreferredAddress());
            ctrl->setDestAddr(L3AddressResolver().resolve(par("dnsAddress")));
            ctrl->setDestPort(23);
            ctrl->setInterfaceId(tempIfId);

            dnsReqMsg->setControlInfo(ctrl);

            UDPPacket *udpPacket = new UDPPacket(dnsReqMsg->getName());
            //TODO UDP_HEADER_BYTES
            udpPacket->setByteLength(8);
            udpPacket->encapsulate(dnsReqMsg);

            // set source and destination port
            udpPacket->setSourcePort(ctrl->getSrcPort());
            udpPacket->setDestinationPort(ctrl->getDestPort());

            IPv6ControlInfo *ipControlInfo = new IPv6ControlInfo();
            ipControlInfo->setProtocol(IP_PROT_UDP);
            ipControlInfo->setSrcAddr(ctrl->getSrcAddr().toIPv6() ());
            ipControlInfo->setDestAddr(ctrl->getDestAddr().toIPv6() ());
            ipControlInfo->setInterfaceId(tempIfId);
            udpPacket->setControlInfo(ipControlInfo);
#else
            UDPPacket *udpPacket = new UDPPacket(dnsReqMsg->getName());
            //TODO UDP_HEADER_BYTES
            udpPacket->setByteLength(8);
            udpPacket->encapsulate(dnsReqMsg);

            // set source and destination port
            udpPacket->setSourcePort(10500);
            udpPacket->setDestinationPort(23);

            IPv6ControlInfo *ipControlInfo = new IPv6ControlInfo();
            ipControlInfo->setProtocol(IP_PROT_UDP);
            ipControlInfo->setSrcAddr(ift->getInterfaceById(tempIfId)->ipv6Data()->getPreferredAddress());
            ipControlInfo->setDestAddr(L3AddressResolver().resolve(par("dnsAddress")).toIPv6());
            ipControlInfo->setInterfaceId(tempIfId);
            udpPacket->setControlInfo(ipControlInfo);
#endif

            send(udpPacket,"udp6Out");

            hipVector.record(1);
            expectingRvsDnsResp = true;
        }
        else
        {
            sendDirect(msg,createStateMachine(rvsIPaddress, _rvsHit), "localIn");
        }
    }
    else if(msg->isName("DNS Response"))
    {
        // Handling the DNS response
        IPv6ControlInfo *networkControlInfo = check_and_cast<IPv6ControlInfo*>(msg->removeControlInfo());
        srcWorkAddress = networkControlInfo->getDestAddr();
        //set rvs ip, rvs HIT
        EV << "Getting partner HIT from dns resp \n";
        DNSBaseMsg* dnsMsg = check_and_cast<DNSBaseMsg *>(check_and_cast<cPacket *>(msg)->decapsulate());
        _rvsHit.set(dnsMsg->data());
        rvsIPaddress = dnsMsg->addrData().toIPv6();
        EV << _rvsHit << endl;
        delete msg;
        delete dnsMsg;
        expectingRvsDnsResp = false;
        sendDirect(new cPacket("HIP_REGISTER_AT_RVS"), createStateMachine(rvsIPaddress, _rvsHit), "localIn");
    }
}

//send the message to specific handle functions
void HIP::handleMessage(cMessage *msg)
{
    //**********REGISTER AT RVS********
    if (msg->isSelfMessage() && msg->isName("HIP_REGISTER_AT_RVS"))
    {
        handleRvsRegistration(msg);
    }
    //************DNS***********
    else if (msg->arrivedOn("udpIn") && strcmp(msg->getName(), "DNS Request") == 0)
    {
        expectingDnsResp = true;
        send(msg, "udp6Out");
    }
    else if (msg->arrivedOn("udp6In") && strcmp(msg->getName(), "DNS Response") == 0)
    {
        if (expectingDnsResp || expectingRvsDnsResp)
        {
            if (expectingDnsResp)
            {

                DNSBaseMsg* dnsMsg = check_and_cast<DNSBaseMsg *>(check_and_cast<cPacket *>(msg)->decapsulate());
                partnerHIT.set(dnsMsg->data());
                if (listHITtoTriggerDNS.count(partnerHIT) > 0)
                {
                    EV << "DNS response recieved, starting FSM\n";
                    // if (partnerHIT.isUnspecified()) partnerHIT.set(this->par("PARTNER_HIT"));
                    sendDirect(listHITtoTriggerDNS.find(partnerHIT)->second, createStateMachine( dnsMsg->addrData().toIPv6(), partnerHIT), "localIn");
                    listHITtoTriggerDNS.erase(partnerHIT);
                    if(listHITtoTriggerDNS.empty())
                    expectingDnsResp = false;
                }
                delete msg;
                delete dnsMsg;

            }
            if (expectingRvsDnsResp)
                handleRvsRegistration(msg);
        }
        else
        {
            EV << "DNS response when not expected, dropping...\n";
            delete msg;
        }
    }

    //***********************"normal message"
    else
    {
        //msg from transport layer
        if(msg->arrivedOn("tcpIn") || msg->arrivedOn("udpIn"))
        {
            handleMsgFromTransport(msg);
        //msg from hip daemon to transport
        }
        else if (msg->arrivedOn("fromFsmIn"))
        {
            IPv6ControlInfo *cinfo = check_and_cast<IPv6ControlInfo *>(msg->getControlInfo());
            if (cinfo->getProtocol() != IP_PROT_UDP)
            {
                if (tcpPresent)
                    send(msg, "tcpOut");
                else
                    delete msg;
            }
            else
            {

                if (udpPresent)
                    send(msg, "udpOut");
                else
                    delete msg;
            }
        //msg from hip daemon to network
        }
        else if(msg->arrivedOn("fromFsmOut"))
        {
            IPv6ControlInfo *cinfo = check_and_cast<IPv6ControlInfo *>(msg->getControlInfo());
            if (cinfo->getProtocol() != IP_PROT_UDP)
                send(msg, "tcp6Out");
            else
                send(msg, "udp6Out");
        //msg from network
        }
        else if (msg->arrivedOn("tcp6In") || msg->arrivedOn("udp6In"))
        {
            EV << "handleMsgFromNetwork invoke\n";
            handleMsgFromNetwork(msg);
        }
        else if(msg->arrivedOn("fromFsmTrigger")) //process previously received message which was trigger
        {
            handleMsgFromTransport(msg);
        }
    }
}

// Handles message from transport layer
void HIP::handleMsgFromTransport(cMessage *msg)
{
    IPRegisterProtocolCommand * regProt = dynamic_cast<IPRegisterProtocolCommand*> (msg->getControlInfo());

    if (regProt)
    {
        if (regProt->getProtocol() == IP_PROT_UDP)
        {
            IPSocket ipSocket(gate("udp6Out"));
            ipSocket.registerProtocol(IP_PROT_UDP);
            udpPresent = true;
        }
        else if (regProt->getProtocol() == IP_PROT_TCP)
        {
            IPSocket ipSocket(gate("tcp6Out"));
            ipSocket.registerProtocol(IP_PROT_TCP);
            tcpPresent = true;
        }
        delete msg;
        return;
    }


    IPv6ControlInfo *networkControlInfo = check_and_cast<IPv6ControlInfo*>(msg->removeControlInfo());

    //if the destination address is in the IP-HIT mapping
    bool exists = false;

    //try to receive HIT from dest address
    EV << "Registered dest addr (HIT): " << networkControlInfo->getDestAddr() << endl;;
    //original destination address
    IPv6Address originalHIT = networkControlInfo->getDestAddr();

    //if hip association already exists, find its hip daemon and forward to it
    for (hitToIpMapIt = hitToIpMap.begin(); hitToIpMapIt != hitToIpMap.end(); hitToIpMapIt++)
    {
        if (originalHIT == hitToIpMapIt->first)
        {
            EV << "Existing HIP association found...sending msg to it....FROM_TRANSPORT\n";
            msg->setControlInfo(networkControlInfo);
            sendDirect(msg, findStateMachine(hitToIpMapIt->second->fsmId),"localIn");
            exists = true;
            break;
        }
    }
    //if not exists start the DNS sequence if it is not started before
    if (!exists && listHITtoTriggerDNS.count(originalHIT) == 0)
    {
        // Need IP address for target HIT
        // Sending DNS
        EV << "Host not associated, need IP address, sending out DNS...\n";
        msg->setControlInfo(networkControlInfo);
        listHITtoTriggerDNS[originalHIT] = msg;

        DNSBaseMsg* dnsReqMsg = new DNSBaseMsg("DNS Request"); //the request
        dnsReqMsg->setId(this->getId());
        dnsReqMsg->setAddrData(originalHIT);
        int tempId = currentIfId;
        if (currentIfId == -1)
        {
            tempId = getTempId();
            //throw cRuntimeError("currentIfId == -1");
        }
#if 0
        //UDP controlinfo
        UDPDataIndication *ctrl = new UDPDataIndication();
        ctrl->setSrcPort(10500);

        ctrl->setSrcAddr(ift->getInterfaceById(tempId)->ipv6Data()->getPreferredAddress());
        ctrl->setDestAddr(L3AddressResolver().resolve(par("dnsAddress")));
        ctrl->setDestPort(23);
        ctrl->setInterfaceId(tempId);
        //dnsReqMsg->setControlInfo(ctrl);

        UDPPacket *udpPacket = new UDPPacket(dnsReqMsg->getName());
        //TODO UDP_HEADER_BYTES
        udpPacket->setByteLength(8);
        udpPacket->encapsulate(dnsReqMsg);

        // set source and destination port
        udpPacket->setSourcePort(ctrl->getSrcPort());
        udpPacket->setDestinationPort(ctrl->getDestPort());

        IPv6ControlInfo *ipControlInfo = new IPv6ControlInfo();
        ipControlInfo->setProtocol(IP_PROT_UDP);
        ipControlInfo->setSrcAddr(ctrl->getSrcAddr().toIPv6() ());
        ipControlInfo->setDestAddr(ctrl->getDestAddr().toIPv6() ());
        ipControlInfo->setInterfaceId(tempId);//FIXME extend IPv6 with this!!!
        udpPacket->setControlInfo(ipControlInfo);
#else
        //UDP controlinfo
        UDPPacket *udpPacket = new UDPPacket(dnsReqMsg->getName());
        //TODO UDP_HEADER_BYTES
        udpPacket->setByteLength(8);
        udpPacket->encapsulate(dnsReqMsg);

        // set source and destination port
        udpPacket->setSourcePort(10500);
        udpPacket->setDestinationPort(23);

        IPv6ControlInfo *ipControlInfo = new IPv6ControlInfo();
        ipControlInfo->setProtocol(IP_PROT_UDP);
        ipControlInfo->setSrcAddr(ift->getInterfaceById(tempId)->ipv6Data()->getPreferredAddress());
        ipControlInfo->setDestAddr(L3AddressResolver().resolve(par("dnsAddress")).toIPv6());
        ipControlInfo->setInterfaceId(tempId);//FIXME extend IPv6 with this!!!
        udpPacket->setControlInfo(ipControlInfo);
#endif

        send(udpPacket,"udp6Out");
        hipVector.record(1);
        expectingDnsResp = true;
    }
}

// Handle message from ip(v6) layer
void HIP::handleMsgFromNetwork(cMessage *msg)
{

    IPv6ControlInfo *networkControlInfo = check_and_cast<IPv6ControlInfo*>(msg->getControlInfo());

    if (dynamic_cast<HIPHeaderMessage *>(msg))
    {
        HIPHeaderMessage *hipHeader = check_and_cast<HIPHeaderMessage *>(msg);
        EV << "Its a hip header\n";
        //if dest locator's SPI is unknown -it is an I1 msg
        int fsmId = -1;
        if (hitToIpMap.find(hipHeader->getSrcHIT()) != hitToIpMap.end())
            fsmId = hitToIpMap.find(hipHeader->getSrcHIT())->second->fsmId;
        if (fsmId < 0)
        {
            IPv6Address realSrc;
            //check if its via rvs
            if (!hipHeader->getFrom_i().isUnspecified())
            {
                realSrc = hipHeader->getFrom_i();
            }
            else
            {
                realSrc = networkControlInfo->getSrcAddr();
            }
            //create new daemon and fw to it

            sendDirect(msg, createStateMachine(realSrc, hipHeader->getSrcHIT()), "remoteIn");
        }
        else
        {
            //spi is known, find daemon and fw to it
            sendDirect(msg, findStateMachine(fsmId), "remoteIn");
        }
    }
    //msg without hipheader, is it ESP?
    else
    {
        //msg->removeget);
        ESPMessage *espHeader = check_and_cast<ESPMessage *>(msg);
        //TODO check if esp header exists
        //which spi?
        EV << "Got esp head: " << espHeader->getEsp()->getSpi() << endl;
        int spi = espHeader->getEsp()->getSpi();
        //decapsulating the msg
        //cPacket *transportPacket = msg->decapsulate();
        //transportPacket->setnetworkControlInfo);

        sendDirect(msg, findStateMachine(spi), "remoteIn");
        //delete msg;
    }
}

// If an address is changed on an interface
// initiate an update mechanism on the fsms
void HIP::handleAddressChange()
{

    for (int i = 0; i < ift->getNumInterfaces(); i++)
    {
        InterfaceEntry *ie = ift->getInterface(i);

        if (!(ie->isLoopback()) && (ie->isUp()))
        {
            if (mapIfaceToConnected.find(ie) != mapIfaceToConnected.end() && mapIfaceToConnected[ie] == true)
            {
                currentIfId = ie->getInterfaceId(); //WTF?
                break;
            }
        }
    }
    for (hitToIpMapIt = hitToIpMap.begin(); hitToIpMapIt != hitToIpMap.end(); hitToIpMapIt++)
        sendDirect(new cPacket("ADDRESS_CHANGED"), findStateMachine(hitToIpMapIt->second->fsmId), "HIPinfo");
}

int HIP::getTempId()
{

    int numIfaces = 0;
    int ifaceId = -1;
    for (int i = 0; i < ift->getNumInterfaces(); i++)
    {
        InterfaceEntry *ie = ift->getInterface(i);

        if (!(ie->isLoopback()) && (ie->isUp()))
        {
            numIfaces++;
            ifaceId = ie->getInterfaceId();
            if (mapIfaceToConnected.find(ie) != mapIfaceToConnected.end() && mapIfaceToConnected[ie] == true)
            {
                currentIfId = ie->getInterfaceId(); //WTF?
                break;
            }
        }
    }
    if (currentIfId == -1)
    {
        for (int i = 0; i < ift->getNumInterfaces(); i++)
        {
            InterfaceEntry *ie = ift->getInterface(i);

            if (!(ie->isLoopback()) && (ie->isUp()))
                return ifaceId = ie->getInterfaceId();
        }
    }
    return -1;
}

// Creates a new FSM and returns its pointer
cModule* HIP::createStateMachine(IPv6Address ipAddress, IPv6Address &HIT)
{
    cModule *newFsm = fsmType->createScheduleInit("HipFsm", this);
    hitToIpMapEntryWork = new HitToIpMapEntry();

    hitToIpMapEntryWork->addr.push_front(ipAddress);
    hitToIpMapEntryWork->fsmId = newFsm->getId();
    hitToIpMap[HIT] = hitToIpMapEntryWork;

    return newFsm;
}

// Returns a pointer to the FSM with the specified ID
cModule* HIP::findStateMachine(int fsmID)
{
    cModule *fsm = getSimulation()->getModule(fsmID);
    return fsm;
}

// Returns if the node is an RVS
bool HIP::isRvs()
{
    return _isRvs;
}

// Returns the nodes assigned RVS's HIT
IPv6Address* HIP::getRvsAddress()
{
    return &rvsIPaddress;
}

// The most recent DNS lookup's address (needed by FSM) TODO: Nicer...
IPv6Address* HIP::getSrcWorkAddress()
{
    return &srcWorkAddress;
}

// Returns the nodes assigned RVS's HIT
IPv6Address* HIP::getRvsHit()
{
    return &_rvsHit;
}

IPv6Address HIP::getPartnerHIT()
{
    return partnerHIT;
}
;

// Counts the HIP messages sent
void HIP::incHipMsgCounter()
{
    hipMsgSent++;
    hipVector.record(1);
}

// Outputs the statistics gathered
void HIP::finish()
{
    recordScalar("HIP msg counter", hipMsgSent);
}

}
