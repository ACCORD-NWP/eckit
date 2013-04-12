/*
 * (C) Copyright 1996-2012 ECMWF.
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 * In applying this licence, ECMWF does not waive the privileges and immunities 
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

// File Inited.h
// Manuel Fuentes - ECMWF Jan 97

#ifndef eckit_Inited_h
#define eckit_Inited_h

#include "eclib/Types.h"


//-----------------------------------------------------------------------------

namespace eckit {

//-----------------------------------------------------------------------------

// Serves to initialise to 0 numerical or pointer values (which
// should be removed when the compilator supports it)
// i.e. long() gets the value which is in the stack

template <class T>
class Inited {
public:

// -- Contructors

	Inited(): value_(0)           {  }
	Inited(const T& v): value_(v) {  }

// -- Destructor

	~Inited()                      {  }

// -- Operators

	operator T&()                  { return value_; }
	operator const T&() const      { return value_; }

	Inited<T>& operator += (const Inited<T>& other);
	Inited<T>& operator -= (const Inited<T>& other);

private:

// -- Members

	T value_;

};

template<>
inline
Inited<Ordinal>& Inited<Ordinal>::operator +=(const Inited<Ordinal>& other)
{
	value_ += other.value_;
	return *this;
}

template<>
inline
Inited<Ordinal>& Inited<Ordinal>::operator -= (const Inited<Ordinal>& other)
{ 
	value_ -= other.value_; 
	return *this;
}


//-----------------------------------------------------------------------------

} // namespace eckit

#endif
