//
// Copyright (C) 2006 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//


#include "inet/linklayer/ieee80211/mgmt/wpa2/SecurityIeee80211MgmtAdhoc.h"
#include "inet/linklayer/common/Ieee802Ctrl.h"
#include "stdlib.h"
//#include "Security.h"

namespace inet {

namespace ieee80211 {

Define_Module(SecurityIeee80211MgmtAdhoc);

void SecurityIeee80211MgmtAdhoc::initialize(int stage)
{
    SecurityIeee80211MgmtBase::initialize(stage);
}

void SecurityIeee80211MgmtAdhoc::handleTimer(cMessage *msg)
{
   ASSERT(false);
}

void SecurityIeee80211MgmtAdhoc::handleUpperMessage(cPacket *msg)
{


    //KATZE check classes and look for further
//    ASSERT(dynamic_cast<Ieee80211DataOrMgmtFrame *>(msg)!=nullptr);
    EV << msg->getClassName() << endl;
    cPacket *msg2 = (cPacket *)msg->getEncapsulatedPacket();
    EV << msg2->getClassName() << endl;
    cPacket *msg3 = (cPacket *)msg2->getEncapsulatedPacket();
    EV << msg3->getClassName() << endl;
    Ieee80211DataFrame *frame = nullptr;
//    Ieee80211ManagementFrame *mframe = nullptr;
//     bool isOLSRFrame = check_and_cast<OLSR_pkt *>(msg3)!=nullptr;
//        if (!isOLSRFrame)
        if(strcmp(msg3->getClassName(), "OLSR_pkt") == 0)
        {            // management frames are inserted into mgmtQueue
//            mgmtQueue.insert(msg);
            EV << "OLSRKATZE" << endl;
            frame = encapsulate(msg);
            if(frame != nullptr)
                   sendDown(frame);
        }
        else{
            EV << "OLSRMIMI" << endl;
            frame = encapsulate(msg);
            if(frame != nullptr)
                sendDown(frame);
        }

//    Ieee80211ManagementFrame *frame = encapsulateMgmt(msg);
    EV << "KRAULBARS" << endl;
//    if(frame != nullptr)
//        sendOrEnqueue(frame);
}

void SecurityIeee80211MgmtAdhoc::handleCommand(int msgkind, cObject *ctrl)
{
    throw cRuntimeError("handleCommand(): no commands supported");
}

Ieee80211DataFrame *SecurityIeee80211MgmtAdhoc::encapsulate(cPacket *msg)
{
    Ieee80211DataFrameWithSNAP *frame = new Ieee80211DataFrameWithSNAP(msg->getName());

    // copy receiver address from the control info (sender address will be set in MAC)
    Ieee802Ctrl *ctrl = check_and_cast<Ieee802Ctrl *>(msg->removeControlInfo());
    frame->setReceiverAddress(ctrl->getDest());
    frame->setEtherType(ctrl->getEtherType());
    delete ctrl;

    frame->encapsulate(msg);
    return frame;
}

Ieee80211ManagementFrame *SecurityIeee80211MgmtAdhoc::encapsulateMgmt(cPacket *msg)
{
    EV << "ENCAPMGMT 1" << endl;
    Ieee80211ManagementFrame *frame = new Ieee80211ManagementFrame(msg->getName());
    EV << "ENCAPMGMT 2" << endl;
    // copy receiver address from the control info (sender address will be set in MAC)
    Ieee802Ctrl *ctrl = check_and_cast<Ieee802Ctrl *>(msg->removeControlInfo());
    frame->setReceiverAddress(ctrl->getDest());
//    frame->setEtherType(ctrl->getEtherType());
    delete ctrl;
    EV << "ENCAPMGMT 3" << endl;
    frame->encapsulate(msg);
    return frame;
}

void SecurityIeee80211MgmtAdhoc::receiveChangeNotification(int category, const cObject *details)
{
    Enter_Method_Silent();
    printNotificationBanner(category, details);
}

void SecurityIeee80211MgmtAdhoc::handleDataFrame(Ieee80211DataFrame *frame)
{
    sendUp(decapsulate(frame));
}

cPacket *SecurityIeee80211MgmtAdhoc::decapsulate(Ieee80211DataFrame *frame)
{
    cPacket *payload = frame->decapsulate();

    Ieee802Ctrl *ctrl = new Ieee802Ctrl();
    ctrl->setSrc(frame->getTransmitterAddress());
    ctrl->setDest(frame->getReceiverAddress());
    Ieee80211DataFrameWithSNAP *frameWithSNAP = dynamic_cast<Ieee80211DataFrameWithSNAP *>(frame);
    if (frameWithSNAP)
        ctrl->setEtherType(frameWithSNAP->getEtherType());
    payload->setControlInfo(ctrl);

    delete frame;
    return payload;
}

void SecurityIeee80211MgmtAdhoc::handleAuthenticationFrame(Ieee80211AuthenticationFrame *frame)
{
    EV << "Authentication from MAC, send it to SecurityModule" << frame << "\n";
       send(frame, "securityOut");
}

void SecurityIeee80211MgmtAdhoc::handleDeauthenticationFrame(Ieee80211DeauthenticationFrame *frame)
{
    dropManagementFrame(frame);
}

void SecurityIeee80211MgmtAdhoc::handleAssociationRequestFrame(Ieee80211AssociationRequestFrame *frame)
{
    dropManagementFrame(frame);
}

void SecurityIeee80211MgmtAdhoc::handleAssociationResponseFrame(Ieee80211AssociationResponseFrame *frame)
{
    dropManagementFrame(frame);
}

void SecurityIeee80211MgmtAdhoc::handleReassociationRequestFrame(Ieee80211ReassociationRequestFrame *frame)
{
    dropManagementFrame(frame);
}

void SecurityIeee80211MgmtAdhoc::handleReassociationResponseFrame(Ieee80211ReassociationResponseFrame *frame)
{
    dropManagementFrame(frame);
}

void SecurityIeee80211MgmtAdhoc::handleDisassociationFrame(Ieee80211DisassociationFrame *frame)
{
    dropManagementFrame(frame);
}

void SecurityIeee80211MgmtAdhoc::handleBeaconFrame(Ieee80211BeaconFrame *frame)
{
    EV << "Beacon from MAC, send it to SecurityModule" << frame << "\n";
    send(frame, "securityOut");
}

void SecurityIeee80211MgmtAdhoc::handleProbeRequestFrame(Ieee80211ProbeRequestFrame *frame)
{
    dropManagementFrame(frame);
}

void SecurityIeee80211MgmtAdhoc::handleProbeResponseFrame(Ieee80211ProbeResponseFrame *frame)
{
    dropManagementFrame(frame);
}

void SecurityIeee80211MgmtAdhoc::handleCCMPFrame(CCMPFrame *frame)
{
    EV << "CCMP Frame from MAC, send it to SecurityModule" << frame << endl;
    send(frame, "securityOut");
}

void SecurityIeee80211MgmtAdhoc::sendOut(cMessage *msg)
{
    EV << "SecurityIeee80211MgmtAdhoc:: sendOut"<<endl;
    msg->setKind(0);
   hasSecurity = 1;

    //mhn
    if(hasSecurity)
    {
        if (msg->arrivedOn("securityIn"))
        {
            send(msg, "macOut");
        }
        else  if (dynamic_cast<CCMPFrame *>(msg))
        {
            EV << "CCMPFrame Frame arrived from Security, send it to Mac" <<endl;
            error("mhn");
            send(msg, "macOut");
        }
        else
        {
            send(msg, "securityOut");
        }
    }
    else
    {
        send(msg, "macOut");
    }
}

}
}
