#include "window-consumer.hpp"

namespace ndn
{
    namespace cc
    {
        namespace client
        {
            WindowConsumer::WindowConsumer(Face &face, const Options &options)
                : Consumer(face, options),
                  m_window(options.initialWindowSize),
                  m_inFlight(0),
                  m_initialWindowSize(options.initialWindowSize),
                  m_setInitialWindowOnTimeout(options.setInitialWindowOnTimeout),
                  fixedRate(options.fixedRate),
                  delayStart(options.delayStart),
                  timingStop(options.timingStop),
                  delayGreedy(options.delayGreedy),
                  greedyRate(options.greedyRate),
                  dsz(options.dsz)
            {
            }

            void WindowConsumer::startGreedy()
            {
                this->fixedRate = this->greedyRate;
            }

            void WindowConsumer::scheduleNextPacket()
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
                if (m_window == static_cast<uint32_t>(0))
                {
                    m_nextInterestEvent.cancel();
                    m_nextInterestEvent = m_scheduler.schedule(time::milliseconds(1000), [this]
                                                               { sendPacket(); });
                }
                else if (m_inFlight >= m_window)
                {
                    // simply do nothing
                }
                else if (!m_nextInterestEvent)
                {
                    // if (m_nextInterestEvent)
                    // {
                    //     std::cout << "???" << std::endl;
                    //     m_nextInterestEvent.cancel();
                    // }
                    // uint32_t waitTime = 1 / m_window * 1000000000;
                    m_nextInterestEvent = m_scheduler.schedule(time::nanoseconds(0), [this]
                                                               { sendPacket(); });
                }
            }

            void WindowConsumer::onTimeout(uint32_t seq)
            {
                if (m_inFlight > static_cast<uint32_t>(0))
                {
                    m_inFlight -= 1;
                }
                if (m_setInitialWindowOnTimeout)
                {
                    m_window = m_initialWindowSize;
                }
                Consumer::onTimeout(seq);
            }

            void WindowConsumer::onData(const Data &data, uint32_t seq, const time::steady_clock::TimePoint &sendTime)
            {
                Consumer::onData(data, seq, sendTime);
                m_window++;
                if (m_inFlight > static_cast<uint32_t>(0))
                {
                    m_inFlight--;
                }
                scheduleNextPacket();
            }

            void WindowConsumer::willSendInterest(uint32_t seq)
            {

                m_inFlight++;
                Consumer::willSendInterest(seq);
            }
        }
    }
}
