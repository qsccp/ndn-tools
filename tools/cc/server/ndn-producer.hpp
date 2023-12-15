#include "core/common.hpp"

namespace ndn
{
    namespace cc
    {
        namespace server
        {
            /**
             * @brief Options for NDN producer
             */
            struct Options
            {
                Name prefix;                              //!< prefix to register
                time::milliseconds freshnessPeriod = 4_s; //!< data freshness period
                uint32_t payloadSize = 0;                 //!< response payload size (0 == no payload)
            };

            class Producer : noncopyable
            {
            public:
                Producer(Face &face, KeyChain &keyChain, const Options &options);

                /**
                 * @brief Signals when Interest received
                 *
                 * @param name incoming interest name
                 */
                signal::Signal<Producer, Name> afterReceive;

                /**
                 * @brief Signals when finished pinging
                 */
                signal::Signal<Producer> afterFinish;

                void
                run();

                /**
                 * @brief starts the Interest filter
                 *
                 * @note This method is non-blocking and caller need to call face.processEvents()
                 */
                void
                start();

                /**
                 * @brief Unregister set interest filter
                 */
                void
                stop();

            public:
                /**
                 * @brief Called when interest received
                 *
                 * @param interest incoming interest
                 */
                void
                onInterest(const Interest &interest);

            private:
                const Options &m_options;
                Face &m_face;
                KeyChain &m_keyChain;
                Block m_payload;
                bool m_isRunning;
                uint32_t m_signature;
                RegisteredPrefixHandle m_registeredPrefix;
            };
        }
    }
}