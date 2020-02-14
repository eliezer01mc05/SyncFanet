//
// Copyright (C) OpenSim Ltd.
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

#include <algorithm>
#include "inet/common/ModuleAccess.h"
#include "inet/common/NotifierConsts.h"
#include "inet/linklayer/contract/IMACFrame.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/mobility/contract/IMobility.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/visualizer/base/LinkBreakVisualizerBase.h"

namespace inet {

namespace visualizer {

LinkBreakVisualizerBase::LinkBreakVisualization::LinkBreakVisualization(int transmitterModuleId, int receiverModuleId) :
    transmitterModuleId(transmitterModuleId),
    receiverModuleId(receiverModuleId)
{
}

LinkBreakVisualizerBase::~LinkBreakVisualizerBase()
{
    if (displayLinkBreaks)
        unsubscribe();
}

void LinkBreakVisualizerBase::initialize(int stage)
{
    VisualizerBase::initialize(stage);
    if (!hasGUI()) return;
    if (stage == INITSTAGE_LOCAL) {
        displayLinkBreaks = par("displayLinkBreaks");
        nodeFilter.setPattern(par("nodeFilter"));
        interfaceFilter.setPattern(par("interfaceFilter"));
        packetFilter.setPattern(par("packetFilter"));
        icon = par("icon");
        iconTintAmount = par("iconTintAmount");
        if (iconTintAmount != 0)
            iconTintColor = cFigure::Color(par("iconTintColor"));
        fadeOutMode = par("fadeOutMode");
        fadeOutTime = par("fadeOutTime");
        fadeOutAnimationSpeed = par("fadeOutAnimationSpeed");
        if (displayLinkBreaks)
            subscribe();
    }
}

void LinkBreakVisualizerBase::handleParameterChange(const char *name)
{
    if (name != nullptr) {
        if (!strcmp(name, "nodeFilter"))
            nodeFilter.setPattern(par("nodeFilter"));
        else if (!strcmp(name, "interfaceFilter"))
            interfaceFilter.setPattern(par("interfaceFilter"));
        else if (!strcmp(name, "packetFilter"))
            packetFilter.setPattern(par("packetFilter"));
        removeAllLinkBreakVisualizations();
    }
}

void LinkBreakVisualizerBase::refreshDisplay() const
{
    AnimationPosition currentAnimationPosition;
    std::vector<const LinkBreakVisualization *> removedLinkBreakVisualizations;
    for (auto it : linkBreakVisualizations) {
        auto linkBreakVisualization = it.second;
        double delta;
        if (!strcmp(fadeOutMode, "simulationTime"))
            delta = (currentAnimationPosition.getSimulationTime() - linkBreakVisualization->linkBreakAnimationPosition.getSimulationTime()).dbl();
        else if (!strcmp(fadeOutMode, "animationTime"))
            delta = currentAnimationPosition.getAnimationTime() - linkBreakVisualization->linkBreakAnimationPosition.getAnimationTime();
        else if (!strcmp(fadeOutMode, "realTime"))
            delta = currentAnimationPosition.getRealTime() - linkBreakVisualization->linkBreakAnimationPosition.getRealTime();
        else
            throw cRuntimeError("Unknown fadeOutMode: %s", fadeOutMode);
        if (delta > fadeOutTime)
            removedLinkBreakVisualizations.push_back(linkBreakVisualization);
        else
            setAlpha(linkBreakVisualization, 1 - delta / fadeOutTime);
    }
    for (auto linkBreakVisualization : removedLinkBreakVisualizations) {
        const_cast<LinkBreakVisualizerBase *>(this)->removeLinkBreakVisualization(linkBreakVisualization);
        delete linkBreakVisualization;
    }
}

void LinkBreakVisualizerBase::subscribe()
{
    auto subscriptionModule = getModuleFromPar<cModule>(par("subscriptionModule"), this);
    subscriptionModule->subscribe(NF_LINK_BREAK, this);
}

void LinkBreakVisualizerBase::unsubscribe()
{
    // NOTE: lookup the module again because it may have been deleted first
    auto subscriptionModule = getModuleFromPar<cModule>(par("subscriptionModule"), this, false);
    if (subscriptionModule != nullptr)
        subscriptionModule->unsubscribe(NF_LINK_BREAK, this);
}

void LinkBreakVisualizerBase::receiveSignal(cComponent *source, simsignal_t signal, cObject *object, cObject *details)
{
    Enter_Method_Silent();
    if (signal == NF_LINK_BREAK) {
        MACAddress transmitterAddress;
        MACAddress receiverAddress;
        if (auto frame = dynamic_cast<IMACFrame *>(object)) {
            transmitterAddress = frame->getTransmitterAddress();
            receiverAddress = frame->getReceiverAddress();
        }
        if (auto frame = dynamic_cast<ieee80211::Ieee80211TwoAddressFrame *>(object)) {
            transmitterAddress = frame->getTransmitterAddress();
            receiverAddress = frame->getReceiverAddress();
        }
        auto transmitter = findNode(transmitterAddress);
        auto receiver = findNode(receiverAddress);
        if (nodeFilter.matches(transmitter) && nodeFilter.matches(receiver)) {
            auto key = std::pair<int, int>(transmitter->getId(), receiver->getId());
            auto it = linkBreakVisualizations.find(key);
            if (it == linkBreakVisualizations.end())
                addLinkBreakVisualization(createLinkBreakVisualization(transmitter, receiver));
            else {
                auto linkBreakVisualization = it->second;
                linkBreakVisualization->linkBreakAnimationPosition = AnimationPosition();
            }
        }
    }
    else
        throw cRuntimeError("Unknown signal");
}

void LinkBreakVisualizerBase::addLinkBreakVisualization(const LinkBreakVisualization *linkBreakVisualization)
{
    auto key = std::pair<int, int>(linkBreakVisualization->transmitterModuleId, linkBreakVisualization->receiverModuleId);
    linkBreakVisualizations[key] = linkBreakVisualization;
}

void LinkBreakVisualizerBase::removeLinkBreakVisualization(const LinkBreakVisualization *linkBreakVisualization)
{
    auto key = std::pair<int, int>(linkBreakVisualization->transmitterModuleId, linkBreakVisualization->receiverModuleId);
    auto it = linkBreakVisualizations.find(key);
    linkBreakVisualizations.erase(it);
}

// TODO: inefficient, create L2AddressResolver?
cModule *LinkBreakVisualizerBase::findNode(MACAddress address)
{
    L3AddressResolver addressResolver;
    for (cModule::SubmoduleIterator it(getSystemModule()); !it.end(); it++) {
        auto networkNode = *it;
        if (isNetworkNode(networkNode)) {
            auto interfaceTable = addressResolver.findInterfaceTableOf(networkNode);
            for (int i = 0; i < interfaceTable->getNumInterfaces(); i++)
                if (interfaceTable->getInterface(i)->getMacAddress() == address)
                    return networkNode;
        }
    }
    return nullptr;
}

void LinkBreakVisualizerBase::removeAllLinkBreakVisualizations()
{
    std::vector<const LinkBreakVisualization *> removedLinkBreakVisualizations;
    for (auto it : linkBreakVisualizations)
        removedLinkBreakVisualizations.push_back(it.second);
    for (auto it : removedLinkBreakVisualizations) {
        removeLinkBreakVisualization(it);
        delete it;
    }
}

} // namespace visualizer

} // namespace inet

