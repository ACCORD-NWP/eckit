/*
 * (C) Copyright 1996-2017 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <unistd.h>
#include <sys/mman.h>

#include "eckit/eckit.h"
#include "eckit/memory/MMap.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/thread/AutoLock.h"
#include "eckit/thread/Mutex.h"

namespace eckit {


static long count_;
static long maxCount_;

static size_t length_;
static size_t maxLength_;

static eckit::Mutex mutex_;

//----------------------------------------------------------------------------------------------------------------------

void* MMap::mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    void* r = ::mmap(addr, length, prot, flags, fd, offset);
    if (r != MAP_FAILED) {
        AutoLock<Mutex> lock(mutex_);

        count_++;
        maxCount_ = std::max(count_, maxCount_);

        length_ += length;
        maxLength_ = std::max(length_, maxLength_);
    }
    return r;
}

int MMap::munmap(void* addr, size_t length)
{
    int r = ::munmap(addr, length);
    if (r == 0) {
        AutoLock<Mutex> lock(mutex_);

        count_--;
        length_ -= length;
    }
    return r;
}


void MMap::dump(std::ostream& out) {

    AutoLock<Mutex> lock(mutex_);

    if (count_) {
        out << ", mmap count: " << count_;
    }

    if (count_) {
        out << ", mmap size: " << length_;
    }
}


//----------------------------------------------------------------------------------------------------------------------

} // namespace eckit
