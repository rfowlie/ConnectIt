// Fill out your copyright notice in the Description page of Project Settings.


#include "GameEvent/GameEventTask_Async.h"


void UGameEventTask_Async::CallOnComplete()
{
	if (OnComplete.IsBound()) { OnComplete.Broadcast(this); }
}
