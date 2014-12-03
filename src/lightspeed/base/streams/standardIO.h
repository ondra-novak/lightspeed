/**
 * console.h 
 *
 *  Created on: 19.8.2009
 *      Author: ondra
 */

#ifndef _LIGHTSPEED_STREAMS_CONSOLE_H_
#define _LIGHTSPEED_STREAMS_CONSOLE_H_

#include "utf.h"
#include "fileiobuff.h"
#include "../text/textOut.h"
#include "../text/textIn.h"
#include "../text/textstream.h"

namespace LightSpeed {


	template<typename TextInput, typename TextOutput, natural buffSize = 256>
	class ConsoleT {		
	public:

		///Type of input character stream to read characters directly
		typedef typename TextInput::IterT In;
		///Type of output character stream to write characters directly
		typedef typename TextOutput::IterT Out;
		///Type of variable print or error
		typedef TextOutput Print;
		///Type of variable scan
		typedef TextInput Scan;


		ConsoleT():genericInOut(getStdInput(),getStdOutput())
			,inoutbuff(&genericInOut),errbuff(getStdError())
			,scan(SeqFileInput(&inoutbuff))
			,print(SeqFileOutput(&inoutbuff))
			,error(SeqFileOutput(&errbuff))
			,in(scan.nxChain())
			,out(print.nxChain())
			,err(error.nxChain())
		{
			genericInOut.setStaticObj();
			inoutbuff.setStaticObj();
			errbuff.setStaticObj();
			inoutbuff.setAutoflush(&errbuff);
		}

		ConsoleT(PInputStream in, POutputStream out,
				POutputStream err):genericInOut(in,out)
			,inoutbuff(&genericInOut),errbuff(err)
			,scan(SeqFileInput(&inoutbuff))
			,print(SeqFileOutput(&inoutbuff))
			,error(SeqFileOutput(&errbuff))
			,in(scan.nxChain())
			,out(print.nxChain())
			,err(error.nxChain())
		{
			genericInOut.setStaticObj();
			inoutbuff.setStaticObj();
			errbuff.setStaticObj();
			inoutbuff.setAutoflush(&errbuff);
		}

		static PInputStream getStdInput() {
			return IFileIOServices::getIOServices().openStdFile(IFileIOServices::stdInput).get();
		}
		static POutputStream getStdOutput() {
			return IFileIOServices::getIOServices().openStdFile(IFileIOServices::stdOutput).get();
		}
		static POutputStream getStdError() {
			return IFileIOServices::getIOServices().openStdFile(IFileIOServices::stdError).get();
		}


		void flush() {inoutbuff.flush();errbuff.flush();}
	protected:
		SeqBidirStream genericInOut;
		IOBuffer<buffSize> inoutbuff;
		IOBuffer<buffSize> errbuff;
	public:
		///formatted input (scan)
		TextInput scan;
		///formatted output
		TextOutput print;
		///formatted error output
		TextOutput error;

		///Direct (buffered) text input (to read characters, block operations)
		typename TextInput::IterT &in;
		///Direct (buffered) text output (to write characters, block operations)
		typename TextOutput::IterT &out,&err;

	};


	typedef ConsoleT<ScanTextA, PrintTextA> ConsoleA;
	typedef ConsoleT<ScanTextW, PrintTextW> ConsoleW;
	typedef ConsoleT<ScanTextA, PrintTextA, 16384> FastConsoleA;
	typedef ConsoleT<ScanTextW, PrintTextW, 16384> FastConsoleW;

}




#endif /* CONSOLE_H_ */
