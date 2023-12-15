#ifndef BBR_CONSUMER_H
#define BBR_CONSUMER_H
#include "ndn-consumer.hpp"

namespace ndn
{
    namespace cc
    {
        namespace client
        {
            class BbrConsumer : public Consumer
            {
            public:
                enum BbrMode
                {
                    STARTUP,
                    DRAIN,
                    PROBE_BW,
                    PROBE_RTT
                };

                explicit BbrConsumer(Face &face, const Options &options);

                virtual void onData(const Data &data, uint32_t seq, const BbrPacketInfo &bbrInfo);

                virtual void onTimeout(uint32_t seq);

                virtual void willSendInterest(uint32_t seq);

            protected:
                virtual void scheduleNextPacket();

                void startGreedy();

                void enterProbeBandwidthMode();

                void enterProbeRttMode();

                void doGainCycle();

            private:
                // window
                uint32_t m_inFlight;

                int32_t fixedRate;
                uint64_t delayStart;
                int32_t timingStop;
                int32_t delayGreedy;
                int32_t greedyRate;
                uint64_t dsz;

                // bbr
                double m_minRtt;
                double m_maxBandwidth;
                double m_PacingGain;
                double m_CwndGain;
                BbrMode m_mode;
                uint32_t m_roundCount;
            };
        }
    }
}

#endif // BBR_CONSUMER_H