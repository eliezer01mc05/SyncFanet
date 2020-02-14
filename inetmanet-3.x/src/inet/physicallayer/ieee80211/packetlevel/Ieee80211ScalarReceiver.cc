#include "inet/physicallayer/base/packetlevel/FlatTransmissionBase.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211ScalarReceiver.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211ScalarTransmission.h"

namespace inet {

namespace physicallayer {

Define_Module(Ieee80211ScalarReceiver);

Ieee80211ScalarReceiver::Ieee80211ScalarReceiver() :
    Ieee80211ReceiverBase()
{
}

std::ostream& Ieee80211ScalarReceiver::printToStream(std::ostream& stream, int level) const
{
    stream << "Ieee80211ScalarReceiver";
    return Ieee80211ReceiverBase::printToStream(stream, level);
}

bool Ieee80211ScalarReceiver::computeIsReceptionPossible(const IListening *listening, const ITransmission *transmission) const
{
    const Ieee80211ScalarTransmission *ieee80211Transmission = check_and_cast<const Ieee80211ScalarTransmission *>(transmission);
    return NarrowbandReceiverBase::computeIsReceptionPossible(listening, transmission) && modeSet->containsMode(ieee80211Transmission->getMode());
}

} // namespace physicallayer

} // namespace inet

