package inet.node.base;

import inet.common.MessageDispatcher;
import inet.transportlayer.contract.ISCTP;
import inet.transportlayer.contract.ITCP;
import inet.transportlayer.contract.IUDP;

module TransportLayerNodeBase extends NetworkLayerNodeBase
{
    parameters:
        bool hasUdp = default(firstAvailableOrEmpty("Udp") != "");
        bool hasTcp = default(firstAvailableOrEmpty("Tcp", "TcpLwip", "TcpNsc") != "");
        bool hasSctp = default(false);
        @figure[transportLayer](type=rectangle; pos=250,156; size=1000,130; fillColor=#ff0000; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[transportLayer.title](type=text; pos=1245,161; anchor=ne; text="transport layer");
    submodules:
        udp: <default(firstAvailableOrEmpty("Udp"))> like IUDP if hasUdp {
            parameters:
                @display("p=375,226");
        }
        tcp: <default(firstAvailableOrEmpty("Tcp", "TcpLwip", "TcpNsc"))> like ITCP if hasTcp {
            parameters:
                @display("p=525,226");
        }
        sctp: <default(firstAvailableOrEmpty("Sctp"))> like ISCTP if hasSctp {
            parameters:
                @display("p=675,226");
        }
        tn: MessageDispatcher {
            parameters:
                @display("p=750,296;b=1000,5,,,,1");
        }

    connections allowunconnected:
        udp.ipOut --> tn.in++ if hasUdp;
        udp.ipIn <-- tn.out++ if hasUdp;

        tcp.ipOut --> tn.in++ if hasTcp;
        tcp.ipIn <-- tn.out++ if hasTcp;

        sctp.to_ip --> tn.in++ if hasSctp;
        tn.out++ --> sctp.from_ip if hasSctp;

        tn.out++ --> ipv4.transportIn[0] if hasIpv4;
        tn.in++ <-- ipv4.transportOut[0] if hasIpv4;

        tn.out++ --> ipv6.transportIn[0] if hasIpv6;
        tn.in++ <-- ipv6.transportOut[0] if hasIpv6;

        tn.out++ --> generic.transportIn[0] if hasGn;
        tn.in++ <-- generic.transportOut[0] if hasGn;

        tn.out++ --> nl.in++;
        tn.in++ <-- nl.out++;
}