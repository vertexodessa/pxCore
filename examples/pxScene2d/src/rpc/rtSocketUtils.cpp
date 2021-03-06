#include "rtSocketUtils.h"

#include <cstdio>
#include <sstream>

#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>

#include <rtLog.h>

#include <rapidjson/memorystream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#ifndef RT_REMOTE_LOOPBACK_ONLY
static rtError
rtFindFirstInetInterface(char* name, size_t len)
{
  rtError e = RT_FAIL;
  ifaddrs* ifaddr = NULL;
  int ret = getifaddrs(&ifaddr);
  if (ret == -1)
  {
    e = rtErrorFromErrno(errno);
    rtLogError("failed to get list of interfaces: %s", rtStrError(e));
    return e;
  }

  for (ifaddrs* i = ifaddr; i != nullptr; i = i->ifa_next)
  {
    if (i->ifa_addr == nullptr)
      continue;
    if (i->ifa_addr->sa_family != AF_INET && i->ifa_addr->sa_family != AF_INET6)
      continue;
    if (strcmp(i->ifa_name, "lo") == 0)
      continue;

    strncpy(name, i->ifa_name, len);
    e = RT_OK;
    break;
  }

  if (ifaddr)
    freeifaddrs(ifaddr);

  return e;
}
#endif


rtError
rtParseAddress(sockaddr_storage& ss, char const* addr, uint16_t port, uint32_t* index)
{
  int ret = 0;

  if (index != nullptr)
    *index = -1;

  sockaddr_in* v4 = reinterpret_cast<sockaddr_in *>(&ss);
  ret = inet_pton(AF_INET, addr, &v4->sin_addr);

  if (ret == 1)
  {
    #ifndef __linux__
    v4->sin_len = sizeof(sockaddr_in);
    #endif

    v4->sin_family = AF_INET;
    v4->sin_port = htons(port);
    ss.ss_family = AF_INET;
    return RT_OK;
  }
  else if (ret == 0)
  {
    sockaddr_in6* v6 = reinterpret_cast<sockaddr_in6 *>(&ss);
    ret = inet_pton(AF_INET6, addr, &v6->sin6_addr);
    if (ret == 0)
    {
      // try hostname
      rtError err = rtGetInterfaceAddress(addr, ss);
      if (err != RT_OK)
        return err;

      if (index != nullptr)
        *index = if_nametoindex(addr);

      if (ss.ss_family == AF_INET)
      {
        v4->sin_family = AF_INET;
        v4->sin_port = htons(port);
      }
      if (ss.ss_family == AF_INET6)
      {
        v6->sin6_family = AF_INET6;
        v6->sin6_port = htons(port);
      }
    }
    else if (ret == 1)
    {
      // it's a numeric address
      v6->sin6_family = AF_INET6;
      v6->sin6_port = htons(port);
    }
  }
  else
  {
    return rtErrorFromErrno(errno);
  }
  return RT_OK;
}

rtError
rtSocketGetLength(sockaddr_storage const& ss, socklen_t* len)
{
  if (!len)
    return RT_ERROR_INVALID_ARG;

  if (ss.ss_family == AF_INET)
    *len = sizeof(sockaddr_in);
  else if (ss.ss_family == AF_INET6)
    *len = sizeof(sockaddr_in6);
  else
    *len = 0;

  return RT_OK;
}

rtError
rtGetInterfaceAddress(char const* name, sockaddr_storage& ss)
{
  rtError error = RT_FAIL;
  ifaddrs* ifaddr = NULL;

  int ret = getifaddrs(&ifaddr);

  if (ret == -1)
  {
    error = rtErrorFromErrno(errno);
    rtLogError("failed to get list of interfaces. %s", rtStrError(error));
    return error;
  }

  for (ifaddrs* i = ifaddr; i != NULL; i = i->ifa_next)
  {
    if (i->ifa_addr == NULL)
      continue;

    if (strcmp(name, i->ifa_name) != 0)
      continue;

    if (i->ifa_addr->sa_family != AF_INET && i->ifa_addr->sa_family != AF_INET6)
      continue;

    ss.ss_family = i->ifa_addr->sa_family;

    socklen_t len;
    rtSocketGetLength(ss, &len);

    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];

    ret = getnameinfo(i->ifa_addr, len, host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST);
    if (ret != 0)
    {
      // TODO: add error class for gai errors
      rtLogError("failed to get address for %s. %s", name, gai_strerror(ret));
      error = RT_FAIL;
      goto out;
    }
    else
    {
      void* addr = NULL;
      rtGetInetAddr(ss, &addr);

      ret = inet_pton(ss.ss_family, host, addr);
      if (ret != 1)
      {
        int err = errno;
        rtLogError("failed to parse: %s as valid ipv4 address", host);
        error = rtErrorFromErrno(err);
      }
      error = RT_OK;
      goto out;
    }
  }

out:
  if (ifaddr)
    freeifaddrs(ifaddr);

  return error;
}

rtError
rtGetInetAddr(sockaddr_storage const& ss, void** addr)
{
  sockaddr_in const* v4 = reinterpret_cast<sockaddr_in const*>(&ss);
  sockaddr_in6 const* v6 = reinterpret_cast<sockaddr_in6 const*>(&ss);

  void const* p = (ss.ss_family == AF_INET)
    ? reinterpret_cast<void const *>(&(v4->sin_addr))
    : reinterpret_cast<void const *>(&(v6->sin6_addr));

  *addr = const_cast<void *>(p);

  return RT_OK;
}

rtError
rtGetPort(sockaddr_storage const& ss, uint16_t* port)
{
  sockaddr_in const* v4 = reinterpret_cast<sockaddr_in const *>(&ss);
  sockaddr_in6 const* v6 = reinterpret_cast<sockaddr_in6 const *>(&ss);
  *port = ntohs((ss.ss_family == AF_INET) ? v4->sin_port : v6->sin6_port);
  return RT_OK;
}

rtError
rtPushFd(fd_set* fds, int fd, int* max_fd)
{
  if (fd != -1)
  {
    FD_SET(fd, fds);
    if (max_fd && fd > *max_fd)
      *max_fd = fd;
  }
  return RT_OK;
}

rtError
rtReadUntil(int fd, char* buff, int n)
{
  ssize_t bytes_read = 0;
  ssize_t bytes_to_read = n;

  while (bytes_read < bytes_to_read)
  {
    ssize_t n = read(fd, buff + bytes_read, (bytes_to_read - bytes_read));
    if (n == 0)
    {
      rtLogWarn("tring to read from file descriptor: %d. It looks closed", fd);
      return RT_FAIL;
    }

    if (n == -1)
    {
      rtError e = rtErrorFromErrno(errno);
      rtLogError("failed to read from fd %d. %s", fd, rtStrError(e));
      return e;
    }

    bytes_read += n;
  }
  return RT_OK;
}

std::string
rtSocketToString(sockaddr_storage const& ss)
{
  // TODO make this more effecient
  void* addr = NULL;
  rtGetInetAddr(ss, &addr);

  uint16_t port;
  rtGetPort(ss, &port);

  char addr_buff[128];
  memset(addr_buff, 0, sizeof(addr_buff));
  inet_ntop(ss.ss_family, addr, addr_buff, sizeof(addr_buff));

  std::stringstream buff;
  buff << addr_buff;
  buff << ':';
  buff << port;
  return buff.str();
}

rtError
rtSendDocument(rapidjson::Document const& doc, int fd, sockaddr_storage const* dest)
{
  rapidjson::StringBuffer buff;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buff);
  doc.Accept(writer);

  #ifdef RT_RPC_DEBUG
  sockaddr_storage remote_endpoint;
  memset(&remote_endpoint, 0, sizeof(sockaddr_storage));
  if (dest)
    remote_endpoint = *dest;
  else
    rtGetPeerName(fd, remote_endpoint);

  char const* verb = (dest != NULL ? "sendto" : "send");
  rtLogDebug("%s [%d/%s] (%d):\n***OUT***\t\"%.*s\"\n",
    verb,
    fd,
    rtSocketToString(remote_endpoint).c_str(),
    static_cast<int>(buff.GetSize()),
    static_cast<int>(buff.GetSize()),
    buff.GetString());
  #endif

  if (dest)
  {
    socklen_t len;
    rtSocketGetLength(*dest, &len);

    int flags = 0;
    #ifndef __APPLE__
    flags = MSG_NOSIGNAL;
    #endif

    if (sendto(fd, buff.GetString(), buff.GetSize(), flags,
          reinterpret_cast<sockaddr const *>(dest), len) < 0)
    {
      rtError e = rtErrorFromErrno(errno);
      rtLogError("sendto failed. %s. dest:%s family:%d", rtStrError(e), rtSocketToString(*dest).c_str(),
        dest->ss_family);
      return e;
    }
  }
  else
  {
    // send length first
    int n = buff.GetSize();
    n = htonl(n);

    int flags = 0;
    #ifndef __APPLE__
    flags = MSG_NOSIGNAL;
    #endif
    if (send(fd, reinterpret_cast<char *>(&n), 4, flags) < 0)
    {
      rtError e = rtErrorFromErrno(errno);
      rtLogError("failed to send length of message. %s", rtStrError(e));
      return e;
    }

    if (send(fd, buff.GetString(), buff.GetSize(), flags) < 0)
    {
      rtError e = rtErrorFromErrno(errno);
      rtLogWarn("failed to send: %s", rtStrError(e));
      return e;
    }
  }

  return RT_OK;
}

rtError
rtReadMessage(int fd, rtSocketBuffer& buff, rtJsonDocPtr& doc)
{
  rtError err = RT_OK;

  int n = 0;
  int capacity = static_cast<int>(buff.capacity());

  err = rtReadUntil(fd, reinterpret_cast<char *>(&n), 4);
  if (err != RT_OK)
    return err;

  n = ntohl(n);

  if (n > capacity)
  {
    rtLogWarn("buffer capacity %d not big enough for message size: %d", capacity, n);
    // TODO: should drain, and discard message
    assert(false);
    return RT_FAIL;
  }

  buff.resize(n + 1);
  buff[n] = '\0';

  err = rtReadUntil(fd, &buff[0], n);
  if (err != RT_OK)
  {
    rtLogError("failed to read payload message of length %d from socket", n);
    return err;
  }
    
  #ifdef RT_RPC_DEBUG
  rtLogDebug("read (%d):\n***IN***\t\"%.*s\"\n", static_cast<int>(buff.size()), static_cast<int>(buff.size()), &buff[0]);
  #endif

  return rtParseMessage(&buff[0], n, doc);
}

rtError
rtParseMessage(char const* buff, int n, rtJsonDocPtr& doc)
{
  if (!buff)
    return RT_FAIL;

  doc.reset(new rapidjson::Document());

  rapidjson::MemoryStream stream(buff, n);
  if (doc->ParseStream<rapidjson::kParseDefaultFlags>(stream).HasParseError())
  {
    int begin = doc->GetErrorOffset() - 16;
    if (begin < 0)
      begin = 0;
    int end = begin + 64;
    if (end > n)
      end = n;
    int length = (end - begin);

    rtLogWarn("unparsable JSON read:%d offset:%d", doc->GetParseError(), (int) doc->GetErrorOffset());
    rtLogWarn("\"%.*s\"\n", length, buff + begin);

    return RT_FAIL;
  }
  
  return RT_OK;
}

std::string
rtStrError(int e)
{
  char buff[256];
  memset(buff, 0, sizeof(buff));

  char* s = strerror_r(e, buff, sizeof(buff));
  if (s)
    return std::string(s);

  std::snprintf(buff, sizeof(buff), "unknown error: %d", e);
  return std::string(buff);
}

rtError
rtGetPeerName(int fd, sockaddr_storage& endpoint)
{
  sockaddr_storage addr;
  memset(&addr, 0, sizeof(sockaddr_storage));

  socklen_t len;
  rtSocketGetLength(endpoint, &len);

  int ret = getpeername(fd, (sockaddr *)&addr, &len);
  if (ret == -1)
  {
    rtError err = rtErrorFromErrno(errno);
    rtLogWarn("failed to get the peername for fd:%d endpoint. %s", fd, rtStrError(err));
    return err;
  }

  memcpy(&endpoint, &addr, sizeof(sockaddr_storage));
  return RT_OK;
}

rtError
rtGetSockName(int fd, sockaddr_storage& endpoint)
{
  assert(fd > 2);

  sockaddr_storage addr;
  memset(&addr, 0, sizeof(sockaddr_storage));

  socklen_t len;
  rtSocketGetLength(endpoint, &len);

  int ret = getsockname(fd, (sockaddr *)&addr, &len);
  if (ret == -1)
  {
    rtError err = rtErrorFromErrno(errno);
    rtLogWarn("failed to get the socket name for fd:%d endpoint. %s", fd, rtStrError(err));
    return err;
  }

  memcpy(&endpoint, &addr, sizeof(sockaddr_storage));
  return RT_OK;
}

rtError
rtCloseSocket(int& fd)
{
  if (fd != kInvalidSocket)
  {
    ::close(fd);
    fd = kInvalidSocket;
  }
  return RT_OK;
}

rtError
rtGetDefaultInterface(sockaddr_storage& addr, uint16_t port)
{
  char name[64];
  memset(name, 0, sizeof(name));

  #ifdef RT_REMOTE_LOOPBACK_ONLY
  rtError e = RT_OK;
  strcpy(name, "127.0.0.1");
  #else
  rtError e = rtFindFirstInetInterface(name, sizeof(name));
  #endif

  if (e == RT_OK)
  {
    sockaddr_storage temp;
    e = rtParseAddress(temp, name, port, nullptr);
    if (e == RT_OK)
      memcpy(&addr, &temp, sizeof(sockaddr_storage));
  }

  return e;
}
