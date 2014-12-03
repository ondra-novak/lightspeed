#include "../containers/constStr.h"
#include "../qualifier.h"
#pragma once
#include "../containers/constStr.h"
#include "../qualifier.h"

namespace LightSpeed {


	///StringKey is object very useble as key in maps. 
	/**
	 * StringKey can be initialized as ConstStringT and also as String. ConstString
	 * is only reference, while String need allocation of memory. 
	 *
	 * While instance of this object may be inserted to the map, it must be
	 * initialized as String with memory allocation. But while keys need to
	 * be searched and compared, ConstStringT as reference can be used and
	 * then reduce number of required allocations.
	 *
	 * @note be careful to best choose how object is constructred. That is
	 * why all constructors are explicit
	 *
	 * @tparam StringImpl specify type of string, that implements storage for this
	 * string. You can use String, StringA or StringW. Also note, that 
	 * object doesn't support custom comparison specified with StringTC. It 
	 * also cannot handle comparison for StringI, StringIA and StringIW. If 
	 * you need case insensitive comparison, change default compare-operator 
	 * specified with the map
	 */
	 
	template<typename StringImpl>
	class StringKey: public ConstStringT<typename ConstObject<typename StringImpl::ItemT>::Remove> {
	public:
		typedef ConstStringT<typename ConstObject<typename StringImpl::ItemT>::Remove> Super;

		StringKey() {}
		explicit StringKey(const Super &super):Super(super) {}
		explicit StringKey(const StringImpl &param):Super(param),impl(param) {}

		const StringImpl &getString() const {return impl;}

	protected:
		StringImpl impl;
	};


}
