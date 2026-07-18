// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ActionLoadoutDataAsset.h"
#include "Action/TurnBasedSpectatorAction.h"
#include "Misc/DataValidation.h"


// --- System Action Vending ---

UTurnBasedAction* UActionLoadOutDataAsset::GetRootAction(UObject* Outer) const
{
    if (!RootActionClass || !IsValid(Outer)) return nullptr;

    return NewObject<UTurnBasedAction>(Outer, RootActionClass);
}

UTurnBasedSpectatorAction* UActionLoadOutDataAsset::GetIdleViewerAction(UObject* Outer) const
{
    if (!IdleViewerActionClass || !IsValid(Outer)) return nullptr;

    return NewObject<UTurnBasedSpectatorAction>(Outer, IdleViewerActionClass);
}

UTurnBasedSpectatorAction* UActionLoadOutDataAsset::GetSpectatorAction(UObject* Outer) const
{
    if (!SpectatorViewerActionClass || !IsValid(Outer)) return nullptr;

    return NewObject<UTurnBasedSpectatorAction>(
        Outer, SpectatorViewerActionClass);
}

UTurnBasedSpectatorAction* UActionLoadOutDataAsset::GetPauseAction(UObject* Outer) const
{
    if (!PauseViewerActionClass || !IsValid(Outer)) return nullptr;

    return NewObject<UTurnBasedSpectatorAction>(Outer, PauseViewerActionClass);
}

// --- Turn Action Accessors ---

TArray<UTurnBasedAction*> UActionLoadOutDataAsset::GetPermittedActions() const
{
    TArray<UTurnBasedAction*> Permitted;
    Permitted.Reserve(Actions.Num());

    for (UTurnBasedAction* Action : Actions)
    {
        if (!IsValid(Action)) continue;
        if (BannedActionTags.HasTag(Action->ActionTag)) continue;
        Permitted.Add(Action);
    }

    return Permitted;
}

TArray<UTurnBasedAction*> UActionLoadOutDataAsset::GetRequiredActions() const
{
    TArray<UTurnBasedAction*> Required;

    for (UTurnBasedAction* Action : GetPermittedActions())
    {
        if (IsValid(Action) && Action->bIsRequired)
        {
            Required.Add(Action);
        }
    }

    return Required;
}

TArray<UTurnBasedAction*> UActionLoadOutDataAsset::GetOptionalActions() const
{
    TArray<UTurnBasedAction*> Optional;

    for (UTurnBasedAction* Action : GetPermittedActions())
    {
        if (IsValid(Action) && !Action->bIsRequired)
        {
            Optional.Add(Action);
        }
    }

    return Optional;
}

bool UActionLoadOutDataAsset::IsActionPermitted(FGameplayTag ActionTag) const
{
    if (BannedActionTags.HasTag(ActionTag)) return false;

    return Actions.ContainsByPredicate(
        [ActionTag](const UTurnBasedAction* Action)
        {
            return IsValid(Action) && Action->ActionTag == ActionTag;
        });
}

#if WITH_EDITOR
EDataValidationResult UActionLoadOutDataAsset::IsDataValid(
    FDataValidationContext& Context) const
{
    const EDataValidationResult Result = Super::IsDataValid(Context);

    // Warn if no root action set
    if (!RootActionClass)
    {
        Context.AddWarning(FText::FromString(FString::Printf(
            TEXT("ActionLoadoutDataAsset '%s': No RootActionClass set. "
                 "Root action is mandatory -- create a do-nothing action "
                 "if your game has no turn idle state."),
            *LoadoutName)));
    }

    // Warn if no required actions
    if (GetRequiredActions().IsEmpty())
    {
        Context.AddWarning(FText::FromString(FString::Printf(
            TEXT("ActionLoadoutDataAsset '%s': No required actions. "
                 "Turn end will always be available immediately."),
            *LoadoutName)));
    }

    // Warn on duplicate tags
    TSet<FGameplayTag> SeenTags;
    for (const UTurnBasedAction* Action : Actions)
    {
        if (!IsValid(Action)) continue;

        if (!Action->ActionTag.IsValid())
        {
            Context.AddWarning(FText::FromString(FString::Printf(
                TEXT("ActionLoadoutDataAsset '%s': "
                     "An action has no ActionTag set."),
                *LoadoutName)));
            continue;
        }

        if (SeenTags.Contains(Action->ActionTag))
        {
            Context.AddWarning(FText::FromString(FString::Printf(
                TEXT("ActionLoadoutDataAsset '%s': "
                     "Duplicate ActionTag '%s' found."),
                *LoadoutName,
                *Action->ActionTag.ToString())));
        }

        SeenTags.Add(Action->ActionTag);
    }

    return Result;
}
#endif