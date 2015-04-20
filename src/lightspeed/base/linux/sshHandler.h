/*
 * sshHandler.h
 *
 *  Created on: 24. 2. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_LINUX_SSHHANDLER_H_
#define LIGHTSPEED_BASE_LINUX_SSHHANDLER_H_
#include "../streams/fileio_ifc.h"

namespace LightSpeed {

class SshHandler: public IFileIOHandler {
public:

    virtual PInOutStream openSeqFile(ConstStrW fname,  FileOpenMode mode, OpenFlags::Type flags);
    virtual PRndFileHandle openRndFile(ConstStrW ofn, FileOpenMode mode, OpenFlags::Type flags);
    virtual bool canOpenFile(ConstStrW name, FileOpenMode mode) const;
    virtual PFolderIterator openFolder(ConstStrW pathname);
	virtual PFolderIterator getFileInfo(ConstStrW pathname);
    virtual void createFolder(ConstStrW folderName, bool parents);
    virtual void removeFolder(ConstStrW folderName, bool recursive);
    virtual void copy(ConstStrW from, ConstStrW to, bool overwrite);
    virtual void move(ConstStrW from, ConstStrW to, bool overwrite);
    virtual void link(ConstStrW linkName, ConstStrW target, bool overwrite);
    virtual void remove(ConstStrW what);
    virtual PMappedFile mapFile(ConstStrW filename, FileOpenMode mode);


};

} /* namespace LightSpeed */

#endif /* LIGHTSPEED_BASE_LINUX_SSHHANDLER_H_ */
