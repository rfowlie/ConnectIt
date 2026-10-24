// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ActionLoadoutDataAsset.h"
#include "Action/TurnBasedSpectatorAction.h"
#include "Misc/DataValidation.h"


// --- System Action Vending ---

UTurnBasedPlayerAction* UActionLoadoutDataAsset::GetRootAction(UObject* Outer) const
{
    return CreateSystemAction<UTurnBasedPlayerAction>(RootActionClass, Outer);
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
    // TurnUsesByClass maps a config class to its per-turn cap.
    void ValidateTurnEndNode(
        const UTurnEndRequirementNode* Node,
        const TMap<TSubclassOf<UTurnBasedPlayerAction>, int32>& TurnUsesByClass,
        const FString& LoadoutName,
        FDataValidationContext& Context,
        EDataValidationResult& InOutResult)
    {
        if (!IsValid(Node)) return;

        if (const UTurnEndRequirementGroup* Group = Cast<UTurnEndRequirementGroup>(Node))
        {
            for (const UTurnEndRequirementNode* Child : Group->Children)
            {
                ValidateTurnEndNode(Child, TurnUsesByClass, LoadoutName, Context, InOutResult);
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

        const int32* Cap = TurnUsesByClass.Find(Leaf->ActionClass);
        if (!Cap)
        {
            Context.AddError(FText::FromString(FString::Printf(
                TEXT("Loadout '%s': turn-end leaf references '%s', which isn't "
                     "in PermanentActions."),
                *LoadoutName, *Leaf->ActionClass->GetName())));
            InOutResult = EDataValidationResult::Invalid;
        }
        else if (Leaf->RequiredUsesThisTurn > *Cap)
        {
            Context.AddError(FText::FromString(FString::Printf(
                TEXT("Loadout '%s': turn-end leaf needs %d uses of '%s' but its "
                     "TurnUses is %d -- the leaf can never be satisfied."),
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

    // --- Permanent actions ---
    // A class may appear once
    TMap<TSubclassOf<UTurnBasedPlayerAction>, int32> TurnUsesByClass;

    for (const FPermanentActionConfig& Config : PermanentActions)
    {
        if (!Config.ActionClass)
        {
            Context.AddError(FText::FromString(FString::Printf(
                TEXT("Loadout '%s': an entry in PermanentActions has no ActionClass."),
                *LoadoutName)));
            Result = EDataValidationResult::Invalid;
            continue;
        }

        const FString ClassName = Config.ActionClass->GetName();
        if (TurnUsesByClass.Contains(Config.ActionClass))
        {
            Context.AddError(FText::FromString(FString::Printf(
                TEXT("Loadout '%s': '%s' appears more than once in PermanentActions."),
                *LoadoutName, *ClassName)));
            Result = EDataValidationResult::Invalid;
            continue;
        }

        if (Config.TurnUses < 1)
        {
            Context.AddError(FText::FromString(FString::Printf(
                TEXT("Loadout '%s': '%s' has TurnUses %d -- a turn always has a "
                     "limit of at least 1."),
                *LoadoutName, *ClassName, Config.TurnUses)));
            Result = EDataValidationResult::Invalid;
        }

        if (Config.MatchUses == 0)
        {
            Context.AddWarning(FText::FromString(FString::Printf(
                TEXT("Loadout '%s': '%s' has MatchUses 0 -- it can never be used "
                     "(-1 = unlimited)."),
                *LoadoutName, *ClassName)));
        }

        if (!UTurnBasedPlayerAction::GetTagForClass(Config.ActionClass).IsValid())
        {
            Context.AddWarning(FText::FromString(FString::Printf(
                TEXT("Loadout '%s': '%s' returns no ActionTag from GetActionTag."),
                *LoadoutName, *ClassName)));
        }

        TurnUsesByClass.Add(Config.ActionClass, Config.TurnUses);
    }

    // --- Turn-end tree ---
    if (IsValid(TurnEndRequirements))
    {
        if (TurnUsesByClass.IsEmpty())
        {
            Context.AddWarning(FText::FromString(FString::Printf(
                TEXT("Loadout '%s': a TurnEndRequirements tree is set but no "
                     "PermanentActions are configured."),
                *LoadoutName)));
        }
        else
        {
            ValidateTurnEndNode(TurnEndRequirements, TurnUsesByClass,
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

    // A loadout with no turn actions gives the player nothing to do (unless
    // the game seeds its own list)
    if (TurnUsesByClass.IsEmpty())
    {
        Context.AddWarning(FText::FromString(FString::Printf(
            TEXT("ActionLoadoutDataAsset '%s': no PermanentActions configured -- "
                 "the player has no turn actions unless the game gives them some."),
            *LoadoutName)));
    }

    // Without a tree no action contributes to turn end, so a turn can be
    // ended at any time (and never ends automatically)
    if (!IsValid(TurnEndRequirements) && !TurnUsesByClass.IsEmpty())
    {
        Context.AddWarning(FText::FromString(FString::Printf(
            TEXT("ActionLoadoutDataAsset '%s': no TurnEndRequirements tree set -- "
                 "turns never end automatically and can be ended at any time."),
            *LoadoutName)));
    }

    return Result;
}
#endif