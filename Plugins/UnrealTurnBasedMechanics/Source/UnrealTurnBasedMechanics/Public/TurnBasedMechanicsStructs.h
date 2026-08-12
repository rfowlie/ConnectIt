// TurnBasedTypes.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GridMechanicsBaseStructs.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedMechanicsStructs.generated.h"

class ATurnBasedPlayerState;
class AGridTileBase;


// Snapshot of one participant's state — replicated in TArray on GameState
USTRUCT(BlueprintType)
struct UNREALTURNBASEDMECHANICS_API FTurnParticipantInfo
{
    GENERATED_BODY()
    
    // Replicates properly to all clients unlike AController
    // Valid for both human and AI participants
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<ATurnBasedPlayerState> PlayerState = nullptr;

    UPROPERTY(BlueprintReadOnly)
    EParticipantType ParticipantType = EParticipantType::Human;

    // Stable for lifetime of match -- doubles as FactionID
    UPROPERTY(BlueprintReadOnly)
    int32 SlotIndex = -1;

    UPROPERTY(BlueprintReadOnly)
    bool bConnected = true;

    // Per-player mutable state lives on ATurnBasedPlayerState
    // Read via cast when needed
    bool IsActiveParticipant() const;

    // Convenience -- reads from PlayerState
    FString GetDisplayName() const;
    
};

// Notification payload sent to participants on turn events
USTRUCT(BlueprintType)
struct FTurnNotification
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FTurnParticipantInfo ParticipantInfo;

    UPROPERTY(BlueprintReadOnly)
    ETurnPhase Phase = ETurnPhase::WaitingForParticipants;

    UPROPERTY(BlueprintReadOnly)
    ETurnEndReason EndReason = ETurnEndReason::ParticipantEnded;

    UPROPERTY(BlueprintReadOnly)
    int32 TurnNumber = 0;

    // Clients start their own local countdown from this value
    // Never tick-replicated — set once per turn start
    UPROPERTY(BlueprintReadOnly)
    float TurnDuration = 0.f;
};

// Lightweight log entry per action — debug and replay foundation
USTRUCT(BlueprintType)
struct FTurnBasedActionRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FGameplayTag ActionTag;

    UPROPERTY(BlueprintReadOnly)
    ETurnBasedActionState OutcomeState = ETurnBasedActionState::Completed;

    UPROPERTY(BlueprintReadOnly)
    int32 TurnNumber = 0;

    UPROPERTY(BlueprintReadOnly)
    FString DebugNote;

    UPROPERTY(BlueprintReadOnly)
    float Timestamp = 0.f;
};

// Request sent from UTurnBasedAction to UTurnBasedActionComponent
// Routed to server then to project-specific board manager
// Action tag identifies the type of change requested
USTRUCT(BlueprintType)
struct FTurnActionRequest
{
    GENERATED_BODY()

    // Identifies what kind of change is requested
    // e.g. ConnectIt.Board.PlacePiece, ConnectIt.Board.ShiftRow
    UPROPERTY(BlueprintReadWrite)
    FGameplayTag RequestType;

    // Grid positions relevant to this request
    UPROPERTY(BlueprintReadWrite)
    TArray<FGridPosition> Positions;

    // Faction making the request — from SlotIndex on participant component
    UPROPERTY(BlueprintReadWrite)
    int32 FactionID = -1;

    // Flexible additional data — shard type, shift direction, etc.
    UPROPERTY(BlueprintReadWrite)
    FGameplayTagContainer AdditionalData;

    bool IsValid() const
    {
        return RequestType.IsValid() && FactionID >= 0;
    }
};

// Modifier written by one participant, read by another
// Lives in ConnectItBlackboardSubsystem keyed by slot index
USTRUCT(BlueprintType)
struct FTurnModifier
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    ETurnModifierType ModifierType = ETurnModifierType::ForcedMove;

    UPROPERTY(BlueprintReadOnly)
    AGridTileBase* TargetTile = nullptr;

    // Tag identifying which shard or power created this modifier
    UPROPERTY(BlueprintReadOnly)
    FGameplayTag SourceTag;

    UPROPERTY(BlueprintReadOnly)
    int32 AppliedOnTurn = 0;

    UPROPERTY(BlueprintReadOnly)
    FGameplayTagContainer CustomTags;

    bool IsValid() const
    {
        return ModifierType != ETurnModifierType::Custom
            || !CustomTags.IsEmpty();
    }
};