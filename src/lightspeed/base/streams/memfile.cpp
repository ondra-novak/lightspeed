/*
 * memfile.cpp
 *
 *  Created on: 28.2.2011
 *      Author: ondra
 */

#include <string.h>
#include "memfile.tcc"

namespace LightSpeed {


template class MemFile<>;


natural MemFileStr::read(void* buffer, natural size) {
	natural remain = text.length() - pos;
	if (remain < size) size = remain;
	memcpy(buffer,text.data()+pos,size);
	pos+=size;
	return size;
}

natural MemFileStr::write(const void* , natural ) {
	return 0;
}

natural MemFileStr::peek(void* buffer, natural size) const {
	natural remain = text.length() - pos;
	if (remain < size) size = remain;
	memcpy(buffer,text.data()+pos,size);
	return size;
}

bool MemFileStr::canRead() const {
	return pos < text.length();
}

bool MemFileStr::canWrite() const {
	return false;
}

natural MemFileStr::read(void* buffer, natural size,
		FileOffset offset) const {

	natural o = (natural)offset;
	if (o > text.length()) return 0;
	natural remain = text.length() - o;
	if (remain < size) size = remain;
	memcpy(buffer,text.data()+o,size);
	return size;
}

natural MemFileStr::write(const void* , natural ,
		FileOffset ) {
	return 0;
}

void MemFileStr::setSize(FileOffset ) {

}

MemFileStr::FileOffset MemFileStr::size() const {
	return text.length();
}
  // namespace LightSpeed
}
