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
#include "stdafx.h"
#include "PathDictionary.h"
#include "ContainerException.h"
#include <iostream>

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
// CPathDictionary
///////////////////////////////////////////////////////////////

// index check utility

void CPathDictionary::CheckParentIndex (index_t index) const
{
#if !defined (_SECURE_SCL)
	if (index >= paths.size())
        throw CContainerException ("parent path index out of range");
#else
    UNREFERENCED_PARAMETER(index);
#endif
}

// construction utility

void CPathDictionary::Initialize()
{
    paths.Insert (std::make_pair ( (index_t) NO_INDEX, 0));
}

// construction (create root path) / destruction

CPathDictionary::CPathDictionary()
{
	Initialize();
}

CPathDictionary::~CPathDictionary(void)
{
}

// dictionary operations

index_t CPathDictionary::GetParent (index_t index) const
{
	return paths[index].first;
}

const char* CPathDictionary::GetPathElement (index_t index) const
{
	return pathElements [paths [index].second];
}

index_t CPathDictionary::GetPathElementSize (index_t index) const
{
    return pathElements.GetLength (paths [index].second);
}

index_t CPathDictionary::GetPathElementID (index_t index) const
{
	return paths [index].second;
}

index_t CPathDictionary::Find (index_t parent, const char* pathElement) const
{
	index_t pathElementIndex = pathElements.Find (pathElement);
	return pathElementIndex == NO_INDEX
		? NO_INDEX
		: paths.Find (std::make_pair (parent, pathElementIndex));
}

index_t CPathDictionary::Insert (index_t parent, const char* pathElement)
{
	CheckParentIndex (parent);

	index_t pathElementIndex = pathElements.AutoInsert (pathElement);
	return paths.Insert (std::make_pair (parent, pathElementIndex));
}

index_t CPathDictionary::AutoInsert (index_t parent, const char* pathElement)
{
	CheckParentIndex (parent);

	index_t pathElementIndex = pathElements.AutoInsert (pathElement);
	return paths.AutoInsert (std::make_pair ( parent
											, pathElementIndex));
}

// return false if concurrent read accesses
// would potentially access invalid data.

bool CPathDictionary::CanInsertThreadSafely 
    ( index_t elements
    , size_t chars) const
{
    return paths.CanInsertThreadSafely (elements)
        && pathElements.CanInsertThreadSafely (elements, chars);
}

// reset content

void CPathDictionary::Clear()
{
	pathElements.Clear();
	paths.Clear();

	Initialize();
}

// "merge" with another container:
// add new entries and return ID mapping for source container

index_mapping_t CPathDictionary::Merge (const CPathDictionary& source)
{
	index_mapping_t result;
	result.insert (0, 0);
	result.insert ((index_t)NO_INDEX, (index_t)NO_INDEX);

	index_mapping_t elementMapping = pathElements.Merge (source.pathElements);
	for (index_t i = 1, count = source.size(); i < count; ++i)
	{
		const std::pair<index_t, index_t>& sourcePath = source.paths[i];

		std::pair<index_t, index_t> destEntry 
			( *result.find (sourcePath.first)
			, *elementMapping.find (sourcePath.second));

		result.insert (i, paths.AutoInsert (destEntry));
	}

	return result;
}

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CPathDictionary& dictionary)
{
	// read the path elements

	IHierarchicalInStream* elementsStream
		= stream.GetSubStream (CPathDictionary::ELEMENTS_STREAM_ID);
	*elementsStream >> dictionary.pathElements;

	// read the second elements

	IHierarchicalInStream* pathsStream
		= stream.GetSubStream (CPathDictionary::PATHS_STREAM_ID);
	*pathsStream >> dictionary.paths;

	// ready

	return stream;
}

IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CPathDictionary& dictionary)
{
	// write path elements

	IHierarchicalOutStream* elementsStream 
		= stream.OpenSubStream<CCompositeOutStream>
			(CPathDictionary::ELEMENTS_STREAM_ID);
	*elementsStream << dictionary.pathElements;

	// write paths

	IHierarchicalOutStream* pathsStream
		= stream.OpenSubStream<CCompositeOutStream>
			(CPathDictionary::PATHS_STREAM_ID);
	*pathsStream << dictionary.paths;

	// ready

	return stream;
}

///////////////////////////////////////////////////////////////
//
// CDictionaryBasedPath
//
///////////////////////////////////////////////////////////////

// construction utility: lookup and optionally auto-insert

void CDictionaryBasedPath::ParsePath ( const std::string& path
								     , CPathDictionary* writableDictionary
									 , std::vector<std::string>* relPath)
{
    if (!path.empty())
	{
		std::string temp (path);

		index_t currentIndex = index;
        size_t pos = temp[0] == '/' ? 0 : (size_t)(-1);
        size_t nextPos = temp.find ('/', pos+1);

        do
		{
			// get the current path element and terminate it properly

			const char* pathElement = temp.c_str() + pos+1;
			if (nextPos != std::string::npos)
				temp[nextPos] = 0;

			// try move to the next sub-path

			index_t nextIndex = dictionary->Find (currentIndex, pathElement);
			if (nextIndex == NO_INDEX)
			{
				// not found. Do we have to stop here?

				if (writableDictionary != NULL)
				{
					// auto-insert

					nextIndex = writableDictionary->Insert ( currentIndex
														   , pathElement);
					index = nextIndex;
				}
				else if (relPath != NULL)
				{
					// build relative path

					relPath->push_back (pathElement);
				}
				else
				{
					// must stop at the last known parent

					break;
				}
			}
			else
			{
				// we are now one level deeper

				index = nextIndex;
			}

			currentIndex = nextIndex;
            pos = nextPos;
            nextPos = temp.find ('/', nextPos);
		}
        while (pos != std::string::npos);
	}

#ifdef _DEBUG
    _path = GetPath();
#endif
}

// element access

std::string CDictionaryBasedPath::ReverseAt (size_t reverseIndex) const
{
    index_t elementIndex = index;
    for (; reverseIndex > 0; --reverseIndex)
        elementIndex = dictionary->GetParent (elementIndex);

    return std::string ( dictionary->GetPathElement (elementIndex)
                       , dictionary->GetPathElementSize (elementIndex));
}

// construction: lookup (stop at last known parent, if necessary)

CDictionaryBasedPath::CDictionaryBasedPath ( const CPathDictionary* aDictionary
										   , const std::string& path)
	: dictionary (aDictionary)
	, index (0)
{
	ParsePath (path, NULL);
}

CDictionaryBasedPath::CDictionaryBasedPath ( CPathDictionary* aDictionary
										   , const std::string& path
										   , bool nextParent)
	: dictionary (aDictionary)
	, index (0)
{
	ParsePath (path, nextParent ? NULL : aDictionary);
}

// return false if concurrent read accesses
// would potentially access invalid data.

bool CDictionaryBasedPath::CanParsePathThreadSafely 
    ( const CPathDictionary* dictionary
    , const std::string& path)
{
    // trivial case

    if (path.empty())
        return true;

    // parse path and look for a suitable chain of parsed elements
    // within the path dictionary

    std::string temp (path);

    index_t currentIndex = (index_t)0;
    size_t pos = temp[0] == '/' ? 0 : (size_t)(-1);
    size_t nextPos = temp.find ('/', pos+1);

    index_t toAdd = 0;
    do
    {
        // get the current path element and terminate it properly

        const char* pathElement = temp.c_str() + pos+1;
        if (nextPos != std::string::npos)
            temp[nextPos] = 0;

        // try move to the next sub-path

        if (currentIndex != NO_INDEX)
            currentIndex = dictionary->Find (currentIndex, pathElement);

        if (currentIndex == NO_INDEX)
            ++toAdd;

        pos = nextPos;
        nextPos = temp.find ('/', nextPos);
    }
    while (pos != std::string::npos);

    // no problems found

    return (toAdd == 0) || dictionary->CanInsertThreadSafely (toAdd, path.size());
}

index_t CDictionaryBasedPath::GetDepth() const
{
    if (IsValid())
    {
        index_t result = 0;
        for (index_t i = index; i != 0; i = dictionary->GetParent (i))
            ++result;

        return result;
    }
    else
        return static_cast<index_t>(NO_INDEX);
}

bool CDictionaryBasedPath::IsSameOrParentOf ( index_t lhsIndex
											, index_t rhsIndex) const
{
	// the root is always a parent of / the same as rhs

	if (lhsIndex == 0)
		return true;

	// crawl rhs up to the root until we find it to be equal to *this

	for (; rhsIndex >= lhsIndex; rhsIndex = dictionary->GetParent (rhsIndex))
	{
		if (lhsIndex == rhsIndex)
			return true;
	}

	// *this has not been found among rhs' parent paths

	return false;
}

// convert to string

void CDictionaryBasedPath::GetPath (std::string& result) const
{
    if (index == NO_INDEX)
    {
#ifdef _DEBUG
        // only used to set _path to a proper value

        static const std::string noPath ("<INVALID_PATH>");
        assert (_path.empty() || (_path == noPath));

        result = noPath;
        return;
#else
        // an assertion is of little use here ...

        throw CContainerException ("Access to invalid path object");
#endif
    }

	// fetch all path elements bottom-up except the root
	// and calculate the total string length

	const char* pathElements [MAX_PATH];
	index_t sizes[MAX_PATH];
    size_t depth = 0;

	size_t size = 0;
	for ( index_t currentIndex = index
		; (currentIndex != 0) && (depth < MAX_PATH)
		; currentIndex = dictionary->GetParent (currentIndex))
	{
		pathElements[depth] = dictionary->GetPathElement (currentIndex);
        sizes[depth] = dictionary->GetPathElementSize (currentIndex);
        size += sizes[depth] + 1;
        ++depth;
	}

	// build result

    result.clear();
    result.resize (std::max ((size_t)1, size), '/');
    char* target = &result[0];

	for (size_t i = depth; i > 0; --i)
	{
        memcpy (++target, pathElements[i-1], sizes[i-1]);
        target += sizes[i-1];
	}
}

std::string CDictionaryBasedPath::GetPath() const
{
    std::string result;
    GetPath (result);
	return result;
}

CDictionaryBasedPath CDictionaryBasedPath::GetCommonRoot (index_t rhsIndex) const
{
	assert ((index != NO_INDEX) && (rhsIndex != NO_INDEX));

	index_t lhsIndex = index;

	while (lhsIndex != rhsIndex)
	{
		// the parent has *always* a smaller index
		// -> a common parent cannot be larger than lhs or rhs

		if (lhsIndex < rhsIndex)
		{
			rhsIndex = dictionary->GetParent (rhsIndex);
		}
		else
		{
			lhsIndex = dictionary->GetParent (lhsIndex);
		}
	}

	return CDictionaryBasedPath (dictionary, lhsIndex);
}

bool CDictionaryBasedPath::Contains (index_t pathElementID) const
{
    for ( index_t pathIndex = index
        ; pathIndex != 0
        ; pathIndex = dictionary->GetParent (pathIndex))
    {
        if (dictionary->GetPathElementID (pathIndex) == pathElementID)
            return true;
    }

    // not found

    return false;
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}
