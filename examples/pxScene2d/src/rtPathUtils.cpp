// rtCore CopyRight 2007-2015 John Robinson
// rtPathUtils.h

#include "rtPathUtils.h"
#include <stdlib.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif


#ifdef _WIN32
rtError rtGetCurrentDirectory(rtString& d)
{
  rtError e = RT_FAIL;

  TCHAR buff[MAX_PATH];
  DWORD dwRet = GetCurrentDirectory(MAX_PATH - 1, buff);
  if (dwRet)
  {
    buff[dwRet] = '\0';
    d = buff;
    e = RT_OK;
  }

  return e;
}
#else
rtError rtGetCurrentDirectory(rtString& d) 
{
  char* p = getcwd(NULL, 0);

  if (p) {
    d = p;
    free(p);
  }
  return RT_OK;
}
#endif

