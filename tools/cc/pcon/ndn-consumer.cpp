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
                  m_traceTimes(0),
                  m_firstTime(true)
            {
                if (m_options.seqMax < 0)
                {
                    m_seqMax = std::numeric_limits<uint32_t>::max();
                }
                m_scheduler.schedule(
                    time::milliseconds(500),
                    bind(&Consumer::traceRate, this));
                m_scheduler.schedule(
                    time::milliseconds(50),
                    bind(&Consumer::checkRetxTimeout, this));
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
                m_retxEvent.cancel();
                m_stopFlag = true;
            }

            void Consumer::checkRetxTimeout()
            {
                auto now = time::steady_clock::now();

                auto rto = m_rtt.RetransmitTimeout();

                std::cout << "RTO: " << m_rtt.duration_to_second_double(rto) << std::endl;

                while (!m_seqTimeouts.empty())
                {
                    SeqTimeoutsContainer::index<i_timestamp>::type::iterator entry =
                        m_seqTimeouts.get<i_timestamp>().begin();
                    if (entry->time + rto <= now) // timeout expired?
                    {
                        // std::cout << "RTOTIMEOUT: " << entry->seq << std::endl;
                        uint32_t seqNo = entry->seq;
                        if (m_seqRetxCounts.find(seqNo) != m_seqRetxCounts.end())
                        {
                            m_seqTimeouts.get<i_timestamp>().erase(entry);
                            onTimeout(seqNo);
                        }
                    }
                    else
                        break; // nothing else to do. All later packets need not be retransmitted
                }

                m_retxEvent = m_scheduler.schedule(time::milliseconds(50), bind(&Consumer::checkRetxTimeout, this));
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
                interest.setServiceClass(5);
                interest.setInterestLifetime(m_options.lifetime);

                auto now = time::steady_clock::now();
                m_face.expressInterest(interest,
                                       bind(&Consumer::onData, this, _2, seq, now),
                                       bind(&Consumer::onNack, this, _2, seq, now),
                                       bind(&Consumer::onTimeout, this, seq));

                willSendInterest(seq);

                scheduleNextPacket();
            }

            void Consumer::onTimeout(uint32_t seq)
            {
                std::cout << "onTimeout: " << seq << std::endl;
                afterTimeout(seq);

                m_rtt.IncreaseMultiplier(); // Double the next RTO
                m_rtt.SentSeq(seq, 1);      // make sure to disable RTT calculation for this sample
                m_retxSeqs.insert(seq);

                scheduleNextPacket();
                // finish();
            }

            void Consumer::onData(const Data &data, uint32_t seq, const time::steady_clock::TimePoint &sendTime)
            {
                auto now = time::steady_clock::now();
                std::cout << "cc:delay:<" << (now - m_startTime).count() << "," << seq << "," << (now - sendTime).count() << ">" << std::endl;

                afterData(seq, now - sendTime);
                if (m_seqRetxCounts.find(seq) != m_seqRetxCounts.end())
                {
                    m_recvBytes += data.wireEncode().size();
                    m_seqRetxCounts.erase(seq);
                }

                m_seqFullDelay.erase(seq);
                m_seqLastDelay.erase(seq);

                m_seqTimeouts.erase(seq);
                m_retxSeqs.erase(seq);

                m_rtt.AckSeq(seq);
                // finish();
            }

            void Consumer::onNack(const lp::Nack &nack, uint32_t seq, const time::steady_clock::TimePoint &sendTime)
            {
                std::cout << "onNack: " << nack.getReason() << std::endl;
                afterNack(seq, time::steady_clock::now() - sendTime, nack.getHeader());
                scheduleNextPacket();
                // finish();
            }

            Name Consumer::makeInterestName(uint32_t seq)
            {
                return Name(m_options.prefix).appendNumber(seq);
            }

            void Consumer::willSendInterest(uint32_t seq)
            {
                ++m_nSent;
                m_seqTimeouts.insert(SeqTimeout(seq, time::steady_clock::now()));
                m_seqFullDelay.insert(SeqTimeout(seq, time::steady_clock::now()));

                m_seqLastDelay.erase(seq);
                m_seqLastDelay.insert(SeqTimeout(seq, time::steady_clock::now()));

                m_seqRetxCounts[seq]++;

                m_rtt.SentSeq(seq, 1);
            }
        }
    }
}
