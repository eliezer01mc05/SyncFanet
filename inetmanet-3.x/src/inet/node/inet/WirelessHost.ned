package inet.node.inet;

//
// Models a host with (default) one wireless (802.11) card in infrastructure mode.
// This module is basically a ~StandardHost with an ~Ieee80211Nic with
// mgmtType = ~Ieee80211MgmtSTA added.
// It should be used in conjunction with ~AccessPoint, or any other AP model
// which contains ~Ieee80211Nic with mgmtType = ~Ieee80211MgmtAP.
//
module WirelessHost extends StandardHost {
    @display("i=misc/quadcopter-front");
    numRadios = default(1);
}
