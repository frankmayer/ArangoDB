////////////////////////////////////////////////////////////////////////////////
/// @brief abstract base class for jobs
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
/// @author Martin Schoenert
/// @author Copyright 2009-2014, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef TRIAGENS_DISPATCHER_JOB_H
#define TRIAGENS_DISPATCHER_JOB_H 1

#include "Statistics/StatisticsAgent.h"

// -----------------------------------------------------------------------------
// --SECTION--                                              forward declarations
// -----------------------------------------------------------------------------

namespace triagens {
  namespace basics {
    class TriagensError;
  }

  namespace rest {
    class DispatcherThread;

// -----------------------------------------------------------------------------
// --SECTION--                                                         class Job
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief abstract base class for jobs
////////////////////////////////////////////////////////////////////////////////

    class Job : public RequestStatisticsAgent {
      private:
        Job (Job const&);
        Job& operator= (Job const&);

// -----------------------------------------------------------------------------
// --SECTION--                                                      public types
// -----------------------------------------------------------------------------

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief job types
////////////////////////////////////////////////////////////////////////////////

        enum JobType {
          READ_JOB,
          WRITE_JOB,
          SPECIAL_JOB
        };

////////////////////////////////////////////////////////////////////////////////
/// @brief status of execution
////////////////////////////////////////////////////////////////////////////////

        enum status_e {
          JOB_DONE,
          JOB_DETACH,
          JOB_REQUEUE,
          JOB_FAILED
        };

////////////////////////////////////////////////////////////////////////////////
/// @brief result of execution
////////////////////////////////////////////////////////////////////////////////

        class status_t {
          public:
            status_t ()
              : status(JOB_FAILED) {
            }

            explicit
            status_t (status_e status)
              : status(status) {
            }

            status_e status;
            double sleep;
        };

// -----------------------------------------------------------------------------
// --SECTION--                                      constructors and destructors
// -----------------------------------------------------------------------------

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief constructs a job
////////////////////////////////////////////////////////////////////////////////

        explicit
        Job (string const& name);

////////////////////////////////////////////////////////////////////////////////
/// @brief destructor
////////////////////////////////////////////////////////////////////////////////

        virtual ~Job ();

// -----------------------------------------------------------------------------
// --SECTION--                                                    public methods
// -----------------------------------------------------------------------------

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief getter for the name
////////////////////////////////////////////////////////////////////////////////

        const string& getName () const;

// -----------------------------------------------------------------------------
// --SECTION--                                            virtual public methods
// -----------------------------------------------------------------------------

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief gets the type of the job
///
/// Note that initialise can change the job type.
////////////////////////////////////////////////////////////////////////////////

        virtual JobType type () = 0;

////////////////////////////////////////////////////////////////////////////////
/// @brief returns the queue name to use
////////////////////////////////////////////////////////////////////////////////

        virtual string const& queue ();

////////////////////////////////////////////////////////////////////////////////
/// @brief sets the thread which currently dealing with the job
////////////////////////////////////////////////////////////////////////////////

        virtual void setDispatcherThread (DispatcherThread*);

////////////////////////////////////////////////////////////////////////////////
/// @brief starts working
////////////////////////////////////////////////////////////////////////////////

        virtual status_t work () = 0;

////////////////////////////////////////////////////////////////////////////////
/// @brief cleans up after work and delete
////////////////////////////////////////////////////////////////////////////////

        virtual void cleanup () = 0;

////////////////////////////////////////////////////////////////////////////////
/// @brief shuts down the execution and deletes everything
////////////////////////////////////////////////////////////////////////////////

        virtual bool beginShutdown () = 0;

////////////////////////////////////////////////////////////////////////////////
/// @brief handle error and delete
////////////////////////////////////////////////////////////////////////////////

        virtual void handleError (basics::TriagensError const&) = 0;

// -----------------------------------------------------------------------------
// --SECTION--                                                 private variables
// -----------------------------------------------------------------------------

      private:

////////////////////////////////////////////////////////////////////////////////
/// @brief name of the job
////////////////////////////////////////////////////////////////////////////////

        const string& _name;
    };
  }
}

#endif

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// {@inheritDoc}\\|/// @addtogroup\\|/// @page\\|// --SECTION--\\|/// @\\}"
// End:
