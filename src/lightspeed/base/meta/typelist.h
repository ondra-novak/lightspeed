/*
 * typelists.h
 *
 *  Created on: 3.5.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_META_TYPELISTS_H_
#define LIGHTSPEED_META_TYPELISTS_H_


namespace TLists {


///Empty class used as terminator of list
struct NilT {
	typedef NilT This;
	static const unsigned int length = 0;

};


///Internal namespace
namespace _internal {

				struct MTrue {int padding[10];};
				struct MFalse {};


				template<class From, class To>
				struct MIsConvertible {

					static MTrue _check(To x);
					static MFalse _check(...);
					static From x;

					static const bool value = sizeof(_check(x)) == sizeof(MTrue);
				};

				template<class From>
				struct MIsConvertible<From, bool> {
					static const bool value = false;
				};

				template<class X>
				struct MIsConvertible<X, X> {
					static const bool value = true;
				};

				template<>
				struct MIsConvertible<bool, bool> {
					static const bool value = true;
				};


}



///Declaration of list node
/**
 * @tparam Head contains item of list, the head item. Each list contains
 * head item and tail, where tail is also list
 * @tparam Tail contains rest of list, which must be aslo List containing
 * head and tail item. It can be also Empty to define end of list
 */
template<typename Head, typename Tail = NilT>
struct Node {

	typedef Node<Head,Tail> This;


	typedef Head H;
	typedef typename Tail::This T;

	///Contains length of list (terminator is not included).
	static const unsigned int length = T::length + 1;

};

template<typename List, unsigned int i>
struct TypeAt {
	typedef  typename TypeAt<typename List::T,i-1>::T T;
};
template<typename List>
struct TypeAt<List,0> {
	typedef typename List::H T;
};


template<typename Head,typename Tail>
struct OptiNode {
	typedef Node<Head,typename OptiNode<typename Tail::H, typename Tail::T>::T > T;
};

template<typename Tail>
struct OptiNode<NilT,Tail> {
	typedef NilT T;
};


template<
		typename T1,typename T2 = NilT,typename T3 = NilT,typename T4 = NilT,
		typename T11 = NilT,typename T12 = NilT,typename T13 = NilT,typename T14 = NilT,
		typename T21 = NilT,typename T22 = NilT,typename T23 = NilT,typename T24 = NilT,
		typename T31 = NilT,typename T32 = NilT,typename T33 = NilT,typename T34 = NilT,
	typename T41 = NilT,typename T42 = NilT,typename T43 = NilT,typename T44 = NilT>
struct TList: OptiNode<T1,Node<T2,Node<T3,Node<T4,
			     Node<T11,Node<T12,Node<T13,Node<T14,
			     Node<T21,Node<T22,Node<T23,Node<T24,
			     Node<T31,Node<T32,Node<T33,Node<T34,
			     Node<T41,Node<T42,Node<T43,Node<T44
			      > > > > > > > > > > > > > > > > > > > >::T {};

template<typename T1>
struct TList<T1,NilT,NilT,NilT,NilT,NilT,NilT,NilT,NilT,NilT,
				NilT,NilT,NilT,NilT,NilT,NilT,NilT,NilT,NilT,NilT>
	:Node<T1> {};


///Calculates concatenation of two lists
/**
 * @tparam List1 first list
 * @tparam List@ second list
 * @return result is directly the type constructed using this template
 */
template<typename List1, typename List2>
struct Concat: Node<typename List1::H, Concat<typename List1::T,List2> > {};
template<typename List2>
struct Concat<NilT,List2>: List2::This {};


template<typename L>
struct Tuple;

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




		template<typename Tuple, typename TName, typename T, bool canConv>
		struct SetGet {
			static const T *get(const Tuple *owner, TName fldNames, TName curName) {
				return SetGet<typename Tuple::Super, TName, T,
					MIsConvertible< const typename Tuple::Super::T *, const T *>::value
						>::get(owner,fldNames+1,curName);
			}
			static bool set(Tuple *owner, TName fldNames, TName curName, const T &value) {
				return SetGet<typename Tuple::Super, TName, T,
					MIsConvertible<T, typename Tuple::Super::T>::value
						>::set(owner,fldNames+1,curName,value);
			}
		};

		template<typename TName, typename T, bool canConv>
		struct SetGet<Tuple<NilT>,TName,T,canConv > {
			static const T *get(const Tuple<NilT> *owner, TName fldNames, TName curName) {
				return 0;
			}
			static bool set(Tuple<NilT> *owner, TName fldNames, TName curName, const T &value) {
				return false;
			}
		};

		template<typename TName, typename T>
		struct SetGet<Tuple<NilT>,TName,T,true > {
			static const T *get(const Tuple<NilT> *owner, TName fldNames, TName curName) {
				return 0;
			}
			static bool set(Tuple<NilT> *owner, TName fldNames, TName curName, const T &value) {
				return false;
			}
		};

		template<typename Tuple, typename TName, typename T>
		struct SetGet<Tuple,TName,T,true> {
			static const T *get(const Tuple *owner, TName fldNames, TName curName) {
				if (fldNames == curName)
					return &owner->value;
				else
					return SetGet<typename Tuple::Super, TName, T,
					MIsConvertible< const typename Tuple::Super::T *, const T *>::value
						>::get(owner,fldNames+1,curName);
			}
			static bool set(Tuple *owner, TName fldNames, TName curName, const T &value) {
				if (fldNames == curName) {
					owner->value = value;
					return true;
				} else
					return SetGet<typename Tuple::Super, TName, T,
					MIsConvertible< T, typename Tuple::Super::T>::value
						>::set(owner,fldNames+1,curName,value);
			}
		};

		template<typename Tuple, typename TName>
		class NameHlp {
		public:

			NameHlp(Tuple *owner,TName fldNames,TName curName)
				:owner(owner),fldNames(fldNames),curName(curName) {}

			template<typename T>
			const T *asPtr() const {
				const T *res = SetGet<Tuple,TName,T,
					MIsConvertible< const typename Tuple::T *, const T *>::value
					>::get(owner,fldNames,curName);
				return res;
			}

			template<typename T>
			const T &as() const {
				const T *res = SetGet<Tuple,TName,T,
					MIsConvertible< const typename Tuple::T *, const T *>::value
					>::get(owner,fldNames,curName);
				if (res == 0)
					throw; //TODO: not found exception
				return *res;
			}

			template<typename T>
			bool set(const T &value) {
				return SetGet<Tuple,TName,T,
					MIsConvertible<T, typename Tuple::T>::value
					>::set(owner,fldNames,curName,value);
			}


			template<typename T>
			const T &operator=(const T &value) {
				if (!set(value)) {
					throw; //TODO: not found exception
				}
				return value;
			}

			template<typename T>
			operator const T &() const {
				const T *res = asPtr<T>();
				if (res == 0)
					throw; //TODO: not found exception
				return *res;
			}

			template<typename T>
			operator T () const {return (const T &)(*this);}

		protected:
			Tuple *owner;
			TName fldNames;
			TName curName;

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
class Tuple: public Tuple<typename L::T> {
public:
	typedef Tuple<typename L::T> Super;
	typedef L SrcList;
	typedef typename L::H T;
	typedef Super Nx;

	///Contains value of head item (first item). All other values are stored in the super class
	T value;

	///Constructs tuple and all fields using default constructors
	Tuple() {}

	///Construct tuple by specifying value of head item and instance of tail
	/**
	 * @param value new value for head item
	 * @param other instance to initialize the tail
	 * @return
	 */
	Tuple(const T &value, const Super &other):Super(other),value(value) {}


	Tuple(const T &a):value(a) {}
	Tuple(const T &a, const typename Nx::T &b)
		:Super(b),value(a) {}
	Tuple(const T &a, const typename Nx::T &b, const typename Nx::Nx::T &c)
		:Super(b,c),value(a) {}
	Tuple(const T &a, const typename Nx::T &b, const typename Nx::Nx::T &c,
		const typename Nx::Nx::Nx::T &d)
		:Super(b,c,d),value(a) {}
	Tuple(const T &a, const typename Nx::T &b, const typename Nx::Nx::T &c,
		const typename Nx::Nx::Nx::T &d,const typename Nx::Nx::Nx::Nx::T &e)
		:Super(b,c,d,e),value(a) {}
	Tuple(const T &a, const typename Nx::T &b, const typename Nx::Nx::T &c,
		const typename Nx::Nx::Nx::T &d,const typename Nx::Nx::Nx::Nx::T &e,
		const typename Nx::Nx::Nx::Nx::Nx::T &f)
		:Super(b,c,d,e,f),value(a) {}
	Tuple(const T &a, const typename Nx::T &b, const typename Nx::Nx::T &c,
		const typename Nx::Nx::Nx::T &d,const typename Nx::Nx::Nx::Nx::T &e,
		const typename Nx::Nx::Nx::Nx::Nx::T &f,
		const typename Nx::Nx::Nx::Nx::Nx::Nx::T &g,
		const typename Nx::Nx::Nx::Nx::Nx::Nx::Nx::T &h)
		:Super(b,c,d,e,f,g,h),value(a) {}

	///Constructs tuple as a copy of another
	/**
	 * @param other source used for copying
	 */
	Tuple(const Tuple &other):Super(other),value(other.value) {}

	///Retrieves type at specified position
	/**
	 * @tparam index position in the list.
	 * @return Return value is typedef named as T. So use typename TypeAt<N>::T, where
	 * N is position.
	 */
	template<unsigned int index>
	struct TypeAt: TLists::TypeAt<SrcList,index> {};


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
	void serializeWithName(Archive &arch,const FieldName &fieldList) {
		arch(value,fieldList);
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

	///Retrieves next field specified by its type
	/**
	 * @tparam Type specified type which will be retrieved
	 * @tparam index zero based order of field relative to its type.
	 * @return reference to variable of specified type.
	 */
	template<typename Type, unsigned int index>
	const Type &byTypeNext() const {
		return _internal::FindHelper<Tuple,Type,index>::find(*this,index);
	}

	///Retrieves next field specified by its type
	/**
	 * @tparam Type specified type which will be retrieved
	 * @tparam index zero based order of field relative to its type.
	 * @return reference to variable of specified type.
	 */
	template<typename Type, unsigned int index>
	Type &byTypeNext()  {
		return _internal::FindHelper<Tuple,Type,index>::find(*this,index);
	}


	///Calls functor for each field
	/**
	 * @param fn functor. It must accept one parameter and must return boolean
	 * @retval true function has been stopped, because functor returned true
	 * @retval false function finished reaching end of list (functor returned false)
	 */
	template<typename Functor>
	bool forEach(Functor fn) const {
		if (fn(value)) return true;
		return Super::forEach(fn);
	}

	///Calls functor for each field
	/**
	 * @param fn functor. It must accept one parameter and must return boolean
	 * @retval true function has been stopped, because functor returned true
	 * @retval false function finished reaching end of list (functor returned false)
	 */
	template<typename Functor>
	bool forEach(Functor fn)  {
		if (fn(value)) return true;
		return Super::forEach(fn);
	}

	///Calls functor for each field in reverse order
	/**
	 * @param fn functor. It must accept one parameter and must return boolean
	 * @retval true function has been stopped, because functor returned true
	 * @retval false function finished reaching end of list (functor returned false)
	 */
	template<typename Functor>
	bool forEachRev(Functor fn) const {
		if (Super::forEach(fn)) return true;
		return  fn(value);
	}

	///Calls functor for each field in reverse order
	/**
	 * @param fn functor. It must accept one parameter and must return boolean
	 * @retval true function has been stopped, because functor returned true
	 * @retval false function finished reaching end of list (functor returned false)
	 */
	template<typename Functor>
	bool forEachRev(Functor fn)  {
		if (Super::forEach(fn)) return true;
		return fn(value);
	}


	///Access fields by index
	/**
	 * Allows access fields as array where each field is accessible by zero based index
	 *
	 *
	 * @param index zero-based index
	 * @return temporary object that allows to access result. You can assign
	 *  	result to the variable or you can put result to the left of
	 *  	assignment operator to make write into the variable. Note that
	 *  	returned object "has no type". You have to convert it first, if
	 *  	conversion is not implicit. To make conversion, use as<> function,
	 *  	you will receive const reference to the value.
	 *  @exception BadCastException when conversion cannot be performed.
	 *  @note function has complexity O(N). It is better to use field template
	 *   function. Only when index cannot be constant, this function will be useful.
	 */
	_internal::NameHlp<Tuple<L>, unsigned int> operator[](unsigned int index) {
		return _internal::NameHlp<Tuple<L>, unsigned int>(this,0,index);
	}
	///Access fields by index
	/**
	 * Allows access fields as array where each field is accessible by zero based index
	 *
	 *
	 * @param index zero-based index
	 * @return temporary object that allows to access result. You can assign
	 *  	result to the variable. Note that
	 *  	returned object "has no type". You have to convert it first, if
	 *  	conversion is not implicit. To make conversion, use as<> function,
	 *  	you will receive const reference to the value.
	 *  @exception BadCastException when conversion cannot be performed.
	 *  @note function has complexity O(N). It is better to use field template
	 *   function. Only when index cannot be constant, this function will be useful.
	 */
	const _internal::NameHlp<Tuple<L>, unsigned int> operator[](unsigned int index) const {
		return _internal::NameHlp<Tuple<L>, unsigned int>(
				const_cast<Tuple<L> *>(this),0,index);
	}



};

///Named tuple is class where each field has assigned name
/**
 * @tparam L List definition
 * @tparam FldNameType specify type used to define name of field. The class
 * must have following features. Compare operator to allow compare
 * each instances, and operator+ to access next name in the table.
 *
 * Accessing names is implemented through operator+(int). Instance
 * is passed to the template when points to the first name. After using
 * operator+(1), function returns instance that points to second name,
 * and so on.
 */
template<typename L, typename FldNameType, const FldNameType &fields>
struct NamedTuple: public Tuple<L> {


	///Constructs default tuple
	NamedTuple() {}
	////Constructs and initialize tuple
	template<typename T> NamedTuple(const T &other):Tuple<L>(other) {}
	////Constructs and initialize tuple
	template<typename T,typename U>
	NamedTuple(const T &value, const U &other):Tuple<L>(value,other) {}

	///Perform serialization with the names
	template<typename Archive>
	void serialize(Archive &arch) {
		Tuple<L>::serializeWithName(arch,fields);
	}

	///Retrieve field with given name
	_internal::NameHlp<Tuple<L>, FldNameType> operator()(const FldNameType &name) {
		return _internal::NameHlp<Tuple<L>, FldNameType>(this,fields,name);
	}
	///Retrieve field with given name
	const _internal::NameHlp<Tuple<L>, FldNameType> operator()(const FldNameType &name) const {
		return _internal::NameHlp<Tuple<L>, FldNameType>(
				const_cast<Tuple<L> *>(this),fields,name);
	}

};

template<typename L, typename FldNameType, const FldNameType &fields>
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



template<> class Tuple<NilT> {
public:
	typedef NilT SrcList;

	typedef NilT T;
	typedef Tuple<NilT> Nx;

	Tuple() {}


	template<typename T> Tuple(const T &other) {}

	template<typename Archive>
	void serialize(Archive &arch) {}

	template<typename Functor>
	bool forEach(Functor fn)  const {return false;}

	template<typename Functor>
	bool forEachRev(Functor fn)  const {return false;}

	template<typename Archive, typename FieldName>
	void serializeWithName(Archive &arch, const FieldName &fieldList) {}
};




template<typename T, typename CmpFunctor>
struct TStrName {
	const T **name;
	const T *tmpName;
	CmpFunctor cmp;

	TStrName(const T *name):name(&tmpName),tmpName(name) {}
	TStrName(const T **name):name(name) {}
	TStrName(const T *name, CmpFunctor cmp):name(&tmpName),tmpName(name),cmp(cmp) {}
	TStrName(const T **name, CmpFunctor cmp):name(name),cmp(cmp) {}
	operator const T *() const {return *name;}

	bool operator==(const TStrName &other) const {
		return cmp(*name,*other.name);}
	TStrName operator+(int cnt) const {
		return TStrName(name+cnt);
	}
};

template<typename T>
struct TStrNameCmp {
	bool operator()(const T *a, const T *b) {
		do {
			if (*a != *b) return false;
			if (*a == 0) return true;
			++a;++b;
		} while(true);
	}
};

typedef TStrName<char, TStrNameCmp<char> > StrNameA;
typedef TStrName<wchar_t, TStrNameCmp<wchar_t> > StrNameW;


template<typename List, unsigned int pos>
struct Cut: Cut<typename List::T, pos - 1> {};
template<typename List>
struct Cut<List,0>:List::This {};

}

namespace LightSpeed {
using namespace TLists;
}



#endif /* TYPELISTS_H_ */
