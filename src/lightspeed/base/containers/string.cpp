#include "string.tcc"

namespace LightSpeed {

template struct StrCmpCS<wchar_t>;
template struct StrCmpCS<char>;
template struct StrCmpCS<byte>;


}
