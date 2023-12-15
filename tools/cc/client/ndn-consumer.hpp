#include "core/common.hpp"

namespace ndn
{
    namespace cc
    {
        namespace client
        {
            typedef time::duration<double, time::milliseconds::period> Rtt;

            class Options
            {
            public:
                Name prefix;
                uint64_t startSeq;           // Initial sequence number
                time::milliseconds lifetime; // Lifetime of Interest
                int64_t seqMax;              // Max sequence number
                uint32_t tos;                // Type of Service
                uint32_t dsz;                // Data size
                int32_t reqNum;              // Request Data Num(-1 indicator infinity)
                uint32_t initialSendRate;    // Initial Send Rate
                int32_t fixedRate;          // Fixed Send Rate
                int32_t timingStop;          // Timing Stop(-1 indicator infinity)
                int32_t delayGreedy;         // Start greedy after the specified time (ms)
                int32_t greedyRate;          // Greedy Send Rate(-1 indicator no greedy)
                uint32_t delayStart;         // Delay Start(ms)
            };

            class Consumer : noncopyable
            {
            public:
                Consumer(Face &face, const Options &options);

                virtual ~Consumer(){}

                signal::Signal<Consumer, uint64_t, Rtt> afterData;

                signal::Signal<Consumer, uint64_t, Rtt, lp::NackHeader> afterNack;

                signal::Signal<Consumer, uint64_t> afterTimeout;

                signal::Signal<Consumer> afterFinish;

                void start();

                void stop();

            public:
                Name makeInterestName(uint64_t seq);

                virtual void sendPacket();

                virtual void onTimeout(uint64_t seq);

                virtual void onData(const Data& data, uint64_t seq, const time::steady_clock::TimePoint &sendTime);

                virtual void onNack(const lp::Nack &nack, uint64_t seq, const time::steady_clock::TimePoint &sendTime);

            protected:
                virtual void scheduleNextPacket() = 0;

            private:
                void traceRate();

            protected:
                const Options &m_options;
                int m_nSent;
                uint64_t m_nextSeq;
                int64_t m_seqMax;
                int m_nOutstanding;
                Face &m_face;
                Scheduler m_scheduler;
                int m_stopFlag;

                uint64_t m_recvBytes;
                uint64_t m_traceTimes;
                time::steady_clock::TimePoint m_startTime;
                /// @cond include_hidden
                /**
                 * \struct This struct contains sequence numbers of packets to be retransmitted
                 */
                struct RetxSeqsContainer : public std::set<uint32_t>
                {
                };
                RetxSeqsContainer m_retxSeqs; ///< \brief ordered set of sequence numbers to be retransmitted
                scheduler::EventId m_nextInterestEvent;
            };
        }
    }
}