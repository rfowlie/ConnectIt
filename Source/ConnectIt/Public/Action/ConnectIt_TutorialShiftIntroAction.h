// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action/TurnBasedAction.h"
#include "Board/Shift/GridShiftTypes.h"
#include "ConnectIt_TutorialShiftIntroAction.generated.h"


class UTurnBasedParticipantComponent;
class AConnectIt_BoardManager;
class UUserWidget;

// ConnectIt tutorial action -- forces a scripted "learn the shift mechanic"
// moment as a single action with an internal state machine, rather than two
// separate actions handed off across the stack. Deliberately a dedicated,
// single-purpose instance: bIsRequired/bIsCancellable/bAllowsOptionalInterrupt
// are its permanent, honest values (set once in the constructor), never
// toggled/restored, since it is never confused with the real
// UConnectIt_ShiftAction.
//
// State machine:
//   ShowingInfo          -- info widget up. bRequiresSelection stays false
//                           so the base class never auto-binds hover input
//                           for this state; tile hover/selection from
//                           whatever was active underneath is already
//                           stopped for free (ForceDeactivate on push).
//   SelectingShiftTarget -- widget dismissed (NotifyInfoDismissed), hover
//                           selection bound directly, waiting for the
//                           player to pick a shift target.
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class CONNECTIT_API UConnectIt_TutorialShiftIntroAction : public UTurnBasedAction
{
    GENERATED_BODY()

    UConnectIt_TutorialShiftIntroAction();

public:

    // Request type tag sent to board manager -- must match
    // ConnectIt_Game_Shift (Source/ConnectIt/Public/ConnectIt_GameplayTags.h),
    // the same request type the real UConnectIt_ShiftAction uses
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Tags")
    FGameplayTag Tag_RequestType;

    // Widget shown while CurrentState == ShowingInfo
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Config")
    TSubclassOf<UUserWidget> InfoWidgetClass;

    // Called by the info widget's "Next" button. The widget resolves this
    // action instance itself via
    // OwningController->FindComponentByClass<UTurnBasedActionsComponent>()
    // ->GetTopAction() (already BlueprintPure) rather than being handed a
    // reference at creation time -- no new wiring mechanism needed.
    // Dismisses the widget and transitions into shift-target selection.
    UFUNCTION(BlueprintCallable, Category = "Tutorial")
    void NotifyInfoDismissed();

protected:

    virtual void Activate_Internal_Implementation() override;

    // Resolves and caches BoardManager/TurnBasedParticipantManager via
    // OwningController -- mirrors UConnectIt_PlacePieceAction/
    // UConnectIt_ShiftAction's pattern.
    virtual void PostInitialiseAction_Implementation() override;

    // Selection hooks -- only ever fire while CurrentState ==
    // SelectingShiftTarget, since hover input is only ever bound during
    // that state (NotifyInfoDismissed), but gated defensively regardless.
    virtual bool IsValidHoverTile_Implementation(AGridTileBase* Tile) const override;
    virtual void HandleValidHover_Implementation(AGridTileBase* Tile) override;
    virtual void HandleHoverCleared_Implementation(AGridTileBase* PreviousTile) override;
    virtual void HandleValidSelection_Implementation(AGridTileBase* Tile) override;

    // Derives the shift operation to perform from the selected tile.
    // BlueprintNativeEvent so a project/BP pass can decide the actual
    // scripted target (fixed pre-scripted target vs. free row/column
    // choice) -- deliberately left as a placeholder here; the default
    // implementation returns an invalid FShiftOperation and warns. This is
    // a presentation/design decision, not part of this round's scope.
    UFUNCTION(BlueprintNativeEvent, Category = "Tutorial")
    FShiftOperation GetShiftOperationForTile(AGridTileBase* Tile) const;
    virtual FShiftOperation GetShiftOperationForTile_Implementation(AGridTileBase* Tile) const;

private:

    enum class EInternalState : uint8
    {
        ShowingInfo,
        SelectingShiftTarget
    };

    EInternalState CurrentState = EInternalState::ShowingInfo;

    // UPROPERTY()
    // TObjectPtr<UUserWidget> ActiveInfoWidget = nullptr;

    UPROPERTY()
    TObjectPtr<AConnectIt_BoardManager> BoardManager = nullptr;

    UPROPERTY()
    TObjectPtr<UTurnBasedParticipantComponent> TurnBasedParticipantManager = nullptr;

    void ShowInfoWidget();
    void HideInfoWidget();

    // Gets owning faction ID from participant component slot index
    int32 GetOwningFactionID() const;
};
