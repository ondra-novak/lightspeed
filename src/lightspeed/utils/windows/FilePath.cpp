/*
 * FilePath.cpp
 *
 *  Created on: 3.5.2011
 *      Author: ondra
 */

#include "../FilePath.tcc"

namespace LightSpeed {


FilePathConfig defaultPathConfig = {
		ConstStrW(L"\\"),
		ConstStrW(L".."),
		ConstStrW(L"..\\"),
		ConstStrW(L"\\"),
		ConstStrW(L"."),
		false
};

template class FilePathT<>;


}
