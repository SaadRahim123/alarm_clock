#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include "main.h"

////////////////// Defines ////////////////////
#define NTP_TIMESTAMP_DELTA 2208988800ull // Seconds between Jan 1, 1900 and Jan 1, 1970
//////////////////////////////////////////////

void InitializeandSyncSntp(void);
time_t GetSntpTimeBlocking(void);
time_t GetSntpTimeBlockingDNS(void);
void ForceCustomDNS();
uint64_t GetGlobalEpochTime(void);
#endif