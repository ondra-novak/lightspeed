/** @file
 * Copyright (c) 2006, Seznam.cz, a.s.
 * All Rights Reserved.
 * 
 * $Id: typeconv.h 240 2011-10-18 12:51:20Z ondrej.novak $
 *
 *
 * DESCRIPTION
 * Short description
 * 
 * AUTHOR
 * Ondrej Novak <ondrej.novak@firma.seznam.cz>
 *
 */


#ifndef LIGHTSPEED_ITER_TYPECONV_H_
#define LIGHTSPEED_ITER_TYPECONV_H_
#include "iterator.h"
#include "../containers/optional.h"

#pragma once

namespace LightSpeed {

	///Converts items in the stream during reading
	/** Iterator fetches data from the other iterator, converts them
	 * and returns in converted state. There must be default conversion available
	 * for the new type
	 *
	 * @tparam Iter source iterator, can be defined also as reference
	 * @tparam TargetType new type required as output.
	 *
	 * Iterator contains storage for one converted object, and returns
	 * reference to this object.
	 */
	template<typename Iter, typename TargetType>
	class TypeConvReader:
		public IteratorBase<TargetType,TypeConvReader<Iter, TargetType> > {

	public:

		typedef TargetType T;

		bool hasItems() const;
        natural getRemain() const;
        bool equalTo(const TypeConvReader &other) const;
        bool lessThan(const TypeConvReader &other) const;
        const T &getNext();
        const T &peek() const;

	    void skip();

	    TypeConvReader(Iter iter):iter(iter) {}
	protected:
	    Iter iter;
	    Optional<TargetType> store;
	};


	///Converts items in the stream during writing
	/** Iterator converts data written as one type to the other, Input type
	 * is defined as template argument. Required type is taken from the iterator
	 *
	 * @tparam Iter source iterator, can be defined also as reference
	 * @tparam TargetType new type required as input.
	 *
	 */

	template<typename Iter, typename TargetType>
	class TypeConvWriter:
		public WriteIteratorBase<TargetType,TypeConvReader<Iter, TargetType> > {

	public:

		typedef TargetType T;

		bool hasItems() const;
		natural getRemain() const;
		bool equalTo(const TypeConvWriter &other) const;
		bool lessThan(const TypeConvWriter &other) const;
		void write(const T &item);

		void skip();
		void flush();
		void reserve(natural itemCount);
		bool canAccept(const T &x) const;

	    TypeConvWriter(Iter iter):iter(iter) {}
	protected:
	    Iter iter;


	};


template<typename Iter, typename TargetType>
bool TypeConvReader<Iter,TargetType>::hasItems() const
{
	return iter.hasItems();
}



template<typename Iter, typename TargetType>
natural TypeConvReader<Iter,TargetType>::getRemain() const
{
	return iter.getRemain();
}



template<typename Iter, typename TargetType>
bool TypeConvReader<Iter,TargetType>::equalTo(const TypeConvReader & other) const
{
	return iter.equalTo(other.iter);
}



template<typename Iter, typename TargetType>
bool TypeConvReader<Iter,TargetType>::lessThan(const TypeConvReader & other) const
{
	return iter.lessThan(other.iter);
}



template<typename Iter, typename TargetType>
const TargetType & TypeConvReader<Iter,TargetType>::getNext()
{
	store.set((TargetType)iter.getNext());
	return *store;
}



template<typename Iter, typename TargetType>
const TargetType & TypeConvReader<Iter,TargetType>::peek() const
{
	store.set((TargetType)iter.peek());
	return *store;
}



template<typename Iter, typename TargetType>
void TypeConvReader<Iter,TargetType>::skip()
{
	iter.skip();
}



template<typename Iter, typename TargetType>
bool TypeConvWriter<Iter,TargetType>::hasItems() const
{
	return iter.hasItems();
}


template<typename Iter, typename TargetType>
 natural TypeConvWriter<Iter,TargetType>::getRemain() const
{
	return iter.getRemain();
}



template<typename Iter, typename TargetType>
bool TypeConvWriter<Iter,TargetType>::equalTo(const TypeConvWriter & other) const
{
	return iter.equalTo(other.iter);
}



template<typename Iter, typename TargetType>
bool TypeConvWriter<Iter,TargetType>::lessThan(const TypeConvWriter & other) const
{
	return iter.lessThan(other.iter);
}



template<typename Iter, typename TargetType>
void TypeConvWriter<Iter,TargetType>::write(const T & item)
{
	typedef typename OriginT<Iter>::T::ItemT ItemT;
	iter.write((ItemT)item);
}



template<typename Iter, typename TargetType>
void TypeConvWriter<Iter,TargetType>::skip()
{
	iter.skip();
}



template<typename Iter, typename TargetType>
void TypeConvWriter<Iter,TargetType>::flush()
{
	iter.flush();
}



template<typename Iter, typename TargetType>
void TypeConvWriter<Iter,TargetType>::reserve(natural itemCount)
{
	iter.reserve(itemCount);
}



template<typename Iter, typename TargetType>
bool TypeConvWriter<Iter,TargetType>::canAccept(const T & x) const
{
	return iter.canAccept(x);
}

}


#endif /* LIGHTSPEED_ITER_TYPECONV_H_ */
