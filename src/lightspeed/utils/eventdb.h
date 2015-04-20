/*
 * DBFile.h
 *
 *  Created on: 26. 5. 2014
 *      Author: ondra
 */

#ifndef _LIGHTSPEED_UTILS_EVENTDB_
#define _LIGHTSPEED_UTILS_EVENTDB_
#include <time.h>
#include <string.h>
#include "../base/containers/constStr.h"
#include "../base/streams/fileio.h"
#include "../base/containers/autoArray.h"
#include "../mt/mutex.h"
#include "../base/streams/random.h"
#include "../base/exceptions/errorMessageException.h"
#include "../base/containers/map.h"
#include "../mt/thread.h"



namespace LightSpeed {

///Event database is database updated by events.
/**
 * Actually EventDB is not complete database. EventDB classes only helps to build
 * simple memory database with ability to store and replicate database state.
 *
 * State of database is stored as series of update events. Every event is single
 * change of the database. All changes in the database must be performed through the
 * EventDB interface. Changes are stored to the disk file. That file can be later
 * replayed to achieve the same state of the database.
 *
 * To use EventDB, you need to install various listeners (extends UpdateAdapter) that
 * receive updates and performs storing and indexing data in the memory. To store data it
 * is better to use standard containers. Other parts of software can read data directly
 * from the these containers. But changes to them must be provided through the EventDB.
 *
 * EventDB is responsible to store and replay events, replicate events.
 *
 * Events are represented as simple structures in POD format (Plain-old data, structure
 * which consists from members of standard types and POD compatibile types). Event can also
 * contain one string or one array with dynamic size where each item id POD compatible type.
 *
 * @code
 * struct SimpleEvent {
 *    int val1;
 *    float val2
 * }
 * @endcode
 *
 * To create event with string value, use following technique
 *
 * @code
 * struct EventWithString {
 * 	  int val1;
 * 	  char stringval[1];
 * }
 * @endcode
 *
 * The member stringval should be always one character length. While event is stored,
 * system puts whole string beyond the structure so, they can be later accessible directly
 * by the variable
 *
 * @code
 * struct  EventWithTwoStrings {
 * 	  int val1;
 * 	  int strval1len;
 * 	  char strvals[1];
 * @endcode
 *
 * Because you cannot have two or more strings in the single event, you can store
 * these strings as concatenation of both string, and store length of the first string
 * to the member variable. You can then load whole string into ConstStrA and use head and tail
 * to split this string into two.
 *
 *
 * Format of the event file
 * @code
 * (8 bytes)
 * +----------------+-----------------+--------------+----------------+-------------------+
 * |  checksum (2b) |  eventType (2b) |  length (2b) |  timeDiff (2b) |   data (variable) |
 * +----------------+-----------------+--------------+----------------+-------------------+
 * @endcode
 *
 * @b checksum two bytes of checksum calculated by EventLog object. It used to check
 *  integrity of the log file. First record has predefined value and it is used as magic value
 * @b eventType user defined type of event stored in this record. There can be
 *     65535 event types. Event 0xFFFF is reserved and should not be used
 * @b length length of the data section. Length is stored in 8b granuality. Value 0 means
 *    that the event has no data. Value 1 means, that there are 8 bytes of the data. Value 2 means
 *    that the event has 16 bytes, and so on.
 * @b timeDiff count of seconds ellapsed from previous event. Field is used to
 *    store time when event was recorded. Time is stored in seconds (EventLog cannot to
 *    handle shorter interval. If there is more then 65535 seconds between two
 *    events, special event 0xFFFF is put between and timeDiff contains high two bytes
 *    of the difference. First event put into log file is always put with 0xFFFF prefix,
 *    because time of "previous event" is zero.
 *
 *
 * Format of the cell.
 *
 * Cells are stored in the special cell file. Cells are used to store events that
 * are often updated and application logic doesn't need to remember their previous state.
 * Each event, that replaces its previous state has its own cellId. Any event with
 * the same cellId replaces previous event with the same cellId
 *
 * @code
 * (16 bytes)
 * +----------------+------------------+---------------+-------------------+---------------+-----------------+
 * |  frameLen(4b)  |  timestamp (4b)  |  cellId (4b)  |  recordType (2b)  |  length (2b)  | data (variable) |
 * +----------------+------------------+---------------+-------------------+---------------+-----------------+
 * @endcode
 *
 * @b frameLen length of the cell frame. Frame can be larger than event stored in it. Extra space can be
 * used later when event takes more space. If the frame is not large enough, new frame is allocated
 * @b timestamp time of store. Timestamp is always positive number, so it can go beyond 0x7FFFFFFF
 * @b cellId id of this cell
 * @b recordType type of record
 * @b length length of the data in 8b granulate
 */

namespace EventDB {

///Contains data of a record
/**
 * you can use either fixed() if event has fixed length or dynamic() when event has dynamic length
 */
template<natural align>
class RecDataT: public ConstBin {
public:
	RecDataT(ConstBin b):ConstBin(b) {}
	RecDataT() {}

	struct Fixed {
		const RecDataT &owner;

		Fixed(const RecDataT &owner):owner(owner) {}
		template<typename T>
		operator const T & () const;
	};

	struct Dynamic {
		const RecDataT &owner;

		Dynamic(const RecDataT &owner):owner(owner) {}
		template<typename T>
		operator const T & () const;

		template<typename T>
		ConstStringT<T> extractArray(const T *arrayPtr) const {
			const byte *start = owner.data();
			const byte *arrstart = reinterpret_cast<const byte *>(arrayPtr);
			return ConstStringT<T>(arrayPtr, (owner.length() - (arrstart - start))/sizeof(T));
		}

	};


	Fixed fixed() const {return Fixed(*this);}
	Dynamic dynamic() const {return Dynamic(*this);}


};

class EventLogHdrs {
protected:
	struct Header {
		///lowest 16-bits of checksum
		Bin::natural16 checksum;
		///type of record - user defined (0xFFFF = time sync)
		Bin::natural16 recordType;
		///size of record in divided by 8 (multiply this value by 8)
		Bin::natural16 recordSize;
		///time advice - how many seconds ellapses between this and previous record
		Bin::natural16 timeadv;
	};

public:
	static const natural blockSize = sizeof(Header);

};

typedef RecDataT<EventLogHdrs::blockSize> RecData;

class IUpdateListener {
public:
	typedef RndFileBase::FileOffset FileOffset;
	typedef FileOffset ID;

	///Sends update to the listener
	/**
	 * @param offset offset in the file - note that offset is in the clusters (8 bytes). This value
	 *          can be zero in case, that event is not located in active database file.
	 *          This can happen when cell is processed (because onUpdateCell is not
	 *          defined) or when event appear in a rotated log. Rotated logs can be
	 *          compressed or read sequentially, so there is no way how to access this
	 *          log later.
	 *
	 * @param timestamp time of the event
	 * @param recordType type of the event
	 * @param data data of the event
	 * @retval true continue listening
	 * @retval false disable listening - remove listener or stop rescanning
	 */
	virtual void onUpdate(FileOffset offset, time_t timestamp, natural recordType, RecData data) = 0;
	///Called when cell is updated
	/**
	 * function can be used to perform special action when event came from cell.
	 * Default implementation for the DBUpdateAdapter is call onUpdate
	 * discarding information about cellId and with offset set to zero (which
	 * makes this field invalid)
	 *
	 * @param cellId id of cell
	 * @param offset offset in cell file
	 * @param timestamp time of last event
	 * @param recordType type of event
	 * @param data data of the event
	 */
	virtual void onUpdateCell(natural cellId, FileOffset offset, time_t timestamp, natural recordType, RecData data) = 0;
	///called before rescan is performed
	/**
	 * Listener can prepare itself to rescan operation, for example to clear its internal state
	 */
	virtual void onStartRescan() = 0;
	///called after all events are replayed
	/**
	 * Lister can prepare itself for realtime data
	 */
	virtual void onEndRescan() = 0;
	///Called when source database is perfoming destruction of the instance
	virtual void onRelease() = 0;
	virtual ~IUpdateListener() {}
};

///Replication output stream
/**
 * Very simplified output stream to write replication data
 */
class IReplicationOutput {
public:
	///Writes replication data to the output
	/**
	 * @param data data to write
	 * @param size size of the data
	 *
	 * @note Function must not throw an exception. If you experience
	 * with failure, store failstate into the object and deal with it
	 * later or in an another thread
	 *
	 */
	virtual void write(const void *data, natural size) throw() = 0;
	virtual ~IReplicationOutput() {}
};

class IReplicationInput {
public:
	///Reads replication data from the input
	/**
	 * @param data buffer where to put data
	 * @param size size of buffer. Function must read all requested data
	 * @retval true completed
	 * @retval false end of file (data cannot be read complete)
	 *
	 * @note Function must not throw an exception. If you experience
	 * with failure, store failstate into the object and deal with it
	 * later or in an another thread. In failstate function can return false
	 * to terminate reading prematurely
	 */

	virtual bool read(void *data, natural size) throw()  = 0;
	virtual ~IReplicationInput() {}
};

class ILogDiscovery {
public:
	///resets log discovery and returns first log file
	/**
	 * @return opened log or nil if there is no log available
	 */
	virtual PInputStream getFirstLog()  = 0;
	///opens next log.
	/**
	 * @return opened log or nil if there is no more logs available
	 */
	virtual PInputStream getNextLog()  = 0;
	virtual ~ILogDiscovery() {}
};

class UpdateAdapter: public IUpdateListener {
public:
	virtual void onUpdate(FileOffset , time_t , natural , RecData ) {}
	virtual void onUpdateCell(natural, FileOffset, time_t timestamp, natural recordType, RecData data) {
		onUpdate(0,timestamp, recordType, data);
	}
	virtual void onStartRescan() {}
	virtual void onEndRescan() {}
	virtual void onRelease() {}
};


///Main class of the event database
/**
 * Construct object from this class, define listeners and specify output file. It will
 * be rescaned to put database into last know state.
 *
 *
 * To send update, construct object Transaction and use SendUpdateT function
 */
class EventLog: public EventLogHdrs {


public:
	typedef RndFileBase::FileOffset FileOffset;


	EventLog():slaveMode(false),nextCellId(1) {}

	void addListener(IUpdateListener *listener);
	///Adds listener a perform synchronization
	/**
	 * @param listener listener to add
	 */
	void addListenerAndSync(IUpdateListener *listener);
	void removeListener(IUpdateListener *listener);



	class Transaction {
	public:

		///Creates transaction
		/**
		 * @param owner database where to create transaction
		 */
		Transaction(EventLog &owner);
		///Creates transaction allowing to specify the time
		/**
		 * This function helps to replication when time is carried with the transaction from the master
		 *
		 * @param owner database
		 * @param time timestamp
		 *
		 * @note when timestamp is less than timestamp of the previous event, exception is thrown
		 * when event is sent
		 */
		Transaction(EventLog &owner, const time_t &time);
		~Transaction();

		///Sends update to the database file and broadcasts this update to the all listeners
		/**
		 * @param recordType record type
		 * @param recordData data of the record
		 * @param cellId if nonzero, stores event into specified cell. Cell must be allocated
		 *            by the function allocCell().
		 * @return offset of the record.
		 */
		FileOffset sendUpdate(natural recordType, ConstBin recordData, natural cellId = 0) throw();

		///Sends update to the database file and broadcasts this update to the all listeners
		/**
		 * @param recordType record type
		 * @param data any data in POD compatible form. Data are directly copied into the file
		 * @param cellId if nonzero, stores event into specified cell. Cell must be allocated
		 *            by the function allocCell().
		 * @return
		 */
		template<typename T>
		FileOffset sendUpdateT(natural recordType, const T &data, natural cellId = 0) {
			return sendUpdate(recordType,ConstBin(reinterpret_cast<const byte *>(&data),sizeof(data)),cellId);
		}

		///Sends update with string
		/** Function expects that structure ends with char[1] variable, where first character of the string
		 * will be mapped. String content is placed into str variable. String should not contain character
		 * 'zero' becaused zero is used as end of the string. However, you can use this feature
		 * to pass multiple strings each terminated by zero. Then last string is separated by two zeroes.
		 * @param recordType record type
		 * @param data structure in POD compatible form with member variable containing char[1]
		 * @param trgStr pointer to member variable contains char[1]. It must lay within the structure
		 * @param str value of the string variable. Function always put terminating zero beyond the string.
		 * @param cellId if nonzero, stores event into specified cell. Cell must be allocated
		 *            by the function allocCell().
		 *
		 * @note if you need to store string with zero character, you have to put length
		 * of the string within the data structure.
		 */
		template<typename T>
		FileOffset sendUpdateStr(natural recordType, const T &data,  char const (*trgStr)[1], ConstStrA str, natural cellId = 0) {
			natural headsz = reinterpret_cast<const byte *>(trgStr) - reinterpret_cast<const byte *>(&data);
			assert (headsz < sizeof(T));
			natural needsz = headsz + str.length() + 1;
			byte *b = (byte *)alloca(needsz);
			memcpy(b,&data,headsz);
			memcpy(b+headsz,str.data(),str.length());
			b[needsz - 1] = 0;
			return sendUpdate(recordType,ConstBin(b,needsz), cellId);
		}

		///Sends update containing an array
		/**
		 * Function works similar as sending string, you just can specify type of
		 * the element of the array. Elements must be also in POD compatible form
		 * @param recordType record type
		 * @param data structure in POD compatible form with member variable containing A[1].
		 * @param trgStr pointer to member variable contains A[1]. It must lay within the structure
		 * @param str reference to the array
		 * @param cellId if nonzero, stores event into specified cell. Cell must be allocated
		 *            by the function allocCell().
		 *
		 * @note Size of the array is not stored. You should handle it by yourself. You cannot
		 * use size of final record because it can contain a padding (unless the array element
		 * has more than 7 bytes)
		 */
		template<typename T, typename A>
		FileOffset sendUpdateArr(natural recordType, const T &data,  A const (*trgStr)[1], ConstStringT<A> str, natural cellId = 0) {
			natural headsz = reinterpret_cast<const byte *>(trgStr) - reinterpret_cast<const byte *>(&data);
			assert (headsz < sizeof(T));
			natural needsz = headsz + str.length()*sizeof(A);
			byte *b = (byte *)alloca(needsz);
			memcpy(b,&data,headsz);
			memcpy(b+headsz,str.data(),str.length()*sizeof(A));
			return sendUpdate(recordType,ConstBin(b,needsz), cellId);
		}

		///allocates cell id
		/**
		 * @return new cell id
		 *
		 * @note allocating of the cell is not stored anywhere. To achieve the same
		 * state after replaying the log, you have to store result with some event
		 * to the log file. You should also store the first event to the newly allocated
		 * cell to make cell visible during replaying
		 *
		 * Other strategy is to allocate cells during processing the updates by some of
		 * the listeners. Replaying the log file should also achieve the same state
		 *  of the cell file allocation
		 */
		natural allocCell() {
			return owner.allocCell();
		}

protected:
		EventLog &owner;
		///new - time of event is time of the transaction
		time_t curTime;

	};

	static const natural flagReadOnly = 1;
	static const natural flagRescan = 2;

	friend class Transaction;

	///reads record from database file
	/**
	 * @param offset offset (listeners should know it)
	 * @param data pointer to buffer
	 * @param size size of buffer
	 * @param recordType - type of expected record - must match header
	 * @return size fo data
	 */
	natural readRecord(FileOffset offset, void *data, natural size, natural recordType);

	///Open database log
	/**
	 * @param name name of log file
	 * @param flags additional flags
	 * @param discovery pointer to interface that is able to open additional log files
	 *   These log files are read before main file is opened. Interface was introduced to
	 *   support rotated logs. Object should open all historical logs.
	 *   Note object must stay valid until the database is closed
	 *
	 * @note to perform logrotate, close the the database and reopen it without rescaning
	 */
	void open(ConstStrW name, natural flags = flagRescan, ILogDiscovery *discovery = 0);


	void open(PRndFileHandle file, PRndFileHandle cellFile,  bool rescan = true, ILogDiscovery *discovery = 0);
	void close();

	void rescan();

	~EventLog();


	///Reads next string in multistring update
	/**
	 * Function cannot detect that input is multistring. It only expects, that
	 * end of multistring is recorded as double zeroes.
	 * @param str first or last known string. (first string is alsways referenced from the header)
	 * @return pointer to next string or NULL, if no more strings
	 */
	static const char *getNextStr(const char *str) {
		const char *c = str;
		if (c == 0) return 0;
		while (*c) c++;
		c++;
		if (*c == 0) return 0; else return c;
	}




	class ChecksumError: public Exception {
		LIGHTSPEED_EXCEPTIONFINAL;
	public:
		ChecksumError(const ProgramLocation &loc, Bin::natural16 found, Bin::natural16 expected, FileOffset offset)
			:Exception(loc),found(found),expected(expected),offset(offset) {}
		Bin::natural16 getExpected() const {
			return expected;
		}

		Bin::natural16 getFound() const {
			return found;
		}

		FileOffset getOffset() const {
			return offset;
		}
	protected:
			Bin::natural16 found,expected;
			FileOffset offset;

			void message(ExceptionMsg &msg) const;
	};


	struct WriteState {
		  Bin::natural16 currentCheckSum;
		  time_t lastTimestamp;
		  Rand rnd;

		  WriteState():currentCheckSum(0),lastTimestamp(0) {}
	};

	struct WriteState2: WriteState {
		FileOffset ofs;

		WriteState2(const WriteState &other):WriteState(other) {}
		WriteState2() {}
		WriteState2(const WriteState &other, FileOffset ofs):WriteState(other),ofs(ofs) {}
	};


	///rescans file for single listener in specified range
	/**
	 * @param listener listener that receives data.
	 * @param from address to start from - virtual offset in blocks - set 0 to start from begining
	 * @param to address where to end - virtual offset in blocks - set 0 to process remaining of the file
	 * @param curTime time offset.
	 * @return virtual offset in blocks, where scanning stopped
	 *
	 * @note This can be good to scan database for historical changes in specified range. Note that
	 * you need to know bounds of that range, - it means from, to and cutTime values. The best technique
	 * is to build index for interesting events which can contain above values. Then scanning between
	 * these events can be easy.
	 */
	FileOffset rescan(IUpdateListener &listener, FileOffset from, FileOffset to, time_t curTime) const;

	///Initializes replication state and returns last known offset of local database copy
	/**
	 * Function initializes internal structures. You have to call this function everytime you reconnect to master server.
	 * @return information of event log file. It contains position and time of last event which
	 * may be send to the other side
	 *
	 * @note function also switch object into slave mode. Any write not made by master is rejected.
	 */
	WriteState2 initReplication();
	///Reads data to from master to replicate. Object must be in slave mode, otherwise exception is emited
	/**
	 *  Function reads packets from master. If this function is called right after initReplication, function
	 *  expects first packet and adjusts all other internal states to be in sync with the master
	 *
	 * @param input input stream
	 * @param timeOffset client-side specified time offset. It is exchanged during initialization. In case
	 * that client sent its timestamp, there can be zero. If client sent zero of timestamp, this
	 * value should be equal to last event time.
	 *
	 * Function can throw exception, if stream is corrupted or lost during reading. In this case, you have
	 * to call initReplication and reconnect the master
	 */
	bool readReplicationData(IReplicationInput &input, time_t timeOffset = 0);

	///Stops replication and switch to the master mode
	/** can be used, when master fails. Current slave can be switched back to the
	 * master mode and maintain current database state.
	 */
	void stopReplication();

	///Determines slave state
	/**
	 * @retval true object is in slave state
	 * @retval false object is in master state
	 */
	bool isSlave() const;


	static natural writeBlock(SeqFileOutput toStream,
							  	  natural recordType,
								  ConstBin recordData,
								  time_t curTime,
								  WriteState &state,
								  natural &offset);


	void setMaxUpdateSpeed(natural maxSpeedInBytes);


	///Allocates cell index
	/** Cell is database event which is not stored with history
	 * Recent event replaces previous. Cells are great to store values that are changing
	 * too often and where the recent value is more important then history. Cells use
	 * separate partition of the database and they are also replicated. In time of the update,
	 * cells are send as the last events
	 *
	 * @return new cell ID.
	 *
	 * @note To correct use of the cells ids, always write ID of newly allocated cell into
	 * database as the standard event. Note that you cannot delete cell, it is reserved forever.
	 */
	natural allocCell();

protected:
	PRndFileHandle dbfile;
	PRndFileHandle fixedfile;
	typedef AutoArray<IUpdateListener *> Listeners;
	Listeners listeners;
	mutable FastLockR lock;
	typedef Synchronized<FastLockR> Sync;
	Pointer<ILogDiscovery> discovery;

	struct FixedHeader {
		Bin::natural32 timestamp;
		Bin::natural32 cellId;
		Bin::natural16 recordType;
		Bin::natural16 length;

	};

	struct FixedRecord: FixedHeader {
		byte data[1];
	};


	struct StFrame {
		Bin::natural32 frameLen;
		FixedHeader hdr;
	};

	struct StFrameRecord: StFrame {
		byte data[1];
	};

	struct StFrameInfo {
		FileOffset offset;
		natural length;

		StFrameInfo():offset(0),length(0) {}
	};

	typedef AutoArray<StFrameInfo> CellMap;
	CellMap cellMap;

	///Peform rescan for given listener
	/**
	 * @param listener listener that receives rescanned data. If NULL, all registered listeners receive data
	 * @param from offset from - must be physical offset
	 * @param to offset to - must be physical offset
	 * @param checksum - initial checksum (and final checksum)
	 * @param curtime - absolute time of start scanning - set zero when you start from begining
	 * @param noinitialchecksumcheck - set true, if you don't want to check the checksum of the first block (i.e. you don't know the checksum)
	 * @return offset where stopped
	 */
	FileOffset rescan(IUpdateListener *listener, FileOffset from, FileOffset to, Bin::natural16 &checksum, time_t &curtime, bool noinitialchecksumcheck = false) const;
	FileOffset fileSize() const;

	void rescanOtherLog(PInputStream input);

	void scanStEvents();
	void rescanStEvents(IUpdateListener *listener = 0) const;
	void loadStEvent(FileOffset offset, IUpdateListener *listener = 0) const;


public:

	///calculates count of blocks used by specified record type
	/** @tparam T record type
	 *
	 * result is stored in 'result' variable.
	 *
	 * @note each block consists from header and data. Header takes 8 bytes. Block size is
	 * block of one header. Each record takes one block for header and n blocks for data, where
	 * n is calculated as ceil(sizeof(T)/8). Not useful for dynamic sized block, because they
	 * can take more blocks depend on size. You can use this number to divide value returned by getNextID()
	 * to generate unique IDs for given record type, note that such ID cannot be used to
	 * calculate offset.
	 */
	template<typename T>
	struct CountBlocks {
		static const natural result = ((sizeof(T) + blockSize - 1)/blockSize + 1);
	};

	///retrieves time of last event
	const WriteState &getWriteState() const {return writeState;}

protected:

	FileOffset writePos;
	WriteState writeState;

	bool slaveMode;
	atomic nextCellId;


	struct UpdateSpeedWatcher {

	///speed in bytes in seconds allowed in total. When speed is above exception is thrown
	natural maxSpeed;
	time_t startTime;
	FileOffset startSize;

	UpdateSpeedWatcher():maxSpeed(naturalNull),startTime(0),startSize(0) {}

	natural checkLimit(FileOffset curSize, time_t curTime) {
		FileOffset diff = curSize - startSize;
		FileOffset sec = diff/maxSpeed;
		time_t needTime = startTime+sec;
		if (needTime > curTime) {
			return (natural)(needTime - curTime);
		} else {
			startTime = curTime;
			startSize = curSize;
			return 0;
		}
	}

	};

	UpdateSpeedWatcher watcher;




	///Sends update to the database file and broadcasts this update to the all listeners
	/**
	 * @param recordType record type
	 * @param recordData data of the record
	 * @return offset of the record
	 */
	FileOffset sendUpdate_trn(natural recordType, ConstBin recordData, time_t time);

	FileOffset updateStEvent_trn(natural cellId, natural recordType, ConstBin recordData, time_t time);

	template<typename T>
	natural parseBlock(SeqFileInput input, Header &hdr, Bin::natural16 &checksum, time_t &curtime,T &buffer, bool noinitialchecksumcheck = false ) const;

	void expandCellMap(natural cellId);
	friend class ReplicationListener;

};


class ReplicationListener: public UpdateAdapter {
public:

	typedef EventLog::FixedHeader Header;
	typedef EventLog::FixedRecord Record;

	///Creates replication listener with binary output
	/**
	 * @param out stream to output data
	 *
	 * @note any exception thrown from the stream is ignored and causes that
	 * replication listener is removed from the database
	 */
	ReplicationListener(IReplicationOutput &out);

	///Bind listener to the database and run rescan operation
	/**
	 * Use this method to bind listener. Do not bind listener directly,it will not perform scanning
	 *
	 * @param db database to bind
	 * @param scanFrom offset in the replication file
	 * @param timeBase time base - because time is always relative to previous
	 *   event, rescanning is unable to recover time of event on specified offset
	 *   Thus replication slave must supply own time to calculate absolute time
	 *   It can also supply zero and perform time calculation o the client side
	 *
	 *
	 * Rescanning is performed everytime even if scanFrom equal to the top
	 * of the database because it always sent update of all non-cellable events
	 */
	void bind(EventLog &db, EventLog::FileOffset scanFrom, time_t timeBase);


protected:
	time_t timeOffset,lastEventTime;
	IReplicationOutput &out;

	virtual void onUpdate(FileOffset offset, time_t timestamp, natural recordType, RecData data);
	virtual void onUpdateCell(natural cellId, FileOffset offset, time_t timestamp, natural recordType, RecData data);


};




template<natural align>
template<typename T>
inline RecDataT<align>::Fixed::operator const T & () const {
	static const natural reqSize = ((sizeof(T) + align - 1)/align) * align;
	if (owner.length() != reqSize)
		throw ErrorMessageException(THISLOCATION,ConstStrA("Unable to load record, size mistmatch: ")+ ConstStrA(typeid(T).name()));
	return reinterpret_cast<const T &>(*owner.data());
}

template<natural align>
template<typename T>
inline RecDataT<align>::Dynamic::operator const T & () const {
	static const natural reqSize = (sizeof(T) + align - 1)/align;
	if (owner.length() < reqSize)
		throw ErrorMessageException(THISLOCATION,ConstStrA("Unable to load record, size mistmatch: ") + ConstStrA(typeid(T).name()));
	return reinterpret_cast<const T &>(*owner.data());

}

extern const char *str_rescanErrorMsg;
typedef GenException2<str_rescanErrorMsg, EventLog::FileOffset, natural> RescanErrorException;
extern const char *str_invalidOpcodeMsg;
typedef GenException1<str_invalidOpcodeMsg, natural> InvalidOpcodeException;


}
}


#endif /* _LIGHTSPEED_UTILS_EVENTDB_ */

