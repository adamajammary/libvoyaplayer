#ifndef LVP_MAIN_H
	#include "main.h"
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
			LVP_TimeOut(uint64_t timeOut = 5000ULL);

		private:
			bool     started;
			uint64_t startTime;
			uint64_t timeOut;

		public:
			static int InterruptCallback(void* data);
			bool       isTimedOut() const;
			void       start();
			void       stop();
		};
	}
}

#endif
