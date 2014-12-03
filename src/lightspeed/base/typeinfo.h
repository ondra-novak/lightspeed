#pragma once

#include <typeinfo>
#include "compare.h"
#include "../base/exceptions/throws.h"

namespace LightSpeed {

	class TypeInfo: public ComparableLessAndEqual<TypeInfo> {
	public:
		TypeInfo(const std::type_info &tinfo):tinfo(&tinfo) {}
		TypeInfo():tinfo(0) {}

		bool isNil() const {return tinfo == 0;}

		bool lessThan(const TypeInfo &other) const {
			return tinfo != other.tinfo && other.tinfo != 0 && (tinfo == 0 || tinfo->before(*other.tinfo) != 0);
		}
		bool equalTo(const TypeInfo &other) const {
			return tinfo == other.tinfo 
				|| (tinfo != 0 && other.tinfo != 0 && *tinfo == *other.tinfo);
				
		}

		operator const std::type_info &() const {
			if (tinfo == 0) throwNullPointerException(THISLOCATION);
			return *tinfo;
		}

		operator const std::type_info *() const {
			return tinfo;
		}

		const std::type_info *operator->() const {
			if (tinfo == 0) throwNullPointerException(THISLOCATION);
			return tinfo;
		}

		const char *name() const {return tinfo->name();}


	protected:
		const std::type_info *tinfo;
	};

}
