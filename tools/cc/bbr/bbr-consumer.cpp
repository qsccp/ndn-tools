#include "bbr-consumer.hpp"
#include <cmath>

namespace ndn
{
    namespace cc
    {
        namespace client
        {
            BbrConsumer::BbrConsumer(Face &face, const Options &options)
                : Consumer(face, options),
                  m_inFlight(0),
                  fixedRate(options.fixedRate),
                  delayStart(options.delayStart),
                  timingStop(options.timingStop),
                  delayGreedy(options.delayGreedy),
                  greedyRate(options.greedyRate),
                  dsz(options.dsz),
                  m_minRtt(std::numeric_limits<double>::max()),
                  m_maxBandwidth(0.0),
                  m_PacingGain(2 / std::log(2)),
                  m_CwndGain(2 / std::log(2)),
                  m_mode(STARTUP),
                  m_roundCount(0)
            {
            }

            void BbrConsumer::startGreedy()
            {
                this->fixedRate = this->greedyRate;
            }

            void BbrConsumer::scheduleNextPacket()
            {
                if (m_stopFlag)
                {
                    return;
                }
                if (this->m_firstTime)
                {
                    this->m_firstTime = false;
                    if (this->timingStop > 0)
                    {
                        m_scheduler.schedule(time::milliseconds(this->timingStop), [this]
                                             { stop(); });
                    }
                    if (this->delayGreedy > 0 && this->greedyRate > 0)
                    {
                        m_scheduler.schedule(time::milliseconds(this->delayGreedy), [this]
                                             { startGreedy(); });
                    }
                    m_nextInterestEvent = m_scheduler.schedule(time::milliseconds(this->delayStart), [this]
                                                               { sendPacket(); });
                    return;
                }

                if (this->fixedRate > 0)
                {
                    auto waitTime = (this->dsz * 1000000000) / this->fixedRate + 1;
                    m_nextInterestEvent = m_scheduler.schedule(time::nanoseconds(waitTime), [this]
                                                               { sendPacket(); });
                    return;
                }
                if (!m_nextInterestEvent)
                {
                    auto bdp = m_minRtt * m_maxBandwidth;
                    // std::cout << "BDP: " << bdp << ", inFlight: " << m_inFlight << ", minRtt: " << m_minRtt << ", maxBandwidth: " << m_maxBandwidth << std::endl;
                    if (m_mode == DRAIN && m_inFlight < bdp)
                    {
                        doGainCycle();
                        enterProbeBandwidthMode();

                        // Enter ProbeRTT mode every 10 seconds
                        m_scheduler.schedule(time::seconds(10), [this]
                                             { enterProbeRttMode(); });
                    }

                    if (m_mode == PROBE_RTT)
                    {
                        if (m_inFlight >= 4)
                        {
                            return;
                        }
                        m_nextInterestEvent = m_scheduler.schedule(time::milliseconds(250), [this]
                                                                   { sendPacket(); });
                    }
                    else
                    {
                        if (m_inFlight >= m_CwndGain * bdp)
                        {
                            return;
                        }
                        uint32_t waitTime = m_packetSize / (m_PacingGain * m_maxBandwidth) * 1000000000;
                        // std::cout << "waitTime: " <<  m_packetSize / (m_PacingGain * m_maxBandwidth) << std::endl;
                        m_nextInterestEvent = m_scheduler.schedule(time::nanoseconds(waitTime), [this]
                                                                   { sendPacket(); });
                    }
                }
            }

            void BbrConsumer::willSendInterest(uint32_t seq)
            {

                m_inFlight++;
                Consumer::willSendInterest(seq);
            }

            void BbrConsumer::onTimeout(uint32_t seq)
            {
                if (m_inFlight > static_cast<uint32_t>(0))
                {
                    m_inFlight--;
                }

                Consumer::onTimeout(seq);
            }

            void BbrConsumer::onData(const Data &data, uint32_t seq, const BbrPacketInfo &bbrInfo)
            {
                Consumer::onData(data, seq, bbrInfo);

                if (!bbrInfo.isRetx)
                {
                    auto now = time::steady_clock::now();
                    // 1. Update minimum RTT
                    auto rtt = m_rtt.duration_to_second_double(now - bbrInfo.sendTime);
                    if (m_minRtt > rtt)
                    {
                        m_minRtt = rtt;
                    }

                    // 2. Calculate the current delivery rate
                    auto delivered_rate = (m_delivered - bbrInfo.deliveredAtSend) / m_rtt.duration_to_second_double((m_deliveredTime - bbrInfo.deliveredTime));
                    // std::cout << "delivered_rate: " << delivered_rate << ", m_delivered: " << m_delivered << ", deliveredAtSend: " << bbrInfo.deliveredAtSend << ", rtt: " << m_rtt.duration_to_second_double(m_deliveredTime - bbrInfo.deliveredTime)  << std::endl;
                    if (delivered_rate > m_maxBandwidth)
                    {
                        m_maxBandwidth = delivered_rate;
                    }
                    else if (m_mode == STARTUP)
                    {
                        m_mode = DRAIN;
                        m_PacingGain = std::log(2) / 2;
                    }
                }

                if (m_inFlight > static_cast<uint32_t>(0))
                {
                    m_inFlight--;
                }

                scheduleNextPacket();
            }

            void BbrConsumer::doGainCycle()
            {
                m_roundCount++;
                auto gainStage = m_roundCount % 8;
                if (gainStage == 0)
                {
                    m_PacingGain = 1.25;
                }
                else if (gainStage == 1)
                {
                    m_PacingGain = 0.75;
                }
                else
                {
                    m_PacingGain = 1;
                }
                uint32_t waitTime = m_minRtt * 1000000000;
                m_doGainCycleEvent = m_scheduler.schedule(time::nanoseconds(waitTime), [this]
                                                        { doGainCycle(); });
            }

            void BbrConsumer::enterProbeBandwidthMode()
            {
                m_mode = PROBE_BW;
            }

            void BbrConsumer::enterProbeRttMode()
            {
                m_mode = PROBE_RTT;

                auto finishRttProbeDuration = m_minRtt;
                if (finishRttProbeDuration > 0.2)
                {
                    finishRttProbeDuration = 0.2;
                }

                uint32_t waitTime = finishRttProbeDuration * 1000000000;

                // Leave PROBE_RTT mode after max(rtt, 200ms)
                m_scheduler.schedule(time::nanoseconds(waitTime),
                                     [this]
                                     {
                                         enterProbeBandwidthMode();
                                     });

                // Enter PROBE_RTT again after 10 seconds
                m_probeRTTEvent = m_scheduler.schedule(time::seconds(10), [this]
                                                     { enterProbeRttMode(); });
            }

        }
    }
}
