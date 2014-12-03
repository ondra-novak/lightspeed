
#include "constStr.h"

namespace LightSpeed {
	namespace _intr {

	template<>
	const char *StringBaseCharAndWide<char>::getEmptyString() {return "";}
	template<>
	const char *StringBaseCharAndWide<const char>::getEmptyString() {return  "";}
	template<>
	const wchar_t *StringBaseCharAndWide<wchar_t>::getEmptyString() {return L"";}
	template<>
	const wchar_t *StringBaseCharAndWide<const wchar_t>::getEmptyString() {return L"";}


	}
}
