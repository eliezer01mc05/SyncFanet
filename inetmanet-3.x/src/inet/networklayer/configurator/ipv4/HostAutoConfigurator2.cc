/*
* HostAutoConfigurator - automatically assigns IP addresses and sets up routing table
* Copyright (C) 2009 Christoph Sommer <christoph.sommer@informatik.uni-erlangen.de>
* Copyright (C) 2010 Alfonso Ariza
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <stdexcept>
#include <algorithm>

#include "inet/networklayer/configurator/ipv4/HostAutoConfigurator2.h"

#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/ipv4/IPv4InterfaceData.h"
#include "inet/networklayer/ipv4/IIPv4RoutingTable.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/contract/ipv4/IPv4Address.h"


namespace inet{

std::deque<L3Address> HostAutoConfigurator2::asignedAddress;
bool HostAutoConfigurator2::firstTime = true;

Define_Module(HostAutoConfigurator2);

static IPv4Address defaultAddr;

HostAutoConfigurator2::HostAutoConfigurator2()
{
    defaultAddr.set(0,0,0,0);
    if (firstTime)
    {
        asignedAddress.clear();
        firstTime = false;
    }
}

HostAutoConfigurator2::~HostAutoConfigurator2()
{
    for (unsigned int j=0;j<myAddressList.size();j++)
    {
        for (unsigned int i=0;i<asignedAddress.size();i++)
        {
            if (asignedAddress[i] == myAddressList[j])
            {
                asignedAddress.erase(asignedAddress.begin()+i);
                break;
            }
        }
    }
}

void HostAutoConfigurator2::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == 0)
    {
        debug = par("debug").boolValue();
    }
    else if (stage == INITSTAGE_NETWORK_LAYER_2)
    {
        setupNetworkLayer();
        if (par("isDefaultRoute"))
        {
            if (!defaultAddr.isUnspecified())
                throw cRuntimeError("default router is defined yet");
            cModule* host = getParentModule();
            IInterfaceTable *ift = L3AddressResolver().interfaceTableOf(host);
            InterfaceEntry* ie = ift->getInterfaceByName(par("defaultAddressInterface"));
            if (ie==nullptr)
                throw cRuntimeError("default ID interface doesn't exist");
            defaultAddr=ie->ipv4Data()->getIPAddress();
        }
    }
    else if (stage == INITSTAGE_NETWORK_LAYER_3)
    {
        setupRoutingTable();
    }
}

void HostAutoConfigurator2::finish()
{
}

void HostAutoConfigurator2::handleMessage(cMessage* apMsg)
{
}

void HostAutoConfigurator2::handleSelfMsg(cMessage* apMsg)
{
}
namespace
{

void addToMcastGroup(InterfaceEntry* ie, IIPv4RoutingTable* routingTable, const IPv4Address& mcastGroup)
{

    ie->ipv4Data()->joinMulticastGroup(mcastGroup);

    IPv4Route* re = new IPv4Route(); //TODO: add @c delete to destructor
    re->setDestination(mcastGroup);
    re->setNetmask(IPv4Address::ALLONES_ADDRESS); // TODO: can't set this to none?
    re->setGateway(IPv4Address()); // none
    re->setInterface(ie);
    re->setSourceType(IRoute::MANUAL);
    re->setMetric(1);
    routingTable->addRoute(re);
}

void addRoute(InterfaceEntry* ie, IIPv4RoutingTable* routingTable, const IPv4Address& ipaddress,const IPv4Address& maskaddress)
{
    //
	//
	IPv4Route* re = new IPv4Route(); //TODO: add @c delete to destructor
    re->setDestination(ipaddress);
    re->setNetmask(maskaddress); // TODO: can't set this to none?
    re->setGateway(IPv4Address::UNSPECIFIED_ADDRESS); // none
    re->setInterface(ie);
    re->setSourceType(IRoute::MANUAL);
    re->setMetric(1);
    routingTable->addRoute(re);
}
}




void HostAutoConfigurator2::addDefaultRoutes()
{
    // add default route to nodes with exactly one (non-loopback) interface
    // get our interface table
    // get our host module
    cModule* host = getParentModule();
    if (!host) throw cRuntimeError("No parent module found");
    IInterfaceTable *ift = L3AddressResolver().interfaceTableOf(host);
    IIPv4RoutingTable* rt = L3AddressResolver().routingTableOf(host);

    // count non-loopback interfaces
    int numIntf = 0;
    InterfaceEntry *ie = nullptr;
    for (int k=0; k<ift->getNumInterfaces(); k++)
        if (!ift->getInterface(k)->isLoopback())
            {ie = ift->getInterface(k); numIntf++;}

    if (numIntf!=1)
        return; // only deal with nodes with one interface plus loopback
    int i=0;
    while (i<rt->getNumRoutes())
    {
        if (rt->getRoute(i)->getInterface()==ie)
    	{
            rt->deleteRoute(rt->getRoute(i));
            i=0;
    	}
    	else
    	    i++;
    }
    IPv4Route *e = new IPv4Route();
    e->setDestination(IPv4Address());
    e->setNetmask(IPv4Address());
    e->setInterface(ie);
    e->setSourceType(IRoute::MANUAL);
     //e->getMetric() = 1;
    rt->addRoute(e);
}


void HostAutoConfigurator2::addDefaultRoute()
{
    if (!par("setDefaultRoute").boolValue())
        return;


    std::string addressMask = par("defaultRouteInterface").stringValue();

    // add default route to nodes with exactly one (non-loopback) interface
    // get our interface table
    // get our host module
    cModule* host = getParentModule();
    if (!host) throw cRuntimeError("No parent module found");
    IInterfaceTable *ift = L3AddressResolver().interfaceTableOf(host);
    IRoutingTable* rt = L3AddressResolver().routingTableOf(host);

    // count non-loopback interfaces
    InterfaceEntry *ie = ift->getInterfaceByName(addressMask.c_str());
    if (!ie)
        return;

    IPv4Route *e = new IPv4Route();
    e->setDestination(IPv4Address());
    e->setNetmask(IPv4Address());
    e->setInterface(ie);
    e->setSourceType(IPv4Route::MANUAL);
    rt->addRoute(e);
}

void HostAutoConfigurator2::setupNetworkLayer()
{
    EV << "host auto configuration started" << std::endl;

    std::string interfaces = par("interfaces").stringValue();
    std::string addressBaseToken = par("addressBaseList").stringValue();
    std::string addressMask = par("addressMask").stringValue();

    cStringTokenizer interfaceTokenizer(interfaces.c_str());
    cStringTokenizer addressBaseTokenizer(addressBaseToken.c_str());
    cStringTokenizer addressMaskTokenizer(addressMask.c_str());


    //IPv4Address addressBase = IPv4Address(par("addressBase").stringValue());
    //std::string mcastGroups = par("mcastGroups").stringValue();


    // get our host module
    cModule* host = getParentModule();
    if (!host) throw cRuntimeError("No parent module found");

    // get our routing table
    IIPv4RoutingTable* routingTable = L3AddressResolver().routingTableOf(host);
    if (!routingTable) throw cRuntimeError("No routing table found");

    // get our interface table
    IInterfaceTable *ift = L3AddressResolver().interfaceTableOf(host);
    if (!ift) throw cRuntimeError("No interface table found");

    IPv4Address myAddress;
    IPv4Address netmask;
    // look at all interface table entries
    const char *ifname;
    std::vector<std::string> vectorInterfaceToken;
    std::vector<std::string> vectorAddressToken;
    std::vector<std::string> vectorMaskToken;
    while((ifname = interfaceTokenizer.nextToken()) != nullptr)
    {
    	std::string val(ifname);
    	vectorInterfaceToken.push_back(val);
    }

    while((ifname = addressBaseTokenizer.nextToken()) != nullptr)
    {
    	std::string val(ifname);
    	vectorAddressToken.push_back(val);
    }

    while((ifname = addressMaskTokenizer.nextToken()) != nullptr)
    {
    	std::string val(ifname);
    	vectorMaskToken.push_back(val);
    }

    if ((vectorInterfaceToken.size() > 1) && (vectorAddressToken.size()!=vectorInterfaceToken.size()))
    	throw cRuntimeError("interfaces and addressBaseList has different sizes");

    if (vectorAddressToken.size()!=vectorMaskToken.size())
    	throw cRuntimeError("addressMask and addressBaseList has different sizes");


    bool interfaceFound = false;
    for (unsigned int i=0;i<vectorInterfaceToken.size();i++)
    {
    	ifname = vectorInterfaceToken[i].c_str();
        InterfaceEntry* ie = ift->getInterfaceByName(ifname);

        if (!ie)
            continue;

        // assign IP Address to all connected interfaces
        if (ie->isLoopback())
        {
            EV << "interface " << ifname << " skipped (is loopback)" << std::endl;
            continue;
        }
        interfaceFound=true;
        IPv4Address addressBase;
        if (vectorAddressToken.size()>1)
        {
            addressBase.set(vectorAddressToken[i].c_str());
            netmask = IPv4Address(vectorMaskToken[i].c_str());
        }
        else
        {
        	addressBase.set(vectorAddressToken[0].c_str());
        	netmask = IPv4Address(vectorMaskToken[0].c_str());
        }

        int cont = 0;

        do
        {
            cont++;
            // search other address
            myAddress = IPv4Address(addressBase.getInt() + cont);
            // check if the address is valid.
            if (!IPv4Address::maskedAddrAreEqual(myAddress, addressBase, netmask)) // match
                error("check address asignation, more address that possible with this address base %s and this mask %s",addressBase.str().c_str(),netmask.str().c_str());

        }
        while(checkIfExist(myAddress));

        ie->ipv4Data()->setIPAddress(myAddress);
        ie->ipv4Data()->setNetmask(netmask);
        ie->setBroadcast(true);
        asignedAddress.push_back(myAddress);
        myAddressList.push_back(myAddress);
        EV << "interface " << ifname << " gets " << myAddress.str() << "/" << netmask.str() << std::endl;
    }
    if (!interfaceFound)
    	throw cRuntimeError("Not interface register");
}

void HostAutoConfigurator2::setupRoutingTable()
{
    std::string interfaces = par("interfaces").stringValue();
    std::string addressBaseToken = par("addressBaseList").stringValue();
    std::string addressMask = par("addressMask").stringValue();

    cStringTokenizer interfaceTokenizer(interfaces.c_str());
    cStringTokenizer addressBaseTokenizer(addressBaseToken.c_str());
    cStringTokenizer addressMaskTokenizer(addressMask.c_str());


    cModule* host = getParentModule();
    // get our routing table
    IIPv4RoutingTable* routingTable = L3AddressResolver().routingTableOf(host);
    // get our interface table
    IInterfaceTable *ift = L3AddressResolver().interfaceTableOf(host);

    if (ift->getNumInterfaces()==2)
    {
        addDefaultRoutes();
        return;
    }

    const char *ifname;
    std::vector<std::string> vectorInterfaceToken;
    std::vector<std::string> vectorAddressToken;
    std::vector<std::string> vectorMaskToken;
    while((ifname = interfaceTokenizer.nextToken()) != nullptr)
    {
    	std::string val(ifname);
    	vectorInterfaceToken.push_back(val);
    }

    while((ifname = addressBaseTokenizer.nextToken()) != nullptr)
    {
    	std::string val(ifname);
    	vectorAddressToken.push_back(val);
    }

    while((ifname = addressMaskTokenizer.nextToken()) != nullptr)
    {
    	std::string val(ifname);
    	vectorMaskToken.push_back(val);
    }

    if (vectorAddressToken.size()>1 && (vectorAddressToken.size()!=vectorInterfaceToken.size()))
    	throw cRuntimeError("interfaces and addressBaseList has different sizes");

    if (vectorAddressToken.size()!=vectorMaskToken.size())
    	throw cRuntimeError("addressMask and addressBaseList has different sizes");


    if (vectorAddressToken.size()==1)
    {
    	fillRoutingTables();
        return;
    }
    // get our host module


    for (unsigned int j = 0; j < vectorInterfaceToken.size(); j++)
    {
    	ifname = vectorInterfaceToken[j].c_str();
        InterfaceEntry* ie = ift->getInterfaceByName(ifname);
        if (!ie)
            continue;
        IPv4Address add(ie->ipv4Data()->getIPAddress().getInt() & ie->ipv4Data()->getNetmask().getInt());
        int i=0;
        while ( i < routingTable->getNumRoutes())
        {
            if (routingTable->getRoute(i)->getInterface() == ie)
        	{
            	routingTable->deleteRoute(routingTable->getRoute(i));
                i=0;
        	}
        	else
        	    i++;
        }
        addRoute(ie,routingTable,add,ie->ipv4Data()->getNetmask());
    }

    if (!defaultAddr.isUnspecified())
    {
    	InterfaceEntry* ie = ift->getInterfaceByName(par("defaultInterface"));
        if (ie==nullptr)
            throw cRuntimeError("default ID interface doesn't exist");
        IPv4Route *e = new IPv4Route();
        e->setDestination(IPv4Address());
        e->setNetmask(IPv4Address());
        e->setGateway(defaultAddr);
        e->setInterface(ie);
        e->setSourceType(IRoute::MANUAL);
         //e->getMetric() = 1;
        routingTable->addRoute(e);
    }
}



void HostAutoConfigurator2::fillRoutingTables()
{
    // fill in routing tables with static routes
	cTopology topo("topo");
	topo.extractByProperty("networkNode");

    cModule* host = getParentModule();
    // get our routing table
    IIPv4RoutingTable* routingTable = L3AddressResolver().routingTableOf(host);

    for (int i=0; i<topo.getNumNodes(); i++)
    {
    	if (getParentModule()== topo.getNode(i)->getModule())
    		continue;
        cTopology::Node *destNode = topo.getNode(i);

        // skip bus types

        std::string destModName = destNode->getModule()->getFullName();

        // calculate shortest paths from everywhere towards destNode
        topo.calculateUnweightedSingleShortestPathsTo(destNode);


        // get our interface table
        IInterfaceTable *ift = L3AddressResolver().interfaceTableOf(topo.getNode(i)->getModule());
        if (ift==nullptr)
            continue;

        // add route (with host=destNode) to every routing table in the network
        // (excepting nodes with only one interface -- there we'll set up a default route)
        for (int j=0; j<topo.getNumNodes(); j++)
        {
            if (i==j) continue;

            cTopology::Node *atNode = topo.getNode(j);
            if (getParentModule()!=topo.getNode(j)->getModule())
            	continue;

            if (atNode->getNumPaths()==0)
                continue; // not connected

            IInterfaceTable *iftN = L3AddressResolver().interfaceTableOf(topo.getNode(j)->getModule());
            if (iftN==nullptr)
            	continue;

          	int outputGateIdTarget =  atNode->getPath(atNode->getNumInLinks()-1)->getLocalGate()->getId();
            InterfaceEntry *ieTarget = ift->getInterfaceByNodeOutputGateId(outputGateIdTarget);
            if (!ieTarget)
                error("%s has no interface for output gate id %d", ift->getFullPath().c_str(), outputGateIdTarget);


            int outputGateId = atNode->getPath(0)->getLocalGate()->getId();
            InterfaceEntry *ie = iftN->getInterfaceByNodeOutputGateId(outputGateId);

            if (!ie)
                error("%s has no interface for output gate id %d", iftN->getFullPath().c_str(), outputGateId);

            IPv4Route *e = new IPv4Route();
            e->setDestination(ieTarget->ipv4Data()->getIPAddress());
            e->setNetmask(IPv4Address(255,255,255,255)); // full match needed
            e->setInterface(ie);
            e->setSourceType(IRoute::MANUAL);
            //e->getMetric() = 1;
            routingTable->addRoute(e);
        }
    }
}


bool HostAutoConfigurator2::checkIfExist(const L3Address &add)
{
    for (unsigned int i=0; i<asignedAddress.size();i++)
    {
        if (add == asignedAddress[i])
            return true;
    }
    return false;
}

} //namespace

