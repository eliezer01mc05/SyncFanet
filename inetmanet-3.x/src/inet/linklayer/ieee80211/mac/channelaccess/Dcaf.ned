package inet.linklayer.ieee80211.mac.channelaccess;

import inet.linklayer.ieee80211.mac.contention.Contention;


module Dcaf
{
    parameters:
        double difsTime @unit("s") = default(-1s);
        int cwMin = default(-1);
        int cwMax = default(-1);

        string rateSelectionModule;
        string rxModule;

        @class("Dcaf");
        @display("i=block/star");
    submodules:
        contention: Contention {
            parameters:
	            @display("p=100,100");
        }
}
