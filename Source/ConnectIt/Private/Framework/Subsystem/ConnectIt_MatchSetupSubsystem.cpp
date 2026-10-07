// Fill out your copyright notice in the Description page of Project Settings.

#include "Framework/Subsystem/ConnectIt_MatchSetupSubsystem.h"
#include "AI/ConnectIt_AIProfile.h"
#include "Framework/Data/ConnectIt_LevelCatalog.h"
#include "Framework/Data/ConnectIt_LevelConfigSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"


bool UConnectIt_MatchSetupSubsystem::StartMatch(
    const FConnectItMatchSettings& InSettings, const FString& Options)
{
    if (InSettings.Level.IsNull())
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_MatchSetupSubsystem: StartMatch -- no level chosen"));
        return false;
    }

    SetMatchSettings(InSettings);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_MatchSetupSubsystem: starting '%s' (AI profile '%s', "
             "target score %.0f)"),
        *InSettings.Level.GetAssetName(), *GetNameSafe(InSettings.AIProfile),
        InSettings.TargetScore);

    UGameplayStatics::OpenLevelBySoftObjectPtr(GetGameInstance(), InSettings.Level, true, Options);
    return true;
}

void UConnectIt_MatchSetupSubsystem::SetMatchSettings(const FConnectItMatchSettings& InSettings)
{
    Settings = InSettings;
    bHasSettings = true;
}

void UConnectIt_MatchSetupSubsystem::ClearMatchSettings()
{
    Settings = FConnectItMatchSettings();
    bHasSettings = false;
}

UConnectIt_LevelCatalog* UConnectIt_MatchSetupSubsystem::GetLevelCatalog() const
{
    const UConnectIt_LevelConfigSettings* ProjectSettings = GetDefault<UConnectIt_LevelConfigSettings>();
    return IsValid(ProjectSettings) ? ProjectSettings->LevelCatalog.LoadSynchronous() : nullptr;
}

bool UConnectIt_MatchSetupSubsystem::GetSettingsForCurrentLevel(
    const UObject* WorldContextObject, FConnectItMatchSettings& OutSettings)
{
    const UWorld* World = IsValid(WorldContextObject) ? WorldContextObject->GetWorld() : nullptr;
    const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    const UConnectIt_MatchSetupSubsystem* Subsystem =
        GameInstance ? GameInstance->GetSubsystem<UConnectIt_MatchSetupSubsystem>() : nullptr;

    if (!Subsystem || !Subsystem->bHasSettings) return false;

    // Only for the level they were chosen for (PIE prefixes are stripped)
    const FString CurrentLevel = UGameplayStatics::GetCurrentLevelName(WorldContextObject, /*bRemovePrefixString=*/true);
    if (Subsystem->Settings.Level.GetAssetName() != CurrentLevel) return false;

    OutSettings = Subsystem->Settings;
    return true;
}
