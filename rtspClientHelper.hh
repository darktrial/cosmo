#include <unistd.h>
#include <string.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <queue>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>
#include <memory>
#include <thread>
#include <tuple>
#include <functional>
#include <atomic>
#include <mutex>
#include <memory>
#include <chrono>
#include <random>
#include <condition_variable>
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#define RTSP_CLIENT_VERBOSITY_LEVEL 1 // by default, print verbose output from each "RTSPClient"
//#define REQUEST_STREAMING_OVER_TCP False
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 1048576 // 100000
// #define DEBUG_PRINT_EACH_RECEIVED_FRAME 0

enum retcode
{
  OK = 1,
  FAIL,
  ERR_INVALID_PARAM,
  ERR_TIMEOUT,
  ERR_NOT_IMPLEMENTED,
  ERR_NOT_FOUND,
  ERR_NOT_SUPPORT,
  ERR_NOT_READY,
  ERR_NOT_EXIST,
  ERR_NOT_INIT,
  ERR_NOT_MATCH,
  ERR_NOT_ENOUGH_SPACE,
  ERR_NOT_ENOUGH_DATA,
  ERR_NOT_ENOUGH_TIME,
  ERR_NOT_ENOUGH_RESOURCE,
  ERR_NOT_ENOUGH_STORAGE,
  ERR_NOT_ENOUGH_MEMORY
};

class StreamClientState
{
public:
  StreamClientState();
  virtual ~StreamClientState();

public:
  MediaSubsessionIterator *iter;
  MediaSession *session;
  MediaSubsession *subsession;
  TaskToken streamTimerTask;
  double duration;
};

class rtspPlayer
{
public:
  char watchVariable;
  bool isPlaying;
  bool overTCP;
  int startRTSP(const char *url, bool overTCP, const char *username, const char *password);
  void stopRTSP();
  void (*onConnectionSetup)(char *, void *);
  void (*onFrameData)(unsigned char *, const char *, unsigned, unsigned, struct timeval, void *);
  void *privateData;
  rtspPlayer(void *data)
  {
    onFrameData = NULL;
    onConnectionSetup = NULL;
    overTCP=false;
    privateData = data;
  }
  ~rtspPlayer()
  {
  }

private:
  void playRTSP(const char *url, const char *username, const char *password);
};

class ourRTSPClient : public RTSPClient
{
public:
  rtspPlayer *player;
  unsigned rtspClientCount;
  bool isClosed;
  static ourRTSPClient *createNew(UsageEnvironment &env, char const *rtspURL,
                                  int verbosityLevel = 0,
                                  char const *applicationName = NULL,
                                  portNumBits tunnelOverHTTPPortNum = 0);

protected:
  ourRTSPClient(UsageEnvironment &env, char const *rtspURL,
                int verbosityLevel, char const *applicationName, portNumBits tunnelOverHTTPPortNum);
  // called only by createNew();
  virtual ~ourRTSPClient();

public:
  StreamClientState scs;
};

class DummySink : public MediaSink
{
public:
  rtspPlayer *player;
  static DummySink *createNew(UsageEnvironment &env,
                              MediaSubsession &subsession,  // identifies the kind of data that's being received
                              char const *streamId = NULL); // identifies the stream itself (optional)

private:
  DummySink(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId);
  // called only by "createNew()"
  virtual ~DummySink();

  static void afterGettingFrame(void *clientData, unsigned frameSize,
                                unsigned numTruncatedBytes,
                                struct timeval presentationTime,
                                unsigned durationInMicroseconds);
  void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
                         struct timeval presentationTime, unsigned durationInMicroseconds);

private:
  // redefined virtual functions:
  virtual Boolean continuePlaying();

private:
  u_int8_t *fReceiveBuffer;
  MediaSubsession &fSubsession;
  char *fStreamId;
};