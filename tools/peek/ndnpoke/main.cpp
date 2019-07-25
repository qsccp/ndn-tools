/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of ndn-tools (Named Data Networking Essential Tools).
 * See AUTHORS.md for complete list of ndn-tools authors and contributors.
 *
 * ndn-tools is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndn-tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndn-tools, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Jerald Paul Abraham <jeraldabraham@email.arizona.edu>
 */

#include "ndnpoke.hpp"
#include "core/version.hpp"

namespace ndn {
namespace peek {

namespace po = boost::program_options;

static void
usage(std::ostream& os, const std::string& program, const po::options_description& options)
{
  os << "Usage: " << program << " [options] /name\n"
     << "\n"
     << "Reads a payload from the standard input and sends it as a single Data packet.\n"
     << options;
}

static int
main(int argc, char* argv[])
{
  std::string progName(argv[0]);
  PokeOptions options;
  bool wantDigestSha256 = false;

  po::options_description visibleOptDesc;
  visibleOptDesc.add_options()
    ("help,h",      "print help and exit")
    ("force,f",     po::bool_switch(&options.wantForceData),
                    "send the Data packet without waiting for an incoming Interest")
    ("final,F",     po::bool_switch(&options.wantFinalBlockId),
                    "set FinalBlockId to the last component of the Data name")
    ("freshness,x", po::value<int>(), "set FreshnessPeriod (in milliseconds)")
    ("identity,i",  po::value<std::string>(), "use the specified identity for signing")
    ("digest,D",    po::bool_switch(&wantDigestSha256),
                    "use DigestSha256 signing method instead of SignatureSha256WithRsa")
    ("timeout,w",   po::value<int>(), "set timeout (in milliseconds)")
    ("version,V",   "print version and exit")
  ;

  po::options_description hiddenOptDesc;
  hiddenOptDesc.add_options()
    ("name", po::value<std::string>(), "Data name");

  po::options_description optDesc;
  optDesc.add(visibleOptDesc).add(hiddenOptDesc);

  po::positional_options_description optPos;
  optPos.add("name", -1);

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(optDesc).positional(optPos).run(), vm);
    po::notify(vm);
  }
  catch (const po::error& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 2;
  }

  if (vm.count("help") > 0) {
    usage(std::cout, progName, visibleOptDesc);
    return 0;
  }

  if (vm.count("version") > 0) {
    std::cout << "ndnpoke " << tools::VERSION << std::endl;
    return 0;
  }

  if (vm.count("name") == 0) {
    std::cerr << "ERROR: missing name\n\n";
    usage(std::cerr, progName, visibleOptDesc);
    return 2;
  }

  try {
    options.name = vm["name"].as<std::string>();
  }
  catch (const Name::Error& e) {
    std::cerr << "ERROR: invalid name: " << e.what() << std::endl;
    return 2;
  }

  if (options.name.empty()) {
    std::cerr << "ERROR: name cannot have zero components" << std::endl;
    return 2;
  }

  if (vm.count("freshness") > 0) {
    if (vm["freshness"].as<int>() < 0) {
      std::cerr << "ERROR: freshness cannot be negative" << std::endl;
      return 2;
    }
    options.freshnessPeriod = time::milliseconds(vm["freshness"].as<int>());
  }

  if (vm.count("identity") > 0) {
    if (wantDigestSha256) {
      std::cerr << "ERROR: conflicting '--digest' and '--identity' options specified" << std::endl;
      return 2;
    }
    try {
      options.signingInfo.setSigningIdentity(vm["identity"].as<std::string>());
    }
    catch (const Name::Error& e) {
      std::cerr << "ERROR: invalid identity name: " << e.what() << std::endl;
      return 2;
    }
  }

  if (wantDigestSha256) {
    options.signingInfo.setSha256Signing();
  }

  if (vm.count("timeout") > 0) {
    if (options.wantForceData) {
      std::cerr << "ERROR: conflicting '--force' and '--timeout' options specified" << std::endl;
      return 2;
    }
    if (vm["timeout"].as<int>() < 0) {
      std::cerr << "ERROR: timeout cannot be negative" << std::endl;
      return 2;
    }
    options.timeout = time::milliseconds(vm["timeout"].as<int>());
  }

  try {
    Face face;
    KeyChain keyChain;
    NdnPoke program(face, keyChain, std::cin, options);

    program.start();
    face.processEvents();

    return program.didSendData() ? 0 : 1;
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
  }
}

} // namespace peek
} // namespace ndn

int
main(int argc, char* argv[])
{
  return ndn::peek::main(argc, argv);
}
