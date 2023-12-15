#include "qsccp-consumer.hpp"
#include <iostream>

namespace ndn
{
    namespace cc
    {
        namespace client
        {
            QsccpConsumer::QsccpConsumer(Face &face, const Options &options)
                : Consumer(face, options),
                  m_firstTime(true),
                  tos(options.tos),
                  delayStart(options.delayStart),
                  dsz(options.dsz),
                  sendRate(options.initialSendRate),
                  fixedRate(options.fixedRate),
                  timingStop(options.timingStop),
                  delayGreedy(options.delayGreedy),
                  greedyRate(options.greedyRate),
                  m_recvDataNum(0)

            {
            }

            uint64_t QsccpConsumer::updateRate(uint64_t newRate)
            {
                if (this->fixedRate > 0)
                {
                    return this->fixedRate;
                }
                if (this->sendRate == 0)
                {
                    this->sendRate = newRate;
                }
                else
                {
                    this->sendRate = 0.8 * this->sendRate + 0.2 * newRate;
                }
                return this->sendRate;
            }

            void QsccpConsumer::startGreedy()
            {
                this->fixedRate = this->greedyRate;
                this->sendRate = this->greedyRate;
            }

            void QsccpConsumer::onData(const Data &data, uint64_t seq, const time::steady_clock::TimePoint &sendTime)
            {
                // print tags
                auto targetRate = data.getTargetRate();
                // std::cout << "onData: " << seq << ", targetRate: " << *targetRate << std::endl;
                if (targetRate)
                {
                    this->updateRate(*targetRate);
                }
                else
                {
                    // NS_LOG_DEBUG("no target rate");
                }

                scheduleNextPacket();

                Consumer::onData(data, seq, sendTime);

                ++this->m_recvDataNum;
                if (this->m_seqMax == this->m_recvDataNum)
                {
                    this->stop();
                    return;
                }
            }

            void QsccpConsumer::scheduleNextPacket()
            {
                if (m_stopFlag)
                {
                    return;
                }
                if (this->m_firstTime)
                {
                    m_startTime = time::steady_clock::now();
                    this->m_firstTime = false;

                    if (this->fixedRate > 0)
                    {
                        this->sendRate = fixedRate;
                    }
                    if (this->timingStop > 0)
                    {
                        m_scheduler.schedule(time::milliseconds(this->timingStop), [this]
                                             { 
                            stop(); });
                    }
                    if (this->delayGreedy > 0 && this->greedyRate > 0)
                    {
                        m_scheduler.schedule(time::milliseconds(this->delayGreedy), [this]
                                             { startGreedy(); });
                    }
                    m_nextInterestEvent = m_scheduler.schedule(time::milliseconds(this->delayStart), [this]
                                                               { sendPacket(); });
                }
                else if (!m_nextInterestEvent)
                {
                    if (this->sendRate <= 0)
                    {
                        return;
                    }
                    auto waitTime = (this->dsz * 1000000000) / this->sendRate + 1;
                    m_nextInterestEvent = m_scheduler.schedule(time::nanoseconds(waitTime), [this]
                                                               { sendPacket(); });
                }
            }

            void QsccpConsumer::sendPacket()
            {
                scheduleNextPacket();
                // send packet here
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
                            stop();
                            // finish();
                            return; // we are totally done
                        }
                    }
                    seq = m_nextSeq++;
                }

                Interest interest(makeInterestName(seq));
                interest.setCanBePrefix(false);
                interest.setMustBeFresh(true);
                interest.setServiceClass(this->tos);
                interest.setDsz(this->dsz);
                interest.setInterestLifetime(m_options.lifetime);

                auto now = time::steady_clock::now();
                m_face.expressInterest(interest,
                                       bind(&Consumer::onData, this, _2, seq, now),
                                       bind(&Consumer::onNack, this, _2, seq, now),
                                       bind(&Consumer::onTimeout, this, seq));

                ++m_nSent;
                ++m_nOutstanding;
            }
        }
    }
}