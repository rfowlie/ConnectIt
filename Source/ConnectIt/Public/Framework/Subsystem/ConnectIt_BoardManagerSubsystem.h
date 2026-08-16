// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ConnectIt_BoardManagerSubsystem.generated.h"

class AConnectIt_BoardManager;
class UConnectIt_BoardStateComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBoardManagerReady, AConnectIt_BoardManager*, BoardManager);

// Per-world cache for the single AConnectIt_BoardManager in the level.
// Avoids repeated TActorIterator world scans from hot-path Blueprint call
// sites (see UConnectIt_GameUtilityLibrary::GetBoardManager). Populated by
// AConnectIt_BoardManager::BeginPlay on both server and client.
//
// Does not relay any board events -- piece placed, line scored, player win,
// and shift are all UGameEventTaskSubsystem tags; bind to the tag subsystem
// directly instead (see Workflows/GameEventSubsystem_Workflow.txt).
UCLASS()
class CONNECTIT_API UConnectIt_BoardManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	// Sets the cached board manager and broadcasts OnBoardManagerReady. Safe
	// to call more than once -- last caller wins (e.g. PIE re-entry).
	void RegisterBoardManager(AConnectIt_BoardManager* InBoardManager);
	
	// Blueprint-exposed wrapper around GetCachedBoardManager -- returns null
	// if the board manager hasn't registered with this subsystem yet, with
	// no logging (pure accessor). Blueprint logic can call this first and
	// only fall back to binding OnBoardManagerReady if it comes back null.
	UFUNCTION(BlueprintPure, Category = "ConnectIt|Board", DisplayName = "Get Board Manager")
	AConnectIt_BoardManager* GetBoardManager() const { return CachedBoardManager; }

	UFUNCTION(BlueprintPure, Category = "ConnectIt|Board", DisplayName = "Get Board Manager")
	UConnectIt_BoardStateComponent* GetBoardState() const { return CachedBoardState; }

	// --- Delegates ---
	// Fired when the cached reference becomes valid -- for listeners that
	// need to call functions on the board manager itself (not just react
	// to its events) and aren't already gated on a board-state-ready
	// signal, instead of polling GetBoardManager from Tick/Construct.
	UPROPERTY(BlueprintAssignable, Category = "ConnectIt|Board")
	FOnBoardManagerReady OnBoardManagerReady;
	
private:

	UPROPERTY()
	TObjectPtr<AConnectIt_BoardManager> CachedBoardManager = nullptr;

	UPROPERTY()
	TObjectPtr<UConnectIt_BoardStateComponent> CachedBoardState = nullptr;
};
