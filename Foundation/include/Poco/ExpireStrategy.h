//
// ExpireStrategy.h
//
// $Id: //poco/1.3/Foundation/include/Poco/ExpireStrategy.h#1 $
//
// Library: Foundation
// Package: Cache
// Module:  ExpireStrategy
//
// Definition of the ExpireStrategy class.
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#ifndef  Foundation_ExpireStrategy_INCLUDED
#define  Foundation_ExpireStrategy_INCLUDED


#include "Poco/KeyValueArgs.h"
#include "Poco/ValidArgs.h"
#include "Poco/AbstractStrategy.h"
#include "Poco/Bugcheck.h"
#include "Poco/Timestamp.h"
#include "Poco/EventArgs.h"
#include <set>
#include <map>


namespace Poco {


template < 
	class TKey,
	class TValue
>
class ExpireStrategy: public AbstractStrategy<TKey, TValue>
	/// An ExpireStrategy implements time based expiration of cache entries
{
public:
	typedef std::multimap<Timestamp, TKey>     TimeIndex;
	typedef typename TimeIndex::iterator       IndexIterator;
	typedef typename TimeIndex::const_iterator ConstIndexIterator;
	typedef std::map<TKey, IndexIterator>      Keys;
	typedef typename Keys::iterator            Iterator;

public:
	ExpireStrategy(Timestamp::TimeDiff expireTimeInMilliSec): _expireTime(expireTimeInMilliSec * 1000)
		/// Create an expire strategy. Note that the smallest allowed caching time is 25ms.
		/// Anything lower than that is not useful with current operating systems.
	{
		if (_expireTime < 25000) throw InvalidArgumentException("expireTime must be at least 25 ms");  
	}

	~ExpireStrategy()
	{
	}

	void onAdd(const void*, const KeyValueArgs <TKey, TValue>& args)
	{
		Timestamp now;
		IndexIterator it = _keyIndex.insert(std::make_pair(now, args.key()));
		std::pair<Iterator, bool> stat = _keys.insert(std::make_pair(args.key(), it));
		if (!stat.second)
		{
			_keyIndex.erase(stat.first->second);
			stat.first->second = it;
		}
	}

	void onRemove(const void*, const TKey& key)
	{
		Iterator it = _keys.find(key);
		if (it != _keys.end())
		{
			_keyIndex.erase(it->second);
			_keys.erase(it);
		}
	}

	void onGet(const void*, const TKey& key)
	{
		// get triggers no changes in an expire
	}

	void onClear(const void*, const EventArgs& args)
	{
		_keys.clear();
		_keyIndex.clear();
	}

	void onIsValid(const void*, ValidArgs<TKey>& args)
	{
		Iterator it = _keys.find(args.key());
		if (it != _keys.end())
		{
			if (it->second->first.isElapsed(_expireTime))
			{
				args.invalidate();
			}
		}
	}

	void onReplace(const void*, std::set<TKey>& elemsToRemove)
	{
		// Note: replace only informs the cache which elements
		// it would like to remove!
		// it does not remove them on its own!
		IndexIterator it = _keyIndex.begin();
		while (it != _keyIndex.end() && it->first.isElapsed(_expireTime))
		{
			elemsToRemove.insert(it->second);
			++it;
		}
	}

protected:
	Timestamp::TimeDiff _expireTime;
	Keys      _keys;     /// For faster replacement of keys, the iterator points to the _keyIndex map
	TimeIndex _keyIndex; /// Maps time to key value
};


} // namespace Poco


#endif
