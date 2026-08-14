// Fill out your copyright notice in the Description page of Project Settings.


#include "Action/ConnectIt_TutorialShiftIntroAction.h"
#include "Board/ConnectIt_BoardManager.h"
#include "ConnectIt_Structs.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "Tile/GridTileBase.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"


UConnectIt_TutorialShiftIntroAction::UConnectIt_TutorialShiftIntroAction()
{
    // Dedicated, single-purpose instance -- these are its permanent,
    // honest values. Never confused with the real UConnectIt_ShiftAction,
    // so unlike that action there is no need to toggle/restore anything.
    bIsRequired             = true;
    bIsCancellable          = false;
    bAllowsOptionalInterrupt = false;
    bRequiresSelection      = false;
    MaxCompletionsPerTurn   = 1;
    CooldownTurns           = 0;
}

void UConnectIt_TutorialShiftIntroAction::PostInitialiseAction_Implementation()
{
    BoardManager = UConnectIt_GameUtilityLibrary::GetBoardManager(OwningController);

    TurnBasedParticipantManager = IsValid(OwningController)
        ? OwningController->FindComponentByClass<UTurnBasedParticipantComponent>()
        : nullptr;

    if (!IsValid(BoardManager))
    {
        UE_LOG(LogTemp, Error,
            TEXT("TutorialShiftIntroAction: PostInitialiseAction -- "
                 "no AConnectIt_BoardManager found"));
    }

    if (!IsValid(TurnBasedParticipantManager))
    {
        UE_LOG(LogTemp, Error,
            TEXT("TutorialShiftIntroAction: PostInitialiseAction -- "
                 "no UTurnBasedParticipantComponent on owning controller"));
    }
}

void UConnectIt_TutorialShiftIntroAction::Activate_Internal_Implementation()
{
    // bRequiresSelection stays false, so the base class's own
    // Activate_Internal_Implementation (not called here -- we fully
    // replace it) never auto-binds hover input for this state
    CurrentState = EInternalState::ShowingInfo;
    ShowInfoWidget();

    UE_LOG(LogTemp, Log,
        TEXT("TutorialShiftIntroAction: Activated -- showing info widget"));
}

void UConnectIt_TutorialShiftIntroAction::NotifyInfoDismissed()
{
    if (CurrentState != EInternalState::ShowingInfo)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("TutorialShiftIntroAction: NotifyInfoDismissed called "
                 "outside ShowingInfo state -- ignoring"));
        return;
    }

    HideInfoWidget();

    CurrentState = EInternalState::SelectingShiftTarget;
    BindSelectionInput();

    UE_LOG(LogTemp, Log,
        TEXT("TutorialShiftIntroAction: Info dismissed -- "
             "awaiting shift target selection"));
}

bool UConnectIt_TutorialShiftIntroAction::IsValidHoverTile_Implementation(AGridTileBase* Tile) const
{
    return CurrentState == EInternalState::SelectingShiftTarget && IsValid(Tile);
}

void UConnectIt_TutorialShiftIntroAction::HandleValidHover_Implementation(AGridTileBase* Tile)
{
    // Hook point for tutorial visuals -- highlighting/pointing at a square,
    // fading other tiles. Left to a project/BP override; not designed here.
}

void UConnectIt_TutorialShiftIntroAction::HandleHoverCleared_Implementation(AGridTileBase* PreviousTile)
{
    // Hook point for tutorial visuals -- see HandleValidHover_Implementation.
}

void UConnectIt_TutorialShiftIntroAction::HandleValidSelection_Implementation(AGridTileBase* Tile)
{
    if (CurrentState != EInternalState::SelectingShiftTarget) return;
    if (!IsValid(Tile)) return;

    const FShiftOperation Operation = GetShiftOperationForTile(Tile);

    FTurnActionRequest Request;
    Request.RequestType = Tag_RequestType;
    Request.FactionID = GetOwningFactionID();
    Request.Payload.InitializeAs<FConnectItRequestBoardShift>(
        FConnectItRequestBoardShift{ .ShiftOperation = Operation });

    UE_LOG(LogTemp, Log,
        TEXT("TutorialShiftIntroAction: Shift target selected -- "
             "%s %d by %d, faction %d -- requesting board change"),
        Operation.Axis == EShiftAxis::Row ? TEXT("Row") : TEXT("Column"),
        Operation.Index,
        Operation.GetSignedAmount(),
        Request.FactionID);

    RequestBoardChange(Request);
    Complete();
}

FShiftOperation UConnectIt_TutorialShiftIntroAction::GetShiftOperationForTile_Implementation(
    AGridTileBase* Tile) const
{
    UE_LOG(LogTemp, Warning,
        TEXT("TutorialShiftIntroAction: GetShiftOperationForTile has no "
             "override -- returning a default FShiftOperation. Override "
             "this (C++ or Blueprint) to decide the actual scripted "
             "target for this tutorial moment."));

    return FShiftOperation();
}

void UConnectIt_TutorialShiftIntroAction::ShowInfoWidget()
{
    // APlayerController* PC = Cast<APlayerController>(OwningController);
    // if (!IsValid(PC)) return;
    // if (!InfoWidgetClass) return;
    //
    // ActiveInfoWidget = CreateWidget<UUserWidget>(PC, InfoWidgetClass);
    // if (IsValid(ActiveInfoWidget))
    // {
    //     ActiveInfoWidget->AddToViewport();
    //
    //     UE_LOG(LogTemp, Log,
    //         TEXT("TutorialShiftIntroAction: Info widget added to viewport"));
    // }
}

void UConnectIt_TutorialShiftIntroAction::HideInfoWidget()
{
    // if (IsValid(ActiveInfoWidget))
    // {
    //     ActiveInfoWidget->RemoveFromParent();
    //     ActiveInfoWidget = nullptr;
    //
    //     UE_LOG(LogTemp, Log,
    //         TEXT("TutorialShiftIntroAction: Info widget removed from viewport"));
    // }
}

int32 UConnectIt_TutorialShiftIntroAction::GetOwningFactionID() const
{
    return IsValid(TurnBasedParticipantManager)
        ? TurnBasedParticipantManager->GetActiveParticipantSlotIndex()
        : -1;
}
