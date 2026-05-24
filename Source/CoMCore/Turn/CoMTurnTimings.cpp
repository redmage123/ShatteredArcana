// Copyright Mythforge Studios. All Rights Reserved.

#include "CoMTurnTimings.h"

namespace CoMTurnTimings
{
	bool bEnabled = false;
	TMap<FName, double> Elapsed;
	TMap<FName, int32>  CallCounts;

	void Reset()
	{
		Elapsed.Empty();
		CallCounts.Empty();
	}

	void Add(FName Phase, double Seconds)
	{
		Elapsed.FindOrAdd(Phase)    += Seconds;
		CallCounts.FindOrAdd(Phase) += 1;
	}
}
