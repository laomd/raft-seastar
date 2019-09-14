/*
Tencent is pleased to support the open source community by making
PhxPaxos available.
Copyright (C) 2016 THL A29 Limited, a Tencent company.
All rights reserved.

Licensed under the BSD 3-Clause License (the "License"); you may
not use this file except in compliance with the License. You may
obtain a copy of the License at

https://opensource.org/licenses/BSD-3-Clause

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing
permissions and limitations under the License.

See the AUTHORS file for names of contributors.
*/

#include "echo_server.h"
#include <iostream>
#include <phxpaxos/options.h>
#include <seastar/core/app-template.hh>
#include <seastar/core/reactor.hh>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <lao_utils/logger.hh>

using namespace phxecho;
using namespace phxpaxos;
using namespace std;

int parse_ipport(const char *pcStr, NodeInfo &oNodeInfo) {
  char sIP[32] = {0};
  int iPort = -1;

  int count = sscanf(pcStr, "%[^':']:%d", sIP, &iPort);
  if (count != 2) {
    return -1;
  }

  oNodeInfo.SetIPPort(sIP, iPort);

  return 0;
}

int parse_ipport_list(const char *pcStr, NodeInfoList &vecNodeInfoList) {
  string sTmpStr;
  int iStrLen = strlen(pcStr);

  for (int i = 0; i < iStrLen; i++) {
    if (pcStr[i] == ',' || i == iStrLen - 1) {
      if (i == iStrLen - 1 && pcStr[i] != ',') {
        sTmpStr += pcStr[i];
      }

      NodeInfo oNodeInfo;
      int ret = parse_ipport(sTmpStr.c_str(), oNodeInfo);
      if (ret != 0) {
        return ret;
      }

      vecNodeInfoList.push_back(oNodeInfo);

      sTmpStr = "";
    } else {
      sTmpStr += pcStr[i];
    }
  }

  return 0;
}

int main(int argc, char **argv) {
  seastar::app_template::config cf;
  cf.auto_handle_sigint_sigterm = false;
  seastar::app_template app(cf);
  namespace bpo = boost::program_options;
  app.add_options()
    ("me", bpo::value<std::string>())
    ("nodelist", bpo::value<std::string>());
  app.run(argc, argv, [&app] {
    auto &args = app.configuration();
    NodeInfo oMyNode;
    NodeInfoList vecNodeInfoList;
    if (args.count("me") && args.count("nodelist")) {
      parse_ipport(args["me"].as<std::string>().c_str(), oMyNode);
      parse_ipport_list(args["nodelist"].as<std::string>().c_str(),
                        vecNodeInfoList);
    } else {
      return seastar::make_exception_future(
          std::runtime_error("usage: ./phxecho --me=xx --nodelist=xx,xx,xx"));
    }

    auto oEchoServer = make_shared<PhxEchoServer>(oMyNode, vecNodeInfoList);
    if (oEchoServer->RunPaxos() != 0) {
      return seastar::make_exception_future(
          std::runtime_error("failed to run paxos."));
    }
    LOG(INFO) << "echo server start, ip: " << oMyNode.GetIP() << " port: " << oMyNode.GetPort();
    return seastar::keep_doing([oEchoServer] {
      string sEchoReqValue;
      printf("\nplease input: <echo req value>\n");
      getline(cin, sEchoReqValue);
      string sEchoRespValue;
      if (oEchoServer->Echo(sEchoReqValue, sEchoRespValue) != 0) {
        LOG(ERROR) << "Echo fail";
      } else {
        LOG(INFO) << "echo resp value " << sEchoRespValue;
      }
      return seastar::make_ready_future();
    });
  });
  return 0;
}
