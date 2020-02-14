#include "inet/mobility/contract/IMobility.h"
#include "inet/physicallayer/contract/packetlevel/IRadio.h"
#include "inet/physicallayer/contract/packetlevel/RadioControlInfo_m.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211ScalarTransmitter.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211ScalarTransmission.h"

namespace inet {

namespace physicallayer {

Define_Module(Ieee80211ScalarTransmitter);

Ieee80211ScalarTransmitter::Ieee80211ScalarTransmitter() :
    Ieee80211TransmitterBase()
{
}

std::ostream& Ieee80211ScalarTransmitter::printToStream(std::ostream& stream, int level) const
{
    stream << "Ieee80211ScalarTransmitter";
    return Ieee80211TransmitterBase::printToStream(stream, level);
}

const ITransmission *Ieee80211ScalarTransmitter::createTransmission(const IRadio *transmitter, const cPacket *macFrame, simtime_t startTime) const
{
    const TransmissionRequest *transmissionRequest = dynamic_cast<TransmissionRequest *>(macFrame->getControlInfo());
    const IIeee80211Mode *transmissionMode = computeTransmissionMode(transmissionRequest);
    const Ieee80211Channel *transmissionChannel = computeTransmissionChannel(transmissionRequest);
    W transmissionPower = computeTransmissionPower(transmissionRequest);
    bps transmissionBitrate = transmissionMode->getDataMode()->getNetBitrate();
    if (transmissionMode->getDataMode()->getNumberOfSpatialStreams() > transmitter->getAntenna()->getNumAntennas())
        throw cRuntimeError("Number of spatial streams is higher than the number of antennas");
    const simtime_t duration = transmissionMode->getDuration(macFrame->getBitLength());
    const simtime_t endTime = startTime + duration;
    IMobility *mobility = transmitter->getAntenna()->getMobility();
    const Coord startPosition = mobility->getCurrentPosition();
    const Coord endPosition = mobility->getCurrentPosition();
    const EulerAngles startOrientation = mobility->getCurrentAngularPosition();
    const EulerAngles endOrientation = mobility->getCurrentAngularPosition();
    int headerBitLength = transmissionMode->getHeaderMode()->getBitLength();
    int64_t payloadBitLength = macFrame->getBitLength();
    const simtime_t preambleDuration = transmissionMode->getPreambleMode()->getDuration();
    const simtime_t headerDuration = transmissionMode->getHeaderMode()->getDuration();
    const simtime_t dataDuration = duration - headerDuration - preambleDuration;
    return new Ieee80211ScalarTransmission(transmitter, macFrame, startTime, endTime, preambleDuration, headerDuration, dataDuration, startPosition, endPosition, startOrientation, endOrientation, modulation, headerBitLength, payloadBitLength, carrierFrequency, bandwidth, transmissionBitrate, transmissionPower, transmissionMode, transmissionChannel);
}

} // namespace physicallayer

} // namespace inet

