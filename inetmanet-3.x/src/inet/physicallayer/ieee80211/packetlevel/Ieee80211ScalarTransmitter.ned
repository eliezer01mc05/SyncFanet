package inet.physicallayer.ieee80211.packetlevel;

//
// This transmitter model produces IEEE 802.11 transmissions that have scalar
// transmission power in their analog representation. The bit domain, symbol
// domain, and sample domains of the transmissions are not represented.
//
// See also ~Ieee80211ScalarReceiver, ~Ieee80211ScalarRadio, and
// ~Ieee80211ScalarRadioMedium.
//
module Ieee80211ScalarTransmitter extends Ieee80211TransmitterBase
{
    parameters:
        @class(Ieee80211ScalarTransmitter);
}
