//
// Copyright (C) 2008 Alfonso Ariza
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
//

package inet.linklayer.ieee80211;

import inet.linklayer.common.IIeee8021dQoSClassifier;
import inet.linklayer.contract.IWirelessNic;
import inet.linklayer.ieee80211.mac.Ieee80211Mac;
import inet.linklayer.ieee80211.mgmt.Ieee80211AgentSTA;
import inet.physicallayer.contract.packetlevel.IRadio;
import inet.securityModule.SecurityWPA2;
import inet.linklayer.ieee80211.mgmt.SecurityIeee80211MgmtAdhoc;

//
// This NIC implements an 802.11g network interface card in ad-hoc mode.
//
module SecurityIeee80211Nic like IWirelessNic
{
    parameters:
        string interfaceTableModule;
        string energySourceModule = default("");
        string mgmtType = default("SecurityIeee80211MgmtAdhoc"); // name of the management module type (implements IIeee80211Mgmt)
        string radioType = default("Ieee80211Radio");
        string opMode @enum("b","g","a","p") = default("g");
        int numOutputHooks = default(0);
        int numInputHooks = default(0);
        string macType = default("Ieee80211OldMac");   // choose from Ieee80211OldMac, Ieee80211Mac, Ieee80211CompatibleMac
        double bitrate @unit("bps") = default(opMode == "b" ? 11Mbps : opMode == "p" ? 27Mbps : 54Mbps);
        bool _agentNeeded = false;   // internal par. do not use, shows if optional agent module is needed        
        bool securityNeeded = true;
        bool hasSecurity = default(false);
        @display("i=block/ifcard;bgb=259,357");
        *.interfaceTableModule = default(absPath(interfaceTableModule));
        *.energySourceModule = default(absPath(energySourceModule));        
    gates:
        input upperLayerIn; // to upper layers
        output upperLayerOut; // from upper layers
      
        input radioIn @labels(AirFrame); // to receive AirFrames
    submodules:
        classifier: <classifierType> like IIeee8021dQoSClassifier if classifierType != "" {
            @display("p=110,59;i=block/classifier");
        }
        // optional agent module (can be either 0 or 1 sized vector)
        agent: Ieee80211AgentSTA if _agentNeeded {
            parameters:
                @display("p=202,136");
        }
        security: SecurityWPA2 if hasSecurity {
            // security: Security if securityNeeded {
            parameters:
                securityActive = hasSecurity;
                @display("p=234,43;i=block/encrypt");
        }

        mgmt: SecurityIeee80211MgmtAdhoc {
            parameters:
                
                @display("p=96,136;q=wlanDataQueue;i=block/switch");
            gates:
                macIn;
                macOut;
        }
        mac: <macType> like IIeee80211Mac {
            parameters:
                @display("p=157,217");
        }
        radio: <radioType> like IRadio {
            parameters:
                @display("p=157,302");
        }
    connections allowunconnected:
        radioIn --> radio.radioIn;
        radio.upperLayerIn <-- mac.lowerLayerOut;
        radio.upperLayerOut --> mac.lowerLayerIn;

        mac.upperLayerOut --> mgmt.macIn;
        mac.upperLayerIn <-- mgmt.macOut;

        mgmt.agentOut --> agent.mgmtIn if _agentNeeded;
        mgmt.agentIn <-- agent.mgmtOut if _agentNeeded;
        
        mgmt.securityOut --> security.mgmtIn if hasSecurity;
        mgmt.securityIn <-- security.mgmtOut if hasSecurity;
        

       
        mgmt.upperLayerOut --> upperLayerOut if numInputHooks == 0;
        mgmt.upperLayerIn <-- upperLayerIn if numOutputHooks == 0;

        mgmt.upperLayerOut --> { @display("m=n"); } --> upperLayerOut;
        mgmt.upperLayerIn <-- { @display("m=n"); } <-- upperLayerIn if classifierType == "";
        mgmt.upperLayerIn <-- { @display("m=n"); } <-- classifier.out if classifierType != "";
        classifier.in <-- { @display("m=n"); } <-- upperLayerIn if classifierType != "";
      
}




