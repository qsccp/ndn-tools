#include "ndn-rtt-mean-deviation.hpp"

namespace ndn
{
    namespace cc
    {
        namespace client
        {
            RttMeanDeviation::RttMeanDeviation()
                : m_gain(0.125),
                  m_gain2(0.25),
                  m_variance(0)
            {
            }

            RttMeanDeviation::RttMeanDeviation(const RttMeanDeviation &c)
                : RttEstimator(c), m_gain(c.m_gain), m_gain2(c.m_gain2), m_variance(c.m_variance)
            {
            }

            void
            RttMeanDeviation::Measurement(time::steady_clock::duration m)
            {
                if (m_nSamples)
                { // Not first
                    time::steady_clock::duration err(m - m_currentEstimatedRtt);
                    double gErr = duration_to_second_double(err) * m_gain;

                    m_currentEstimatedRtt += second_double_to_duration(gErr);

                    auto abs_err = err > time::steady_clock::duration::zero() ? err : -err;

                    time::steady_clock::duration difference = abs_err - m_variance;
                    m_variance += second_double_to_duration(m_gain2 * duration_to_second_double(difference));
                }
                else
                {                              // First sample
                    m_currentEstimatedRtt = m; // Set estimate to current
                    // variance = sample / 2;               // And variance to current / 2
                    // m_variance = m; // try this  why????
                    m_variance = second_double_to_duration(duration_to_second_double(m) / 2);
                }
                m_nSamples++;
            }

            time::steady_clock::duration
            RttMeanDeviation::RetransmitTimeout()
            {
                // std::cout << "RTO -> m_currentEstimatedRtt:" << duration_to_second_double(m_currentEstimatedRtt) << ", m_variance:" << duration_to_second_double(m_variance) << std::endl;
                double retval = std::min(
                    duration_to_second_double(m_maxRto),
                    std::max(
                        m_multiplier * duration_to_second_double(m_minRto),
                        m_multiplier * (duration_to_second_double(m_currentEstimatedRtt) + 4 * duration_to_second_double(m_variance))));
                return second_double_to_duration(retval);
            }

            void
            RttMeanDeviation::Reset()
            {
                // Reset to initial state
                m_variance = time::steady_clock::duration::zero();
                RttEstimator::Reset();
            }

            void
            RttMeanDeviation::Gain(double g)
            {
                m_gain = g;
            }

            void
            RttMeanDeviation::SentSeq(uint32_t seq, uint32_t size)
            {
                RttHistory_t::iterator i;
                for (i = m_history.begin(); i != m_history.end(); ++i)
                {
                    if (seq == i->seq)
                    { // Found it
                        i->retx = true;
                        break;
                    }
                }

                // Note that a particular sequence has been sent
                if (i == m_history.end())
                    m_history.push_back(RttHistory(seq, size, time::steady_clock::now()));
            }

            time::steady_clock::duration
            RttMeanDeviation::AckSeq(uint32_t ackSeq)
            {
                // An ack has been received, calculate rtt and log this measurement
                // Note we use a linear search (O(n)) for this since for the common
                // case the ack'ed packet will be at the head of the list
                auto m = time::steady_clock::duration::zero();
                if (m_history.size() == 0)
                    return (m); // No pending history, just exit

                for (RttHistory_t::iterator i = m_history.begin(); i != m_history.end(); ++i)
                {
                    if (ackSeq == i->seq)
                    { // Found it
                        if (!i->retx)
                        {
                            m = time::steady_clock::now() - i->time; // Elapsed time
                            Measurement(m);                          // Log the measurement
                            ResetMultiplier();                       // Reset multiplier on valid measurement
                        }
                        m_history.erase(i);
                        break;
                    }
                }

                return m;
            }
        }
    }
}
