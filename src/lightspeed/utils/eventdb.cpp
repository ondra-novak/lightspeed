/*
 * DBFile.cpp
 *
 *  Created on: 26. 5. 2014
 *      Author: ondra
 */


#include "../base/exceptions/invalidParamException.h"
#include "../base/sync/synchronize.h"
#include "../base/containers/autoArray.tcc"
#include "../base/streams/fileiobuff.tcc"

#include "../base/exceptions/errorMessageException.h"
#include "../base/debug/dbglog.h"

#include "eventdb.h"


namespace LightSpeed {

namespace EventDB {


const Bin::natural16 initialChecksum = 'C' + (256 * 'S');

static void updateChecksum(ConstBin buffer, Bin::natural16& checksum) {
	for (natural i = 0; i < buffer.length(); i++) {
		checksum += buffer[i];
	}
}

void EventLog::addListener(IUpdateListener* listener) {
	Sync _(lock);
	listeners.add(listener);
}

void EventLog::addListenerAndSync(IUpdateListener* listener) {
	UpdateAdapter emptyListener;
	if (dbfile == 0) throw ErrorMessageException(THISLOCATION, "Not initialized");


	FileOffset fsend = 0, fsbeg = 0;
	Bin::natural16 chk = initialChecksum;
	time_t lastTimestamp;

	listener->onStartRescan();
	fsend = fileSize();
	fsbeg = rescan(listener, fsbeg, fsend, chk, lastTimestamp, false);
	fsend = fileSize();
	fsbeg = rescan(listener, fsbeg, fsend, chk, lastTimestamp, false);

	Sync _(lock);
	listeners.add(listener);
	fsend = dbfile->size();
	rescan(listener,fsbeg,fsend, chk, lastTimestamp, false);
}

void EventLog::removeListener(IUpdateListener* listener) {
	for (natural i = 0; i < listeners.length(); i++) {
		if (listeners[i] == listener) {
			listeners.erase(i);
			return;
		}
	}
}

natural EventLog::writeBlock(SeqFileOutput toStream,
						  natural recordType,
						  ConstBin recordData,
						  time_t curTime,
						  WriteState &state,
						  natural &offset) {

	natural wrcount = 0;

	byte padding[blockSize];
	if (recordType > 0xFFFF) throw InvalidParamException(THISLOCATION,0,"Record type cannot be higher than 65535");
	if (recordData.length() > (0xFFFF* blockSize)) throw InvalidParamException(THISLOCATION,1,"Record is limited up to 512KiB");
	Header hdr;
	natural timediff = (natural)(curTime - state.lastTimestamp);

	if (timediff > 0xFFFF) {
		hdr.checksum = state.currentCheckSum;
		hdr.recordType = 0xFFFF;
		hdr.recordSize = 0;
		hdr.timeadv = (Bin::natural16)(timediff >> 16);
		toStream.blockWrite(&hdr,sizeof(hdr),true);
		state.currentCheckSum+=hdr.timeadv;
		state.currentCheckSum+=hdr.recordType;
		wrcount += sizeof(hdr);
	}
	hdr.checksum = state.currentCheckSum;
	hdr.recordType = recordType;
	hdr.recordSize = (recordData.length() + blockSize - 1)/blockSize;
	hdr.timeadv = (timediff & 0xFFFF);
	offset = wrcount / blockSize;
	toStream.blockWrite(&hdr,sizeof(hdr));
	wrcount += sizeof(hdr);
	toStream.blockWrite(recordData.data(), recordData.length());
	wrcount += recordData.length();
	updateChecksum(recordData,state.currentCheckSum);
	//because blocks are padded to nearest 8 bytes, fill remaing bytes by random numbers
	natural remain = hdr.recordSize * blockSize - recordData.length();
	if (remain) {
		//create random numbers
		for (natural i = 0; i < remain; i++) padding[i] = (byte)(state.rnd.getNext() & 0xFF);
		//write bytes
		toStream.blockWrite(padding,remain,true);
		wrcount+=remain;
		//update checksum
		updateChecksum(ConstBin(padding,remain),state.currentCheckSum);
	}
	//update checksum
	state.currentCheckSum+=hdr.timeadv;
	//update checksum
	state.currentCheckSum+=hdr.recordType;
	state.lastTimestamp = curTime;
	toStream.flush();

	return wrcount;
}

EventLog::WriteState2 EventLog::initReplication() {
	slaveMode = true;
	return WriteState2(writeState,writePos/blockSize);
}

bool EventLog::readReplicationData(IReplicationInput& input, time_t timeOffset) {
	if (!isSlave()) throw ErrorMessageException(THISLOCATION,"Slave mode is not initialized");
	ReplicationListener::Header hdr;
	if (!input.read(&hdr, sizeof(hdr))) return false;
	byte *buffer = hdr.length?(byte *)alloca(hdr.length):0;
	if (hdr.length) {
		if (!input.read(buffer,hdr.length)) return false;
	}

	Synchronized<FastLockR> _(lock);
	ConstBin binbuff(buffer,hdr.length);
	time_t realtime = hdr.timestamp+timeOffset;
	if (hdr.cellId != 0)
		updateStEvent_trn(hdr.cellId,hdr.recordType,binbuff,realtime);
	else
		sendUpdate_trn(hdr.recordType,binbuff,realtime);

	return true;
}

void EventLog::stopReplication() {
	slaveMode = false;
}

bool EventLog::isSlave() const {
	return slaveMode;
}

EventLog::FileOffset EventLog::sendUpdate_trn(natural recordType, ConstBin recordData, time_t time) {

	SeqFileOutput outp(dbfile,writePos);
	SeqFileOutBuff<> outb(outp);

	natural offset = 0;
	natural wrcount = 0;



	wrcount = writeBlock(outb, recordType,recordData,time,writeState,offset);
	outb.flush();




	FileOffset ofs = writePos + offset;


	try {
		for (natural i = 0; i < listeners.length(); i++) {
			try {
				listeners[i]->onUpdate(ofs/blockSize,writeState.lastTimestamp,recordType,recordData);
			} catch (Exception &e) {
				e.appendReason(ErrorMessageException(THISLOCATION, ConstStrA("Exception while processing handler") + ConstStrA(typeid(*listeners[i]).name())));
				throw;
			}
		}
	} catch (std::exception &e) {
		dbfile->setSize(writePos);
		LS_LOG.fatal("Error during update database: %1 - this is fatal - position: %2 [byte]") << e.what() << writePos;
		throw;
	}

	writePos += wrcount;

	natural limitCheck = watcher.checkLimit(writePos,time);
	if (limitCheck > 0) {
		LS_LOG.warning("Write limit in effect, waiting for %1 seconds") << limitCheck;
		Timeout waitTill(limitCheck*1000);
		Thread::deepSleep(waitTill);
	}



	return (writePos - wrcount)/blockSize + offset;
}

void EventLog::open(ConstStrW name, natural flags , ILogDiscovery *discovery) {
	bool ro = (flags & flagReadOnly) != 0;
	PRndFileHandle f =  IFileIOServices::getIOServices().openRndFile(name,
			ro?IFileIOServices::fileOpenRead:IFileIOServices::fileOpenReadWrite,
					OpenFlags::create);
	PRndFileHandle f2 =  IFileIOServices::getIOServices()
		.openRndFile(String(name+ConstStrW(L".cells")),
			ro?IFileIOServices::fileOpenRead:IFileIOServices::fileOpenReadWrite,
					OpenFlags::create);

	open(f,f2,(flags & flagRescan) != 0,discovery);

}

void EventLog::close() {
	dbfile = nil;
	fixedfile = nil;
}


void EventLog::open(PRndFileHandle file, PRndFileHandle fixed, bool rescan, ILogDiscovery *discovery) {
	dbfile = file;
	fixedfile = fixed;
	if (fixedfile != nil) {
		scanStEvents();
	}
	this->discovery = discovery;
	if (rescan) {
		this->rescan();
	}
}

void EventLog::rescan() {
	LS_LOGOBJ(lg);
	Sync _(lock);
	for (natural i = 0; i < listeners.length(); i++) {
		listeners[i]->onStartRescan();
	}

	if (discovery != nil) {
		PInputStream instr = discovery->getFirstLog();
		while (instr != nil) {
			rescanOtherLog(instr);
			instr = discovery->getNextLog();
		}
	}

	writeState.currentCheckSum = initialChecksum;
	writeState.lastTimestamp = 0;
	FileOffset size = dbfile->size();
	writePos = rescan(0,0,size,writeState.currentCheckSum,writeState.lastTimestamp);

	rescanStEvents(0);

	for (natural i = 0; i < listeners.length(); i++) {
		listeners[i]->onEndRescan();
	}


	if (writePos != size) {
		throw ErrorMessageException(THISLOCATION,"Fatal: Unexpected end of file found");
	}

}

natural EventLog::readRecord(FileOffset offset, void* data, natural size, natural recordType) {
	Header hdr;
	offset *= blockSize;
	dbfile->read(&hdr,sizeof(hdr), offset);
	natural sz = hdr.recordSize;
	if (hdr.recordType != recordType)
		throw ErrorMessageException(THISLOCATION, "Invalid offset - recordType doesn't match header");
	if (sz < size) size = sz;
	dbfile->read(data,size,offset+sizeof(hdr));
	return sz;
}

EventLog::~EventLog() {
	for (natural i = 0; i < listeners.length(); i++) {
		listeners[i]->onRelease();
	}
}

template<typename T>
natural EventLog::parseBlock(SeqFileInput input, Header &hdr, Bin::natural16 &checksum, time_t &curtime,T &buffer, bool noinitialchecksumcheck ) const {
	natural ofs = 0;
	input.blockRead(&hdr,sizeof(hdr),true);
	if (hdr.checksum != checksum){
		if (noinitialchecksumcheck) {
			checksum = hdr.checksum;
		} else {
			throw ChecksumError(THISLOCATION, hdr.checksum, checksum, ofs);
		}
	}
	if (hdr.recordType == 0xFFFF && hdr.recordSize == 0) {
		curtime += ((natural)hdr.timeadv) << 16;
		ofs+= sizeof(hdr);
	} else {
		curtime += hdr.timeadv;
		natural sz = hdr.recordSize * EventLog::blockSize;
		buffer.resize(sz);
		input.blockRead(buffer.data(),buffer.length(),true);
		ofs+=sizeof(hdr) + sz;
		updateChecksum(buffer, checksum);
	}
	checksum += hdr.timeadv;
	checksum += hdr.recordType;

	return ofs;
}


const char *str_rescanErrorMsg = "Rescan error at position: %1 [byte] - opcode: %2";

EventLog::FileOffset EventLog::rescan(IUpdateListener* listener, FileOffset from,
		FileOffset to, Bin::natural16 &checksum, time_t &curtime,
		bool noinitialchecksumcheck) const {

	SeqFileInput reader(dbfile,from);
	SeqFileInBuff<> rdbuff(reader);
	FileOffset ofs = from;
	AutoArray<byte, SmallAlloc<1024> > buffer;
	while (ofs < to) {
		Header hdr;
		try {
			natural adv = parseBlock(rdbuff,hdr,checksum,curtime,buffer,noinitialchecksumcheck);
			noinitialchecksumcheck = false;
			if (hdr.recordType != 0xFFFF || hdr.recordSize != 0) {
					if (listener) {
						listener->onUpdate(ofs/blockSize,curtime,hdr.recordType,ConstBin(buffer));
					} else {
						for (natural i = 0; i < listeners.length(); i++) {
							listeners[i]->onUpdate(ofs/blockSize,curtime,hdr.recordType,ConstBin(buffer));
						}
					}
			}
			ofs+=adv;
		} catch (Exception &e) {
			throw RescanErrorException(THISLOCATION, ofs, hdr.recordType) << e;
		}
	}
	return ofs;
}


EventLog::FileOffset EventLog::fileSize() const {
	Sync _(lock);
	return dbfile->size();
}

void EventLog::ChecksumError::message(ExceptionMsg& msg) const {
	msg("Corrupted database file - checksum error at %1 - expected: %2, found %3")
			<< offset << expected << found;
}

static void checkSlaveExcept(EventLog& owner) {
	if (owner.isSlave())
		throw ErrorMessageException(THISLOCATION,
				"Can't open write transaction under slave mode");
}

EventLog::Transaction::Transaction(EventLog& owner, const time_t& time)
:owner(owner),curTime(time)
{
	checkSlaveExcept(owner);
	owner.lock.lock();
}


EventLog::Transaction::Transaction(EventLog& owner)
:owner(owner) {
	checkSlaveExcept(owner);
	owner.lock.lock();
	time(&curTime);
}

EventLog::Transaction::~Transaction() {
	owner.lock.unlock();
}

EventLog::FileOffset EventLog::Transaction::sendUpdate(natural recordType,
		ConstBin recordData, natural cellId) throw () {

	try {
		if (cellId != 0) return owner.updateStEvent_trn(cellId,recordType, recordData, curTime);
		else return owner.sendUpdate_trn(recordType,recordData,curTime);
	} catch (...) {
		owner.close();
		std::unexpected();
		return 0;
	}

}

EventLog::FileOffset EventLog::rescan(IUpdateListener& listener, FileOffset from,
		FileOffset to, time_t curTime) const {
	listener.onStartRescan();
	from *= blockSize;
	to *= blockSize;
	if (to == 0) to = fileSize();
	Bin::natural16 checksum;
	FileOffset ret = rescan(&listener,from,to,checksum,curTime,true);
	rescanStEvents(&listener);
	listener.onEndRescan();
	return ret /= blockSize;
}


void EventLog::setMaxUpdateSpeed(natural maxSpeedInBytes) {
	watcher.maxSpeed = maxSpeedInBytes ;
}




ReplicationListener::ReplicationListener(IReplicationOutput& out):out(out) {
}

void ReplicationListener::bind(EventLog& db, EventLog::FileOffset scanFrom,
		time_t timeBase) {

	lastEventTime = 0;
	timeOffset = 0;
	EventLog::FileOffset scanned = db.rescan(*this,scanFrom,0,timeBase);
	Synchronized<FastLockR> _sync(db.lock);
	db.rescan(*this,scanned,0,timeBase);
	db.addListener(this);
	const EventLog::WriteState &wrState = db.getWriteState();
	timeOffset = wrState.lastTimestamp - lastEventTime;


}


void ReplicationListener::onUpdate(FileOffset offset, time_t timestamp,
		natural recordType, RecData data) {

	onUpdateCell(0,offset,timestamp,recordType,data);

}

void ReplicationListener::onUpdateCell(natural cellId, FileOffset,
		time_t timestamp, natural recordType, RecData data) {

	try {
		natural recsize = sizeof(Header) + data.length() - 1;
		Record *rec = (Record *)alloca(recsize);
		rec->timestamp = Bin::natural32 (timestamp - lastEventTime);
		rec->recordType = recordType;
		rec->length = data.length();
		rec->cellId = cellId;
		memcpy(rec->data, data.data(),data.length());
		out.write(&rec, recsize);
		lastEventTime = timestamp;

	} catch (std::exception &) {

	}

}


natural EventLog::allocCell() {
	return lockInc(nextCellId) - 1;
}

void EventLog::expandCellMap(natural cellId) {
	if (cellId >= cellMap.length()) {
		if (cellId > cellMap.length()+10000) throw ErrorMessageException(THISLOCATION, "Too large gap in the cellmap");
		cellMap.resize(cellId+1);
	}
}

EventLog::FileOffset EventLog::updateStEvent_trn(natural cellId, natural recordType, ConstBin recordData, time_t time) {
	if (fixedfile == nil) throwNullPointerException(THISLOCATION);
	expandCellMap(cellId);
	natural granSize = (recordData.length() + 7)/blockSize;
	StFrameInfo &f = cellMap(cellId);
	natural needSz = sizeof(StFrame)+granSize*blockSize;
	if (f.length < needSz) {
		f.offset = fixedfile->size();
		f.length = needSz;
	}
	SeqFileOutput out(fixedfile, f.offset);
	StFrame hdr;
	hdr.frameLen = f.length;
	hdr.hdr.cellId = cellId;
	hdr.hdr.length = granSize;
	hdr.hdr.recordType = recordType;
	hdr.hdr.timestamp = (Bin::natural32)time;
	out.blockWrite(&hdr, sizeof(hdr), true);
	out.blockWrite(recordData, true);
	natural remain = granSize*blockSize - recordData.length();
	if (remain)
		for (natural i = 0; i < remain; i++) out.write(0);
	out.flush();

	try {
		for (natural i = 0; i < listeners.length(); i++) {
			try {
				listeners[i]->onUpdateCell(cellId,f.offset,time,recordType,recordData);
			} catch (Exception &e) {
				e.appendReason(ErrorMessageException(THISLOCATION, ConstStrA("Exception while processing handler") + ConstStrA(typeid(*listeners[i]).name())));
				throw;
			}
		}
	} catch (std::exception &e) {
		LS_LOG.fatal("Error during update database: %1 - this is fatal - position: %2 [byte]") << e.what() << f.offset;
		throw;
	}

	return f.offset;
}


void EventLog::scanStEvents() {
	cellMap.clear();
	FileOffset position = 0;
	FileOffset len = fixedfile->size();
	while (position < len) {
		StFrame hdr;
		fixedfile->read(&hdr,sizeof(hdr), position);
		StFrameInfo nfo;
		nfo.offset = position;
		nfo.length = hdr.frameLen;
		expandCellMap(hdr.hdr.cellId);
		cellMap(hdr.hdr.cellId) = nfo;
		position += hdr.frameLen;
	}
}

void EventLog::rescanStEvents(IUpdateListener* listener) const {
	for (CellMap::Iterator iter = cellMap.getFwIter(); iter.hasItems();) {
		const StFrameInfo &ofs = iter.getNext();
		loadStEvent(ofs.offset, listener);
	}
}

void EventLog::rescanOtherLog(PInputStream input) {
	AutoArray<byte> buffer;
	SeqFileInBuff<> infile(input);
	Bin::natural16 checksum = initialChecksum;
	Header hdr;
	time_t tm = 0;

	while (infile.hasItems()) {
		parseBlock(input,hdr,checksum,tm,buffer,false);
		for (natural i = 0; i < listeners.length(); i++) {
			try {
				listeners[i]->onUpdate(0,tm,hdr.recordType,RecData(buffer.head(hdr.recordSize*blockSize)));
			} catch (Exception &e) {
				e.appendReason(ErrorMessageException(THISLOCATION, ConstStrA("Exception while processing handler") + ConstStrA(typeid(*listeners[i]).name())));
				throw;
			}
		}
	}
}

void EventLog::loadStEvent(FileOffset offset, IUpdateListener* listener) const {
	StFrame hdr;
	fixedfile->read(&hdr,sizeof(hdr),offset);
	natural datasz = hdr.hdr.length*blockSize;
	byte *buff = (byte *)alloca(datasz);
	fixedfile->read(buff,datasz,offset+sizeof(hdr));
	ConstBin recordData(buff,datasz);
	if (listener == 0) {
		try {
			for (natural i = 0; i < listeners.length(); i++) {
				try {
					listeners[i]->onUpdateCell(hdr.hdr.cellId,offset,hdr.hdr.timestamp,hdr.hdr.recordType,recordData);
				} catch (Exception &e) {
					e.appendReason(ErrorMessageException(THISLOCATION, ConstStrA("Exception while processing handler") + ConstStrA(typeid(*listeners[i]).name())));
					throw;
				}
			}
		} catch (std::exception &e) {
			LS_LOG.fatal("Error during update database: %1 - this is fatal - position: %2 [byte]") << e.what() << offset;
			throw;
		}
	} else {
		listener->onUpdate(offset,hdr.hdr.timestamp,hdr.hdr.recordType,recordData);
	}
}

const char *str_invalidOpcodeMsg = "Invalid opcode: %1";

}

}

