// Fill out your copyright notice in the Description page of Project Settings.

#include "Adventure/ConnectIt_AdventureSubsystem.h"
#include "Adventure/ConnectIt_AdventureSaveGame.h"
#include "Adventure/ConnectIt_ActionShardTableDataAsset.h"
#include "Adventure/ConnectIt_MapNodeDataAsset.h"
#include "Action/ItemActionInventoryDataAsset.h"
#include "Kismet/GameplayStatics.h"

const FString UConnectIt_AdventureSubsystem::SaveSlotName = TEXT("Adventure");

void UConnectIt_AdventureSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadProgress();
}

bool UConnectIt_AdventureSubsystem::LoadProgress()
{
    Progress = FConnectItAdventureProgress();
    if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0)) return true;

    const UConnectIt_AdventureSaveGame* Save =
        Cast<UConnectIt_AdventureSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
    if (!Save)
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectIt_AdventureSubsystem: the '%s' save couldn't be read -- starting fresh"),
            *SaveSlotName);
        return false;
    }
    Progress = Save->Progress;
    return true;
}

bool UConnectIt_AdventureSubsystem::SaveProgress()
{
    UConnectIt_AdventureSaveGame* Save = Cast<UConnectIt_AdventureSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UConnectIt_AdventureSaveGame::StaticClass()));
    if (!Save) return false;

    Save->Progress = Progress;
    const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, 0);
    if (!bSaved)
    {
        UE_LOG(LogTemp, Error, TEXT("ConnectIt_AdventureSubsystem: couldn't write the '%s' save"), *SaveSlotName);
    }
    return bSaved;
}

EConnectItNodeState UConnectIt_AdventureSubsystem::GetNodeState(const UConnectIt_MapNodeDataAsset* Node, bool bRevealed,
    FConnectItNodeRequirementCheck& OutCheck) const
{
    OutCheck = FConnectItNodeRequirementCheck();
    if (!IsValid(Node)) return EConnectItNodeState::Hidden;
    return Progress.GetNodeState(*Node, bRevealed, UConnectIt_ActionShardTableDataAsset::Get(), OutCheck);
}

int32 UConnectIt_AdventureSubsystem::GetPermanentUses(FGameplayTag Action) const
{
    return Progress.GetPermanentUses(Action, UConnectIt_ActionShardTableDataAsset::Get());
}

bool UConnectIt_AdventureSubsystem::CanBreakSeal(const UConnectIt_MapNodeDataAsset* Node, FString& OutWhyNot) const
{
    OutWhyNot.Reset();
    if (!IsValid(Node))
    {
        OutWhyNot = TEXT("no node");
        return false;
    }
    return Progress.CanBreakSeal(*Node, &OutWhyNot);
}

bool UConnectIt_AdventureSubsystem::BreakSeal(const UConnectIt_MapNodeDataAsset* Node)
{
    if (!IsValid(Node) || !Progress.BreakSeal(*Node)) return false;
    SaveProgress();
    return true;
}

bool UConnectIt_AdventureSubsystem::RecordResult(const UConnectIt_MapNodeDataAsset* Node, bool bWon,
    const TArray<FItemActionEntry>& Inventory)
{
    if (!IsValid(Node)) return false;
    const bool bRewarded = Progress.RecordResult(*Node, bWon, Inventory, UItemActionInventoryDataAsset::Get());
    SaveProgress();
    return bRewarded;
}
