// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once

///////////////////////////////////////////////////////////////
// include used classes
///////////////////////////////////////////////////////////////

#include "StreamException.h"

///////////////////////////////////////////////////////////////
// we use ints (or at least int-like types) for IDs
///////////////////////////////////////////////////////////////

typedef int STREAM_TYPE_ID;
typedef int SUB_STREAM_ID;

///////////////////////////////////////////////////////////////
//
// IStreamFactory<>
//
//		interface for stream factories. It's a template since
//		the interface is symmetric for in and out streams.
//		I.e. there will be only two instances of it.
//
//		I is the desired stream object interface 
//		(either in or out stream interface).
//		B is the corresponding file buffer class 
//		(used by the streams to r/w their data)
//
///////////////////////////////////////////////////////////////

template<class I, class B>
class IStreamFactory
{
public:

	// it's a class with v-table .. 
	// so, provide an explicitly virtual destructor

	virtual ~IStreamFactory() {};

	// the actual interface methods
	// the type of streams that will be created
	// and the actual creator method

	virtual STREAM_TYPE_ID GetTypeID() const = 0;
	virtual I* CreateStream ( B* buffer
							, int id) const = 0;
};

///////////////////////////////////////////////////////////////
//
// CStreamFactoryPool<>
//
//		Pool (singleton) of all stream factories. There will be 
//		two instances of this template (in and out).
//
//		I is an IStreamFactory<> instance.
//
//		Factories will not be removed from the pool during 
//		application tear-down. You must not access the pool
//		during the destruction of any static object.
//
//		Usage:
//
//		stream = pool::GetInstance()->GetFactory(type)->CreateStream(..)
//
///////////////////////////////////////////////////////////////

template<class I>
class CStreamFactoryPool
{
private:

	// all factories that registered to this pool

	typedef std::map<STREAM_TYPE_ID, I*> TFactories;
	TFactories factories;

	// singleton -> hide construction

	CStreamFactoryPool() {};

public:

	// destruction (nothing to do)

	~CStreamFactoryPool() {};

	// find the factory to the given stream type

	const I* GetFactory (STREAM_TYPE_ID type) const
	{
		typename TFactories::const_iterator iter = factories.find (type);
		if (iter == factories.end())
			throw CStreamException ("No factory for that stream type");

		return iter->second;
	}

	// register a new factory (and don't register twice)
		
	void Add (I* factory)
	{
		assert (factories.find (factory->GetTypeID()) == factories.end());
		factories [factory->GetTypeID()] = factory;
	}

	// Meyer's singleton

	static CStreamFactoryPool<I>* GetInstance()
	{
		static CStreamFactoryPool<I> instance;
		return &instance;
	}
};

///////////////////////////////////////////////////////////////
//
// CStreamFactory<>
//
//		Implements IStreamFactory<> for a given stream class T.
//		I.e. it provides a singleton factory class for that type.
//
//		T is the stream class.
//		I is the generic stream interface (either in or out).
//		B is the corresponding file buffer class 
//		type is a unique type identifier.
//
//		All instances of this class auto-register to a central
//		factory pool (see CStreamFactoryPool<>).
//
//		Stream types are expected to declare a suitable factory
//		as static member.
//
///////////////////////////////////////////////////////////////

template<class T, class I, class B, STREAM_TYPE_ID type> 
class CStreamFactory : public IStreamFactory<I, B>
{
private:

	// singleton -> we hide the constructors
	// auto-register to pool during construction

	CStreamFactory()
	{
		typedef CStreamFactoryPool< IStreamFactory<I, B> > TPool;
		TPool::GetInstance()->Add (this);
	}

public:

	// as we have static run-time, we cannot be sure that the
	// pool is still alive when we get destroyed
	// -> don't deregister from pool upon destruction

	virtual ~CStreamFactory()
	{
	}

	// implement IStreamFactory

	virtual STREAM_TYPE_ID GetTypeID() const
	{
		return type;
	}

	virtual I* CreateStream (B* buffer, int id) const 
	{
		return new T (buffer, id);
	}

	// Meyer's singleton

	static const IStreamFactory<I, B>* GetInstance() 
	{
		static CStreamFactory<T, I, B, type> instance;
		return &instance;
	}

	// creator utility

	struct CCreator
	{
		CCreator() 
		{
			CStreamFactory::GetInstance();
		}
	};
};

///////////////////////////////////////////////////////////////
// forward declarations: the two basic stream interfaces (I & O)
///////////////////////////////////////////////////////////////

class IHierarchicalInStream;
class IHierarchicalOutStream;

///////////////////////////////////////////////////////////////
// forward declarations: the two buffered file abstractions (I & O)
///////////////////////////////////////////////////////////////

class CCacheFileInBuffer;
class CCacheFileOutBuffer;

///////////////////////////////////////////////////////////////
// some frequently used template instances
///////////////////////////////////////////////////////////////

typedef IStreamFactory<IHierarchicalInStream, CCacheFileInBuffer> IInStreamFactory;
typedef IStreamFactory<IHierarchicalOutStream, CCacheFileOutBuffer> IOutStreamFactory;
typedef CStreamFactoryPool<IInStreamFactory> CInStreamFactoryPool;
typedef CStreamFactoryPool<IOutStreamFactory> COutStreamFactoryPool;

