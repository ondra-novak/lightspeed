#pragma once

namespace LightSpeed {

template<typename Base>
class WindowsSocketResource: public NetworkResourceCommon<Base>{

public:
	const UINT_PTR sock;


	WindowsSocketResource(UINT_PTR socket, natural defWait, bool noclose = false)
		: sock(socket),defWait(defWait),noclose(noclose) {}
	virtual ~WindowsSocketResource();

	virtual natural getDefaultWait() const {return defWait;}
	virtual natural doWait(natural waitFor, natural timeout) const;

protected:


	natural defWait;
	bool noclose;
};

}