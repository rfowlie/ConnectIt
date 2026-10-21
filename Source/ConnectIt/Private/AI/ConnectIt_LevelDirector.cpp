// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_LevelDirector.h"
#include "Board/Rules/ConnectIt_MatchConditionAsset.h"
#include "Board/Rules/ConnectIt_RuleSet.h"
#include "Misc/DataValidation.h"
#include "Framework/GameMode/ConnectIt_GameMode.h"


void UConnectIt_LevelDirector::RearmTrigger(UConnectIt_MatchConditionAsset* Condition)
{
    for (int32 Index = 0; Index < Triggers.Num(); Index++)
    {
        if (Triggers[Index].Condition == Condition)
        {
            FiredTriggers.Remove(Index);
        }
    }
}

bool UConnectIt_LevelDirector::IsTriggerArmed(UConnectIt_MatchConditionAsset* Condition) const
{
    for (int32 Index = 0; Index < Triggers.Num(); Index++)
    {
        if (Triggers[Index].Condition == Condition)
        {
            return !FiredTriggers.Contains(Index);
        }
    }
    return false;
}

void UConnectIt_LevelDirector::CheckTriggers(const FConnectItBoardState& Board, const FConnectItRuleSet& Rules)
{
    if (Board.bGameOver) return;

    const FConnectItConditionContext Context{ Board, Rules };
    for (int32 Index = 0; Index < Triggers.Num(); Index++)
    {
        if (FiredTriggers.Contains(Index)) continue;

        const FConnectItLevelTrigger& Trigger = Triggers[Index];
        const UConnectIt_MatchConditionAsset* Asset = Trigger.Condition;
        if (!IsValid(Asset) || Asset->Conditions.IsEmpty()) continue;

        for (int32 Faction = 0; Faction < Board.ScoreBoard.Num(); Faction++)
        {
            const bool bIsAIFaction = Rules.AIFaction != INDEX_NONE && Faction == Rules.AIFaction;
            const bool bSideMatches =
                Trigger.Side == EConnectItTriggerSide::Either
                || (Trigger.Side == EConnectItTriggerSide::AI && bIsAIFaction)
                || (Trigger.Side == EConnectItTriggerSide::Player && !bIsAIFaction);
            if (!bSideMatches) continue;

            const bool bAllMet = !Asset->Conditions.ContainsByPredicate(
                [&Context, Faction](const TInstancedStruct<FConnectItMatchCondition>& Entry)
                {
                    const FConnectItMatchCondition* Condition = Entry.GetPtr();
                    return !Condition || !Condition->IsMetFor(Context, Faction);
                });
            if (!bAllMet) continue;

            // Disarm before reacting: the reaction may re-arm it
            FiredTriggers.Add(Index);
            UE_LOG(LogTemp, Log, TEXT("ConnectIt_LevelDirector: trigger '%s' fired for faction %d"),
                *Asset->GetName(), Faction);
            OnTriggerFired(Triggers[Index].Condition, Faction);
            break;
        }
    }
}

void UConnectIt_LevelDirector::ApplyBoardOperation(const FInstancedStruct& Operation, int32 AsFaction, EConnectItChangeTiming Timing)
{
    if (AConnectIt_GameMode* GameMode = GetGameMode())
    {
        GameMode->ApplyLevelOperation(Operation, AsFaction, Timing);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ConnectIt_LevelDirector: %s has no GameMode -- board not changed"), *GetName());
    }
}

bool UConnectIt_LevelDirector::SetScoringRule(const FInstancedStruct& ScoringRule, EConnectItChangeTiming Timing)
{
    AConnectIt_GameMode* GameMode = GetGameMode();
    return IsValid(GameMode) && GameMode->SetScoringRule(ScoringRule, Timing);
}

FConnectItRuleSet UConnectIt_LevelDirector::GetMatchRules() const
{
    const AConnectIt_GameMode* GameMode = GetGameMode();
    return IsValid(GameMode) ? GameMode->GetRules() : FConnectItRuleSet();
}

TArray<TObjectPtr<UConnectIt_MatchConditionAsset>> UConnectIt_LevelDirector::GetTriggerConditions() const
{
    TArray<TObjectPtr<UConnectIt_MatchConditionAsset>> Conditions;
    for (const FConnectItLevelTrigger& Trigger : Triggers)
    {
        Conditions.Add(Trigger.Condition);
    }
    return Conditions;
}

#if WITH_EDITOR
EDataValidationResult UConnectIt_LevelDirector::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);
    ValidateConditionList(this, TEXT("Triggers"), GetTriggerConditions(), Context, Result);
    return Result;
}

void UConnectIt_LevelDirector::AddConditionRuleWarnings(const FConnectItRuleSet& Rules, FDataValidationContext& Context) const
{
    AddConditionListRuleWarnings(this, TEXT("Triggers"), GetTriggerConditions(), Rules, Context);
}
#endif
