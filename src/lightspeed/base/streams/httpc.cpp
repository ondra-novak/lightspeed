#include "httpc.tcc"

namespace LightSpeed {

	template class  HttpRequest<void(*)(const ConstBin &)>;
	template class  HttpResponse<byte (*)(void)>;


}