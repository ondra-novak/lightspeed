#pragma once

namespace LightSpeed {


		class IWinWaitableObject {
	public:
		///Prepares object for external waiting (inside of waiting pool) and returns waitable handle
		/**
		@param waitFor one of wait operation - waitForInput or waitForOutput. You cant specify both
		@return handle can be used for waiting. If handle is not signaled you don't need to perform
		any action. Once handle is signaled, you should perform onWaitSuccess to finish all operations
		with the handle, otherwise handle can remain signaled */
		virtual HANDLE getWaitHandle(natural waitFor) = 0;

		///Should be called when handle is signaled
		/** 
		@param waitFor operation which has been signaled
		@retval true signal accepted
		@retval false false signal, continue to wait (handle is reset)
		*/
		virtual bool onWaitSuccess(natural waitFor) = 0;
	};


}