#include "../types.h"
#include <utility>
#include "../tags.h"
#include "../memory/pointer.h"
namespace LightSpeed {


	class ProgressMonitor;

	///Progress reporting class
	/** Very useful class to implement progress bars and report current progress status to
	  the user interface.

	  Program can create instance of this class and sets progress range and position. The
	  state of progress is reported to the instance of ProgressMonitor which must be created
	  before long operation is started. ProgressMonitor uses thread variable to register its
	  instance and Progress class uses this variable to find and connect the ProgressMonitor.

	  Progress can be reported in two ways. Classical way is use Progress::set() function to
	  change current position of progress. This way is not recommended, because disables
	  recursive progress reporting, which allows to calculate position from nested subprocesses.

	  Better and recomended way is use function Progress::adv() before program starts to
	  work on particular task, because it allows to predict position of progress from nested subprocesses.

	  Example:
	  @code
	  Progress p(cnt);
	  for (int i = 0; i < cnt; i++) {
		p.adv(1);
		//do something long there for index i.
	  }
	  @endcode

		
	*/

	class Progress {
	public:
		
		typedef std::pair<float,float> Position;
		friend class ProgressMonitor;

		///Constructs progress instance
		/** 
		Sets default range to 100, but expects, that range will be changed by setMax() function
		*/
		Progress():next(0),monitor(0),nextValue(0),maxValue(100),curValue(0) {
			attachToMonitor();
		}

		///Constructs progress and sets range
		/**
		 * @param maxValue sets maximum for position. Progress always starts 
			at zero and this defines value for 100%. Progress cannot be higher than 100%
		 */
		 
		Progress(natural maxValue):next(0),monitor(0),nextValue(0),maxValue(maxValue),curValue(0) {
			attachToMonitor();
		}

		///Destroys progress
		/**
		 * Calls done() to finish last cycle
		 * Detaches itself from the monitor
		 */
		 
		~Progress() {
			done();
			detachMonitor();
		}

		///Changes maximum of the range
		void setMax(natural maxValue) {
			bool cb = callback && this->maxValue != maxValue;
			this->maxValue = maxValue;
			if (cb)	onChange();
		}

		///Advances progress by specified value
		/**
		 * You advances progress before program starts its work on particular task. 
		 * @param nextVal specifies offset from current position where progress will
		 * be after finishing the task.
		 * Function also calls done() to ensure, that finishing of the previous task has been reported, so
		 * you don't need to call done() explitictly
		 */
		 
		void adv(natural offset = 1) {
			done();
			nextValue += offset;
			if (nextValue > maxValue) nextValue = maxValue;
		}

		void setNext(natural pos) {
			done();
			if (pos < curValue) pos = curValue;
			else nextValue = pos;
		}

		///Explicitly reports finishing of previous task moving current position about specified offset
		/** Very often you don't need to call this function explicitly */
		void done() {
			bool cb = callback && curValue != nextValue;
			curValue = nextValue;
			if (cb) onChange();
		}

		///Sets position 
		/**
		 * @param curVal new position. Note that function also resets offset specified by adv(),
		 * so task is treat as done.
		 */
		 
		void set(natural curVal) {
			if (curVal > maxValue) curVal = maxValue;
			bool cb = callback && curValue != curVal;
			nextValue = curValue = curVal;
			if (cb) onChange();
			
		}

		///Increments position by 1
		/**
		 * Uses adv(1) to increment position                                                                     
		 */
		 
		Progress &operator ++() {
			adv(1);return *this;
		}

		///Increments position by 1
		/**
		 * Uses adv(1) to increment position                                                                     
		 */
		natural operator++(int) {
			adv(1);return curValue;
		}

		///Increments position by argument
		/**
		 * Uses adv(a) to increment position                                                                     
		 */
		Progress &operator+=(natural a) {
			adv(a);return *this;
		}

		///Calculates current position
		/**
		 * @return current position in percent normalized from 0 to 1. Position is reported
		 * as two numbers. First number is position before task started, second number is position
		 * after task will be finished.
		 */
		 
		Position getCurPos() const {
			float maxV = (float)maxValue;
			if (maxV == 0) return Position(0.0f,1.0f);
			return Position(curValue / maxV, nextValue / maxV);
		}

		///Calculates overall position 
		/**
		 */
		 
		Position getPos(natural level = 0) const {
			
			natural x = 0;
			return getPosRec(level,x);
		}


		///Sets progress state
		/**
		 * Allows to report state of current progres displayed at user interface near to progress bar
		 * @param state predefined state. User interface selects appropriate message associated with this state.
		 *   State can contain english text message which can be displayed when string table is not available
		 * 
		 */
		 
		void setState(Tag state) {
			bool cb = callback && curState != state;
			curState = state;
			curStateParam = 0;
			if (cb)	onChangeState();
		}

		void setState(Tag state, const void *param) {
			bool cb = (callback && curState != state) || param != curStateParam;
			curState = state;
			curStateParam = param;
			if (cb)	onChangeState();

		}

		Tag getState() const {return curState;}
		const void *getStateParam() const {return curStateParam;}



	protected:
		Progress *next;
		ProgressMonitor *monitor;
		bool callback;

		natural nextValue;
		natural maxValue;
		natural curValue;
		Tag curState;
		const void *curStateParam;

		void attachToMonitor();
		void detachMonitor();
		void onChange();
		void onChangeState();
		
		Position getPosRec(natural level, natural &tmp) const {

			Position k;
			if (next == 0) {
				k = Position(0.0f,1.0f);
				tmp = 0;
			}
			else {
				k = next->getPosRec(level,tmp);
			}
			if (tmp < level) {
				++tmp;
				return Position(0.0f,1.0f);
			}
			else {
				++tmp;
				Position l = getCurPos();
				return Position(l.first * (k.second - k.first) + k.first,
					l.second * (k.second - k.first) + k.first);
			}



		}


	};

	
	///Base class for progress monitor 
	/**
	 * Class processes informations send by instance of Progress class. It can 
	 * use user interface to display progress bar and state of progress.
	 *
	 * Progress monitor can work in synchronous and asynchronous mode. In 
	 * synchronous mode, object is notified every time progress is changed through
	 * function onChange(). In asynchronous mode, object must periodically
	 * call function getPos to retrieve current position. Asynchronous mode
	 * is faster, because instance of Progress class don't need to notify
	 * monitor about every change. Reading position can be implemented
	 * without locking, because reading is atomic. Locking is made
	 * during attaching and detaching nested instances of Progress to
	 * prevent accessing destroyed objects.
	 *
	 * In asynchronous mode, extending class must implement the lock, not
	 * included in this class.
	 */
	 
	class ProgressMonitor {
	public:

		///Creates monitor and registers it at current thread
		/**
		 * Registration is always made in current thread. For asynchronous
		 * access when UI is controlled in current thread and calculation
		 * starting in the new thread, you can construct object in 
		 * new thread and then pass pointer to current thread using proper
		 * synchronization
	     *
		 * if there is already registered monitor, constructor throws an exception.
		 */
		 
		ProgressMonitor();
		///Destructor - unregisters monitor
		virtual ~ProgressMonitor();
		///called to lock object, you need to implement for asynchronous access
		virtual void lock() const {}
		///called to unlock object, you need to implement this for asynchronous access
		virtual void unlock() const {}
		///called in synchronous mode when progress has been changed
		/**
		 * @param p pointer to progress which has been affected by this change
		 */		 
		virtual void onChange(const Progress *) {}
		///called in synchronous mode when state has been changed
		/**
		 * @param p pointer to progress which has been affected by this change
		 */
		virtual void onChangeState(const Progress *) {}
		///retrieves current position. Can be called in both modes
		/**
		 * @param level specifies from which progress has been retrieved. 
		 *    Level 0 is always first created Progress object, level 1 is
		 *		first nested Progress object, and so on. Function returns progress
		 *		at specified level. Specifiying 0 retrieves overall progress. Specifying
		      1 retrieves particular progress for every task processed at first level. This
			  allows to display two or more progress bars where first progress bar is
			  overall and other progressbars display progress of particular task. For
			  example while files are copied, first progress bar displays 
			  state of whole job and is retrieved with level 0. Second progress bar
			  display state of single file and is retrievd with level 1
		 */
		 
		float getPos(natural level = 0) const;
		
		typedef std::pair<Tag, const void *> State;
		State getState(const Progress *p) const;
		///returns true, when asynchronous mode is enabled
		/**
		 * Do not change mode during lifetime of this object
		 */		 
		virtual bool asyncEnabled() const {return false;}
		natural getLevel(const Progress *x);

	protected:

		Pointer<Progress> head;
		friend class Progress;

	};

	

}
