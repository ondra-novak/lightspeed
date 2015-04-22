/*
 * stringpool.cpp
 *
 *  Created on: Sep 18, 2012
 *      Author: ondra
 */




#include "stringpool.tcc"

template class LightSpeed::StringPool<char>;
template class LightSpeed::StringPool<wchar_t>;


void *LightSpeed::ptrStringPoolNull = 0;