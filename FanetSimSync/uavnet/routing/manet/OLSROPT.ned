package uavnet.routing.manet;

import uavnet.networklayer.contract.ManetRouting;

@namespace(inet::inetmanet);
//
// Optimized Link State Routing Protocol
//
// Implements core functionality as described in rfc3626 chapter 2.1.
// Supports optional Link Layer feedback (see the use_mac parameter).
// Auxiliary functions are NOT supported.
//
// NOTE:
//  - The initialization of the routing component is done in stage=4
//  - hello, tc, mid intervals accept INTEGER values only
//  - IPv4 addresses are assumed in the messages. If you need IPv6 recompile with -DOLSR_IPv6 added to CFLAGS
//  - Multiple interfaces per OLSR node is supported only with adding -DMULTIPLE_IFACES_SUPPORT to CFLAGS
//
simple OLSROPT extends OLSR like ManetRouting {
    
    parameters:
        @class(OLSROPT);
}



