// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_AIDirector.h"
#include "AI/ConnectIt_AIStrategy_MinMax.h"
#include "AI/ConnectIt_AISelector.h"
#include "Framework/Controller/ConnectIt_AIController.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Board/Operations/ConnectIt_BoardOperation.h"
#include "Misc/DataValidation.h"


UConnectIt_AIDirector::UConnectIt_AIDirector(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // The classic opponent until configured otherwise
    Strategies.Add(CreateDefaultSubobject<UConnectIt_AIStrategy_MinMax>(TEXT("MinMax")));
    Selector = CreateDefaultSubobject<UConnectIt_AISelector_FirstAvailable>(TEXT("FirstAvailable"));
}

AConnectIt_AIController* UConnectIt_AIDirector::GetOwningController() const
{
    return Cast<AConnectIt_AIController>(GetOuter());
}

int32 UConnectIt_AIDirector::GetFaction() const
{
    const AConnectIt_AIController* Controller = GetOwningController();
    const ATurnBasedPlayerState* PS = Controller ? Controller->GetPlayerState<ATurnBasedPlayerState>() : nullptr;
    return PS ? PS->GetSlotIndex() : INDEX_NONE;
}

#if WITH_EDITOR
EDataValidationResult UConnectIt_AIDirector::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    if (!IsValid(StartingLoadout))
    {
        Context.AddError(FText::FromString(TEXT(
            "ConnectIt_AIDirector: No Starting Loadout set -- every move this AI makes "
            "would be rejected.")));
        Result = EDataValidationResult::Invalid;
    }

    FGameplayTagContainer Supported;
    bool bAnyStrategy = false;
    for (const UConnectIt_AIStrategy* Strategy : Strategies)
    {
        if (!IsValid(Strategy)) continue;
        bAnyStrategy = true;
        Supported.AppendTags(Strategy->GetSupportedRequestTypes());
    }
    if (!bAnyStrategy)
    {
        Context.AddError(FText::FromString(TEXT(
            "ConnectIt_AIDirector: No Strategies -- this AI never proposes a move.")));
        Result = EDataValidationResult::Invalid;
    }

    if (!IsValid(Selector))
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_AIDirector: No Selector -- First Available is used.")));
    }

    // Does what the strategies can play match what the loadout allows?
    // (Strategies that don't declare their moves are not checked.)
    if (IsValid(StartingLoadout) && !Supported.IsEmpty())
    {
        FGameplayTagContainer Granted;
        for (const FGameplayTag& RequestType : FConnectItBoardOperation::GetAllRequestTypes())
        {
            if (UConnectIt_AIStrategy::LoadoutGrantsRequestType(StartingLoadout, RequestType))
            {
                Granted.AddTag(RequestType);
            }
        }

        if (!Granted.HasAnyExact(Supported))
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_AIDirector: the strategies only play %s, and the starting loadout "
                "grants none of it -- this AI can never move."),
                *Supported.ToStringSimple())));
            Result = EDataValidationResult::Invalid;
        }

        FGameplayTagContainer NeverPlayed;
        for (const FGameplayTag& RequestType : Granted)
        {
            if (!Supported.HasTagExact(RequestType))
            {
                NeverPlayed.AddTag(RequestType);
            }
        }
        if (!NeverPlayed.IsEmpty())
        {
            Context.AddWarning(FText::FromString(FString::Printf(TEXT(
                "ConnectIt_AIDirector: the starting loadout grants %s, which no strategy "
                "plays."),
                *NeverPlayed.ToStringSimple())));
        }
    }

    // Not invalid (a preset may be played versus only), but an AI with no
    // goals can't win
    if (WinConditions.IsEmpty())
    {
        Context.AddWarning(FText::FromString(TEXT(
            "ConnectIt_AIDirector: No Win Conditions -- this AI can never win.")));
    }
    ValidateConditionList(this, TEXT("Win Conditions"), WinConditions, Context, Result);
    ValidateConditionList(this, TEXT("Lose Conditions"), LoseConditions, Context, Result);

    return Result;
}

void UConnectIt_AIDirector::AddConditionRuleWarnings(const FConnectItRuleSet& Rules, FDataValidationContext& Context) const
{
    AddConditionListRuleWarnings(this, TEXT("Win Conditions"), WinConditions, Rules, Context);
    AddConditionListRuleWarnings(this, TEXT("Lose Conditions"), LoseConditions, Rules, Context);
}
#endif

void UConnectIt_AIDirector::ApplyPendingChanges()
{
    if (bHasPendingStrategies)
    {
        bHasPendingStrategies = false;
        Strategies = MoveTemp(PendingStrategies);
        PendingStrategies.Reset();
    }
    if (IsValid(PendingSelector))
    {
        Selector = PendingSelector;
        PendingSelector = nullptr;
    }
}

void UConnectIt_AIDirector::SetAILoadout(UActionLoadoutDataAsset* Loadout)
{
    if (AConnectIt_AIController* Controller = GetOwningController())
    {
        Controller->ChangeLoadout(Loadout);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ConnectIt_AIDirector: %s has no AI controller -- loadout not changed"), *GetName());
    }
}

UActionLoadoutDataAsset* UConnectIt_AIDirector::GetAILoadout() const
{
    const AConnectIt_AIController* Controller = GetOwningController();
    const ATurnBasedPlayerState* PS = Controller ? Controller->GetPlayerState<ATurnBasedPlayerState>() : nullptr;
    return PS ? PS->GetLoadout() : nullptr;
}

void UConnectIt_AIDirector::SetAIStrategies(const TArray<UConnectIt_AIStrategy*>& StrategyTemplates)
{
    // Our own copies: a decision may be running on the current ones
    PendingStrategies.Reset();
    for (const UConnectIt_AIStrategy* Template : StrategyTemplates)
    {
        if (IsValid(Template))
        {
            PendingStrategies.Add(DuplicateObject<UConnectIt_AIStrategy>(Template, this));
        }
    }
    bHasPendingStrategies = true;

    UE_LOG(LogTemp, Log, TEXT("ConnectIt_AIDirector: %d strateg%s from the next decision"),
        PendingStrategies.Num(), PendingStrategies.Num() == 1 ? TEXT("y") : TEXT("ies"));
}

void UConnectIt_AIDirector::SetAISelector(UConnectIt_AISelector* SelectorTemplate)
{
    if (!IsValid(SelectorTemplate)) return;

    PendingSelector = DuplicateObject<UConnectIt_AISelector>(SelectorTemplate, this);
    UE_LOG(LogTemp, Log, TEXT("ConnectIt_AIDirector: selector changes to %s from the next decision"),
        *GetNameSafe(SelectorTemplate->GetClass()));
}
