//
// Generated file, do not edit! Created by nedtool 5.5 from uavnet/routing/manet/base/LocatorNotificationInfo.msg.
//

#ifndef __INET__INETMANET_LOCATORNOTIFICATIONINFO_M_H
#define __INET__INETMANET_LOCATORNOTIFICATIONINFO_M_H

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <omnetpp.h>

// nedtool version check
#define MSGC_VERSION 0x0505
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of nedtool: 'make clean' should help.
#endif

// cplusplus {{
#include "inet/linklayer/common/MACAddress.h"
#include "inet/networklayer/contract/ipv4/IPv4Address.h"
// }}


namespace inet {
namespace inetmanet {

/**
 * Class generated from <tt>uavnet/routing/manet/base/LocatorNotificationInfo.msg:32</tt> by nedtool.
 * <pre>
 * class LocatorNotificationInfo
 * {
 *     MACAddress macAddr;   //
 *     IPv4Address ipAddr;
 * }
 * </pre>
 */
class LocatorNotificationInfo : public ::omnetpp::cObject
{
  protected:
    MACAddress macAddr;
    IPv4Address ipAddr;

  private:
    void copy(const LocatorNotificationInfo& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const LocatorNotificationInfo&);

  public:
    LocatorNotificationInfo();
    LocatorNotificationInfo(const LocatorNotificationInfo& other);
    virtual ~LocatorNotificationInfo();
    LocatorNotificationInfo& operator=(const LocatorNotificationInfo& other);
    virtual LocatorNotificationInfo *dup() const override {return new LocatorNotificationInfo(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
    virtual MACAddress& getMacAddr();
    virtual const MACAddress& getMacAddr() const {return const_cast<LocatorNotificationInfo*>(this)->getMacAddr();}
    virtual void setMacAddr(const MACAddress& macAddr);
    virtual IPv4Address& getIpAddr();
    virtual const IPv4Address& getIpAddr() const {return const_cast<LocatorNotificationInfo*>(this)->getIpAddr();}
    virtual void setIpAddr(const IPv4Address& ipAddr);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const LocatorNotificationInfo& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, LocatorNotificationInfo& obj) {obj.parsimUnpack(b);}

} // namespace inetmanet
} // namespace inet

#endif // ifndef __INET__INETMANET_LOCATORNOTIFICATIONINFO_M_H

