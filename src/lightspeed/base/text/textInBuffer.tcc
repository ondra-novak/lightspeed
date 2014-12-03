/*
 * textInBuffer.tcc
 *
 *  Created on: 12.3.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_TEXT_TEXTINBUFFER_TCC_
#define LIGHTSPEED_TEXT_TEXTINBUFFER_TCC_


#include "textInBuffer.h"
#include "../containers/autoArray.tcc"


namespace LightSpeed {


template<typename Iter, typename Alloc>
void TextInBuffer<Iter,Alloc>::commit(const Iterator &pos) {

	mark(pos);
	commit();

}
template<typename Iter, typename Alloc>
void TextInBuffer<Iter,Alloc>::commit() {

	buffer.getArray().erase(0,commitPos);
	commitCounter+=commitPos;
	commitPos = 0;

}
template<typename Iter, typename Alloc>
void TextInBuffer<Iter,Alloc>::mark(const Iterator &pos) {

	if (pos.pos < commitCounter) {
		commitPos = 0;
	} else {
		commitPos = pos.pos - commitCounter;
	}

}
template<typename Iter, typename Alloc>
natural TextInBuffer<Iter,Alloc>::preload(natural pos) const {

	while (buffer.length() < pos+1 && iter.hasItems()) {
		buffer.write(iter.getNext());
	}
	return buffer.length();

}

template<typename Iter, typename Alloc>
const typename TextInBuffer<Iter,Alloc>::T &
		TextInBuffer<Iter,Alloc>::Iterator::getNext() {
	natural absp = absPos();
	natural avail = owner.preload(absp);
	if (avail < absp) throwIteratorNoMoreItems(THISLOCATION,typeid(T));
	pos++;
	if (automark) owner.mark(*this);
	return owner.buffer.getArray()[absp];

}
template<typename Iter, typename Alloc>
const typename TextInBuffer<Iter,Alloc>::T &
	TextInBuffer<Iter,Alloc>::Iterator::peek() const {
	natural absp = absPos();
	natural avail = owner.preload(absp);
	if (avail <= absp) throwIteratorNoMoreItems(THISLOCATION,typeid(T));
	return owner.buffer.getArray()[absp];

}
template<typename Iter, typename Alloc>
bool TextInBuffer<Iter,Alloc>::Iterator::hasItems() const {
	natural absp = absPos();
	if (owner.buffer.length() > absp) return true;
	if (owner.buffer.length() == absp) return owner.iter.hasItems();
	return false;
}

template<typename Iter, typename Alloc>
natural TextInBuffer<Iter,Alloc>::Iterator::getRemain() const {
	natural absp = absPos();
	natural rm = owner.iter.getRemain();
	if (rm == naturalNull) return rm;
	return rm + owner.buffer.length() - absp;
}
template<typename Iter, typename Alloc>
bool TextInBuffer<Iter,Alloc>::Iterator::equalTo(const Iterator &other) const {
	return &other.owner == &owner && other.pos == pos;
}
template<typename Iter, typename Alloc>
bool TextInBuffer<Iter,Alloc>::Iterator::lessThan(const Iterator &other) const {
	return &other.owner == &owner && other.pos > pos;

}



}  // namespace LightSpeed


#endif /* LIGHTSPEED_TEXT_TEXTINBUFFER_TCC_ */
