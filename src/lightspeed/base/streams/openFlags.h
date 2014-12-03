#ifndef LIGHTSPEED_STREAMS_OPENFLAGS_H_
#define LIGHTSPEED_STREAMS_OPENFLAGS_H_

#include "../containers/string.h"

namespace LightSpeed
{


namespace OpenFlags {

	///Enables sequential access optimization
	/**By enabling sequential access, operation system can
	* provide optimization for this kind of access by managing
	* buffers or preloads of the content. If OS not supports this
	* feature, request is ignored.
	*
	* @note you still able to use random access, but it can
	* have significant performance hit
	*
	* @note SeqInputFile and SeqOutputFile has always enabled this
	* feature
	*
	* @par implementation details
	* Function is supported only at @b Windows platform. Otherwise
	* it can ignored
	*/
	static const natural accessSeq = 		1;
    ///Enables random access optimization
    /**By enabling random access, operation system can provide
     * optimization for this king of access by managing buffers
     * or preloads the  content.If OS not supports this
     * feature, request is ignored.
     *
     * @note SeqInputFile and SeqOutputFile will ignore this request
     *
     * @par implementation details
     * Function is supported only at @b Windows platform. Otherwise
     * it is ignored
     */
	static const natural accessRnd = 		1<<1;
    ///Enables write through
    /** when write through is enabled, every write request is
     * carried directly on target media. By default, write cache
     * can keep written data for a while, before they are written to
     * the media. Using writeThrough causes worse performance, but
     * high probability, that written data will not lost during power
     * failure
     *
     * @par implementation details
     * Function is supported at both @b Windows and @b Linux platform.
     * Windows directly support this flag. At Linux, O_SYNC flag is
     * used
     */
	static const natural writeThrough = 	1<<2;

    ///Specifies, that file si temporary
    /**
     * Temporary file commands cache manager to keep data as long
     * as possible in the memory cache without writing to the
     * media.
     *
     * @par implementation details
     * Function is supported only at @b Windows platform. Otherwise
     * it can ignored
     *
     *
     * @see MemFile
     */
	static const natural temporary = 		1<<3;
    ///Specifies, that file should be deleted after it is closed
    /**
     * @par implementation details
     * Function is supported at both @b Windows and @b Linux platform.
     * Windows directly support this flag. At Linux, feature is
     * emulated during file object destruction.
     */
	static const natural deleteOnClose = 	1<<4;
    ///Specified, that file can be opened for reading multiple times
    /** If flag is not specified, operation system locks the
     * file for reading and any future open fails
     *
     * @par implementation details
     * Function is supported only at @b Windows platform. Otherwise
     * it is ignored. On linux platform, all opened files can be
     * later opened for reading
     */
	static const natural shareRead = 		1<<5;
    ///Specified, that file can be opened for writing multiple times
    /** If flag is not specified, operation system locks the
     * file for writing and any future open fails
     *
     * @par implementation details
     * Function is supported only at @b Windows platform. Otherwise
     * it is ignored. On linux platform, all opened files can be
     * later opened for writing
     */
	static const natural shareWrite = 		1<<6;
    ///Specified, that file can be deleted while it is opened
    /** If flag is not specified, operation system locks the
     * file and cannot be deleted
     *
     * @par implementation details
     * Function is supported only at @b Windows platform. Otherwise
     * it is ignored. On linux platform, all opened files can be
     * deleted anytime. Even if file is deleted, it is still available
     * for the descriptor owner.
     */
	static const natural shareDelete = 		1<<7;

    ///Specifies, that data will be appended
    /** This flag is recomended to be used with SeqOutputFile only.
     * Only legal operation is append and thus reading and writting
     * to the another location may be rejected
     *
     * @par implementation details
     * Function is supported at @b Linux platform. Otherwise, it
     * can be emulated by opening file for writting only and
     * moving pointer to the end of file.
     */
	static const natural append = 			1<<8;

    ///Specifies, that file can be created, if not exists
    /** If flag is not specified, file must exist, otherwise, open
     * function will fail.
     */
	static const natural create	=			1<<9;
    ///Specifies, that file will be truncated to 0 bytes, if exists
	static const natural truncate = 		1<<10;
	///Specifies, that new file will be created.
	/** It causes error, if file already exist. It cannot be combined with
	 * truncate
	 */
	static const natural newFile = 			1<<11;

    ///Specifies, that create will also create subfolder(s) if doesn't exists
    /** Flags is useful only with create. Otherwise, it will be ignored */

	static const natural createFolder = 	1<<12;

	///Keep old file until new is not closed
	/** Writes into temporary file until file is closed.
	 * Then old file is removed and new file is renamed.
	 *
	 * Flag is useful, when you need to write file in consistent state. All
	 * changes made in the file before it is closed are not visible for other
	 * processes. When file is closed, old file is atomatically replaced by
	 * new.
	 *
	 * Flag adds create and trunacte, but it is better to specify them for
	 * system which doesn't support this feature. If you specify
	 * newFile, opening fails in case, that temporary file still exists
	 *
	 * Flag cannot be combined with deleteOnClose. If both flags are specified
	 * deleteOnClose is suppresed
	 *
	 * Content of file is discarded when destructor is called during stack
	 * unwind due exception. If you wish to prevent this or anytime
	 * you need to commit the file before it is closed, use function closeOutput()
	 *
	 */
	static const natural commitOnClose = 	1<<13;


	typedef natural Type;
}

}
#endif /*OPENFILE_H_*/
