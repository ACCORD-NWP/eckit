/*
 * (C) Copyright 1996-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <unistd.h>

#include "eckit/filesystem/TmpFile.h"

//-----------------------------------------------------------------------------

namespace eckit {

//-----------------------------------------------------------------------------

static PathName tmp()
{
    const char *tmpdir = ::getenv("TMPDIR");
	if(!tmpdir)
	{
        tmpdir = "/tmp";
    }

    long max = pathconf(tmpdir, _PC_PATH_MAX);
    char path[max];
    sprintf(path, "%s/eckitXXXXXXXXXXX", tmpdir);
    int fd;
    SYSCALL(fd = ::mkstemp(path));

    PathName result(path);
    result.touch();

    SYSCALL(::close(fd));

    return result;
}


TmpFile::TmpFile(): PathName(tmp())
{
}

TmpFile::~TmpFile()
{
    unlink();
}

//-----------------------------------------------------------------------------

} // namespace eckit

