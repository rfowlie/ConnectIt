// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ActionLoadoutDataAsset.h"
#include "Action/TurnBasedSpectatorAction.h"
#include "Misc/DataValidation.h"


// --- System Action Vending ---

UTurnBasedAction* UActionLoadoutDataAsset::GetRootAction(UObject* Outer) const
{
    return CreateSystemAction<UTurnBasedAction>(RootActionClass, Outer);
}

UTurnBasedSpectatorAction* UActionLoadoutDataAsset::GetIdleViewerAction(UObject* Outer) const
{
    return CreateSystemAction<UTurnBasedSpectatorAction>(IdleViewerActionClass, Outer);
}

UTurnBasedSpectatorAction* UActionLoadoutDataAsset::GetSpectatorAction(UObject* Outer) const
{
    return CreateSystemAction<UTurnBasedSpectatorAction>(SpectatorViewerActionClass, Outer);
}

UTurnBasedSpectatorAction* UActionLoadoutDataAsset::GetPauseAction(UObject* Outer) const
{
    return CreateSystemAction<UTurnBasedSpectatorAction>(PauseViewerActionClass, Outer);
}

UTurnBasedSpectatorAction* UActionLoadoutDataAsset::GetAwaitingConfirmationAction(UObject* Outer) const
{
    return CreateSystemAction<UTurnBasedSpectatorAction>(AwaitingConfirmationActionClass, Outer);
}

#if WITH_EDITOR
namespace
{
    // Walks the turn-end tree checking each leaf against the loadout's config.
    // MaxUsesPerTurnByClass maps a config class to its cap (0 = unlimited).
    void ValidateTurnEndNode(
        const UTurnEndRequirementNode* Node,
        const TMap<TSubclassOf<UTurnBasedAction>, int32>& MaxUsesPerTurnByClass,
        const FString& LoadoutName,
        FDataValidationContext& Context,
        EDataValidationResult& InOutResult)
    {
        if (!IsValid(Node)) return;

        if (const UTurnEndRequirementGroup* Group = Cast<UTurnEndRequirementGroup>(Node))
        {
            for (const UTurnEndRequirementNode* Child : Group->Children)
            {
                ValidateTurnEndNode(Child, MaxUsesPerTurnByClass, LoadoutName, Context, InOutResult);
            }
            return;
        }

        const UTurnEndRequirementLeaf* Leaf = Cast<UTurnEndRequirementLeaf>(Node);
        if (!Leaf) return;

        if (!Leaf->ActionClass)
        {
            Context.AddWarning(FText::FromString(FString::Printf(
                TEXT("Loadout '%s': a turn-end leaf has no ActionClass "
                     "(it will be skipped)."), *LoadoutName)));
            return;
        }

        const int32* Cap = MaxUsesPerTurnByClass.Find(Leaf->ActionClass);
        if (!Cap)
        {
            Context.AddError(FText::FromString(FString::Printf(
                TEXT("Loadout '%s': turn-end leaf references '%s', which is in "
                     "neither PermanentActions nor NumberedActions."),
                *LoadoutName, *Leaf->ActionClass->GetName())));
            InOutResult = EDataValidationResult::Invalid;
        }
        else if (*Cap > 0 && Leaf->RequiredUsesThisTurn > *Cap)
        {
            Context.AddError(FText::FromString(FString::Printf(
                TEXT("Loadout '%s': turn-end leaf needs %d uses of '%s' but its "
                     "MaxUsesPerTurn is %d -- the leaf can never be satisfied."),
                *LoadoutName, Leaf->RequiredUsesThisTurn,
                *Leaf->ActionClass->GetName(), *Cap)));
            InOutResult = EDataValidationResult::Invalid;
        }
    }
}

EDataValidationResult UActionLoadoutDataAsset::IsDataValid(
    FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);

    // --- Action config arrays ---
    // A class may appear once, in exactly one of the two arrays.
    TMap<TSubclassOf<UTurnBasedAction>, int32> MaxUsesPerTurnByClass;

    auto NoteClass = [&](const TSubclassOf<UTurnBasedAction>& ActionClass,
                         int32 MaxUsesPerTurn, const TCHAR* ArrayName)
    {
        if (!ActionClass)
        {
            Context.AddError(FText::FromString(FString::Printf(
                TEXT("Loadout '%s': an entry in %s has no ActionClass."),
                *LoadoutName, ArrayName)));
            Result = EDataValidationResult::Invalid;
            return;
        }

        if (MaxUsesPerTurnByClass.Contains(ActionClass))
        {
            Context.AddError(FText::FromString(FString::Printf(
                TEXT("Loadout '%s': '%s' appears more than once across "
                     "PermanentActions/NumberedActions."),
                *LoadoutName, *ActionClass->GetName())));
            Result = EDataValidationResult::Invalid;
            return;
        }

        if (!UTurnBasedAction::GetTagForClass(ActionClass).IsValid())
        {
            Context.AddWarning(FText::FromString(FString::Printf(
                TEXT("Loadout '%s': '%s' returns no ActionTag from GetActionTag."),
                *LoadoutName, *ActionClass->GetName())));
        }

        MaxUsesPerTurnByClass.Add(ActionClass, MaxUsesPerTurn);
    };

    for (const FPermanentActionConfig& Config : PermanentActions)
    {
        NoteClass(Config.ActionClass, Config.MaxUsesPerTurn, TEXT("PermanentActions"));
    }

    for (const FNumberedActionConfig& Config : NumberedActions)
    {
        NoteClass(Config.ActionClass, Config.MaxUsesPerTurn, TEXT("NumberedActions"));

        if (Config.MaxHeldUses > 0 && Config.StartingMatchUses > Config.MaxHeldUses)
        {
            Context.AddError(FText::FromString(FString::Printf(
                TEXT("Loadout '%s': '%s' starts with %d uses but MaxHeldUses is %d."),
                *LoadoutName,
                Config.ActionClass ? *Config.ActionClass->GetName() : TEXT("(none)"),
                Config.StartingMatchUses, Config.MaxHeldUses)));
            Result = EDataValidationResult::Invalid;
        }
    }

    // --- Turn-end tree ---
    if (IsValid(TurnEndRequirements))
    {
        if (MaxUsesPerTurnByClass.IsEmpty())
        {
            Context.AddWarning(FText::FromString(FString::Printf(
                TEXT("Loadout '%s': a TurnEndRequirements tree is set but no "
                     "PermanentActions/NumberedActions are configured."),
                *LoadoutName)));
        }
        else
        {
            ValidateTurnEndNode(TurnEndRequirements, MaxUsesPerTurnByClass,
                LoadoutName, Context, Result);
        }
    }

    // Warn if no root action set
    if (!RootActionClass)
    {
        Context.AddWarning(FText::FromString(FString::Printf(
            TEXT("ActionLoadoutDataAsset '%s': No RootActionClass set. "
                 "Root action is mandatory -- create a do-nothing action "
                 "if your game has no turn idle state."),
            *LoadoutName)));
    }

    // A loadout with no turn actions gives the player nothing to do
    if (MaxUsesPerTurnByClass.IsEmpty())
    {
        Context.AddWarning(FText::FromString(FString::Printf(
            TEXT("ActionLoadoutDataAsset '%s': no PermanentActions or "
                 "NumberedActions configured -- the player has no turn actions."),
            *LoadoutName)));
    }

    // Without a tree no action contributes to turn end, so a turn can be
    // ended at any time (and never ends automatically)
    if (!IsValid(TurnEndRequirements) && !MaxUsesPerTurnByClass.IsEmpty())
    {
        Context.AddWarning(FText::FromString(FString::Printf(
            TEXT("ActionLoadoutDataAsset '%s': no TurnEndRequirements tree set -- "
                 "turns never end automatically and can be ended at any time."),
            *LoadoutName)));
    }

    return Result;
}
#endif