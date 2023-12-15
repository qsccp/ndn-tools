#include "ndn-consumer.hpp"

namespace ndn
{
    namespace cc
    {
        namespace client
        {
            class WindowConsumer : public Consumer
            {
            public:
                explicit WindowConsumer(Face &face, const Options &options);

                virtual void onData(const Data &data, uint32_t seq, const time::steady_clock::TimePoint &sendTime);

                virtual void onTimeout(uint32_t seq);

                virtual void willSendInterest(uint32_t seq);

            protected:
                virtual void scheduleNextPacket();

                void startGreedy();

            protected:
                double m_window;
                uint32_t m_inFlight;

                uint32_t m_initialWindowSize;
                bool m_setInitialWindowOnTimeout;

                int32_t fixedRate;
                uint64_t delayStart;
                int32_t timingStop;
                int32_t delayGreedy;
                int32_t greedyRate;
                uint64_t dsz;
            };
        }
    }
}