#ifndef NDN_CONSUMER_H
#define NDN_CONSUMER_H
#include "core/common.hpp"
#include "ndn-rtt-mean-deviation.hpp"
#include <set>
#include <map>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

namespace ndn
{
    namespace cc
    {
        namespace client
        {
            typedef time::duration<double, time::milliseconds::period> Rtt;

            enum CcAlgorithm
            {
                AIMD,
                BIC,
                CUBIC
            };

            class Options
            {
            public:
                Name prefix;
                uint32_t startSeq;           // Initial sequence number
                time::milliseconds lifetime; // Lifetime of Interest
                int64_t seqMax;              // Max sequence number
                uint32_t tos;                // Type of Service
                uint32_t dsz;                // Data size
                int32_t reqNum;              // Request Data Num(-1 indicator infinity)
                uint32_t initialSendRate;    // Initial Send Rate
                int32_t fixedRate;           // Fixed Send Rate
                int32_t timingStop;          // Timing Stop(-1 indicator infinity)
                int32_t delayGreedy;         // Start greedy after the specified time (ms)
                int32_t greedyRate;          // Greedy Send Rate(-1 indicator no greedy)
                uint32_t delayStart;         // Delay Start(ms)
            };

            struct BbrPacketInfo {
                // Interest packet send time
                time::steady_clock::TimePoint sendTime;
                // Total data bytes received while send this Interest
                uint64_t deliveredAtSend;
                // Last acked data received time while send this Interest
                time::steady_clock::TimePoint deliveredTime;
                bool isRetx;
            };

            typedef struct BbrPacketInfo BbrPacketInfo;

            class Consumer : noncopyable
            {
            public:
                Consumer(Face &face, const Options &options);

                virtual ~Consumer() {}

                signal::Signal<Consumer, uint32_t, Rtt> afterData;

                signal::Signal<Consumer, uint32_t, Rtt, lp::NackHeader> afterNack;

                signal::Signal<Consumer, uint32_t> afterTimeout;

                signal::Signal<Consumer> afterFinish;

                void start();

                void stop();

            public:
                Name makeInterestName(uint32_t seq);

                virtual void sendPacket();

                virtual void onTimeout(uint32_t seq);

                virtual void onData(const Data &data, uint32_t seq, const BbrPacketInfo &bbrInfo);

                virtual void onNack(const lp::Nack &nack, uint32_t seq, const BbrPacketInfo &bbrInfo);

                virtual void willSendInterest(uint32_t seq);

            protected:
                virtual void scheduleNextPacket() = 0;

                /**
                 * \brief Checks if the packet need to be retransmitted becuase of retransmission timer expiration
                 */
                void
                checkRetxTimeout();

            private:
                void traceRate();

            protected:
                const Options &m_options;
                int m_nSent;
                uint32_t m_nextSeq;
                int64_t m_seqMax;
                int m_nOutstanding;
                Face &m_face;
                Scheduler m_scheduler;
                int m_stopFlag;

                uint64_t m_recvBytes;
                uint64_t m_traceTimes;
                time::steady_clock::TimePoint m_startTime;
                scheduler::EventId m_nextInterestEvent;
                scheduler::EventId m_probeRTTEvent;
                scheduler::EventId m_doGainCycleEvent;
                bool m_firstTime = true;

                // bbr
                uint64_t m_delivered;
                time::steady_clock::TimePoint m_deliveredTime;
                uint32_t m_packetSize = 8624;

                /// @cond include_hidden
                /**
                 * \struct This struct contains sequence numbers of packets to be retransmitted
                 */
                struct RetxSeqsContainer : public std::set<uint32_t>
                {
                };

                RetxSeqsContainer m_retxSeqs; ///< \brief ordered set of sequence numbers to be retransmitted

                RttMeanDeviation m_rtt;
                scheduler::EventId m_retxEvent;
                // RttEstimator m_rtt;
                /**
                 * \struct This struct contains a pair of packet sequence number and its timeout
                 */
                struct SeqTimeout
                {
                    SeqTimeout(uint32_t _seq, time::steady_clock::TimePoint _time)
                        : seq(_seq), time(_time)
                    {
                    }

                    uint32_t seq;
                    time::steady_clock::TimePoint time;
                };
                /// @endcond

                /// @cond include_hidden
                class i_seq
                {
                };
                class i_timestamp
                {
                };
                /// @endcond

                /// @cond include_hidden
                /**
                 * \struct This struct contains a multi-index for the set of SeqTimeout structs
                 */
                struct SeqTimeoutsContainer
                    : public boost::multi_index::
                          multi_index_container<SeqTimeout,
                                                boost::multi_index::
                                                    indexed_by<boost::multi_index::
                                                                   ordered_unique<boost::multi_index::tag<i_seq>,
                                                                                  boost::multi_index::
                                                                                      member<SeqTimeout, uint32_t,
                                                                                             &SeqTimeout::seq>>,
                                                               boost::multi_index::
                                                                   ordered_non_unique<boost::multi_index::
                                                                                          tag<i_timestamp>,
                                                                                      boost::multi_index::
                                                                                          member<SeqTimeout, time::steady_clock::TimePoint,
                                                                                                 &SeqTimeout::time>>>>
                {
                };

                SeqTimeoutsContainer m_seqTimeouts; ///< \brief multi-index for the set of SeqTimeout structs

                SeqTimeoutsContainer m_seqLastDelay;
                SeqTimeoutsContainer m_seqFullDelay;
                std::map<uint32_t, uint32_t> m_seqRetxCounts;
            };
        }
    }
}
#endif /* RTT_ESTIMATOR_H */