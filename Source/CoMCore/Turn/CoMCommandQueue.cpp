// Copyright Mythforge Studios 2026. All Rights Reserved.
// CoMCommandQueue.cpp — Command queue implementation for deterministic turn simulation.

#include "CoMCore/Turn/CoMCommandQueue.h"

void UCoMCommandQueue::Enqueue(const FCoMCommandPayload& Command)
{
	Commands.Add(Command);
}

bool UCoMCommandQueue::Dequeue(FCoMCommandPayload& OutCommand)
{
	if (Commands.IsEmpty())
	{
		return false;
	}
	OutCommand = Commands[0];
	Commands.RemoveAt(0);
	return true;
}

bool UCoMCommandQueue::Peek(FCoMCommandPayload& OutCommand) const
{
	if (Commands.IsEmpty())
	{
		return false;
	}
	OutCommand = Commands[0];
	return true;
}
