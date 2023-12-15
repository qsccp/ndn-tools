#ifndef NDN_RTT_MEAN_DEVIATION_H
#define NDN_RTT_MEAN_DEVIATION_H

#include "ndn-rtt-estimator.hpp"

#endif /* NDN_RTT_MEAN_DEVIATION_H */
namespace ndn
{
    namespace cc
    {
        namespace client
        {

            /**
             * \ingroup ndn-apps
             *
             * \brief The modified version of "Mean--Deviation" RTT estimator, as discussed by Van Jacobson that
             *better suits NDN communication model
             *
             * This class implements the "Mean--Deviation" RTT estimator, as discussed
             * by Van Jacobson and Michael J. Karels, in
             * "Congestion Avoidance and Control", SIGCOMM 88, Appendix A
             *
             */
            class RttMeanDeviation : public RttEstimator
            {
            public:
                RttMeanDeviation();

                RttMeanDeviation(const RttMeanDeviation &);

                void
                SentSeq(uint32_t seq, uint32_t size);

                time::steady_clock::duration
                AckSeq(uint32_t ackSeq);

                void
                Measurement(time::steady_clock::duration measure);

                time::steady_clock::duration
                RetransmitTimeout();

                void
                Reset();

                void
                Gain(double g);

            private:
                double m_gain;                           // Filter gain
                double m_gain2;                          // Filter gain
                time::steady_clock::duration m_variance; // Current variance
            };
        }
    }
}