/*
 * messages.cpp
 *
 *  Created on: 2.1.2011
 *      Author: ondra
 */

#include "badcast.h"
#include "fileExceptions.h"
#include "../interface.tcc"
#include "invalidParamException.h"
#include "iterator.h"
#include "memoryleakexception.h"
#include "outofmemory.h"
#include "pointerException.h"
#include "rangeException.h"
#include "stdexception.h"
#include "systemException.h"
#include "unsupportedFeature.h"
#include "utf.h"
#include "waitException.h"
#include "../streams/netio.h"
#include "netExceptions.h"
#include "../containers/map.h"
#include "serializerException.h"
#include "../framework/app.h"
#include "../framework/iservices.h"
#include "../sync/trysynchronized.h"
#include "stringException.h"
#include "httpStatusException.h"
#include "../exceptions/invalidNumberFormat.h"
#include "canceledException.h"


namespace LightSpeed {

#define GetText(cls,txt) static const char *_msgText_##cls = txt;\
	const char *&cls::msgText {return _msgText_##cls;}
#define GetTextExt(cls,var,txt) static const char *_##var##cls = txt;\
	const char *&cls::var() {return _##var##cls;}

 const char *BadCastException::msgText = "Cannot convert from '%1' to '%2'. Types are incompatible";;
 const char *FileOpenError::msgText = "Failed to open file: '%1', because: ";
 const char *FileIOError::msgText = "Unable to perform I/O operation on file: '%1', because: ";
 const char *FolderCreationException::msgText = "Cannot create folder '%1', because: ";
 const char *InterfaceNotImplementedException::msgText = "Object '%1' doesn't implement the interface '%2'";
 const char *InvalidParamException::msgText = "Invalid parameter %1: '%2'";
 const char *NoMoreObjects::msgText = "There are no more objects of required type: '%1'";
 const char *IteratorNoMoreItems::msgText = "Iterator has no more objects of given type: '%1'";
 const char *WriteIteratorNoSpace::msgText = "There is no space to write object of type: '%1'";
 const char *WriteIteratorNotAcceptable::msgText = "Iterator cannot accept the item (canAccept() will return false)";
 const char *FilterIteratorBusyException::msgText = "Iterator cannot accept the item, because it is busy (cannot handle request now)";
 const char *MemoryLeakException::msgText = "Memory leak detected: \n";
 const char *MemoryLeakException::dumpText = "\tAddr: 0x%1, size: %2, id: %3 \n";
 const char *OutOfMemoryException::msgText = "Out of memory. Cannot allocate %1 bytes.";
 const char *AllocatorLimitException::msgText = "Allocator '%1' cannot perform allocation of %2 item(s), because only %3 items are available";
 const char *NullPointerException::msgText = "Invalid usage of NULL pointer";
 const char *InvalidPointerException::msgText = "Pointer is not valid (0x%1)";
 const char *IRangeException::msgText = "Range exception: ";
 const char *IRangeException::msgTextNA = "%3 is not in unspecified range";
 const char *IRangeException::msgTextFrom="%3 cannot be below %1";
 const char *IRangeException::msgTextTo="%3 cannot be above %2";
 const char *IRangeException::msgTextFromTo="%3 cannot be outside of range < %2 ; %3 >";
 const char *UnknownException::msgText = "Unknown exception (not std::exception)";
 const char *ErrNoException::msgText = "Error nr: %1 - ";
 const char *UnsupportedFeature::msgText = "Instance of class '%1' doesn't support the feature '%2'";
 const char *InvalidUTF8Character::msgText = "Invalid UTF-8 character / sequence: 0x%{02}1";
 const char *WaitingException::msgText = "Exception while waiting:";
 const char *NetworkIOError::msgText = "Network I/O error: ";
 const char *NetworkPortOpenException::msgText = "Network: Unable to open local port: %1. ";
 const char *NetworkInvalidAddressException::msgText = "Invalid network address:  '%1' ";
 const char *NetworkInvalidAddressException::msgTextNull = "<null>";
 const char *NetworkTimeoutException::msgText = "Network timeout during operation: %1. Timeout: %2 ms,";
 const char *NetworkTimeoutException::msgTextReading="READ";
 const char *NetworkTimeoutException :: msgTextWriting="WRITE";
 const char *NetworkTimeoutException :: msgTextConnecting="CONNECT";
 const char *NetworkTimeoutException ::msgTextListening="LISTEN";
 const char *NetworkTimeoutException ::msgTextUnknown="UNKNOWN";
 const char *NetworkConnectFailedException::msgText = "Unable to make connection to address: '%1' ";
 const char *NetworkResolveError::msgText = "Unable to resolve network address: '%1' - ";
 const char *NotFoundException::keyNotFound_msgText = "Key not found in the map: '%1'";
 const char *SerializerNoSectionActiveException::msgText = "Serializer has no active section: '%1' ";
 const char *ClassNotRegisteredException::msgText = "Class '%1' is not registered";
 const char *UnknownObjectIdentifierException::msgText = "Unknown object identifier: '%1'";
 const char *SubsectionException::msgText = "Exception has been thrown inside of section '%1'";
 const char *RequiredSectionException::msgText = "Section '%1' is required but missing";
 const char *UnexpectedSectionException::msgText = "Section '%1' is not allowed or expected: ";
 const char *NoCurrentApplicationException::msgText = "No current application. You have to declare at least one object that extends App class";
//const char *ServiceNotFoundException::msgText = "Service '%1' is not registered";
 const char *SynchronizedException::msgText = "Lock or resource is busy (tryLock failed)";
 const char *UnknownCodePageException::msgText = "Unknown code-page '%1', cannot perform conversion:";
 const char *InvalidCharacterException::msgText = "Invalid character '%1' in the string:";
 const char *FolderNotEmptyException::msgText = "Folder '%1' is not empty, cannot be deleted. ";
 const char *FileDeletionException::msgText = "File or folder '%1' cannot be deleted: ";
 const char *HttpStatusException::msgText = "HTTP unexpected status: %2 %3 while trying to download: %1";
 const char *CanceledException::msgText = "Request has been canceled";
 const char *FileCopyException::msgText = "Can't copy the file '%1' to the file '%2': ";

const char *Synchronized_TryLockFailedMsg = "Failed to lock object while non-blocking operation has been requested (trylock)";
const char *InvalidNumberFormatException::msgText = "Invalid number format: %1";
const char *NotImplementedExcption::msgText = "Operation \"%1\" is not implemented.";

}

