//
// Copyright (C) 2008 Alfonso Ariza
// Copyright (C) 2012 Alfonso Ariza
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

#include "inet/linklayer/ieee80211/mgmt/Ieee80211Etx.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211ControlInfo_m.h"
#include "inet/common/GlobalWirelessLinkInspector.h"
#include "inet/common/ModuleAccess.h"

namespace inet {

namespace ieee80211 {

using namespace physicallayer;

Define_Module(Ieee80211Etx);

void Ieee80211Etx::initialize(int stage)
{
    if (stage==2)
    {
        ettIndex = 0;
        etxTimer = new cMessage("etxTimer");
        ettTimer = new cMessage("etxTimer");
        etxInterval = par("ETXInterval");
        ettInterval = par("ETTInterval");
        etxMeasureInterval = par("ETXMeasureInterval");
        etxSize = par("ETXSize");
        ettSize1 = par("ETTSize1");
        ettSize2 = par("ETTSize2");
        ettWindow = par("EttWindow");
        if (ettInterval > 0)
            ettMeasureInterval = (ettInterval*(ettWindow+1));
        if (!par("EttTimeLimit").boolValue() || ettInterval <= 0)
            ettMeasureInterval = SimTime::getMaxTime();
        maxLive = par("TimeToLive");
        powerWindow = par("powerWindow");
        powerWindowTime = par("powerWindowTime");
        hysteresis = par("ETXHysteresis");
        pasiveMeasure = par("pasiveMeasure");
        subscribe(NF_LINK_BREAK,this);
        subscribe(NF_LINK_FULL_PROMISCUOUS,this);
        IInterfaceTable *inet_ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        for (int i = 0; i < inet_ift->getNumInterfaces(); i++)
        {
            InterfaceEntry * ie = inet_ift->getInterface(i);
            if (ie->getMacAddress()==myAddress)
                ie->setEstimateCostProcess(par("Index").longValue(), this);
        }
        if (etxSize>0 && etxInterval>0 && !pasiveMeasure)
            scheduleAt(simTime()+par("startEtx"), etxTimer);
        if (ettInterval>0 && ettSize1>0 && ettSize2>0 && !pasiveMeasure)
        {
            // integrity check
            if (etxSize <0  || etxInterval < 0)
                throw cRuntimeError("ETT need ETX");
            if (ettInterval/etxInterval < 2)
                throw cRuntimeError("ETT interval must be at least 2 times the ETX interval");
            scheduleAt(simTime()+par("startEtt"), ettTimer);
        }
    }
}


Ieee80211Etx::~Ieee80211Etx()
{
    for (unsigned int i = 0; i<neighbors.size(); i++)
    {
        neighbors[i].clear();
    }
    neighbors.clear();
    cancelAndDelete(etxTimer);
    cancelAndDelete(ettTimer);
}

void Ieee80211Etx::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        // process timers
        EV << "Timer expired: " << msg << "\n";
        handleTimer(msg);
        return;
    }
    if (dynamic_cast<MACETXPacket *>(msg))
    {
        handleEtxMessage(dynamic_cast<MACETXPacket *>(msg));
    }
    else if (dynamic_cast<MACBwPacket *>(msg))
    {
        handleBwMessage(dynamic_cast<MACBwPacket *>(msg));
    }
}

void Ieee80211Etx::handleTimer(cMessage *msg)
{
    if (msg == etxTimer)
    {
        for (unsigned int i = 0; i< neighbors.size(); i++)
        {
            MACETXPacket *pkt = new MACETXPacket();
            pkt->setByteLength(etxSize);
            pkt->setSource(myAddress);
            pkt->setDest(MACAddress::BROADCAST_ADDRESS);
            for (auto it = neighbors[i].begin(); it != neighbors[i].end();)
            {
                if (simTime() - it->second.getTime() > maxLive)
                {
                    auto itAux = it;
                    it++;
                    neighbors[i].erase(itAux);
                    continue;
                }
                it++;
            }
            pkt->setNeighborsArraySize(neighbors[i].size());
            pkt->setRecPacketsArraySize(neighbors[i].size());
            int j = 0;
            for (auto it = neighbors[i].begin(); it != neighbors[i].end(); it++)
            {
                pkt->setNeighbors(j, it->second.getAddress());
                checkSizeEtxArray(&(it->second));
                checkSizeEttArray(&(it->second));
                pkt->setRecPackets(j, it->second.timeVector.size());
                j++;
            }

            if (!pkt->hasPar("indexGate"))
                pkt->addPar("indexGate") = i;
            else
                pkt->par("indexGate") = i;
            send(pkt, "toMac");
        }
        if (!pasiveMeasure)
            scheduleAt(simTime()+par("ETXjitter")+etxInterval, etxTimer);
    }
    else if (msg == ettTimer)
    {
        for (unsigned int i = 0; i < neighbors.size(); i++)
        {
            for (auto it = neighbors[i].begin(); it != neighbors[i].end();)
            {
                if (simTime() - it->second.getTime() > maxLive)
                {
                    auto itAux = it;
                    it++;
                    neighbors[i].erase(itAux);
                    continue;
                }
                MACBwPacket *pkt1 = new MACBwPacket();
                MACBwPacket *pkt2 = new MACBwPacket();
                pkt1->setSource(myAddress);
                pkt2->setSource(myAddress);
                pkt1->setByteLength(ettSize1);
                pkt2->setByteLength(ettSize2);
                pkt1->setDest(it->second.getAddress());
                pkt2->setDest(it->second.getAddress());
                it->second.setEttTime(simTime());
                pkt1->setType(0);
                pkt2->setType(0);

                if (!pkt1->hasPar("indexGate"))
                    pkt1->addPar("indexGate") = i;
                 else
                     pkt1->par("indexGate") = i;

                if (!pkt2->hasPar("indexGate"))
                    pkt2->addPar("indexGate") = i;
                else
                    pkt2->par("indexGate") = i;

                pkt1->setIndex(ettIndex);
                pkt2->setIndex(ettIndex);
                ettIndex ++;
                send(pkt1, "toMac");
                send(pkt2, "toMac");
                it++;
            }
        }
        if (!pasiveMeasure)
            scheduleAt(simTime() + par("ETXjitter") + ettInterval, ettTimer);
    }
}

int Ieee80211Etx::getEtx(const MACAddress &add, double &val)
{
    MacEtxNeighbor *neig;
    val = 1e300;
    int interface = -1;
    for (unsigned int i = 0; i < neighbors.size(); i++)
    {
        auto it = neighbors[i].find(add);
        if (it == neighbors[i].end())
        {
            continue;
        }
        else
        {
            neig = &(it->second);
            int expectedPk = etxMeasureInterval / etxInterval;
            int pkRec = neig->timeVector.size();
            double pr = (double) pkRec / (double) expectedPk;
            double ps = (double) neig->getPackets() / (double) expectedPk;
            if (pr > 1)
                pr = 1;
            if (ps > 1)
                ps = 1;
            double results;
            if (ps == 0 || pr == 0)
                results = 1e100;
            results = 1 / (ps * pr);
            if (results<val)
            {
                val = results;
                interface = (int)i;
            }
        }
    }
    return interface;
}

double Ieee80211Etx::getEtx(const MACAddress &add, const int &iface)
{
    auto it = neighbors[iface].find(add);
    MacEtxNeighbor *neig;
    if (it==neighbors[iface].end())
    {
        return -1;
    }
    else
    {
        neig = &(it->second);
        int expectedPk = etxMeasureInterval/etxInterval;
        int pkRec = neig->timeVector.size();
        double pr = (double) pkRec/(double) expectedPk;
        double ps = (double) neig->getPackets()/(double)expectedPk;
        if (pr>1) pr = 1;
        if (ps>1) ps = 1;
        if (ps == 0 || pr==0)
            return 1e300;
        return 1/(ps*pr);
    }
}


int Ieee80211Etx::getEtxEtt(const MACAddress &add, double &etx, double &ett)
{
    ett = 1e300;
    etx = 1e300;
    int interface = -1;
    for (unsigned int i = 0; i < neighbors.size(); i++)
    {
        auto it = neighbors[i].find(add);
        MacEtxNeighbor *neig;
        if (it == neighbors[i].end())
        {
            continue;
        }
        else
        {

            neig = &(it->second);
            int expectedPk = etxMeasureInterval / etxInterval;
            int pkRec = neig->timeVector.size();
            double pr = (double) pkRec / (double) expectedPk;
            double ps = (double) neig->getPackets() / (double) expectedPk;
            if (pr > 1)
                pr = 1;
            if (ps > 1)
                ps = 1;
            double resultEtx;
            if (ps == 0 || pr == 0)
                resultEtx = 1e100;
            resultEtx = 1 / (ps * pr);

            simtime_t minTime = 100.0;
            double resultEtt = 1e300;
            if (!neig->timeETT.empty())
            {
                for (unsigned int i = 0; i < neig->timeETT.size(); i++)
                    if (minTime > neig->timeETT[i].delay)
                        minTime = neig->timeETT[i].delay;
                double bw = (double) ettSize2 / SIMTIME_DBL(minTime);
                resultEtt = resultEtx * (etxSize / bw);
            }
            if (resultEtx < etx || (resultEtx <=etx && resultEtt < ett ))
            {
                etx = resultEtx;
                ett = resultEtt;
                interface = (int)i;
            }
        }
    }
    return interface;
}


double Ieee80211Etx::getEtt(const MACAddress &add, const int &iface)
{
    if (ettInterval<=0 || ettSize1 <= 0 || ettSize2<=0)
	    return -1;

    auto it = neighbors[iface].find(add);
    MacEtxNeighbor *neig;
    if (it==neighbors[iface].end())
    {
        return -1;
    }
    else
    {
        neig = &(it->second);
        if (neig->timeETT.empty())
            return -1;
        int expectedPk = etxMeasureInterval/etxInterval;
        int pkRec = neig->timeVector.size();
        double pr = (double) pkRec/ (double)expectedPk;
        double ps = (double) neig->getPackets()/(double) expectedPk;
        if (pr>1) pr = 1;
        if (ps>1) ps = 1;
        if (ps == 0 || pr==0)
            return 1e300;
        double etx =  1/(ps*pr);
        simtime_t minTime = 10000.0;
        for (unsigned int i = 0; i<neig->timeETT.size(); i++)
            if (minTime>neig->timeETT[i].delay)
                minTime = neig->timeETT[i].delay;
        double bw = ettSize2/minTime;
        return etx*(etxSize/bw);
    }
}


int Ieee80211Etx::getEtt(const MACAddress &add, double &val)
{
    if (ettInterval <= 0 || ettSize1 <= 0 || ettSize2 <= 0)
        return -1;
    val = 1e300;
    int interface = -1;
    for (unsigned int i = 0; i < neighbors.size(); i++)
    {
        auto it = neighbors[i].find(add);
        MacEtxNeighbor *neig;
        if (it == neighbors[i].end())
        {
            continue;
        }
        else
        {
            neig = &(it->second);
            if (neig->timeETT.empty())
                continue;
            int expectedPk = etxMeasureInterval / etxInterval;
            int pkRec = neig->timeVector.size();
            double pr = (double) pkRec / (double)expectedPk;
            double ps = (double) neig->getPackets() / (double) expectedPk;
            if (pr > 1)
                pr = 1;
            if (ps > 1)
                ps = 1;
            double result;
            if (ps == 0 || pr == 0)
                result = 1e100;
            else
            {
                double etx = 1 / (ps * pr);
                simtime_t minTime = 100.0;
                for (unsigned int i = 0; i < neig->timeETT.size(); i++)
                    if (minTime > neig->timeETT[i].delay)
                        minTime = neig->timeETT[i].delay;
                double bw = (double) ettSize2 / SIMTIME_DBL(minTime);
                result = etx * (etxSize / bw);
            }
            if (result<val)
            {
                val = result;
                interface = (int)i;
            }
        }
    }
    return interface;
}

double Ieee80211Etx::getPrec(const MACAddress &add, const int &iface)
{
    auto it = neighbors[iface].find(add);
    MacEtxNeighbor *neig;
    if (it==neighbors[iface].end())
    {
        return 0;
    }
    else
    {
        neig = &(it->second);
        if (neig->signalToNoiseAndSignal.empty())
            return 0;

        double sum = 0;

        for (auto itNeig = neig->signalToNoiseAndSignal.begin(); itNeig!=neig->signalToNoiseAndSignal.end();)
        {
            if ((simTime()- itNeig->snrTime)>powerWindowTime)
            {
                auto itAux = itNeig+1;
                neig->signalToNoiseAndSignal.erase(itNeig);
                itNeig = itAux;
                continue;
            }
            sum += itNeig->signalPower;
            itNeig++;
        }
        return sum/neig->signalToNoiseAndSignal.size();
    }
}


int Ieee80211Etx::getPrec(const MACAddress &add, double &val)
{
    val = 0;
    int interface = -1;
    for (unsigned int i = 0; i < neighbors.size(); i++)
    {
        auto it = neighbors[i].find(add);
        MacEtxNeighbor *neig;
        if (it == neighbors[i].end())
        {
            continue;
        }
        else
        {
            neig = &(it->second);
            if (neig->signalToNoiseAndSignal.empty())
                continue;

            double sum = 0;

            for (auto itNeig = neig->signalToNoiseAndSignal.begin(); itNeig != neig->signalToNoiseAndSignal.end();)
            {
                if ((simTime() - itNeig->snrTime) > powerWindowTime)
                {
                    auto itAux = itNeig + 1;
                    neig->signalToNoiseAndSignal.erase(itNeig);
                    itNeig = itAux;
                    continue;
                }
                sum += itNeig->signalPower;
                itNeig++;
            }

            double result = sum / (double) neig->signalToNoiseAndSignal.size();
            if (result>val)
            {
                val = result;
                interface = (int)i;
            }
        }
    }
    return interface;
}

double Ieee80211Etx::getSignalToNoise(const MACAddress &add, const int &iface)
{
    auto it = neighbors[iface].find(add);
    MacEtxNeighbor *neig;
    if (it==neighbors[iface].end())
    {
        return 0;
    }
    else
    {
        neig = &(it->second);
        if (neig->signalToNoiseAndSignal.empty())
            return 0;

        double sum = 0;

        for (auto itNeig = neig->signalToNoiseAndSignal.begin(); itNeig!=neig->signalToNoiseAndSignal.end();)
        {
            if ((simTime()- itNeig->snrTime)>powerWindowTime)
            {
                auto itAux = itNeig+1;
                neig->signalToNoiseAndSignal.erase(itNeig);
                itNeig = itAux;
                continue;
            }
            sum += itNeig->snrData;
            itNeig++;
        }
        return sum/neig->signalToNoiseAndSignal.size();
    }
}

int Ieee80211Etx::getSignalToNoise(const MACAddress &add, double &val)
{
    MacEtxNeighbor *neig;
    val = 0;
    int interface = -1;
    for (unsigned int i = 0; i < neighbors.size(); i++)
    {
        auto it = neighbors[i].find(add);
        if (it == neighbors[i].end())
        {
            continue;
        }
        else
        {
            neig = &(it->second);
            if (neig->signalToNoiseAndSignal.empty())
                continue;

            double sum = 0;

            for (auto itNeig = neig->signalToNoiseAndSignal.begin(); itNeig != neig->signalToNoiseAndSignal.end();)
            {
                if ((simTime() - itNeig->snrTime) > powerWindowTime)
                {
                    auto itAux = itNeig + 1;
                    neig->signalToNoiseAndSignal.erase(itNeig);
                    itNeig = itAux;
                    continue;
                }
                sum += itNeig->snrData;
                itNeig++;
            }
            double result = sum / (double) neig->signalToNoiseAndSignal.size();
            if (result>val)
            {
                val = result;
                interface = (int)i;
            }
        }
    }
    return interface;
}

double Ieee80211Etx::getPacketErrorToNeigh(const MACAddress &add, const int &iface)
{
    auto it = neighbors[iface].find(add);
    MacEtxNeighbor *neig;

    if (it == neighbors[iface].end())
    {
        return -1;
    }
    else
    {
        neig = &(it->second);
        int expectedPk = etxMeasureInterval / etxInterval;
        double ps = (double) neig->getPackets() / (double) expectedPk;
        if (ps > 1)
            ps = 1;
        return 1 - ps;
    }
}


int Ieee80211Etx::getPacketErrorToNeigh(const MACAddress &add, double &val)
{
    MacEtxNeighbor *neig;
    val = 1;
    int interface = -1;
    for (unsigned int i = 0; i < neighbors.size(); i++)
    {
        auto it = neighbors[i].find(add);
        if (it == neighbors[i].end())
        {
            continue;
        }
        else
        {
            neig = &(it->second);
            int expectedPk = etxMeasureInterval / etxInterval;
            double ps = (double) neig->getPackets() / (double) expectedPk;
            if (ps > 1)
                ps = 1;
            double resul = 1 - ps;
            if (val>resul)
            {
                interface = i;
                val = resul;
            }
        }
    }
    return interface;
}

double Ieee80211Etx::getPacketErrorFromNeigh(const MACAddress &add, const int &iface)
{
    auto it = neighbors[iface].find(add);
    MacEtxNeighbor *neig;
    if (it==neighbors[iface].end())
    {
        return -1;
    }
    else
    {
        neig = &(it->second);
        int expectedPk = etxMeasureInterval/etxInterval;
        int pkRec = neig->timeVector.size();
        double pr = (double) pkRec/ (double)expectedPk;
        if (pr>1) pr = 1;
        return 1-pr;
    }
}

int Ieee80211Etx::getPacketErrorFromNeigh(const MACAddress &add, double &val)
{
    val = 1;
    int interface = -1;
    for (unsigned int i = 0; i < neighbors.size(); i++)
    {
        auto it = neighbors[i].find(add);
        MacEtxNeighbor *neig;
        if (it == neighbors[i].end())
        {
            continue;
        }
        else
        {
            neig = &(it->second);
            int expectedPk = etxMeasureInterval / etxInterval;
            int pkRec = neig->timeVector.size();
            double pr = (double) pkRec / (double)expectedPk;
            if (pr > 1)
                pr = 1;
            double resul = 1 - pr;
            if (val > resul)
            {
                interface = i;
                val = resul;
            }
        }
    }
    return interface;
}


void Ieee80211Etx::handleEtxMessage(MACETXPacket *msg)
{
    int interface = msg->getKind();
    if (interface<0 || interface>= (int)numInterfaces)
        interface = 0;
    auto it = neighbors[interface].find(msg->getSource());
    MacEtxNeighbor *neig;
    if (it==neighbors[interface].end())
    {
        MacEtxNeighbor aux;
        aux.setAddress(msg->getSource());
        neighbors[interface][msg->getSource()] = aux;
        it = neighbors[interface].find(msg->getSource());
    }

    neig = &(it->second);
    checkSizeEtxArray(neig);
    neig->timeVector.push_back(simTime());
    neig->setTime(simTime());
    neig->setPackets(0);
    neig->setNumFailures(0);
    for (unsigned int i = 0; i<msg->getNeighborsArraySize(); i++)
    {
        if (myAddress==msg->getNeighbors(i))
        {
            neig->setPackets(msg->getRecPackets(i));
            break;
        }
    }
    //
    if (GlobalWirelessLinkInspector::isActive())
    {
        // compute and actualize the costs
        GlobalWirelessLinkInspector::Link link;
        L3Address org = L3Address(this->myAddress);
        L3Address dest = L3Address(msg->getSource());

        double etx;
        double ett;
        getEtxEtt(msg->getSource(),etx,ett);

        if (GlobalWirelessLinkInspector::getLinkCost(org,dest,link))
        {
            link.costEtx = etx;
            link.costEtt = ett;
            GlobalWirelessLinkInspector::setLinkCost(org,dest,link);
        }
        else
        {
            link.costEtt = ett;
            link.costEtx = etx;
            link.snr = 0;
            GlobalWirelessLinkInspector::setLinkCost(org,dest,link);
        }
    }

    delete msg;
}

void Ieee80211Etx::handleBwMessage(MACBwPacket *msg)
{
    int interface = msg->getKind();
    if (interface<0 || interface>= (int)numInterfaces)
        interface = 0;
    auto it = neighbors[interface].find(msg->getSource());
    MacEtxNeighbor *neig = nullptr;
    if (it!=neighbors[interface].end())
    {
        neig = &(it->second);
        neig->setNumFailures(0);
    }

    if (!msg->getType())
    {
        if (msg->getByteLength() == ettSize1)
        {
            InfoEttData infoEttData;
            infoEttData.ettIndex = msg->getIndex();
            infoEttData.prevTime = simTime();
            infoEtt[msg->getSource()] = infoEttData;
        }
        else if (msg->getByteLength() == ettSize2)
        {
            auto it = infoEtt.find(msg->getSource());
            if (it != infoEtt.end())
            {
                if (msg->getIndex() == it->second.ettIndex) // if different index some packet of the pair has been lost
                {
                    msg->setTime(simTime()-it->second.prevTime);
                    msg->setType(1);
                    msg->setDest(msg->getSource());
                    msg->setByteLength(ettSize1);
                    msg->setSource(myAddress);
                    send(msg, "toMac");
                    msg = nullptr;
                }
                infoEtt.erase(it);
            }
        }
        if (msg)
            delete msg;
        return;
    }
    if (!neig)
    {
        neig = new MacEtxNeighbor;
        MacEtxNeighbor aux;
        aux.setAddress(msg->getSource());
        neighbors[interface][msg->getSource()] = aux;
        it = neighbors[interface].find(msg->getSource());
        neig = &(it->second);
    }
    if (msg->getByteLength() == ettSize1)
    {
        if (neig->timeETT.size() > (unsigned int)ettWindow)
            neig->timeETT.erase(neig->timeETT.begin());
        MacEtxNeighbor::ETTData data;
        data.delay = msg->getTime();
        data.recordTime = simTime();
        neig->timeETT.push_back(data);

        if (GlobalWirelessLinkInspector::isActive())
        {
            // compute and actualize the costs
            GlobalWirelessLinkInspector::Link link;
            L3Address org = L3Address(this->myAddress);
            L3Address dest = L3Address(msg->getSource());
            double ett;
            getEtt(msg->getSource(),ett);

            if (GlobalWirelessLinkInspector::getLinkCost(org,dest,link))
            {
                if (link.costEtt != ett)
                {
                    link.costEtt = ett;
                    GlobalWirelessLinkInspector::setLinkCost(org,dest,link);
                }
            }
            else
            {
                link.costEtx = 0;
                link.costEtt = ett;
                link.snr = 0;
                GlobalWirelessLinkInspector::setLinkCost(org,dest,link);
            }
        }
    }
    delete msg;
    return;
}

void Ieee80211Etx::getNeighbors(std::vector<MACAddress> & add,const int &iface)
{
    Enter_Method_Silent();
    add.clear();
    for (auto it = neighbors[iface].begin(); it != neighbors[iface].end(); it++)
    {
        add.push_back(it->second.getAddress());
    }
    return;
}

void Ieee80211Etx::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj)
{
    Enter_Method("Ieee80211Etx llf");
    if (obj==nullptr)
        return;
    Ieee80211TwoAddressFrame *frame = dynamic_cast<Ieee80211TwoAddressFrame *>(const_cast<cObject*> (obj));
    if (frame==nullptr)
        return;
    int index = frame->getKind();
    if (signalID == NF_LINK_BREAK)
    {
        auto it = neighbors[index].find(frame->getReceiverAddress());
        if (it!=neighbors[index].end())
        {
            it->second.setNumFailures(it->second.getNumFailures()+1);
            if (it->second.getNumFailures()>1)
            {
                neighbors[index].erase(it);
            }
        }
    }
    else if (signalID == NF_LINK_FULL_PROMISCUOUS)
    {
        auto it = neighbors[index].find(frame->getTransmitterAddress());
        if (it!=neighbors[index].end())
            it->second.setNumFailures(0);
        if (powerWindow>0)
        {
            Ieee80211ReceptionIndication * cinfo = dynamic_cast<Ieee80211ReceptionIndication *> (frame->getControlInfo());
            // use only data frames
            if (!dynamic_cast<Ieee80211DataFrame *>(frame))
                return;
            if (cinfo)
            {
                if (it == neighbors[index].end())
                {
                    // insert new element
                    MacEtxNeighbor neig;
                    neig.setAddress(frame->getTransmitterAddress());
                    neighbors[index].insert(std::pair<MACAddress, MacEtxNeighbor>(frame->getTransmitterAddress(),neig));
                    it = neighbors[index].find(frame->getTransmitterAddress());
                }

                MacEtxNeighbor *ng = &(it->second);

                if (!ng->signalToNoiseAndSignal.empty())
                {
                    while ((int)ng->signalToNoiseAndSignal.size()>powerWindow-1)
                        ng->signalToNoiseAndSignal.erase(ng->signalToNoiseAndSignal.begin());
                    while (simTime() - ng->signalToNoiseAndSignal.front().snrTime > powerWindowTime && !ng->signalToNoiseAndSignal.empty())
                        ng->signalToNoiseAndSignal.erase(ng->signalToNoiseAndSignal.begin());
                }

                SNRDataTime snrDataTime;
                snrDataTime.signalPower = cinfo->getRecPow();
                snrDataTime.snrData = cinfo->getSnr();
                snrDataTime.snrTime = simTime();
                snrDataTime.testFrameDuration = cinfo->getTestFrameDuration();
                snrDataTime.testFrameError = cinfo->getTestFrameError();
                snrDataTime.airtimeMetric = cinfo->getAirtimeMetric();
                if (snrDataTime.airtimeMetric)
                    snrDataTime.airtimeValue = (uint32_t)ceil((snrDataTime.testFrameDuration/10.24e-6)/(1-snrDataTime.testFrameError));
                else
                    snrDataTime.airtimeValue = 0xFFFFFFF;
                ng->signalToNoiseAndSignal.push_back(snrDataTime);
                if (snrDataTime.airtimeMetric)
                {
                    // found the best
                    uint32_t cost = 0xFFFFFFFF;

                    for (unsigned int i = 0; i < ng->signalToNoiseAndSignal.size(); i++)
                    {

                        if (ng->signalToNoiseAndSignal[i].airtimeMetric && cost > ng->signalToNoiseAndSignal[i].airtimeValue)
                            cost = ng->signalToNoiseAndSignal[i].airtimeValue;
                    }
                    ng->setAirtimeMetric(cost);
                }
                else
                    ng->setAirtimeMetric(0xFFFFFFF);
            }
        }
    }
}


uint32_t Ieee80211Etx::getAirtimeMetric(const MACAddress &addr, const int &iface)
{
    auto it = neighbors[iface].find(addr);
    if (it != neighbors[iface].end())
    {
        MacEtxNeighbor ng = it->second;
        while (!ng.signalToNoiseAndSignal.empty() && (simTime() - ng.signalToNoiseAndSignal.front().snrTime > powerWindowTime))
            ng.signalToNoiseAndSignal.erase(ng.signalToNoiseAndSignal.begin());
        if (ng.signalToNoiseAndSignal.empty() && (simTime() - ng.getTime() > maxLive))
        {
            neighbors[iface].erase(it);
            return 0xFFFFFFF;
        }
        else if (ng.signalToNoiseAndSignal.empty())
            return 0xFFFFFFF;
        else
            return ng.getAirtimeMetric();
    }
    else
        return 0xFFFFFFF;
}

void Ieee80211Etx::getAirtimeMetricNeighbors(std::vector<MACAddress> &addr, std::vector<uint32_t> &cost, const int &iface)
{
    addr.clear();
    cost.clear();
    for (auto it = neighbors[iface].begin(); it != neighbors[iface].end();)
    {
        MacEtxNeighbor ng = it->second;
        while (simTime() - ng.signalToNoiseAndSignal.front().snrTime > powerWindowTime)
            ng.signalToNoiseAndSignal.erase(ng.signalToNoiseAndSignal.begin());
        if (ng.signalToNoiseAndSignal.empty() && (simTime() - ng.getTime() > maxLive))
        {
            auto itAux = it;
            it++;
            neighbors[iface].erase(itAux);
        }
        else if (ng.signalToNoiseAndSignal.empty())
        {
            it++;
        }
        else if (ng.signalToNoiseAndSignal.empty())
        {
            addr.push_back(it->first);
            cost.push_back(it->second.getAirtimeMetric());
            it++;
        }
    }
}



void Ieee80211Etx::setNumInterfaces(unsigned int iface)
{
    if (iface == 0)
        return;
    numInterfaces = iface;
    if (neighbors.size()<numInterfaces)
    {

        for (unsigned int i = numInterfaces; i< neighbors.size(); i++)
        {
            neighbors[i].clear();
        }
    }
    neighbors.resize(numInterfaces);
}

std::string Ieee80211Etx::detailedInfo() const
{
   return info();
}

std::string Ieee80211Etx::info() const
{
    std::stringstream out;
    for (unsigned int i = 0; i < neighbors.size(); ++i)
    {
        out << "interface : " << i << "num neighbors :" << neighbors[i].size() <<"\n";
        for (NeighborsMap::const_iterator it = neighbors[i].begin(); it != neighbors[i].end(); it++)
        {
            out << "address : " << it->second.getAddress().str() <<"\n";;
        }
    }
    return out.str();
}

}

}
