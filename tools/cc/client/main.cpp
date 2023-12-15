#include "core/common.hpp"
#include "core/version.hpp"
#include "qsccp-consumer.hpp"
#include <iostream>

namespace ndn
{
    namespace cc
    {
        namespace client
        {
            class Runner : noncopyable
            {
            public:
                explicit Runner(const Options &options) : m_consumer(m_face, options)
                {
                    m_consumer.afterFinish.connect([this]
                                                   { this->cancel(); });
                }

                int
                run()
                {
                    try
                    {
                        m_consumer.start();
                        m_face.processEvents();
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "ERROR: " << e.what() << std::endl;
                        // m_tracer.onError(e.what());
                        return 2;
                    }
                    return 0;
                }

            private:
                void
                cancel()
                {
                    m_consumer.stop();
                }

            private:
                Face m_face;
                QsccpConsumer m_consumer;
            };

            static void
            usage(const boost::program_options::options_description &options)
            {
                std::cout << "Usage: ndnping [options] ndn:/name/prefix\n"
                             "\n"
                             "Ping a NDN name prefix using Interests with name ndn:/name/prefix/ping/number.\n"
                             "The numbers in the Interests are randomly generated unless specified.\n"
                             "\n"
                          << options;
                exit(2);
            }

            static int
            main(int argc, char *argv[])
            {
                Options options;
                // options.shouldAllowStaleData = false;
                // options.nPings = -1;
                // options.interval = time::milliseconds(getDefaultPingInterval());
                // options.timeout = time::milliseconds(getDefaultPingTimeoutThreshold());
                // options.startSeq = 0;
                // options.shouldGenerateRandomSeq = true;
                // options.shouldPrintTimestamp = false;

                // std::string identifier;

                namespace po = boost::program_options;

                po::options_description visibleOptDesc("Options");
                visibleOptDesc.add_options()("help,h", "print this message and exit")("version,V", "display version and exit");
                visibleOptDesc.add_options()(
                    "startSeq", po::value<uint64_t>(&options.startSeq)->default_value(0), "start sequence number");
                visibleOptDesc.add_options()(
                    "seqMax", po::value<int64_t>(&options.seqMax)->default_value(-1), "maximum sequence number");
                visibleOptDesc.add_options()(
                    "tos", po::value<uint32_t>(&options.tos)->default_value(5), "set the TOS field");
                visibleOptDesc.add_options()(
                    "dsz", po::value<uint32_t>(&options.dsz)->default_value(8624), "data size");
                visibleOptDesc.add_options()(
                    "delayStart", po::value<uint32_t>(&options.delayStart)->default_value(0), "delay start time, in milliseconds");
                visibleOptDesc.add_options()(
                    "initialSendRate", po::value<uint32_t>(&options.initialSendRate)->default_value(0), "initial send rate, in milliseconds");
                visibleOptDesc.add_options()(
                    "fixedRate", po::value<int32_t>(&options.fixedRate)->default_value(-1), "fixed rate, in milliseconds");
                visibleOptDesc.add_options()(
                    "timingStop", po::value<int32_t>(&options.timingStop)->default_value(-1), "timing stop, in milliseconds");
                visibleOptDesc.add_options()(
                    "delayGreedy", po::value<int32_t>(&options.delayGreedy)->default_value(-1), "delay greedy, in milliseconds");
                visibleOptDesc.add_options()(
                    "greedyRate", po::value<int32_t>(&options.greedyRate)->default_value(-1), "greedy rate, in milliseconds");
                visibleOptDesc.add_options()(
                    "lifetime", po::value<time::milliseconds::rep>()->default_value(4000), "Interest lifetime, in milliseconds");

                po::options_description hiddenOptDesc;
                hiddenOptDesc.add_options()("prefix", po::value<std::string>(), "content prefix to request");

                po::options_description optDesc;
                optDesc.add(visibleOptDesc).add(hiddenOptDesc);

                po::positional_options_description optPos;
                optPos.add("prefix", -1);

                try
                {
                    po::variables_map optVm;
                    po::store(po::command_line_parser(argc, argv).options(optDesc).positional(optPos).run(), optVm);
                    po::notify(optVm);

                    if (optVm.count("help") > 0)
                    {
                        usage(visibleOptDesc);
                    }

                    if (optVm.count("version") > 0)
                    {
                        std::cout << "qsccp-client " << tools::VERSION << std::endl;
                        exit(0);
                    }

                    if (optVm.count("prefix") > 0)
                    {
                        options.prefix = Name(optVm["prefix"].as<std::string>());
                    }
                    else
                    {
                        std::cerr << "ERROR: No prefix specified" << std::endl;
                        usage(visibleOptDesc);
                    }

                    options.lifetime = time::milliseconds(optVm["lifetime"].as<time::milliseconds::rep>());
                }
                catch (const po::error &e)
                {
                    std::cerr << "ERROR: " << e.what() << std::endl;
                    usage(visibleOptDesc);
                }

                std::cout << "PING " << options.prefix << std::endl;
                return Runner(options).run();
            }
        }
    }
}

int main(int argc, char *argv[])
{
    return ndn::cc::client::main(argc, argv);
}