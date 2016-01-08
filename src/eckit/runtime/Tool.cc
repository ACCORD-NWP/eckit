/*
 * (C) Copyright 1996-2015 ECMWF.
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 * In applying this licence, ECMWF does not waive the privileges and immunities 
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <stdlib.h>

#include "eckit/runtime/Context.h"
#include "eckit/bases/Loader.h"
#include "eckit/log/Log.h"
#include "eckit/filesystem/PathName.h"
#include "eckit/config/Resource.h"

#include "eckit/runtime/Tool.h"
#include "eckit/runtime/ToolBehavior.h"

//-----------------------------------------------------------------------------

namespace eckit {

//-----------------------------------------------------------------------------

Tool::Tool(int argc,char **argv) : 
    name_("undefined")
{
	name_ = PathName(argv[0]).baseName(false);
    
    Context::instance().setup( argc, argv );
    Context::instance().behavior( new ToolBehavior() );
    Context::instance().runName( name_ );

    Loader::callAll(&Loader::execute);
}

Tool::~Tool()
{
}

void Tool::reconfigure()
{
    Log::info() << "Tool::reconfigure" << std::endl;
    
    int debug = Resource<int>(this,"debug;$DEBUG;-debug",0);
    
    Context::instance().debug( debug );

    // forward to context
    Context::instance().reconfigure();     
}

int Tool::start()
{
	int status = 0;

    int debug = Resource<int>(this,"debug;$DEBUG;-debug",0);
    
    Context::instance().debug( debug );

    std::string displayName = Resource<std::string>("-name",name_);
    
    Context::instance().displayName( displayName );
    
	try 
    {
        run();
	}
    catch( Exception& e ){
        status = 1;
        Log::error() << "** " << e.what() << " Caught in "  << Here() << std::endl;
        Log::error() << "** Exception is terminates " << name() << std::endl;
        Exception::exceptionStack( Log::error(), true );
    }
    catch( std::exception& e ){
		status = 1;
		Log::error() << "** " << e.what() << " Caught in "  << Here() << std::endl;
		Log::error() << "** Exception is terminates " << name() << std::endl;
	}

    return status;
}

//-----------------------------------------------------------------------------

} // namespace eckit

