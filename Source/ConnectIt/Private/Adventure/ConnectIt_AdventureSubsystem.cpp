// Fill out your copyright notice in the Description page of Project Settings.

#include "Adventure/ConnectIt_AdventureSubsystem.h"
#include "Adventure/ConnectIt_AdventureSaveGame.h"
#include "Adventure/ConnectIt_ActionShardTableDataAsset.h"
#include "Adventure/ConnectIt_MapNodeDataAsset.h"
#include "Action/ItemActionInventoryDataAsset.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "Framework/Data/ConnectIt_MatchPresetDataAsset.h"
#include "Framework/Subsystem/ConnectIt_MatchSetupSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"

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
    OnProgressChanged.Broadcast();
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
    OnProgressChanged.Broadcast();
    return true;
}

bool UConnectIt_AdventureSubsystem::StartNode(const UObject* WorldContextObject, UConnectIt_MapNodeDataAsset* Node,
    const FString& Options)
{
    if (!IsValid(Node)) return false;

    FConnectItNodeRequirementCheck Check;
    const EConnectItNodeState State = Progress.GetNodeState(*Node, true, UConnectIt_ActionShardTableDataAsset::Get(), Check);
    if (State != EConnectItNodeState::Available || Node->IsSubMap() || Node->Level.IsNull())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_AdventureSubsystem: node %s can't be started (completed, gated, a sub-map or no level)"),
            *Node->GetName());
        return false;
    }

    UConnectIt_MatchSetupSubsystem* MatchSetup = GetGameInstance()->GetSubsystem<UConnectIt_MatchSetupSubsystem>();
    if (!MatchSetup) return false;

    // The hub to come back to: the map this is called from (its saved name,
    // not a PIE copy's)
    const UWorld* World = IsValid(WorldContextObject) ? WorldContextObject->GetWorld() : nullptr;
    if (World)
    {
        const FString Package = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());
        HubMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(Package + TEXT(".") + FPackageName::GetShortName(Package)));
    }

    PlayingNode = Node;
    bPlayingNodeRecorded = false;

    FConnectItMatchSettings Settings;
    Settings.Level = Node->Level;
    Settings.MatchPreset = Node->MatchPreset.LoadSynchronous();
    return MatchSetup->StartMatch(Settings, Options);
}

UConnectIt_MapNodeDataAsset* UConnectIt_AdventureSubsystem::GetPlayingNode(const UObject* WorldContextObject) const
{
    // Only in the node's own level (the match settings say so)
    FConnectItMatchSettings Settings;
    if (!IsValid(PlayingNode)
        || !UConnectIt_MatchSetupSubsystem::GetSettingsForCurrentLevel(WorldContextObject, Settings)
        || Settings.Level != PlayingNode->Level)
    {
        return nullptr;
    }
    return PlayingNode;
}

bool UConnectIt_AdventureSubsystem::SeedPlayerState(ATurnBasedPlayerState* PlayerState, UActionLoadoutDataAsset* Loadout) const
{
    const UConnectIt_MapNodeDataAsset* Node = IsValid(PlayerState) ? GetPlayingNode(PlayerState) : nullptr;
    if (!Node) return false;

    const UConnectIt_ActionShardTableDataAsset* Table = UConnectIt_ActionShardTableDataAsset::Get();
    if (Node->ActionDesign == EConnectItActionDesign::Open)
    {
        PlayerState->InitialiseActionState(Loadout, Progress.BuildOpenPermanentActions(Loadout, Table));
    }
    else
    {
        PlayerState->InitialiseActionState(Loadout);
    }
    PlayerState->InitialiseInventory(Progress.MakeInventory(Table, UItemActionInventoryDataAsset::Get()));
    PlayerState->SetItemBudget(Node->ItemBudget);

    UE_LOG(LogTemp, Log, TEXT("ConnectIt_AdventureSubsystem: %s set up for node %s (%s)"),
        *PlayerState->GetPlayerName(), *Node->GetName(),
        Node->ActionDesign == EConnectItActionDesign::Open ? TEXT("open") : TEXT("closed"));
    return true;
}

void UConnectIt_AdventureSubsystem::RecordPlayingNodeResult(const UObject* WorldContextObject, bool bWon,
    const TArray<FItemActionEntry>& Inventory)
{
    const UConnectIt_MapNodeDataAsset* Node = GetPlayingNode(WorldContextObject);
    if (!Node || bPlayingNodeRecorded) return;

    bPlayingNodeRecorded = true;
    RecordResult(Node, bWon, Inventory);
}

bool UConnectIt_AdventureSubsystem::ReturnToHub(const UObject* WorldContextObject)
{
    if (HubMap.IsNull()) return false;

    PlayingNode = nullptr;
    if (UConnectIt_MatchSetupSubsystem* MatchSetup = GetGameInstance()->GetSubsystem<UConnectIt_MatchSetupSubsystem>())
    {
        MatchSetup->ClearMatchSettings();
    }
    UGameplayStatics::OpenLevelBySoftObjectPtr(WorldContextObject, HubMap);
    return true;
}

bool UConnectIt_AdventureSubsystem::RecordResult(const UConnectIt_MapNodeDataAsset* Node, bool bWon,
    const TArray<FItemActionEntry>& Inventory)
{
    if (!IsValid(Node)) return false;
    const bool bRewarded = Progress.RecordResult(*Node, bWon, Inventory, UItemActionInventoryDataAsset::Get());
    SaveProgress();
    OnProgressChanged.Broadcast();
    return bRewarded;
}
