//
// Copyright (C) 2008 Alfonso Ariza
// Copyright (C) 2010 Alfonso Ariza
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

#define CHEAT_IEEE80211MESH
#include <string.h>

#include "inet/linklayer/ieee80211mesh/mgmt/Ieee80211Mesh.h"
#include "inet/routing/extras/base/MeshControlInfo_m.h"
#include "inet/routing/extras/base/ControlInfoBreakLink_m.h"
#include "inet/routing/extras/base/ControlManetRouting_m.h"
#include "inet/routing/extras/olsr/OLSRpkt_m.h"
#include "inet/routing/extras/dymo/dymo_msg_struct.h"
#include "inet/routing/extras/aodv-uu/aodv_msg_struct.h"
#include "inet/networklayer/ipv4/IPv4Datagram.h"
#include "inet/networklayer/ipv6/IPv6Datagram.h"
#include "inet/networklayer/ted/LinkStatePacket_m.h"
#include "inet/networklayer/mpls/MPLSPacket.h"
#include "inet/networklayer/arp/ipv4/ARPPacket_m.h"
#include "inet/routing/ospfv2/OSPFPacket_m.h"
#include "inet/routing/extras/olsr/OLSR.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"



// TODO: create a specific LWMPLs class and separate the specific LWMPS proccess

/* WMPLS */

#define WLAN_MPLS_TIME_DELETE  6

#define WLAN_MPLS_TIME_REFRESH 3



#if !defined (UINT32_MAX)
#   define UINT32_MAX  4294967295UL
#endif

namespace inet{

namespace ieee80211 {

cPacket *Ieee80211Mesh::decapsulateMpls(LWMPLSPacket *frame)
{
    cPacket *payload = frame->decapsulate();
    // ctrl->setSrc(frame->getAddress3());
    Ieee802Ctrl *ctrl = (Ieee802Ctrl*) frame->removeControlInfo();
    payload->setControlInfo(ctrl);
    delete frame;
    return payload;
}

void Ieee80211Mesh::mplsSendAck(int label)
{
    if (label >= LWMPLS_MAX_LABEL || label <= 0)
        throw cRuntimeError("mplsSendAck error in label %i", label);
    LWMPLSPacket *mpls_pk_aux_ptr = new LWMPLSPacket();
    mpls_pk_aux_ptr->setLabelReturn(label);
    LWmpls_Forwarding_Structure * forwarding_ptr = mplsData->lwmpls_forwarding_data(label, 0, 0);

    MACAddress sta_addr;
    int return_label = -1;
    if (forwarding_ptr->input_label == label)
    {
        sta_addr = MACAddress(forwarding_ptr->input_mac_address);
        return_label = forwarding_ptr->return_label_output;
    }
    else if (forwarding_ptr->return_label_input == label)
    {
        sta_addr = MACAddress(forwarding_ptr->mac_address);
        return_label = forwarding_ptr->output_label;
    }
    else
        throw cRuntimeError("Label error");
    mpls_pk_aux_ptr->setType(WMPLS_ACK);
    mpls_pk_aux_ptr->setLabel(return_label);
    mpls_pk_aux_ptr->setDest(sta_addr);
    mpls_pk_aux_ptr->setSource(myAddress);
//  sendFrameDown(encapsulate (mpls_pk_aux_ptr, sta_addr));
    sendFrameDown(encapsulate(mpls_pk_aux_ptr, MACAddress::BROADCAST_ADDRESS));
    /* initialize the mac timer */
    mplsInitializeCheckMac();
}

//
// Crea las estructuras para enviar los paquetes por mpls e inicializa los registros del mac
//
void Ieee80211Mesh::mplsCreateNewPath(int label, LWMPLSPacket *mpls_pk_ptr, MACAddress sta_addr)
{
    int label_out = label;
// Alwais send a ACK
    int label_in;

    LWmpls_Interface_Structure * interface = nullptr;

    LWmpls_Forwarding_Structure * forwarding_ptr = mplsData->lwmpls_forwarding_data(0, label_out, sta_addr.getInt());
    if (forwarding_ptr!=nullptr)
    {
        mplsData->lwmpls_check_label(forwarding_ptr->input_label, "begin");
        mplsData->lwmpls_check_label(forwarding_ptr->return_label_input, "begin");
        forwarding_ptr->last_use = simTime();

        mplsData->lwmpls_init_interface(&interface, forwarding_ptr->input_label, sta_addr.getInt(), LWMPLS_INPUT_LABEL);
// Is the destination?
        if (mpls_pk_ptr->getDest()==myAddress)
        {
            sendUp(decapsulateMpls(mpls_pk_ptr));
            forwarding_ptr->order = LWMPLS_EXTRACT;
            forwarding_ptr->output_label = 0;
            if (MACAddress(forwarding_ptr->input_mac_address) == sta_addr)
                mplsSendAck(forwarding_ptr->input_label);
            else if (MACAddress(forwarding_ptr->mac_address) == sta_addr)
                mplsSendAck(forwarding_ptr->return_label_input);
            return;
        }
        int usedOutLabel;
        int usedIntLabel;
        MACAddress nextMacAddress;
        if (sta_addr == MACAddress(forwarding_ptr->input_mac_address)) // forward path
        {
            usedOutLabel = forwarding_ptr->output_label;
            usedIntLabel = forwarding_ptr->input_label;
            nextMacAddress = MACAddress(forwarding_ptr->mac_address);
        }
        else if (sta_addr == MACAddress(forwarding_ptr->mac_address)) // reverse path
        {
            usedOutLabel = forwarding_ptr->return_label_output;
            usedIntLabel = forwarding_ptr->return_label_input;
            nextMacAddress = MACAddress(forwarding_ptr->input_mac_address);
        }
        else
            throw cRuntimeError("mplsCreateNewPath mac address incorrect");
        label_in = usedIntLabel;

        if (usedOutLabel>0)
        {
            /* path already exist */
            /* change to normal */
            mpls_pk_ptr->setType(WMPLS_NORMAL);
            cPacket * pk = mpls_pk_ptr->decapsulate();
            mpls_pk_ptr->setVectorAddressArraySize(0);
            mpls_pk_ptr->setByteLength(4);
            if (pk)
                mpls_pk_ptr->encapsulate(pk);

            //int dist = mpls_pk_ptr->getDist();
            //mpls_pk_ptr->setDist(0);
            /*op_pk_nfd_set_int32 (mpls_pk_ptr, "label",forwarding_ptr->output_label);*/
            Ieee80211MeshFrame *frame = new Ieee80211MeshFrame(mpls_pk_ptr->getName());
            frame->setTTL(mpls_pk_ptr->getTTL());
            frame->setTimestamp(mpls_pk_ptr->getCreationTime());
            if (usedOutLabel<=0 || usedIntLabel<=0)
                throw cRuntimeError("mplsCreateNewPath Error in label");

            mpls_pk_ptr->setLabel(usedOutLabel);
            frame->setReceiverAddress(nextMacAddress);
            frame->setAddress4(mpls_pk_ptr->getDest());
            frame->setAddress3(mpls_pk_ptr->getSource());

            label_in = usedIntLabel;

            if (mpls_pk_ptr->getControlInfo())
                delete mpls_pk_ptr->removeControlInfo();

            frame->encapsulate(mpls_pk_ptr);
            if (frame->getReceiverAddress().isUnspecified())
                ASSERT(!frame->getReceiverAddress().isUnspecified());
            sendFrameDown(frame);
        }
        else
        {

            if (MACAddress(forwarding_ptr->mac_address).isUnspecified())
            {
                forwarding_ptr->output_label = 0;
                if (mpls_pk_ptr->getType()==WMPLS_BEGIN ||
                        mpls_pk_ptr->getVectorAddressArraySize()==0 )
                    //mpls_pk_ptr->getDist()==0 )
                {
                    std::vector<L3Address> add;
                    add.resize(1);
                    int dist = 0;
                    bool toGateWay = false;
                    if (routingModuleReactive)
                    {
                        if (routingModuleReactive->findInAddressGroup(L3Address(mpls_pk_ptr->getDest())))
                            toGateWay = true;
                    }
                    else if (routingModuleProactive)
                    {
                        if (routingModuleProactive->findInAddressGroup(L3Address(mpls_pk_ptr->getDest())))
                             toGateWay = true;
                    }
                    else if (routingModuleHwmp)
                    {
                        if (routingModuleHwmp->findInAddressGroup(L3Address(mpls_pk_ptr->getDest())))
                             toGateWay = true;
                    }

                    L3Address gateWayAddress;
                    if (routingModuleProactive)
                    {
                        if (toGateWay)
                        {
                            bool isToGw;
                            dist = routingModuleProactive->getRouteGroup(L3Address(mpls_pk_ptr->getDest()), add, gateWayAddress, isToGw);
                        }
                        else
                        {
                            dist = routingModuleProactive->getRoute(L3Address(mpls_pk_ptr->getDest()), add);
                        }
                    }
                    if (dist==0 && routingModuleReactive)
                    {
                        int iface;
                        add.resize(1);
                        double cost;
                        if (toGateWay)
                        {
                            bool isToGw;
                            if (routingModuleReactive->getNextHopGroup(L3Address(mpls_pk_ptr->getDest()), add[0], iface, gateWayAddress, isToGw))
                               dist = 1;
                        }
                        else
                            if (routingModuleReactive->getNextHop(L3Address(mpls_pk_ptr->getDest()),add[0],iface,cost))
                               dist = 1;
                    }

                    if (dist==0 && routingModuleHwmp)
                    {
                        int iface;
                        add.resize(1);
                        double cost;
                        if (toGateWay)
                        {
                            bool isToGw;
                            if (routingModuleHwmp->getNextHopGroup(L3Address(mpls_pk_ptr->getDest()),add[0],iface,gateWayAddress,isToGw))
                               dist = 1;
                        }
                        else
                            if (routingModuleHwmp->getNextHop(L3Address(mpls_pk_ptr->getDest()),add[0],iface,cost))
                               dist = 1;
                    }

                    if (dist==0)
                    {
                        // Destination unreachable
                        if (routingModuleReactive)
                        {
                            ControlManetRouting *ctrlmanet = new ControlManetRouting();
                            ctrlmanet->setOptionCode(MANET_ROUTE_NOROUTE);
                            ctrlmanet->setDestAddress(L3Address(mpls_pk_ptr->getDest()));
                            ctrlmanet->setSrcAddress(L3Address(mpls_pk_ptr->getSource()));
                            send(ctrlmanet, "routingOutReactive");
                        }
                        if (routingModuleHwmp)
                        {
                            ControlManetRouting *ctrlmanet = new ControlManetRouting();
                            ctrlmanet->setOptionCode(MANET_ROUTE_NOROUTE);
                            ctrlmanet->setDestAddress(L3Address(mpls_pk_ptr->getDest()));
                            ctrlmanet->setSrcAddress(L3Address(mpls_pk_ptr->getSource()));
                            send(ctrlmanet, "routingOutHwmp");
                        }
                        mplsData->deleteForwarding(forwarding_ptr);
                        delete mpls_pk_ptr;
                        return;
                    }
                    forwarding_ptr->mac_address = add[0].toMAC().getInt();
                }
                else
                {
                    int position = -1;
                    int arraySize = mpls_pk_ptr->getVectorAddressArraySize();
                    //int arraySize = mpls_pk_ptr->getDist();
                    for (int i=0; i<arraySize; i++)
                        if (mpls_pk_ptr->getVectorAddress(i)==myAddress)
                            position = i;
                    if (position==(arraySize-1))
                        forwarding_ptr->mac_address = mpls_pk_ptr->getDest().getInt();
                    else if (position>=0)
                    {
// Check if neigbourd?
                        forwarding_ptr->mac_address = mpls_pk_ptr->getVectorAddress(position+1).getInt();
                    }
                    else
                    {
// Local route
                        std::vector<L3Address> add;
                        int dist = 0;
                        bool toGateWay = false;
                        L3Address gateWayAddress;
                        if (routingModuleReactive)
                        {
                            if (routingModuleReactive->findInAddressGroup(L3Address(mpls_pk_ptr->getDest())))
                                toGateWay = true;
                        }
                        else if (routingModuleProactive)
                        {
                            if (routingModuleProactive->findInAddressGroup(L3Address(mpls_pk_ptr->getDest())))
                                 toGateWay = true;
                        }
                        else if (routingModuleHwmp)
                        {
                            if (routingModuleHwmp->findInAddressGroup(L3Address(mpls_pk_ptr->getDest())))
                                 toGateWay = true;
                        }
                        if (routingModuleProactive)
                        {
                            if (toGateWay)
                            {
                                bool isToGw;
                                dist = routingModuleProactive->getRouteGroup(L3Address(mpls_pk_ptr->getDest()), add, gateWayAddress, isToGw);
                            }
                            else
                            {
                                dist = routingModuleProactive->getRoute(L3Address(mpls_pk_ptr->getDest()), add);
                            }
                        }
                        if (dist==0 && routingModuleReactive)
                        {
                            int iface;
                            add.resize(1);
                            double cost;
                            if (toGateWay)
                            {

                                bool isToGw;
                                if (routingModuleReactive->getNextHopGroup(L3Address(mpls_pk_ptr->getDest()), add[0], iface, gateWayAddress, isToGw))
                                    dist = 1;
                            }
                            else
                                if (routingModuleReactive->getNextHop(L3Address(mpls_pk_ptr->getDest()), add[0], iface, cost))
                                     dist = 1;
                        }

                        if (dist==0 && routingModuleHwmp)
                        {
                            int iface;
                            add.resize(1);
                            double cost;
                            if (toGateWay)
                            {
                                bool isToGw;
                                if (routingModuleHwmp->getNextHopGroup(L3Address(mpls_pk_ptr->getDest()), add[0], iface, gateWayAddress, isToGw))
                                   dist = 1;
                            }
                            else
                                if (routingModuleHwmp->getNextHop(L3Address(mpls_pk_ptr->getDest()), add[0], iface, cost))
                                    dist = 1;
                        }
                        if (dist==0)
                        {
                            // Destination unreachable
                            if (routingModuleReactive)
                            {
                                ControlManetRouting *ctrlmanet = new ControlManetRouting();
                                ctrlmanet->setOptionCode(MANET_ROUTE_NOROUTE);
                                ctrlmanet->setDestAddress(L3Address(mpls_pk_ptr->getDest()));
                                ctrlmanet->setSrcAddress(L3Address(mpls_pk_ptr->getSource()));
                                send(ctrlmanet, "routingOutReactive");
                            }
                            mplsData->deleteForwarding(forwarding_ptr);
                            delete mpls_pk_ptr;
                            return;
                        }
                        forwarding_ptr->mac_address = add[0].toMAC().getInt();
                        mpls_pk_ptr->setVectorAddressArraySize(0);
                        //mpls_pk_ptr->setDist(0);
                    }
                }
            }

            if (forwarding_ptr->return_label_input<=0)
                throw cRuntimeError("Error in label");

            mpls_pk_ptr->setLabel(forwarding_ptr->return_label_input);
            mpls_pk_ptr->setLabelReturn(0);
            Ieee80211MeshFrame *frame = new Ieee80211MeshFrame(mpls_pk_ptr->getName());
            frame->setTTL(mpls_pk_ptr->getTTL());
            frame->setTimestamp(mpls_pk_ptr->getCreationTime());
            frame->setReceiverAddress(MACAddress(forwarding_ptr->mac_address));
            frame->setAddress4(mpls_pk_ptr->getDest());
            frame->setAddress3(mpls_pk_ptr->getSource());

            if (mpls_pk_ptr->getControlInfo())
                delete mpls_pk_ptr->removeControlInfo();


            frame->encapsulate(mpls_pk_ptr);
            if (frame->getReceiverAddress().isUnspecified())
                ASSERT(!frame->getReceiverAddress().isUnspecified());
            sendFrameDown(frame);
        }
    }
    else
    {
// New structure
        /* Obtain a label */
        label_in = mplsData->getLWMPLSLabel();
        mplsData->lwmpls_init_interface(&interface, label_in, sta_addr.getInt(), LWMPLS_INPUT_LABEL);
        /* es necesario introducir el nuevo path en la lista de enlace */
        //lwmpls_initialize_interface(lwmpls_data_ptr,&interface_str_ptr,label_in,sta_addr, ip_address,LWMPLS_INPUT_LABEL);
        /* es necesario ahora introducir los datos en la tabla */
        forwarding_ptr = new LWmpls_Forwarding_Structure();
        forwarding_ptr->output_label = 0;
        forwarding_ptr->input_label = label_in;
        forwarding_ptr->return_label_input = 0;
        forwarding_ptr->return_label_output = label_out;
        forwarding_ptr->order = LWMPLS_EXTRACT;
        forwarding_ptr->input_mac_address = sta_addr.getInt();
        forwarding_ptr->label_life_limit = mplsData->mplsMaxTime();
        forwarding_ptr->last_use = simTime();

        forwarding_ptr->path.push_back(mpls_pk_ptr->getSource().getInt());
        for (unsigned int i=0; i<mpls_pk_ptr->getVectorAddressArraySize(); i++)
            //for (int i=0 ;i<mpls_pk_ptr->getDist();i++)
            forwarding_ptr->path.push_back(mpls_pk_ptr->getVectorAddress(i).getInt());
        forwarding_ptr->path.push_back(mpls_pk_ptr->getDest().getInt());

        // Add structure
        mplsData->lwmpls_forwarding_input_data_add(label_in, forwarding_ptr);
        if (!mplsData->lwmpls_forwarding_output_data_add(label_out, sta_addr.getInt(), forwarding_ptr, true))
        {
            mplsBasicSend(mpls_pk_ptr, sta_addr);
            return;
        }

        if (mpls_pk_ptr->getDest()==myAddress)
        {
            mplsSendAck(label_in);
            mplsData->registerRoute(mpls_pk_ptr->getSource().getInt(), label_in);
            sendUp(decapsulateMpls(mpls_pk_ptr));
            // Register route
            return;
        }

        if (mpls_pk_ptr->getType()==WMPLS_BEGIN ||
                mpls_pk_ptr->getVectorAddressArraySize()==0 )
            //mpls_pk_ptr->getDist()==0 )
        {
            std::vector<L3Address> add;
            int dist = 0;
            if (routingModuleProactive)
            {
                dist = routingModuleProactive->getRoute(L3Address(mpls_pk_ptr->getDest()), add);
            }
            if (dist==0 && routingModuleReactive)
            {
                int iface;
                add.resize(1);
                double cost;
                if (routingModuleReactive->getNextHop(L3Address(mpls_pk_ptr->getDest()), add[0], iface, cost))
                    dist = 1;
            }

            if (dist==0 && routingModuleHwmp)
            {
                int iface;
                add.resize(1);
                double cost;
                if (routingModuleHwmp->getNextHop(L3Address(mpls_pk_ptr->getDest()), add[0], iface, cost))
                    dist = 1;
            }
            if (dist==0)
            {
                // Destination unreachable
                if (routingModuleReactive)
                {
                    ControlManetRouting *ctrlmanet = new ControlManetRouting();
                    ctrlmanet->setOptionCode(MANET_ROUTE_NOROUTE);
                    ctrlmanet->setDestAddress(L3Address(mpls_pk_ptr->getDest()));
                    ctrlmanet->setSrcAddress(L3Address(mpls_pk_ptr->getSource()));
                    send(ctrlmanet, "routingOutReactive");
                }

                mplsData->deleteForwarding(forwarding_ptr);
                delete mpls_pk_ptr;
                return;
            }
            forwarding_ptr->mac_address = add[0].toMAC().getInt();
        }
        else
        {
            int position = -1;
            int arraySize = mpls_pk_ptr->getVectorAddressArraySize();
            //int arraySize = mpls_pk_ptr->getDist();
            for (int i=0; i<arraySize; i++)
                if (mpls_pk_ptr->getVectorAddress(i)==myAddress)
                {
                    position = i;
                    break;
                }
            if (position==(arraySize-1) && (position>=0))
                forwarding_ptr->mac_address = mpls_pk_ptr->getDest().getInt();
            else if (position>=0)
            {
// Check if neigbourd?
                forwarding_ptr->mac_address = mpls_pk_ptr->getVectorAddress(position+1).getInt();
            }
            else
            {
// Local route o discard?
                // delete mpls_pk_ptr
                // return;
                std::vector<L3Address> add;
                int dist = 0;
                if (routingModuleProactive)
                {
                    dist = routingModuleProactive->getRoute(L3Address(mpls_pk_ptr->getDest()), add);
                }
                if (dist==0 && routingModuleReactive)
                {
                    int iface;
                    add.resize(1);
                    double cost;

                    if (routingModuleReactive->getNextHop(L3Address(mpls_pk_ptr->getDest()), add[0], iface, cost))
                        dist = 1;
                }
                if (dist==0 && routingModuleHwmp)
                {
                    int iface;
                    add.resize(1);
                    double cost;

                    if (routingModuleHwmp->getNextHop(L3Address(mpls_pk_ptr->getDest()), add[0], iface, cost))
                        dist = 1;
                }
                if (dist==0)
                {
                    // Destination unreachable
                    if (routingModuleReactive)
                    {
                        ControlManetRouting *ctrlmanet = new ControlManetRouting();
                        ctrlmanet->setOptionCode(MANET_ROUTE_NOROUTE);
                        ctrlmanet->setDestAddress(L3Address(mpls_pk_ptr->getDest()));
                        ctrlmanet->setSrcAddress(L3Address(mpls_pk_ptr->getSource()));
                        send(ctrlmanet, "routingOutReactive");
                    }
                    mplsData->deleteForwarding(forwarding_ptr);
                    delete mpls_pk_ptr;
                    return;
                }
                forwarding_ptr->mac_address = add[0].toMAC().getInt();
                mpls_pk_ptr->setVectorAddressArraySize(0);
                //mpls_pk_ptr->setDist(0);
            }
        }

// Send to next node
        Ieee80211MeshFrame *frame = new Ieee80211MeshFrame(mpls_pk_ptr->getName());
        frame->setTTL(mpls_pk_ptr->getTTL());
        frame->setTimestamp(mpls_pk_ptr->getCreationTime());
        frame->setReceiverAddress(MACAddress(forwarding_ptr->mac_address));
        frame->setAddress4(mpls_pk_ptr->getDest());
        frame->setAddress3(mpls_pk_ptr->getSource());

// The reverse path label
        forwarding_ptr->return_label_input = mplsData->getLWMPLSLabel();
// Initialize the next interface
        interface = nullptr;
        mplsData->lwmpls_init_interface(&interface, forwarding_ptr->return_label_input, forwarding_ptr->mac_address, LWMPLS_INPUT_LABEL_RETURN);
// Store the reverse path label
        mplsData->lwmpls_forwarding_input_data_add(forwarding_ptr->return_label_input, forwarding_ptr);

        mpls_pk_ptr->setLabel(forwarding_ptr->return_label_input);
        mpls_pk_ptr->setLabelReturn(0);

        if (mpls_pk_ptr->getControlInfo())
            delete mpls_pk_ptr->removeControlInfo();

        frame->encapsulate(mpls_pk_ptr);
        if (frame->getReceiverAddress().isUnspecified())
            ASSERT(!frame->getReceiverAddress().isUnspecified());
        sendFrameDown(frame);
    }
    mplsSendAck(label_in);
}

void Ieee80211Mesh::mplsBasicSend(LWMPLSPacket *mpls_pk_ptr, MACAddress sta_addr)
{
    if (mpls_pk_ptr->getDest() == myAddress)
    {
        sendUp(decapsulateMpls(mpls_pk_ptr));
    }
    else
    {
        std::vector<L3Address> add;
        int dist = 0;

        if (routingModuleProactive)
        {
            dist = routingModuleProactive->getRoute(L3Address(mpls_pk_ptr->getDest()), add);
        }

        if (dist==0 && routingModuleReactive)
        {
            int iface;
            add.resize(1);
            double cost;
            if (routingModuleReactive->getNextHop(L3Address(mpls_pk_ptr->getDest()), add[0], iface, cost))
                dist = 1;
        }

        if (dist==0 && routingModuleHwmp)
        {
            int iface;
            add.resize(1);
            double cost;
            if (routingModuleHwmp->getNextHop(L3Address(mpls_pk_ptr->getDest()), add[0], iface, cost))
                dist = 1;
        }
        if (dist==0)
        {
            // Destination unreachable
            if (routingModuleReactive)
            {
                ControlManetRouting *ctrlmanet = new ControlManetRouting();
                ctrlmanet->setOptionCode(MANET_ROUTE_NOROUTE);
                ctrlmanet->setDestAddress(L3Address(mpls_pk_ptr->getDest()));
                ctrlmanet->setSrcAddress(L3Address(mpls_pk_ptr->getSource()));
                send(ctrlmanet, "routingOutReactive");
            }
            delete mpls_pk_ptr;
            return;
        }

        mpls_pk_ptr->setType(WMPLS_SEND);
        cPacket * pk = mpls_pk_ptr->decapsulate();
        mpls_pk_ptr->setVectorAddressArraySize(0);
        //mpls_pk_ptr->setByteLength(0);
        if (pk)
            mpls_pk_ptr->encapsulate(pk);
        Ieee80211MeshFrame *frame = new Ieee80211MeshFrame(mpls_pk_ptr->getName());
        frame->setTimestamp(mpls_pk_ptr->getCreationTime());
        frame->setAddress4(mpls_pk_ptr->getDest());
        frame->setAddress3(mpls_pk_ptr->getSource());
        frame->setTTL(mpls_pk_ptr->getTTL());


        if (dist>1)
            frame->setReceiverAddress(add[0].toMAC());
        else
            frame->setReceiverAddress(mpls_pk_ptr->getDest());

        if (mpls_pk_ptr->getControlInfo())
            delete mpls_pk_ptr->removeControlInfo();
        frame->encapsulate(mpls_pk_ptr);
        if (frame->getReceiverAddress().isUnspecified())
            ASSERT(!frame->getReceiverAddress().isUnspecified());
        sendFrameDown(frame);
    }
}

void Ieee80211Mesh::mplsBreakPath(int label, LWMPLSPacket *mpls_pk_ptr, MACAddress sta_addr)
{
    // printf("break %f\n",time);
    // printf("code %i my_address %d org %d lin %d \n",code,my_address,sta_addr,label);
    // liberar todos los path, dos pasos quien detecta la ruptura y quien la propaga.
    // Es mecesario tambien liberar los caminos de retorno.
    /*  forwarding_ptr= lwmpls_forwarding_data(lwmpls_data_ptr,0,label,sta_addr);*/
    MACAddress send_mac_addr;
    LWmpls_Forwarding_Structure * forwarding_ptr = mplsData->lwmpls_interface_delete_label(label);
    if (forwarding_ptr == nullptr)
    {
        delete mpls_pk_ptr;
        return;
    }

    if (label == forwarding_ptr->input_label)
    {
        mpls_pk_ptr->setLabel(forwarding_ptr->output_label);
        send_mac_addr = MACAddress(forwarding_ptr->mac_address);
    }
    else
    {
        mpls_pk_ptr->setLabel(forwarding_ptr->return_label_output);
        send_mac_addr = MACAddress(forwarding_ptr->input_mac_address);
    }

    mplsPurge(forwarding_ptr, true);
    // Must clean the routing tables?

    if ((forwarding_ptr->order==LWMPLS_CHANGE) && (!send_mac_addr.isUnspecified()))
    {
        Ieee80211MeshFrame *frame = new Ieee80211MeshFrame(mpls_pk_ptr->getName());
        frame->setTimestamp(mpls_pk_ptr->getCreationTime());
        frame->setReceiverAddress(send_mac_addr);
        frame->setAddress4(mpls_pk_ptr->getDest());
        frame->setAddress3(mpls_pk_ptr->getSource());
        frame->setTTL(mpls_pk_ptr->getTTL());
        if (mpls_pk_ptr->getControlInfo())
            delete mpls_pk_ptr->removeControlInfo();
        frame->encapsulate(mpls_pk_ptr);
        if (frame->getReceiverAddress().isUnspecified())
            ASSERT(!frame->getReceiverAddress().isUnspecified());
        sendFrameDown(frame);
    }
    else
    {
        // Firts or last node
        delete mpls_pk_ptr;
        //mplsData->deleteRegisterLabel(forwarding_ptr->input_label);
        //mplsData->deleteRegisterLabel(forwarding_ptr->return_label_input);
    }

    mplsData->deleteForwarding(forwarding_ptr);
}


void Ieee80211Mesh::mplsNotFoundPath(int label, LWMPLSPacket *mpls_pk_ptr, MACAddress sta_addr)
{
    LWmpls_Forwarding_Structure * forwarding_ptr = mplsData->lwmpls_forwarding_data(0, label, sta_addr.getInt());
    MACAddress send_mac_addr;
    if (forwarding_ptr == nullptr)
        delete mpls_pk_ptr;
    else
    {
        mplsData->lwmpls_interface_delete_label(forwarding_ptr->input_label);
        mplsData->lwmpls_interface_delete_label(forwarding_ptr->return_label_input);
        if (label == forwarding_ptr->output_label)
        {
            mpls_pk_ptr->setLabel(forwarding_ptr->input_label);
            send_mac_addr = MACAddress(forwarding_ptr->input_mac_address);
        }
        else
        {
            mpls_pk_ptr->setLabel(forwarding_ptr->return_label_input);
            send_mac_addr = MACAddress(forwarding_ptr->mac_address);
        }
        mplsPurge(forwarding_ptr, false);

        if ((forwarding_ptr->order==LWMPLS_CHANGE) && (!send_mac_addr.isUnspecified()))
        {
            Ieee80211MeshFrame *frame = new Ieee80211MeshFrame(mpls_pk_ptr->getName());
            frame->setTimestamp(mpls_pk_ptr->getCreationTime());
            frame->setReceiverAddress(send_mac_addr);
            frame->setAddress4(mpls_pk_ptr->getDest());
            frame->setAddress3(mpls_pk_ptr->getSource());
            frame->setTTL(mpls_pk_ptr->getTTL());
            if (mpls_pk_ptr->getControlInfo())
                delete mpls_pk_ptr->removeControlInfo();
            frame->encapsulate(mpls_pk_ptr);
            if (frame->getReceiverAddress().isUnspecified())
                ASSERT(!frame->getReceiverAddress().isUnspecified());
            sendFrameDown(frame);
        }
        else
        {
            delete mpls_pk_ptr;
            //deleteRegisterLabel(forwarding_ptr->input_label);
            //deleteRegisterLabel(forwarding_ptr->return_label_input);
        }
        /* */
        mplsData->deleteForwarding(forwarding_ptr);
    }
}

void Ieee80211Mesh::mplsForwardData(int label, LWMPLSPacket *mpls_pk_ptr, MACAddress sta_addr, LWmpls_Forwarding_Structure *forwarding_data)
{
    /* Extraer la etiqueta y la direcci�n de enlace del siguiente salto */
    LWmpls_Forwarding_Structure * forwarding_ptr = forwarding_data;
    if (forwarding_ptr==nullptr)
        forwarding_ptr = mplsData->lwmpls_forwarding_data(label, 0, 0);
    forwarding_ptr->last_use = simTime();
    bool is_source = false;
    int output_label, input_label_aux;
    MACAddress send_mac_addr;

    if (forwarding_ptr->order==LWMPLS_CHANGE || is_source)
    {
        if ((label == forwarding_ptr->input_label) || is_source)
        {
            output_label = forwarding_ptr->output_label;
            input_label_aux = forwarding_ptr->return_label_input;
            send_mac_addr = MACAddress(forwarding_ptr->mac_address);
        }
        else
        {
            output_label = forwarding_ptr->return_label_output;
            input_label_aux = forwarding_ptr->input_label;
            send_mac_addr = MACAddress(forwarding_ptr->input_mac_address);
        }
        if (output_label > 0)
        {
            mpls_pk_ptr->setLabel(output_label);
        }
        else
        {
            mpls_pk_ptr->setType(WMPLS_BEGIN);
            mpls_pk_ptr->setLabel(input_label_aux);
        }
        // Enviar al mac
        // polling = wlan_poll_list_member_find (send_mac_addr);
        // wlan_hlpk_enqueue (mpls_pk_ptr, send_mac_addr, polling,false);

        sendFrameDown(encapsulate(mpls_pk_ptr, send_mac_addr));
        return;
    }
    else if (forwarding_ptr->order==LWMPLS_EXTRACT)
    {

        if (mpls_pk_ptr->getDest()==myAddress)
        {
            sendUp(decapsulateMpls(mpls_pk_ptr));
            return;
        }
        mplsBasicSend(mpls_pk_ptr, sta_addr);
        return;

        if (!(dynamic_cast<LWMPLSPacket*> (mpls_pk_ptr->getEncapsulatedPacket())))
        {
// Source or destination?

            if (sta_addr != MACAddress(forwarding_ptr->input_mac_address) || forwarding_ptr->mac_address == 0)
            {
                mplsBasicSend(mpls_pk_ptr, sta_addr);
                return;
            }

            output_label = forwarding_ptr->output_label;
            send_mac_addr = MACAddress(forwarding_ptr->mac_address);

            if (output_label>0)
            {
                forwarding_ptr->order = LWMPLS_CHANGE;
                mpls_pk_ptr->setLabel(output_label);
                sendFrameDown(encapsulate(mpls_pk_ptr, send_mac_addr));
            }
            else
            {
                mpls_pk_ptr->setLabel(forwarding_ptr->return_label_input);
                if (forwarding_ptr->path.size()>0)
                {
                    mpls_pk_ptr->setType(WMPLS_BEGIN_W_ROUTE);
                    int dist = forwarding_ptr->path.size()-2;
                    mpls_pk_ptr->setVectorAddressArraySize(dist);
                    for (int i = 0; i<dist; i++)
                        mpls_pk_ptr->setVectorAddress(i, MACAddress(forwarding_ptr->path[i+1]));
                }
                else
                    mpls_pk_ptr->setType(WMPLS_BEGIN);

                sendFrameDown(encapsulate(mpls_pk_ptr, send_mac_addr));
            }
        }
        else
        {
            if (dynamic_cast<LWMPLSPacket*>(mpls_pk_ptr->getEncapsulatedPacket()))
            {
                LWMPLSPacket *seg_pkptr =  dynamic_cast<LWMPLSPacket*>(mpls_pk_ptr->decapsulate());
                seg_pkptr->setTTL(mpls_pk_ptr->getTTL());
                delete mpls_pk_ptr;
                mplsDataProcess((LWMPLSPacket*)seg_pkptr, sta_addr);
            }
            else
                delete mpls_pk_ptr;
        }
        // printf("To application %d normal %f \n",time);
    }
}

void Ieee80211Mesh::mplsAckPath(int label, LWMPLSPacket *mpls_pk_ptr, MACAddress sta_addr)
{
    //   printf("ack %f\n",time);
    int label_out = mpls_pk_ptr->getLabelReturn();

    /* es necesario ahora introducir los datos en la tabla */
    LWmpls_Forwarding_Structure * forwarding_ptr = mplsData->lwmpls_forwarding_data(label, 0, 0);

// Intermediate node


    int *labelOutPtr;
    int *labelInPtr;


    if (MACAddress(forwarding_ptr->mac_address)==sta_addr)
    {
        labelOutPtr = &forwarding_ptr->output_label;
        labelInPtr = &forwarding_ptr->return_label_input;
    }
    else if (MACAddress(forwarding_ptr->input_mac_address)==sta_addr)
    {
        labelOutPtr = &forwarding_ptr->return_label_output;
        labelInPtr = &forwarding_ptr->input_label;
    }
    else
    {
        delete mpls_pk_ptr;
        return;
    }

    if (*labelOutPtr==0)
    {
        *labelOutPtr = label_out;
        mplsData->lwmpls_forwarding_output_data_add(label_out, sta_addr.getInt(), forwarding_ptr, false);
    }
    else
    {
        if (*labelOutPtr!=label_out)
        {
            /* change of label */
            // prg_string_hash_table_item_remove (lwmpls_data_ptr->forwarding_table_output,forwarding_ptr->key_output);
            *labelOutPtr = label_out;
            mplsData->lwmpls_forwarding_output_data_add(label_out, sta_addr.getInt(), forwarding_ptr, false);
        }
    }

    forwarding_ptr->last_use = simTime();
    /* initialize the mac timer */
// init the
    LWmpls_Interface_Structure *interface = nullptr;
    mplsData->lwmpls_init_interface(&interface, *labelInPtr, sta_addr.getInt(), LWMPLS_INPUT_LABEL_RETURN);
    mplsInitializeCheckMac();

    if (forwarding_ptr->return_label_output>0 && forwarding_ptr->output_label>0)
        forwarding_ptr->order = LWMPLS_CHANGE;

    delete mpls_pk_ptr;
}

void Ieee80211Mesh::mplsDataProcess(LWMPLSPacket * mpls_pk_ptr, MACAddress sta_addr)
{
    int label;
    LWmpls_Forwarding_Structure *forwarding_ptr = nullptr;
    bool         label_found;
    int code;
    simtime_t     time;

    /* First check for the case where the received segment contains the */
    /* entire data packet, i.e. the data is transmitted as a single     */
    /* fragment.*/
    time = simTime();
    code = mpls_pk_ptr->getType();
    label = mpls_pk_ptr->getLabel();
    bool is_source = false;
    label_found = true;

    if (code==WMPLS_ACK && mpls_pk_ptr->getDest()!=myAddress)
    {
        delete mpls_pk_ptr;
        return;
    }
    // printf("code %i my_address %d org %d lin %d %f \n",code,my_address,sta_addr,label,op_sim_time());
    bool testMplsData = (code!=WMPLS_BEGIN) && (code!=WMPLS_NOTFOUND) &&
                        (code != WMPLS_BEGIN_W_ROUTE) && (code!=WMPLS_SEND) &&
                        (code!=WMPLS_BROADCAST) && (code!=WMPLS_ANNOUNCE_GATEWAY) && (code!=WMPLS_REQUEST_GATEWAY); // broadcast code

    if (testMplsData)
    {
        if ((code == WMPLS_REFRES) && (label==0))
        {
            /* In this case the refresh message is used for refresh the mac connections */
            delete mpls_pk_ptr;
            // printf("refresh %f\n",time);
            // printf("fin 1 %i \n",code);
            return;
        }
        if (label>0)
        {
            if ((forwarding_ptr = mplsData->lwmpls_forwarding_data(label, 0, 0))!=nullptr)
            {
                if  (code == WMPLS_NORMAL)
                {
                    if (!is_source)
                    {
                        if (forwarding_ptr->input_label == label && forwarding_ptr->input_mac_address!=sta_addr.getInt())
                            forwarding_ptr = nullptr;
                        else if (forwarding_ptr->return_label_input == label && forwarding_ptr->mac_address!=sta_addr.getInt())
                            forwarding_ptr = nullptr;
                    }
                }
            }
            //printf (" %p \n",forwarding_ptr);
            if (forwarding_ptr == nullptr)
                label_found = false;
        }

        if (!label_found)
        {
            if (code == WMPLS_NORMAL)
                mplsBasicSend((LWMPLSPacket*)mpls_pk_ptr->dup(), sta_addr);
            if (code != WMPLS_ACK)
                delete mpls_pk_ptr->decapsulate();

            // � es necesario destruir label_msg_ptr? mirar la memoria
            //op_pk_nfd_set_ptr (seg_pkptr, "pointer", label_msg_ptr);
            mpls_pk_ptr->setType(WMPLS_NOTFOUND);
            // Enviar el mensaje al mac
            //polling = wlan_poll_list_member_find (sta_addr);
            // wlan_hlpk_enqueue (mpls_pk_ptr, sta_addr, polling,true);
            sendFrameDown(encapsulate(mpls_pk_ptr, sta_addr));
            return;
        }
    }

    switch (code)
    {

    case WMPLS_NORMAL:
        mplsForwardData(label, mpls_pk_ptr, sta_addr, forwarding_ptr);
        break;

    case WMPLS_BEGIN:
    case WMPLS_BEGIN_W_ROUTE:
        mplsCreateNewPath(label, mpls_pk_ptr, sta_addr);
        break;

    case WMPLS_REFRES:
        // printf("refresh %f\n",time);
        forwarding_ptr->last_use = simTime();
        if (forwarding_ptr->order==LWMPLS_CHANGE)
        {
            if (!(MACAddress(forwarding_ptr->mac_address).isUnspecified()))
            {
                mpls_pk_ptr->setLabel(forwarding_ptr->output_label);
                sendFrameDown(encapsulate(mpls_pk_ptr, MACAddress(forwarding_ptr->mac_address)));
            }
            else
                delete mpls_pk_ptr;
        }
        else if (forwarding_ptr->order==LWMPLS_EXTRACT)
        {
            delete mpls_pk_ptr;
        }
        break;

    case WMPLS_END:
        break;

    case WMPLS_BREAK:
        mplsBreakPath(label, mpls_pk_ptr, sta_addr);
        break;

    case WMPLS_NOTFOUND:
        mplsNotFoundPath(label, mpls_pk_ptr, sta_addr);
        break;

    case WMPLS_ACK:
        mplsAckPath(label, mpls_pk_ptr, sta_addr);
        break;
    case WMPLS_SEND:
        mplsBasicSend(mpls_pk_ptr, sta_addr);
        break;
    case WMPLS_ADITIONAL:
        break;
    case WMPLS_BROADCAST:
    case WMPLS_ANNOUNCE_GATEWAY:
    case WMPLS_REQUEST_GATEWAY:
        uint32_t newCounter = mpls_pk_ptr->getCounter();

        if (floodingConfirmation && code == WMPLS_BROADCAST)
        {
            for (unsigned int i = 0; i < confirmationFrames.size(); i++)
            {
                LWMPLSPacket * mplsPkAux = dynamic_cast<LWMPLSPacket *>(confirmationFrames[i].frame->getEncapsulatedPacket());
                if (mplsPkAux)
                {
                    if (mpls_pk_ptr->getSource() == mplsPkAux->getSource() && mpls_pk_ptr->getCounter() == mplsPkAux->getCounter())
                    {
                        // cancel and delete
                        cancelAndDelete(confirmationFrames[i].frame);
                        confirmationFrames.erase(confirmationFrames.begin()+i);
                        break;
                    }
                }
            }
        }

        if (mpls_pk_ptr->getSource() == myAddress)
        {
            // los paquetes propios deben ser borrados
            delete mpls_pk_ptr;
            return;
        }
        if (mplsIsBroadcastProcessed(mpls_pk_ptr->getSource(), newCounter))
        {
            delete mpls_pk_ptr;
            return;
        }

        // send up and Resend
        if (code==WMPLS_BROADCAST)
        {
            // check if the node must propagate broadcast
            if (hasRelayUnit)
            {
                cPacket *msg = mpls_pk_ptr->getEncapsulatedPacket()->dup();

                EthernetIIFrame *ethframe = new EthernetIIFrame(msg->getName()); //TODO option to use EtherFrameWithSNAP instead
                ethframe->setDest(mpls_pk_ptr->getDest());
                ethframe->setSrc(mpls_pk_ptr->getSource());
                ethframe->setEtherType(0);
                ethframe->encapsulate(msg);
                if (ethframe->getByteLength() < MIN_ETHERNET_FRAME_BYTES)
                    ethframe->setByteLength(MIN_ETHERNET_FRAME_BYTES);
                sendUp(ethframe);
            }
            else
            {
                cPacket *pkt = mpls_pk_ptr->getEncapsulatedPacket()->dup();
                pkt->setControlInfo(mpls_pk_ptr->removeControlInfo());
                sendUp(pkt);
            }
            if (!mplsForwardBroadcast(sta_addr)) // no propagate
            {
                delete mpls_pk_ptr;
                return;
            }
        }
        else
            processControlPacket(dynamic_cast<LWMPLSControl*>(mpls_pk_ptr));
//        sendFrameDown(encapsulate(mpls_pk_ptr,MACAddress::BROADCAST_ADDRESS));
//       small random delay. Avoid the collision
        Ieee80211DataFrame *meshFrame = encapsulate(mpls_pk_ptr, MACAddress::BROADCAST_ADDRESS);
        if (floodingConfirmation)
        {
            ConfirmationInfo cinfo;
            cinfo.frame = dynamic_cast<Ieee80211MeshFrame*>(meshFrame)->dup();
            cinfo.reintent = 0;
            confirmationFrames.push_back(cinfo);
            scheduleAt(simTime()+1,cinfo.frame);
        }
        scheduleAt(simTime()+par("MacBroadcastDelay"), meshFrame);
        break;
    }
}


/* clean the path and create the message WMPLS_BREAK and send */
void Ieee80211Mesh::mplsBreakMacLink(MACAddress macAddress)
{
    LWmpls_Forwarding_Structure *forwarding_ptr;

    uint64_t des_add;
    int out_label;
    uint64_t mac_id;
    mac_id = macAddress.getInt();


    LWmpls_Interface_Structure * mac_ptr = mplsData->lwmpls_interface_structure(mac_id);
    if (!mac_ptr)
        return;

// Test para evitar falsos positivos por colisiones
    if ((simTime()-mac_ptr->lastUse())<mplsData->mplsMacLimit())
        return;

    int numRtr = mac_ptr->numRtr();

    if (numRtr<mplsData->mplsMaxMacRetry())
    {
        mac_ptr->numRtr() = numRtr+1;
        return;
    }

    auto it = mplsData->interfaceMap->find(mac_id);
    if (it != mplsData->interfaceMap->end())
        if (!it->second->numLabels())
        {
            delete it->second;
            mplsData->interfaceMap->erase(it);
        }

    for (unsigned int i = 1; i < mplsData->label_list.size(); i++)
    {
        forwarding_ptr = mplsData->lwmpls_forwarding_data(i, 0, 0);
        if (forwarding_ptr!=nullptr)
        {
            if ((forwarding_ptr->mac_address == mac_id) || (forwarding_ptr->input_mac_address == mac_id))
            {
                mplsPurge(forwarding_ptr, true);
                /* prepare and send break message */
                if (forwarding_ptr->input_mac_address == mac_id)
                {
                    des_add = forwarding_ptr->mac_address;
                    out_label = forwarding_ptr->output_label;
                }
                else
                {
                    des_add = forwarding_ptr->input_mac_address;
                    out_label = forwarding_ptr->return_label_output;
                }
                if (des_add!=0)
                {
                    LWMPLSPacket *lwmplspk = new LWMPLSPacket;
                    lwmplspk->setType(WMPLS_BREAK);
                    lwmplspk->setLabel(out_label);
                    sendFrameDown(encapsulate(lwmplspk, MACAddress(des_add)));
                }
                mplsData->deleteForwarding(forwarding_ptr);
                forwarding_ptr = nullptr;
            }
        }
    }
}


void Ieee80211Mesh::mplsCheckRouteTime()
{
    simtime_t actual_time;
    bool active = false;
    LWmpls_Forwarding_Structure *forwarding_ptr;
    int out_label;
    uint64_t mac_id;
    uint64_t des_add;

    actual_time = simTime();

    for (auto it=mplsData->interfaceMap->begin(); it != mplsData->interfaceMap->end();)
    {
        if ((actual_time - it->second->lastUse()) < (multipler_active_break*timer_active_refresh))
        {
            it++;
            continue;
        }

        mac_id = it->second->macAddress();
        delete it->second;
        mplsData->interfaceMap->erase(it);
        it = mplsData->interfaceMap->begin();
        if (mac_id==0)
            continue;

        for (unsigned int i = 1; i < mplsData->label_list.size(); i++)
        {
            forwarding_ptr = mplsData->lwmpls_forwarding_data(i, 0, 0);
            if (forwarding_ptr && (mac_id == forwarding_ptr->mac_address || mac_id == forwarding_ptr->input_mac_address))
            {
                mplsPurge(forwarding_ptr, true);
                /* prepare and send break message */
                if (forwarding_ptr->input_mac_address == mac_id)
                {
                    des_add = forwarding_ptr->mac_address;
                    out_label = forwarding_ptr->output_label;
                }
                else
                {
                    des_add = forwarding_ptr->input_mac_address;
                    out_label = forwarding_ptr->return_label_output;
                }
                if (des_add!=0)
                {
                    LWMPLSPacket *lwmplspk = new LWMPLSPacket;
                    lwmplspk->setType(WMPLS_BREAK);
                    lwmplspk->setLabel(out_label);
                    sendFrameDown(encapsulate(lwmplspk, MACAddress(des_add)));
                }
                mplsData->deleteForwarding(forwarding_ptr);
            }
        }


    }

    if (mplsData->lwmpls_nun_labels_in_use()>0)
        active = true;

    if (activeMacBreak &&  active && WMPLSCHECKMAC)
    {
        if (!WMPLSCHECKMAC->isScheduled())
            scheduleAt(actual_time+(multipler_active_break*timer_active_refresh), WMPLSCHECKMAC);
    }
}


void Ieee80211Mesh::mplsInitializeCheckMac()
{
    int list_size;
    bool active = false;

    if (WMPLSCHECKMAC==nullptr)
       return;
    if (activeMacBreak == false)
        return;

    list_size = mplsData->lwmpls_nun_labels_in_use();

    if (list_size>0)
        active = true;

    if (active == true)
    {
        if (!WMPLSCHECKMAC->isScheduled())
            scheduleAt(simTime()+(multipler_active_break*timer_active_refresh), WMPLSCHECKMAC);
    }
    return;
}


void Ieee80211Mesh::mplsPurge(LWmpls_Forwarding_Structure *forwarding_ptr, bool purge_break)
{
// �Como? las colas estan en otra parte.

    if (forwarding_ptr==nullptr)
        return;

    return;
}

bool Ieee80211Mesh::mplsIsBroadcastProcessed(const MACAddress &src, const uint32 &newCounter)
{
    uint32_t cont;
    if (mplsData->getBroadCastCounter(src.getInt(), cont))
    {
        if (newCounter == cont)
            return true;
        else if (newCounter < cont) //
        {
            if (!(cont > UINT32_MAX - 100 && newCounter < 100)) // Dado la vuelta
                return true;
        }
    }
    mplsData->setBroadCastCounter(src.getInt(), newCounter);
    return false;
}

bool Ieee80211Mesh::mplsForwardBroadcast(const MACAddress &addr)
{

    if (!par("inteligentForward").boolValue())
        return true;
    if (routingModuleProactive == nullptr)
        return false;

    OLSR *olsr = dynamic_cast<OLSR*>(routingModuleProactive);
    if (olsr == nullptr)
        throw cRuntimeError("inteligentForward OLSR not found");
    if (!olsr->isNodeCandidate(L3Address(addr)))
        return false;
    return true;
}

}

}
