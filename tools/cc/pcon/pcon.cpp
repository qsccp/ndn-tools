#include "core/common.hpp"
#include "core/version.hpp"
#include "pcon-consumer.hpp"
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
                PconConsumer m_consumer;
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
                    "startSeq", po::value<uint32_t>(&options.startSeq)->default_value(0), "start sequence number");
                visibleOptDesc.add_options()(
                    "seqMax", po::value<int64_t>(&options.seqMax)->default_value(-1), "maximum sequence number");
                visibleOptDesc.add_options()(
                    "dsz", po::value<uint32_t>(&options.dsz)->default_value(8624), "data size");
                visibleOptDesc.add_options()(
                    "delayStart", po::value<uint32_t>(&options.delayStart)->default_value(0), "delay start time, in milliseconds");
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
                visibleOptDesc.add_options()(
                    "schema", po::value<std::string>(&options.schema)->default_value("QSCCP"), "Schema, QSCCP or PCON");
                visibleOptDesc.add_options()(
                    "initialWindowSize", po::value<uint32_t>(&options.initialWindowSize)->default_value(1), "Initial Window Size");
                visibleOptDesc.add_options()(
                    "setInitialWindowOnTimeout", po::value<bool>(&options.setInitialWindowOnTimeout)->default_value(false), "Set initial window size on timeout");
                visibleOptDesc.add_options()(
                    "ccAlgorithm", po::value<std::string>()->default_value("BIC"), "Specify which window adaptation algorithm to use (AIMD, BIC, or CUBIC)");
                visibleOptDesc.add_options()(
                    "beta", po::value<double>(&options.beta)->default_value(0.5), "TCP Multiplicative Decrease factor");
                visibleOptDesc.add_options()(
                    "cubicBeta", po::value<double>(&options.cubicBeta)->default_value(0.8), "TCP CUBIC Multiplicative Decrease factor");
                visibleOptDesc.add_options()(
                    "addRttSuppress", po::value<double>(&options.addRttSuppress)->default_value(0.5), "Minimum number of RTTs (1 + this factor) between window decreases");
                visibleOptDesc.add_options()(
                    "reactToCongestionMarks", po::value<bool>(&options.reactToCongestionMarks)->default_value(false), "React to congestion marks (ECN, CE)");
                visibleOptDesc.add_options()(
                    "useCwa", po::value<bool>(&options.useCwa)->default_value(false), "Use Congestion Window Acceleration (CWA) algorithm");
                visibleOptDesc.add_options()(
                    "useCubicFastConv", po::value<bool>(&options.useCubicFastConv)->default_value(true), "Use CUBIC fast convergence algorithm");

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

                    if (optVm.count("ccAlgorithm") > 0)
                    {
                        std::string algorithm_str = optVm["ccAlgorithm"].as<std::string>();
                        if (algorithm_str == "AIMD")
                        {
                            options.ccAlgorithm = CcAlgorithm::AIMD;
                        }
                        else if (algorithm_str == "BIC")
                        {
                            options.ccAlgorithm = CcAlgorithm::BIC;
                        }
                        else if (algorithm_str == "CUBIC")
                        {
                            options.ccAlgorithm = CcAlgorithm::CUBIC;
                        }
                        else
                        {
                            std::cerr << "ERROR: Not support CC Algorithm: " << algorithm_str << std::endl;
                        }
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