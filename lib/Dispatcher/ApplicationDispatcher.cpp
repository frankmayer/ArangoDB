////////////////////////////////////////////////////////////////////////////////
/// @brief application server with dispatcher
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is triAGENS GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
/// @author Copyright 2009-2014, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#include "BasicsC/win-utils.h"
#endif

#include "ApplicationDispatcher.h"

#include "BasicsC/logging.h"
#include "Dispatcher/Dispatcher.h"
#include "Scheduler/Scheduler.h"
#include "Scheduler/PeriodicTask.h"

using namespace std;
using namespace triagens::basics;
using namespace triagens::rest;

// -----------------------------------------------------------------------------
// --SECTION--                                                   private classes
// -----------------------------------------------------------------------------

namespace {

////////////////////////////////////////////////////////////////////////////////
/// @brief produces a dispatcher status report
////////////////////////////////////////////////////////////////////////////////

  class DispatcherReporterTask : public PeriodicTask {
    public:
      DispatcherReporterTask (Dispatcher* dispatcher, double reportInterval)
        : Task("Dispatcher-Reporter"), PeriodicTask(0.0, reportInterval), _dispatcher(dispatcher) {
      }

    public:
      bool handlePeriod () {
        _dispatcher->reportStatus();
        return true;
      }

    public:
      Dispatcher* _dispatcher;
  };
}

// -----------------------------------------------------------------------------
// --SECTION--                                       class ApplicationDispatcher
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// --SECTION--                                      constructors and destructors
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief constructor
////////////////////////////////////////////////////////////////////////////////

ApplicationDispatcher::ApplicationDispatcher ()
  : ApplicationFeature("dispatcher"),
    _applicationScheduler(0),
    _dispatcher(0),
    _dispatcherReporterTask(0),
    _reportInterval(60.0) {
}

////////////////////////////////////////////////////////////////////////////////
/// @brief destructor
////////////////////////////////////////////////////////////////////////////////

ApplicationDispatcher::~ApplicationDispatcher () {
  if (_dispatcher != 0) {
    delete _dispatcher;
  }
}

// -----------------------------------------------------------------------------
// --SECTION--                                                    public methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief sets the scheduler
////////////////////////////////////////////////////////////////////////////////

void ApplicationDispatcher::setApplicationScheduler (ApplicationScheduler* scheduler) {
  _applicationScheduler = scheduler;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief returns the dispatcher
////////////////////////////////////////////////////////////////////////////////

Dispatcher* ApplicationDispatcher::dispatcher () const {
  return _dispatcher;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief builds the dispatcher queue
////////////////////////////////////////////////////////////////////////////////

void ApplicationDispatcher::buildStandardQueue (size_t nrThreads,
                                                size_t maxSize) {
  if (_dispatcher == 0) {
    LOG_FATAL_AND_EXIT("no dispatcher is known, cannot create dispatcher queue");
  }

  LOG_TRACE("setting up a standard queue with %d threads", (int) nrThreads);

  _dispatcher->addQueue("STANDARD", nrThreads, maxSize);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief builds the named dispatcher queue
////////////////////////////////////////////////////////////////////////////////

void ApplicationDispatcher::buildNamedQueue (string const& name,
                                             size_t nrThreads,
                                             size_t maxSize) {
  if (_dispatcher == 0) {
    LOG_FATAL_AND_EXIT("no dispatcher is known, cannot create dispatcher queue");
  }

  LOG_TRACE("setting up a named queue '%s' with %d threads", name.c_str(), (int) nrThreads);

  _dispatcher->addQueue(name, nrThreads, maxSize);
}

// -----------------------------------------------------------------------------
// --SECTION--                                        ApplicationFeature methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

void ApplicationDispatcher::setupOptions (map<string, ProgramOptionsDescription>& options) {

  options[ApplicationServer::OPTIONS_SERVER + ":help-extended"]
    ("dispatcher.report-interval", &_reportInterval, "dispatcher report interval")
  ;
}

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

bool ApplicationDispatcher::prepare () {
  buildDispatcher(_applicationScheduler->scheduler());

  return true;
}

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

bool ApplicationDispatcher::start () {
  buildDispatcherReporter();

  bool ok = _dispatcher->start();

  if (! ok) {
    LOG_FATAL_AND_EXIT("cannot start dispatcher");
  }

  while (! _dispatcher->isStarted()) {
    LOG_DEBUG("waiting for dispatcher to start");
    usleep(500 * 1000);
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

bool ApplicationDispatcher::open () {
  if (_dispatcher != 0) {
    return _dispatcher->open();
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

void ApplicationDispatcher::stop () {
  if (_dispatcherReporterTask != 0) {
    _dispatcherReporterTask = 0;
  }

  if (_dispatcher != 0) {
    static size_t const MAX_TRIES = 50; // 10 seconds (50 * 200 ms)

    _dispatcher->beginShutdown();

    for (size_t count = 0;  count < MAX_TRIES && _dispatcher->isRunning();  ++count) {
      LOG_TRACE("waiting for dispatcher to stop");
      usleep(200 * 1000);
    }

    _dispatcher->shutdown();

    delete _dispatcher;
    _dispatcher = 0;
  }
}

// -----------------------------------------------------------------------------
// --SECTION--                                                   private methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief builds the dispatcher
////////////////////////////////////////////////////////////////////////////////

void ApplicationDispatcher::buildDispatcher (Scheduler* scheduler) {
  if (_dispatcher != 0) {
    LOG_FATAL_AND_EXIT("a dispatcher has already been created");
  }

  _dispatcher = new Dispatcher(scheduler);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief builds the dispatcher reporter
////////////////////////////////////////////////////////////////////////////////

void ApplicationDispatcher::buildDispatcherReporter () {
  if (_dispatcher == 0) {
    LOG_FATAL_AND_EXIT("no dispatcher is known, cannot create dispatcher reporter");
  }

  if (0.0 < _reportInterval) {
    _dispatcherReporterTask = new DispatcherReporterTask(_dispatcher, _reportInterval);

    _applicationScheduler->scheduler()->registerTask(_dispatcherReporterTask);
  }
}

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// {@inheritDoc}\\|/// @addtogroup\\|/// @page\\|// --SECTION--\\|/// @\\}"
// End:
