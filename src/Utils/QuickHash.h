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

#include "auto_buffer.h"

/**
 * A quick linear (array) hash index class. It requires HF to
 * provide the following interface:
 *
 * value_type		data type of the hash
 * index_type		type of the index to store the hash
 * NO_INDEX			an index type to mark empty buckets
 *
 * operator()		value_type -> size_t hash function
 * value()			index_type -> value_type
 * equal()			(value_type, index_type) -> bool
 *
 * The capacity approximately doubles by each rehash().
 * Only insertion and lookup are provided. Collisions are
 * resolved using linear probing.
 *
 * Use statistics() to monitor the cache performance.
 */
template<class HF>
class quick_hash
{
public:

	typedef typename HF::value_type value_type;
	typedef typename HF::index_type index_type;
	
	enum {NO_INDEX = (index_type)(HF::NO_INDEX)};

	struct statistics_t
	{
		size_t capacity;
		size_t used;
		size_t collisions;
		size_t max_path;
		size_t collision_path_sum;

		statistics_t()
			: capacity (1)
			, used (0)
			, collisions (0)
			, max_path (1)
			, collision_path_sum (0)
		{
		}
	};

private:

	class prime_grower
	{
	private:

		static const size_t primes[31];
		statistics_t statistics;
		size_t index;
        size_t nextStride;

	public:

		prime_grower()
			: index (0)
            , nextStride (1)
			, statistics()
		{
			statistics.capacity = primes[index];
		}

		size_t capacity() const 
		{
			return statistics.capacity;
		}

		size_t size() const 
		{
			return statistics.used;
		}
		
		size_t collisions() const 
		{
			return statistics.collisions;
		}
		
		void inserted_cleanly() 
		{
			++statistics.used;
		}

		void inserted_collision (size_t path_size) 
		{
			++statistics.used;
			++statistics.collisions;
			statistics.collision_path_sum += path_size;
			if (statistics.max_path <= path_size)
				statistics.max_path = path_size + 1;
		}

        void batch_inserted (size_t count, size_t collisions_count)
        {
			statistics.used += count;
			statistics.collisions += collisions_count;
        }

		void grow() 
		{
			statistics.capacity = primes[++index];
			statistics.collisions = 0;
			statistics.used = 0;
			statistics.collision_path_sum = 0;
			statistics.max_path = 1;
			
            nextStride = 46337 % capacity();
		}
		
		size_t map (size_t hash_value) const 
		{
			return hash_value % capacity();
		}
		
		size_t next (size_t index) const 
		{
            index += nextStride;
            if (index >= capacity())
                index -= capacity();
			return index;
		}

		const statistics_t& get_statistics() const
		{
			return statistics;
		}
	};

	index_type* data;
	prime_grower grower;
	HF hf;
	
private:

	/// check if we're allowed to add new entries to the hash
	/// without re-hashing.
	bool should_grow() const
	{
		// grow, if there are many collisions 
		// or capacity is almost exceeded
		// There must also be at least one empty entry

		return grower.size() + grower.collisions() + 1 >= grower.capacity();
	}

	bool should_grow (size_t size) const
	{
		return grower.size() + grower.collisions() + size >= grower.capacity();
	}

	/// initialize the new array before re-hashing
	void create_data()
	{
		size_t new_capacity = grower.capacity();
	
		data = new index_type[new_capacity];
		for (size_t i = 0; i < new_capacity; ++i)
			data[i] = (index_type)NO_INDEX;
	}
	
	/// add a value to the hash 
	/// (must not be in it already; hash must not be full)
	void internal_insert (size_t bucket, index_type index)
	{
		// first try: un-collisioned insertion

		index_type* target = data + bucket;
		if (*target == NO_INDEX)
		{
			*target = index;
			grower.inserted_cleanly();

			return;
		}

		// collision -> look for an empty bucket

		size_t collision_path_size = 0;
		do
		{
			bucket = grower.next (bucket);
			target = data + bucket;
			++collision_path_size;
		}
		while (*target != NO_INDEX);
		
		// insert collisioned item

		*target = index;
		grower.inserted_collision (collision_path_size);
	}
		
	void rehash (index_type* old_data, size_t old_data_size)
	{
		create_data();

		for (size_t i = 0; i < old_data_size; ++i)
		{
			index_type index = old_data[i];
			if (index != NO_INDEX)
            {
                size_t bucket = grower.map (hf (hf.value (index)));
                internal_insert (bucket, index);
            }
		}

		delete[] old_data;
	}
	
public:

	/// construction / destruction
	quick_hash (const HF& hash_function) 
		: data(NULL)
		, hf (hash_function)
		, grower()
	{
		create_data();
	}
	
	quick_hash (const quick_hash& rhs) 
		: data (NULL)
		, hf (rhs.hf)
		, grower (rhs.grower)
	{
		create_data();
		operator= (rhs);
	}
	
	~quick_hash() 
	{
		delete[] data;
	}

	/// find the bucket containing the desired value;
	/// return NO_INDEX if not contained in hash
	index_type find (const value_type& value) const
	{
		size_t bucket = grower.map (hf (value));
		index_type index = data[bucket];

		while (index != NO_INDEX)
		{
			// found?

			if (hf.equal (value, index))
				break;

			// collision -> look at next bucket position

			bucket = grower.next (bucket);
			index = data[bucket];
		}

		// either found or not in hash
		
		return index;
	}
	
	void insert (const value_type& value, index_type index)
	{
		assert (find (value) == NO_INDEX);
		
		if (should_grow())
			reserve (grower.capacity()+1);
	
		internal_insert (grower.map (hf (value)), index);
	}

	void reserve (size_t min_bucket_count)
	{
		if (size_t(-1) / sizeof (index_type[4]) > min_bucket_count)
			min_bucket_count += min_bucket_count / 2;

		index_type* old_data = data;
		size_t old_data_size = grower.capacity();
		
		while (grower.capacity() < min_bucket_count)
			grower.grow();
			
		if (grower.capacity() != old_data_size)
			rehash (old_data, old_data_size);
	}

    /// batch insertion. Start at index.

    template<class IT>
	void presorted_insert (IT first, IT last, index_type index)
    {
        // definitions

        enum {MAX_CLUSTERS = 256};
        typedef std::pair<unsigned, index_type> TPair;

        // preparation

        size_t clusterSize = std::max<size_t> (1, (last - first) / MAX_CLUSTERS);
        size_t shift = 0;
        while (((size_t)MAX_CLUSTERS << shift) < grower.capacity())
            ++shift;

        auto_buffer<TPair> tempBuffer (MAX_CLUSTERS * clusterSize);
        TPair* temp = tempBuffer.get();

        size_t used[MAX_CLUSTERS];
        memset (used, 0, sizeof (used));

        // sort main: fill bucket chains

        for (; first != last; ++first)
        {
            size_t bucket = grower.map (hf (*(first)));
            size_t clusterIndex = bucket >> shift;

            size_t offset = used[clusterIndex] + clusterIndex * clusterSize;

            TPair& current = temp[offset];
            current.first = static_cast<index_type>(bucket);
            current.second = index++;

            if (++used[clusterIndex] == clusterSize)
            {
                used[clusterIndex] = 0;

                // write data

	            size_t collisions_count = 0;
                for ( TPair* iter = temp + clusterIndex * clusterSize
                    , *end = iter + clusterSize
                    ; iter != end
                    ; ++iter)
                {
                    size_t bucket = iter->first;
		            index_type* target = data + bucket;

		            while (*target != NO_INDEX)
		            {
			            bucket = grower.next (bucket);
			            target = data + bucket;
			            ++collisions_count;
                    }

                    *target = iter->second;
                }

                grower.batch_inserted (clusterSize, collisions_count);
            }
        }

        // write remaining data

        for (int i = 0; i < MAX_CLUSTERS; ++i)
        {
            size_t collisions_count = 0;
            for ( TPair* iter = temp + i * clusterSize
                , *end = iter + used[i]
                ; iter != end
                ; ++iter)
            {
                size_t bucket = iter->first;
	            index_type* target = data + bucket;

	            while (*target != NO_INDEX)
	            {
		            bucket = grower.next (bucket);
		            target = data + bucket;
		            ++collisions_count;
                }

                *target = iter->second;
            }

            grower.batch_inserted (used[i], collisions_count);
        }
    }

    template<class IT>
	void insert (IT first, IT last, index_type index)
	{
        size_t count = last - first;
		if (should_grow (count))
			reserve (grower.size() + count);

        // small numbers should be added using conventional methods

        if (count < 1000)
        {
            for (; first != last; ++first, ++index)
                internal_insert (grower.map (hf (*first)), index);
        }
        else
        {
            presorted_insert (first, last, index);
        }
	}

	/// assignment

	quick_hash& operator=(const quick_hash& rhs)
	{
		if (grower.capacity() != rhs.grower.capacity())
		{
			delete[] data;
			data = new index_type [rhs.grower.capacity()];
		}

		grower = rhs.grower;

		for ( index_type* source = rhs.data, *target = data
			, *end = source + rhs.grower.capacity()
			; source != end
			; ++source, ++target)
		{
			*target = *source;
		}

		return *this;
	}

	/// get rid of all entries

	void clear()
	{
		if (grower.size() > 0)
		{
			delete[] data;
			grower = prime_grower();
			create_data();
		}
	}

	/// efficiently exchange two containers

	void swap (quick_hash& rhs)
	{
		std::swap (data, rhs.data);

		prime_grower temp = grower;
		grower = rhs.grower;
		rhs.grower = temp;
	}

	/// read cache performance statistics
	const statistics_t& statistics() const
	{
		return grower.get_statistics();
	}

    /// test, whether a given numbers of additional entries
    /// might cause the cache to be resized

    bool may_cause_growth (size_t toAdd) const
    {
        size_t maxEffectiveSize 
            = grower.size() + grower.collisions() + 2 * toAdd;

        return maxEffectiveSize >= grower.capacity();
    }

};

template<class HF>
const size_t quick_hash<HF>::prime_grower::primes[31] = 
	{1, 3, 7, 17, 
	 31, 67, 127, 257, 
	 509, 1021, 2053, 4099,
	 8191, 16381, 32771, 65537,
	 131071, 262147, 524287, 1048573,
	 2097143, 4194301, 8388617, 16777213,
	 33554467, 67108859, 134217757, 268435459,
	 536870909, 1073741827};

