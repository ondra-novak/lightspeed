#ifndef _LIGHTSPEED_LINUX_EOLN_
#define _LIGHTSPEED_LINUX_EOLN_
#pragma once
namespace LightSpeed {

	template<typename It>
	struct DefaultTextControlComposer {
		typedef TextControlComposer<It> Composer;
	};

	template<typename It>
	struct DefaultTextControlParser {
		typedef TextControlParser<It> Parser;
	};

}
#endif
