// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ConnectIt_MatchSetupSubsystem.generated.h"

class UConnectIt_AIProfile;
class UConnectIt_LevelCatalog;


// What the player chose in the main menu for the next match.
USTRUCT(BlueprintType)
struct FConnectItMatchSettings
{
    GENERATED_BODY()

    // The map to play. Settings only apply when this is the level loaded.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
    TSoftObjectPtr<UWorld> Level;

    // Opponent for a vs-AI match; unset = the level config's AIProfile
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
    TObjectPtr<UConnectIt_AIProfile> AIProfile = nullptr;

    // Score needed to win; 0 = the level's own win condition value
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match", meta = (ClampMin = 0.0))
    float TargetScore = 0.f;
};

// Carries the main menu's match choices (level, AI opponent, target score)
// into the match. Lives on the GameInstance, so it survives OpenLevel.
//
// The menu builds an FConnectItMatchSettings (options from GetLevelCatalog)
// and calls StartMatch. In the match, AConnectIt_GameMode and
// AConnectIt_AIController read it through GetSettingsForCurrentLevel, which
// only returns settings whose Level is the level actually loaded -- so stale
// choices never leak into a different map (or a PIE session started straight
// into a level, which simply uses the level's defaults).
UCLASS()
class CONNECTIT_API UConnectIt_MatchSetupSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:

    // Store Settings and open its level. Options are OpenLevel URL options
    // (e.g. "listen"). False if Settings has no level.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Match Setup")
    bool StartMatch(const FConnectItMatchSettings& InSettings, const FString& Options);

    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Match Setup")
    void SetMatchSettings(const FConnectItMatchSettings& InSettings);

    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Match Setup")
    void ClearMatchSettings();

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Match Setup")
    bool HasMatchSettings() const { return bHasSettings; }

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Match Setup")
    FConnectItMatchSettings GetMatchSettings() const { return Settings; }

    // The project's catalog of playable levels and AI opponents (loaded on
    // demand from ConnectIt_LevelConfigSettings). Null if none is set.
    UFUNCTION(BlueprintCallable, Category = "ConnectIt|Match Setup")
    UConnectIt_LevelCatalog* GetLevelCatalog() const;

    // The stored settings, if there are any AND they are for the level
    // WorldContextObject is in.
    static bool GetSettingsForCurrentLevel(
        const UObject* WorldContextObject, FConnectItMatchSettings& OutSettings);

private:

    UPROPERTY()
    FConnectItMatchSettings Settings;

    bool bHasSettings = false;
};
