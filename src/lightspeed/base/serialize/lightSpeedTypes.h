#ifndef LIGHTSPEED_SERIALIZE_LSTYPES_H_
#define LIGHTSPEED_SERIALIZE_LSTYPES_H_

#include "basicTypes.h"
#include "../containers/autoArray.h"
#include "../memory/smallAlloc.h"
#include "../meta/emptyClass.h"

namespace LightSpeed
{

	template<typename Itm>
	class SerializableString {
	public:
		

		template<typename StringType, typename Archive>
		static void serialize(StringType &object, Archive &arch) {

			typename Archive::Array arr(arch,object.length());
			if (arch.loading()) {
				typedef AutoArrayStream<Itm, SmallAlloc<1024/sizeof(Itm)+1> > MemStream;
				MemStream tmp;
				while (arr.next()) {
					Itm x;
					arch(x);
					tmp.write(x);
				}
				typename StringType::WriteIterator iter 
							= object.createBufferIter(tmp.length());
				typename MemStream::Iterator rditer = tmp.getFwIter();
				iter.copy(rditer);
			} else if (arch.storing()) {
				typename StringType::Iterator iter = object.getFwIter();
				while (arr.next()) {
					Itm x = iter.getNext();
					arch(x);
					arr.next();
				}
			}
		}
	};

	template<>
	class Serializable<String>: public SerializableString<wchar_t> {};
	template<template<class> class Cmp>
	class Serializable<UniString<Cmp> >: public SerializableString<wchar_t> {};
	template<typename T>
	class Serializable<StringCore<T> >: public SerializableString<T> {};
	template<typename T, template<class> class Cmp>
	class Serializable<StringTC<T,Cmp> >: public SerializableString<T> {};

	template<>
	class Serializable<Empty> {
	public:
		template<typename Archive>
		static void serialize(Empty &object, Archive &arch) {
		}
	};

	template<typename T, typename Allocator>
	class Serializable<AutoArray<T,Allocator> > {

	public:
		template<typename Archive>
		static void serialize(AutoArray<T,Allocator> &object, Archive &arch) {
			typename Archive::Array arrsect(arch,object.length());
			if (arch.loading()) {
				object.clear();
				if (arrsect.length() != naturalNull)
					object.reserve(arrsect.length());
				typename AutoArray<T,Allocator>::WriteIter iter
							= object.getWriteIterator();
				while (arrsect.next()) {
					T itm;
					arch(itm); 
					iter.write(itm);
				}
			} else if (arch.storing()) {
				typename AutoArray<T,Allocator>::Iterator iter = object.getFwIter();
				while (iter.hasItems() && arrsect.next()) {
					T &x = const_cast<T &>(iter.getNext());
					arch(x);
				}
			}
		}
	};

}
#endif
