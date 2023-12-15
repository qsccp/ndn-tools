#include "ndn-consumer.hpp"

namespace ndn
{
    namespace cc
    {
        namespace client
        {
            Consumer::Consumer(Face &face, const Options &options)
                : m_options(options),
                  m_nSent(0),
                  m_nextSeq(options.startSeq),
                  m_seqMax(options.seqMax),
                  m_nOutstanding(0),
                  m_face(face),
                  m_scheduler(m_face.getIoService()),
                  m_stopFlag(false),
                  m_recvBytes(0),
                  m_traceTimes(0)
            {
                if (m_options.seqMax < 0)
                {
                    m_seqMax = std::numeric_limits<uint64_t>::max();
                }
                m_scheduler.schedule(
                    time::milliseconds(500),
                    bind(&Consumer::traceRate, this));
            }

            void Consumer::traceRate()
            {
                if (!m_stopFlag)
                {
                    m_scheduler.schedule(
                        time::milliseconds(500),
                        bind(&Consumer::traceRate, this));
                }

                m_traceTimes++;
                // print rate
                std::cout << "cc:rate:<" << m_traceTimes << "," << m_recvBytes << ">" << m_nSent << std::endl;
                m_recvBytes = 0;
                m_nSent = 0;
            }

            void Consumer::start()
            {
                m_stopFlag = false;
                scheduleNextPacket();
            }

            void Consumer::stop()
            {
                m_nextInterestEvent.cancel();
                m_stopFlag = true;
            }

            void Consumer::sendPacket()
            {
                uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid
                while (m_retxSeqs.size())
                {
                    seq = *m_retxSeqs.begin();
                    m_retxSeqs.erase(m_retxSeqs.begin());
                    break;
                }

                if (seq == std::numeric_limits<uint32_t>::max())
                {
                    if (m_seqMax != std::numeric_limits<uint32_t>::max())
                    {
                        if (m_nextSeq >= m_seqMax)
                        {
                            return; // we are totally done
                        }
                    }
                    seq = m_nextSeq++;
                }

                Interest interest(makeInterestName(seq));
                interest.setCanBePrefix(false);
                interest.setMustBeFresh(true);
                interest.setInterestLifetime(m_options.lifetime);

                auto now = time::steady_clock::now();
                m_face.expressInterest(interest,
                                       bind(&Consumer::onData, this, _2, seq, now),
                                       bind(&Consumer::onNack, this, _2, seq, now),
                                       bind(&Consumer::onTimeout, this, seq));

                ++m_nSent;
                ++m_nOutstanding;

                scheduleNextPacket();

                // if (m_nSent < m_seqMax)
                // {
                //     m_nextInterestEvent = m_scheduler.schedule(m_options.interval, [this]
                //                                                { scheduleNextInterest(); });
                // }
                // else
                // {
                //     finish();
                // }
            }

            void Consumer::onTimeout(uint64_t seq)
            {
                m_retxSeqs.insert(seq);
                afterTimeout(seq);
                scheduleNextPacket();
                // finish();
            }

            void Consumer::onData(const Data &data, uint64_t seq, const time::steady_clock::TimePoint &sendTime)
            {
                auto now = time::steady_clock::now();
                std::cout << "cc:delay:<" << (now - m_startTime).count() << "," << seq << "," << (now - sendTime).count() << ">" << std::endl;

                afterData(seq, now - sendTime);
                m_recvBytes += data.wireEncode().size();
                // finish();
            }

            void Consumer::onNack(const lp::Nack &nack, uint64_t seq, const time::steady_clock::TimePoint &sendTime)
            {
                std::cout << "onNack: " << nack.getReason() << std::endl;
                afterNack(seq, time::steady_clock::now() - sendTime, nack.getHeader());
                scheduleNextPacket();
                // finish();
            }

            Name Consumer::makeInterestName(uint64_t seq)
            {
                return Name(m_options.prefix).appendNumber(seq);
            }

        }
    }
}