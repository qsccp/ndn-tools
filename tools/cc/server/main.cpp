#include "ndn-producer.hpp"
#include "core/common.hpp"
#include "core/version.hpp"

namespace ndn
{
    namespace cc
    {
        namespace server
        {

            namespace po = boost::program_options;

            class Runner : noncopyable
            {
            public:
                explicit Runner(const Options &options)
                    : m_options(options),
                    m_producer(m_face, m_keyChain, options)
                {
                    m_producer.afterFinish.connect([this]
                                                   {
                        m_producer.stop();
                        });

                }

                int
                run()
                {
                    std::cout << "Producer SERVER " << m_options.prefix << std::endl;

                    try
                    {
                        // Interest interest(Name("/A/1"));
                        // interest.setCanBePrefix(false);
                        // interest.setMustBeFresh(true);
                        // interest.setServiceClass(0);
                        // m_producer.onInterest(interest);
                        m_producer.start();
                        m_face.processEvents();
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "ERROR: " << e.what() << std::endl;
                        return 1;
                    }

                    return 0;
                }

            private:
                const Options &m_options;

                Face m_face;
                KeyChain m_keyChain;
                Producer m_producer;
            };

            static void
            usage(std::ostream &os, const std::string &programName, const po::options_description &options)
            {
                os << "Usage: " << programName << " [options] <prefix>\n"
                   << "\n"
                   << "Starts a NDN ping server that responds to Interests under name ndn:<prefix>/ping\n"
                   << "\n"
                   << options;
            }

            static int
            main(int argc, char *argv[])
            {
                Options options;
                std::string prefix;

                po::options_description visibleDesc("Options");
                visibleDesc.add_options()("help,h", "print this help message and exit");
                visibleDesc.add_options()("freshness,f",
                                          po::value<time::milliseconds::rep>()->default_value(4000),
                                          "FreshnessPeriod of the ping response, in milliseconds");
                visibleDesc.add_options()("size,s",
                                          po::value<uint32_t>()->default_value(1024),
                                          "size of response payload");
                visibleDesc.add_options()("version,V", "print program version and exit");

                po::options_description hiddenDesc;
                hiddenDesc.add_options()("prefix", po::value<std::string>(&prefix));

                po::options_description optDesc;
                optDesc.add(visibleDesc).add(hiddenDesc);

                po::positional_options_description posDesc;
                posDesc.add("prefix", -1);

                po::variables_map vm;
                try
                {
                    po::store(po::command_line_parser(argc, argv).options(optDesc).positional(posDesc).run(), vm);
                    po::notify(vm);
                }
                catch (const po::error &e)
                {
                    std::cerr << "ERROR: " << e.what() << "\n\n";
                    usage(std::cerr, argv[0], visibleDesc);
                    return 2;
                }
                catch (const boost::bad_any_cast &e)
                {
                    std::cerr << "ERROR: " << e.what() << "\n\n";
                    usage(std::cerr, argv[0], visibleDesc);
                    return 2;
                }

                if (vm.count("help") > 0)
                {
                    usage(std::cout, argv[0], visibleDesc);
                    return 0;
                }

                if (vm.count("version") > 0)
                {
                    std::cout << "ndnpingserver " << tools::VERSION << std::endl;
                    return 0;
                }

                if (prefix.empty())
                {
                    std::cerr << "ERROR: no name prefix specified\n\n";
                    usage(std::cerr, argv[0], visibleDesc);
                    return 2;
                }
                options.prefix = Name(prefix);

                options.freshnessPeriod = time::milliseconds(vm["freshness"].as<time::milliseconds::rep>());
                if (vm.count("_deprecated_freshness_") > 0)
                {
                    options.freshnessPeriod = time::milliseconds(vm["_deprecated_freshness_"].as<time::milliseconds::rep>());
                }
                if (options.freshnessPeriod < 0_ms)
                {
                    std::cerr << "ERROR: FreshnessPeriod cannot be negative" << std::endl;
                    return 2;
                }

                options.payloadSize = vm["size"].as<uint32_t>();

                return Runner(options).run();
            }

        } // namespace server
    }     // namespace ping
} // namespace ndn

int main(int argc, char *argv[])
{
    return ndn::cc::server::main(argc, argv);
}
