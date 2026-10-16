// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_AIDirector.h"
#include "AI/ConnectIt_AIStrategy.h"
#include "Framework/Controller/ConnectIt_AIController.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"


AConnectIt_AIController* UConnectIt_AIDirector::GetOwningController() const
{
    return Cast<AConnectIt_AIController>(GetOuter());
}

UWorld* UConnectIt_AIDirector::GetWorld() const
{
    if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)) return nullptr;
    const UObject* Outer = GetOuter();
    return Outer ? Outer->GetWorld() : nullptr;
}

void UConnectIt_AIDirector::SetAILoadout(UActionLoadoutDataAsset* Loadout)
{
    if (AConnectIt_AIController* Controller = GetOwningController())
    {
        Controller->ChangeLoadout(Loadout);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ConnectIt_AIDirector: %s has no AI controller -- loadout not changed"), *GetName());
    }
}

void UConnectIt_AIDirector::SetAIStrategy(UConnectIt_AIStrategy* StrategyTemplate)
{
    if (AConnectIt_AIController* Controller = GetOwningController())
    {
        Controller->ChangeStrategy(StrategyTemplate);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ConnectIt_AIDirector: %s has no AI controller -- strategy not changed"), *GetName());
    }
}

UActionLoadoutDataAsset* UConnectIt_AIDirector::GetAILoadout() const
{
    const AConnectIt_AIController* Controller = GetOwningController();
    const ATurnBasedPlayerState* PS = Controller ? Controller->GetPlayerState<ATurnBasedPlayerState>() : nullptr;
    return PS ? PS->GetLoadout() : nullptr;
}
