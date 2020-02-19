#pragma once

#include <arpa/inet.h>
#include <cstdlib>
#include <memory>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

inline uint16_t getAvailableListenPort() {
  // 1. create a socket
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  // 2. create a sockaddr，and set its port to 0
  struct sockaddr_in addrto;
  memset(&addrto, 0, sizeof(struct sockaddr_in));
  addrto.sin_family = AF_INET;
  addrto.sin_addr.s_addr = inet_addr("0.0.0.0");
  addrto.sin_port = 0;

  // 3. bind
  int ret =
      ::bind(sock, (struct sockaddr *)&(addrto), sizeof(struct sockaddr_in));
  if (0 != ret) {
    return 0;
  }

  // 4. getsockname
  struct sockaddr_in connAddr;
  memset(&connAddr, 0, sizeof(struct sockaddr_in));
  unsigned int len = sizeof(connAddr);
  ret = ::getsockname(sock, (sockaddr *)&connAddr, &len);

  if (0 != ret) {
    return 0;
  }

  uint16_t port = ntohs(connAddr.sin_port); // get port
  if (0 != close(sock)) {
    return 0;
  }
  return port;
}