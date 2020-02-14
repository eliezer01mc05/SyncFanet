/*
 * Copyright (C) 2003 Andras Varga; CTIE, Monash University, Australia
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __INET_MACADDRESS_H
#define __INET_MACADDRESS_H

#include <string>

#include "inet/common/INETDefs.h"

namespace inet {

#define MAC_ADDRESS_SIZE 6
#define MAC_ADDRESS_SIZE64 8

#define MAC_ADDRESS_MASK 0xffffffffffffULL
#define MAC_ADDRESS_MASK64 0xffffffffffffffffULL

class IPv4Address;
class InterfaceToken;

/**
 * Stores an IEEE 802 MAC address (6 octets = 48 bits).
 */
class INET_API MACAddress
{
  private:
    uint64 address;    // 6*8=48 bit address, lowest 6 bytes are used, highest 2 bytes are always zero
    static unsigned int autoAddressCtr;    // global counter for generateAutoAddress()
    static bool simulationLifecycleListenerAdded;
    bool macAddress64;

  public:
    class SimulationLifecycleListener : public cISimulationLifecycleListener
    {
        virtual void lifecycleEvent(SimulationLifecycleEventType eventType, cObject *details) {
            if (eventType == LF_PRE_NETWORK_INITIALIZE)
                autoAddressCtr = 0;
        }

        virtual void listenerRemoved() {
            delete this;
        }
    };

    /** The unspecified MAC address, 00:00:00:00:00:00 */
    static const MACAddress UNSPECIFIED_ADDRESS;

    /** The broadcast MAC address, ff:ff:ff:ff:ff:ff */
    static const MACAddress BROADCAST_ADDRESS;

    static const MACAddress BROADCAST_ADDRESS64;

    /** The special multicast PAUSE MAC address, 01:80:C2:00:00:01 */
    static const MACAddress MULTICAST_PAUSE_ADDRESS;

    /** The spanning tree protocol bridge's multicast address, 01:80:C2:00:00:00 */
    static const MACAddress STP_MULTICAST_ADDRESS;

    /**
     * Default constructor initializes address bytes to zero.
     */
    MACAddress() {
        address = 0;
        macAddress64 = false;
    }

    /**
     * Initializes the address from the lower 48 bits of the 64-bit argument
     */

    explicit MACAddress(uint64 bits) {
        if (bits & !MAC_ADDRESS_MASK)
            macAddress64 = true;
        else
            macAddress64 = false;
        address = bits;
    }


    /**
     * Constructor which accepts a hex string (12 hex digits, may also
     * contain spaces, hyphens and colons)
     */
    explicit MACAddress(const char *hexstr) { setAddress(hexstr); }

    /**
     * Copy constructor.
     */
    MACAddress(const MACAddress& other) { address = other.address; macAddress64 = other.macAddress64;}

    /**
     * Assignment.
     */
    MACAddress& operator=(const MACAddress& other) { address = other.address; macAddress64 = other.macAddress64; return *this; }

    /**
     * Returns the address size in bytes, that is, 6.
     */
    unsigned int getAddressSize() const { return (macAddress64?MAC_ADDRESS_SIZE64:MAC_ADDRESS_SIZE); }

    /**
     * Returns the kth byte of the address.
     */
    unsigned char getAddressByte(unsigned int k) const;

    /**
     * Sets the kth byte of the address.
     */
    void setAddressByte(unsigned int k, unsigned char addrbyte);

    /**
     * Sets the address and returns true if the syntax of the string
     * is correct. (See setAddress() for the syntax.)
     */
    bool tryParse(const char *hexstr);

    /**
     * Converts address value from hex string (12 hex digits, may also
     * contain spaces, hyphens and colons)
     */
    void setAddress(const char *hexstr);

    /**
     * Copies the address to the given pointer (array of 6 unsigned chars).
     */
    void getAddressBytes(unsigned char *addrbytes) const;
    void getAddressBytes(char *addrbytes) const { getAddressBytes((unsigned char *)addrbytes); }

    /**
     * Sets address bytes. The argument should point to an array of 6 unsigned chars.
     */
    void setAddressBytes(unsigned char *addrbytes);
    void setAddressBytes(char *addrbytes) { setAddressBytes((unsigned char *)addrbytes); }

    /**
     * Sets the address to the broadcast address (hex ff:ff:ff:ff:ff:ff).
     */
    void setBroadcast() {address = (macAddress64 ? (uint64_t)MAC_ADDRESS_MASK64 :(uint64_t)MAC_ADDRESS_MASK); }

    /**
     * Returns true if this is the broadcast address (hex ff:ff:ff:ff:ff:ff).
     */
    bool isBroadcast() const {return (macAddress64 ? address == (uint64_t)MAC_ADDRESS_MASK64 : address == MAC_ADDRESS_MASK); }

    /**
     * Returns true if this is a multicast logical address (first byte's lsb is 1).
     */
    bool isMulticast() const { return getAddressByte(0) & 0x01; };

    /**
     * Returns true if all address bytes are zero.
     */
    bool isUnspecified() const { return address == 0; }

    /**
     * Converts address to a hex string.
     */
    std::string str() const;

    /**
     * Converts address to 48 bits integer.
     */
    uint64 getInt() const { return address; }

    /**
     * Returns true if the two addresses are equal.
     */
    bool equals(const MACAddress& other) const { return address == other.address; }

    /**
     * Returns true if the two addresses are equal.
     */
    bool operator==(const MACAddress& other) const { return address == other.address; }

    /**
     * Returns true if the two addresses are not equal.
     */
    bool operator!=(const MACAddress& other) const { return address != other.address; }

    /**
     * Returns -1, 0 or 1 as result of comparison of 2 addresses.
     */
    int compareTo(const MACAddress& other) const;

    /**
     * Create interface identifier (IEEE EUI-64) which can be used by IPv6
     * stateless address autoconfiguration.
     */
    InterfaceToken formInterfaceIdentifier() const;

    /**
     * Generates a unique address which begins with 0a:aa and ends in a unique
     * suffix.
     */
    static MACAddress generateAutoAddress();

    /**
     * Form a MAC address for a multicast IPv4 address, see  RFC 1112, section 6.4
     */
    static MACAddress makeMulticastAddress(IPv4Address addr);

    bool operator<(const MACAddress& other) const { return address < other.address; }

    bool operator>(const MACAddress& other) const { return address > other.address; }

    // works with MACaddress (64 bits)
    // convert EUI-48 to EUI64
    void convert64();

    // get the address like EUI64, with independence if it is EUI64 or EUI48
    MACAddress getEui64();

    // try to convert EUI-64 to EUI-48 if us possible
    void convert48();

    // try to get the address like EUI-48, if is possible
    MACAddress getEui48();

    MACAddress getMaskEUI48() { return MACAddress(MAC_ADDRESS_MASK); }

    MACAddress getMaskEUI64() { return MACAddress((uint64_t)MAC_ADDRESS_MASK64); }

    bool getFlagEui64() const {return macAddress64; }
    void setFlagEui64(bool v)  {macAddress64 = v; }

    bool operator&(const MACAddress& other) const { return address & other.address; }
    bool operator|(const MACAddress& other) const { return address | other.address; }

};

inline std::ostream& operator<<(std::ostream& os, const MACAddress& mac)
{
    return os << mac.str();
}

} // namespace inet

#endif // ifndef __INET_MACADDRESS_H

