/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

/// @file   Application.h
/// @date   May 1996
/// @author Baudouin Raoult
/// @author Tiago Quintino
/// @author Florian Rathgeber

#ifndef eckit_Application_h
#define eckit_Application_h

#include "eckit/runtime/Main.h"
#include "eckit/runtime/Task.h"

//-----------------------------------------------------------------------------

namespace eckit {

//-----------------------------------------------------------------------------

class Application : public Task, public Main {
public:

    // -- Contructors

    Application(int argc, char **argv, const char* homeenv);

    // -- Destructor

    virtual ~Application();

    // -- Methods

    virtual void run() = 0;

    /// call this function once to make this application unique on this host
    void unique();

    // -- Overridden methods

    // From Task

    virtual void start();
    virtual void stop();
    virtual void kill();
    virtual void wait() {}
    virtual bool active() { return true; }

    std::string appName() const { return name_; }
    bool running() const { return running_; }

    time_t uptime();


protected: // methods

    // From Configurable

    virtual void   reconfigure();
    virtual std::string name() const   { return name_; }

private: // members


    bool   running_;

    /// From Main
    virtual void terminate();

    /// overriden from Configurable

    virtual std::string kind() const  { return "Application"; }

};

//-----------------------------------------------------------------------------

} // namespace eckit

#endif
