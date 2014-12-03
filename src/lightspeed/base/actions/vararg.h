/*
 * vararg.h
 *
 *  Created on: 4.8.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_VARARG_H_
#define LIGHTSPEED_VARARG_H_

#include "../qualifier.h"
#include "../meta/metaIf.h"
#include "../exceptions/throws.h"
#include "../meta/emptyClass.h"
#include "../meta/isConvertible.h"
#include "../containers/arrayref.h"


namespace LightSpeed {


	class IVarArg;

	template<typename V, typename N>
	struct VarArg ;

	typedef VarArg<void,void> VarArgEmpty;

	template<typename V, typename N, bool flat = false>
	class VarArgImpl;



	///One node in variable argument list
	/**
	 * @tparam T type of argument that holds
	 * @tparam N type of node of the next argument
	 *
	 * @note arguments are reversed. First argument is latest argument in the node-list
	 *
	 */
	template<typename V, typename N>
	struct VarArg {

		///Value of the node
		V value;
		///next node
		N next;

		///Contains type of the arguyment
		typedef V ItemT;
		///Contains type of the next node
		typedef N Next;
		///Contains original type of the item (removes any reference)
		typedef typename OriginT<V>::T OrgItemT;
		///Contains original type of the node (removes any reference)
		typedef typename OriginT<N>::T OrgNext;
		///Contains type used to declare argument of the function (const T &)
		typedef typename FastParam<V>::T ParamT;
		///Contains type used to declare argument of the function (const N &)
		typedef typename FastParam<N>::T ParamN;

		///Declaration 'flat' version of VarArgNode
		/**
		 * The 'flat' version is version, which is stored in signle memory block
		 * as one object without any reference
		 */
		typedef VarArg<OrgItemT, typename OrgNext::Flat> Flat;

		///Declaration 'ref' version of VarArgNode
		/**
		 * The 'ref' version is version, where all variables stored as reference
		 */
		typedef VarArg<ParamT, typename OrgNext::Ref> Ref;

		///Declaration 'pref' version of VarArgNode
		/**
		 * The 'pref' version is version, where all variables stored as reference and nodes are referenced
		 * this mode equal result of using operator comma while arguments are built
		 */
		typedef VarArg<ParamT, typename FastParam<typename OrgNext::PRef>::T> PRef;


		///Constructs node using two arguments
		/**
		 * @param x value stored into this node
		 * @param nx remain of list - constructor is called recursiveli
		 */
		VarArg(ParamT x, ParamN nx):value(x),next(nx) {}

		///Construct node from PRef version - build 
		VarArg(const PRef &oth)
			:value(oth.value),next(oth.next) {}
		VarArg(const Ref &oth)
			:value(oth.value),next(oth.next) {}
		VarArg(const Flat &oth)
			:value(oth.value),next(oth.next) {}

		///Allows to constructs node using a node of different type
		/**
		 * @param oth source node can be node of different type. It need to
		 * have value of compatible type and need to have same count of the
		 * items as constructed node
		 */
		template<typename A, typename B>
		explicit VarArg(const VarArg<A,B> &oth)
			:value(oth.value),next(oth.next) {}

		///Contains count of items stored in whole list starting by this node
		/** It can be also used as zero-based index of current node */
		static const natural count = OrgNext::count + 1;

		///Position of this argument
		static const natural position = count - 1;

		///Contains true, when whole node is flat
		/**
		 * Flat note has no reference to the other nodes or values. Can be used
		 * as standalone object. Not-flat node can refer another node or value
		 * using reference. This can cause crash when refered node is destroyed
		 */
		static const bool isFlat = MIsSame<V,OrgItemT>::value && MIsSame<N,OrgNext>::value
				&& OrgNext::isFlat;


		static const bool empty = false;
		///Finds node and returns its type
		/**
		 * @tparam n zero-based position of node.
		 */
		template<natural n>
		struct FindNode {
			///Contains type of the node
			typedef typename MIf<n == position, VarArg, typename OrgNext::template FindNode<n>::T >::T T;

		};

		template<natural n>
		struct TypeOf {
			///Contains type of the node
			typedef typename FindNode<n>::T::ItemT T;

		};

		///Finds and returns node defined by zero-based order
		/**
		 * @tparam n zero-based position of the node
		 * @return whole node
		 */
		template<natural n>
		inline const typename FindNode<n>::T &findNode() const {
			return findNode<n>(MIf<n == position,MTrue,MFalse>());
		}

		///Finds and returns node defined by zero-based order
		/**
		 * @tparam n zero-based position of the node
		 * @return whole node
		 */
		template<natural n>
		inline typename FindNode<n>::T &findNode() {
			return findNode<n>(MIf<n == position,MTrue,MFalse>());
		}

		///Returns value od node specified zero-based index
		/**
		 * @tparam n zero-based position of the node
		 * @return value of the node
		 */
		template<natural n>
		inline const typename FindNode<n>::T::ItemT &getValue() const {
			return findNode<n>().value;
		}
		///Returns value od node specified zero-based index
		/**
		 * @tparam n zero-based position of the node
		 * @return value of the node
		 */
		template<natural n>
		inline typename FindNode<n>::T::ItemT &getValue() {
			return findNode<n>().value;
		}

		///Calls functor for each argument in the list
		/**
		 * @param fn functor. Functor has zero argument and returns true, to stop
		 * 	  cycle, or false to continue.
		 * @retval true functor returned true and enumeration has been stopped
		 * @retval false processed all items
		 */
		template<typename Fn>
		inline bool forEach(Fn fn) const {
			if (next.forEach(fn)) return true;
			return fn(value);
		}

		///Calls functor for each argument in the list
		/**
		 * @param fn functor. Functor has zero argument and returns true, to stop
		 * 	  cycle, or false to continue.
		 * @retval true functor returned true and enumeration has been stopped
		 * @retval false processed all items
		 */
		template<typename Fn>
		inline bool forEach(Fn fn) {
			if (next.forEach(fn)) return true;
			return fn(value);
		}

		///Construct variable argument list using comma operator
		template<typename X>
		VarArg<const X &,const VarArg<V,N> &> operator,(const X &arg) const  {
			return VarArg<const X &,const VarArg<V,N> &>(arg,*this);
		}
		///Construct variable argument list using | operator
		template<typename X>
		VarArg<const X &,const VarArg<V,N> &> operator|(const X &arg) const  {
			return VarArg<const X &,const VarArg<V,N> &>(arg,*this);
		}

		///Construct variable argument list using () operator
		template<typename X>
		VarArg<const X &,const VarArg<V,N> &> operator()(const X &arg) const  {
			return VarArg<const X &,const VarArg<V,N> &>(arg,*this);
		}

		///Construct variable argument list chaining call of val()  function
		template<typename X>
		VarArg<const X &,const VarArg<V,N> &> val(const X &arg) const  {
			return VarArg<const X &,const VarArg<V,N> &>(arg,*this);
		}

		///Construct reference to a variable
		template<typename X>
		VarArg<X &,const VarArg<V,N> &> ref(X &arg) const  {
			return VarArg< X &,const VarArg<V,N> &>(arg,*this);
		}
		///Construct reference to a variable
		template<typename X>
		VarArg<X &,const VarArg<V,N> &> operator &(X &arg) const  {
			return VarArg< X &,const VarArg<V,N> &>(arg,*this);
		}

		///Converts current object to object which implements IVarArg interface
		/**
		 * Use when function requires IVarArg interface
		 * @return object, which implements IVarArg interface
		 */
		VarArgImpl<V,N> ivararg() const;

		///Retrieves value defined with specified field
		/**
		 * @param fieldName name or ID of the field to retrieve
		 * @param defVal default value returned when field is not found
		 * @return value or default value
	 * @see field()
		 */
		template<typename K, typename VV>
		VV field(const K fieldName, const VV &defVal) const {
			if (next.testfield(fieldName,typename MIsConvertible<K,typename OrgNext::OrgItemT>::MValue()))
				return getValue(defVal, typename MIsConvertible<OrgItemT,VV>::MValue());
			else
				return next.field(fieldName,defVal);
		}

		///Retrieves value defined with specified field
		/**
		 * @param fieldName name or ID of the field to retrieve
		 * @param defVal default value returned when field is not found
		 * @param ok stores true, if field is defined and value has been retrieved
		 * @return value or default value
	 * @see field()
		 */
		template<typename K, typename VV>
		VV field(const K fieldName, const VV &defVal, bool &ok) const {
			if (next.testfield(fieldName,typename MIsConvertible<K,typename OrgNext::OrgItemT>::MValue()))
				return getValue(defVal, ok, typename MIsConvertible<OrgItemT,VV>::MValue());
			else
				return next.field(fieldName,defVal,ok);
		}


	protected:

		template<typename A, typename B> friend struct VarArg;

		/*
		template<natural n>
		inline const typename FindNode<n>::T &findNode(MTrue) const {
			return *this;
		}
		template<natural n>
		inline const typename FindNode<n>::T &findNode(MFalse) const {
			return next.findNode<n>();
		}

		template<natural n>
		inline typename FindNode<n>::T &findNode(MTrue) {
			return *this;
		}
		template<natural n>
		inline typename FindNode<n>::T &findNode(MFalse) {
			return next.findNode<n>();
		}
*/
		template<typename K>
		inline bool testfield(const K &val, MTrue) const {
			return value == val;
		}

		template<typename K>
		inline bool testfield(const K &,MFalse) const {
			return false;
		}

		template<typename K>
		inline K getValue(const K &, MTrue) const {
			return value;
		}
		template<typename K>
		inline K getValue(const K &defVal, MFalse) const {
			return defVal;
		}

		template<typename K>
		inline K getValue(const K &, bool &ok, MTrue) const {
			ok = true;
			return value;
		}
		template<typename K>
		inline K getValue(const K &defVal, bool &ok, MFalse) const {
			ok = false;
			return defVal;
		}

	};

	///Termination of VarArg-list;
	/**
	 * Included into every list as argument 0.
	 */

	template<>
	struct VarArg<void,void> {

		typedef void ItemT;
		typedef void Next;
		typedef void ParamT;
		typedef void ParamN;
		typedef void OrgItemT;
		typedef void OrgNext;
		typedef VarArg<void,void> Flat;
		typedef VarArg<void,void> Ref;
		typedef VarArg<void,void> PRef;

		static const natural count = 0;
		static const bool isFlat = true;
		static const bool empty = true;

		template<natural n>
		struct FindNode {
			typedef VarArg<void,void> T;
		};
		template<natural n>
		const VarArg<void,void> &findNode() const {
			return *this;
		}
		template<natural n>
		VarArg<void,void> &findNode()  {
			return *this;
		}

		template<typename Fn>
		inline bool forEach(Fn ) const {return false;}

		template<typename Fn>
		inline bool forEach(Fn ) {return false;}

		template<typename X>
		VarArg<const X &,const VarArg<void,void> &> operator,(const X &arg) const  {
			return VarArg<const X &,const VarArg<void,void> &>(arg,*this);
		}
		template<typename X>
		VarArg<const X &,const VarArg<void,void> &> operator|(const X &arg) const  {
			return VarArg<const X &,const VarArg<void,void> &>(arg,*this);
		}
		template<typename X>
		VarArg<const X &,const VarArg<void,void> &> operator()(const X &arg) const  {
			return VarArg<const X &,const VarArg<void,void> &>(arg,*this);
		}
		template<typename X>
		VarArg<const X &,const VarArg<void,void> &> val(const X &arg) const  {
			return VarArg<const X &,const VarArg<void,void> &>(arg,*this);
		}
		template<typename X>
		VarArg<X &,const VarArg<void,void> &> ref(X &arg) const  {
			return VarArg< X &,const VarArg<void,void> &>(arg,*this);
		}
		template<typename X>
		VarArg<X &,const VarArg<void,void> &> operator &(X &arg) const  {
			return VarArg< X &,const VarArg<void,void> &>(arg,*this);
		}

		VarArgImpl<void,void> ivararg() const;

		template<typename K, typename VV>
		bool find(const K &, VV &) const {return false;}
		inline bool testfield(...) const {return false;}
		template<typename K, typename VV>
		VV field(const K , const VV &defVal) const {
			return defVal;
		}

		template<typename K, typename VV>
		VV field(const K , const VV &defVal, bool &ok) const {
			ok = false;
			return defVal;
		}

	};

	extern VarArgEmpty varArg;

	template<typename A, typename B, int n>
	class VarArg<const A (&)[n], B>: public VarArg<ArrayRef<const A>, B > {
	public:
		VarArg(const A (&x)[n], typename VarArg<ArrayRef<const A>, B>::ParamN nx)
			:VarArg<ArrayRef<const A>, B > (ArrayRef<const A>(x,n),nx) {}
	};

	template<typename A, typename B>
	class VarArgFieldPair;

	template<typename T>
	class VarArgField {
	public:
		VarArgField(typename FastParam<T>::T x):fieldName(x) {}

		bool operator==(const VarArgField &other) const {
			return other.fieldName == fieldName;
		}
		bool operator!=(const VarArgField &other) const {
			return !(other == *this);
		}

		template<typename X>
		VarArgFieldPair<T,X> operator=(const X &c) const {
			return VarArgFieldPair<T,X>(*this,c);
		}

	protected:
		T fieldName;
	};

	template<typename A, typename B>
	class VarArgFieldPair {
	public:

		const VarArgField<A> &name;
		const B &value;

		VarArgFieldPair(const VarArgField<A> &name,const B &value):name(name),value(value) {}
	};

	template<typename A, typename B, typename C>
	class VarArg<const VarArgFieldPair<A,B> &, C >: public VarArg<const B &, VarArg<VarArgField<A>, C> > {
	public:
		VarArg(const VarArgFieldPair<A,B> &a, typename FastParam<C>::T c)
			:VarArg<const B &, VarArg<VarArgField<A>, C> >(a.value,
					VarArg<VarArgField<A>, C>(VarArgField<A>(a.name),c)) {}
	};

	///Includes tagged fields into varArg object
	/**
	 *
	 * @code
	 * field(tag) = value
	 * @endcode
	 *
	 *
	 * @param f
	 * @return
	 */
	template<typename T>
	VarArgField<T> field(const T &f) {
		return VarArgField<T>(f);
	}

	///Allows to declare type with user defined variable arguments
	/**
	  Because C++ doesn't support variadric templates, declaration limits count of
	  arguments to 26. It uses default template arguments for unused arguments 
	*/

	template<typename A=void,typename B=void,
		     typename C=void,typename D=void,
			 typename E=void,typename F=void,
			 typename G=void,typename H=void,
			 typename I=void,typename J=void,
			 typename K=void,typename L=void,
			 typename M=void,typename N=void,
			 typename O=void,typename P=void,
			 typename Q=void,typename R=void,
			 typename S=void,typename T=void,
			 typename U=void,typename V=void,
			 typename W=void,typename X=void,
			 typename Y=void,typename Z=void> class VarArgDecl;

	template<typename A,typename B,typename C,typename D,
			 typename E,typename F,typename G,typename H,
			 typename I,typename J,typename K,typename L,
			 typename M,typename N,typename O,typename P,
			 typename Q,typename R,typename S,typename T,
			 typename U,typename V,typename W,typename X,
			 typename Y,typename Z> 
	
	struct VarArgDeclBuild {
		typedef VarArg<A,typename VarArgDeclBuild<B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,void>::Res> Res;
	};

	template<>
	struct VarArgDeclBuild<void,void,void,void,void,void,void,void,void,void,void,void,void,void,void,void,void,void,void,void,void,void,void,void,void,void> {
		typedef VarArgEmpty Res;
	};

	template<typename V1, typename R1 = VarArgEmpty>
	struct VarArgDeclRev {
		typedef typename VarArgDeclRev<typename OriginT<V1>::T::Next,
			 VarArg<typename OriginT<V1>::T::ItemT,R1> >::Res Res;
	};

	template<typename R1>
	struct VarArgDeclRev<VarArgEmpty,R1> {
		typedef R1 Res;
	};


	template<typename A,typename B,typename C,typename D,
			 typename E,typename F,typename G,typename H,
			 typename I,typename J,typename K,typename L,
			 typename M,typename N,typename O,typename P,
			 typename Q,typename R,typename S,typename T,
			 typename U,typename V,typename W,typename X,
			 typename Y,typename Z> 
	class VarArgDecl: 
		public VarArgDeclRev<typename VarArgDeclBuild<A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z>::Res>::Res {
	public:
		typedef typename VarArgDeclRev<typename VarArgDeclBuild<A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z>::Res>::Res Super;
		typedef Super TT;

//		VarArgDecl(const Super &list):Super(list) {}
		
		template<typename VV,typename HH>
		VarArgDecl(const VarArg<VV,HH> &arg):Super(arg) {}
	};





}

#endif /* LIGHTSPEED_VARARG_H_ */
