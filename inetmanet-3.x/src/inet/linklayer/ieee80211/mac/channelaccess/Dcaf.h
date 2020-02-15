#ifndef __INET_DCFCHANNELACCESS_H
#define __INET_DCFCHANNELACCESS_H

#include "inet/linklayer/ieee80211/mac/common/ModeSetListener.h"
#include "inet/linklayer/ieee80211/mac/contract/IChannelAccess.h"
#include "inet/linklayer/ieee80211/mac/contract/IContention.h"
#include "inet/linklayer/ieee80211/mac/contract/IRecoveryProcedure.h"

namespace inet {
namespace ieee80211 {

class INET_API Dcaf : public IChannelAccess, public IContention::ICallback, public IRecoveryProcedure::ICwCalculator, public ModeSetListener
{
    protected:
        Ieee80211ModeSet *modeSet = nullptr;
        IContention *contention = nullptr;
        IChannelAccess::ICallback *callback = nullptr;

        bool owning = false;
        bool contentionInProgress = false;

        simtime_t slotTime = -1;
        simtime_t sifs = -1;
        simtime_t ifs = -1;
        simtime_t eifs = -1;

        int cw = -1;
        int cwMin = -1;
        int cwMax = -1;

    protected:
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;

        virtual void calculateTimingParameters();
        virtual void receiveSignal(cComponent* source, simsignal_t signalID, cObject* obj, cObject* details) override;

    public:
        // IChannelAccess::ICallback
        virtual void requestChannel(IChannelAccess::ICallback* callback) override;
        virtual void releaseChannel(IChannelAccess::ICallback* callback) override;

        // IContention::ICallback
        virtual void channelAccessGranted() override;
        virtual void expectedChannelAccess(simtime_t time) override;

        // IRecoveryProcedure::ICallback
        virtual void incrementCw() override;
        virtual void resetCw() override;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_DCFCHANNELACCESS_H
