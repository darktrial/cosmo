#include "rtspClientHelper.hh"

static void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode, char *resultString);
static void continueAfterSETUP(RTSPClient *rtspClient, int resultCode, char *resultString);
static void continueAfterPLAY(RTSPClient *rtspClient, int resultCode, char *resultString);
static void subsessionAfterPlaying(void *clientData);
static void subsessionByeHandler(void *clientData, char const *reason);
static void streamTimerHandler(void *clientData);
static void openURL(UsageEnvironment &env, char const *progName, RTSPClient *rtspClient, char const *rtspURL, Authenticator *authenticator);
static void setupNextSubsession(RTSPClient *rtspClient);
static void shutdownStream(RTSPClient *rtspClient, int exitCode = 1);

UsageEnvironment &operator<<(UsageEnvironment &env, const RTSPClient &rtspClient)
{
  return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

UsageEnvironment &operator<<(UsageEnvironment &env, const MediaSubsession &subsession)
{
  return env << subsession.mediumName() << "/" << subsession.codecName();
}

void usage(UsageEnvironment &env, char const *progName)
{
  env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
  env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}

void rtspPlayer::playRTSP(const char *url, const char *username, const char *password)
{
  TaskScheduler *scheduler = BasicTaskScheduler::createNew();
  UsageEnvironment *env = BasicUsageEnvironment::createNew(*scheduler);
  Authenticator *authenticator = NULL;
  if (username != NULL && password != NULL)
    authenticator = new Authenticator(username, password);

  RTSPClient *rtspClient = ourRTSPClient::createNew(*env, url, RTSP_CLIENT_VERBOSITY_LEVEL, "RTSP Client");
  if (rtspClient == NULL)
  {
    std::cout << "Cant allocate RTSP client memory" << std::endl;
    return;
  }
  ((ourRTSPClient *)rtspClient)->rtspClientCount = 0;
  ((ourRTSPClient *)rtspClient)->isClosed = false;
  ((ourRTSPClient *)rtspClient)->player = this;
  openURL(*env, "RTSP Client", rtspClient, url, authenticator);
  env->taskScheduler().doEventLoop(&watchVariable);
  // std::cout << "RTSP Client: Exiting event loop" << std::endl;
  if (((ourRTSPClient *)rtspClient)->isClosed == false)
    shutdownStream(rtspClient);

  env->reclaim();
  env = NULL;
  delete scheduler;
  scheduler = NULL;
  delete authenticator;
  isPlaying = false;
  return;
}

int rtspPlayer::startRTSP(const char *url, bool overTCP, const char *username, const char *password)
{
  watchVariable = 0;
  isPlaying = false;
  this->overTCP = overTCP;
  std::thread t1(&rtspPlayer::playRTSP, this, url, username, password);
  t1.detach();
  while (isPlaying == false)
  {
    if (watchVariable == 1)
      return FAIL;
    usleep(10);
  }
  return OK;
}

void rtspPlayer::stopRTSP()
{
  watchVariable = 1;
  while (isPlaying == true)
  {
    usleep(10);
  }
}

static void openURL(UsageEnvironment &env, char const *progName, RTSPClient *rtspClient, char const *rtspURL, Authenticator *authenticator)
{
  ((ourRTSPClient *)rtspClient)->rtspClientCount++;
  if (authenticator != NULL)
    rtspClient->sendDescribeCommand(continueAfterDESCRIBE, authenticator);
  else
    rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
}

static void continueAfterDESCRIBE(RTSPClient *rtspClient, int resultCode, char *resultString)
{
  do
  {
    UsageEnvironment &env = rtspClient->envir();
    StreamClientState &scs = ((ourRTSPClient *)rtspClient)->scs;

    if (resultCode != 0)
    {
      env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
      delete[] resultString;
      break;
    }

    char *const sdpDescription = resultString;
    // env << *rtspClient << "Got a SDP description:\n"<< sdpDescription << "\n";

    scs.session = MediaSession::createNew(env, sdpDescription);
    delete[] sdpDescription;
    if (scs.session == NULL)
    {
      env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
      break;
    }
    else if (!scs.session->hasSubsessions())
    {
      env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
      break;
    }

    scs.iter = new MediaSubsessionIterator(*scs.session);
    setupNextSubsession(rtspClient);
    return;
  } while (0);
  shutdownStream(rtspClient);
}

static void setupNextSubsession(RTSPClient *rtspClient)
{
  UsageEnvironment &env = rtspClient->envir();
  StreamClientState &scs = ((ourRTSPClient *)rtspClient)->scs;

  scs.subsession = scs.iter->next();
  if (scs.subsession != NULL)
  {
    if (!scs.subsession->initiate())
    {
      env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
      setupNextSubsession(rtspClient);
    }
    else
    {
      // env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
      if (scs.subsession->rtcpIsMuxed())
      {
        // env << "client port " << scs.subsession->clientPortNum();
      }
      else
      {
        // env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
      }
      // env << ")\n";
      rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, ((ourRTSPClient *)rtspClient)->player->overTCP);
    }
    return;
  }
  if (scs.session->absStartTime() != NULL)
  {
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
  }
  else
  {
    scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
  }
}

static void continueAfterSETUP(RTSPClient *rtspClient, int resultCode, char *resultString)
{
  do
  {
    UsageEnvironment &env = rtspClient->envir();
    StreamClientState &scs = ((ourRTSPClient *)rtspClient)->scs;

    if (resultCode != 0)
    {
      env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
      break;
    }

    // env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
    if (scs.subsession->rtcpIsMuxed())
    {
      // env << "client port " << scs.subsession->clientPortNum();
    }
    else
    {
      // env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
    }
    // env << ")\n";

    scs.subsession->sink = DummySink::createNew(env, *scs.subsession, rtspClient->url());
    if (scs.subsession->sink == NULL)
    {
      env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
          << "\" subsession: " << env.getResultMsg() << "\n";
      break;
    }
    if (((ourRTSPClient *)rtspClient)->player->onConnectionSetup != NULL)
    {
      ((ourRTSPClient *)rtspClient)->player->onConnectionSetup((char *)(MediaSubsession *)scs.subsession->codecName(), ((ourRTSPClient *)rtspClient)->player->privateData);
    }
    ((DummySink *)scs.subsession->sink)->player = ((ourRTSPClient *)rtspClient)->player;
    // env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
    scs.subsession->miscPtr = rtspClient;
    scs.subsession->sink->startPlaying(*(scs.subsession->readSource()), subsessionAfterPlaying, scs.subsession);
    if (scs.subsession->rtcpInstance() != NULL)
    {
      scs.subsession->rtcpInstance()->setByeWithReasonHandler(subsessionByeHandler, scs.subsession);
    }
  } while (0);
  delete[] resultString;
  setupNextSubsession(rtspClient);
}

static void continueAfterPLAY(RTSPClient *rtspClient, int resultCode, char *resultString)
{
  Boolean success = False;

  do
  {
    UsageEnvironment &env = rtspClient->envir();
    StreamClientState &scs = ((ourRTSPClient *)rtspClient)->scs;

    if (resultCode != 0)
    {
      env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
      break;
    }
    if (scs.duration > 0)
    {
      unsigned const delaySlop = 2;
      scs.duration += delaySlop;
      unsigned uSecsToDelay = (unsigned)(scs.duration * 1000000);
      scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc *)streamTimerHandler, rtspClient);
    }

    // env << *rtspClient << "Started playing session";
    ((ourRTSPClient *)rtspClient)->player->isPlaying = true;
    if (scs.duration > 0)
    {
      env << " (for up to " << scs.duration << " seconds)";
    }
    // env << "...\n";

    success = True;
  } while (0);
  delete[] resultString;

  if (!success)
  {
    shutdownStream(rtspClient);
  }
}

static void subsessionAfterPlaying(void *clientData)
{
  MediaSubsession *subsession = (MediaSubsession *)clientData;
  RTSPClient *rtspClient = (RTSPClient *)(subsession->miscPtr);

  Medium::close(subsession->sink);
  subsession->sink = NULL;

  MediaSession &session = subsession->parentSession();
  MediaSubsessionIterator iter(session);
  while ((subsession = iter.next()) != NULL)
  {
    if (subsession->sink != NULL)
      return;
  }
  shutdownStream(rtspClient);
}

static void subsessionByeHandler(void *clientData, char const *reason)
{
  MediaSubsession *subsession = (MediaSubsession *)clientData;
  RTSPClient *rtspClient = (RTSPClient *)subsession->miscPtr;
  UsageEnvironment &env = rtspClient->envir(); // alias

  env << *rtspClient << "Received RTCP \"BYE\"";
  if (reason != NULL)
  {
    env << " (reason:\"" << reason << "\")";
    delete[] (char *)reason;
  }
  env << " on \"" << *subsession << "\" subsession\n";
  subsessionAfterPlaying(subsession);
}

static void streamTimerHandler(void *clientData)
{
  ourRTSPClient *rtspClient = (ourRTSPClient *)clientData;
  StreamClientState &scs = rtspClient->scs;

  scs.streamTimerTask = NULL;
  shutdownStream(rtspClient);
}

static void shutdownStream(RTSPClient *rtspClient, int exitCode)
{
  UsageEnvironment &env = rtspClient->envir();
  StreamClientState &scs = ((ourRTSPClient *)rtspClient)->scs;

  if (scs.session != NULL)
  {
    Boolean someSubsessionsWereActive = False;
    MediaSubsessionIterator iter(*scs.session);
    MediaSubsession *subsession;

    while ((subsession = iter.next()) != NULL)
    {
      if (subsession->sink != NULL)
      {
        Medium::close(subsession->sink);
        subsession->sink = NULL;

        if (subsession->rtcpInstance() != NULL)
        {
          subsession->rtcpInstance()->setByeHandler(NULL, NULL);
        }

        someSubsessionsWereActive = True;
      }
    }

    if (someSubsessionsWereActive)
    {
      rtspClient->sendTeardownCommand(*scs.session, NULL);
    }
  }

  // env << *rtspClient << "Closing the stream.\n";
  ((ourRTSPClient *)rtspClient)->isClosed = true;
  ((ourRTSPClient *)rtspClient)->rtspClientCount--;
  ((ourRTSPClient *)rtspClient)->player->watchVariable = 1;
  Medium::close(rtspClient);
}

ourRTSPClient *ourRTSPClient::createNew(UsageEnvironment &env, char const *rtspURL, int verbosityLevel, char const *applicationName, portNumBits tunnelOverHTTPPortNum)
{
  return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment &env, char const *rtspURL,
                             int verbosityLevel, char const *applicationName, portNumBits tunnelOverHTTPPortNum)
    : RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1)
{
}

ourRTSPClient::~ourRTSPClient()
{
}

StreamClientState::StreamClientState() : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0)
{
}

StreamClientState::~StreamClientState()
{
  delete iter;
  if (session != NULL)
  {
    UsageEnvironment &env = session->envir(); // alias

    env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
    Medium::close(session);
  }
}

DummySink *DummySink::createNew(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId)
{
  return new DummySink(env, subsession, streamId);
}

DummySink::DummySink(UsageEnvironment &env, MediaSubsession &subsession, char const *streamId)
    : MediaSink(env),
      fSubsession(subsession)
{
  fStreamId = strDup(streamId);
  fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
}

DummySink::~DummySink()
{
  delete[] fReceiveBuffer;
  delete[] fStreamId;
}

void DummySink::afterGettingFrame(void *clientData, unsigned frameSize, unsigned numTruncatedBytes,
                                  struct timeval presentationTime, unsigned durationInMicroseconds)
{
  DummySink *sink = (DummySink *)clientData;

  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
                                  struct timeval presentationTime, unsigned /*durationInMicroseconds*/)
{
  u_int8_t start_code[4] = {0x00, 0x00, 0x00, 0x01};
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  if (fStreamId != NULL)
    envir() << "Stream \"" << fStreamId << "\"; ";
  envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << "\tReceived " << frameSize << " bytes";
  if (numTruncatedBytes > 0)
    envir() << " (with " << numTruncatedBytes << " bytes truncated)";
  char uSecsStr[6 + 1]; // used to output the 'microseconds' part of the presentation time
  sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
  envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
  if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP())
  {
    envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
  }
#ifdef DEBUG_PRINT_NPT
  envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
  envir() << "\n";
#endif
  if (this->player->onFrameData != NULL)
    this->player->onFrameData(fReceiveBuffer, fSubsession.codecName(), frameSize, numTruncatedBytes, presentationTime, this->player->privateData);
  continuePlaying();
}

Boolean DummySink::continuePlaying()
{
  if (fSource == NULL)
    return False;
  fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                        afterGettingFrame, this,
                        onSourceClosure, this);
  return True;
}
