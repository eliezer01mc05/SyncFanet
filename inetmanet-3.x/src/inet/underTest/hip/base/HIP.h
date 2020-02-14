//*********************************************************************************
// File:           HIP.h
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

#ifndef __HIP_H__
#define __HIP_H__

#include "inet/common/INETDefs.h"
#include "inet/networklayer/contract/ipv6/IPv6Address.h"
#include "inet/underTest/hip/base/HipMessages_m.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/common/INETDefs.h"
#include "inet/networklayer/contract/IInterfaceTable.h"


#define HIP_START_CHECK_TIMER 4
#define HIP_CHECK_TIMER 0.1

namespace inet {


class INET_API HIP : public cSimpleModule, public cListener
{

   protected:
    // Connection attempts waiting for DNS response
	typedef std::map<IPv6Address, cMessage *> ListHITtoTriggerDNS;
	ListHITtoTriggerDNS listHITtoTriggerDNS;
	ListHITtoTriggerDNS::iterator listHITtoTriggerDNSIt;


	bool firstUpdate;
	bool expectingDnsResp;
	bool expectingRvsDnsResp;
	int hipMsgSent;
	cOutVector hipVector;
	int currentIfId;
	bool udpPresent;
	bool tcpPresent;

	IInterfaceTable *ift;

   public:
    //constructor/destructor
    HIP();
    virtual ~HIP();

	void incHipMsgCounter();

	std::map<InterfaceEntry *, IPv6Address> mapIfaceToIP;
	std::map<InterfaceEntry *, bool> mapIfaceToConnected;

	//HIT - IP struct and mapping
    typedef struct HitToIpMapEntry{
		std::list<IPv6Address> addr;
		int fsmId;
	} HitToIpMapEntry;

    typedef std::map<IPv6Address,HitToIpMapEntry *> HitToIpMap;
    HitToIpMap hitToIpMap;
	HitToIpMap::iterator hitToIpMapIt;

	//pointer of FSM instance
    cModuleType *fsmType;

	//Address of the RVS
	IPv6Address rvsIPaddress;

    //working variables
	IPv6Address srcWorkAddress;
	std::vector<std::string> address;
    IPv6Address partnerHITwork;
    int dstSPIwork;
    int fsmIDwork;
    IPv6Address partnerHIT;
    HitToIpMapEntry * hitToIpMapEntryWork;

	//returns the partner HIT
    IPv6Address getPartnerHIT();

    //returns if the HIP module is an  RVS module
    bool isRvs();
	//returns the RVS address
	IPv6Address* getRvsAddress();
	//returns the working variable for source address
	IPv6Address* getSrcWorkAddress();
	//returns the RVS HIT
	IPv6Address* getRvsHit();
	//returns if there was a HIT-IP swap
	//bool isAddressSwitched();

	//void receiveChangeNotification(int category, const cObject * details);
	virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;

  protected:
	virtual int getTempId();
    virtual int numInitStages() const override
    {
        return NUM_INIT_STAGES;
    }
    virtual void initialize(int) override;
    virtual void specInitialize();
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;

    virtual void handleMsgFromTransport(cMessage *msg);
    virtual void handleMsgFromNetwork(cMessage *msg);
    virtual void handleRvsRegistration(cMessage *msg);
    virtual void handleAddressChange();

	//creates a new FSM daemon, returns its pointer
    virtual cModule* createStateMachine(IPv6Address ipAddress, IPv6Address &HIT);

	//searches for an FSM daemon by its module id
    virtual cModule* findStateMachine(int fsmID);

    bool _isRvs;
	IPv6Address _rvsHit;
};
}

#endif
