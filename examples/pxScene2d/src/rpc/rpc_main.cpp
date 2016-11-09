
#include "rtRemote.h"
#include "rtRemoteConfig.h"
#include "rtRemoteNameService.h"
#include <rtObject.h>
#include <functional>

#include <unistd.h>
#include <iostream>
#include <map>

#include <time.h>
#include <unistd.h>

#include "glib_utils.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#include <stdio.h>
#include <math.h>
#include <algorithm> // std::min_element
#include <iterator>  // std::begin, std::end

  const int iter_count = 1000;


rtRemoteEnvironment* env = nullptr;

class rtEcho : public rtObject
{
  rtDeclareObject(rtEcho, rtEcho);
  rtProperty(message, getMessage, setMessage, rtString);
  rtMethod1ArgAndNoReturn("callTestMethod", callTestMethod, rtString);
public:
  rtError getMessage(rtString& s) const
    { s = m_msg; return RT_OK; }

  rtError setMessage(rtString const& )
  {
    static int i=0;
    if ( ! (++i % 1000))
        puts("1k iters!");
    return RT_OK;
  }
  rtError callTestMethod(rtString const& )
  {
    static int i=0;
    if ( ! (++i % 1000))
        puts("callTestMethod 1k iters!");
    return RT_OK;   
  }
private:
  rtString m_msg;
  rtFunctionRef m_func;
};

rtDefineObject(rtEcho, rtObject);
rtDefineProperty(rtEcho, message);
rtDefineMethod(rtEcho, callTestMethod);

GMainLoop* gLoop;
int gPipefd[2];

void rtMainLoopCb(void*)
{
  rtError err;
  err = rtRemoteProcessSingleItem();
  if(RT_OK != err)
	  printf("error processing item, %d\n", err);
}

void rtRemoteCallback_(void*)
{
  static char temp[1];
  int ret = HANDLE_EINTR_EAGAIN(write(gPipefd[PIPE_WRITE], temp, 1));
  if (ret == -1)
    perror("can't write to pipe");
}


timespec time_diff(timespec start, timespec end)
{
    timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}

float calculateSD(std::vector<float> v)
{
  double sum = std::accumulate(v.begin(), v.end(), 0.0);
  double mean = sum / v.size();

  std::vector<double> diff(v.size());
  std::transform(v.begin(), v.end(), diff.begin(),
                 std::bind2nd(std::minus<double>(), mean));
  double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
  double stdev = std::sqrt(sq_sum / v.size());
  return stdev;
}

void Test_Set_Property(rtObjectRef obj)
{
  WTF_SCOPE0("Test_Set_Property");
  rtError e;

  int i = iter_count;
  timespec start, end, diff, total_start, total_end;

  vector<float> results;
  results.resize(iter_count);

  clock_gettime(CLOCK_REALTIME, &total_start);
  while (i--)
  {
    WTF_SCOPE0("Test_Set_Property#loop_iteration");
    clock_gettime(CLOCK_REALTIME, &start);
    e = obj.set("message", "test");
    if (e != RT_OK)
      puts("error setting property");
    clock_gettime(CLOCK_REALTIME, &end);
    diff = time_diff(start, end);
    float diff_ms = ((diff.tv_sec * 1000) + diff.tv_nsec/1000000.0);
    results[i] = diff_ms;
  }
  clock_gettime(CLOCK_REALTIME, &total_end);

  diff = time_diff(total_start, total_end);
  float diff_ms = ((diff.tv_sec * 1000) + diff.tv_nsec/1000000.0);

  printf("%d iterations. Min time: %f, max time: %f, Deviation = %f ms, total time = %f ms, avg time per iteration: %f\n",
      iter_count,
      *std::min_element(results.begin(), results.end()),
      *std::max_element(results.begin(), results.end()),
      calculateSD(results),
      diff_ms,
      diff_ms/iter_count);
}

void Test_Call_Method(rtObjectRef obj)
{
  WTF_SCOPE0("Test_Call_Method");
  int i = iter_count;
  rtError e;

  timespec start, end, diff, total_start, total_end;

  vector<float> results;
  results.resize(iter_count);

  clock_gettime(CLOCK_REALTIME, &total_start);

  while (i--)
  {
    WTF_SCOPE0("Test_Call_Method#loop_iteration");
    clock_gettime(CLOCK_REALTIME, &start);
    e = obj.send("callTestMethod", "test");
    if (e != RT_OK)
      puts("error calling method");
    clock_gettime(CLOCK_REALTIME, &end);
    diff = time_diff(start, end);
    double diff_ms = ((diff.tv_sec * 1000) + diff.tv_nsec/1000000.0);
    results[i] = diff_ms;
  }
  clock_gettime(CLOCK_REALTIME, &total_end);

  diff = time_diff(total_start, total_end);
  float diff_ms = ((diff.tv_sec * 1000) + diff.tv_nsec/1000000.0);

  printf("%d iterations. Min time: %f, max time: %f, Deviation = %f ms, total time = %f ms, avg time per iteration: %f\n",
      iter_count,
      *std::min_element(results.begin(), results.end()),
      *std::max_element(results.begin(), results.end()),
      calculateSD(results),
      diff_ms,
      diff_ms/iter_count);

}

void Test_Echo_Client()
{
  rtObjectRef obj;
  rtError e = rtRemoteLocateObject(env, "echo.object", obj);
  RT_ASSERT(e == RT_OK);

  Test_Set_Property(obj);
  Test_Call_Method(obj);

  puts("done!");
}

void Test_Echo_Server()
{
  gLoop = g_main_loop_new(g_main_context_default(), FALSE);

  auto *source = pipe_source_new(gPipefd, rtMainLoopCb, nullptr);
  g_source_attach(source, g_main_loop_get_context(gLoop));

  rtRemoteRegisterQueueReadyHandler( rtEnvironmentGetGlobal(), rtRemoteCallback_, nullptr );

  rtError e = rtRemoteInit();

  if (e != RT_OK)
      rtLogError("failed to initialize rtRemoteInit: %d", e);

  rtObjectRef obj(new rtEcho());
  e = rtRemoteRegisterObject(env, "echo.object", obj);
  RT_ASSERT(e == RT_OK);

  g_main_loop_run(gLoop);
  rtRemoteShutdown();
  g_source_unref(source);
}

int main(int /*argc*/, char* /*argv*/[])
{
  WTF_THREAD_ENABLE("main");
  env = rtEnvironmentGetGlobal();
  {
    std::string local_file_name;
    if (getenv("CLIENT_ONLY"))
    {
      rtError e = rtRemoteInit(env);
      if(RT_OK != e)
    	  puts("error running init");
      Test_Echo_Client();
      wtf::Runtime::GetInstance()->SaveToFile("client.wtf-trace");
    }
    else
    {
      Test_Echo_Server();
    }

    //rtRemoteShutdown(env);

    return 0;
  }
}
