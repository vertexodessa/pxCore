#include "rtRemoteFunction.h"
#include "rtRemoteClient.h"
#include "rtRemoteConfig.h"

rtRemoteFunction::rtRemoteFunction(std::string const& id, std::string const& name, std::shared_ptr<rtRemoteClient> const& client)
  : m_ref_count(0)
  , m_id(id)
  , m_name(name)
  , m_rpc_client(client)
{
    printf("setting meepalive for m_name %s\n", m_name.c_str());
  if (!strcmp(id.c_str(), "global"))
  {
    m_rpc_client->keepAlive(m_name);
  }
}

rtRemoteFunction::~rtRemoteFunction()
{
  Release();
}

rtError
rtRemoteFunction::Send(int argc, rtValue const* argv, rtValue* result)
{
  return m_rpc_client->send(m_id, m_name, argc, argv, result,
    rtRemoteSetting<uint32_t>("rt.rpc.default.request_timeout"));
}

unsigned long
rtRemoteFunction::AddRef()
{
  return rtAtomicInc(&m_ref_count);
}

unsigned long
rtRemoteFunction::Release()
{
  unsigned long n = rtAtomicDec(&m_ref_count);
  if (n == 0)
{
  if (!strcmp(m_id.c_str(), "global"))
  {
    printf("removing meepalive for m_name %s\n", m_name.c_str());
    m_rpc_client->removeKeepAlive(m_name);
  }
    delete this;
}
  // TODO: send deref here
  return n;
}
