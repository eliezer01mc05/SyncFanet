/*
 * Copyright (c) 2008,2009 IITP RAS
 * Copyright (c) 2011 Universidad de M�laga
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 * Author: Alfonso Ariza <aarizaq@uma.es>
 */

#include "inet/linklayer/ieee80211mesh/hwmp/hwmp.h"
#include "inet/linklayer/ieee80211/mgmt/Ieee80211Etx.h"
#include "inet/routing/extras/base/ControlManetRouting_m.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211ControlInfo_m.h"
#include "inet/linklayer/common/Ieee802Ctrl.h"
#include "inet/linklayer/ieee80211/mgmt/Ieee80211MgmtFrames_m.h"

#define delay uniform(0.0,0.01)

namespace inet {

namespace ieee80211 {

using namespace physicallayer;

std::ostream& operator<<(std::ostream& os, const HwmpRtable::ReactiveRoute& e)
{
    if (e.whenExpire < simTime() || e.retransmitter.isUnspecified())
        os << "Invalid" << "\n";
    else
        os << "Valid" << "\n";
    os << " Next hop " << e.retransmitter << "\n";
    os << "Interface " << e.interface << "\n";
    os << "metric 0x" << std::hex << e.metric << "\n";
    os << "hops " << (int) e.hops << "\n";
    os << "whenExpire " << e.whenExpire << "\n";
    os << "seqnum " << e.seqnum << "\n";
    return os;
}


std::ostream& operator<<(std::ostream& os, const HwmpRtable::ProactiveRoute& e)
{
    if (e.whenExpire < simTime() || e.root.isUnspecified())
        os << "Invalid" << "\n";
    else
        os << "Valid" << "\n";
    os << " Root " << e.root << "\n";
    os << " Next hop " << e.retransmitter << "\n";
    os << "Interface " << e.interface << "\n";
    os << "metric 0x" << std::hex << e.metric << "\n";
    os << "hops " << (int) e.hops << "\n";
    os << "whenExpire " << e.whenExpire << "\n";
    os << "seqnum " << e.seqnum << "\n";
    return os;
}


Define_Module(HwmpProtocol);

void ProactivePreqTimer::expire()
{
    EV << "expire " << endl;
    HwmpProtocol* hwmpProtocol = dynamic_cast<HwmpProtocol*>(agent_);
    if (hwmpProtocol == nullptr)
        throw cRuntimeError("agent not valid");
    hwmpProtocol->sendPreqProactive();
}

void PreqTimeout::expire()
{
    EV << "expire2 " << endl;
    HwmpProtocol* hwmpProtocol = dynamic_cast<HwmpProtocol*>(agent_);
    if (hwmpProtocol == nullptr)
        throw cRuntimeError("agent not valid");
    hwmpProtocol->retryPathDiscovery(dest);
}

PreqTimeout::PreqTimeout(MACAddress d, ManetRoutingBase* agent) :
        ManetTimer(agent)
{
    dest = d;
}

PreqTimeout::PreqTimeout(MACAddress d) :
        ManetTimer()
{
    dest = d;
}

void PreqTimer::expire()
{
    EV << "expire3 " << endl;
    HwmpProtocol* hwmpProtocol = dynamic_cast<HwmpProtocol*>(agent_);
    if (hwmpProtocol == nullptr)
        throw cRuntimeError("agent not valid");
    hwmpProtocol->sendMyPreq();
}

void PerrTimer::expire()
{
    EV << "expire4 " << endl;
    HwmpProtocol* hwmpProtocol = dynamic_cast<HwmpProtocol*>(agent_);
    if (hwmpProtocol == nullptr)
        throw cRuntimeError("agent not valid");
    hwmpProtocol->sendMyPerr();
}

void GannTimer::expire()
{
    EV << "expire5 " << endl;
    HwmpProtocol* hwmpProtocol = dynamic_cast<HwmpProtocol*>(agent_);
    if (hwmpProtocol == nullptr)
        throw cRuntimeError("agent not valid");
    hwmpProtocol->sendGann();
}

HwmpProtocol::HwmpProtocol()
{    EV << "hwmp " << endl;
    m_preqTimer = nullptr;
    m_perrTimer = nullptr;
    m_proactivePreqTimer = nullptr;
    m_rtable = nullptr;
    m_gannTimer = nullptr;
    neighborMap.clear();
    useEtxProc = false;
    m_isGann = false;
    ganVector.clear();
}

HwmpProtocol::~HwmpProtocol()
{
    EV << "~hmwp" << endl;
    delete m_proactivePreqTimer;
    delete m_preqTimer;
    delete m_perrTimer;
    delete m_rtable;
    delete m_gannTimer;
    neighborMap.clear();
    ganVector.clear();
    while (!m_rqueue.empty())
    {
        delete m_rqueue.back().pkt;
        m_rqueue.pop_back();
    }
}

void HwmpProtocol::initialize(int stage)
{
    if (stage == INITSTAGE_ROUTING_PROTOCOLS)
    {
        createTimerQueue();
        registerRoutingModule();
        gannSeqNumber = 0;
        m_hwmpSeqno = 0;
        m_preqId = 0;
        m_maxQueueSize = par("maxQueueSize");
        m_dot11MeshHWMPmaxPREQretries = par("dot11MeshHWMPmaxPREQretries");
        m_dot11MeshHWMPnetDiameterTraversalTime = par("dot11MeshHWMPnetDiameterTraversalTime");
        m_dot11MeshHWMPpreqMinInterval = par("dot11MeshHWMPpreqMinInterval");
        m_dot11MeshHWMPperrMinInterval = par("dot11MeshHWMPperrMinInterval");
        m_dot11MeshHWMPactiveRootTimeout = par("dot11MeshHWMPactiveRootTimeout");
        m_dot11MeshHWMPactivePathTimeout = par("dot11MeshHWMPactivePathTimeout");
        m_dot11MeshHWMPpathToRootInterval = par("dot11MeshHWMPpathToRootInterval");
        m_dot11MeshHWMPrannInterval = par("dot11MeshHWMPrannInterval");
        m_isRoot = par("isRoot");
        m_maxTtl = par("maxTtl");
        m_unicastPerrThreshold = par("unicastPerrThreshold");
        m_unicastPreqThreshold = par("unicastPreqThreshold");
        m_unicastDataThreshold = par("unicastDataThreshold");
        m_ToFlag = par("ToFlag");
        m_concurrentReactive = par("concurrentReactive");
        m_preqTimer = new PreqTimer(this);
        m_perrTimer = new PerrTimer(this);
        m_proactivePreqTimer = new ProactivePreqTimer(this);
        m_gannTimer = new GannTimer(this);
        m_rtable = new HwmpRtable();
        timeLimitQueue = par("timeLimitInQueue");

        propagateProactive = par("propagateProactive").boolValue();
        if (isRoot())
            setRoot();

        m_dot11MeshHWMPGannInterval = par("dot11MeshHWMPgannInterval");
        m_isGann = par("isGan");
        if (m_isGann)
        {
            double randomStart = par("randomStart");
            m_gannTimer->resched(randomStart);
        }

        WATCH_MAP(m_rtable->m_routes);
        WATCH(m_rtable->m_root);
        WATCH(nextProactive);
        Ieee80211Etx * etx = dynamic_cast<Ieee80211Etx *>(interface80211ptr->getEstimateCostProcess(0));
        if (etx == nullptr)
            useEtxProc=false;
        else
            useEtxProc=true;

        linkFullPromiscuous();
        linkLayerFeeback();
        scheduleEvent();
    }
}

void HwmpProtocol::handleMessage(cMessage *msg)
{    EV << "handlemessage " << endl;
    if (!isNodeOperational())
    {
        delete msg;
        return;
    }
    if (!checkTimer(msg))
        processData(msg);
    scheduleEvent();
}

void HwmpProtocol::processData(cMessage *msg)
{
    EV << "processdata " << endl;
    Ieee80211ActionMeshFrame * pkt = dynamic_cast<Ieee80211ActionMeshFrame*>(msg);
    if (pkt == nullptr)
    {
        Ieee80211MeshFrame * pkt = dynamic_cast<Ieee80211MeshFrame*>(msg);
        ControlManetRouting *ctrlmanet = dynamic_cast<ControlManetRouting*>(msg);
        if (pkt) // is data packet, enqueue and search new route
        {
            // seach route to destination
            // pkt->getAddress4(); has the destination address

            HwmpRtable::LookupResult result;
            L3Address apAddr;
            if (getAp(L3Address(pkt->getAddress4()),apAddr))
            {
                // search this address
                result = m_rtable->LookupReactive(MACAddress(apAddr.toMAC()));
            }
            else
            {
                result = m_rtable->LookupReactive(pkt->getAddress4());
            }
            HwmpRtable::LookupResult resultProact = m_rtable->LookupProactive();
            // Intermediate search
            if (hasPar("intermediateSeach") && !par("intermediateSeach").boolValue()
                    && !addressIsForUs(L3Address(pkt->getAddress3())))
            {
                if (result.retransmitter.isUnspecified() && resultProact.retransmitter.isUnspecified())
                {
                    EV<<"sendPERROR"<<endl;
                    // send perror
                    std::vector < HwmpFailedDestination > destinations;
                    HwmpFailedDestination dst;
                    dst.destination = pkt->getAddress4();
                    dst.seqnum = 0;

                    destinations.push_back(dst);
                    initiatePathError (makePathError(destinations));
                    delete msg;
                    return;
                }
            }
            simtime_t now = simTime();

            while (!m_rqueue.empty() && (now - m_rqueue.front().queueTime > timeLimitQueue))
            {
                delete m_rqueue.front().pkt;
                m_rqueue.pop_front();
            }

            if (m_rqueue.size() > m_maxQueueSize)
            {
                delete msg;
                return;
            }
            // enqueue and request route
            QueuedPacket qpkt;
            qpkt.pkt = pkt;
            qpkt.dst = pkt->getAddress4();
            qpkt.src = pkt->getAddress3();
            // Intermediate search
            if (pkt->getControlInfo())
            {
                Ieee802Ctrl *ctrl = check_and_cast<Ieee802Ctrl*>(pkt->removeControlInfo());
                qpkt.inInterface = ctrl->getInterfaceId();
                delete ctrl;
            }
            this->QueuePacket(qpkt);
//            HwmpRtable::LookupResult result = m_rtable->LookupReactive (qpkt.dst);
//            HwmpRtable::LookupResult resultProact = m_rtable->LookupProactive ();
            if (result.retransmitter.isUnspecified() && resultProact.retransmitter.isUnspecified())
            {
                EV<<"processdata if"<<endl;
                if (shouldSendPreq(qpkt.dst))
                {
                    GetNextHwmpSeqno();
                    uint32_t dst_seqno = 0;
                    m_stats.initiatedPreq++;
                    requestDestination(qpkt.dst, dst_seqno);
                }
            }
        }
        else if (ctrlmanet && ctrlmanet->getOptionCode() == MANET_ROUTE_NOROUTE)
        {

            MACAddress destination = ctrlmanet->getDestAddress().toMAC();
            HwmpRtable::LookupResult result = m_rtable->LookupReactive(destination);
            if (result.retransmitter.isUnspecified()) // address not valid
            {
                L3Address apAddr;
                if (getAp(ctrlmanet->getDestAddress(),apAddr))
                {
                    // search this address
                    result = m_rtable->LookupReactive(apAddr.toMAC());
                }
            }
            HwmpRtable::LookupResult resultProact = m_rtable->LookupProactive();
            if (result.retransmitter.isUnspecified() && resultProact.retransmitter.isUnspecified())
            {
                // send perror
                std::vector < HwmpFailedDestination > destinations;
                HwmpFailedDestination dst;
                dst.destination = pkt->getAddress4();
                dst.seqnum = 0;
                destinations.push_back(dst);
                initiatePathError (makePathError(destinations));
            }
            delete msg;
        }
        else
            delete msg; // in other case delete
        return;
    }

    if (pkt->getBody().getTTL() == 0)
    {
        delete msg;
        return;
    }

    pkt->getBody().setTTL(pkt->getBody().getTTL() - 1);
    pkt->getBody().setHopsCount(pkt->getBody().getHopsCount() + 1);

    switch (pkt->getBody().getId())
    {
        case IE11S_RANN:
            processRann(msg);
            break;
        case IE11S_PREQ:
            processPreq(msg);
            break;
        case IE11S_PREP:
            processPrep(msg);
            break;
        case IE11S_PERR:
            processPerr(msg);
            break;
        case IE11S_GANN:
            processGann(msg);
            break;
        default:
            throw cRuntimeError("");
            break;
    }

}

void HwmpProtocol::sendPreq(PREQElem preq, bool isProactive)
{
    EV << "sendpreq " << endl;
    std::vector < PREQElem > preq_vector;
    preq_vector.push_back(preq);
    sendPreq(preq_vector, isProactive);
}

void HwmpProtocol::sendPreq(std::vector<PREQElem> preq, bool isProactive)
{
    EV << "sendpreq2 " << endl;
    Ieee802Ctrl *ctrl = new Ieee802Ctrl();
    ctrl->setInterfaceId(interface80211ptr->getInterfaceId());
    std::vector < MACAddress > receivers = getPreqReceivers(interface80211ptr->getInterfaceId());
    if (receivers.size() == 1 && receivers[0] == MACAddress::BROADCAST_ADDRESS)
    {
        Ieee80211ActionPREQFrame* msg = createPReq(preq, false, MACAddress::BROADCAST_ADDRESS, isProactive);
        msg->setName("Hwmp::PREQ");
        for (int i = 1; i < getNumWlanInterfaces(); i++)
        {
            // It's necessary to duplicate the the control info message and include the information relative to the interface
            Ieee802Ctrl *ctrlAux = ctrl->dup();
            Ieee80211ActionPREQFrame *msgAux = msg->dup();
            // Set the control info to the duplicate packet
            ctrlAux->setInterfaceId(getWlanInterfaceEntry(i)->getInterfaceId());
            msgAux->setControlInfo(ctrlAux);
            if (msgAux->getBody().getTTL() == 0)
                delete msgAux;
            else
            {
                EV << "Sending preq frame to " << msgAux->getReceiverAddress() << endl;
                sendDelayed(msgAux, par("broadcastDelay"), "ipOut");
            }
        }
        ctrl->setInterfaceId(getWlanInterfaceEntry(0)->getInterfaceId());
        msg->setControlInfo(ctrl);
        if (msg->getBody().getTTL() == 0)
            delete msg;
        else
        {
            EV << "Sending preq frame to " << msg->getReceiverAddress() << endl;
            sendDelayed(msg, par("broadcastDelay"), "ipOut");
        }
    }
    else
    {
        for (unsigned int i = 0; i < receivers.size(); i++)
        {
            Ieee80211ActionPREQFrame* msg = createPReq(preq, true, receivers[i], isProactive);
            // It's necessary to duplicate the the control info message and include the information relative to the interface
            Ieee802Ctrl *ctrlAux = ctrl->dup();
            ctrlAux->setDest(receivers[i]);
            int index = getInterfaceReceiver(receivers[i]);
            InterfaceEntry *ie = nullptr;
            if (index >= 0)
                ie = getInterfaceEntryById(index);
            // Set the control info to the duplicate packet
            if (ie)
                ctrlAux->setInterfaceId(ie->getInterfaceId());
            msg->setControlInfo(ctrlAux);
            if (msg->getBody().getTTL() == 0)
                delete msg;
            else
            {
                EV << "Sending preq frame to " << msg->getReceiverAddress() << endl;
                sendDelayed(msg, par("unicastDelay"), "ipOut");
            }
        }
        delete ctrl;
    }
}

void HwmpProtocol::sendPrep(MACAddress src,
                            MACAddress targetAddr,
                            MACAddress retransmitter,
                            uint32_t initMetric,
                            uint32_t originatorSn,
                            uint32_t targetSn,
                            uint32_t lifetime,
                            uint32_t interface,
                            uint8_t hops,
                            bool proactive)
{
    EV << "sendprep" << endl;
    Ieee80211ActionPREPFrame* ieee80211ActionPrepFrame = new Ieee80211ActionPREPFrame();
    ieee80211ActionPrepFrame->setName("Hwmp::PREP");
    ieee80211ActionPrepFrame->getBody().setHopsCount(hops);
    ieee80211ActionPrepFrame->getBody().setTTL(GetMaxTtl());

    ieee80211ActionPrepFrame->getBody().setOriginator(src);
    ieee80211ActionPrepFrame->getBody().setOriginatorSeqNumber(originatorSn); // must be actualized previously
    ieee80211ActionPrepFrame->getBody().setLifeTime(GetActivePathLifetime());
    ieee80211ActionPrepFrame->getBody().setMetric(0);
    ieee80211ActionPrepFrame->getBody().setTarget(targetAddr);
    ieee80211ActionPrepFrame->getBody().setTargetSeqNumber(targetSn); // must be actualized previously

    ieee80211ActionPrepFrame->setReceiverAddress(retransmitter);
    ieee80211ActionPrepFrame->setTransmitterAddress(GetAddress());
    ieee80211ActionPrepFrame->setAddress3(ieee80211ActionPrepFrame->getTransmitterAddress());

    //Send Management frame
    m_stats.txPrep++;
    m_stats.txMgt++;
    m_stats.txBytes += ieee80211ActionPrepFrame->getByteLength();
    // It's necessary to duplicate the the control info message and include the information relative to the interface
    Ieee802Ctrl *ctrl = new Ieee802Ctrl;
    ctrl->setDest(retransmitter);

    int index = getInterfaceReceiver(retransmitter);
    // Set the control info to the duplicate packet
    InterfaceEntry *ie = nullptr;
    if (index >= 0)
        ie = getInterfaceEntryById(index);
    if (ie)
        ctrl->setInterfaceId(ie->getInterfaceId());
    ieee80211ActionPrepFrame->setControlInfo(ctrl);
    if (ieee80211ActionPrepFrame->getBody().getTTL() == 0)
        delete ieee80211ActionPrepFrame;
    else
    {
        EV << "Sending prep frame to " << ieee80211ActionPrepFrame->getReceiverAddress() << endl;
        if (proactive)
            sendDelayed(ieee80211ActionPrepFrame, par("broadcastDelay"), "ipOut");
        else
            sendDelayed(ieee80211ActionPrepFrame, par("unicastDelay"), "ipOut");
    }
    m_stats.initiatedPrep++;
}

void HwmpProtocol::sendPreqProactive()
{
    EV << "sendPreqProactive " << endl;
    if (!isRoot())
        return;
    PREQElem preq;
    preq.TO = true;
    preq.targetAddress = MACAddress::BROADCAST_ADDRESS;
    GetNextHwmpSeqno();
    sendPreq(preq, true);
    m_proactivePreqTimer->resched(m_dot11MeshHWMPpathToRootInterval);
    nextProactive = simTime()+m_dot11MeshHWMPpathToRootInterval;
}

void HwmpProtocol::sendGann()
{
    if (!isGann())
        return;
    Ieee80211ActionGANNFrame * gannFrame = new Ieee80211ActionGANNFrame();
    gannFrame->setName("Hwmp::GANN");
    gannFrame->getBody().setMeshGateAddress(GetAddress());
    gannFrame->getBody().setMeshGateSeqNumber(GetNextGannSeqno());
    m_gannTimer->resched(m_dot11MeshHWMPGannInterval);

    gannFrame->getBody().setHopsCount(0);
    gannFrame->getBody().setTTL(GetMaxTtl());
    gannFrame->setReceiverAddress(MACAddress::BROADCAST_ADDRESS);

    gannFrame->getBody().setMeshGateAddress(GetAddress());
    gannFrame->setTransmitterAddress(GetAddress());
    gannFrame->setAddress3(gannFrame->getTransmitterAddress());

    for (int i = 1; i < getNumWlanInterfaces(); i++)
    {
        // It's necessary to duplicate the the control info message and include the information relative to the interface
        Ieee802Ctrl *ctrl = new Ieee802Ctrl();
        Ieee80211ActionGANNFrame *msgAux = gannFrame->dup();
        // Set the control info to the duplicate packet
        ctrl->setInterfaceId(getWlanInterfaceEntry(i)->getInterfaceId());
        msgAux->setControlInfo(ctrl);
        if (msgAux->getBody().getTTL() == 0)
            delete msgAux;
        else
        {
            EV << "Sending gann frame " << endl;
            sendDelayed(msgAux, par("broadcastDelay"), "ipOut");
        }
    }
    Ieee802Ctrl *ctrl = new Ieee802Ctrl();
    ctrl->setInterfaceId(getWlanInterfaceEntry(0)->getInterfaceId());
    gannFrame->setControlInfo(ctrl);
    if (gannFrame->getBody().getTTL() == 0)
        delete gannFrame;
    else
    {
        EV << "Sending gann frame " << endl;
        sendDelayed(gannFrame, par("broadcastDelay"), "ipOut");
    }
}

Ieee80211ActionPREQFrame*
HwmpProtocol::createPReq(PREQElem preq, bool individual, MACAddress addr, bool isProactive)
{
    EV << "createpreq " << endl;
    std::vector < PREQElem > preqVec;
    preqVec.push_back(preq);
    return createPReq(preqVec, individual, addr, isProactive);
}

Ieee80211ActionPREQFrame*
HwmpProtocol::createPReq(std::vector<PREQElem> preq, bool individual, MACAddress addr, bool isProactive)
{
    EV << "createpreq2 " << endl;
    Ieee80211ActionPREQFrame* ieee80211ActionPreqFrame = new Ieee80211ActionPREQFrame();
    ieee80211ActionPreqFrame->setName("Hwmp::PREQ");
    if (isRoot())
        ieee80211ActionPreqFrame->getBody().setFlags(ieee80211ActionPreqFrame->getBody().getFlags() | 0x80);
    else
        ieee80211ActionPreqFrame->getBody().setFlags(ieee80211ActionPreqFrame->getBody().getFlags() & 0x7F);

    if (individual)
    {
        ieee80211ActionPreqFrame->getBody().setFlags(ieee80211ActionPreqFrame->getBody().getFlags() | 0x40);
        ieee80211ActionPreqFrame->setReceiverAddress(addr);
    }
    else
    {
        ieee80211ActionPreqFrame->setReceiverAddress(MACAddress::BROADCAST_ADDRESS);
        ieee80211ActionPreqFrame->getBody().setFlags(ieee80211ActionPreqFrame->getBody().getFlags() & 0xBF);
    }

    if (isProactive)
    {
        ieee80211ActionPreqFrame->getBody().setFlags(ieee80211ActionPreqFrame->getBody().getFlags() | 0x20);
        ieee80211ActionPreqFrame->getBody().setLifeTime(GetRootPathLifetime());
    }
    else
    {
        ieee80211ActionPreqFrame->getBody().setFlags(ieee80211ActionPreqFrame->getBody().getFlags() & 0xDF);
        ieee80211ActionPreqFrame->getBody().setLifeTime(GetActivePathLifetime());
    }

    ieee80211ActionPreqFrame->getBody().setHopsCount(0);
    ieee80211ActionPreqFrame->getBody().setTTL(GetMaxTtl());

    ieee80211ActionPreqFrame->getBody().setPreqElemArraySize(preq.size());
    ieee80211ActionPreqFrame->getBody().setPathDiscoveryId(GetNextPreqId());
    ieee80211ActionPreqFrame->getBody().setOriginator(GetAddress());
    ieee80211ActionPreqFrame->getBody().setOriginatorSeqNumber(m_hwmpSeqno); // must be actualized previously
    ieee80211ActionPreqFrame->getBody().setMetric(0);

    for (unsigned int i = 0; i < preq.size(); i++)
    {
        ieee80211ActionPreqFrame->getBody().setPreqElem(i, preq[i]);
    }
    ieee80211ActionPreqFrame->getBody().setTargetCount(ieee80211ActionPreqFrame->getBody().getPreqElemArraySize());

    ieee80211ActionPreqFrame->getBody().setBodyLength(ieee80211ActionPreqFrame->getBody().getBodyLength() + (preq.size() * PREQElemLen));
    ieee80211ActionPreqFrame->setByteLength(ieee80211ActionPreqFrame->getByteLength() + (preq.size() * PREQElemLen));

    ieee80211ActionPreqFrame->setTransmitterAddress(GetAddress());
    ieee80211ActionPreqFrame->setAddress3(ieee80211ActionPreqFrame->getTransmitterAddress());
    return ieee80211ActionPreqFrame;
}

void HwmpProtocol::requestDestination(MACAddress dst, uint32_t dst_seqno)
{
    EV << "requesdestination " << endl;
    L3Address apAddr;
    if (getAp(L3Address(dst), apAddr))
    {
        dst = apAddr.toMAC();
    }
    PREQElem preq;
    preq.targetAddress = dst;
    preq.targetSeqNumber = dst_seqno;
    preq.TO = GetToFlag();
    myPendingReq.push_back(preq);
    sendMyPreq();
}

void HwmpProtocol::sendMyPreq()
{
    EV << "sendmypreq " << endl;
    if (m_preqTimer->isScheduled())
        return;

    if (myPendingReq.empty())
        return;

    //reschedule sending PREQ
    m_preqTimer->resched((double) m_dot11MeshHWMPpreqMinInterval);
//    while (!myPendingReq.empty())
    {
        if (myPendingReq.size() > 20) // The maximum value of N is 20. 80211s
        {
            std::vector < PREQElem > vectorAux;
            vectorAux.assign(myPendingReq.begin(), myPendingReq.begin() + 20);
            sendPreq (vectorAux);
            myPendingReq.erase(myPendingReq.begin(), myPendingReq.begin() + 20);
        }
        else
        {
            sendPreq (myPendingReq);
            myPendingReq.clear();
        }
    }
}

bool HwmpProtocol::shouldSendPreq(MACAddress dst)
{
    EV << "shoudlsendpreq " << endl;
    L3Address apAddr;
    if (getAp(L3Address(dst), apAddr))
    {
        dst = apAddr.toMAC();
    }
    std::map<MACAddress, PreqEvent>::const_iterator i = m_preqTimeouts.find(dst);
    if (i == m_preqTimeouts.end())
    {
        m_preqTimeouts[dst].preqTimeout = new PreqTimeout(dst, this);
        m_preqTimeouts[dst].preqTimeout->resched(m_dot11MeshHWMPnetDiameterTraversalTime * 2.0);
        m_preqTimeouts[dst].whenScheduled = simTime();
        m_preqTimeouts[dst].numOfRetry = 1;
        return true;
    }
    return false;
}

void HwmpProtocol::retryPathDiscovery(MACAddress dst)
{
    EV << "retrypathdiscrvery" << endl;
    HwmpRtable::LookupResult result = m_rtable->LookupReactive(dst);
    L3Address apAddr;
    if (result.retransmitter.isUnspecified()) // address not valid
    {
        if (getAp(L3Address(dst),apAddr))
        {
            // search this address
            result = m_rtable->LookupReactive(apAddr.toMAC());
        }
    }
    HwmpRtable::LookupResult resultProact = m_rtable->LookupProactive();

    auto i = m_preqTimeouts.find(dst);
    ASSERT(i != m_preqTimeouts.end()); // always must be the preqTimeouts in the table
    if (!result.retransmitter.isUnspecified()) // address valid, don't retransmit
    {
        i->second.preqTimeout->removeTimer();
        delete i->second.preqTimeout;
        m_preqTimeouts.erase(i);
        return;
    }
    else if (!resultProact.retransmitter.isUnspecified() && !m_concurrentReactive) // address valid, don't retransmit
    {
        i->second.preqTimeout->removeTimer();
        delete i->second.preqTimeout;
        m_preqTimeouts.erase(i);
        return;
    }

    if (i->second.numOfRetry > m_dot11MeshHWMPmaxPREQretries)
    {
        QueuedPacket packet = dequeueFirstPacketByDst(dst);
        //purge queue and delete entry from retryDatabase
        while (packet.pkt != 0)
        {
            m_stats.totalDropped++;
            // what to do? packet.reply (false, packet.pkt, packet.src, packet.dst, packet.protocol, HwmpRtable::MAX_METRIC);
            delete packet.pkt;
            packet = dequeueFirstPacketByDst(dst);
        }
        m_preqTimeouts.erase(i);
        i->second.preqTimeout->removeTimer();
        delete i->second.preqTimeout;
        return;
    }
    /*
     if (i == m_preqTimeouts.end ())
     {
     m_preqTimeouts[dst].preqTimeout = new PreqTimeout(dst,this);
     m_preqTimeouts[dst].preqTimeout->resched(m_dot11MeshHWMPnetDiameterTraversalTime * 2.0);
     m_preqTimeouts[dst].whenScheduled = simTime();
     m_preqTimeouts[dst].numOfRetry = 0;
     }

     i  = m_preqTimeouts.find (dst);
     */
    i->second.numOfRetry++;
    GetNextHwmpSeqno(); // actualize the sequence number
    uint32_t dst_seqno;
    if (apAddr != L3Address())
        dst_seqno = m_rtable->LookupReactiveExpired(apAddr.toMAC()).seqnum;
    else
        dst_seqno = m_rtable->LookupReactiveExpired(dst).seqnum;
    requestDestination(dst, dst_seqno);
    i->second.preqTimeout->resched((2 * (i->second.numOfRetry + 1)) * m_dot11MeshHWMPnetDiameterTraversalTime);
}

void HwmpProtocol::sendPerr(std::vector<HwmpFailedDestination> failedDestinations, std::vector<MACAddress> receivers)
{
    EV << "sendPerr " << endl;
    Ieee80211ActionPERRFrame * ieee80211ActionPerrFrame = new Ieee80211ActionPERRFrame();
    ieee80211ActionPerrFrame->setName("Hwmp::PERR");
    ieee80211ActionPerrFrame->getBody().setPerrElemArraySize(failedDestinations.size());
    ieee80211ActionPerrFrame->getBody().setBodyLength(ieee80211ActionPerrFrame->getBody().getBodyLength() + (failedDestinations.size() * PERRElemLen));
    ieee80211ActionPerrFrame->setByteLength(ieee80211ActionPerrFrame->getByteLength() + (failedDestinations.size() * PERRElemLen));
    ieee80211ActionPerrFrame->getBody().setTTL(GetMaxTtl());

    ieee80211ActionPerrFrame->setTransmitterAddress(GetAddress());
    ieee80211ActionPerrFrame->setAddress3(ieee80211ActionPerrFrame->getTransmitterAddress());

    for (unsigned int i = 0; i < failedDestinations.size(); i++)
    {
        PERRElem perr;
        perr.destAddress = failedDestinations[i].destination;
        perr.destSeqNumber = failedDestinations[i].seqnum;
        perr.reasonCode = RC_MESH_PATH_ERROR_DESTINATION_UNREACHABLE;
        ieee80211ActionPerrFrame->getBody().setPerrElem(i, perr);
    }

    if (receivers.size() >= GetUnicastPerrThreshold() || receivers.empty())
    {
        receivers.clear();
        receivers.push_back(MACAddress::BROADCAST_ADDRESS);
    }
    for (unsigned int i = 0; i < receivers.size() - 1; i++)
    {
        //
        // 64-bit Intel valgrind complains about hdr.SetAddr1 (*i).  It likes this
        // just fine.
        //
        Ieee80211ActionPERRFrame * frameAux = ieee80211ActionPerrFrame->dup();
        frameAux->setReceiverAddress(receivers[i]);

        m_stats.txPerr++;
        m_stats.txMgt++;
        m_stats.txBytes += frameAux->getByteLength();
        // TODO: is necessary to obtain the interface id from the routing table in the future
        Ieee802Ctrl *ctrl = new Ieee802Ctrl();
        ctrl->setInterfaceId(interface80211ptr->getInterfaceId());
        frameAux->setControlInfo(ctrl);
        if (frameAux->getBody().getTTL() == 0)
            delete frameAux;
        else
        {
            EV << "Sending perr frame to " << frameAux->getReceiverAddress() << endl;
            sendDelayed(frameAux, par("unicastDelay"), "ipOut");
        }
    }
    ieee80211ActionPerrFrame->setReceiverAddress(receivers.back());
    m_stats.txPerr++;
    m_stats.txMgt++;
    m_stats.txBytes += ieee80211ActionPerrFrame->getByteLength();
    Ieee802Ctrl *ctrl = new Ieee802Ctrl();
    // TODO: is necessary to obtain the interface id from the routing table in the future
    ctrl->setInterfaceId(interface80211ptr->getInterfaceId());
    ieee80211ActionPerrFrame->setControlInfo(ctrl);
    if (ieee80211ActionPerrFrame->getBody().getTTL() == 0)
        delete ieee80211ActionPerrFrame;
    else
    {
        EV << "Sending perr frame to " << ieee80211ActionPerrFrame->getReceiverAddress() << endl;
        if (ieee80211ActionPerrFrame->getReceiverAddress() == MACAddress::BROADCAST_ADDRESS)
            sendDelayed(ieee80211ActionPerrFrame, par("broadcastDelay"), "ipOut");
        else
            sendDelayed(ieee80211ActionPerrFrame, par("unicastDelay"), "ipOut");
    }
}

void HwmpProtocol::forwardPerr(std::vector<HwmpFailedDestination> failedDestinations, std::vector<MACAddress> receivers)
{

    while (!failedDestinations.empty())
    {
        if (failedDestinations.size() > 20) // The maximum value of N is 20. 80211s
        {
            std::vector < HwmpFailedDestination > vectorAux;
            vectorAux.assign(failedDestinations.begin(), failedDestinations.begin() + 20);
            sendPerr(vectorAux, receivers);
            failedDestinations.erase(failedDestinations.begin(), failedDestinations.begin() + 20);
        }
        else
        {
            sendPerr(failedDestinations, receivers);
            failedDestinations.clear();
        }
    }
}

void HwmpProtocol::initiatePerr(std::vector<HwmpFailedDestination> failedDestinations,
        std::vector<MACAddress> receivers)
{
    EV << "initiateperr" << endl;
    //All duplicates in PERR are checked here, and there is no reason to
    //check it at any another place
    {
        std::vector<MACAddress>::const_iterator end = receivers.end();
        L3Address addAp;
        for (std::vector<MACAddress>::const_iterator i = receivers.begin(); i != end; i++)
        {
            bool should_add = true;
            MACAddress errorAdd;

            for (std::vector<MACAddress>::const_iterator j = m_myPerr.receivers.begin(); j != m_myPerr.receivers.end(); j++)
            {
                if (getAp(L3Address(*i), addAp))
                {
                    if (addAp.toMAC() == *j)
                        should_add = false;
                }
                else
                {
                    if ((*i) == (*j))
                        should_add = false;
                }

                if ((*i) == (*j))
                {
                    should_add = false;
                }
            }
            if (should_add)
            {
                if (addAp !=L3Address())
                    m_myPerr.receivers.push_back(addAp.toMAC());
                else
                    m_myPerr.receivers.push_back(*i);
            }
        }
    }
    {
        std::vector<HwmpFailedDestination>::const_iterator end = failedDestinations.end();
        for (std::vector<HwmpFailedDestination>::const_iterator i = failedDestinations.begin(); i != end; i++)
        {
            bool should_add = true;
            for (std::vector<HwmpFailedDestination>::const_iterator j = m_myPerr.destinations.begin(); j != m_myPerr.destinations.end(); j++)
                    {
                if (((*i).destination == (*j).destination) && ((*j).seqnum > (*i).seqnum))
                {
                    should_add = false;
                }
            }
            if (should_add)
            {
                m_myPerr.destinations.push_back(*i);
            }
        }
    }
    sendMyPerr();
}

void HwmpProtocol::sendMyPerr()
{
    if (m_perrTimer->isScheduled())
        return;
    m_perrTimer->resched(GetPerrMinInterval());
    forwardPerr(m_myPerr.destinations, m_myPerr.receivers);
    m_myPerr.destinations.clear();
    m_myPerr.receivers.clear();
}

uint32_t HwmpProtocol::GetLinkMetric(const MACAddress &peerAddress)
{
    EV << "createpreqetmetric " << endl;
    // TODO: Replace ETX by Airlink metric
    auto it = neighborMap.find(peerAddress);
    if (it == neighborMap.end())
        return 0xFFFFFFF; // no Neighbor
    if (it->second.lastTime + par("neighborLive") < simTime())
    {
        neighborMap.erase(it);
        // delete the route with this address like next hop
        m_rtable->deleteNeighborRoutes(peerAddress);
        return 0xFFFFFFF; // no Neighbor
    }
    if (par("minHopCost").boolValue()) // the cost is 1
        return 1; //WARNING: min hop for testing only

    if (useEtxProc)
    {
        Ieee80211Etx * etx = dynamic_cast<Ieee80211Etx *>(interface80211ptr->getEstimateCostProcess(0));if (etx)
            return etx->getAirtimeMetric(peerAddress);
        else
            return 1;
    }
    else
    {
        return it->second.cost;
    }
}

void HwmpProtocol::processPreq(cMessage *msg)
{
    EV << "processpreq " << endl;
    Ieee80211ActionPREQFrame *frame = dynamic_cast<Ieee80211ActionPREQFrame*>(msg);
    if (frame == nullptr)
    {
        delete msg;
        return;
    }
    MACAddress from = frame->getTransmitterAddress();
    MACAddress fromMp = frame->getAddress3();
    uint32_t metric = GetLinkMetric(from);
    uint32_t interface;
    if (frame->getControlInfo())
    {
        Ieee802Ctrl *ctrl = check_and_cast<Ieee802Ctrl*>(frame->removeControlInfo());
        interface = ctrl->getInterfaceId();
        delete ctrl;
    }
    else
        interface = interface80211ptr->getInterfaceId();
    EV << "Received preq from " << from << " with destination address " << frame->getReceiverAddress() << endl;
    receivePreq(frame, from, interface, fromMp, metric);
}

void HwmpProtocol::processPrep(cMessage *msg)
{
    EV << "processprep " << endl;
    Ieee80211ActionPREPFrame *frame = dynamic_cast<Ieee80211ActionPREPFrame*>(msg);
    if (frame == nullptr)
    {
        delete msg;
        return;
    }
    MACAddress from = frame->getTransmitterAddress();
    MACAddress fromMp = frame->getAddress3();
    uint32_t metric = GetLinkMetric(from);
    uint32_t interface;
    if (frame->getControlInfo())
    {
        Ieee802Ctrl *ctrl = check_and_cast<Ieee802Ctrl*>(frame->removeControlInfo());
        interface = ctrl->getInterfaceId();
        delete ctrl;
    }
    else
        interface = interface80211ptr->getInterfaceId();

    EV << "Received prep from " << from << " with destination address " << frame->getReceiverAddress() << endl;
    receivePrep(frame, from, interface, fromMp, metric);
}

void HwmpProtocol::processPerr(cMessage *msg)
{
    EV << "processperr " << endl;
    Ieee80211ActionPERRFrame *frame = dynamic_cast<Ieee80211ActionPERRFrame*>(msg);
    if (frame == nullptr)
    {
        delete msg;
        return;
    }
    MACAddress from = frame->getTransmitterAddress();
    MACAddress fromMp = frame->getAddress3();
    uint32_t interface;
    if (frame->getControlInfo())
    {
        Ieee802Ctrl *ctrl = check_and_cast<Ieee802Ctrl*>(frame->removeControlInfo());
        interface = ctrl->getInterfaceId();
        delete ctrl;
    }
    else
        interface = interface80211ptr->getInterfaceId();

    std::vector < HwmpFailedDestination > destinations;
    for (unsigned int i = 0; i < frame->getBody().getPerrElemArraySize(); i++)
    {
        PERRElem perr = frame->getBody().getPerrElem(i);
        HwmpFailedDestination fdest;
        fdest.destination = perr.destAddress;
        fdest.seqnum = perr.destSeqNumber;

    }
    delete msg;
    EV << "Received perr from " << from << " with destination address " << frame->getReceiverAddress() << endl;
    receivePerr(destinations, from, interface, fromMp);
}

void HwmpProtocol::processGann(cMessage *msg)
{
    Ieee80211ActionGANNFrame * gannFrame = dynamic_cast<Ieee80211ActionGANNFrame*>(msg);
    if (!gannFrame)
    {
        delete msg;
        return;
    }
    if (gannFrame->getBody().getMeshGateAddress() == GetAddress())
    {
        delete msg;
        return;
    }
    discoverRouteToGan(gannFrame->getBody().getMeshGateAddress());
    int index = -1;

    for (unsigned int i = 0; ganVector.size(); i++)
    {
        if (ganVector[i].gannAddr == gannFrame->getBody().getMeshGateAddress())
        {
            if ((int32_t)(ganVector[i].seqNum - gannFrame->getBody().getMeshGateSeqNumber()) > 0)
            {

                delete msg;
                return;
            }
            index = (int) i;
            break;
        }
    }
    if (index < 0)
    {
        GannData data;
        data.gannAddr = gannFrame->getBody().getMeshGateAddress();
        data.numHops = gannFrame->getBody().getHopsCount();
        data.seqNum = gannFrame->getBody().getMeshGateSeqNumber();
        ganVector.push_back(data);
    }
    else
    {
        ganVector[index].numHops = gannFrame->getBody().getHopsCount();
        ganVector[index].seqNum = gannFrame->getBody().getMeshGateSeqNumber();
    }

    gannFrame->setTransmitterAddress(GetAddress());
    gannFrame->setAddress3(gannFrame->getTransmitterAddress());
    for (int i = 1; i < getNumWlanInterfaces(); i++)
    {
        Ieee802Ctrl *ctrl = new Ieee802Ctrl();
        Ieee80211ActionGANNFrame *msgAux = gannFrame->dup();
        // Set the control info to the duplicate packet
        ctrl->setInterfaceId(getWlanInterfaceEntry(i)->getInterfaceId());
        msgAux->setControlInfo(ctrl);
        if (msgAux->getBody().getTTL() == 0)
            delete msgAux;
        else
        {
            EV << "Sending gann frame " << endl;
            sendDelayed(msgAux, par("broadcastDelay"), "ipOut");
        }
    }
    Ieee802Ctrl *ctrl = new Ieee802Ctrl();
    ctrl->setInterfaceId(getWlanInterfaceEntry(0)->getInterfaceId());
    gannFrame->setControlInfo(ctrl);
    if (gannFrame->getBody().getTTL() == 0)
        delete gannFrame;
    else
    {
        EV << "Sending gann frame " << endl;
        sendDelayed(gannFrame, par("broadcastDelay"), "ipOut");
    }
}

void HwmpProtocol::discoverRouteToGan(const MACAddress &add)
{
    HwmpRtable::LookupResult result = m_rtable->LookupReactive(add);
    if (result.retransmitter.isUnspecified())
    {
        L3Address apAddr;
        if (getAp(L3Address(add), apAddr))
        {
            // search this address
            result = m_rtable->LookupReactive(apAddr.toMAC());
        }
    }
    if (result.retransmitter.isUnspecified())
    {
        if (shouldSendPreq(add))
        {
            GetNextHwmpSeqno();
            uint32_t dst_seqno = 0;
            m_stats.initiatedPreq++;
            requestDestination(add, dst_seqno);
        }
    }
}

void HwmpProtocol::receivePreq(Ieee80211ActionPREQFrame *preqFrame, MACAddress from, uint32_t interface,
        MACAddress fromMp, uint32_t metric)
{
    preqFrame->getBody().setMetric(preqFrame->getBody().getMetric() + metric);
    uint32_t totalMetric = preqFrame->getBody().getMetric();
    MACAddress originatorAddress = preqFrame->getBody().getOriginator();
    uint32_t originatorSeqNumber = preqFrame->getBody().getOriginatorSeqNumber();
    bool addMode = (preqFrame->getBody().getFlags() & 0x40) != 0;

    bool propagate = true;

    //acceptance criteria:
    if (isLocalAddress(L3Address(originatorAddress)))
    {
        delete preqFrame;
        return;
    }
    std::map<MACAddress, std::pair<uint32_t, uint32_t> >::const_iterator i = m_hwmpSeqnoMetricDatabase.find(
            originatorAddress);
    bool freshInfo(true);
    if (preqFrame->getControlInfo())
        delete preqFrame->removeControlInfo();
    if (i != m_hwmpSeqnoMetricDatabase.end())
    {
        if ((int32_t)(i->second.first - originatorSeqNumber) > 0)
        {
            EV << "Old PREQ discard \n";
            delete preqFrame;
            return;
        }
        if (i->second.first == originatorSeqNumber)
        {
            freshInfo = false;
            if (i->second.second <= totalMetric)
            {
                EV << " PREQ with higher cost discard \n";
                delete preqFrame;
                return;
            }
        }
    }
    EV << " Fresh PREQ \n";
    m_hwmpSeqnoMetricDatabase[originatorAddress] = std::make_pair(originatorSeqNumber, totalMetric);
    EV << "I am " << GetAddress() << " Accepted preq from address: " << from << ", preq originator address: " << originatorAddress << endl;
    //Add reactive path to originator:

    if ((freshInfo)
            || ((m_rtable->LookupReactive(originatorAddress).retransmitter.isUnspecified())
                    || (m_rtable->LookupReactive(originatorAddress).metric > totalMetric)))
    {

        L3Address addAp;
        // it this address has an AP add the AP address only
        if (getAp(L3Address(originatorAddress), addAp))
        {
            m_rtable->AddReactivePath(addAp.toMAC(), from, interface, totalMetric,
                    ((double) preqFrame->getBody().getLifeTime() * 1024.0) / 1000000.0, originatorSeqNumber,
                    preqFrame->getBody().getHopsCount(), true);
        }
        else
        {
            m_rtable->AddReactivePath(originatorAddress, from, interface, totalMetric,
                    ((double) preqFrame->getBody().getLifeTime() * 1024.0) / 1000000.0, originatorSeqNumber,
                    preqFrame->getBody().getHopsCount(), true);
        }
        reactivePathResolved(originatorAddress);
    }
    if ((m_rtable->LookupReactive(fromMp).retransmitter.isUnspecified())
            || (m_rtable->LookupReactive(fromMp).metric > metric))
    {
        // we don't know the sequence number of this node
        m_rtable->AddReactivePath(fromMp, from, interface, metric,
                ((double) preqFrame->getBody().getLifeTime() * 1024.0) / 1000000.0, originatorSeqNumber, 1, false);
        reactivePathResolved(fromMp);
    }
    std::vector < MACAddress > delAddress;
    for (unsigned int preqCount = 0; preqCount < preqFrame->getBody().getPreqElemArraySize(); preqCount++)
    {
        PREQElem preq = preqFrame->getBody().getPreqElem(preqCount);
        if (preq.targetAddress == MACAddress::BROADCAST_ADDRESS)
        {
            //only proactive PREQ contains destination
            //address as broadcast! Proactive preq MUST
            //have destination count equal to 1 and
            //per destination flags DO and RF
            ASSERT(preqFrame->getBody().getTargetCount() == 1);
            //Add proactive path only if it is the better then existed
            //before
            EV << "adding proactive PREQ" <<"\n";
            if ((freshInfo) || ((m_rtable->LookupProactive()).retransmitter.isUnspecified())
                    || ((m_rtable->LookupProactive()).metric > totalMetric))
            {
                m_rtable->AddProactivePath(totalMetric, originatorAddress, from, interface,
                        ((double) preqFrame->getBody().getLifeTime() * 1024.0) / 1000000.0, originatorSeqNumber,
                        preqFrame->getBody().getHopsCount());
                proactivePathResolved();
            }
            bool proactivePrep = false;
            if ((preqFrame->getBody().getFlags() & 0x20) != 0 && preq.TO)
                proactivePrep = true;
            if (proactivePrep && propagateProactive)
            {
                sendPrep(GetAddress(), originatorAddress, from, (uint32_t) 0, GetNextHwmpSeqno(), originatorSeqNumber,
                        preqFrame->getBody().getLifeTime(), interface, 0, true);
            }
            if (!propagateProactive)
                propagate = false;
            break;
        }
        if (isAddressInProxyList(L3Address(preq.targetAddress)) ||  preq.targetAddress == GetAddress())
        {
            sendPrep(preq.targetAddress, originatorAddress, from, (uint32_t) 0, GetNextHwmpSeqno(), originatorSeqNumber,
                    preqFrame->getBody().getLifeTime(), interface, 0);
            ASSERT(!m_rtable->LookupReactive(originatorAddress).retransmitter.isUnspecified());
            delAddress.push_back(preq.targetAddress);
            continue;
        }
        //check if can answer:
        HwmpRtable::LookupResult result = m_rtable->LookupReactive(preq.targetAddress);
        // Case E2 hops == 1
        if (!result.retransmitter.isUnspecified() && result.hops == 1)
            preq.TO = true;
        else if (result.retransmitter.isUnspecified())
        {
            L3Address add;
            if (getAp(L3Address(preq.targetAddress),add))
            {
                result = m_rtable->LookupReactive(add.toMAC());
                if (!result.retransmitter.isUnspecified() && result.hops == 1)
                    preq.TO = true;
            }
        }
        if ((!(preq.TO)) && (!result.retransmitter.isUnspecified()))
        {
            //have a valid information and can answer
            uint32_t lifetime = (result.lifetime.dbl() * 1000000.0 / 1024.0);
            if ((lifetime > 0) && ((int32_t)(result.seqnum - preq.targetSeqNumber) >= 0))
            {
                sendPrep(preq.targetAddress, originatorAddress, from, result.metric, result.seqnum, originatorSeqNumber,
                        lifetime, interface, result.hops);
                m_rtable->AddPrecursor(preq.targetAddress, interface, from,
                        (preqFrame->getBody().getLifeTime() * 1024) / 1000000);
                delAddress.push_back(preq.targetAddress); // not propagate
                continue;
            }
        }
        if (addMode && preq.TO && result.retransmitter.isUnspecified())
        {
            delAddress.push_back(preq.targetAddress); // not propagate
            continue;
        }
    }

    if (preqFrame->getBody().getPreqElemArraySize() == delAddress.size())
    {
        delete preqFrame;
        return;
    }
    if (!delAddress.empty())
    {
        std::vector < PREQElem > preqElements;
        for (unsigned int preqCount = 0; preqCount < preqFrame->getBody().getPreqElemArraySize(); preqCount++)
        {
            PREQElem preq = preqFrame->getBody().getPreqElem(preqCount);
            for (unsigned int i = 0; i < delAddress.size(); i++)
            {
                if (preq.targetAddress == delAddress[i])
                {
                    preqElements.push_back(preq);
                    delAddress.erase(delAddress.begin() + i);
                    break;
                }
            }
        }
        //check if must retransmit:
        if (preqElements.size() == 0)
        {
            delete preqFrame;
            return;
        }
        // prepare the frame for retransmission
        int numEleDel = preqFrame->getBody().getPreqElemArraySize() - preqElements.size();
        preqFrame->getBody().setPreqElemArraySize(preqElements.size());
        for (unsigned int i = 0; i < preqFrame->getBody().getPreqElemArraySize(); i++)
            preqFrame->getBody().setPreqElem(i, preqElements[i]);
        // actualize sizes
        preqFrame->getBody().setBodyLength(preqFrame->getBody().getBodyLength() - (numEleDel * PREQElemLen));
        preqFrame->setByteLength(preqFrame->getByteLength() - (numEleDel * PREQElemLen));
    }

    // actualize address
    preqFrame->setTransmitterAddress(GetAddress());
    preqFrame->setAddress3(preqFrame->getTransmitterAddress());

    if (addMode)
    {
        int iId = interface80211ptr->getInterfaceId();
        std::vector < MACAddress > receivers = getPreqReceivers(iId);
        if (receivers.size() == 1 && receivers[0] == MACAddress::BROADCAST_ADDRESS)
        {
            preqFrame->getBody().setFlags(preqFrame->getBody().getFlags() & 0xBF);
            preqFrame->setReceiverAddress(MACAddress::BROADCAST_ADDRESS);
        }
        else
        {
            for (unsigned int i = 0; i < receivers.size() - 1; i++)
            {
                if (from == receivers[i])
                    continue;
                preqFrame->setReceiverAddress(receivers[i]);
                if (preqFrame->getBody().getTTL() == 0)
                {
                    delete preqFrame;
                    return;
                }
                else
                {
                    cPacket *pktAux = preqFrame->dup();
                    // TODO: is necessary to obtain the interface id from the routing table in the future
                    Ieee802Ctrl *ctrl = new Ieee802Ctrl();
                    ctrl->setInterfaceId(interface80211ptr->getInterfaceId());
                    pktAux->setControlInfo(ctrl);
                    EV << "Propagating preq frame to " << preqFrame->getReceiverAddress() << endl;
                    sendDelayed(pktAux, par("unicastDelay"), "ipOut");
                }
            }
            if (from != receivers.back())
                preqFrame->setReceiverAddress(receivers.back());
            else
            {
                delete preqFrame;
                return;
            }
        }
    }
    else
        preqFrame->setReceiverAddress(MACAddress::BROADCAST_ADDRESS);
    Ieee802Ctrl *ctrl = new Ieee802Ctrl();
    ctrl->setInterfaceId(interface80211ptr->getInterfaceId());
    preqFrame->setControlInfo(ctrl);

    if (preqFrame->getBody().getTTL() == 0 || !propagate)
        delete preqFrame;
    else
    {
        EV << "Propagating preq frame to " << preqFrame->getReceiverAddress() << endl;
        if (preqFrame->getReceiverAddress() == MACAddress::BROADCAST_ADDRESS)
            sendDelayed(preqFrame, par("broadcastDelay"), "ipOut");
        else
            sendDelayed(preqFrame, par("unicastDelay"), "ipOut");
    }
}

void HwmpProtocol::receivePrep(Ieee80211ActionPREPFrame * prepFrame, MACAddress from, uint32_t interface,
        MACAddress fromMp, uint32_t metric)
{
    uint32_t totalMetric = prepFrame->getBody().getMetric() + metric;
    prepFrame->getBody().setMetric(totalMetric);
    MACAddress originatorAddress = prepFrame->getBody().getOriginator();
    uint32_t originatorSeqNumber = prepFrame->getBody().getOriginatorSeqNumber();
    MACAddress destinationAddress = prepFrame->getBody().getTarget();
    //acceptance cretirea:
    std::map<MACAddress, std::pair<uint32_t, uint32_t> >::const_iterator i;
    L3Address addAp;
    L3Address addApDest;

    if (getAp(L3Address(originatorAddress), addAp))
        i = m_hwmpSeqnoMetricDatabase.find(addAp.toMAC());
    else
        i = m_hwmpSeqnoMetricDatabase.find(originatorAddress);

    if (prepFrame->getControlInfo())
        delete prepFrame->removeControlInfo();
    bool freshInfo(true);
    if (i != m_hwmpSeqnoMetricDatabase.end())
    {
        if ((int32_t)(i->second.first - originatorSeqNumber) > 0)
        {
            delete prepFrame;
            return;
        }
        if (i->second.first == originatorSeqNumber)
        {
            freshInfo = false;
        }
    }
    if (addAp != L3Address())
        m_hwmpSeqnoMetricDatabase[addAp.toMAC()] = std::make_pair(originatorSeqNumber, totalMetric);
    else
        m_hwmpSeqnoMetricDatabase[originatorAddress] = std::make_pair(originatorSeqNumber, totalMetric);
    //update routing info
    //Now add a path to destination and add precursor to source
    EV << "received Prep with originator :" << originatorAddress << " from " << from << endl;

    //Add a reactive path only if seqno is fresher or it improves the
    //metric
    HwmpRtable::LookupResult result;
    if (addAp != L3Address())
        result = m_rtable->LookupReactive(addAp.toMAC());
    else
        result = m_rtable->LookupReactive(originatorAddress);

    HwmpRtable::LookupResult resultDest;
    if (getAp(L3Address(destinationAddress), addApDest))
        resultDest = m_rtable->LookupReactive(addApDest.toMAC());
    else
        resultDest = m_rtable->LookupReactive(destinationAddress);


    if ((freshInfo)
            || (result.retransmitter.isUnspecified())
                    || (result.metric > totalMetric))
    {

        if (addAp != L3Address())
        {
            m_rtable->AddReactivePath(addAp.toMAC(), from, interface, totalMetric,
                    ((double) prepFrame->getBody().getLifeTime() * 1024.0) / 1000000.0, originatorSeqNumber,
                    prepFrame->getBody().getHopsCount(), true);
        }
        else
        {
            m_rtable->AddReactivePath(originatorAddress, from, interface, totalMetric,
                    ((double) prepFrame->getBody().getLifeTime() * 1024.0) / 1000000.0, originatorSeqNumber,
                    prepFrame->getBody().getHopsCount(), true);
        }
        m_rtable->AddPrecursor(destinationAddress, interface, from,
                ((double) prepFrame->getBody().getLifeTime() * 1024.0) / 1000000.0);
        if (!resultDest.retransmitter.isUnspecified())
        {
            m_rtable->AddPrecursor(originatorAddress, interface, resultDest.retransmitter, resultDest.lifetime);
        }
        reactivePathResolved(originatorAddress);
    }
    if (((m_rtable->LookupReactive(fromMp)).retransmitter.isUnspecified())
            || ((m_rtable->LookupReactive(fromMp)).metric > metric))
    {
        // we don't know the neighbor sequence number
        m_rtable->AddReactivePath(fromMp, from, interface, metric,
                ((double) prepFrame->getBody().getLifeTime() * 1024.0) / 1000000.0, originatorSeqNumber, 1, false);
        reactivePathResolved(fromMp);
    }
    if (destinationAddress == GetAddress() || isAddressInProxyList(L3Address(destinationAddress)) || (addApDest.toMAC() == GetAddress()))
    {
        EV << "I am " << GetAddress() << ", resolved " << originatorAddress << endl;
        delete prepFrame;
        return;
    }
    if (resultDest.retransmitter.isUnspecified())
    {
        delete prepFrame;
        return;
    }
    //Forward PREP
    // HwmpProtocolMacMap::const_iterator prep_sender = m_interfaces.find (result.ifIndex);
    // ASSERT (prep_sender != m_interfaces.end ());
    prepFrame->setTransmitterAddress(GetAddress());
    prepFrame->setAddress3(prepFrame->getTransmitterAddress());
    prepFrame->setReceiverAddress(resultDest.retransmitter);
    // TODO: obtain the correct interface ID
    Ieee802Ctrl *ctrl = new Ieee802Ctrl();
    ctrl->setInterfaceId(interface80211ptr->getInterfaceId());
    prepFrame->setControlInfo(ctrl);

    if (prepFrame->getBody().getTTL() == 0)
        delete prepFrame;
    else
    {
        EV << "Propagating prep frame to " << prepFrame->getReceiverAddress() << endl;
        sendDelayed(prepFrame, par("unicastDelay"), "ipOut");
    }
}

void HwmpProtocol::receivePerr(std::vector<HwmpFailedDestination> destinations, MACAddress from, uint32_t interface,
        MACAddress fromMp)
{
    //Acceptance cretirea:
    EV << GetAddress() << ", received PERR from " << from << endl;
    std::vector < HwmpFailedDestination > retval;
    HwmpRtable::LookupResult result;
    for (unsigned int i = 0; i < destinations.size(); i++)
    {
        result = m_rtable->LookupReactiveExpired(destinations[i].destination);
        if (!((result.retransmitter != from) || (result.ifIndex != interface)
                || ((int32_t)(result.seqnum - destinations[i].seqnum) > 0) ))
        {
            retval.push_back(destinations[i]);
        }
    }
    if (retval.size() == 0)
    {
        return;
    }
    forwardPathError (makePathError(retval));}

void HwmpProtocol::processLinkBreak(const cObject *details)
{
    Ieee80211TwoAddressFrame *frame = dynamic_cast<Ieee80211TwoAddressFrame *>(const_cast<cObject*>(details));
    if (frame)
    {
        auto it = neighborMap.find(frame->getReceiverAddress());
        if (it != neighborMap.end())
        {
            EV<<"second.lost"<<it->second.lost<<endl;
            it->second.lost++;
            if (it->second.lost < (unsigned int) par("lostThreshold").longValue())
                return;
            neighborMap.erase(it);
        }
        Ieee80211TwoAddressFrame *frame = dynamic_cast<Ieee80211TwoAddressFrame *>(const_cast<cObject*>(details));
        packetFailedMac(frame);
    }
}

void HwmpProtocol::processLinkBreakManagement(const cObject *details)
{
    Ieee80211ActionPREPFrame *frame = dynamic_cast<Ieee80211ActionPREPFrame *>(const_cast<cObject*>(details));
    if (frame)
    {
        auto it = neighborMap.find(frame->getReceiverAddress());
        if (it != neighborMap.end())
        {
            EV<<"second.lost++"<<it->second.lost<<endl;
            it->second.lost++;
            if (it->second.lost < (unsigned int) par("lostThreshold").longValue())
                return;
            neighborMap.erase(it);
        }
        std::vector < HwmpFailedDestination > destinations = m_rtable->GetUnreachableDestinations(
                frame->getReceiverAddress());
        initiatePathError (makePathError(destinations));
    }
}

void HwmpProtocol::packetFailedMac(Ieee80211TwoAddressFrame *frame)
{
    std::vector < HwmpFailedDestination > destinations = m_rtable->GetUnreachableDestinations(
            frame->getReceiverAddress());
    initiatePathError (makePathError(destinations));
}

HwmpProtocol::PathError HwmpProtocol::makePathError(const std::vector<HwmpFailedDestination> &destinations)
{
    EV<<"makepatherror"<<endl;
    PathError retval;
    //HwmpRtable increments a sequence number as written in 11B.9.7.2
    retval.receivers = getPerrReceivers(destinations);
    if (retval.receivers.size() == 0)
    {
        return retval;
    }
    m_stats.initiatedPerr++;
    for (unsigned int i = 0; i < destinations.size(); i++)
    {
        retval.destinations.push_back(destinations[i]);
        m_rtable->DeleteReactivePath(destinations[i].destination);
    }
    return retval;
}

void HwmpProtocol::initiatePathError(PathError perr)
{
    for (int i = 0; i < getNumWlanInterfaces(); i++)
    {
        std::vector < MACAddress > receivers_for_interface;
        for (unsigned int j = 0; j < perr.receivers.size(); j++)
        {
            if (getWlanInterfaceEntry(i)->getInterfaceId() == (int) perr.receivers[j].first)
                receivers_for_interface.push_back(perr.receivers[j].second);
        }
        initiatePerr(perr.destinations, receivers_for_interface);
    }
}

void HwmpProtocol::forwardPathError(PathError perr)
{
    for (int i = 0; i < getNumWlanInterfaces(); i++)
    {
        std::vector < MACAddress > receivers_for_interface;
        for (unsigned int j = 0; j < perr.receivers.size(); j++)
        {
            if (getWlanInterfaceEntry(i)->getInterfaceId() == (int) perr.receivers[j].first)
                receivers_for_interface.push_back(perr.receivers[j].second);
        }
        forwardPerr(perr.destinations, receivers_for_interface);
    }
}

std::vector<std::pair<uint32_t, MACAddress> > HwmpProtocol::getPerrReceivers(const
        std::vector<HwmpFailedDestination> &failedDest)
{
    EV << "getPerrReceivers"<<endl;
    HwmpRtable::PrecursorList retval;
    for (unsigned int i = 0; i < failedDest.size(); i++)
    {
        HwmpRtable::PrecursorList precursors = m_rtable->GetPrecursors(failedDest[i].destination);
        m_rtable->DeleteReactivePath(failedDest[i].destination);
        m_rtable->DeleteProactivePath(failedDest[i].destination);
        for (unsigned int j = 0; j < precursors.size(); j++)
            retval.push_back(precursors[j]);
    }
    //Check if we have dublicates in retval and precursors:
    for (unsigned int i = 0; i < retval.size(); i++)
    {
        for (unsigned int j = i + 1; j < retval.size(); j++)
        {
            if (retval[i].second == retval[j].second)
                retval.erase(retval.begin() + j);
        }
    }
    return retval;
}

std::vector<MACAddress> HwmpProtocol::getPreqReceivers(uint32_t interface)
{
    std::vector < MACAddress > retval;
    int numNeigh = 0;

    if (1 >= m_unicastPreqThreshold)
    {
        retval.clear();
        retval.push_back(MACAddress::BROADCAST_ADDRESS);
        return retval;
    }

    if (useEtxProc)
    {
        InterfaceEntry *ie = getInterfaceEntryById(interface);
        if (ie->getEstimateCostProcess(0))
            numNeigh = ie->getEstimateCostProcess(0)->getNumNeighbors();

        if ((numNeigh >= m_unicastPreqThreshold) || (numNeigh == 0))
        {
            retval.clear();
            retval.push_back(MACAddress::BROADCAST_ADDRESS);
            return retval;
        }
        std::vector<MACAddress> addr;
        ie->getEstimateCostProcess(0)->getNeighbors(addr);
        retval.clear();
        for (int i = 0; i < numNeigh; i++)
            retval.push_back(addr[i]);
    }
    else
    {
        for (auto it = neighborMap.begin(); it != neighborMap.end(); it++)
        {
            if (it->second.lastTime + par("neighborLive") < simTime())
                neighborMap.erase(it);
            else
            {
                retval.push_back(it->first);
            }
        }
    }
    return retval;
}

std::vector<MACAddress> HwmpProtocol::getBroadcastReceivers(uint32_t interface)
{
    std::vector < MACAddress > retval;
    int numNeigh = 0;

    if (1 >= m_unicastDataThreshold)
    {
        retval.clear();
        retval.push_back(MACAddress::BROADCAST_ADDRESS);
        return retval;
    }

    if (useEtxProc)
    {
        InterfaceEntry *ie = getInterfaceEntryById(interface);

        if (ie->getEstimateCostProcess(0))
            numNeigh = ie->getEstimateCostProcess(0)->getNumNeighbors();

        if ((numNeigh >= m_unicastDataThreshold) || (numNeigh == 0))
        {
            retval.clear();
            retval.push_back(MACAddress::BROADCAST_ADDRESS);
            return retval;
        }

        std::vector<MACAddress> addr;
        ie->getEstimateCostProcess(0)->getNeighbors(addr);
        retval.clear();
        for (int i = 0; i < numNeigh; i++)
            retval.push_back(addr[i]);
    }
    else
    {
        for (auto it = neighborMap.begin(); it != neighborMap.end(); it++)
        {
            if (it->second.lastTime + par("neighborLive") < simTime())
                neighborMap.erase(it);
            else
            {
                retval.push_back(it->first);
            }
        }
    }
    return retval;
}

bool HwmpProtocol::QueuePacket(QueuedPacket packet)
{
    //delete old packets
    if (m_rqueue.size() > m_maxQueueSize)
    {
        return false;
    }
    packet.queueTime = simTime();

    m_rqueue.push_back(packet);
    return true;
}

HwmpProtocol::QueuedPacket HwmpProtocol::dequeueFirstPacketByDst(MACAddress dst)
{
    HwmpProtocol::QueuedPacket retval;
    retval.pkt = 0;
    for (auto i = m_rqueue.begin(); i != m_rqueue.end(); i++)
    {
        if ((*i).dst == dst)
        {
            retval = (*i);
            m_rqueue.erase(i);
            break;
        }
    }
    return retval;
}

HwmpProtocol::QueuedPacket HwmpProtocol::dequeueFirstPacket()
{
    HwmpProtocol::QueuedPacket retval;
    retval.pkt = 0;
    if (m_rqueue.size() != 0)
    {
        retval = m_rqueue[0];
        m_rqueue.erase(m_rqueue.begin());
    }
    return retval;
}

void HwmpProtocol::reactivePathResolved(MACAddress dst)
{


    //Send all packets stored for this destination
    HwmpRtable::LookupResult result;
    L3Address apAddr;
    std::vector<MACAddress> listAddress;
    if (getAp(L3Address(dst), apAddr))
    {
        result = m_rtable->LookupReactive(apAddr.toMAC());
        ASSERT(result.retransmitter != MACAddress::BROADCAST_ADDRESS);
        if (result.retransmitter.isUnspecified())
        {
            // send and error and stop the simulation?
            EV
                    << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            EV << "!!!!!!!!!!!!!!!! WANING HWMP try to send a packet and the protocol doesnt' know the next hop address \n";
            EV
                    << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            return;
        }
        getApList(apAddr.toMAC(),listAddress);
    }
    else
    {
        result = m_rtable->LookupReactive(dst);
        ASSERT(result.retransmitter != MACAddress::BROADCAST_ADDRESS);
        if (result.retransmitter.isUnspecified())
        {
            // send and error and stop the simulation?
            EV
                    << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            EV << "!!!!!!!!!!!!!!!! WANING HWMP try to send a packet and the protocol doesnt' know the next hop address \n";
            EV
                    << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n";
            return;
        }
        getApList(dst,listAddress);
    }

    while (!listAddress.empty())
    {
        QueuedPacket packet = dequeueFirstPacketByDst(listAddress.back());

        auto i = m_preqTimeouts.find(listAddress.back());
        if (i != m_preqTimeouts.end()) // cancel pending preq
        {
            i->second.preqTimeout->removeTimer();
            delete i->second.preqTimeout;
            m_preqTimeouts.erase(i);
        }

        while (packet.pkt != 0)
        {
            m_stats.txUnicast++;
            m_stats.txBytes += packet.pkt->getByteLength();
            Ieee802Ctrl * ctrl = new Ieee802Ctrl();
            ctrl->setInterfaceId(result.ifIndex);
            ctrl->setDest(result.retransmitter);
            packet.pkt->setControlInfo(ctrl);
            send(packet.pkt, "ipOut");
            packet = dequeueFirstPacketByDst(listAddress.back());
        }
        listAddress.pop_back();
    }
}

void HwmpProtocol::proactivePathResolved()
{
    EV << "proactivePathResolved " << endl;
    //send all packets to root
    HwmpRtable::LookupResult result = m_rtable->LookupProactive();
    ASSERT(!result.retransmitter.isUnspecified());
    QueuedPacket packet = dequeueFirstPacket();
    while (packet.pkt != 0)
    {
        m_stats.txUnicast++;
        m_stats.txBytes += packet.pkt->getByteLength();
        EV << "Send queue packets " << endl;
        Ieee802Ctrl * ctrl = new Ieee802Ctrl();
        ctrl->setInterfaceId(result.ifIndex);
        ctrl->setDest(result.retransmitter);
        packet.pkt->setControlInfo(ctrl);
        sendDelayed(packet.pkt, par("unicastDelay"), "ipOut");
        packet = dequeueFirstPacket();
    }
}

//Proactive PREQ routines:
void HwmpProtocol::setRoot()
{
    EV << "setRoot " << endl;
    double randomStart = par("randomStart");
    m_proactivePreqTimer->resched(randomStart);
    EV << "ROOT IS: " << GetAddress() << endl;
    m_isRoot = true;
}

void HwmpProtocol::UnsetRoot()
{
    m_proactivePreqTimer->removeTimer();
}

bool HwmpProtocol::GetToFlag()
{
    return m_ToFlag;
}

double HwmpProtocol::GetPreqMinInterval()
{
    return m_dot11MeshHWMPpreqMinInterval;
}

double HwmpProtocol::GetPerrMinInterval()
{
    return m_dot11MeshHWMPperrMinInterval;
}
uint8_t HwmpProtocol::GetMaxTtl()
{
    return m_maxTtl;
}

uint32_t HwmpProtocol::GetNextPreqId()
{
    m_preqId++;
    return m_preqId;
}

uint32_t HwmpProtocol::GetNextHwmpSeqno()
{
    m_hwmpSeqno++;
    return m_hwmpSeqno;
}

uint32_t HwmpProtocol::GetNextGannSeqno()
{
    gannSeqNumber++;
    return gannSeqNumber;
}

uint32_t HwmpProtocol::GetActivePathLifetime()
{
    EV << m_dot11MeshHWMPactivePathTimeout * 1000000 / 1024 <<endl;
    return m_dot11MeshHWMPactivePathTimeout * 1000000 / 1024;
}

uint32_t HwmpProtocol::GetRootPathLifetime()
{
    EV <<m_dot11MeshHWMPactiveRootTimeout * 1000000 / 1024 <<endl;
    return m_dot11MeshHWMPactiveRootTimeout * 1000000 / 1024;
}

uint8_t HwmpProtocol::GetUnicastPerrThreshold()
{
    return m_unicastPerrThreshold;
}

MACAddress HwmpProtocol::GetAddress()
{
    return getAddress().toMAC();
}

bool HwmpProtocol::isOurType(cPacket *p)
{
    if (dynamic_cast<Ieee80211ActionMeshFrame*>(p))
        return true;
    return false;
}

bool HwmpProtocol::getDestAddress(cPacket *msg, L3Address &addr)
{
    Ieee80211ActionPREQFrame *rreq = dynamic_cast<Ieee80211ActionPREQFrame *>(msg);
    if (!rreq)
        return false;
    PREQElem pqeq = rreq->getBody().getPreqElem(0);
    addr = L3Address(pqeq.targetAddress);
    return true;
}


bool HwmpProtocol::getNextHop(const L3Address &dest, L3Address &add, int &iface, double &cost)
{
    EV<<"getNextHop"<<endl;
    L3Address apAddr;
    HwmpRtable::LookupResult result = m_rtable->LookupReactive(dest.toMAC());
    if (result.retransmitter.isUnspecified()) // address not valid
    {
        if (getAp(dest,apAddr))
        {
            MACAddress macAddr(apAddr.toMAC());
            // search this address
            HwmpRtable::LookupResult result = m_rtable->LookupReactive(macAddr);
            if (!result.retransmitter.isUnspecified())
            {
                add = L3Address(result.retransmitter);
                cost = result.metric;
                iface = result.ifIndex;
                return true;
            }
        }
        if (m_concurrentReactive && !isRoot())
        {
            if (shouldSendPreq(dest.toMAC()))
            {
                GetNextHwmpSeqno();
                uint32_t dst_seqno = 0;
                m_stats.initiatedPreq++;
                requestDestination(dest.toMAC(), dst_seqno);
            }
        }
    }
    else
    {
        // check
        if (getAp(dest, apAddr))
        {
            HwmpRtable::LookupResult resultAp = m_rtable->LookupReactive(apAddr.toMAC());
            if (result.retransmitter.getInt() != resultAp.retransmitter.getInt())
            {
                EV<<"deletereactivepath"<<endl;
                if (resultAp.retransmitter.isUnspecified())
                    m_rtable->DeleteReactivePath(dest.toMAC());
                else
                {
                    m_rtable->AddReactivePath(apAddr.toMAC(),resultAp.retransmitter,resultAp.ifIndex,
                            resultAp.metric,resultAp.lifetime,resultAp.seqnum,resultAp.hops,true);
                    reactivePathResolved(apAddr.toMAC());

                    add = L3Address(resultAp.retransmitter);
                    cost = resultAp.metric;
                    iface = resultAp.ifIndex;
                    return true;
                }
            }
        }
        else
        {
            add = L3Address(result.retransmitter);
            cost = result.metric;
            iface = result.ifIndex;
            return true;
        }
    }

    if (isRoot())
        return false; // the node is root and doesn't have a valid route

    HwmpRtable::LookupResult resultProact = m_rtable->LookupProactive();
    if (resultProact.retransmitter.isUnspecified())
        return false; // the Mesh code should send the packet to hwmp
    add = L3Address(resultProact.retransmitter);
    cost = resultProact.metric;
    iface = resultProact.ifIndex;
    return true;
}

uint32_t HwmpProtocol::getRoute(const L3Address &dest, std::vector<L3Address> &add)
{
    return 0;
}

bool HwmpProtocol::getNextHopReactive(const L3Address &dest, L3Address &add, int &iface, double &cost)
{
    HwmpRtable::LookupResult result = m_rtable->LookupReactive(dest.toMAC());
    if (result.retransmitter.isUnspecified()) // address not valid
    {
        L3Address apAddr;
        if (getAp(dest,apAddr))
        {
            MACAddress macAddr(apAddr.toMAC());
            if (!macAddr.isUnspecified())
            {
                // search this address
                HwmpRtable::LookupResult result = m_rtable->LookupReactive(macAddr);
                if (!result.retransmitter.isUnspecified())
                {
                    add = L3Address(result.retransmitter);
                    cost = result.metric;
                    iface = result.ifIndex;
                    return true;
                }
            }
        }
        return false;
    }

    add = L3Address(result.retransmitter);
    cost = result.metric;
    iface = result.ifIndex;
    return true;
}

bool HwmpProtocol::getNextHopProactive(const L3Address &dest, L3Address &add, int &iface, double &cost)
{
    EV << "getNextHopProactive " << endl;
    HwmpRtable::LookupResult result = m_rtable->LookupProactive();
    if (result.retransmitter.isUnspecified())
        return false;
    add = L3Address(result.retransmitter);
    cost = result.metric;
    iface = result.ifIndex;
    return true;
}

int HwmpProtocol::getInterfaceReceiver(MACAddress add)
{
    EV << "getInterfaceReceiver" << endl;

    HwmpRtable::LookupResult result = m_rtable->LookupReactive(add);
    if (result.retransmitter.isUnspecified()) // address not valid
    {
        L3Address apAddr;
        if (getAp(L3Address(add), apAddr))
            result = m_rtable->LookupReactive(apAddr.toMAC());
    }
    if (result.retransmitter.isUnspecified())
        result = m_rtable->LookupProactive();
    if (result.retransmitter.isUnspecified())
        return -1;
    if (result.retransmitter != add)
        return -1;
    return result.ifIndex;
}

void HwmpProtocol::setRefreshRoute(const L3Address &destination, const L3Address &nextHop, bool isReverse)
{
   EV<<"setRefreshRoute"<<endl;
    if (!par("updateLifetimeInFrowarding").boolValue())
        return;
    HwmpRtable::ReactiveRoute * route = m_rtable->getLookupReactivePtr(destination.toMAC());
    if (!route)
    {
        L3Address apAddr;
        if (getAp(destination,apAddr))
            route = m_rtable->getLookupReactivePtr(apAddr.toMAC());
    }
    if (par("checkNextHop").boolValue())
    {
        if (route && nextHop.toMAC() == route->retransmitter)
        {
            route->whenExpire = simTime() + m_dot11MeshHWMPactivePathTimeout;
        }
        else
        {
            EV<<"deleting route"<<endl;
            route = nullptr;
        }
    }
    else
    {
        if (route)
        {
            route->whenExpire = simTime() + m_dot11MeshHWMPactivePathTimeout;
        }
    }
    if (isReverse && !route && par("gratuitousReverseRoute").boolValue())
    {

        L3Address addAp;
        if (getAp(destination, addAp))
        {
            m_rtable->AddReactivePath(addAp.toMAC(), nextHop.toMAC(),
                    interface80211ptr->getInterfaceId(), HwmpRtable::MAX_METRIC, m_dot11MeshHWMPactivePathTimeout, 0,
                    HwmpRtable::MAX_HOPS, false);
        }
        else
        {
            m_rtable->AddReactivePath(destination.toMAC(), nextHop.toMAC(),
                    interface80211ptr->getInterfaceId(), HwmpRtable::MAX_METRIC, m_dot11MeshHWMPactivePathTimeout, 0,
                    HwmpRtable::MAX_HOPS, false);
        }
        reactivePathResolved(destination.toMAC());
    }
    /** the root is only actualized by the proactive mechanism
     HwmpRtable::ProactiveRoute * root = m_rtable->getLookupProactivePtr ();
     if (!isRoot() && root)
     {
     if (dest.getMACAddress() ==root->root)
     {
     root->whenExpire=simTime()+m_dot11MeshHWMPactiveRootTimeout;
     }
     }
     */
}

void HwmpProtocol::processFullPromiscuous(const cObject *details)
{
    Enter_Method("HwmpProtocol Promiscuous");
    if (details == nullptr)
        return;
    Ieee80211TwoAddressFrame *frame = dynamic_cast<Ieee80211TwoAddressFrame *>(const_cast<cObject*>(details));
    if (frame == nullptr)
        return;

    Ieee80211ReceptionIndication * cinfo = dynamic_cast<Ieee80211ReceptionIndication *>(frame->getControlInfo());
    uint32_t cost = 1;
    if (cinfo)
    {
        if (dynamic_cast<Ieee80211DataFrame *>(frame))
        {
            if (cinfo->getAirtimeMetric())
            {
                SNRDataTime snrDataTime;
                snrDataTime.signalPower = cinfo->getRecPow();
                snrDataTime.snrData = cinfo->getSnr();
                snrDataTime.snrTime = simTime();
                snrDataTime.testFrameDuration = cinfo->getTestFrameDuration();
                snrDataTime.testFrameError = cinfo->getTestFrameError();
                snrDataTime.airtimeMetric = cinfo->getAirtimeMetric();
                cost = ((uint32_t) ceil(
                        (snrDataTime.testFrameDuration / 10.24e-6) / (1 - snrDataTime.testFrameDuration)));
            }
            else
                cost = 1;
        }
    }

    auto it = neighborMap.find(frame->getTransmitterAddress());

    if (it != neighborMap.end())
    {
        it->second.cost = cost;
        it->second.lastTime = simTime();
        it->second.lost = 0;
    }
    else
    {
        Neighbor ne;
        ne.cost = cost;
        ne.lastTime = simTime();
        ne.lost = 0;
        neighborMap.insert(std::pair<MACAddress, Neighbor>(frame->getTransmitterAddress(), ne));
    }
}

bool HwmpProtocol::getBestGan(L3Address &gannAddr, L3Address &nextHop)
{
    EV << "getBestGan" << endl;
    if (m_isGann)
    {
        L3Address aux = L3Address(GetAddress());
        gannAddr = aux;
        nextHop = aux;
        return true;
    }
    HwmpRtable::ProactiveRoute *rootPath = m_rtable->getLookupProactivePtr();
    if (ganVector.empty())
    {
        if (rootPath == nullptr)
            return false;
        nextHop = L3Address(rootPath->retransmitter);
        gannAddr = L3Address(rootPath->root);
        return true;
    }
    HwmpRtable::ReactiveRoute *bestGanPath = nullptr;
    MACAddress bestGanPathAddr;
    for (unsigned int i = 0; i < ganVector.size(); i++)
    {
        HwmpRtable::ReactiveRoute *ganPath = m_rtable->getLookupReactivePtr(ganVector[i].gannAddr);
        if (ganPath == nullptr)
            continue;
        if (bestGanPath == nullptr)
        {
            bestGanPath = ganPath;
            bestGanPathAddr = ganVector[i].gannAddr;
        }
        else if (bestGanPath->metric > ganPath->metric)
        {
            bestGanPath = ganPath;
            bestGanPathAddr = ganVector[i].gannAddr;
        }
        else if (bestGanPath->metric == ganPath->metric && bestGanPath->whenExpire < ganPath->whenExpire)
        {
            bestGanPath = ganPath;
            bestGanPathAddr = ganVector[i].gannAddr;
        }
    }
    if (bestGanPath == nullptr && rootPath == nullptr)
    {
        return false;
    }
    if (bestGanPath == nullptr || (rootPath && (bestGanPath->metric > rootPath->metric)))
    {
        nextHop = L3Address(rootPath->retransmitter);
        gannAddr = L3Address(rootPath->root);
    }
    else
    {
        nextHop = L3Address(bestGanPath->retransmitter);
        gannAddr = L3Address(bestGanPathAddr);
    }
    return true;
}

bool HwmpProtocol::handleNodeStart(IDoneCallback *doneCallback)
{
    m_isGann = par("isGan");
    if (m_isGann)
    {
        double randomStart = par("randomStart");
        m_gannTimer->resched(randomStart);
    }
    scheduleEvent();
    return true;
}

bool HwmpProtocol::handleNodeShutdown(IDoneCallback *doneCallback)
{
    while (!getTimerMultimMap()->empty())
    {
        ManetTimer * timer = getTimerMultimMap()->begin()->second;
        getTimerMultimMap()->erase(getTimerMultimMap()->begin());
        if (timer == m_proactivePreqTimer)
            continue;
        else if (timer ==  m_preqTimer)
            continue;
        else if (timer ==  m_perrTimer)
            continue;
        else if (timer ==  m_gannTimer)
            continue;
        delete timer;
    }
    m_rtable->clearTable();
    neighborMap.clear();
    ganVector.clear();
    while (!m_rqueue.empty())
    {
        delete m_rqueue.back().pkt;
        m_rqueue.pop_back();
    }
    return true;
}

void HwmpProtocol::handleNodeCrash()
{
    while (!getTimerMultimMap()->empty())
    {
        ManetTimer * timer = getTimerMultimMap()->begin()->second;
        getTimerMultimMap()->erase(getTimerMultimMap()->begin());
        if (timer == m_proactivePreqTimer)
            continue;
        else if (timer ==  m_preqTimer)
            continue;
        else if (timer ==  m_perrTimer)
            continue;
        else if (timer ==  m_gannTimer)
            continue;
        delete timer;
    }
    m_rtable->clearTable();
    neighborMap.clear();
    ganVector.clear();
    while (!m_rqueue.empty())
    {
        delete m_rqueue.back().pkt;
        m_rqueue.pop_back();
    }
}

}

}
