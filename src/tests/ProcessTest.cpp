/*
 * ProcessTest.cpp
 *
 *  Created on: 25.6.2012
 *      Author: ondra
 */

#include "../lightspeed/base/streams/fileio.h"
#include "../lightspeed/base/streams/random.h"
#include "../lightspeed/mt/process.h"
#include "../lightspeed/base/streams/fileiobuff.h"
#include "../lightspeed/base/text/textstream.tcc"
#include "../lightspeed/base/streams/standardIO.tcc"
#include "ProcessTest.h"

namespace LightSpeedTest {


integer ProcessTest::start(const Args& args) {

	if (args.length() > 1) {
		if (args[1] == ConstStrW(L"generate") && args.length() > 2) {

			SeqFileOutput fout(args[2],OpenFlags::create|OpenFlags::truncate);
			Random<natural> rnd('A','Z');

			for (natural i = 0; i < 1000; i++) {
				for (natural j = 0; j < 30; j++) {
					fout.write((char)rnd.getNext());
				}
				fout.write('\n');
			}
		}
	} else {

		Process proc(getAppPathname());
		SeqFileInput readResult(nil);
		proc.arg("generate").arg(readResult).start();

		SeqFileInBuff<> readBuffered(readResult);
		SeqTextInA textRes(readBuffered);

		ConsoleA console;
		console.out.copy(textRes);
		proc.join();

	}

	return 0;
}

}

 /* namespace LightSpeedTest */
