#include "ndn-rtt-estimator.hpp"
#include <cmath>

namespace ndn
{
    namespace cc
    {
        namespace client
        {
            void
            RttEstimator::SetMinRto(time::steady_clock::duration minRto)
            {
                m_minRto = minRto;
            }

            time::steady_clock::duration
            RttEstimator::GetMinRto(void) const
            {
                return m_minRto;
            }

            void
            RttEstimator::SetMaxRto(time::steady_clock::duration maxRto)
            {
                m_maxRto = maxRto;
            }

            time::steady_clock::duration
            RttEstimator::GetMaxRto(void) const
            {
                return m_maxRto;
            }

            void
            RttEstimator::SetCurrentEstimate(time::steady_clock::duration estimate)
            {
                m_currentEstimatedRtt = estimate;
            }

            time::steady_clock::duration
            RttEstimator::GetCurrentEstimate(void) const
            {
                return m_currentEstimatedRtt;
            }

            // RttHistory methods
            RttHistory::RttHistory(uint32_t s, uint32_t c, time::steady_clock::TimePoint t)
                : seq(s), count(c), time(t), retx(false)
            {
            }

            RttHistory::RttHistory(const RttHistory &h)
                : seq(h.seq), count(h.count), time(h.time), retx(h.retx)
            {
            }

            // Base class methods
            RttEstimator::RttEstimator()
                : m_next(1),
                  m_maxMultiplier(64),
                  m_initialEstimatedRtt(second_double_to_duration(1)), // 1 seconds
                  m_minRto(second_double_to_duration(0.2)),            // 0.2 seconds
                  m_maxRto(second_double_to_duration(200)),            // 200 seconds
                  m_nSamples(0),
                  m_multiplier(1),
                  m_history()
            {
                m_currentEstimatedRtt = m_initialEstimatedRtt;
            }

            RttEstimator::RttEstimator(const RttEstimator &c)
                : m_next(c.m_next), m_maxMultiplier(c.m_maxMultiplier), m_initialEstimatedRtt(c.m_initialEstimatedRtt), m_currentEstimatedRtt(c.m_currentEstimatedRtt), m_minRto(c.m_minRto), m_maxRto(c.m_maxRto), m_nSamples(c.m_nSamples), m_multiplier(c.m_multiplier), m_history(c.m_history)
            {
            }

            RttEstimator::~RttEstimator()
            {
            }

            void
            RttEstimator::SentSeq(uint32_t seq, uint32_t size)
            {
                // Note that a particular sequence has been sent
                if (seq == m_next)
                { // This is the next expected one, just log at end
                    m_history.push_back(RttHistory(seq, size, time::steady_clock::now()));
                    m_next = seq + uint32_t(size); // Update next expected
                }
                else
                { // This is a retransmit, find in list and mark as re-tx
                    for (RttHistory_t::iterator i = m_history.begin(); i != m_history.end(); ++i)
                    {
                        if ((seq >= i->seq) && (seq < (i->seq + uint32_t(i->count))))
                        { // Found it
                            i->retx = true;
                            // One final test..be sure this re-tx does not extend "next"
                            if ((seq + uint32_t(size)) > m_next)
                            {
                                m_next = seq + uint32_t(size);
                                i->count = ((seq + uint32_t(size)) - i->seq); // And update count in hist
                            }
                            break;
                        }
                    }
                }
            }

            time::steady_clock::duration
            RttEstimator::AckSeq(uint32_t ackSeq)
            {
                // An ack has been received, calculate rtt and log this measurement
                // Note we use a linear search (O(n)) for this since for the common
                // case the ack'ed packet will be at the head of the list
                time::steady_clock::duration m = time::steady_clock::duration::zero();
                if (m_history.size() == 0)
                {
                    return (m); // No pending history, just exit
                }
                RttHistory &h = m_history.front();
                if (!h.retx && ackSeq >= (h.seq + uint32_t(h.count)))
                {                                           // Ok to use this sample
                    m = time::steady_clock::now() - h.time; // Elapsed time::nanoseconds
                    Measurement(m);                         // Log the measurement
                    ResetMultiplier();                      // Reset multiplier on valid measurement
                }
                // Now delete all ack history with seq <= ack
                while (m_history.size() > 0)
                {
                    RttHistory &h = m_history.front();
                    if ((h.seq + uint32_t(h.count)) > ackSeq)
                        break;             // Done removing
                    m_history.pop_front(); // Remove
                }
                return m;
            }

            void
            RttEstimator::ClearSent()
            {
                // Clear all history entries
                m_next = 1;
                m_history.clear();
            }

            void
            RttEstimator::IncreaseMultiplier()
            {
                m_multiplier = (m_multiplier * 2 < m_maxMultiplier) ? m_multiplier * 2 : m_maxMultiplier;
            }

            void
            RttEstimator::ResetMultiplier()
            {
                m_multiplier = 1;
            }

            void
            RttEstimator::Reset()
            {
                // Reset to initial state
                m_next = 1;
                m_currentEstimatedRtt = m_initialEstimatedRtt;
                m_history.clear(); // Remove all info from the history
                m_nSamples = 0;
                ResetMultiplier();
            }
        }
    }
}
