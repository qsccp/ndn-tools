#include "window-consumer.hpp"

namespace ndn
{
    namespace cc
    {
        namespace client
        {
            class PconConsumer : public WindowConsumer
            {
            public:
                explicit PconConsumer(Face &face, const Options &options);

                virtual void onData(const Data &data, uint32_t seq, const time::steady_clock::TimePoint &sendTime);

                virtual void onTimeout(uint32_t seq);

            private:
                void
                WindowIncrease();

                void
                WindowDecrease();

                void
                CubicIncrease();

                void
                CubicDecrease();

                void
                BicIncrease();

                void
                BicDecrease();

            private:
                CcAlgorithm m_ccAlgorithm;
                double m_beta;
                double m_addRttSuppress;
                bool m_reactToCongestionMarks;
                bool m_useCwa;

                double m_ssthresh;
                uint32_t m_highData;
                double m_recPoint;

                // TCP CUBIC Parameters //
                static constexpr double CUBIC_C = 0.4;
                bool m_useCubicFastConv;
                double m_cubicBeta;

                double m_cubicWmax;
                double m_cubicLastWmax;
                time::steady_clock::TimePoint m_cubicLastDecrease;

                // TCP BIC Parameters //
                //! Regular TCP behavior (including slow start) until this window size
                static constexpr uint32_t BIC_LOW_WINDOW = 14;

                //! Sets the maximum (linear) increase of TCP BIC. Should be between 8 and 64.
                static constexpr uint32_t BIC_MAX_INCREMENT = 16;

                // BIC variables:
                double m_bicMinWin; //!< last minimum cwnd
                double m_bicMaxWin; //!< last maximum cwnd
                double m_bicTargetWin;
                double m_bicSsCwnd;
                double m_bicSsTarget;
                bool m_isBicSs; //!< whether we are currently in the BIC slow start phase
            };
        }
    }
}