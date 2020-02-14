#ifndef __INET_IEEE80211SCALARTRANSMITTER_H
#define __INET_IEEE80211SCALARTRANSMITTER_H

#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211TransmitterBase.h"

namespace inet {

namespace physicallayer {

class INET_API Ieee80211ScalarTransmitter : public Ieee80211TransmitterBase
{
  public:
    Ieee80211ScalarTransmitter();

    virtual std::ostream& printToStream(std::ostream& stream, int level) const override;

    virtual const ITransmission *createTransmission(const IRadio *radio, const cPacket *packet, simtime_t startTime) const override;
};

} // namespace physicallayer

} // namespace inet

#endif // ifndef __INET_IEEE80211SCALARTRANSMITTER_H

