/*
 * bufferedio.cpp
 *
 *  Created on: 9. 3. 2015
 *      Author: ondra
 */



#include "bufferedio.tcc"
#include "../containers/autoArray.tcc"


namespace LightSpeed {

StreamBuffer<0>::StreamBuffer():wrpos(0),rdpos(0),isempty(true),wasFull(false) {
	buffdata.resize(buffdata.getAllocatedSize());
}

ArrayRef<byte> StreamBuffer<0>::getSpace()
{
	if (empty) return ArrayRef<byte>(buffdata);
	else return ArrayRef<byte>(buffdata.data()+wrpos,getContAvailable());

}
bool StreamBuffer<0>::tryExpand() {
	natural expand = buffdata.length()/2;
	buffdata.insert(wrpos,0,expand);
	if (rdpos > wrpos) rdpos+=expand;
	return true;
}



bool StreamBuffer<0>::isEmpty() const
{
	return isempty;
}

bool StreamBuffer<0>::isFull() const
{
	bool f =  (!isempty && wrpos == rdpos);
	if (f) wasFull = true;
	return f;

}

natural StreamBuffer<0>::size() const
{
	return !isempty?(wrpos < rdpos?buffdata.length() - rdpos + wrpos:wrpos - rdpos):0;
}

natural StreamBuffer<0>::getAvailable() const
{
	return buffdata.length() - size();
}

natural StreamBuffer<0>::getContAvailable() const
{
	if (empty) return buffdata.length();
	else if (wrpos < rdpos) return rdpos - wrpos;
	else return buffdata.length()-wrpos;
}

void StreamBuffer<0>::commitWrite(natural count)
{
	wrpos = (wrpos + count) % buffdata.length();
	isempty = false;
	checkResize();
}

const byte& StreamBuffer<0>::getByte() const
{
	return buffdata[rdpos];
}

ConstBin StreamBuffer<0>::getData() const
{
	if (empty) {
		return ConstBin();
	} else if (wrpos <= rdpos) {
		return ConstBin(buffdata.data()+rdpos,  buffdata.length()-rdpos);
	} else {
		return ConstBin(buffdata.data()+rdpos, wrpos - rdpos);
	}

}

void StreamBuffer<0>::commitRead(natural count)
{
	rdpos = (rdpos + count) % buffdata.length();
	isempty = rdpos == wrpos;
	if (isempty) rdpos = wrpos = 0;
}

natural StreamBuffer<0>::lookup(ConstBin data, natural fromPos) const {
	natural bufsz = buffdata.length();
	if (isempty || data.empty()) return naturalNull;
		natural b = (rdpos + fromPos) % bufsz;
		natural cnt = fromPos;
		while (b != wrpos) {
			if (buffdata[b] == data[0]) {
				natural c = b = (b + 1) % bufsz;
				natural i;
				for (i = 1; i < data.length() && c != wrpos; i++, c = (c + 1) % bufsz) {
					if (data[i] != buffdata[c]) break;
				}
				if (i == data.length()) return cnt;
				if (c == wrpos) return naturalNull;
			} else {
				b = (b + 1) % bufsz;
			}
			cnt++;
		}
		return naturalNull;
	}
void StreamBuffer<0>::putBack(byte b)
{
	natural bufsz = buffdata.length();
	if (rdpos == 0) rdpos = bufsz;
	rdpos--;
	buffdata(rdpos) = b;
	isempty = false;
}

void StreamBuffer<0>::putByte(byte b)
{
	natural bufsz = buffdata.length();
	buffdata(wrpos) = b;
	wrpos = (wrpos + 1) % bufsz;
	isempty = false;
	checkResize();

}

void StreamBuffer<0>::checkResize() {
	if (wasFull) {
		time_t tm;
		time(&tm);
		if ((natural)tm == tmmark) {
			tryExpand();
		} else {
			tmmark = tm;
		}
		wasFull = false;
	}
}


}
