//
// Copyright (C) 2012 Univerdidad de Malaga.
// Author: Alfonso Ariza
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "inet/common/WirelessGetNeig.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/ipv4/IPv4InterfaceData.h"
#include "inet/mobility/contract/IMobility.h"

namespace inet{

WirelessGetNeig::WirelessGetNeig() {
    cTopology topo("topo");
    topo.extractByProperty("networkNode");
    cModule *mod = dynamic_cast<cModule*>(getOwner());
    for (mod = dynamic_cast<cModule*>(getOwner())->getParentModule(); mod != 0;
            mod = mod->getParentModule()) {
        cProperties *properties = mod->getProperties();
        if (properties && properties->getAsBool("networkNode"))
            break;
    }
    listNodes.clear();
    for (int i = 0; i < topo.getNumNodes(); i++) {
        cTopology::Node *destNode = topo.getNode(i);
        IMobility *mod;
        cModule *host = destNode->getModule();
        mod = check_and_cast<IMobility *>(host->getSubmodule("mobility"));
        if (mod == nullptr)
            throw cRuntimeError("node or mobility module not found");
        nodeInfo info;
        info.mob = mod;
        info.itable = L3AddressResolver().findInterfaceTableOf(
                destNode->getModule());
        IPv4Address addr =
                L3AddressResolver().getAddressFrom(info.itable).toIPv4();
        listNodes[addr.getInt()] = info;
        for (int i = 0; i < info.itable->getNumInterfaces(); i++) {
            InterfaceEntry * entry = info.itable->getInterface(i);
            if (entry->isLoopback())
                continue;
            listNodesMac[entry->getMacAddress()] = info;

        }
    }
}

WirelessGetNeig::~WirelessGetNeig() {
    // TODO Auto-generated destructor stub
    listNodes.clear();
}

void WirelessGetNeig::getNeighbours(const IPv4Address &node,
        std::vector<IPv4Address>&list, const double &distance) {
    list.clear();

    auto it = listNodes.find(node.getInt());
    if (it == listNodes.end())
        throw cRuntimeError("node not found");

    Coord pos = it->second.mob->getCurrentPosition();
    for (it = listNodes.begin(); it != listNodes.end(); ++it) {
        if (it->first == node.getInt())
            continue;
        if (pos.distance(it->second.mob->getCurrentPosition()) < distance) {
            list.push_back(IPv4Address(it->first));
        }
    }
}

void WirelessGetNeig::getNeighbours(const MACAddress &node, std::vector<MACAddress>&list, const double &distance) {
    list.clear();

    auto it = listNodesMac.find(node);
    if (it == listNodesMac.end())
        throw cRuntimeError("node not found");

    Coord pos = it->second.mob->getCurrentPosition();
    for (it = listNodesMac.begin(); it != listNodesMac.end(); ++it) {
        if (it->first == node)
            continue;
        if (pos.distance(it->second.mob->getCurrentPosition()) < distance) {
            list.push_back(MACAddress(it->first));
        }
    }
}

void WirelessGetNeig::getNeighbours(const IPv4Address &node, std::vector<IPv4Address>&list, const double &distance, std::vector<Coord> & coord) {
    list.clear();

    auto it = listNodes.find(node.getInt());
    if (it == listNodes.end())
        throw cRuntimeError("node not found");

    Coord pos = it->second.mob->getCurrentPosition();
    for (it = listNodes.begin(); it != listNodes.end(); ++it) {
        if (it->first == node.getInt())
            continue;
        if (pos.distance(it->second.mob->getCurrentPosition()) < distance) {
            list.push_back(IPv4Address(it->first));
            coord.push_back(pos);
        }
    }
}

void WirelessGetNeig::getNeighbours(const MACAddress &node, std::vector<MACAddress>&list, const double &distance, std::vector<Coord> &coord) {
    list.clear();

    auto it = listNodesMac.find(node);
    if (it == listNodesMac.end())
        throw cRuntimeError("node not found");

    Coord pos = it->second.mob->getCurrentPosition();
    for (it = listNodesMac.begin(); it != listNodesMac.end(); ++it) {
        if (it->first == node)
            continue;
        if (pos.distance(it->second.mob->getCurrentPosition()) < distance) {
            list.push_back(MACAddress(it->first));
            coord.push_back(pos);
        }
    }
}

}
