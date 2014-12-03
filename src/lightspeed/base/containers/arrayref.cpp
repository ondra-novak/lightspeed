#include "arrayref.h"


template class LightSpeed::ArrayRef<const char>;
template class LightSpeed::ArrayRef<char>;

void test() {

	using namespace LightSpeed;

	ArrayRef<char> a;
//	ArrayRef<const char> b(a.constRef());
}
