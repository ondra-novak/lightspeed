#pragma once

namespace LightSpeed {

template<typename Base>
class LinuxSocketResource: public NetworkResourceCommon<Base>{

public:
	const int sock;


	LinuxSocketResource(int socket, natural defWait, bool noclose = false)
		: sock(socket),defWait(defWait),noclose(noclose) {}
	virtual ~LinuxSocketResource();

	virtual natural getDefaultWait() const {return defWait;}
	virtual natural doWait(natural waitFor, natural timeout) const;

protected:

	natural defWait;
	bool noclose;
};

}
