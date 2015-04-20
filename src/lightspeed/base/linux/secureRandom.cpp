#include "../streams/secureRandom.h"
#include "../streams/fileio_ifc.h"


namespace LightSpeed {




	SecureRandom::SecureRandom():SeqFileInput(L"/dev/urandom",0) {	}



}