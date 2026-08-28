// TurnBasedTypes.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GridMechanicsBaseStructs.h"
#include "StructUtils/InstancedStruct.h"
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

    // How many turns this participant has personally had, including
    // whichever one is currently active. Incremented in
    // UTurnBasedParticipantManagerComponent::StartTurn alongside the
    // match-wide TurnNumber counter -- use this, not TurnNumber, to answer
    // "is this my Nth turn" (TurnNumber increments on every participant's
    // turn, not just this one's).
    UPROPERTY(BlueprintReadOnly)
    int32 TurnsTaken = 0;

    // Per-player mutable state lives on ATurnBasedPlayerState
    // Read via cast when needed
    bool IsActiveParticipant() const;

    // Convenience -- reads from PlayerState
    FString GetDisplayName() const;

};

// Passed to a participant's own ActionsComponent whenever any turn begins --
// their own (NotifyTurnStarted) or someone else's (NotifyOpponentTurnStarted).
// Same shape for both so a project override can react identically regardless
// of whose turn it is. Deliberately not a full snapshot of every participant
// -- Participants/TurnNumber already replicate independently on
// UTurnBasedParticipantManagerComponent, reachable directly wherever needed.
USTRUCT(BlueprintType)
struct FTurnStartContext
{
    GENERATED_BODY()

    // Global, match-wide count -- increments on every participant's turn
    UPROPERTY(BlueprintReadOnly)
    int32 TurnNumber = 0;

    // Whose turn this is -- ActiveParticipant.TurnsTaken is how many turns
    // THAT participant has personally had, including this one
    UPROPERTY(BlueprintReadOnly)
    FTurnParticipantInfo ActiveParticipant;
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
// Generic envelope only -- never needs to grow for a new project action
// type. Concrete per-action data (grid positions, shift parameters, etc.)
// lives in a project-defined USTRUCT wrapped in Payload instead of being
// added here directly.
USTRUCT(BlueprintType)
struct FTurnActionRequest
{
    GENERATED_BODY()

    // Identifies what kind of change is requested
    // e.g. ConnectIt.Game.State.PlacePiece, ConnectIt.Game.State.Shift
    UPROPERTY(BlueprintReadWrite)
    FGameplayTag RequestType;

    // Faction making the request — from SlotIndex on participant component
    UPROPERTY(BlueprintReadWrite)
    int32 FactionID = -1;

    // Flexible additional data — shard type, etc.
    UPROPERTY(BlueprintReadWrite)
    FGameplayTagContainer AdditionalData;

    // Project-specific payload -- the plugin has no idea what concrete
    // struct this holds. Each project defines its own USTRUCTs (e.g.
    // FConnectItRequestPlacePiece) and wraps one here per request type.
    UPROPERTY(BlueprintReadWrite)
    FInstancedStruct Payload;

    bool IsValid() const
    {
        return RequestType.IsValid() && FactionID >= 0 && Payload.IsValid();
    }

    // Used by UTurnBasedActionsComponent's awaiting-confirmation machinery
    // to match an incoming server outcome against the request it's waiting
    // on -- see NotifyBoardChangeOutcome.
    bool operator==(const FTurnActionRequest& Other) const
    {
        return RequestType == Other.RequestType
            && FactionID == Other.FactionID
            && AdditionalData == Other.AdditionalData
            && Payload == Other.Payload;
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