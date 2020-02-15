#include "Dcaf.h"
#include "inet/common/ModuleAccess.h"
#include "inet/linklayer/ieee80211/mac/contract/IRx.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"

namespace inet {
namespace ieee80211 {

Define_Module(Dcaf);

void Dcaf::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        getContainingNicModule(this)->subscribe(NF_MODESET_CHANGED, this);
    }
    else if (stage == INITSTAGE_LINK_LAYER_2) {
        // TODO: calculateTimingParameters()
        contention = check_and_cast<IContention *>(getSubmodule("contention"));
        auto rx = check_and_cast<IRx *>(getModuleByPath(par("rxModule")));
        rx->registerContention(contention);
        calculateTimingParameters();
    }
}


void Dcaf::calculateTimingParameters()
{
    slotTime = modeSet->getSlotTime();
    sifs = modeSet->getSifsTime();
    double difs = par("difsTime");
    // The PIFS and DIFS are derived by the Equation (9-2) and Equation (9-3), as illustrated in Figure 9-14.
    // PIFS = aSIFSTime + aSlotTime (9-2)
    // DIFS = aSIFSTime + 2 × aSlotTime (9-3)
    ifs = difs == -1 ? sifs + 2 * slotTime : difs;
    // The EIFS is derived from the SIFS and the DIFS and the length of time it takes to transmit an ACK frame at the
    // lowest PHY mandatory rate by Equation (9-4).
    // EIFS = aSIFSTime + DIFS + ACKTxTime
    eifs = sifs + ifs + modeSet->getSlowestMandatoryMode()->getDuration(LENGTH_ACK);
    ASSERT(ifs > sifs);
    cwMin = par("cwMin");
    cwMax = par("cwMax");
    if (cwMin == -1)
        cwMin = modeSet->getCwMin();
    if (cwMax == -1)
        cwMax = modeSet->getCwMax();
    cw = cwMin;
}

void Dcaf::incrementCw()
{
    int newCw = 2 * cw + 1;
    if (newCw > cwMax)
        cw = cwMax;
    else
        cw = newCw;
}

void Dcaf::resetCw()
{
    cw = cwMin;
}

void Dcaf::channelAccessGranted()
{
    ASSERT(callback != nullptr);
    owning = true;
    contentionInProgress = false;
    callback->channelGranted(this);
}

void Dcaf::releaseChannel(IChannelAccess::ICallback* callback)
{
    owning = false;
    contentionInProgress = false;
    this->callback = nullptr;
}

void Dcaf::requestChannel(IChannelAccess::ICallback* callback)
{
    this->callback = callback;
    if (owning)
        callback->channelGranted(this);
    else if (!contentionInProgress) {
        contentionInProgress = true;
        contention->startContention(cw, ifs, eifs, slotTime, this);
    }
    else ;
}

void Dcaf::expectedChannelAccess(simtime_t time)
{
    // don't care
}

void Dcaf::receiveSignal(cComponent* source, simsignal_t signalID, cObject* obj, cObject* details)
{
    Enter_Method("receiveModeSetChangeNotification");
    if (signalID == NF_MODESET_CHANGED) {
        modeSet = check_and_cast<Ieee80211ModeSet*>(obj);
        calculateTimingParameters();
    }
}

} /* namespace ieee80211 */
} /* namespace inet */
