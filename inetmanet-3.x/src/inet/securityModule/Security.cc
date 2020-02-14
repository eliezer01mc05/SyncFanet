
/**
*Authors Mohamad.Nehme | Mohamad.Sbeiti \@tu-dortmund.de
*
*copyright (C) 2012 Communication Networks Institute (CNI - Prof. Dr.-Ing. Christian Wietfeld)
* at Technische Universitaet Dortmund, Germany
* http://www.kn.e-technik.tu-dortmund.de/
*
*
* This program is free software; you can redistribute it
* and/or modify it under the terms of the GNU General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later
* version.
* For further information see file COPYING
* in the top level directory
********************************************************************************
* This work is part of the secure wireless mesh networks framework, which is currently under development by CNI */

#include "inet/securityModule/Security.h"
#include "inet/linklayer/ieee80211/mgmt/Ieee80211MgmtAP.h"
#include "inet/securityModule/message/securityPkt_m.h"
#include "inet/transportlayer/udp/UDP.h"
#include "inet/networklayer/ipv4/IPv4InterfaceData.h"
#include "inet/linklayer/common/Ieee802Ctrl_m.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/linklayer/ieee80211/mgmt/Ieee80211Primitives_m.h"
#include "inet/common/NotifierConsts.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Mac.h"

#include "inet/transportlayer/tcp/queues/TCPByteStreamRcvQueue.h"

#include <sstream>
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "inet/networklayer/ipv4/IPv4Datagram.h"
#include "inet/transportlayer/udp/UDPPacket_m.h"
#include "inet/common/ModuleAccess.h"

using namespace std;

// delays
//#define c 1.6e-19    // 1.6 x 10^-19
#define enc 89.2873043e-6
#define dec 72.1017e-6
#define mic_add 1.622e-6
#define mic_verify 1.7379e-6
#define rand_gen 60.4234e-6

#define init 2413.404e-6
#define commit 237.459e-6
#define process_commit 211.4525e-6
#define confirm 142.96e-6
#define process_confirm 72.6265e-6

// msg size
#define sae_1   132
#define sae_2   132
#define sae_3   68
#define sae_4   68
#define ampe_1  214
#define ampe_2  214
#define ampe_3  216
#define ampe_4  216
#define group_1 152
#define group_2 132
#define beacon  108




// message kind values for timers
#define MK_AUTH_TIMEOUT         1
#define GTK_TIMEOUT             3
#define PTK_TIMEOUT             4
#define GK_AUTH_TIMEOUT         5
#define MK_BEACON_TIMEOUT       6
#define PMK_TIMEOUT             7

#define MAX_BEACONS_MISSED 600  // beacon lost timeout, in beacon intervals (doesn't need to be integer)

namespace inet {

namespace ieee80211 {

std::ostream& operator<<(std::ostream& os, const Security::LocEntry& e)
{
    os << " Mac Address " << e.macAddr << "\n";
    return os;
}


std::ostream& operator<<(std::ostream& os, const Security::MeshInfo& mesh)
{
    //  Security::MeshStatus status;
             if(mesh.status==0)   os << "state:" << " NOT_AUTHENTICATED ";
        else if(mesh.status==1)   os << "state:" << " AUTHENTICATED ";
        else if(mesh.status==2)   os << "state:" << " COMMITTED ";
        else if(mesh.status==3)   os << "state:" << " CONFIRMED ";
        else if(mesh.status==4)   os << "state:" << " ACCEPTED ";
    os << "Mesh addr=" << mesh.address
            << " ssid=" << mesh.ssid
            //    << " beaconIntvl=" << mesh.beaconInterval
            << " beaconTimeoutMsg=" << mesh.beaconTimeoutMsg
            << " authTimeoutMsg_a =" << mesh.authTimeoutMsg_a
            << " authTimeoutMsg_b =" << mesh.authTimeoutMsg_b
            //   << " PMKTimerMsg=" << mesh.PMKTimerMsg
            << " groupAuthTimeoutMsg=" << mesh.groupAuthTimeoutMsg
            << " authSeqExpected=" << mesh.authSeqExpected;
    return os;
}

#define MK_STARTUP  1

Define_Module(Security);

simsignal_t Security::AuthTimeoutsNrSignal= SIMSIGNAL_NULL;
simsignal_t Security::BeaconsTimeoutNrSignal= SIMSIGNAL_NULL;
simsignal_t Security::deletedFramesSignal= SIMSIGNAL_NULL;
bool Security::statsAlreadyRecorded;
Security::Security()
{
    // TODO Auto-generated constructor stub
    rt = nullptr;
    itable = nullptr;
}

Security::~Security()
{
    /*  if(PMKTimer)
        PMKTimer =nullptr;
    if(GTKTimer)
        GTKTimer =nullptr;*/
}


void Security::initialize(int stage)
{
    if (stage==INITSTAGE_LOCAL)
    {
        statsAlreadyRecorded = false;
        totalAuthTimeout = 0;
        totalBeaconTimeout = 0;
        NrdeletedFrames =0;
        // read parameters
        authenticationTimeout_a = par("authenticationTimeout_a");
        authenticationTimeout_b = par("authenticationTimeout_b");
        groupAuthenticationTimeout = par("groupAuthenticationTimeout");
        ssid = par("ssid").stringValue();
        beaconInterval = par("beaconInterval");
        PMKTimeout = par("PMKTimeout");
        GTKTimeout = par("GTKTimeout");
        PSK =par("PSK").stringValue();

        numAuthSteps = par("numAuthSteps");
        cModule *host = getContainingNode(this);
        host->subscribe(NF_L2_BEACON_LOST,this);

        activeHandshake = par("activeHandshake");

        counter=0;
        IInterfaceTable *ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);

        myIface = nullptr;
        if (!ift)
        {
            myIface = ift->getInterfaceByName(getParentModule()->getFullName());
        }


        EV << "stage \n" << stage << "\n";
        EV << "Init mesh proccess ** mn * \n";
        // read params and init vars
        channelNumber = -1;  // value will arrive from physical layer in receiveChangeNotification()

        //  WATCH(ssid);
        //  WATCH(channelNumber);
       // WATCH(authTimeoutMsg_b);
        WATCH(numAuthSteps);
        WATCH_LIST(meshList);
        //  WATCH(PMK);
        // WATCH_LIST(simpleMeshList);
        WATCH(counter);
        // start beacon timer (randomize startup time)
        beaconTimer = new cMessage("beaconTimer");
        scheduleAt(simTime()+uniform(0, beaconInterval), beaconTimer);
        AuthTime=0;
        this->AuthTimeoutsNrSignal=   registerSignal("AuthTimeoutsNr");
        this->BeaconsTimeoutNrSignal=   registerSignal("BeaconTimeoutsNr");
        this->deletedFramesSignal=   registerSignal("deletedFramesNr");
    }

    if (stage==INITSTAGE_PHYSICAL_ENVIRONMENT)
    {
        // obtain our address from MAC
        unsigned int numMac=0;
        cModule *nic = getContainingNicModule(this);
        cModule *mac = nic->getSubmodule("mac");
        if (mac == nullptr) {
            // search for vector of mac:
            unsigned int numRadios = 0;
            do {
                mac = nic->getSubmodule("mac",numMac);
                if (mac)
                    numMac++;
            } while (mac);

            if (numMac == 0)
                throw cRuntimeError("MAC module not found; it is expected to be next to this submodule and called 'mac'");
            cModule *radio = nullptr;
            do {
                radio = nic->getSubmodule("radio",numRadios);
                if (radio)
                    numRadios++;
            } while (radio);

            if (numRadios != numMac)
                throw cRuntimeError("numRadios != numMac");

            if (numMac > 1) {
                for (unsigned int i = 0 ; i < numMac; i++) {
                    mac = nic->getSubmodule("mac",i);
                    radio = nic->getSubmodule("radio",i);
                    // radioInterfaces.push_back(dynamic_cast<Radio*>(radio));
                }
            }
            mac = nic->getSubmodule("mac",0);
        }

        if (!mac)
            throw cRuntimeError("MAC module not found; it is expected to be next to this submodule and called 'mac'");
        myAddress = check_and_cast<Ieee80211Mac *>(mac)->getAddress();
        EV << "My MAcAddress is: " << myAddress <<endl;
    }


    /** mn */
    else if (stage!=INITSTAGE_NETWORK_LAYER)
        return;

    rt = findModuleFromPar<IIPv4RoutingTable>(par("routingTableModule"), this);
    itable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
}

void Security::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        handleTimer(msg);
    else
        handleResponse(msg);
}

void Security::handleResponse(cMessage *msg)
{
    // EV << "handleResponse() " <<msg->getName() << "\n";
    if(strstr(msg->getName() ,"Beacon")!=nullptr)
    {
        Ieee80211BeaconFrame *frame= (check_and_cast<Ieee80211BeaconFrame *>(msg));
        handleBeaconFrame(frame);
    }

    else if(strstr(msg->getName() ,"Deauth")!=nullptr)
       {
           Ieee80211AuthenticationFrame *frame= (check_and_cast<Ieee80211AuthenticationFrame *>(msg));
           handleDeauthenticationFrame(frame);
       }
    else if (dynamic_cast<Ieee80211AuthenticationFrame *>(msg))
    {
        Ieee80211AuthenticationFrame *frame= (check_and_cast<Ieee80211AuthenticationFrame *>(msg));
        handleSAE(frame);
    }

    else if(strstr(msg->getName() , "AMPE msg 1/4")!=nullptr || strstr(msg->getName() ,"AMPE msg 2/4")!=nullptr
            ||strstr(msg->getName() , "AMPE msg 3/4")!=nullptr || strstr(msg->getName() ,"AMPE msg 4/4")!=nullptr)
    {
        Ieee80211ActionFrame *frame= (check_and_cast<Ieee80211ActionFrame *>(msg));
        handleAMPE(frame);
    }

    //HWMP
     else if (dynamic_cast<Ieee80211ActionMeshFrame *>(msg))
    {
         if(!activeHandshake)
                 {
                     send(msg,"mgmtOut");
                     return;
                 }
       handleIeee80211ActionMeshFrame(msg);
    }


    else if (dynamic_cast<Ieee80211MeshFrame *>(msg))
    {
        if(!activeHandshake)
        {
            send(msg,"mgmtOut");
            return;
        }
        Ieee80211MeshFrame *frame = (check_and_cast<Ieee80211MeshFrame *>(msg));
        double delay=0;
        if(strstr(msg->getName() ,"EncBrodcast"))
        {
            delay= (double) dec + mic_verify;
            //  send(msg,"mgmtOut");
            MeshInfo *mesh = lookupMesh(frame->getTransmitterAddress());
            if(mesh)
            {
                if(mesh->status==NOT_AUTHENTICATED)
                {
                    EV << frame->getTransmitterAddress() <<"is NOT_AUTHENTICATED"<<endl;
                    return;
                }
            }
            if(!mesh)
            {
                EV << frame->getTransmitterAddress() <<"is unknown"<<endl;
                return;
            }

            sendDelayed(msg, delay,"mgmtOut");

        }
        else if(frame->getReceiverAddress().isBroadcast() || frame->getTransmitterAddress().isBroadcast())
        {
            msg->setName("EncBrodcast");
            // send(msg,"mgmtOut");
            delay= (double) enc + mic_add;
            Ieee80211MeshFrame *frame = (check_and_cast<Ieee80211MeshFrame *>(msg));
                       frame->setByteLength(frame->getByteLength()+16);
                       sendDelayed(frame, delay,"mgmtOut");
        }
        else
        {
            //   send(msg,"mgmtOut");
          //  handleIeee80211DataFrameWithSNAP(msg);
            handleIeee80211MeshFrame(msg);
        }


    }
    else if (dynamic_cast<Ieee80211DataFrameWithSNAP *>(msg))
    {
        if(!activeHandshake)
        {
            send(msg,"mgmtOut");
            return;
        }
        double delay=0;
        EV << msg << "..................................................." <<endl;


        Ieee80211DataFrameWithSNAP *frame = (check_and_cast<Ieee80211DataFrameWithSNAP *>(msg));

        if(strstr(msg->getName() ,"EncBrodcast"))
        {
            delay= (double) dec + mic_verify;
            //  send(msg,"mgmtOut");
            MeshInfo *mesh = lookupMesh(frame->getTransmitterAddress());
                  if(mesh)
                  {
                      if(mesh->status==NOT_AUTHENTICATED)
                      {
                          EV << frame->getTransmitterAddress() <<"is NOT_AUTHENTICATED"<<endl;
                          return;
                      }
                  }
                  if(!mesh)
                  {
                      EV << frame->getTransmitterAddress() <<"is unknown"<<endl;
                      return;
                  }


            sendDelayed(msg, delay,"mgmtOut");

        }
        else if(frame->getReceiverAddress().isBroadcast() || frame->getTransmitterAddress().isBroadcast())
        {
            msg->setName("EncBrodcast");
            // send(msg,"mgmtOut");
            delay= (double) enc + mic_add;
            Ieee80211DataFrameWithSNAP *frame = (check_and_cast<Ieee80211DataFrameWithSNAP *>(msg));
                       frame->setByteLength(frame->getByteLength()+16);
                       sendDelayed(frame, delay,"mgmtOut");
        }
        else
        {
            //   send(msg,"mgmtOut");
            handleIeee80211DataFrameWithSNAP(msg);
        }


    }

    else
    {
        //   EV <<"Security::handleResponse: msg: " <<msg<<endl;
        EV << msg << "..................................................." <<endl;
        send(msg,"mgmtOut");
    }

}

void Security::handleTimer(cMessage *msg)
{
    EV << "HandleTimer " << msg << endl;
    if(strstr(msg->getName() ,"beaconTimer"))
    {
        sendBeacon();
        scheduleAt(simTime()+beaconInterval, beaconTimer);
    }

    else if (dynamic_cast<newcMessage *>(msg) != nullptr)
    {
        newcMessage *newmsg = (check_and_cast<newcMessage *>(msg));
        if(newmsg->getKind())
        {
            if (newmsg->getKind()==MK_BEACON_TIMEOUT)
            {
                EV<<"missed a few consecutive beacons, delete entry" <<endl;
                MeshInfo *mesh = lookupMesh(newmsg->getMeshMACAddress_AuthTimeout());
                if(mesh)
                {
                    totalBeaconTimeout++;
                    emit(BeaconsTimeoutNrSignal, totalBeaconTimeout);
                    EV << "BeaconInterval: " <<mesh->beaconInterval<<endl;
                    EV << "delete mesh " << mesh->address  << " entry from our List" <<endl;
                    clearMeshNode(mesh->address);
                    updateGroupKey();
                }

            }
            else if (newmsg->getKind() == PMK_TIMEOUT)
            {
                EV << "PMK Timeout" << endl;
                MeshInfo *mesh = lookupMesh(newmsg->getMeshMACAddress_AuthTimeout());
                if(mesh)
                {
                    EV << "PMK time out, Mesh address = " << mesh->address << endl;
                    clearMeshNode( mesh->address);
                }
            }
            //Timeout for 4-Way-Handshake
            else if ( newmsg->getKind()==MK_AUTH_TIMEOUT)
            {

                EV << "Authentication time out for 4 WHS" <<endl;
                MeshInfo *mesh = lookupMesh(newmsg->getMeshMACAddress_AuthTimeout());
                if(mesh)
                {
                    totalAuthTimeout++;
                    emit(AuthTimeoutsNrSignal, totalAuthTimeout);
                    EV << "Authentication time out, Mesh address = " << mesh->address << endl;
                    clearMeshNode( mesh->address);
                    // keep listening to Beacons, maybe we'll have better luck next time
                }
            }
            //Timeout for Group key update/Handshake
            else  if ( newmsg->getKind()==GK_AUTH_TIMEOUT)
            {
                EV << "Group Key Timeout" <<endl;
                MeshInfo *mesh = lookupMesh(newmsg->getMeshMACAddress_AuthTimeout());
                if(mesh)
                {
                    EV << "Authentication time out, Mesh address = " << mesh->address << endl;
                }
                // keep listening to Beacons, maybe we'll have better luck next time
            }
            else if (newmsg->getKind() == GTK_TIMEOUT)
            {
                EV << "MGTK Timeout" << endl;
                updateGroupKey();

                if(newmsg) delete newmsg;
            }
        }
    }


    else
    {
        error("unknown msg");
    }
}

void Security::sendBeacon()
{
    EV << "Sending beacon"<<endl;
    Ieee80211BeaconFrame *frame = new Ieee80211BeaconFrame("Beacon");
    Ieee80211BeaconFrameBody& body = frame->getBody();
    body.setSSID(ssid.c_str());
    body.setBeaconInterval(beaconInterval);
    body.setChannelNumber(channelNumber);
    frame->setReceiverAddress(MACAddress::BROADCAST_ADDRESS);
    frame->setFromDS(true);
    frame->setByteLength((int)beacon);
    send(frame,"mgmtOut");
}

void Security::handleBeaconFrame(Ieee80211BeaconFrame *frame)
{
    //drop all beacons while Authentication
    EV << "Received Beacon frame"<<endl;

    if (!activeHandshake)
    {
        return;
        delete frame;
    }


    storeMeshInfo(frame->getTransmitterAddress(), frame->getBody());
    MeshInfo *mesh = lookupMesh(frame->getTransmitterAddress());
    if(mesh)
    {
        // just to avoid undefined states
        if(mesh->authTimeoutMsg_a!=nullptr && mesh->authTimeoutMsg_b!=nullptr)
        {
            EV << "Authentication in Progress, ignore Beacon"<<endl;
            counter ++;
            if(counter==2)
            {
                clearMeshNode(mesh->address);
                counter=0;
            }
            delete frame;
            return;
        }
        else
        {
            // no Authentication in progress, start negotiation
            if (mesh->status==NOT_AUTHENTICATED && mesh->authTimeoutMsg_a==nullptr &&mesh->authTimeoutMsg_b==nullptr)
            {
                checkForAuthenticationStart(mesh);
            }
            // if authenticated Mesh, restart beacon timeout
            else if (mesh->status==AUTHENTICATED && mesh->authTimeoutMsg_a==nullptr &&mesh->authTimeoutMsg_b==nullptr)
            {
                EV << "Beacon is from authenticated Mesh, restarting beacon timeout timer"<<endl;
                EV << "++++++++++++++++++++++++++++++++++++++++++++++++++" <<endl;
                ASSERT(mesh->beaconTimeoutMsg!=nullptr);
                cancelEvent(mesh->beaconTimeoutMsg);
                scheduleAt(simTime()+MAX_BEACONS_MISSED*mesh->beaconInterval, mesh->beaconTimeoutMsg);

            }

        }
    }
    delete frame;
}

void Security::storeMeshInfo(const MACAddress& address, const Ieee80211BeaconFrameBody& body)
{
    MeshInfo *mesh = lookupMesh(address);

       if (mesh)
       {
           EV << "Mesh address=" << address << ", SSID=" << body.getSSID() << " already in our Mesh list, refreshing the info\n";
       }
       else
       {
           EV << "Inserting Mesh address=" << address << ", SSID=" << body.getSSID() << " into our Mesh list\n";
           meshList.push_back(MeshInfo());
           mesh = &meshList.back();
           mesh->status = NOT_AUTHENTICATED;
           mesh->authTimeoutMsg_a=nullptr;
           mesh->authTimeoutMsg_b=nullptr;
           mesh->authSeqExpected = 1;
           mesh->groupAuthTimeoutMsg=nullptr;
           mesh->PMKTimerMsg=nullptr;
           mesh->PWE=0;
           mesh->csA=0;
           mesh->ceA=0;
           mesh->csB=0;
           mesh->ceB=0;
           mesh->k=0;
           mesh->WaitingForAck=0;

       }
       //mesh->channel = body.getChannelNumber();
       mesh->address = address;
       mesh->ssid = body.getSSID();
       mesh->beaconInterval = body.getBeaconInterval();
}




void Security::clearMeshNode(const MACAddress& address)
{
    EV << "clearMeshNode" <<endl;
    EV << "Node: " << address<<endl;

    MeshInfo *mesh = lookupMesh(address);

    if(mesh)
    {
        if(mesh->beaconTimeoutMsg!=nullptr)
        {
            delete cancelEvent(mesh->beaconTimeoutMsg);
            mesh->beaconTimeoutMsg=nullptr;
        }
        if(mesh->authTimeoutMsg_a!=nullptr)
        {
            delete cancelEvent(mesh->authTimeoutMsg_a);
            mesh->authTimeoutMsg_a=nullptr;
        }
        if(mesh->authTimeoutMsg_b!=nullptr)
        {
            delete cancelEvent(mesh->authTimeoutMsg_b);
            mesh->authTimeoutMsg_b=nullptr;
        }

        if(mesh->groupAuthTimeoutMsg!=nullptr)
        {
            delete cancelEvent(mesh->groupAuthTimeoutMsg);
            mesh->groupAuthTimeoutMsg=nullptr;
        }
        if(mesh->PMKTimerMsg!=nullptr)
        {
            delete cancelEvent(mesh->PMKTimerMsg);
            mesh->PMKTimerMsg=nullptr;
        }

        mesh->status=NOT_AUTHENTICATED;
        mesh->authTimeoutMsg_a=nullptr;
        mesh->authTimeoutMsg_b=nullptr;

        clearKey256(mesh->KCK);
        clearKey256(mesh->PMK);
        clearKey256(mesh->AEK);
        clearKey128(mesh->MTK);
        clearKey128(mesh->MGTK_other);
        /*for(MeshList::iterator it=meshList.begin(); it != meshList.end();)
        {
            if (it->address == address)
                it = meshList.erase(it);
            else
                ++it;
        }*/
    }
}

int Security::checkAuthState(const MACAddress& address)
{
    for (MeshList::iterator it=meshList.begin(); it!=meshList.end(); ++it)
    {
        if (it->address == address)
            if(it->status==AUTHENTICATED)
                return 1;
    }
    return 0;
}
int Security::checkMac(const MACAddress& address)
{
    if (
            address.str().compare("10:00:00:00:00:00") == 0 ||
            address.str().compare("10:00:00:00:00:01") == 0 ||
            address.str().compare("10:00:00:00:00:02") == 0 ||
            address.str().compare("10-00-00-00-00-00") == 0 ||
            address.str().compare("10-00-00-00-00-01") == 0 ||
            address.str().compare("10-00-00-00-00-02") == 0
    )
        return 1;
    else
    {
        error("unknown Mac Address");
        return 0;
    }
}



Security::MeshInfo *Security::lookupMesh(const MACAddress& address)
{
    for (MeshList::iterator it=meshList.begin(); it!=meshList.end(); ++it)
        if (it->address == address)
            return &(*it);
    return nullptr;
}


void Security::sendManagementFrame(Ieee80211ManagementFrame *frame, const MACAddress& address)
{
    frame->setToDS(true);
    frame->setReceiverAddress(address);
    send(frame,"mgmtOut");
}

void Security::sendDelayedManagementFrame(Ieee80211ManagementFrame *frame, const MACAddress& address,  simtime_t delay)
{
    frame->setToDS(true);
    frame->setReceiverAddress(address);
    sendDelayed(frame, delay,"mgmtOut");
}


void Security::sendMeshFrame(Ieee80211MeshFrame *frame, const MACAddress& address)
{
    frame->setToDS(true);
    frame->setReceiverAddress(address);
    send(frame,"mgmtOut");
}
void Security::sendDelayedMeshFrame(Ieee80211MeshFrame *frame, const MACAddress& address, simtime_t delay)
{
    frame->setToDS(true);
    frame->setReceiverAddress(address);
    sendDelayed(frame, delay,"mgmtOut");
}

void Security::sendDataOrMgmtFrame(Ieee80211DataOrMgmtFrame *frame, const MACAddress& address)
{
    frame->setToDS(true);
    frame->setReceiverAddress(address);
    send(frame,"mgmtOut");
}
void Security::sendDelayedDataOrMgmtFrame(Ieee80211DataOrMgmtFrame *frame, const MACAddress& address, simtime_t delay)
{
    frame->setToDS(true);
    frame->setReceiverAddress(address);
    sendDelayed(frame, delay,"mgmtOut");
}

/*------------------------------      Open Authentication   --------------------------------*/





/*------------------------------       Authentication   --------------------------------*/

void Security::checkForAuthenticationStart(MeshInfo *mesh)
{
    if (mesh)
        if (mesh->authTimeoutMsg_a==nullptr && mesh->authTimeoutMsg_b==nullptr)
        {
            EV <<"Start SAE"<<endl;
            Ieee80211Prim_AuthenticateRequest *ctrl = new  Ieee80211Prim_AuthenticateRequest();
            ctrl->setTimeout(authenticationTimeout_a);
            startSAE(mesh, ctrl->getTimeout());
        }
}

/*----------------------------------------       -SAE-           ---------------------------------------------*/
void Security::startSAE(MeshInfo *mesh, simtime_t timeout)
{
    double  delay=0;
    /*
     *  - In the following we use the notation of ECC-based SAE in which P(x; y) represents a point on a publicly known elliptic curve of the
              form y^2 = x^3+ax+b. By inv we refer to the additive inverse element of a point on the elliptic curve.
            - PWE: password element which represents a point on an elliptic curve
            - N =PWE x m.
            - The initiating peer A constructs a commit scalar csA = (randA + maskA) mod r and a commit element ceA = inv(maskA x N).
            - randA refers to a random number. maskA is a temporary secret value
            - A computes k = F((randA x (csB x N + ceB))
            - B computes k = F((randB x (csA x N + ceA))
            - F: pre-defined key derivation function
            - PMK =H(k || counter || (csA+csB) mod r || F(ceA +ceB))
     */

    //pwd-seed = H(MAX(STA-A-MAC, STA-B-MAC) || MIN(STA-A-MAC, STA-B-MAC), password || counter)
    mesh->PWE = computePWE(PSK,ssid);
    EV << "PWE" << mesh->PWE<<endl;
    // msg = csA || ceA
    SAEMsg *msg = new SAEMsg();
    mesh->csA = (int)getRNG(1)->intRand(1073741823);
    mesh->ceA = (int)getRNG(1)->intRand(1073741823);
    msg->setSAE_commitScalar(mesh->csA);
    msg->setSAE_commitElement(mesh->ceA);

    // create and send first authentication frame
    Ieee80211AuthenticationFrame *frame = new Ieee80211AuthenticationFrame("SAE msg 1/4");
    frame->getBody().setSequenceNumber(1);
    frame->encapsulate(msg);

    frame->setByteLength((int)sae_1);
    delay= (double) commit;

    EV << "Sending SAE msg 1/4: Commit A"<<endl;
    sendDelayedManagementFrame(frame, mesh->address,delay);
    mesh->authSeqExpected = 2;
    mesh->status= COMMITTED;

    // schedule timeout for side A
    ASSERT(mesh->authTimeoutMsg_a==nullptr);
    mesh->authTimeoutMsg_a = new newcMessage("authTimeout", MK_AUTH_TIMEOUT);
    mesh->authTimeoutMsg_a->setMeshMACAddress_AuthTimeout(mesh->address);
    scheduleAt(simTime()+authenticationTimeout_a, mesh->authTimeoutMsg_a);
}


void Security::handleSAE(Ieee80211AuthenticationFrame *frame)
{
    EV <<"handleSAE" <<endl;

        int frameAuthSeq = frame->getBody().getSequenceNumber();
        MeshInfo *mesh = lookupMesh(frame->getTransmitterAddress());
        if (!mesh)
        {
            EV<<"create Entry" <<endl;
            MACAddress meshAddress = frame->getTransmitterAddress();
            // Candidate For Authentication
            meshList.push_back(MeshInfo());// this implicitly creates a new entry
            mesh = &meshList.back();
            mesh->address = meshAddress;
            mesh->status = NOT_AUTHENTICATED;
            mesh->authSeqExpected = 1;
            mesh->beaconInterval=beaconInterval;
            mesh->authTimeoutMsg_a=nullptr;
            mesh->authTimeoutMsg_b=nullptr;
            mesh->groupAuthTimeoutMsg=nullptr;
            mesh->csA=0;
            mesh->csB=0;
            mesh->ceA=0;
            mesh->ceB=0;
            mesh->WaitingForAck=0;
        }

        EV << "negotiation with: " << mesh->address<<endl;
        SAEMsg* msg= (SAEMsg *)frame->decapsulate();

        double delay=0;
        int msgSize=0;

        if(!msg)
        {
            EV << "Frame decapsulation failed!"<<endl;
            return;
        }

        if(mesh->authTimeoutMsg_a!=nullptr && mesh->authTimeoutMsg_b!=nullptr)
            {
                EV << "illegal state!" <<endl;
                clearMeshNode(mesh->address);
                return;
            }


        if(strstr(frame->getName() ,"SAE-OK msg 4/4") && frameAuthSeq==4)
        {
            EV << "<<<<< 4 >>>>>"<<endl;
            if (mesh->authTimeoutMsg_a==nullptr)
            {
                EV << "No Authentication in progress, ignoring frame\n";
                EV << mesh->authTimeoutMsg_a <<endl;
                //error("HandleAuth. frameAuthSeq == 2");
                clearMeshNode(mesh->address);
                delete frame;
                return;
            }
            if(mesh->status!=CONFIRMED)
            {
             //   error("Illegal Handling HandleSAE msg2/4");
                return;
                clearMeshNode(mesh->address);
            }

            if(computeSmallHash(mesh->k, mesh->csA, mesh->csB, mesh->ceA, mesh->ceB) == msg->getSAE_confirmfield())
            {
                //Install PMK
                mesh->PMK.buf.push_back(mesh->k * mesh->PWE);
                mesh->PMK.buf.push_back(mesh->k * mesh->PWE+1);
                mesh->PMK.buf.push_back(mesh->k * mesh->PWE+2);
                mesh->PMK.buf.push_back(mesh->k * mesh->PWE+3);
                mesh->PMK.len=256;
                //Install KCK
                mesh->KCK.buf.push_back(mesh->k * mesh->PWE*1);
                mesh->KCK.buf.push_back(mesh->k * mesh->PWE+1*2);
                mesh->KCK.buf.push_back(mesh->k * mesh->PWE+2*3);
                mesh->KCK.buf.push_back(mesh->k * mesh->PWE+3*3);
                mesh->KCK.len=256;

                EV << "Schedule next Key refresh"<<endl;
                mesh->PMKTimerMsg = new newcMessage("PMKTimer");
                mesh->PMKTimerMsg->setKind(PMK_TIMEOUT);
                mesh->PMKTimerMsg->setMeshMACAddress_AuthTimeout(mesh->address);
                scheduleAt(simTime()+PMKTimeout, mesh->PMKTimerMsg);
            }


            mesh->status= ACCEPTED;
            EV << "PMK: "<< mesh->PMK.buf.at(0)<<endl;
            // authentication completed
            mesh->authSeqExpected =1 ;

            EV << "Initiator: Authentication with Mesh-Peer completed"<<endl;

            ASSERT(mesh->authTimeoutMsg_a!=nullptr);
            delete cancelEvent(mesh->authTimeoutMsg_a);
            mesh->authTimeoutMsg_a = nullptr;


            AuthTime=  simTime()-AuthTime;
            EV <<".............................. SAE Time: " << AuthTime <<endl;


            if (mesh->authTimeoutMsg_b!=nullptr)
            {
                delete cancelEvent(mesh->authTimeoutMsg_b);
                mesh->authTimeoutMsg_b = nullptr;
            }

            delete frame;
            delete msg;
            return;
        }

        if (frameAuthSeq == 1)
        {
            if (mesh->authTimeoutMsg_b!=nullptr)
                       {
                           EV << "Authentication in progress, ignoring frame\n";
                           EV << mesh->authTimeoutMsg_b <<endl;
                           //error("HandleAuth. frameAuthSeq == 2");
                           clearMeshNode(mesh->address);
                           delete frame;
                           return;
                       }
            //extract commit A
            mesh->csA = msg->getSAE_commitScalar();
            mesh->ceA = msg->getSAE_commitElement();

            mesh->authSeqExpected = 1;

            EV << "<<<<< 1 >>>>>"<<endl;

            mesh->PWE = computePWE(PSK,ssid);

            mesh->csB = (int)getRNG(1)->intRand(1073741823);
            mesh->ceB = (int)getRNG(1)->intRand(1073741823);

            msg->setSAE_commitScalar(mesh->csB);
            msg->setSAE_commitElement(mesh->ceB);
            EV << "Sending SAE msg 2/4: Commit B"<<endl;



            //B computes k = F((randB x (csA x N + ceA))
            mesh->k= mesh->csB * mesh->csA * mesh->ceA;

            mesh->status= COMMITTED;

            ASSERT(mesh->authTimeoutMsg_b==nullptr);
            mesh->authTimeoutMsg_b = new newcMessage("authTimeout", MK_AUTH_TIMEOUT);
            mesh->authTimeoutMsg_b->setMeshMACAddress_AuthTimeout(mesh->address);
            scheduleAt(simTime()+authenticationTimeout_b, mesh->authTimeoutMsg_b);
            msgSize= (int) sae_2;
            delay=(double) commit + process_commit;
        }

        if (frameAuthSeq == 2)
        {
            if (mesh->authTimeoutMsg_a==nullptr)
            {
                EV << "No Authentication in progress, ignoring frame\n";
                EV << mesh->authTimeoutMsg_a <<endl;
                //error("HandleAuth. frameAuthSeq == 2");
                clearMeshNode(mesh->address);
                delete frame;
                return;
            }

           /* if(mesh->status!=COMMITTED)
            {
                error("Illegal Handling HandleSAE msg2/4");
                return;
                clearMeshNode(mesh->address);
            }*/

            EV << "<<<<< 2 >>>>>"<<endl;

            // extract Commit B
            mesh->csB = msg->getSAE_commitScalar();
            mesh->ceB =  msg->getSAE_commitElement();

            //A computes k = F((randA x (csB x N + ceB))
            mesh->k= mesh->csA * mesh->csB * mesh->ceB;
            EV << "Sending SAE msg 3/4: Confirm A"<<endl;
            //H(k || counter || (csA+csB) mod r || F(ceA +ceB))
            EV << mesh->k << mesh->csA << mesh->csB << mesh->ceA << mesh->ceB <<endl;
            msg->setSAE_confirmfield(computeSmallHash(mesh->k, mesh->csA, mesh->csB, mesh->ceA, mesh->ceB));
            mesh->status= CONFIRMED;
            msgSize= (int) sae_3;
            delay= (double)  process_commit + confirm;

        }

        if (frameAuthSeq == 3)
        {
            if (mesh->authTimeoutMsg_b==nullptr)
            {
                EV << "No Authentication in progress, ignoring frame\n";
                EV << mesh->authTimeoutMsg_b <<endl;
                //error("HandleAuth. frameAuthSeq == 2");
                clearMeshNode(mesh->address);
                delete frame;
                return;
            }
           /* if(mesh->status!=COMMITTED)
            {
                error("Illegal Handling HandleSAE msg3/4");
                clearMeshNode(mesh->address);
                return;
            }*/

            EV << "<<<<< 3 >>>>>"<<endl;
            msgSize= (int) sae_4;
            delay = (double) process_confirm + confirm + process_confirm ;

            //verify Hash
            //B computes k = F((randA x (csB x N + ceB))
            mesh->k= mesh->csA * mesh->csB * mesh->ceB;
            //EV << mesh->k << mesh->csA << mesh->csB << mesh->ceA << mesh->ceB <<endl;
            if(computeSmallHash(mesh->k, mesh->csA, mesh->csB, mesh->ceA, mesh->ceB) == msg->getSAE_confirmfield())
            {
                EV << "Hash is ok"<<endl;
                //compute Hash and send it
                msg->setSAE_confirmfield(computeSmallHash(mesh->k, mesh->csA, mesh->csB, mesh->ceA, mesh->ceB));
                EV << "Sending SAE msg 4/4: Confirm B"<<endl;
                mesh->status= CONFIRMED;
            }

            else
            {
                //error("Hash verifying failed");
                EV << "Hash verifying failed"<<endl;
                clearMeshNode(mesh->address);
            }


        }
        // station is authenticated if it made it through the required number of steps
        bool isLast = (frameAuthSeq+1 == numAuthSteps);
        // check authentication sequence number is OK
        if (frameAuthSeq > mesh->authSeqExpected)
        {
            EV << "frameAuthSeq: " << frameAuthSeq<<endl;
            EV << "Wrong sequence number, " << mesh->authSeqExpected << " expected\n";
            Ieee80211AuthenticationFrame *resp = new Ieee80211AuthenticationFrame("SAE-ERROR");
            resp->getBody().setStatusCode(SC_AUTH_OUT_OF_SEQ);
           // resp->setByteLength(msgSize);
            sendManagementFrame(resp, frame->getTransmitterAddress());
            delete frame;
            delete msg;
            mesh->authSeqExpected = 1; // go back to start square
            //  error("Auth-ERROR");
            clearMeshNode(mesh->address);
            return;
        }



        // send OK response (we don't model the cryptography part, just assume successful authentication every time)
        EV << "Sending SAE frame, seqNum=" << (frameAuthSeq+1) << "\n";
        std::stringstream buffer;
        buffer << "SAE msg " <<frameAuthSeq +1<< "/4" << std::endl;

        const char* p  = buffer.str().c_str();
        Ieee80211AuthenticationFrame *resp = new Ieee80211AuthenticationFrame(isLast ? "SAE-OK msg 4/4" : p);
        resp->getBody().setSequenceNumber(frameAuthSeq+1);
        resp->setByteLength(msgSize);
        // if(frameAuthSeq +1== 3)// pass status to other party
        // if(mesh->status==NOT_AUTHENTICATED)
        //    resp->getBody().setStatusCode(SC_TBTT_REQUEST);

        resp->getBody().setIsLast(isLast);
        resp->encapsulate(msg);

        EV << "Delay: " <<delay <<endl;
        sendDelayedManagementFrame(resp, frame->getTransmitterAddress(),delay);

        // update status
        if (isLast)
        {
            EV << "Authentication successful\n";
            mesh->authSeqExpected = 1;

            //wait for Ack
            cModule *host = getContainingNode(this);
            host->subscribe(NF_TX_ACKED,this);
            mesh->status=CONFIRMED;
            mesh->WaitingForAck=1;
            delete frame;
        }

        else
        {
            mesh->authSeqExpected += 2;
            EV << "Expecting Authentication frame " << mesh->authSeqExpected << endl;
            delete frame;
        }

}

/*----------------------------------------       -AMPE-           ---------------------------------------------*/


void Security::startAMPE(const MACAddress& address, int side)
{
    double delay=0;
    EV << "Initialize AMPE Handshake"<<endl;
    MeshInfo *mesh = lookupMesh(address);
    if(mesh)
    {
        if(mesh->status==ACCEPTED || mesh->status==AUTHENTICATED)
        {
            std::stringstream buffer;
            int frame_size=0;

            if(side == 0)//Side A
            {
                //derive Keys from PMK
                deriveMeshKeys(mesh->address);

                buffer << "AMPE msg " << 1<< "/4" << std::endl;
                frame_size= (int)ampe_1;
                delay = (double) enc + mic_add + 2*rand_gen ;


                ASSERT(mesh->authTimeoutMsg_a==nullptr);
                mesh->authTimeoutMsg_a = new newcMessage("authTimeout", MK_AUTH_TIMEOUT);
                mesh->authTimeoutMsg_a->setMeshMACAddress_AuthTimeout(mesh->address);
                scheduleAt(simTime()+authenticationTimeout_a, mesh->authTimeoutMsg_a);

            }
            else if(side == 1)//Side B
            {
                frame_size= (int)ampe_3;
                buffer << "AMPE msg " << 3<< "/4" << std::endl;
                delay = (double) enc + mic_add + 2*rand_gen;
            }
            // create and send first action frame
            const char* p  = buffer.str().c_str();
            Ieee80211ActionFrame *frame = new  Ieee80211ActionFrame(p);

            /*----------------- Security msg ------------*/
            AMPEMsg *msg = new AMPEMsg();
            //share own 'old' GTK
            if(MGTK_self.buf.empty())
            {
                EV <<"Pick Random MGTK(128)"<<endl;
                MGTK.buf.push_back(getRNG(1)->intRand(1073741823));
                MGTK.buf.push_back(getRNG(1)->intRand(1073741823));
                MGTK.len=128;
                MGTK_self=MGTK;
            }

            //Encrypt GTK with AEK
            EV << "Encrypted GTK: " << endl;
            msg->setKey_Data128( encryptAMPEFrames(mesh->AEK, MGTK_self));
            /*-------------------------------------------------------*/
            EV << "Sending AMPE msg 1/4: Open( (GTK)AEK || MIC )"<<endl;

            frame->encapsulate(msg);
            frame->setByteLength(frame_size);
            sendDelayedManagementFrame(frame, mesh->address,delay);
        }
    }
    else
        EV <<"unkown mesh node!" <<endl;
}


void Security::handleAMPE(Ieee80211ActionFrame *frame)
{

    double delay=0;
    MeshInfo *mesh = lookupMesh(frame->getTransmitterAddress());

    if(mesh)
    {
        if(strstr(frame->getName() ,"AMPE msg 1/4") )
        {
            AMPEMsg *msg = (AMPEMsg *)frame->decapsulate();
            delete frame;

            //derive Keys from PMK
            deriveMeshKeys(mesh->address);

            EV << "AMPE msg 1/4 arrived"<<endl;
            Ieee80211ActionFrame *frame = new  Ieee80211ActionFrame("AMPE msg 2/4");

            //Install key
            EV << "Install keys" << endl;
            clearKey128(mesh->MGTK_other);

            EV << "Decrypted GTK:" << endl;
            mesh->MGTK_other = decryptAMPEFrames(mesh->AEK, msg->getKey_Data128());

            EV << "Sending AMPE msg 2/4: Confirm "<<endl;

            delay= (double) dec + mic_verify + rand_gen + enc + mic_add + dec + mic_verify;
            frame->setByteLength((int)ampe_2);
            sendDelayedManagementFrame(frame,  mesh->address,delay);

            mesh->status=AUTHENTICATED;
            mesh->WaitingForAck=1;
            //Wait for ack than start AMPE side B, msg3
            cModule *host = getContainingNode(this);
            host->subscribe(NF_TX_ACKED,this);
            delete msg;

            ASSERT(mesh->authTimeoutMsg_b==nullptr);
            mesh->authTimeoutMsg_b = new newcMessage("authTimeout", MK_AUTH_TIMEOUT);
            mesh->authTimeoutMsg_b->setMeshMACAddress_AuthTimeout(mesh->address);
            scheduleAt(simTime()+authenticationTimeout_b, mesh->authTimeoutMsg_b);

        }

        else  if(strstr(frame->getName() ,"AMPE msg 2/4") )
        {
            EV << "AMPE msg 2/4 arrived";
            //Do nothing

            // schedule Group key timeout
            GTKTimer = new newcMessage("GTKTimer");
            GTKTimer->setKind(GTK_TIMEOUT);
            scheduleAt(simTime()+GTKTimeout, GTKTimer);

            delete frame;

        }
        else  if(strstr(frame->getName() ,"AMPE msg 3/4") )
        {
            AMPEMsg *msg = (AMPEMsg *)frame->decapsulate();
            if(!msg)
                error("handleAMPE: frame->decapsulate()");

            delete frame;
            EV << "AMPE msg 3/4 arrived"<<endl;
            Ieee80211ActionFrame *frame = new  Ieee80211ActionFrame("AMPE msg 4/4");


            //Install key
            EV << "Install keys" << endl;
            clearKey128(mesh->MGTK_other);

            EV << "Decrypted GTK:" << endl;
            mesh->MGTK_other = decryptAMPEFrames(mesh->AEK, msg->getKey_Data128());

            mesh->status=AUTHENTICATED;

            EV << "Sending AMPE msg 4/4: Confirm "<<endl;

            delay= (double) dec + mic_verify + rand_gen + enc + mic_add + dec + mic_verify;
            frame->setByteLength((int)ampe_4);
            sendDelayedManagementFrame(frame,  mesh->address,delay);

            //AMPE over
            ASSERT(mesh->authTimeoutMsg_a!=nullptr);
            delete cancelEvent(mesh->authTimeoutMsg_a);
            mesh->authTimeoutMsg_a = nullptr;

            // schedule beacon timeout
            mesh->beaconTimeoutMsg = new newcMessage("beaconTimeout");
            mesh->beaconTimeoutMsg->setMeshMACAddress_AuthTimeout(mesh->address);
            mesh->beaconTimeoutMsg->setKind(MK_BEACON_TIMEOUT);
            scheduleAt(simTime()+MAX_BEACONS_MISSED*mesh->beaconInterval, mesh->beaconTimeoutMsg);
        }
        else if(strstr(frame->getName() ,"AMPE msg 4/4") )
        {
            EV << "AMPE msg 4/4 arrived";
            // schedule Group key timeout
            GTKTimer = new newcMessage("GTKTimer");
            GTKTimer->setKind(GTK_TIMEOUT);
            scheduleAt(simTime()+GTKTimeout, GTKTimer);

            //AMPE over
            ASSERT(mesh->authTimeoutMsg_b!=nullptr);
            delete cancelEvent(mesh->authTimeoutMsg_b);
            mesh->authTimeoutMsg_b = nullptr;

            // schedule beacon timeout
            mesh->beaconTimeoutMsg = new newcMessage("beaconTimeout");
            mesh->beaconTimeoutMsg->setMeshMACAddress_AuthTimeout(mesh->address);
            mesh->beaconTimeoutMsg->setKind(MK_BEACON_TIMEOUT);
            scheduleAt(simTime()+MAX_BEACONS_MISSED*mesh->beaconInterval, mesh->beaconTimeoutMsg);
            delete frame;
            //Do nothing
        }
    }
}

/*----------------------------------------       Group Handshake           ---------------------------------------------*/

void Security::updateGroupKey()
{
    EV << "Group key update"<<endl;
    for (MeshList::iterator it=meshList.begin(); it!=meshList.end(); ++it)
    {
        if (it->status == AUTHENTICATED)
        {
            EV<< "MGTK Update for:" << it->address << endl;
            MeshInfo *mesh = lookupMesh(it->address);
            if(mesh)
            {
                if(mesh->PMK.buf.size()>=2)
                {
                    Ieee80211Prim_AuthenticateRequest *ctrl = new  Ieee80211Prim_AuthenticateRequest();
                    ctrl->setTimeout(groupAuthenticationTimeout);
                    if(mesh->groupAuthTimeoutMsg==nullptr)
                        sendGroupHandshakeMsg(mesh,ctrl->getTimeout());
                }
            }
            EV << "---------------------------------------" <<endl;
        }
    }
}
void Security::sendGroupHandshakeMsg(MeshInfo *mesh,  simtime_t timeout)
{

    double delay=0;
       if(mesh)
       {
           if(mesh->status==AUTHENTICATED)
           {
               // create and send first action frame
               Ieee80211ActionFrame *frame = new  Ieee80211ActionFrame("Group msg 1/2");

               /*----------------- Security msg ------------*/
               AMPEMsg *msg = new AMPEMsg();
               //share own 'old' GTK
               clearKey128(MGTK_self);

               if(MGTK_self.buf.empty())
               {
                   EV <<"Pick Random MGTK(128)"<<endl;
                   MGTK.buf.push_back(getRNG(1)->intRand(1073741823));
                   MGTK.buf.push_back(getRNG(1)->intRand(1073741823));
                   MGTK.len=128;
                   MGTK_self=MGTK;
               }

               //Encrypt GTK with AEK
               EV << "Encrypted GTK: " << endl;
               msg->setKey_Data128( encryptAMPEFrames(mesh->AEK, MGTK_self));
               /*-------------------------------------------------------*/
               EV << "Sending AMPE msg 3/4: Open( (GTK)AEK || MIC )"<<endl;
               frame->encapsulate(msg);

               //   sendManagementFrame(frame, mesh->address);
               delay = (double) enc + mic_add + rand_gen;

               frame->setByteLength((int)group_1);
               sendDelayedManagementFrame(frame, mesh->address,delay);
           }

       }
       else
           EV <<"unkown mesh node!" <<endl;

}


void Security::handleGroupHandshakeFrame(Ieee80211AuthenticationFrame *frame)
{
    double delay=0;
       MeshInfo *mesh = lookupMesh(frame->getTransmitterAddress());

       if(mesh)
       {
               AMPEMsg *msg = (AMPEMsg *)frame->decapsulate();
               if(strstr(frame->getName() ,"Group msg 1/2") )
               {
                   //derive Keys from PMK
                   deriveMeshKeys(mesh->address);

                   EV << "AMPE msg 1/4 arrived"<<endl;
                   Ieee80211ActionFrame *frame = new  Ieee80211ActionFrame("Group msg 2/2");

                   //Install key

                   EV << "Install keys" << endl;
                   clearKey128(mesh->MGTK_other);

                   EV << "Decrypted GTK:" << endl;
                   mesh->MGTK_other = decryptAMPEFrames(mesh->AEK, msg->getKey_Data128());
                   delay= (double)dec + mic_verify;
                   EV << "Sending AMPE msg 2/4: Confirm "<<endl;

                   frame->setByteLength((int)group_2);
                   sendDelayedManagementFrame(frame,  mesh->address,delay);

                   delete msg;
               }

               else  if(strstr(frame->getName() ,"Group msg 2/2") )
                          {
                                  EV << "AMPE msg 2/4 arrived";
                                  delete frame;
                                  //Do nothing
                          }
       }
}

/*-------------------------------------------            ---------------------------------------------*/

IPv4Datagram*  Security::handleIPv4Datagram(IPv4Datagram* IP, MeshInfo *mesh)
{
    /*
     short version_var;
     short headerLength_var;
     IPv4Address srcAddress_var;
     IPv4Address destAddress_var;
     int transportProtocol_var;
     short timeToLive_var;
     int identification_var;
     bool moreFragments_var;
     bool dontFragment_var;
     int fragmentOffset_var;
     unsigned char typeOfService_var;
     int optionCode_var;
     IPv4RecordRouteOption recordRoute_var;
     IPv4TimestampOption timestampOption_var;
     IPv4SourceRoutingOption sourceRoutingOption_var;
     unsigned int totalPayloadLength_var;
     */

    if(mesh){
        /*   EV << "Before: " <<endl;
        EV <<IP->getHeaderLength() <<endl;
        EV << "" << IP->getSrcAddress() <<endl;
        EV << "" << IP->getDestAddress() <<endl;
        EV << "" << IP->getTransportProtocol() <<endl;
        EV << "" << IP->getTimeToLive() <<endl;
        EV << "" << IP->getIdentification()<<endl;
        //    EV << "" << IP->getMoreFragments() <<endl;
        //EV << "" << IP->getDontFragment() <<endl;
        //EV << "" << IP->getFragmentOffset() <<endl;
        EV << "" << IP->getTypeOfService() <<endl;
        EV << "" << IP->getOptionCode() <<endl;
        EV << "" << IP->getTotalPayloadLength() <<endl;*/
        return IP;
        if(!(mesh->MTK.buf.size()<2))
        {

            IP->setHeaderLength(IP->getHeaderLength()^mesh->MTK.buf.at(0));
            IP->setSrcAddress(IPv4Address((IP->getSrcAddress().getInt())^mesh->MTK.buf.at(0)));
            IP->setDestAddress(IPv4Address((IP->getDestAddress().getInt())^mesh->MTK.buf.at(0)));
            IP->setTransportProtocol(IP->getTransportProtocol()^mesh->MTK.buf.at(0));
            IP->setTimeToLive(IP->getTimeToLive()^mesh->MTK.buf.at(0));
            IP->setIdentification(IP->getIdentification()^mesh->MTK.buf.at(0));
            //IP->setMoreFragments(IP->getMoreFragments()^mesh->KEK.buf.at(0));
            //IP->setDontFragment(IP->getDontFragment()^mesh->KEK.buf.at(0));
            //IP->setFragmentOffset(IP->getFragmentOffset()^mesh->KEK.buf.at(0));
            IP->setTypeOfService(IP->getTypeOfService()^mesh->MTK.buf.at(0));
            //IP->setOptionCode(IP->getOptionCode()^mesh->MTK.buf.at(0));
            IP->setTotalPayloadLength(IP->getTotalPayloadLength()^mesh->MTK.buf.at(0));

            /*     IPv4RecordRouteOption recordRoute_var;
               IPv4TimestampOption timestampOption_var;
               IPv4SourceRoutingOption sourceRoutingOption_var;*/

        }
        else
            error("MTK is too short");

        /*     EV << "Later: " <<endl;
        EV <<IP->getHeaderLength() <<endl;
        EV << "" << IP->getSrcAddress() <<endl;
        EV << "" << IP->getDestAddress() <<endl;
        EV << "" << IP->getTransportProtocol() <<endl;
        EV << "" << IP->getTimeToLive() <<endl;
        EV << "" << IP->getIdentification()<<endl;
        //    EV << "" << IP->getMoreFragments() <<endl;
        //EV << "" << IP->getDontFragment() <<endl;
        //EV << "" << IP->getFragmentOffset() <<endl;
        EV << "" << IP->getTypeOfService() <<endl;
        EV << "" << IP->getOptionCode() <<endl;
        EV << "" << IP->getTotalPayloadLength() <<endl;*/
    }
    return IP;
}

Ieee80211ActionMeshFrame *  Security::encryptActionHWMPFrame(Ieee80211ActionMeshFrame* frame, const MACAddress& address)
{
    EV << "Entering encryptActionHWMPFrame"<<endl;
  //  MeshInfo *mesh = lookupMesh(address);
  //  if(mesh)

    /* PREQ:
     *      bodyLength = 26;
     *      id = IE11S_PREQ;
     *      unsigned int pathDiscoveryId;
     *      MACAddress originator;
     *      unsigned int originatorSeqNumber;
     *      MACAddress originatorExternalAddr;
     *      unsigned int lifeTime;
     *      unsigned int metric = 0;
     *      unsigned char targetCount;
     */

    /* PREP:
     *      bodyLength = 37;
     *      id = IE11S_PREP;
     *      MACAddress target;
     *      unsigned int targetSeqNumber;
     *      MACAddress tagetExternalAddr;
     *      unsigned int lifeTime;
     *      unsigned int metric = 0;
     *      MACAddress originator;
     *      unsigned int originatorSeqNumber;
     */

    if(strstr(frame->getClassName() ,"Ieee80211ActionPREQFrame")!=nullptr)
    {

        Ieee80211ActionPREQFrame *preq = (check_and_cast<Ieee80211ActionPREQFrame *>(frame));
        if(preq)
        {
            Ieee80211ActionPREQFrameBody body = preq->getBody();

           /* EV << "vorher: "<<endl;
            EV <<  body.getBodyLength()<<endl;
            EV <<  body.getId()<<endl;
            EV <<  body.getPathDiscoveryId()<<endl;
            EV <<  body.getOriginator()<<endl;
            EV << body.getOriginatorSeqNumber()<<endl;
            EV <<  body.getOriginatorExternalAddr()<<endl;
            EV << body.getLifeTime()<<endl;
            EV << body.getMetric()<<endl;*/

            body.setBodyLength(body.getBodyLength()^4);
              body.setId(body.getId()^4);
              body.setPathDiscoveryId(body.getPathDiscoveryId()^4);
             body.setOriginator(MACAddress(body.getOriginator().getInt()^4));
             body.setOriginatorSeqNumber(body.getOriginatorSeqNumber()^4);
              body.setOriginatorExternalAddr(MACAddress(body.getOriginatorExternalAddr().getInt()^4));
             body.setLifeTime(body.getLifeTime()^4);
             body.setMetric(body.getMetric()^4);

          /*  EV << "nachher: "<<endl;
            EV <<  body.getBodyLength()<<endl;
            EV <<  body.getId()<<endl;
            EV <<  body.getPathDiscoveryId()<<endl;
            EV <<  body.getOriginator()<<endl;
            EV << body.getOriginatorSeqNumber()<<endl;
            EV <<  body.getOriginatorExternalAddr()<<endl;
            EV << body.getLifeTime()<<endl;
            EV << body.getMetric()<<endl;*/
            preq->setBody(body);

            return preq;
        }
    }


    else if(strstr(frame->getClassName() ,"Ieee80211ActionPREPFrame")!=nullptr){
        Ieee80211ActionPREPFrame *prep = (check_and_cast<Ieee80211ActionPREPFrame *>(frame));
        if(prep)
        {
            Ieee80211ActionPREPFrameBody body = prep->getBody();
          /*  EV << "vorher: "<<endl;
            EV <<  body.getBodyLength()<<endl;
            EV <<  body.getId()<<endl;
            EV << body.getTarget()<<endl;
            EV << body.getTargetSeqNumber()<<endl;
            EV << body.getTagetExternalAddr()<<endl;
            EV << body.getLifeTime()<<endl;
            EV <<  body.getOriginator()<<endl;
            EV << body.getOriginatorSeqNumber()<<endl;*/

            body.setBodyLength(body.getBodyLength()^4);
            body.setId(body.getId()^4);
            body.setTarget(MACAddress(body.getTarget().getInt()^4));
            body.setTargetSeqNumber(body.getTargetSeqNumber()^4);
            body.setTagetExternalAddr(MACAddress(body.getTagetExternalAddr().getInt()^4));
            body.setLifeTime(body.getLifeTime()^4);
            body.setMetric(body.getMetric()^4);
            body.setOriginator(MACAddress(body.getOriginator().getInt()^4));
            body.setOriginatorSeqNumber(body.getOriginatorSeqNumber()^4);

           /* EV << "nachher: "<<endl;
            EV <<  body.getBodyLength()<<endl;
            EV <<  body.getId()<<endl;
            EV << body.getTarget()<<endl;
            EV << body.getTargetSeqNumber()<<endl;
            EV << body.getTagetExternalAddr()<<endl;
            EV << body.getLifeTime()<<endl;
            EV <<  body.getOriginator()<<endl;
            EV << body.getOriginatorSeqNumber()<<endl;
            prep->setBody(body);*/
            return prep;
        }
    }


    return 0;
}

void Security::handleIeee80211MeshFrame(cMessage *msg)
{
    double delay=0;
    //Decryption
    if(strstr(msg->getName() ,"CCMPFrame")!=nullptr)
    {
        Ieee80211MeshFrame *frame = (check_and_cast<Ieee80211MeshFrame *>(msg));
        MeshInfo *mesh = lookupMesh(frame->getTransmitterAddress());
        EV<< frame->getTransmitterAddress()<<endl;

        {
            //is for us?, decrypt und send it to upper layer (DecCCMP)
            if (frame->getFinalAddress().compareTo(myAddress)==0)
            {
                if(mesh)
                {
                    if(mesh->status==AUTHENTICATED)
                    {
                        EV << "CCMP from Mac is for us >>> decrypt msg ...." <<endl;

                        CCMPFrame *ccmpFrame = dynamic_cast<CCMPFrame*> ( frame->decapsulate());
                        IPv4Datagram *IP = dynamic_cast<IPv4Datagram *>(ccmpFrame->decapsulate());

                        EV << frame->getFinalAddress() << " " << myAddress <<endl;
                        frame->encapsulate(handleIPv4Datagram(IP, mesh));
                        frame->setName("DecCCMP");
                        delay = (double)  dec + mic_verify;
                        sendDelayed(frame, delay,"mgmtOut");
                    }
                    else
                        EV <<"Mesh is not AUTHENTICATED!" <<endl;
                }
                else
                    EV <<"Mesh is unknown!" <<endl;
            }

            //not for us, decrypt and encrypt
            else
            {
                Ieee80211MeshFrame *frame2 = (check_and_cast<Ieee80211MeshFrame *>(msg));
                MeshInfo *mesh = lookupMesh(frame2->getReceiverAddress());
                if (mesh)
                {
                    if(mesh->status==AUTHENTICATED)
                    {
                        EV << "CCMP from Mac not for us >>> decrypt then encrypt msg ...." <<endl;

                        //decrypt with transmitter key
                        CCMPFrame *ccmpFrame = dynamic_cast<CCMPFrame*> ( frame->decapsulate());
                        IPv4Datagram *IP = dynamic_cast<IPv4Datagram *>(ccmpFrame->decapsulate());
                        frame->encapsulate(handleIPv4Datagram(IP, mesh));


                        //encrypt with receiver key
                        // New Frame = Old MAC Header | CCMP Header | ENC (Encapsulated Old MSG)
                        EV << "encrypt msg ...."<<msg->getClassName() <<" " << msg->getName() <<endl;

                        Ieee80211MeshFrame *frame2 = (check_and_cast<Ieee80211MeshFrame *>(msg));

                        //create new CCMP frame
                        CCMPFrame *ccmpFrame2 = new CCMPFrame();

                        IPv4Datagram *IP2 = dynamic_cast<IPv4Datagram *>(frame2->decapsulate());
                        if(IP2)
                        {
                            ccmpFrame2->encapsulate(handleIPv4Datagram(IP2, mesh));
                            ccmpFrame2->setCCMP_Header(1);
                            ccmpFrame2->setCCMP_Mic(1);


                            if (frame2==nullptr)
                            {
                                frame2 = new Ieee80211MeshFrame(frame->getName());
                                frame2->setTimestamp(frame->getCreationTime());
                                frame2->setTTL(32);
                            }

                            if (frame->getControlInfo())
                                delete frame->removeControlInfo();
                            frame2->setReceiverAddress(mesh->address);
                            frame2->setTransmitterAddress(frame->getAddress3());

                            if (frame2->getReceiverAddress().isUnspecified())
                            {
                                char name[50];
                                strcpy(name,frame->getName());
                                throw cRuntimeError ("Ieee80211Mesh::encapsulate Bad Address");
                            }
                            if (frame2->getReceiverAddress().isBroadcast())
                                frame2->setTTL(1);
                            frame2->setName("CCMPFrame");
                            frame2->encapsulate(ccmpFrame2);

                            delay = (double) enc + mic_add  + dec + mic_verify;
                            sendDelayedMeshFrame(frame2, mesh->address, delay);
                        }
                        else
                            error("NOT IPv4");
                    }
                    else
                        EV <<"Mesh is not AUTHENTICATED!" <<endl;
                }
                else
                    EV <<"Mesh is unknown!" <<endl;

                //--//
                // delay = (double) enc + mic_add;
                // sendDelayed(msg, delay,"mgmtOut");
                // delete ccmpFrame;
            }





        }
    }

    //Encryption
  else
    {
        // New Frame = Old MAC Header | CCMP Header | ENC (Encapsulated Old MSG)
        EV << "encrypt msg ...."<<msg->getClassName() <<" " << msg->getName() <<endl;

        Ieee80211MeshFrame *frame = (check_and_cast<Ieee80211MeshFrame *>(msg));
        MeshInfo *mesh = lookupMesh(frame->getReceiverAddress());
        //create new CCMP frame
        CCMPFrame *ccmpFrame = new CCMPFrame();

        IPv4Datagram *IP = dynamic_cast<IPv4Datagram *>(frame->decapsulate());
        if(IP)
        {
            if(mesh)
            {
                if(mesh->status==AUTHENTICATED)
                    ccmpFrame->encapsulate(handleIPv4Datagram(IP, mesh));
            }


            ccmpFrame->setCCMP_Header(1);
            ccmpFrame->setCCMP_Mic(1);


            if (frame==nullptr)
            {
                frame = new Ieee80211MeshFrame(msg->getName());
                frame->setTimestamp(msg->getCreationTime());
                frame->setTTL(32);
            }

            if (msg->getControlInfo())
                delete msg->removeControlInfo();
            frame->setReceiverAddress(mesh->address);
            frame->setTransmitterAddress(frame->getAddress3());

            if (frame->getReceiverAddress().isUnspecified())
            {
                char name[50];
                strcpy(name,msg->getName());
                throw cRuntimeError ("Ieee80211Mesh::encapsulate Bad Address");
            }
            if (frame->getReceiverAddress().isBroadcast())
                frame->setTTL(1);
            frame->setName("CCMPFrame");
            frame->encapsulate(ccmpFrame);

            delay = (double) enc + mic_add;
            frame->setByteLength(frame->getByteLength()+16);
            sendDelayedMeshFrame(frame, mesh->address, delay);
        }
        else
            error("NOT IPv4");
    }
}


void Security::handleIeee80211ActionMeshFrame(cMessage *msg)
{
    //Decryption
    if(strstr(msg->getName() ,"CCMPFrame")!=nullptr)
    {
        Ieee80211ActionMeshFrame *hwmpFrame = dynamic_cast<Ieee80211ActionMeshFrame *>(msg);
        MeshInfo *mesh = lookupMesh(hwmpFrame->getTransmitterAddress());
        EV <<hwmpFrame->getTransmitterAddress()<<endl;
        EV << "Encrypted Ieee80211ActionMeshFrame from Mac >>> decrypt frame ...."<< endl;

        if(mesh)
        {
            if(mesh->status==AUTHENTICATED)
            {
                Ieee80211ActionMeshFrame *hwmpFrame = dynamic_cast<Ieee80211ActionMeshFrame *>(msg);
                //Ieee80211ActionMeshFrame *hwmpFrame2 = encryptActionHWMPFrame(hwmpFrame,hwmpFrame->getTransmitterAddress() );
                // hwmpFrame2->setName("DecCCMP");
                // send(hwmpFrame2,"mgmtOut");
                hwmpFrame->setByteLength(hwmpFrame->getByteLength()-16);
                msg->setName("DecCCMP");
                double delay= (double) dec + mic_verify;
                sendDelayed(msg, delay,"mgmtOut");
            }
            else
                EV << "Mesh Node isn't authenticated. Drop pkt"<< endl;
        }


        //   else
        //     EV << "Mesh node not found or not authenticated!"<< endl;
    }
    //Encryption
    else
    {
        Ieee80211ActionMeshFrame *hwmpFrame = dynamic_cast<Ieee80211ActionMeshFrame *>(msg);
        // MeshInfo *mesh = lookupMesh(hwmpFrame->getTransmitterAddress());//self address
        EV <<hwmpFrame->getTransmitterAddress()<<endl;
        EV << "Ieee80211ActionMeshFrame from Mgmt >>> encrypt frame ...."<< endl;


        EV << "Send out " <<endl;

            hwmpFrame->setByteLength(hwmpFrame->getByteLength()+16);

       // Ieee80211ActionMeshFrame *hwmpFrame2 = encryptActionHWMPFrame(hwmpFrame, hwmpFrame->getReceiverAddress() );
        // hwmpFrame2->setName("CCMPFrame");
        //  send(hwmpFrame2,"mgmtOut")
        msg->setName("CCMPFrame");
        double delay= (double) enc + mic_add;
        sendDelayed(msg,delay,"mgmtOut");
    }

}




void Security::handleIeee80211DataFrameWithSNAP(cMessage *msg)
{
    EV << "Entering handleIeee80211DataFrameWithSNAP " << endl;
    double delay=0;
    //Decryption
    if(strstr(msg->getName() ,"CCMPFrame")!=nullptr)
    {
        EV << "msg from Mac >>> decrypt msg ...." <<endl;
        //  cPacket* payloadMsg=nullptr;
        Ieee80211DataFrameWithSNAP *frame = (check_and_cast<Ieee80211DataFrameWithSNAP *>(msg));
        MeshInfo *mesh = lookupMesh(frame->getTransmitterAddress());
        //  EV << "transmitter address before lookup"<< frame->getTransmitterAddress()<<endl;
        frame->setName("DecCCMP");
        delay= (double) dec + mic_verify;
        if(mesh)
        {
            if(mesh->status==AUTHENTICATED)
                sendDelayedDataOrMgmtFrame(frame,frame->getReceiverAddress(),delay);
        }
        /*     if(mesh)
        {
            checkAuthState(mesh->address);
            CCMPFrame *ccmpFrame = dynamic_cast<CCMPFrame*> ( frame->decapsulate());
            IPv4Datagram *IP = dynamic_cast<IPv4Datagram *>(ccmpFrame->decapsulate());// payloadMsg= ccmpFrame->decapsulate();
            if(IP)
            {
                frame->setName("DecCCMP");
                // frame->encapsulate(payloadMsg);
                if(mesh->status==AUTHENTICATED){
                    frame->encapsulate(handleIPv4Datagram(IP, mesh));
                    //   else
                    //   error("frame->encapsulate(handleIPv4Datagram(IP, mesh))");

                    sendDataOrMgmtFrame(frame,frame->getReceiverAddress()); //? send(frame,"mgmtOut");
                }
                else{
                    delete frame;
                    delete msg;
                    return;
                }
            }
            else
                error("Not IPv4");

            delete ccmpFrame;

        }
        // else
        // error("mesh not found");*/

    }

    //Encryption
    else
    {
        // New Frame = Old MAC Header | CCMP Header | ENC (Encapsulated Old MSG)
        EV << "encrypt msg ...."<<msg->getClassName() <<" " << msg->getName() <<endl;
        //  cPacket* payloadMsg=nullptr;
        Ieee80211DataFrameWithSNAP *frame = (check_and_cast<Ieee80211DataFrameWithSNAP *>(msg));
        // EV << "receiver address before lookup"<<frame->getReceiverAddress()<<endl;

        MeshInfo *mesh = lookupMesh(frame->getReceiverAddress());
        frame->setName("CCMPFrame");
        delay = (double) enc + mic_add;
        /*   IPv4Datagram *IP = check_and_cast<IPv4Datagram *>(frame->getEncapsulatedPacket);
       if(IP)
            EV << "IP "<<endl;*/
        if(mesh)
        {

            if(mesh->status==AUTHENTICATED)
            {
                frame->setByteLength(frame->getByteLength()+16);
                sendDelayedDataOrMgmtFrame(frame, mesh->address,delay);
            }
            else
            {
                NrdeletedFrames ++;
            emit(deletedFramesSignal, NrdeletedFrames);

            }

        }


        /*    CCMPFrame *ccmpFrame = new CCMPFrame();

        // payloadMsg = frame->decapsulate();
        IPv4Datagram *IP = dynamic_cast<IPv4Datagram *>(frame->decapsulate());
       if(IP)
        {
            //        payloadMsg->encapsulate(IP);
            //   if (payloadMsg)
            if(mesh)
            {
                if(mesh->status==AUTHENTICATED)
                    ccmpFrame->encapsulate(handleIPv4Datagram(IP, mesh));
            }
            // else
            //   error("Enc: frame2 ERROR");

            ccmpFrame->setCCMP_Header(1);
            ccmpFrame->setCCMP_Mic(1);

            if(checkAuthState(frame->getReceiverAddress())==0)
            {
                delete frame;
                delete ccmpFrame;
                return;
            }
            else
            {
                if (frame==nullptr)
                {
                    frame = new Ieee80211DataFrameWithSNAP(msg->getName());
                    frame->setTimestamp(msg->getCreationTime());

                }

                if (msg->getControlInfo())
                    delete msg->removeControlInfo();
                frame->setReceiverAddress(mesh->address);
                frame->setTransmitterAddress(frame->getAddress3());

                if (frame->getReceiverAddress().isUnspecified())
                {
                    char name[50];
                    strcpy(name,msg->getName());
                    throw cRuntimeError ("Ieee80211Mesh::encapsulate Bad Address");
                }
                if (frame->getReceiverAddress().isBroadcast())

                frame->setName("CCMPFrame");
                frame->encapsulate(ccmpFrame);
            }
            sendDataOrMgmtFrame(frame, mesh->address);
        }
        else
            error("NOT IPv4");*/
    }
}






void Security::sendDeauthentication(const MACAddress& address)
{
    EV << "Send Deauthentication to Mesh " << address<< endl;

    Ieee80211AuthenticationFrame *resp = new Ieee80211AuthenticationFrame("Deauth");
    sendManagementFrame(resp, address);

}
void Security::handleDeauthenticationFrame(Ieee80211AuthenticationFrame *frame)
{
    EV << "Processing Deauthentication frame\n";

    MeshInfo *mesh = lookupMesh(frame->getTransmitterAddress());
    delete frame;
    if(mesh){
        sendDeauthentication(mesh->address);
        if(mesh->beaconTimeoutMsg!=nullptr)
        {
            delete cancelEvent(mesh->beaconTimeoutMsg);
            //  mesh->beaconTimeoutMsg=nullptr;
        }
        if(mesh->authTimeoutMsg_a!=nullptr)
        {
            delete cancelEvent(mesh->authTimeoutMsg_a);
            mesh->authTimeoutMsg_a=nullptr;
        }
        if(mesh->authTimeoutMsg_b!=nullptr)
        {
            delete cancelEvent(mesh->authTimeoutMsg_b);
            mesh->authTimeoutMsg_b=nullptr;
        }
        if(mesh->groupAuthTimeoutMsg!=nullptr)
        {
            delete cancelEvent(mesh->groupAuthTimeoutMsg);
            mesh->groupAuthTimeoutMsg=nullptr;
        }
        if(mesh->PMKTimerMsg!=nullptr)
        {
            delete cancelEvent(mesh->PMKTimerMsg);
            mesh->PMKTimerMsg=nullptr;
        }
        /*   for(MeshList::iterator it=meshList.begin(); it != meshList.end();)
            {
                if (it->address == address)
                    it = meshList.erase(it);
                else
                    ++it;
            }
         */
    }
}




void Security::handleAck()
{

    EV<<"handleAck" <<endl;
    EV<<"\n" <<endl;

    for (MeshList::iterator it=meshList.begin(); it!=meshList.end(); ++it){
        // SAE
        if (it->status == CONFIRMED && it->WaitingForAck == 1)
        {
            MeshInfo *mesh = lookupMesh(it->address);
            if(mesh)
            {
                if (mesh->authTimeoutMsg_b==nullptr)
                {
                    EV << "No Authentication in progress, ignoring frame\n";
                    EV << mesh->authTimeoutMsg_b <<endl;
                    //error("HandleAuth. frameAuthSeq == 2");
                    clearMeshNode(mesh->address);
                    return;
                }

                mesh->status = ACCEPTED;
                mesh->WaitingForAck=0;
                //Install PMK
                mesh->PMK.buf.push_back(mesh->k * mesh->PWE);
                mesh->PMK.buf.push_back(mesh->k * mesh->PWE+1);
                mesh->PMK.buf.push_back(mesh->k * mesh->PWE+2);
                mesh->PMK.buf.push_back(mesh->k * mesh->PWE+3);
                mesh->PMK.len=256;
                //Install KCK
                mesh->KCK.buf.push_back(mesh->k * mesh->PWE*1);
                mesh->KCK.buf.push_back(mesh->k * mesh->PWE+1*2);
                mesh->KCK.buf.push_back(mesh->k * mesh->PWE+2*3);
                mesh->KCK.buf.push_back(mesh->k * mesh->PWE+3*3);
                mesh->KCK.len=256;




                EV << "PMK: "<< mesh->PMK.buf.at(0)<<endl;
                EV << "Schedule next Key refresh"<<endl;
                mesh->PMKTimerMsg = new newcMessage("PMKTimer");
                mesh->PMKTimerMsg->setKind(PMK_TIMEOUT);
                mesh->PMKTimerMsg->setMeshMACAddress_AuthTimeout(mesh->address);
                scheduleAt(simTime()+PMKTimeout, mesh->PMKTimerMsg);

                //start Authentication with other party
                ASSERT(mesh->authTimeoutMsg_b!=nullptr);
                delete cancelEvent(mesh->authTimeoutMsg_b);
                mesh->authTimeoutMsg_b = nullptr;
                startAMPE(mesh->address, 0);
            }
        }
        // AMPE
        else if (it->status == AUTHENTICATED && it->WaitingForAck == 1 )
        {
            MeshInfo *mesh = lookupMesh(it->address);
            if(mesh)
            {
                startAMPE(mesh->address,1);
                mesh->WaitingForAck=0;
            }
        }
    }
}


uint64_t Security::stringToUint64_t(std::string s) {
    using namespace std;
    char * a=new char[s.size()+1];
    a[s.size()]=0;
    memcpy(a,s.c_str(),s.size());
    //std::copy(s.c_str(),s.c_str()+s.size(),a);
    uint64_t zahl = 0;
    for(int len = strlen(a), i = len - 1; i != -1; --i){
        zahl = zahl * 10 + s[i] - '0';}
    return zahl;
}
uint32_t Security::stringToUint32_t(std::string s) {
    using namespace std;
    char * a=new char[s.size()+1];
    a[s.size()]=0;
    memcpy(a,s.c_str(),s.size());
    //std::copy(s.c_str(),s.c_str()+s.size(),a);
    uint32_t zahl = 0;
    for(int len = strlen(a), i = len - 1; i != -1; --i){
        zahl = zahl * 10 + s[i] - '0';}
    return zahl;
}


Security::key256 Security::computePMK(std::string s, std::string s1) {

    //PMK(256) = (PSK XOR SSID)
    uint64_t a = stringToUint64_t(PSK);
    uint64_t b = stringToUint64_t(ssid);
    uint64_t c = a ^ b;

    key256 vec;
    vec.len=256;
    vec.buf.push_back(c);
    //PMK ist zu kurz. Rest einfach mit gleichen Inhalt f�llen
    vec.buf.push_back(c+1);
    vec.buf.push_back(c+2);
    vec.buf.push_back(c+3);
    return vec;
}

int Security::computePWE(std::string s, std::string s1) {

    //PMK(256) = (PSK XOR SSID)
    uint64_t a = stringToUint64_t(PSK);
    uint64_t b = stringToUint64_t(ssid);
    uint64_t c = a ^ b;

     int c_ = (int )c;
    return c_;
}


Security::key384 Security::computePTK(Security::key256 PMK, Security::nonce NonceA, Security::nonce NonceB, const MACAddress & addressA, const MACAddress & addressB) {
    key384 PTK;

    // PTK(384) = PMK Xor (Z1 xor Z2 xor MACA xor MACB)
    PTK.len=384;
    uint64_t a  = stringToUint64_t(addressA.str());
    uint64_t b = stringToUint64_t(addressB.str());
    for (int i = 0; i < 4; i++)
        PTK.buf.push_back(PMK.buf.at(i) ^ NonceA.buf.at(i) ^ NonceB.buf.at(i) ^ a ^ b);

    for (int i = 4; i < 6; i++)
        PTK.buf.push_back(PMK.buf.at(i-4) ^ NonceA.buf.at(i-3) ^ NonceB.buf.at(i-2) ^ a ^ b);

    return PTK;
}



Security::nonce Security::generateNonce() {

    nonce Nonce;
    Nonce.len=256;
    Nonce.buf.push_back( getRNG(1)->intRand(1073741823));
    Nonce.buf.push_back( getRNG(1)->intRand(1073741823));
    Nonce.buf.push_back( getRNG(1)->intRand(1073741823));
    Nonce.buf.push_back( getRNG(1)->intRand(1073741823));

    Nonce.len=256;
    return Nonce;
}



Security::key128 Security::encrypt128(Security::key128 a, Security::key128 b){
    key128 c;
    if(a.buf.size()<2)
        error("encrypt128:a is empty: '%d'", a.buf.size());
    if( b.buf.size()<2)
        error("encrypt128:b is empty: '%d'", b.buf.size());
    for(int i=0;i<2;i++){
        c.buf.push_back(a.buf.at(i)^b.buf.at(i));
        EV<<a.buf.at(i); EV<< " XOR "; EV<< b.buf.at(i);EV<< " = "; EV<< c.buf.at(i)<<endl;
    }
    c.len=128;
    return c;

}
Security::key128 Security::decrypt128(Security::key128 a, Security::key128 b){
    key128 c;
    for(int i=0; i<2;i++){
        c.buf.push_back(a.buf.at(i)^b.buf.at(i));
        EV<< c.buf.at(i)<<endl;
    }
    return c;

}
Security::key128 Security::computeMic128(key128 KCK, cMessage *msg){
    uint64_t mic=0;
    key128 mic_;
    SecurityPkt *pkt = dynamic_cast<SecurityPkt*> (msg);

    for(int i=0;i<12;i++){
        if(pkt->getDescriptor_Type(i)-'0' == 1)
        {
            switch(i)
            {
                case 0: mic^=pkt->getDescriptor_Type(0);  break;
                case 1: mic^=pkt->getKey_Info();  break;
                case 2: mic^=pkt->getKey_Length();  break;
                case 3: mic^=pkt->getKey_RC();  break;
                case 4: for(int i=0; i<(pkt->getKey_Nonce().len/64);i++){mic^=pkt->getKey_Nonce().buf.at(i);}      break;//32 octets
                case 5: for(int i=0; i<(pkt->getEAPOL_KeyIV().len/64);i++) mic^=pkt->getEAPOL_KeyIV().buf.at(i);  break;//16 octets
                case 6: mic^=pkt->getKey_RSC();  break;
                case 7: mic^=pkt->getReserved();  break;
                case 8:   break;//MIC!
                case 9: mic^=pkt->getKey_Data_Length();  break;
                case 10: for(int i=0; i<(pkt->getKey_Data128().len/64);i++) mic^=pkt->getKey_Data128().buf.at(i);  break;//16 octets
                case 11: for(int i=0; i<(pkt->getKey_Data256().len/64);i++) mic^=pkt->getKey_Data256().buf.at(i);  break;//32 octets
                default: mic^=0; break;
            }
        }
    }

    mic_.buf.push_back(mic^KCK.buf.at(0));
    mic_.buf.push_back(mic^KCK.buf.at(1));
    mic_.len=128;
    return mic_;
}

Security::key128 Security::encryptAMPEFrames(Security::key256 key, Security::key128 element){
    key128 c;
    if(key.buf.size()<2)
        error("encrypt128:a is empty: '%d'", key.buf.size());
    if( element.buf.size()<2)
        error("encrypt128:b is empty: '%d'", element.buf.size());
    for(int i=0;i<2;i++){
        c.buf.push_back(key.buf.at(i)^element.buf.at(i));
        EV<<key.buf.at(i); EV<< " XOR "; EV<< element.buf.at(i);EV<< " = "; EV<< c.buf.at(i)<<endl;
    }
    c.len=128;
    return c;

}
Security::key128 Security::decryptAMPEFrames(Security::key256 key, Security::key128 element){
    key128 c;
    for(int i=0; i<2;i++){
        c.buf.push_back(key.buf.at(i)^element.buf.at(i));
        EV<< c.buf.at(i)<<endl;
    }
    return c;

}












void Security::clearKey128(key128 key){
    if(!key.buf.empty())
    {
        key.buf.clear();
    }
}
void Security::clearKey256(key256 key){
    if(!key.buf.empty())
    {
        key.buf.clear();
    }
}
void Security::clearKey384(key384 key){
    if(!key.buf.empty())
    {
        key.buf.clear();
    }
}
void Security::clearNonce(nonce key){
    if(!key.buf.empty())
    {
        key.buf.clear();
    }
}


void Security::derivePTKKeys(MeshInfo *mesh, key384 key)
{
    if(!mesh->KCK.buf.empty() && !mesh->KEK.buf.empty() && !mesh->TK.buf.empty() ){
        mesh->KCK.buf.clear();
        mesh->KEK.buf.clear();
        mesh->TK.buf.clear();
    }
    // PTK = KEC | KEK | TK
    for(int i=0;i<2;i++){
        mesh->KCK.buf.push_back( mesh->PTK.buf.at(i) );
        mesh->KEK.buf.push_back( mesh->PTK.buf.at(i+2) );
        mesh->TK.buf.push_back( mesh->PTK.buf.at(i+4) );
    }
    mesh->KCK.len=128;
    mesh->KEK.len=128;
    mesh->TK.len=128;
}

void Security::deriveMeshKeys(const MACAddress& address)
{
    MeshInfo *mesh = lookupMesh(address);
    if(mesh)
    {
        uint64_t meshAdress  = stringToUint64_t(mesh->address.str());
        uint64_t myMeshAdres = stringToUint64_t(myAddress.str());
        // clear Keys
        if(!mesh->AEK.buf.empty() && !mesh->MTK.buf.empty())
        {
            mesh->AEK.buf.clear();
            mesh->MTK.buf.clear();
        }

        //AEK <- KDF-256(PMK, �AEK Derivation�, Selected AKM Suite || min(localMAC, peerMAC) || max(localMAC, peerMAC))
        for(int i=0;i<3;i++){
            mesh->AEK.buf.push_back( mesh->PMK.buf.at(i)*meshAdress*myMeshAdres );
        }
        mesh->AEK.len=256;

        //MTK <- KDF-X(PMK, �Temporal Key Derivation�, min(localNonce, peerNonce) ||    max(localNonce, peerNonce) || min(localLinkID, peerLinkID) ||
        // max(localLinkID, peerLinkID) || Selected AKM Suite ||    min(localMAC, peerMAC) || max(localMAC, peerMAC))
        for(int i=0;i<2;i++){
            mesh->MTK.buf.push_back( mesh->PMK.buf.at(i)*meshAdress*myMeshAdres);
        }
        mesh->MTK.len=128;
    }

    else
        EV << "unknown Mesh node! ";
}
int Security::computeSmallHash( int arg1, int arg2, int arg3, int arg4, int arg5 )
{
      return arg1*arg2*arg3*arg4*arg5;
}

/*char* Security::encapsulate(cPacket *msg, unsigned int* length)
{


    unsigned int payloadlen;
    static unsigned int iplen = 20; // we don't generate IP options
    static unsigned int udplen = 8;
    cPacket* payloadMsg = nullptr;
    char* buf = nullptr, *payload = nullptr;
    uint32_t saddr, daddr;
    volatile iphdr* ip_buf;
    struct newiphdr {
               short version_var;
               short headerLength_var;
               IPv4Address srcAddress_var;
               IPv4Address destAddress_var;
               int transportProtocol_var;
               short timeToLive_var;
               int identification_var;
               bool moreFragments_var;
               bool dontFragment_var;
               int fragmentOffset_var;
               unsigned char typeOfService_var;
               int optionCode_var;
               IPv4RecordRouteOption recordRoute_var;
               IPv4TimestampOption timestampOption_var;
               IPv4SourceRoutingOption sourceRoutingOption_var;
               unsigned int totalPayloadLength_var;
       }*iphdr;





    IPv4Datagram* IP = check_and_cast<IPv4Datagram*>(msg);

    // FIXME: Cast ICMP-Messages
    UDPPacket* UDP = dynamic_cast<UDPPacket*>(IP->decapsulate());
    if (!UDP) {
        EV << "    Can't parse non-UDP packets (e.g. ICMP) (yet)" << endl;
    }
    payloadMsg = UDP->decapsulate();


 *length = payloadlen + iplen + udplen;
    buf = new char[*length];

    // We use the buffer to build an ip packet.
    // To minimise unnecessary copying, we start with the payload
    // and write it to the end of the buffer
    memcpy( (buf + iplen + udplen), payload, payloadlen);

    // write ip header to begin of buffer
    ip_buf = (iphdr*) buf;
    ip_buf->version = 4; // IPv4
    ip_buf->ihl = iplen / 4;
    ip_buf->saddr = IP->getSrcAddress();
    ip_buf->daddr = IP->getDestAddress();
    ip_buf->protocol = IPPROTO_UDP;
    ip_buf->ttl = IP->getTimeToLive();
    ip_buf->tot_len = htons(*length);
    ip_buf->id = htons(IP->getIdentification());
    ip_buf->frag_off = htons(IP_DF); // DF, no fragments
    ip_buf->check = 0;
//    ip_buf->check = cksum((uint16_t*) ip_buf, iplen);

    delete IP;
    delete UDP;
    delete payloadMsg;
    delete payload;

    return buf;


}*/

void Security::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj)
{
    EV << "receiveChangeNotification()" <<endl;
    Enter_Method_Silent();

    if (signalID == NF_TX_ACKED)
    {
        handleAck();
    }
}

void Security::finish()
 {
     if (!statsAlreadyRecorded) {
        recordScalar("Total AuthTimeout Me", totalAuthTimeout);
        recordScalar("Total BeaconTimeout Me", totalBeaconTimeout);
        statsAlreadyRecorded = true;
     }
 }

}

}

