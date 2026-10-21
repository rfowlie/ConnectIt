// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_Director.h"
#include "AI/ConnectIt_LevelDirector.h"
#include "AI/ConnectIt_PlayerDirector.h"
#include "AI/ConnectIt_AIDirector.h"
#include "Algo/StableSort.h"
#include "Engine/World.h"
#include "Framework/GameMode/ConnectIt_GameMode.h"


UWorld* UConnectIt_Director::GetWorld() const
{
    if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)) return nullptr;
    const UObject* Outer = GetOuter();
    return Outer ? Outer->GetWorld() : nullptr;
}

AConnectIt_GameMode* UConnectIt_Director::GetGameMode() const
{
    const UWorld* World = GetWorld();
    return World ? Cast<AConnectIt_GameMode>(World->GetAuthGameMode()) : nullptr;
}

UConnectIt_LevelDirector* UConnectIt_Director::GetLevelDirector() const
{
    const AConnectIt_GameMode* GameMode = GetGameMode();
    return GameMode ? GameMode->GetLevelDirector() : nullptr;
}

UConnectIt_PlayerDirector* UConnectIt_Director::GetPlayerDirector(int32 Faction) const
{
    const AConnectIt_GameMode* GameMode = GetGameMode();
    return GameMode ? GameMode->GetPlayerDirector(Faction) : nullptr;
}

UConnectIt_AIDirector* UConnectIt_Director::GetAIDirector() const
{
    const AConnectIt_GameMode* GameMode = GetGameMode();
    return GameMode ? GameMode->GetAIDirector() : nullptr;
}

TArray<UConnectIt_Director*> UConnectIt_Director::InDispatchOrder(
    UConnectIt_LevelDirector* Level,
    const TArray<UConnectIt_PlayerDirector*>& Players,
    UConnectIt_AIDirector* AI)
{
    TArray<UConnectIt_Director*> Ordered;
    if (IsValid(Level)) Ordered.Add(Level);

    TArray<UConnectIt_PlayerDirector*> SortedPlayers;
    for (UConnectIt_PlayerDirector* Player : Players)
    {
        if (IsValid(Player)) SortedPlayers.Add(Player);
    }
    Algo::StableSortBy(SortedPlayers, [](const UConnectIt_PlayerDirector* Player) { return Player->GetFaction(); });
    for (UConnectIt_PlayerDirector* Player : SortedPlayers)
    {
        Ordered.Add(Player);
    }

    if (IsValid(AI)) Ordered.Add(AI);
    return Ordered;
}
