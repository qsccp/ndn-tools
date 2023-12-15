#include "ndn-consumer.hpp"

namespace ndn
{
    namespace cc
    {
        namespace client
        {
            class QsccpConsumer : public Consumer
            {
            public:
                explicit QsccpConsumer(Face &face, const Options &options);

                void sendPacket() override;

                void onData(const Data& data, uint64_t seq, const time::steady_clock::TimePoint &sendTime) override;

            protected:
                void scheduleNextPacket() override;

                void startGreedy();

            private:
                uint64_t updateRate(uint64_t newRate);

            private:
                bool m_firstTime = true;
                // private attribute here
                uint64_t tos;
                uint64_t delayStart;
                uint64_t dsz;
                uint64_t sendRate;
                int32_t fixedRate;
                int32_t timingStop;
                int32_t delayGreedy;
                int32_t greedyRate;
                uint64_t m_recvDataNum;
            };
        }
    }
}