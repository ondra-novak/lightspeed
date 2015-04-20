#pragma once
#include "fileio.h"

namespace LightSpeed {

	
	///Implements security random generator as byte stream
	/**
		Generates secure random bytes as infinity stream of random numbers.
		
		Object is strictly implemented using available resources for current
		platform. Under Linux, object simply opens /dev/urandom file and returns
		it as stream (such a standard stream). Under Windows, object implements
		emulation of this stream which reads bytes using rand_s function.

		NOTE: rand_s function is 
			secure. See https://msdn.microsoft.com/en-us/library/sxtz2fa8.aspx

		You can use getStream function to access stream directly. However
		there is no special benefit from it. Implementation of the stream
		can differ by platform.


		@note stream is thread safe.
	*/
	class SecureRandom: public SeqFileInput {
	public:

		SecureRandom();

	};

}