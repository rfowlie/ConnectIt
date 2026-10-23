// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_PlayerDirector.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Misc/DataValidation.h"


ATurnBasedPlayerState* UConnectIt_PlayerDirector::GetDirectedPlayerState() const
{
    return DirectedPlayerState.Get();
}

void UConnectIt_PlayerDirector::SetDirectedPlayerState(ATurnBasedPlayerState* InPlayerState)
{
    DirectedPlayerState = InPlayerState;
}

int32 UConnectIt_PlayerDirector::GetFaction() const
{
    const ATurnBasedPlayerState* PS = DirectedPlayerState.Get();
    return IsValid(PS) ? PS->GetSlotIndex() : INDEX_NONE;
}

void UConnectIt_PlayerDirector::SetPlayerLoadout(UActionLoadoutDataAsset* Loadout, EConnectItChangeTiming Timing)
{
    AConnectIt_GameMode* GameMode = GetGameMode();
    ATurnBasedPlayerState* PS = DirectedPlayerState.Get();
    if (!IsValid(GameMode) || !IsValid(PS))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_PlayerDirector: %s has no GameMode or player -- loadout not changed"), *GetName());
        return;
    }
    GameMode->SetLoadoutForPlayer(PS, Loadout, Timing);
}

UActionLoadoutDataAsset* UConnectIt_PlayerDirector::GetPlayerLoadout() const
{
    const ATurnBasedPlayerState* PS = DirectedPlayerState.Get();
    return IsValid(PS) ? PS->GetLoadout() : nullptr;
}

#if WITH_EDITOR
EDataValidationResult UConnectIt_PlayerDirector::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (!IsValid(StartingLoadout))
    {
        Context.AddError(FText::FromString(TEXT(
            "ConnectIt_PlayerDirector: No Starting Loadout set -- the player would have no actions.")));
        Result = EDataValidationResult::Invalid;
    }
    return Result;
}
#endif
