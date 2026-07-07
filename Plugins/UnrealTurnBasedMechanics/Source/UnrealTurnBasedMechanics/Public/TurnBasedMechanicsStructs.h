// TurnBasedTypes.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TurnBasedMechanicsEnums.h"
#include "TurnBasedMechanicsStructs.generated.h"

class AGridTileBase;


// Phase of the turn state machine
UENUM(BlueprintType)
enum class ETurnPhase : uint8
{
    WaitingForParticipants  UMETA(DisplayName = "Waiting For Participants"),
    WaitingForReady         UMETA(DisplayName = "Waiting For Ready"),
    TurnStart               UMETA(DisplayName = "Turn Start"),
    TurnActive              UMETA(DisplayName = "Turn Active"),
    TurnEnd                 UMETA(DisplayName = "Turn End"),
    TurnResolution          UMETA(DisplayName = "Turn Resolution"),
    TurnTimeout             UMETA(DisplayName = "Turn Timeout"),
    TurnSkipped             UMETA(DisplayName = "Turn Skipped"),
    Paused                  UMETA(DisplayName = "Paused"),
    GameOver                UMETA(DisplayName = "Game Over")
};

// Why a turn ended — passed to listeners for context
UENUM(BlueprintType)
enum class ETurnEndReason : uint8
{
    ParticipantEnded    UMETA(DisplayName = "Participant Ended Turn"),
    TimedOut            UMETA(DisplayName = "Timed Out"),
    Skipped             UMETA(DisplayName = "Skipped"),
    Forfeited           UMETA(DisplayName = "Forfeited"),
    GameOver            UMETA(DisplayName = "Game Over")
};

// Whether a participant is human or AI
UENUM(BlueprintType)
enum class EParticipantType : uint8
{
    Human     UMETA(DisplayName = "Human"),
    AI        UMETA(DisplayName = "AI"),
    Observer  UMETA(DisplayName = "Observer")
};

// Snapshot of one participant's state — replicated in TArray on GameState
USTRUCT(BlueprintType)
struct FTurnParticipantInfo
{
    GENERATED_BODY()

    // The controller owning this participant slot
    UPROPERTY(BlueprintReadOnly)
    AController* Controller = nullptr;

    // Human or AI — AI participants auto-confirm ready
    UPROPERTY(BlueprintReadOnly)
    EParticipantType ParticipantType = EParticipantType::Human;

    // Unique slot index — stable across the game
    UPROPERTY(BlueprintReadOnly)
    int32 SlotIndex = -1;

    // Display name for UI
    UPROPERTY(BlueprintReadOnly)
    FString DisplayName = TEXT("Unknown");

    // How many turns this participant has missed
    UPROPERTY(BlueprintReadOnly)
    int32 TurnsMissed = 0;

    // Whether this participant has forfeited
    UPROPERTY(BlueprintReadOnly)
    bool bForfeited = false;

    // Whether this participant has confirmed ready
    UPROPERTY(BlueprintReadOnly)
    bool bReady = false;

    // Whether this participant is currently connected
    UPROPERTY(BlueprintReadOnly)
    bool bConnected = true;

    bool IsActiveParticipant() const
    {
        return !bForfeited && bConnected;
    }
};

// Notification payload sent to participants on turn events
USTRUCT(BlueprintType)
struct FTurnNotification
{
    GENERATED_BODY()

    // The participant this notification concerns
    UPROPERTY(BlueprintReadOnly)
    FTurnParticipantInfo ParticipantInfo;

    // The current phase when this notification was sent
    UPROPERTY(BlueprintReadOnly)
    ETurnPhase Phase = ETurnPhase::WaitingForParticipants;

    // Why the turn ended — only relevant for end/timeout/skip notifications
    UPROPERTY(BlueprintReadOnly)
    ETurnEndReason EndReason = ETurnEndReason::ParticipantEnded;

    // Turn number — how many turns have elapsed total
    UPROPERTY(BlueprintReadOnly)
    int32 TurnNumber = 0;

    // Duration of this turn in seconds — clients start local timer from this
    UPROPERTY(BlueprintReadOnly)
    float TurnDuration = 0.f;
};

// Lightweight log entry per action — debug and replay
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

// Modifier written by one participant, read by another
// Lives in ConnectItBlackboardSubsystem keyed by slot index
USTRUCT(BlueprintType)
struct FTurnModifier
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    ETurnModifierType ModifierType = ETurnModifierType::ForcedMove;

    // Target tile — used for ForcedMove type modifiers
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<AGridTileBase> TargetTile = nullptr;

    // Tag identifying which shard or power created this modifier
    UPROPERTY(BlueprintReadOnly)
    FGameplayTag SourceTag;

    // Which turn this modifier was applied — for logging
    UPROPERTY(BlueprintReadOnly)
    int32 AppliedOnTurn = 0;

    // Optional custom data — for Custom modifier type
    UPROPERTY(BlueprintReadOnly)
    FGameplayTagContainer CustomTags;

    bool IsValid() const
    {
        return ModifierType != ETurnModifierType::Custom
            || !CustomTags.IsEmpty();
    }
};