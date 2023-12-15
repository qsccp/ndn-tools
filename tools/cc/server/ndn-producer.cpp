#include "ndn-producer.hpp"
#include <ndn-cxx/signature.hpp>
#include <iostream>

namespace ndn
{
    namespace cc
    {
        namespace server
        {
            Producer::Producer(Face &face, KeyChain &keyChain, const Options &options)
                : m_face(face),
                  m_keyChain(keyChain),
                  m_options(options),
                  m_isRunning(false),
                  m_signature(0)
            {
                auto b = make_shared<Buffer>();
                b->assign(m_options.payloadSize, 'a');
                m_payload = Block(tlv::Content, std::move(b));
            }

            void Producer::run()
            {
                m_face.processEvents();
            }

            void Producer::start()
            {
                if (m_isRunning)
                {
                    return;
                }

                m_isRunning = true;

                m_face.setInterestFilter(m_options.prefix,
                                         bind(&Producer::onInterest, this, _2),
                                         [](const auto &, const auto &reason)
                                         {
                                             NDN_THROW(std::runtime_error("Failed to register prefix: " + reason));
                                         });
            }

            void Producer::stop()
            {
                if (!m_isRunning)
                {
                    return;
                }

                m_isRunning = false;

                m_face.setInterestFilter(m_options.prefix, nullptr);
            }

            void Producer::onInterest(const Interest &interest)
            {
                afterReceive(interest.getName());

                Data data(interest.getName());
                if (interest.getServiceClass())
                {
                    int serviceClass = *interest.getServiceClass();
                    data.setServiceClass(serviceClass);
                }
                // 设置 targetRate 为最大32位整数
                data.setTargetRate(std::numeric_limits<uint32_t>::max());
                data.setFreshnessPeriod(m_options.freshnessPeriod);
                data.setContent(m_payload);

                Signature signature;
                SignatureInfo signatureInfo(static_cast<::ndn::tlv::SignatureTypeValue>(255));
                signature.setInfo(signatureInfo);
                signature.setValue(::ndn::makeNonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));
                data.setSignature(signature);
                data.wireEncode();

                // m_keyChain.sign(data);
                m_face.put(data);
                std::cout << "onInterest: " << interest.getName() << std::endl;
            }
        }
    }
}