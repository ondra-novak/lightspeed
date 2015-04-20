/*
 * tcpHandler.h
 *
 *  Created on: 23. 2. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_STREAMS_TCPHANDLER_H_
#define LIGHTSPEED_BASE_STREAMS_TCPHANDLER_H_

#include "fileio_ifc.h"

namespace LightSpeed {

class TcpHandler: public IFileIOHandler {
public:

    virtual PInOutStream openSeqFile(ConstStrW fname,  FileOpenMode mode, OpenFlags::Type flags);
    virtual PRndFileHandle openRndFile(ConstStrW , FileOpenMode , OpenFlags::Type ) {unsupported();throw;}
    virtual bool canOpenFile(ConstStrW , FileOpenMode ) const {return true;}
    virtual PFolderIterator openFolder(ConstStrW ) {unsupported();throw;}
	virtual PFolderIterator getFileInfo(ConstStrW ) {unsupported();throw;}
    virtual void createFolder(ConstStrW , bool ) {unsupported();throw;}
    virtual void removeFolder(ConstStrW , bool ) {unsupported();throw;}
    virtual void copy(ConstStrW , ConstStrW , bool ) {unsupported();throw;}
    virtual void move(ConstStrW , ConstStrW , bool ) {unsupported();throw;}
    virtual void link(ConstStrW , ConstStrW , bool ) {unsupported();throw;}
    virtual void remove(ConstStrW ) {unsupported();throw;}
    virtual PMappedFile mapFile(ConstStrW , FileOpenMode ) {unsupported();throw;}

    void unsupported() const;


};

} /* namespace LightSpeed */

#endif /* LIGHTSPEED_BASE_STREAMS_TCPHANDLER_H_ */
