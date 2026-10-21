// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/ConnectIt_Director.h"
#include "ConnectIt_LevelDirector.generated.h"

class UConnectIt_MatchConditionAsset;
struct FConnectItBoardState;
struct FConnectItRuleSet;

// Whose pieces / score a level trigger's condition is checked for
UENUM(BlueprintType)
enum class EConnectItTriggerSide : uint8
{
    // The human side (every faction in a match with no AI)
    Player  UMETA(DisplayName = "Player"),
    // The AI's faction
    AI      UMETA(DisplayName = "AI"),
    // Any faction
    Either  UMETA(DisplayName = "Either"),
};

// One level trigger: a condition the level watches for, and whose side
USTRUCT(BlueprintType)
struct CONNECTIT_API FConnectItLevelTrigger
{
    GENERATED_BODY()

    // Met when all of its conditions hold (e.g. Score Threshold 10: "the
    // player has reached 10 points")
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger")
    TObjectPtr<UConnectIt_MatchConditionAsset> Condition = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger")
    EConnectItTriggerSide Side = EConnectItTriggerSide::Player;
};

// The level's own script: reacts to the match (the shared director events,
// anything it can reach through the world) and, being the level, is the one
// director that changes the board and the rules -- not a side's loadout
// (ask that side's director). One instance per match, owned by the GameMode;
// named by class on the match preset (none = no level script). Subclass it in
// Blueprint per adventure level.
//
// Triggers: after every committed board change, each armed trigger whose
// condition now holds (for its side) fires On Trigger Fired once and disarms;
// Rearm Trigger arms it again. Fired / armed state lives here. Triggers are
// the level's own business: they don't win or lose anything, and the AI's
// search doesn't see them.
//
// (Blueprint board / scoring-rule changes arrive in the next stage of the
// three-directors plan.)
UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API UConnectIt_LevelDirector : public UConnectIt_Director
{
    GENERATED_BODY()

public:

    // What the level watches for. Each fires once (until re-armed).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Triggers")
    TArray<FConnectItLevelTrigger> Triggers;

    // A trigger's condition held: Condition is the trigger's asset, Faction the
    // side it held for. The trigger is now disarmed.
    UFUNCTION(BlueprintNativeEvent, Category = "Level|Triggers")
    void OnTriggerFired(UConnectIt_MatchConditionAsset* Condition, int32 Faction);
    virtual void OnTriggerFired_Implementation(UConnectIt_MatchConditionAsset* Condition, int32 Faction) {}

    // Arms again every trigger using Condition, so it can fire again the next
    // time its condition holds after a board change
    UFUNCTION(BlueprintCallable, Category = "Level|Triggers")
    void RearmTrigger(UConnectIt_MatchConditionAsset* Condition);

    // Whether the trigger using Condition is armed (false if none uses it)
    UFUNCTION(BlueprintPure, Category = "Level|Triggers")
    bool IsTriggerArmed(UConnectIt_MatchConditionAsset* Condition) const;

    // Checks every armed trigger against a committed board and the match's
    // rules, firing the ones that hold. Called by the GameMode after each
    // board change (nothing after the game is over).
    void CheckTriggers(const FConnectItBoardState& Board, const FConnectItRuleSet& Rules);

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
    virtual void AddConditionRuleWarnings(const FConnectItRuleSet& Rules, FDataValidationContext& Context) const override;
#endif

private:

    // Indices into Triggers that have fired and not been re-armed
    TSet<int32> FiredTriggers;

    TArray<TObjectPtr<UConnectIt_MatchConditionAsset>> GetTriggerConditions() const;
};
