/*
 * directory.h
 *
 *  Created on: 9.6.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_STREAMS_DIRECTORY_H_
#define LIGHTSPEED_STREAMS_DIRECTORY_H_

namespace LightSpeed {


	struct DirEntry {
		ConstStrW name;

		enum Type {
			file,
			directory,
			parentDirectory,
			link,
			pipe,
			socket,
			unknown
		};

		Type type;
	};



	class DirIterator: public IteratorBase<DirEntry, DirIterator> {
	public:
		DirIterator(const String &path);

		bool hasItems() const;
		const DirEntry &getNext() {
			retVal.type = nextType;
			retVal.name = nextName;
			nextName.swap(curName);
			return retVal;
		}

	protected:
		IRuntimeAlloc &alloc;
		void *internals;
		AutoArray<wchar_t, RTAlloc> curName,nextName;
		DirEntry::Type nextType;
		DirEntry retVal;



	};
}


#endif /* LIGHTSPEED_STREAMS_DIRECTORY_H_ */
