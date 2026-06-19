// Fill out your copyright notice in the Description page of Project Settings.


#include "GameTurn/GameTurnHandler.h"
#include "GameEvent/GameEventTaskManager.h"


UGameTurnHandler* UGameTurnHandler::Create()
{
	UGameTurnHandler* Obj = NewObject<UGameTurnHandler>();
	if (!Obj) { return nullptr; }
	Obj->GameEventTurnStart = NewObject<UGameEventTaskManager>();
	Obj->GameEventTurnEnd = NewObject<UGameEventTaskManager>();
	return Obj;
}

void UGameTurnHandler::Initialize()
{
	if (!GameEventTurnStart)
	{
		GameEventTurnStart = NewObject<UGameEventTaskManager>();
		GameEventTurnStart->OnManagerBegin.AddUniqueDynamic(this, &ThisClass::OnTurnStartPreAsync);
		GameEventTurnStart->OnManagerComplete.AddUniqueDynamic(this, &ThisClass::OnTurnStartPostAsync);
	}
	if (!GameEventTurnEnd)
	{
		GameEventTurnEnd = NewObject<UGameEventTaskManager>();
		GameEventTurnEnd->OnManagerBegin.AddUniqueDynamic(this, &ThisClass::OnTurnEndPreAsync);
		GameEventTurnEnd->OnManagerComplete.AddUniqueDynamic(this, &ThisClass::OnTurnEndPostAsync);
	}
}

void UGameTurnHandler::RegisterAsyncTaskTurnStart(UGameEventTask_Async* AsyncTask, const int32 Phase) const
{
	GameEventTurnStart->RegisterAsyncTask(AsyncTask, Phase);
}

void UGameTurnHandler::RegisterAsyncTaskTurnEnd(UGameEventTask_Async* AsyncTask, const int32 Phase) const
{
	GameEventTurnEnd->RegisterAsyncTask(AsyncTask, Phase);
}

void UGameTurnHandler::OnTurnStartPreAsync()
{
	if (TurnStartPreAsync.IsBound()) { TurnStartPreAsync.Broadcast(FactionTagActive); }
}

void UGameTurnHandler::OnTurnStartPostAsync()
{
	if (TurnStartPostAsync.IsBound()) { TurnStartPostAsync.Broadcast(FactionTagActive); }
}

void UGameTurnHandler::OnTurnEndPreAsync()
{
	if (TurnEndPreAsync.IsBound()) { TurnEndPreAsync.Broadcast(FactionTagActive); }
}

void UGameTurnHandler::OnTurnEndPostAsync()
{
	if (TurnEndPostAsync.IsBound()) { TurnEndPostAsync.Broadcast(FactionTagActive); }
}
