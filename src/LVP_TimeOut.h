#ifndef LVP_GLOBAL_H
	#include "LVP_Global.h"
#endif

#ifndef LVP_TIMEOUT_H
#define LVP_TIMEOUT_H

namespace LibVoyaPlayer
{
	namespace System
	{
		class LVP_TimeOut
		{
		public:
			LVP_TimeOut(uint32_t timeOut = 5000U);

		private:
			bool     started;
			uint32_t startTime;
			uint32_t timeOut;

		public:
			static int InterruptCallback(void* data);
			bool       isTimedOut();
			void       start();
			void       stop();

		};
	}
}

#endif
