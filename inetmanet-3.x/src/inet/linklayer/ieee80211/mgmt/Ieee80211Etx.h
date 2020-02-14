//
// Copyright (C) 2006 Andras Varga
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

#ifndef IEEE80211_ETX_ADHOC_H
#define IEEE80211_ETX_ADHOC_H

#include "inet/common/INETDefs.h"

#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/linklayer/ieee80211/mgmt/ETXPacket_m.h"

namespace inet {

namespace ieee80211 {

/**
 *
 * @author Alfonso Ariza
 */
 struct SNRDataTime {
    double snrData;
    double signalPower;
    simtime_t snrTime;
    bool airtimeMetric;
    double testFrameDuration;
    double testFrameError;
    uint32_t airtimeValue;

    SNRDataTime& operator=(const SNRDataTime& other)
    {
        if (this==&other) return *this;
        this->snrData = other.snrData;
        this->snrTime = other.snrTime;
        this->signalPower = other.signalPower;
        this->airtimeMetric=other.airtimeMetric;
        this->testFrameDuration=other.testFrameDuration;
        this->testFrameError=other.testFrameError;
        this->airtimeValue = other.airtimeValue;
        return *this;
    }
}; // Store information about the SNR and the time that that measure was store

class MacEtxNeighbor
{
  private:
    MACAddress address;
    simtime_t  time;
    simtime_t  ettTime;
    simtime_t  ett1Time;
    simtime_t  ett2Time;
    uint32_t   airTimeMetric;
    int     packets;
    int     numFailures;
  public:
    std::vector<simtime_t> timeVector;
    class ETTData
    {
        public:
            simtime_t recordTime;
            simtime_t delay;
    };
    std::vector<ETTData> timeETT;
    std::vector<SNRDataTime> signalToNoiseAndSignal; // S/N received
  public:
    MacEtxNeighbor() {packets = 0; time = 0; numFailures = 0;}
    ~MacEtxNeighbor()
    {
        timeVector.clear();
        timeETT.clear();
        signalToNoiseAndSignal.clear();
    }

    MacEtxNeighbor& operator=(const MacEtxNeighbor& other)
    {
        if (this==&other) return *this;
        address = other.address;
        time = other.time;
        ettTime = other.ettTime;
        ett1Time = other.ett1Time;
        ett2Time = other.ett2Time;
        airTimeMetric = other.airTimeMetric;
        packets = other.packets;
        numFailures = other.numFailures;
        timeVector = other.timeVector;
        timeETT =  other.timeETT;
        signalToNoiseAndSignal = other.signalToNoiseAndSignal;
        return *this;
    }

    // this vector store a window of values
    void setAddress(const MACAddress &addr) {address = addr;}
    MACAddress getAddress() const {return address;}
    void setTime(const simtime_t &t) {time = t;}
    simtime_t getTime() const {return time;}

    void setEttTime(const simtime_t &t) {ettTime = t;}
    simtime_t getEttTime() const {return ettTime;}
    void setEtt1Time(const simtime_t &t) {ett1Time = t;}
    simtime_t getEtt1Time() const {return ett1Time;}
    void setEtt2Time(const simtime_t &t) {ett2Time = t;}
    simtime_t getEtt2Time() const {return ett2Time;}

    void setNumFailures(int num) {numFailures = num;}
    int getNumFailures() {return numFailures;}

    void setPackets(const int &p) {packets = p;}
    int getPackets() const {return packets;}

    uint32_t getAirtimeMetric() const {return airTimeMetric;}
    void  setAirtimeMetric(uint32_t p) {airTimeMetric = p;}
};

typedef std::map<MACAddress,MacEtxNeighbor> NeighborsMap;

class INET_API Ieee80211Etx : public cSimpleModule, public MacEstimateCostProcess, public cListener
{
    enum CostTypes
    {
        ETT,
        ETX,
        POWER_REC,
        SIGNALTONOISE
    };
  protected:

    MACAddress myAddress;
    std::vector<NeighborsMap> neighbors;
    cMessage * etxTimer;
    cMessage * ettTimer;
    simtime_t etxInterval;
    simtime_t ettInterval;
    simtime_t etxMeasureInterval;
    simtime_t ettMeasureInterval;
    int ettWindow;
    int etxSize;
    int ettSize1;
    int ettSize2;
    simtime_t maxLive;
    bool pasiveMeasure;

    double hysteresis;
    long unsigned int ettIndex;
    class InfoEttData
    {
        public:
            long unsigned int ettIndex;
            simtime_t  prevTime;
    };
    typedef std::map<MACAddress,InfoEttData> InfoEtt;
    InfoEtt infoEtt;

    int powerWindow;
    simtime_t powerWindowTime;
    unsigned int numInterfaces;

    void checkSizeEtxArray(MacEtxNeighbor *neig)
    {
        while (!neig->timeVector.empty() && (simTime() - neig->timeVector.front() > (etxMeasureInterval + hysteresis)))
            neig->timeVector.erase(neig->timeVector.begin());
        while (neig->timeVector.size() > etxMeasureInterval / etxInterval)
            neig->timeVector.erase(neig->timeVector.begin());
    }

    void checkSizeEttArray(MacEtxNeighbor *neig)
    {
        if (neig->timeETT.empty())
            return;
        while (simTime() - neig->timeETT.front().recordTime > ettMeasureInterval)
            neig->timeETT.erase(neig->timeETT.begin());
    }

  protected:
    virtual int numInitStages() const {return 3;}
    virtual void initialize(int);

    virtual ~Ieee80211Etx();

    virtual void handleMessage(cMessage*);
    /** Implements abstract to use routing protocols in the mac layer */
    virtual void handleEtxMessage(MACETXPacket *);
    /** Implements abstract Ieee80211MgmtBase method */
    virtual void handleTimer(cMessage *msg);
    /** Implements abstract Ieee80211MgmtBase method */
    virtual void handleBwMessage(MACBwPacket *);
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj);
  public:
    virtual double getEtx(const MACAddress &add,const int &iface = 0);
    virtual int getEtx(const MACAddress &add, double &val);
    virtual int getEtxEtt(const MACAddress &add, double &etx, double &ett);


    virtual double getEtt(const MACAddress &add,const int &iface = 0);
    virtual int getEtt(const MACAddress &add, double &val);

    virtual double getPrec(const MACAddress &add, const int &iface = 0);
    virtual int getPrec(const MACAddress &add, double &val);

    virtual double getSignalToNoise(const MACAddress &add, const int &iface = 0);
    virtual int getSignalToNoise(const MACAddress &add, double &val);

    virtual double getPacketErrorToNeigh(const MACAddress &add, const int &iface = 0);
    virtual int getPacketErrorToNeigh(const MACAddress &add, double &val);

    virtual double getPacketErrorFromNeigh(const MACAddress &add, const int &iface = 0);
    virtual int getPacketErrorFromNeigh(const MACAddress &add, double &val);


    virtual void getNeighbors(std::vector<MACAddress> &,const int &iface = 0);

    uint32_t getAirtimeMetric(const MACAddress &addr, const int &iface = 0);

    void getAirtimeMetricNeighbors(std::vector<MACAddress> &addr, std::vector<uint32_t> &cost, const int &iface = 0);

    virtual void setNumInterfaces(unsigned int iface);
    unsigned int  getNumInterfaces() {return numInterfaces;}
    std::string info() const;
  public:
    std::string detailedInfo() const;
    Ieee80211Etx() {setNumInterfaces(1);}
    void setAddress(const MACAddress &add) {myAddress = add;}
    virtual double getCost(int i, MACAddress &add)
    {
        switch (i)
        {
        case ETT:
            return getEtt(add);
            break;
        case ETX:
            return getEtx(add);
            break;
        case POWER_REC:
            return getPrec(add);
            break;
        case SIGNALTONOISE:
            return getSignalToNoise(add);
            break;
        default:
            return -1;
            break;
        }
    }

    virtual double getNumCost() {return 4;}
    virtual int getNumNeighbors() {return neighbors.size();}
    virtual int getNeighbors(std::vector<MACAddress> &add)
    {
        getNeighbors(add,0);
        return (int)add.size();
    }
};

}
}
#endif


