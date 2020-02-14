/* Copyright (C) Uppsala University
 *
 * This file is distributed under the terms of the GNU general Public
 * License (GPL), see the file LICENSE
 *
 * Author: Erik Nordström, <erikn@it.uu.se>
 */
#ifndef __INET_DSR_OMNETPP_H
#define __INET_DSR_OMNETPP_H

#ifndef OMNETPP
#define OMNETPP
#endif

// #include <endian.h>
#include "uavnet/routing/manet/base/compatibility_dsr.h"
#include "dsr-uu/dsr-opt.h"


#ifdef MobilityFramework
#include "NetwPkt_m.h"
#ifndef IPv4Address
#define IPv4Address int
#endif
#define IPv4Datagram NetwPkt
#else
#include "inet/networklayer/ipv4/IPv4Datagram.h"
#include "inet/networklayer/common/IPProtocolId_m.h"
#endif

#ifdef MobilityFramework

#ifndef IPv4Datagram
#define IPv4Datagram NetwPkt
#endif

#ifndef IPv4Address
#define IPv4Address int

#endif



#endif

namespace inet {

namespace inetmanet {

class EtxCost
{
  public:
    IPv4Address address;
    double cost;
    ~EtxCost() {}
};

class DSRPkt : public IPv4Datagram
{

  protected:
    std::vector<struct dsr_opt_hdr> dsrOptions;
    IPProtocolId encap_protocol = (IPProtocolId)0;
    IPv4Address previous;
    IPv4Address next;
    std::vector <EtxCost>  costVector;
    int dsr_ttl;

  private:
    void copy(const DSRPkt& other);
    void clean();

  public:
    explicit DSRPkt(const char *name=nullptr) : IPv4Datagram(name) {costVector.clear(); dsrOptions.clear(); dsr_ttl=0;}
    ~DSRPkt ();
    DSRPkt (const DSRPkt  &m);
    DSRPkt (struct dsr_pkt *dp,int interface_id);
    DSRPkt &    operator= (const DSRPkt &m);
    virtual DSRPkt *dup() const override {return new DSRPkt(*this);}
    //void addOption();
    //readOption();
#ifdef MobilityFramework
    void setTimeToLive (int ttl) {setTtl(ttl);}
    int getTimeToLive() {return getTtl();}
#endif
    void modDsrOptions (struct dsr_pkt *p,int);
    void setEncapProtocol(IPProtocolId procotol) {encap_protocol = procotol;}
    IPProtocolId getEncapProtocol() {return encap_protocol;}
#ifdef MobilityFramework
    const IPv4Address& prevAddress() const {return previous;}
    void setPrevAddress(const IPv4Address& address_var) {previous = address_var;}
    const IPv4Address& nextAddress() const {return next;}
    void setNextAddress(const IPv4Address& address_var) {next = address_var;}
#else
    const IPv4Address prevAddress() const {return previous;}
    void setPrevAddress(const IPv4Address address_var) {previous = address_var;}
    const IPv4Address nextAddress() const {return next;}
    void setNextAddress(const IPv4Address address_var) {next = address_var;}
#endif
    std::vector<struct dsr_opt_hdr> &getDsrOptions() {return dsrOptions;}
    void  setDsrOptions(const std::vector<struct dsr_opt_hdr> &op) {dsrOptions.clear(); dsrOptions=op;}
    void clearDsrOptions(){dsrOptions.clear();}
    virtual std::string detailedInfo() const override;

    void resetCostVector();
    virtual void getCostVector(std::vector<EtxCost> &cost); // Copy
    virtual std::vector<EtxCost> getCostVector() {return costVector;}

    virtual void setCostVector(std::vector<EtxCost> &cost);

    virtual unsigned getCostVectorSize() const {return costVector.size();}

    virtual void setCostVectorSize(EtxCost);
    virtual void setCostVectorSize(u_int32_t addr, double cost);
};

class EtxList
{
  public:
    IPv4Address address;
    double delivery;// the simulation suppose that the code use a u_int32_t
};

class DSRPktExt: public IPv4Datagram
{
  protected:
    EtxList *extension;
    int size;

  private:
    void copy(const DSRPktExt& other);
    void clean() { clearExtension(); }

  public:
    explicit DSRPktExt(const char *name=nullptr) : IPv4Datagram(name) {size=0; extension=nullptr;}
    ~DSRPktExt ();
    DSRPktExt (const DSRPktExt  &m);
    DSRPktExt &     operator= (const DSRPktExt &m);
    void clearExtension();
    EtxList * addExtension(int len);
    EtxList * delExtension(int len);
    EtxList * getExtension() {return extension;}
    int getSizeExtension () {return size;}
    virtual DSRPktExt *dup() const override {return new DSRPktExt(*this);}

};

} // namespace inetmanet

} // namespace inet

#endif              /* _DSR_PKT_H */
