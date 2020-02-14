//
// Copyright (C) 2016 OpenSim Ltd.
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
// along with this program; if not, see http://www.gnu.org/licenses/.
//

#include "inet/common/INETUtils.h"
#include "inet/common/ModuleAccess.h"
#include "inet/linklayer/ieee80211/mac/contract/IContention.h"
#include "inet/linklayer/ieee80211/mac/contract/IFrameSequence.h"
#include "inet/linklayer/ieee80211/mac/contract/IRx.h"
#include "inet/linklayer/ieee80211/mac/contract/ITx.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Mac.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211ControlInfo_m.h"

namespace inet {
namespace ieee80211 {

Define_Module(Ieee80211Mac);

simsignal_t Ieee80211Mac::stateSignal = SIMSIGNAL_NULL;
simsignal_t Ieee80211Mac::radioStateSignal = SIMSIGNAL_NULL;

Ieee80211Mac::Ieee80211Mac()
{
}

Ieee80211Mac::~Ieee80211Mac()
{
    if (pendingRadioConfigMsg)
        delete pendingRadioConfigMsg;
}

void Ieee80211Mac::initialize(int stage)
{
    MACProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        qosSta = par("qosStation");
        cModule *radioModule = gate("lowerLayerOut")->getNextGate()->getOwnerModule();
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::receptionStateChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radioModule->subscribe(IRadio::receivedSignalPartChangedSignal, this);
        radio = check_and_cast<IRadio *>(radioModule);
        rx = check_and_cast<IRx *>(getSubmodule("rx"));
        tx = check_and_cast<ITx *>(getSubmodule("tx"));
        const char *addressString = par("address");
        if (!strcmp(addressString, "auto")) {
            // change module parameter from "auto" to concrete address
            par("address").setStringValue(MACAddress::generateAutoAddress().str().c_str());
            addressString = par("address");
        }
        address.setAddress(addressString);
        modeSet = Ieee80211ModeSet::getModeSet(par("modeSet").stringValue());
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        registerInterface();
        emit(NF_MODESET_CHANGED, modeSet);
        if (isOperational)
            radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        if (isInterfaceRegistered().isUnspecified())// TODO: do we need multi-MAC feature? if so, should they share interfaceEntry??  --Andras
            registerInterface();
    }
    else if (stage == INITSTAGE_LINK_LAYER_2) {
        rx = check_and_cast<IRx *>(getSubmodule("rx"));
        tx = check_and_cast<ITx *>(getSubmodule("tx"));
        dcf = check_and_cast<Dcf *>(getSubmodule("dcf"));
        hcf = check_and_cast<Hcf *>(getSubmodule("hcf"));
    }
}

const MACAddress& Ieee80211Mac::isInterfaceRegistered()
{
    // if (!par("multiMac").boolValue())
    //    return MACAddress::UNSPECIFIED_ADDRESS;
    IInterfaceTable *ift = findModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    if (!ift)
        return MACAddress::UNSPECIFIED_ADDRESS;
    cModule *interfaceModule = findModuleUnderContainingNode(this);
    if (!interfaceModule)
        throw cRuntimeError("NIC module not found in the host");
    std::string interfaceName = utils::stripnonalnum(interfaceModule->getFullName());
    InterfaceEntry *e = ift->getInterfaceByName(interfaceName.c_str());
    if (e)
        return e->getMacAddress();
    return MACAddress::UNSPECIFIED_ADDRESS;
}

InterfaceEntry *Ieee80211Mac::createInterfaceEntry()
{
    InterfaceEntry *e = new InterfaceEntry(this);
    // address
    e->setMACAddress(address);
    e->setInterfaceToken(address.formInterfaceIdentifier());
    e->setMtu(par("mtu").longValue());
    // capabilities
    e->setBroadcast(true);
    e->setMulticast(true);
    e->setPointToPoint(false);
    return e;
}

void Ieee80211Mac::handleSelfMessage(cMessage *msg)
{
    ASSERT(false);
}

void Ieee80211Mac::handleUpperPacket(cPacket *msg)
{
    processUpperFrame(check_and_cast<Ieee80211DataOrMgmtFrame *>(msg));
}

void Ieee80211Mac::handleLowerPacket(cPacket *msg)
{
    auto frame = check_and_cast<Ieee80211Frame *>(msg);
    if (rx->lowerFrameReceived(frame)) {
        processLowerFrame(frame);
    }
    else { // corrupted frame received
        if (qosSta)
            hcf->corruptedFrameReceived();
        else
            dcf->corruptedFrameReceived();
    }
}

void Ieee80211Mac::handleUpperCommand(cMessage *msg)
{
    if (msg->getKind() == RADIO_C_CONFIGURE) {
        EV_DEBUG << "Passing on command " << msg->getName() << " to physical layer\n";
        if (pendingRadioConfigMsg != nullptr) {
            // merge contents of the old command into the new one, then delete it
            Ieee80211ConfigureRadioCommand *oldConfigureCommand = check_and_cast<Ieee80211ConfigureRadioCommand *>(pendingRadioConfigMsg->getControlInfo());
            Ieee80211ConfigureRadioCommand *newConfigureCommand = check_and_cast<Ieee80211ConfigureRadioCommand *>(msg->getControlInfo());
            if (newConfigureCommand->getChannelNumber() == -1 && oldConfigureCommand->getChannelNumber() != -1)
                newConfigureCommand->setChannelNumber(oldConfigureCommand->getChannelNumber());
            if (std::isnan(newConfigureCommand->getBitrate().get()) && !std::isnan(oldConfigureCommand->getBitrate().get()))
                newConfigureCommand->setBitrate(oldConfigureCommand->getBitrate());
            delete pendingRadioConfigMsg;
            pendingRadioConfigMsg = nullptr;
        }

        if (rx->isMediumFree()) {    // TODO: this should be just the physical channel sense!!!!
            EV_DEBUG << "Sending it down immediately\n";
            // PhyControlInfo *phyControlInfo = dynamic_cast<PhyControlInfo *>(msg->getControlInfo());
            // if (phyControlInfo)
            // phyControlInfo->setAdaptiveSensitivity(true);
            // end dynamic power
            sendDown(msg);
        }
        else {
            // TODO: waiting potentially indefinitely?! wtf?!
            EV_DEBUG << "Delaying " << msg->getName() << " until next IDLE or DEFER state\n";
            pendingRadioConfigMsg = msg;
        }
    }
    else {
        throw cRuntimeError("Unrecognized command from mgmt layer: (%s)%s msgkind=%d", msg->getClassName(), msg->getName(), msg->getKind());
    }
}

void Ieee80211Mac::receiveSignal(cComponent *source, simsignal_t signalID, long value, cObject *details)
{
    Enter_Method_Silent("receiveSignal()");
    if (signalID == IRadio::receptionStateChangedSignal) {
        rx->receptionStateChanged((IRadio::ReceptionState)value);
    }
    else if (signalID == IRadio::transmissionStateChangedSignal) {
        auto oldTransmissionState = transmissionState;
        transmissionState = (IRadio::TransmissionState)value;
        bool transmissionFinished = (oldTransmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && transmissionState == IRadio::TRANSMISSION_STATE_IDLE);
        if (transmissionFinished) {
            tx->radioTransmissionFinished();
            EV_DEBUG << "changing radio to receiver mode\n";
            configureRadioMode(IRadio::RADIO_MODE_RECEIVER); // FIXME: this is in a very wrong place!!! should be done explicitly from UpperMac!
        }
        rx->transmissionStateChanged(transmissionState);
    }
    else if (signalID == IRadio::receivedSignalPartChangedSignal) {
        rx->receivedSignalPartChanged((IRadioSignal::SignalPart)value);
    }
}

void Ieee80211Mac::configureRadioMode(IRadio::RadioMode radioMode)
{
    if (radio->getRadioMode() != radioMode) {
        ConfigureRadioCommand *configureCommand = new ConfigureRadioCommand();
        configureCommand->setRadioMode(radioMode);
        cMessage *message = new cMessage("configureRadioMode", RADIO_C_CONFIGURE);
        message->setControlInfo(configureCommand);
        sendDown(message);
    }
}

void Ieee80211Mac::sendUp(cMessage *msg)
{
    Enter_Method("sendUp(\"%s\")", msg->getName());
    take(msg);
    MACProtocolBase::sendUp(msg);
}

void Ieee80211Mac::sendFrame(Ieee80211Frame *frame)
{
    Enter_Method("sendFrame(\"%s\")", frame->getName());
    take(frame);
    configureRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
    sendDown(frame);
}

void Ieee80211Mac::sendDownPendingRadioConfigMsg()
{
    if (pendingRadioConfigMsg != nullptr) {
        sendDown(pendingRadioConfigMsg);
        pendingRadioConfigMsg = nullptr;
    }
}

void Ieee80211Mac::processUpperFrame(Ieee80211DataOrMgmtFrame* frame)
{
    Enter_Method("processUpperFrame(\"%s\")", frame->getName());
    take(frame);
    EV_INFO << "Frame " << frame << " received from higher layer, receiver = " << frame->getReceiverAddress() << "\n";
    ASSERT(!frame->getReceiverAddress().isUnspecified());
    if (qosSta)
        hcf->processUpperFrame(frame);
    else
        dcf->processUpperFrame(frame);
}

void Ieee80211Mac::processLowerFrame(Ieee80211Frame* frame)
{
    Enter_Method("processLowerFrame(\"%s\")", frame->getName());
    take(frame);
    if (qosSta)
        hcf->processLowerFrame(frame);
    else
        dcf->processLowerFrame(frame);
}

// FIXME
bool Ieee80211Mac::handleNodeStart(IDoneCallback *doneCallback)
{
    if (!doneCallback)
        return true;    // do nothing when called from initialize()

    bool ret = MACProtocolBase::handleNodeStart(doneCallback);
    radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
    return ret;
}

// FIXME
bool Ieee80211Mac::handleNodeShutdown(IDoneCallback *doneCallback)
{
    bool ret = MACProtocolBase::handleNodeStart(doneCallback);
    handleNodeCrash();
    return ret;
}

// FIXME
void Ieee80211Mac::handleNodeCrash()
{
}

} // namespace ieee80211
} // namespace inet
