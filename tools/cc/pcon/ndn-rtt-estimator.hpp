#ifndef NDN_RTT_ESTIMATOR_H
#define NDN_RTT_ESTIMATOR_H
#include "core/common.hpp"
#include <deque>

namespace ndn
{
    namespace cc
    {
        namespace client
        {
            /**
             * \ingroup ndn-apps
             *
             * \brief Helper class to store RTT measurements
             */
            class RttHistory
            {
            public:
                RttHistory(uint32_t seq, uint32_t size, time::steady_clock::TimePoint time);
                RttHistory(const RttHistory &h); // Copy constructor
            public:
                uint32_t seq;                       // First sequence number in packet sent
                uint32_t count;                     // Number of bytes sent
                time::steady_clock::TimePoint time; // Time this one was sent
                bool retx;                          // True if this has been retransmitted
            };

            typedef std::deque<RttHistory> RttHistory_t;

            /**
             * \ingroup tcp
             *
             * \brief Base class for all RTT Estimators
             */
            class RttEstimator
            {
            public:
                RttEstimator();
                RttEstimator(const RttEstimator &);

                virtual ~RttEstimator();

                /**
                 * \brief Note that a particular sequence has been sent
                 * \param seq the packet sequence number.
                 * \param size the packet size.
                 */
                virtual void
                SentSeq(uint32_t seq, uint32_t size);

                /**
                 * \brief Note that a particular ack sequence has been received
                 * \param ackSeq the ack sequence number.
                 * \return The measured RTT for this ack.
                 */
                virtual time::steady_clock::duration
                AckSeq(uint32_t ackSeq);

                /**
                 * \brief Clear all history entries
                 */
                virtual void
                ClearSent();

                /**
                 * \brief Add a new measurement to the estimator. Pure virtual function.
                 * \param t the new RTT measure.
                 */
                virtual void
                Measurement(time::steady_clock::duration t) = 0;

                /**
                 * \brief Returns the estimated RTO. Pure virtual function.
                 * \return the estimated RTO.
                 */
                virtual time::steady_clock::duration
                RetransmitTimeout() = 0;

                /**
                 * \brief Increase the estimation multiplier up to MaxMultiplier.
                 */
                virtual void
                IncreaseMultiplier();

                /**
                 * \brief Resets the estimation multiplier to 1.
                 */
                virtual void
                ResetMultiplier();

                /**
                 * \brief Resets the estimation to its initial state.
                 */
                virtual void
                Reset();

                /**
                 * \brief Sets the Minimum RTO.
                 * \param minRto The minimum RTO returned by the estimator.
                 */
                void
                SetMinRto(time::steady_clock::duration minRto);

                /**
                 * \brief Get the Minimum RTO.
                 * \return The minimum RTO returned by the estimator.
                 */
                time::steady_clock::duration
                GetMinRto(void) const;

                /**
                 * \brief Sets the Maximum RTO.
                 * \param minRto The maximum RTO returned by the estimator.
                 */
                void
                SetMaxRto(time::steady_clock::duration maxRto);

                /**
                 * \brief Get the Maximum RTO.
                 * \return The maximum RTO returned by the estimator.
                 */
                time::steady_clock::duration
                GetMaxRto(void) const;

                /**
                 * \brief Sets the current RTT estimate (forcefully).
                 * \param estimate The current RTT estimate.
                 */
                void
                SetCurrentEstimate(time::steady_clock::duration estimate);

                /**
                 * \brief gets the current RTT estimate.
                 * \return The current RTT estimate.
                 */
                time::steady_clock::duration
                GetCurrentEstimate(void) const;

                double
                duration_to_second_double(time::steady_clock::duration d) {
                    return d.count() / 1000000000.0;
                }

                time::steady_clock::duration
                second_double_to_duration(double d) {
                    return time::steady_clock::duration(static_cast<long long>(d * 1000000000.0));
                }

            private:
                uint32_t m_next; // Next expected sequence to be sent
                uint16_t m_maxMultiplier;
                time::steady_clock::duration m_initialEstimatedRtt;

            protected:
                time::steady_clock::duration m_currentEstimatedRtt; // Current estimate
                time::steady_clock::duration m_minRto;              // minimum value of the timeout
                time::steady_clock::duration m_maxRto;              // maximum value of the timeout
                uint32_t m_nSamples;                     // Number of samples
                uint16_t m_multiplier;                   // RTO Multiplier
                RttHistory_t m_history;                  // List of sent packet
            };
        }
    }
}
#endif /* RTT_ESTIMATOR_H */