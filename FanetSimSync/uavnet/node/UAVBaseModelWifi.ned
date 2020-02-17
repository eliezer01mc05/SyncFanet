package uavnet.node;

import inet.node.inet.WirelessHost;
import uavnet.networklayer.contract.ManetRouting;

module UAVBaseModelWifi extends WirelessHost {
    
    parameters:
        @display("i=misc/quadcopter-front");
        wlan[*].mgmtType = default("Ieee80211MgmtAdhoc");  // use adhoc management
        string routingProtocol @enum("AODVUU","DYMOUM","DYMO","DSRUU","OLSR","OLSR_ETX","DSDV_2","Batman") = default(""); 
        forwarding = default(true);

    submodules:
        manetrouting: <routingProtocol> like ManetRouting if routingProtocol != "" {
            @display("p=522,307");
        }
    connections:
        networkLayer.transportOut++ --> manetrouting.ipIn if routingProtocol != "";
        networkLayer.transportIn++ <-- manetrouting.ipOut if routingProtocol != "";
}