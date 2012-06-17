////////////////////////////////////////////////////////////////////////////////
/// @brief application ZeroMQ
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2012 triAGENS GmbH, Cologne, Germany
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
/// @author Copyright 2012, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef TRIAGENS_ZEROMQ_APPLICATION_ZEROMQ_H
#define TRIAGENS_ZEROMQ_APPLICATION_ZEROMQ_H 1

#include "ApplicationServer/ApplicationFeature.h"

// -----------------------------------------------------------------------------
// --SECTION--                                              forward declarations
// -----------------------------------------------------------------------------

namespace triagens {
  namespace basics {
    class Thread;
  }

  namespace rest {
    class ApplicationServer;

// -----------------------------------------------------------------------------
// --SECTION--                                        class ApplicationScheduler
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup Scheduler
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief application server scheduler implementation
////////////////////////////////////////////////////////////////////////////////

    class ApplicationZeroMQ : public ApplicationFeature {
      private:
        ApplicationZeroMQ (ApplicationZeroMQ const&);
        ApplicationZeroMQ& operator= (ApplicationZeroMQ const&);

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                      constructors and destructors
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup Scheduler
/// @{
////////////////////////////////////////////////////////////////////////////////

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief constructor
////////////////////////////////////////////////////////////////////////////////

        ApplicationZeroMQ (ApplicationServer*);

////////////////////////////////////////////////////////////////////////////////
/// @brief destructor
////////////////////////////////////////////////////////////////////////////////

        ~ApplicationZeroMQ ();

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                        ApplicationFeature methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup Scheduler
/// @{
////////////////////////////////////////////////////////////////////////////////

      public:

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

        void setupOptions (map<string, basics::ProgramOptionsDescription>&);

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

        bool parsePhase1 (basics::ProgramOptions&);

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

        bool parsePhase2 (basics::ProgramOptions&);

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

        bool start ();

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

        bool isRunning ();

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

        void beginShutdown ();

////////////////////////////////////////////////////////////////////////////////
/// {@inheritDoc}
////////////////////////////////////////////////////////////////////////////////

        void shutdown ();

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                   private methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup Scheduler
/// @{
////////////////////////////////////////////////////////////////////////////////

      private:

////////////////////////////////////////////////////////////////////////////////
/// @brief adjusts the file descriptor limits
////////////////////////////////////////////////////////////////////////////////

        void adjustFileDescriptors ();

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                 private variables
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup Scheduler
/// @{
////////////////////////////////////////////////////////////////////////////////

      private:

////////////////////////////////////////////////////////////////////////////////
/// @brief application server
////////////////////////////////////////////////////////////////////////////////

        ApplicationServer* _applicationServer;

////////////////////////////////////////////////////////////////////////////////
/// @brief ZeroMQ worker thread
////////////////////////////////////////////////////////////////////////////////

        basics::Thread** _zeroMQThreads;

////////////////////////////////////////////////////////////////////////////////
/// @brief number of ZeroMQ thread
////////////////////////////////////////////////////////////////////////////////

        uint32_t _nrZeroMQThreads;

////////////////////////////////////////////////////////////////////////////////
/// @brief ZeroMQ context concurrency
////////////////////////////////////////////////////////////////////////////////

        uint32_t _zeroMQConcurrency;

////////////////////////////////////////////////////////////////////////////////
/// @brief ZeroMQ context
////////////////////////////////////////////////////////////////////////////////

        void* _context;

////////////////////////////////////////////////////////////////////////////////
/// @brief ZeroMQ connection string
////////////////////////////////////////////////////////////////////////////////

        string _connection;
    };
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

#endif

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "^\\(/// @brief\\|/// {@inheritDoc}\\|/// @addtogroup\\|/// @page\\|// --SECTION--\\|/// @\\}\\)"
// End:
