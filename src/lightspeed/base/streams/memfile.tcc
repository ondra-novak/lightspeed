/*
 * memfile.tcc
 *
 *  Created on: 28.2.2011
 *      Author: ondra
 */


#ifndef LIGHTSPEED_BASE_STREAMS_MEMFILE_TCC_
#define LIGHTSPEED_BASE_STREAMS_MEMFILE_TCC_

#include <string.h>
#include "memfile.h"
#include "../containers/autoArray.tcc"


namespace LightSpeed {

template<typename Alloc>
natural MemFile<Alloc>::read(void *buffer, natural size)
{
	natural l = filebuffer.length();
	if (pos >= l) return 0;
	natural endrd = pos + size;
	if (endrd >= l) endrd = l;
	natural rdl = endrd - pos;
	memcpy(buffer,filebuffer.data()+pos,rdl);
	pos += rdl;
	return rdl;
}



template<typename Alloc>
natural MemFile<Alloc>::write(const void *buffer, natural size)
{
	natural wrpos = appendMode?filebuffer.length():pos;
	natural l = filebuffer.length();
	natural endrd = wrpos + size;
	if (endrd >= l) {
		filebuffer.resize(endrd);
	}
	memcpy(filebuffer.data()+wrpos,buffer,size);
	pos += size;
	return size;
}



template<typename Alloc>
natural MemFile<Alloc>::peek(void *buffer, natural size) const
{
	natural s = read(buffer,size,pos);
	return s;
}



template<typename Alloc> inline bool MemFile<Alloc>::canRead() const
{
	return pos < filebuffer.length();
}



template<typename Alloc> inline bool MemFile<Alloc>::canWrite() const
{
	return !readOnly;
}



template<typename Alloc> inline natural MemFile<Alloc>::read(void *buffer, natural size, FileOffset offset) const
{
	natural pos = (natural)offset;
	natural l = filebuffer.length();
	if (pos >= l) return 0;
	natural endrd = pos + size;
	if (endrd >= l) endrd = l;
	natural rdl = endrd - pos;
	memcpy(buffer,filebuffer.data()+pos,rdl);
	return rdl;
}



template<typename Alloc> inline natural MemFile<Alloc>::write(const void *buffer, natural size, FileOffset offset)
{
	natural pos = (natural)offset;
	natural l = filebuffer.length();
	natural endrd = pos + size;
	if (endrd >= l) {
		filebuffer.resize(endrd);
	}
	memcpy(filebuffer.data()+pos,buffer,size);
	return size;
}



template<typename Alloc> inline void MemFile<Alloc>::setSize(FileOffset size)
{
	filebuffer.resize((natural)size);
}



template<typename Alloc> inline typename MemFile<Alloc>::FileOffset MemFile<Alloc>::size() const
{
	return filebuffer.length();
}




}

#endif /* LIGHTSPEED_BASE_STREAMS_MEMFILE_TCC_ */
