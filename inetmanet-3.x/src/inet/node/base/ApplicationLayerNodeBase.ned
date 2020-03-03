package inet.node.base;

import inet.applications.contract.IPingApp;
import inet.common.MessageDispatcher;

module ApplicationLayerNodeBase extends TransportLayerNodeBase
{
    parameters:
        int numApps = default(0);
        @figure[applicationLayer](type=rectangle; pos=250,6; size=1000,130; lineColor=#808080; cornerRadius=5; fillColor=#ffff00; fillOpacity=0.1);
        @figure[applicationLayer.title](type=text; pos=1245,11; anchor=ne; text="application layer");

    submodules:
        app[numApps]: <> like IPingApp {
            parameters:
                @display("p=375,76,row,150");
        }
        at: MessageDispatcher {
            parameters:
                @display("p=750,146;b=1000,5,,,,1");
        }

    connections allowunconnected:
        for i=0..numApps-1 {
            app[i].pingOut --> at.in++;
            app[i].pingIn <-- at.out++;
        }

        at.out++ --> udp.appIn[0] if hasUdp;
        at.in++ <-- udp.appOut[0] if hasUdp;

        at.out++ --> tcp.appIn[0] if hasTcp;
        at.in++ <-- tcp.appOut[0] if hasTcp;

        at.out++ --> sctp.from_ip if hasSctp;
        at.in++ <-- sctp.to_ip if hasSctp;

        at.out++ --> tn.in++;
        at.in++ <-- tn.out++;
}
