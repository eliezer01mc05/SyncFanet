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
//

#ifndef IEEE80211_SECURITY_STA_H
#define IEEE80211_SECURITY_STA_H

#include <vector>

#include "inet/common/INETDefs.h"
#include "inet/linklayer/ieee80211/mgmt/Ieee80211Primitives_m.h"
#include "inet/networklayer/common/InterfaceTable.h"
#include <map>
#include <set>
#include "inet/networklayer/contract/IRoutingTable.h"
#include "inet/networklayer/ipv4/IIPv4RoutingTable.h"

#include <iostream>
#include <vector>
#include <cstdarg>

#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/networklayer/ipv4/IPv4Datagram.h"
#include "inet/transportlayer/udp/UDPPacket_m.h"
#include "inet/securityModule/message/NewMsgWithMacAddr_m.h"
#include "inet/securityModule/SecurityKeys.h"

namespace inet {

namespace ieee80211 {

class SecurityPkt;
class SAEMsg;
class AMPEMsg;
class Security : public cSimpleModule, public cListener, public SecurityKeys
{
        const char *msg;

public:
         long totalAuthTimeout;
         long totalBeaconTimeout;
         long NrdeletedFrames;
        static simsignal_t AuthTimeoutsNrSignal;
        static simsignal_t BeaconsTimeoutNrSignal;
        static simsignal_t deletedFramesSignal;
        typedef uint64_t  unit64_t_;
        enum MeshStatus{ NOT_AUTHENTICATED, AUTHENTICATED, COMMITTED, CONFIRMED, ACCEPTED };
        struct MeshInfo
        {
                int channel;
                MACAddress address;
                MeshStatus status;
                int authSeqExpected;
                std::string ssid;
                Ieee80211SupportedRatesElement supportedRates;
                simtime_t beaconInterval;
                newcMessage *authTimeoutMsg_a;
                newcMessage *authTimeoutMsg_b;
                newcMessage *PMKTimerMsg;
                newcMessage *groupAuthTimeoutMsg;
                newcMessage *beaconTimeoutMsg;
                key256 PMK;
                nonce NonceA;
                nonce NonceB;
                key384 PTK;
                key384 PTK_final;
                key384 PTK_a;
                key384 PTK_b;
                key256 KCK; // Key Confirmation key (KCK)
                key128 KEK; // Key Encryption Key (KEK)
                key128 TK;  // Key Temporal key (TK)
                key128 GTK;
                key128 GTKMIC;
                key128 TempMIC;
                int PWE;
                int csA; // Commit Scalar
                int ceA; // Commit Element
                int csB; // Commit Scalar
                int ceB; // Commit Element
                int k;
                key256 AEK;
                key128 MTK;
                key128 MGTK_other;
                int WaitingForAck;

                MeshInfo()
                {
                    status = NOT_AUTHENTICATED;
                    channel = -1;
                    beaconInterval = 0.1;
                    authSeqExpected = -1;
                    authTimeoutMsg_a = nullptr;
                    authTimeoutMsg_b = nullptr;
                    groupAuthTimeoutMsg = nullptr;
                    status=NOT_AUTHENTICATED;
                    beaconTimeoutMsg = nullptr;
                    PMKTimerMsg = nullptr;
                    PWE=0;
                    csA=0;
                    ceA=0;
                    csB=0;
                    ceB=0;
                    k=0;
                    WaitingForAck=0;
               }

        };

        class NotificationInfoMesh : public cObject
        {
                MACAddress MeshAddress;
            public:
                void setMeshAddress(const MACAddress & a)
                {
                    MeshAddress = a;
                }

                const MACAddress & getMeshAddress() const
                {
                    return MeshAddress;
                }

        };
        struct MeshMAC_compare
        {
                bool operator ()(const MACAddress & u1, const MACAddress & u2) const
                {
                    return u1.compareTo(u2) < 0;
                }

        };

        typedef std::list<MeshInfo> MeshList;
        typedef std::list<MeshInfo> SimpleMeshList;
        SimpleMeshList simpleMeshList;

protected:

        static bool statsAlreadyRecorded;


        int counter;
        std::string ssid;
        int channelNumber;
        simtime_t beaconInterval;
        simtime_t PMKTimeout;
        simtime_t GTKTimeout;
        key128 GTK;
        key128 MGTK;
        key128 MGTK_self;

        std::string PSK;
        int numAuthSteps;
        MeshList meshList;
        cMessage *beaconTimer;
        newcMessage *PTKTimer;
        cMessage *GTKTimer;
        key256 PMK;
        bool isConnected;
        int numChannels;
        simtime_t authenticationTimeout_a; // A is the Initiator (sendOpenAuth, sendAuth, handleAuth<<2>>, handleAuth<<4>>)
        simtime_t authenticationTimeout_b;
        simtime_t groupAuthenticationTimeout;
        std::string default_ssid;
        InterfaceEntry *myIface;
        simtime_t AuthTime;
        bool activeHandshake;

        virtual void finish();

protected:
        struct LocEntry
        {
            MACAddress macAddr;
        };
        typedef std::map<MACAddress,LocEntry> securityMapMac;
        typedef securityMapMac::iterator MapMacIterator;
        MACAddress myAddress;
        int interfaceId;
        IInterfaceTable *itable;
        IIPv4RoutingTable *rt;

public:
        friend std::ostream & operator <<(std::ostream & os, const Security::LocEntry & e);
        Security();
        virtual ~Security();
        virtual void initialize(int stage);
        virtual int numInitStages() const
        {
            return NUM_INIT_STAGES;
        }

        virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj);
        virtual void handleMessage(cMessage*);
        virtual void setMacAddress(const MACAddress & add)
        {
            myAddress = add;
        }

        virtual MACAddress getMacAddress()
        {
            return myAddress;
        }

        virtual void sendBeacon();
        virtual void handleResponse(cMessage *msg);
        virtual void handleTimer(cMessage *msg);
        virtual void storeMeshInfo(const MACAddress & address, const Ieee80211BeaconFrameBody & body);

        virtual MeshInfo *lookupMesh(const MACAddress & address);
        virtual void clearMeshNode(const MACAddress & address);
        virtual int checkAuthState(const MACAddress& address);
        virtual int checkMac(const MACAddress& address);
        virtual void updateGroupKey();


        virtual void sendManagementFrame(Ieee80211ManagementFrame *frame, const MACAddress & address);
        virtual void sendDelayedManagementFrame(Ieee80211ManagementFrame *frame, const MACAddress & address, simtime_t delay);

        virtual void sendMeshFrame(Ieee80211MeshFrame *frame, const MACAddress & address);
        virtual void sendDelayedMeshFrame(Ieee80211MeshFrame *frame, const MACAddress & address, simtime_t delay);
        virtual void sendDataOrMgmtFrame(Ieee80211DataOrMgmtFrame *frame, const MACAddress & address);
        virtual void sendDelayedDataOrMgmtFrame(Ieee80211DataOrMgmtFrame *frame, const MACAddress& address, simtime_t delay);


        virtual void startSAE(MeshInfo *mesh, simtime_t timeout);
        virtual void handleSAE(Ieee80211AuthenticationFrame *frame);
        virtual void handleBeaconFrame(Ieee80211BeaconFrame *frame);

        virtual void checkForAuthenticationStart(MeshInfo *mesh);
        virtual void sendDeauthentication(const MACAddress & address);

        virtual void startAMPE(const MACAddress & address, int side);
        virtual void handleAck();

        virtual void handleDeauthenticationFrame(Ieee80211AuthenticationFrame *frame);
        virtual void sendGroupHandshakeMsg(MeshInfo *mesh,simtime_t timeout);
        virtual void handleGroupHandshakeFrame(Ieee80211AuthenticationFrame *frame);

        static uint64_t stringToUint64_t(std::string s);
        virtual key256 computePMK(std::string s, std::string s1);
        virtual int computePWE(std::string s, std::string s1);
        virtual nonce generateNonce();
        virtual key384 computePTK(Security::key256 PMK, Security::nonce NonceA, Security::nonce NonceB, const MACAddress & addressA, const MACAddress & addressB);
        virtual key128 encrypt128(Security::key128 a, Security::key128 b);
        virtual key128 decrypt128(Security::key128 a, Security::key128 b);

        virtual key128 encryptAMPEFrames(Security::key256 Enckey, Security::key128 element);
        virtual key128 decryptAMPEFrames(Security::key256 EncKey, Security::key128 element);

        virtual key128 computeMic128(key128 KCK, cMessage *msg);
        virtual void clearKey128(key128 key);
        virtual void clearKey256(key256 key);
        virtual void clearKey384(key384 key);
        virtual void clearNonce(nonce key);
        virtual void derivePTKKeys(MeshInfo *mesh,key384 key);
        virtual void deriveMeshKeys(const MACAddress & address);
      //  virtual char* encapsulate(cPacket *msg, unsigned int* length);
        virtual uint32_t stringToUint32_t(std::string s);
        virtual IPv4Datagram * handleIPv4Datagram(IPv4Datagram* IP,MeshInfo *mesh);
        virtual Ieee80211ActionMeshFrame * encryptActionHWMPFrame(Ieee80211ActionMeshFrame* frame, const MACAddress & address);

        virtual void handleIeee80211MeshFrame(cMessage *msg);
        virtual void handleIeee80211ActionMeshFrame(cMessage *msg);
        virtual void handleIeee80211DataFrameWithSNAP(cMessage *msg);
        virtual void handleAMPE(Ieee80211ActionFrame *frame);

        virtual int computeSmallHash(int arg1, int arg2, int arg3, int arg4, int arg5);

};

}

}

#endif


