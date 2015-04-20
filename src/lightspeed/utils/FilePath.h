/*
 * FilePath.h
 *
 *  Created on: 3.5.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_UTILS_FILEPATH_H_
#define LIGHTSPEED_UTILS_FILEPATH_H_
#include "../base/containers/string.h"
#include "../base/memory/smallAlloc.h"
#include "../base/streams/utf.h"
#include "../base/containers/autoArray.h"

namespace LightSpeed {


struct FilePathConfig {
	ConstStrW dirSeparator;
	ConstStrW parentDir;
	ConstStrW parentDirWithSep;
	ConstStrW rootSeparator;
	ConstStrW extensionSeparator;
	bool nodrive;

};

enum PathParent {
	parent
};


extern LIGHTSPEED_EXPORT FilePathConfig defaultPathConfig;



template<const FilePathConfig *config  = &defaultPathConfig>
class FilePathT;

namespace _intr {



template<typename H, typename T, const FilePathConfig *config>
struct FilePathChain ;


template<typename T, const FilePathConfig *config>
struct FilePathChain<ConstStrW, T, config> {

	ConstStrW h;
	const T &t;


	natural drvpos;
	bool abs,driveabs;

	FilePathChain(const ConstStrW &h, const T &t):h(h),t(t) {
		drvpos = h.find(config->rootSeparator);
		driveabs = drvpos == 0;
		abs = (config->nodrive && driveabs) || (!config->nodrive && drvpos != naturalNull);        
	}


	template<typename X>
	void writePath(IWriteIterator<wchar_t,X> &wr, natural &skipParents) const {

		ConstStrW p = calcParents(skipParents);
		if (abs) {
			wr.blockWrite(p);
		} else if (driveabs) {
			ConstStrW drive = t.getDrive();
            wr.blockWrite(drive);
			wr.blockWrite(config->rootSeparator);
			wr.blockWrite(p);
		} else {
			t.writePath(wr,skipParents);
			wr.blockWrite(config->dirSeparator);
			wr.blockWrite(p);
		}
	}

	ConstStrW calcParents(natural &p) const {
		return FilePathT<config>::calcParents(h,p,abs||driveabs);
	}

	static const bool isFolder = false;

	natural getPathLength(natural &skipParents) const {		
		ConstStrW p = calcParents(skipParents);
		if (abs) {
			return p.length();
		} else if (driveabs) {
			ConstStrW drive = t.getDrive();
			return p.length() + config->rootSeparator.length() + drive.length();
		} else {
			return t.getPathLength(skipParents) + config->dirSeparator.length() + p.length();
		}
	}
	ConstStrW getDrive() const {
		if (abs) {			
			return h.head(drvpos);
		} else {
			return t.getDrive();
		}
	}	

	template<typename X>
	FilePathChain<X, FilePathChain<ConstStrW, T,config>,config> operator/(const X &relpath) const {
		return FilePathChain<X,FilePathChain<ConstStrW,T,config>,config>(relpath,*this);
	}
	
};

template<typename T, const FilePathConfig* config>
struct FilePathChain<ConstStrA, T, config> {


	static const bool isFolder = false;


	typedef AutoArray<wchar_t, SmallAlloc<40> > Buffer;
	
	Buffer buffer;
	const FilePathChain<ConstStrW, T,config> inner;

	FilePathChain(const ConstStrA &h, const T &t):inner(convertStr(buffer,h),t) {}

	static ConstStrW convertStr(Buffer &buffer, ConstStrA h)
	{
		ConstStrA::Iterator iter = h.getFwIter();	
		Utf8ToWideReader<ConstStrA::Iterator &> rd(iter);
		while (rd.hasItems()) buffer.add(rd.getNext());
		return buffer;
	}

	template<typename X>
	void writePath(IWriteIterator<wchar_t,X> &wr,natural &skipParents) const {		
		inner.writePath(wr,skipParents);
	}
	natural getPathLength(natural &skipParents) const {
		return inner.getPathLength(skipParents);
	}
	ConstStrW getDrive() const {
		return inner.getDrive();
	}

	

	template<typename X>
	FilePathChain<X, FilePathChain<ConstStrA, T,config>,config> operator/(const X &relpath) const {
		return FilePathChain<X,FilePathChain<ConstStrA,T,config>,config>(relpath,*this);
	}

};


template<typename T,const FilePathConfig* config>
struct FilePathChain<PathParent, T, config> {

	const T &t;

	static const bool isFolder = true;

	FilePathChain(PathParent, const T &t):t(t) {}

	template<typename X>
	void writePath(IWriteIterator<wchar_t,X> &wr,natural &skipParents) const {		
		skipParents++;
		t.writePath(wr,skipParents);
		if (skipParents) {
			wr.blockWrite(config->dirSeparator);
			wr.blockWrite(config->parentDir);
			skipParents--;
		}
	}
	natural getPathLength(natural &skipParents) const {
		skipParents++;
		natural cnt = t.getPathLength(skipParents);
		if (skipParents) {
			skipParents--;
			return cnt + config->dirSeparator.length() + config->parentDir.length();
		} else {
			return cnt;
		}
	}
	ConstStrW getDrive() const {
		return t.getDrive();
	}



	template<typename X>
	FilePathChain<X, FilePathChain<PathParent, T,config>,config> operator/(const X &relpath) const {
		return FilePathChain<X, FilePathChain<PathParent, T,config>,config>(relpath,*this);
	}

};


template<typename H, typename T, const FilePathConfig *config, int n>
struct FilePathChain<const H[n], T, config>: public FilePathChain<ConstStringT<H>,T,config> {
	FilePathChain(const H h[n], const T &t):FilePathChain<ConstStringT<H>,T,config>(h,t) {}
};

template<typename T, const FilePathConfig *config>
struct FilePathChain<String, T, config>: public FilePathChain<ConstStrW,T,config> {
	FilePathChain(const String &h, const T &t):FilePathChain<ConstStrW,T,config>(h,t) {}
};

template<typename T, const FilePathConfig *config>
struct FilePathChain<NullType, T, config> {
	const T &t;
	static const bool isFolder = true;

	FilePathChain(NullType, const T &t):t(t) {}

	template<typename X>
	void writePath(IWriteIterator<wchar_t,X> &wr,natural &skipParents) const {		
		t.writePath(wr,skipParents);
	}
	natural getPathLength(natural &skipParents) const {
		return t.getPathLength(skipParents);
	}

	ConstStrW getDrive() const {
		return t.getDrive();
	}
};

}

template<const FilePathConfig *config>
class FilePathT: public String {
public:

	FilePathT() {}
	explicit FilePathT(String absolutePath);
	explicit FilePathT(String absolutePath, bool isDirectory);

	template<typename H, typename T>
	FilePathT(const _intr::FilePathChain<H,T,config> &chain):pathFileSep(naturalNull) {
		loadPath(chain);
	}

	FilePathT &operator=(const FilePathT& absolutePath);

	_intr::FilePathChain<ConstStrW, FilePathT, config> operator/(ConstStrW relpath) const {
		return _intr::FilePathChain<ConstStrW, FilePathT, config>(relpath,*this);
	}
	_intr::FilePathChain<ConstStrA, FilePathT, config> operator/(ConstStrA relpath) const {
		return _intr::FilePathChain<ConstStrA, FilePathT, config>(relpath,*this);
	}
	_intr::FilePathChain<PathParent, FilePathT, config> operator/(PathParent) const {
		return _intr::FilePathChain<PathParent, FilePathT, config>(parent,*this);
	}
	_intr::FilePathChain<NullType, FilePathT, config> operator/(NullType) const {
		return _intr::FilePathChain<NullType, FilePathT, config>(nil,*this);
	}




	ConstStrW getPath() const;
	ConstStrW getFilename() const;
	ConstStrW getDrive() const;
	ConstStrW getTitle() const;
	ConstStrW getExtension() const;
	ConstStrW getParentPath() const;

	natural getPathLength(natural &parents) const {
	     ConstStrW h = calcParents(getPath(),parents,true);
		 return h.length();
	}

	template<typename X>
	void writePath(IWriteIterator<wchar_t,X> &wr, natural &parents) const {
	     ConstStrW h = calcParents(getPath(),parents,true);
		 wr.blockWrite(h,true);
	}

	static const bool isFolder = false;

	static ConstStrW calcParents(ConstStrW h, natural &p, bool abs) {
		ConstStrW z = h;
		while (p != 0) {
			if (z.empty()) {
				if (abs)  p = 0;
				return z;
			}
			natural sep = z.findLast(config->dirSeparator) ;
			if (sep == naturalNull) z = ConstStrW();
			else z = z.head(sep);
			p--;
		}
		return z;
	}


private:
	mutable natural pathFileSep;

	template<typename T>
	void loadPath(const T &chain);
};

template<const FilePathConfig *config>
template<typename T>
void FilePathT<config>::loadPath( const T &chain )
{
	natural parents = 0;
	natural needSz = chain.getPathLength(parents);	
	if (T::isFolder) needSz+=config->dirSeparator.length();
	String::WriteIterator iter = this->createBufferIter(needSz);
	chain.writePath(iter,parents);
	if (T::isFolder) iter.blockWrite(config->dirSeparator,true);
}


typedef FilePathT<> FilePath;

FilePath getCurrentDir();



template<const FilePathConfig* config>
inline FilePathT<config>::FilePathT(String absolutePath):String(absolutePath),pathFileSep(naturalNull) {

}

template<const FilePathConfig* config>
inline FilePathT<config>::FilePathT(String absolutePath,
	bool isDirectory):pathFileSep(naturalNull) {
		if (isDirectory
			&& ConstStrW(absolutePath.tail(config->dirSeparator.length())) != config->dirSeparator) {
				absolutePath = absolutePath + config->dirSeparator;
		}
		String::operator=(absolutePath);
}


template<const FilePathConfig* config>
FilePathT<config> &FilePathT<config>::operator=(const FilePathT& absolutePath) {
	pathFileSep = naturalNull;
	String::operator=(absolutePath);
	return *this;

}

template<const FilePathConfig* config>
inline ConstStrW FilePathT<config>::getPath() const {
	if (tail(config->parentDir.length()) == config->parentDir) return *this;
	if (pathFileSep == naturalNull) {
		pathFileSep = this->findLast(config->dirSeparator);
		if (pathFileSep == naturalNull) return ConstStrW();
	}
	return this->head(pathFileSep);
}

template<const FilePathConfig* config>
inline ConstStrW FilePathT<config>::getFilename() const {
	ConstStrW path = getPath();
	return this->offset(path.length()+config->dirSeparator.length());
}

template<const FilePathConfig* config>
inline ConstStrW FilePathT<config>::getDrive() const {
	natural p = this->find(config->rootSeparator);
	if (p == naturalNull) return ConstStrW();
	else return this->head(p);
}

template<const FilePathConfig* config>
inline ConstStrW FilePathT<config>::getTitle() const {
	ConstStrW fname = getFilename();
	natural p = fname.findLast(config->extensionSeparator);
	if (p == naturalNull) return fname;
	else return fname.head(p);
}

template<const FilePathConfig* config>
inline ConstStrW FilePathT<config>::getExtension() const {
	ConstStrW fname = getFilename();
	natural p = fname.findLast(config->extensionSeparator);
	if (p == naturalNull) return ConstStrW();
	else return fname.offset(p+1);
}

template<const FilePathConfig* config>
inline ConstStrW FilePathT<config>::getParentPath() const {
	ConstStrW path = getPath();
	natural p = path.findLast(config->dirSeparator);
	if (p == naturalNull) return ConstStrW();
	else return path.head(p);
}



}

#endif /* LIGHTSPEED_UTILS_FILEPATH_H_ */

