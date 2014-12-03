#ifndef _LIGHTSPEED_WINDOWS_EOLN_
#define _LIGHTSPEED_WINDOWS_EOLN_
#pragma once
namespace LightSpeed {

	template<typename It>
	struct DefaultTextControlComposer {
		typedef TextControlComposerDos<It> Composer;
	};

	template<typename It>
	struct DefaultTextControlParser {
		typedef TextControlParserDos<It> Parser;
	};
}
#endif