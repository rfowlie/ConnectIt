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

// A board-change request on its way from a UTurnBasedAction, through
// UTurnBasedActionsComponent, to the server. It says two things only: WHICH
// ACTION is asking (so the server can check and spend that action's uses) and
// WHAT is being asked (the payload).
//
// The plugin has no idea what the payload is -- each project defines its own
// USTRUCTs for it (e.g. ConnectIt's FConnectItBoardOperation_PlacePiece) and
// its own server code to carry it out. An action hands over just the payload
// (UTurnBasedAction::RequestBoardChange); the action tag is stamped for it.
//
// Deliberately absent: who is asking. A request arrives from a client, so the
// server must take the requester from the connection it arrived on (the
// sending controller), never from anything inside the request.
USTRUCT(BlueprintType)
struct FTurnActionRequest
{
    GENERATED_BODY()

    // The action that sent this request. Stamped by
    // UTurnBasedAction::RequestBoardChange so an action can't forget it; the
    // server uses it to find (and spend) that action's runtime state on the
    // requester's PlayerState. Client-supplied, so the server must still
    // check this action is in the requester's loadout and may send this
    // payload.
    UPROPERTY(BlueprintReadWrite)
    FGameplayTag ActionTag;

    // What is being asked -- a project-defined struct
    UPROPERTY(BlueprintReadWrite)
    FInstancedStruct Payload;

    bool IsValid() const
    {
        return Payload.IsValid();
    }

    // Used by UTurnBasedActionsComponent's awaiting-confirmation machinery
    // to match an incoming server outcome against the request it's waiting
    // on -- see NotifyBoardChangeOutcome.
    bool operator==(const FTurnActionRequest& Other) const
    {
        return ActionTag == Other.ActionTag
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