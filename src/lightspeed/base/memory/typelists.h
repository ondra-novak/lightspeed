/*
 * typelists.h
 *
 *  Created on: 3.5.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_META_TYPELISTS_H_
#define LIGHTSPEED_META_TYPELISTS_H_


namespace LightSpeed {namespace TL {


///Empty class used as terminator of list
struct Empty {};


///Internal namespace
namespace _internal {

				template<typename List, unsigned int i>
				struct TypeAt {
					typedef  typename TypeAt<typename List::T,i-1>::T T;
				};
				template<typename List>
				struct TypeAt<List,0> {
					typedef typename List::H T;
				};

				template<typename List>
				struct Length {
					static const unsigned int length = Length<List>::length + 1;
				};

				template<>
				struct Length<Empty> {
					static const unsigned int length = 0;
				};
}



///Declaration of list node
/**
 * @tparam Head contains item of list, the head item. Each list contains
 * head item and tail, where tail is also list
 * @tparam Tail contains rest of list, which must be aslo List containing
 * head and tail item. It can be also Empty to define end of list
 */
template<typename Head, typename Tail = Empty>
struct List: Tail {

	typedef Head H;
	typedef Tail T;

	///Contains length of list (terminator is not included).
	static const unsigned int length = _internal::Length<List<Head,Tail> >::length;

	///Retrieves type at specified position
	/**
	 * @tparam index position in the list.
	 * @return Return value is typedef named as T. So use typename TypeAt<N>::T, where
	 * N is position.
	 */
	template<unsigned int index>
	struct TypeAt: _internal::TypeAt<List<Head,Tail>,index> {};

};


///Calculates concatenation of two lists
/**
 * @tparam List1 first list
 * @tparam List@ second list
 * @return result is directly the type constructed using this template
 */
template<typename List1, typename List2>
struct Concat: List<typename List1::H, Concat<typename List1::T,List2> > {};
template<typename List2>
struct Concat<Empty,List2>: List2 {};


namespace _internal {

			template<typename Tuple, unsigned int index>
			struct FieldHelper {
				static const typename Tuple::template TypeAt<index>::T &
					field(const Tuple &obj) {
						return FieldHelper<typename Tuple::Super, index-1>::field(obj);
					}
				static typename Tuple::template TypeAt<index>::T &
					field(Tuple &obj) {
						return FieldHelper<typename Tuple::Super, index-1>::field(obj);
					}
			};
			template<typename Tuple>
			struct FieldHelper<Tuple,0> {
				static const typename Tuple::template TypeAt<0>::T &
					field(const Tuple &obj) {
						return obj.value;
					}
				static typename Tuple::template TypeAt<0>::T &
					field(Tuple &obj) {
						return obj.value;
					}
			};

			template<typename Tuple, typename Type, unsigned int index>
			struct FindHelper {
				static const Type &find(const Tuple &obj) {
					return FindHelper<typename Tuple::Super, Type, index>::find(obj);
				}
				static Type &find(Tuple &obj) {
					return FindHelper<typename Tuple::Super, Type, index>::find(obj);
				}
			};
			template<typename Tuple,  unsigned int index>
			struct FindHelper<Tuple,typename Tuple::T, index> {
				typedef typename Tuple::T Type;
				static const Type &find(const Tuple &obj) {
					return FindHelper<typename Tuple::Super, Type, index-1>::find(obj);
				}
				static Type &find(Tuple &obj) {
					return FindHelper<typename Tuple::Super, Type, index-1>::find(obj);
				}
			};
			template<typename Tuple>
			struct FindHelper<Tuple,typename Tuple::T, 0> {
				typedef typename Tuple::T Type;
				static const Type &find(const Tuple &obj) {
					return obj.value;
				}
				static Type &find(Tuple &obj) {
					return obj.value;
				}
			};


}

///Generates tuple using the list
/**
 * @tparam L list used to construct tuple
 *
 * Tuple is similar to struct, construction using the list is more
 * automate. Tuple is declared as derived class, where superior class is
 * tail of list. To access tail, you have to cast instance to the superior class.
 *
 * The most super. class is Tuple<Empty> used as terminator
 */

template<typename L>
struct Tuple: Tuple<typename L::T> {
	typedef Tuple<typename L::T> Super;
	typedef L SrcList;
	typedef typename L::H T;

	///Contains value of head item (first item). All other values are stored in the super class
	T value;

	///Constructs tuple and all fields using default constructors
	Tuple() {}
	///Constructs tuple as a copy of another
	/**
	 * @param other source used for copying
	 */
	Tuple(const Tuple &other):Super(other),value(other.value) {}
	///Constructs tuple and initialize all fields with the same type using the parameter
	/**
	 * @param other value used to initialize fields. Note that only fields with
	 * compatible type are initialized
	 */
	Tuple(const T &other):Super(other),value(other) {}

	///Construct tuple by specifying value of head item and instance of tail
	/**
	 * @param value new value for head item
	 * @param other instance to initialize the tail
	 * @return
	 */
	Tuple(const T &value, const Super &other):Super(other),value(value) {}


	///Used to allow deliver paramters deeper into the tuple
	template<typename X>
	Tuple(const X &other):Super(other) {}

/*	template<typename X>
	Tuple(const Tuple<List<T,X> > &other)
		:Super(static_cast<const Tuple<X> &>(other))
		,value(other.value) {}
*/

	///Retrieves type at specified position
	/**
	 * @tparam index position in the list.
	 * @return Return value is typedef named as T. So use typename TypeAt<N>::T, where
	 * N is position.
	 */
	template<unsigned int index>
	struct TypeAt: SrcList::template TypeAt<index> {};


	///Returns value at given position
	/**
	 * @tparam index zero based index of position.
	 * @return const reference to the variable containing required value
	 */
	template<unsigned int index>
	const typename TypeAt<index>::T &field() const {
		return _internal::FieldHelper<Tuple,index>::field(*this);
	}
	///Returns reference at given position
	/**
	 * @tparam index zero based index of position.
	 * @return reference to the variable containing required value
	 */
	template<unsigned int index>
	typename TypeAt<index>::T &field()  {
		return _internal::FieldHelper<Tuple,index>::field(*this);
	}

	///Serializes the tuple
	template<typename Archive>
	void serialize(Archive &arch) {
		arch(value);
		Super::serialize(arch);
	}


	///Serialization with list of names
	template<typename Archive, typename FieldName>
	void serializeWithName(Archive &arch, const FieldName *fieldList) {
		arch(value,*fieldList);
		Super::serializeWithName(arch,fieldList+1);
	}


	///Retrieves field specified by its type
	/**
	 * @tparam Type specified type which will be retrieved
	 * @return reference to variable of specified type.
	 */
	template<typename Type>
	const Type &byType() const {
		return _internal::FindHelper<Tuple,Type,0>::find(*this);
	}


	///Retrieves field specified by its type
	/**
	 * @tparam Type specified type which will be retrieved
	 * @return reference to variable of specified type.
	 */
	template<typename Type>
	Type &byType()  {
		return _internal::FindHelper<Tuple,Type,0>::find(*this);
	}
	template<typename Type, unsigned int index>
	const Type &byTypeNext() const {
		return _internal::FindHelper<Tuple,Type,index>::find(*this,index);
	}

	template<typename Type, unsigned int index>
	Type &byTypeNext()  {
		return _internal::FindHelper<Tuple,Type,index>::find(*this,index);
	}

	template<typename Functor>
	bool forEach(Functor fn) const {
		if (fn(value)) return true;
		return Super::forEach(fn);
	}

	template<typename Functor>
	bool forEach(Functor fn)  {
		if (fn(value)) return true;
		return Super::forEach(fn);
	}

	template<typename Functor>
	bool forEachRev(Functor fn) const {
		if (Super::forEach(fn)) return true;
		return  fn(value);
	}

	template<typename Functor>
	bool forEachRev(Functor fn)  {
		if (Super::forEach(fn)) return true;
		return fn(value);
	}

};

template<typename L, typename FldNameType, const FldNameType *fields>
struct NamedTuple: public Tuple<L> {

	NamedTuple() {}
	template<typename T> NamedTuple(const T &other):Tuple<L>(other) {}

	template<typename T,typename U>
	NamedTuple(const T &value, const U &other):Tuple<L>(value,other) {}

	template<typename Archive>
	void serialize(Archive &arch) {
		Tuple<L>::serializeWithName(arch,fields);
	}
};

template<typename L, typename FldNameType, const FldNameType *fields>
struct StructTuple: public NamedTuple<L,FldNameType,fields> {
	typedef NamedTuple<L,FldNameType,fields> Super;
	StructTuple() {}

	template<typename T> StructTuple(const T &other):Super(other) {}
	template<typename T,typename U>
	StructTuple(const T &value, const U &other):Super(value,other) {}

	template<typename Archive>
	void serialize(Archive &arch) {
		typename Archive::Struct strt(arch);
		Super::serialize(strt);
	}
};



template<> struct Tuple<Empty> {
	typedef Empty SrcList;

	Tuple() {}

	template<typename T> Tuple(const T &other) {}

	template<typename Archive>
	void serialize(Archive &arch) {}

	template<typename Functor>
	bool forEach(Functor fn)  const {return false;}

	template<typename Functor>
	bool forEachRev(Functor fn)  const {return false;}

	template<typename Archive, typename FieldName>
	void serializeWithName(Archive &arch, const FieldName *fieldList) {}
};








template<typename H1, typename H2,  typename T = Empty>
struct List2: List<H1,List<H2,T> > {};
template<typename H1, typename H2, typename H3, typename T = Empty>
struct List3: List2<H1,H2,List<H3,T> > {};
template<typename H1, typename H2, typename H3, typename H4, typename T = Empty>
struct List4: List3<H1,H2,H3,List<H4,T> > {};

template<typename H1, typename H2, typename H3, typename H4,
		typename H5,  typename T = Empty>
struct List5: List4<H1,H2,H3,H4,List<H5,T> > {};
template<typename H1, typename H2, typename H3, typename H4,
		typename H5, typename H6, typename T = Empty>
struct List6: List5<H1,H2,H3,H4,H5,List<H6,T> > {};
template<typename H1, typename H2, typename H3, typename H4,
		typename H5, typename H6, typename H7, typename T = Empty>
struct List7: List6<H1,H2,H3,H4,H5,H6,List<H7,T> > {};
template<typename H1, typename H2, typename H3, typename H4,
		typename H5, typename H6, typename H7, typename H8, typename T = Empty>
struct List8: List7<H1,H2,H3,H4,H5,H6,H7,List<H8,T> > {};
template<typename H1, typename H2, typename H3, typename H4,
		typename H5, typename H6, typename H7, typename H8,
		typename H9,typename T = Empty>
struct List9: List8<H1,H2,H3,H4,H5,H6,H7,H8,List<H9,T> > {};
template<typename H1, typename H2, typename H3, typename H4,
		typename H5, typename H6, typename H7, typename H8,
		typename H9,typename H10,typename T = Empty>
struct List10: List9<H1,H2,H3,H4,H5,H6,H7,H8,H9,List<H10,T> > {};











}}


#endif /* TYPELISTS_H_ */
