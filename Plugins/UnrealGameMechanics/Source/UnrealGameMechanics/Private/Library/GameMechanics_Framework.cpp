// Fill out your copyright notice in the Description page of Project Settings.

#include "Library/GameMechanics_Framework.h"


UGameInstance* UGameMechanics_Framework::GetGameInstance(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return World ? World->GetGameInstance() : nullptr;
}

AGameModeBase* UGameMechanics_Framework::GetGameMode(UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return World ? World->GetAuthGameMode() : nullptr;
}
