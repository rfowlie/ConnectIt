// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Board/ConnectIt_BoardManager.h"
#include "ConnectIt_BoardManagerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBoardManagerReady, AConnectIt_BoardManager*, BoardManager);

// Per-world cache for the single AConnectIt_BoardManager in the level.
// Avoids repeated TActorIterator world scans from hot-path Blueprint call
// sites (see UConnectIt_GameUtilityLibrary::GetBoardManager). Populated by
// AConnectIt_BoardManager::BeginPlay on both server and client.
//
// Also relays OnShiftApplied -- the one board event that still fires as a
// direct delegate (its tag-based equivalent, on UGameEventTaskSubsystem,
// is deferred; see ConnectIt_BoardManager.h). Every other board event
// (piece placed, line scored, player win) is a UGameEventTaskSubsystem tag
// now -- this subsystem does not relay those, bind to the tag subsystem
// directly instead (see Workflows/GameEventSubsystem_Workflow.txt).
UCLASS()
class CONNECTIT_API UConnectIt_BoardManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	// Sets the cached board manager, rebinds the event relay to it, and
	// broadcasts OnBoardManagerReady. Safe to call more than once -- last
	// caller wins (e.g. PIE re-entry); any previously bound board manager
	// is unbound first.
	void RegisterBoardManager(AConnectIt_BoardManager* InBoardManager);

	AConnectIt_BoardManager* GetCachedBoardManager() const { return CachedBoardManager; }

	// Blueprint-exposed wrapper around GetCachedBoardManager -- returns null
	// if the board manager hasn't registered with this subsystem yet, with
	// no logging (pure accessor). Blueprint logic can call this first and
	// only fall back to binding OnBoardManagerReady if it comes back null.
	UFUNCTION(BlueprintPure, Category = "ConnectIt|Board", DisplayName = "Get Board Manager")
	AConnectIt_BoardManager* GetBoardManager() const { return GetCachedBoardManager(); }

	// Fired when the cached reference becomes valid -- for listeners that
	// need to call functions on the board manager itself (not just react
	// to its events) and aren't already gated on a board-state-ready
	// signal, instead of polling GetBoardManager from Tick/Construct.
	UPROPERTY(BlueprintAssignable, Category = "ConnectIt|Board")
	FOnBoardManagerReady OnBoardManagerReady;

	// --- Board Event Relay ---
	// Mirrors AConnectIt_BoardManager's OnShiftApplied -- the one board
	// event still fired as a direct delegate (deferred tag migration, see
	// ConnectIt_BoardManager.h). Bind here instead of on the board manager
	// directly -- this subsystem always exists, so there is no need to
	// check for or wait on the board manager's existence before binding.

	UPROPERTY(BlueprintAssignable, Category = "ConnectIt|Board")
	FOnShiftApplied OnShiftApplied;

private:

	UPROPERTY()
	TObjectPtr<AConnectIt_BoardManager> CachedBoardManager = nullptr;

	void BindToBoardManager(AConnectIt_BoardManager* InBoardManager);
	void UnbindFromBoardManager();

	UFUNCTION()
	void HandleShiftApplied(const FShiftOperation& Operation, const FShiftResult& Result);
};
