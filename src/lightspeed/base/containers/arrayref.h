/** @file
 * 
 * $Id: arrayref.h 707 2015-05-18 08:12:36Z bredysoft $
 *
 * DESCRIPTION
 * Short description
 * 
 * AUTHOR
 * Ondrej Novak
 *
 */


#ifndef LIGHTSPEED_CONTAINERS_ARRAYREF_H_
#define LIGHTSPEED_CONTAINERS_ARRAYREF_H_
#include "flatArray.h"
#include "../meta/assert.h"
namespace LightSpeed {


	template<typename T>
	class ArrayRef: public FlatArrayBase<T, ArrayRef<T> > {
	public:

		typedef FlatArrayBase<T, ArrayRef<T> > Super;
		typedef typename Super::ItemT ItemT;
		typedef typename Super::OrgItemT OrgItemT;
		typedef typename Super::ConstItemT ConstItemT;
		typedef typename Super::NoConstItemT NoConstItemT;
		typedef typename ConstObject<T>::Remove MutableItemT;
		typedef typename Super::ConstRef ConstRef;
		typedef typename Super::Ref Ref;

		ArrayRef(ItemT *p, natural length):refdata(p),len(length) {}
		ArrayRef():refdata(0),len(0) {}

		template<typename Y>
		ArrayRef(const FlatArray<MutableItemT,Y> &other):refdata(other._invoke().refData((T *)0)),len(other._invoke().length()) {
		}

		template< typename Y>
		ArrayRef(FlatArray<MutableItemT,Y> &other):refdata(other._invoke().data()),len(other._invoke().length()) {
		}

		template<typename Y>
		ArrayRef(const FlatArray<ConstItemT,Y> &other):refdata(other._invoke().refData(other._invoke().data())),len(other._invoke().length()) {
		}

		template< typename Y>
		ArrayRef(FlatArray<ConstItemT,Y> &other):refdata(other._invoke().data()),len(other._invoke().length()) {
		}

		template<typename X, typename Y>
		ArrayRef &operator=(const FlatArray<MutableItemT,Y> &other) {
			refdata=other.refdata;
			len = other.len;
			return *this;
		}

		natural length() const {return len;}
		ConstItemT *data() const {return refdata;}
		ItemT *data() {return refdata;}
		OrgItemT *refData(NoConstItemT *) const {return const_cast<OrgItemT *>(refdata);}
		const OrgItemT *refData(ConstItemT *) const {return refdata;}

		ArrayRef<ConstItemT> cc() const {
			return ArrayRef<ConstItemT>(refdata,len);
		}


		operator ArrayRef<ConstItemT>() const {
			return ArrayRef<ConstItemT>(refdata,len);
		}


		
	protected:
		T *refdata;
		natural len;

		friend class ArrayRef<typename ConstObject<T>::Invert>;



	};

}  // namespace LightSpeed




#endif /* LIGHTSPEED_CONTAINERS_ARRAYREF_H_ */
