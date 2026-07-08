// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Controller/ConnectIt_AIController.h"
#include "EngineUtils.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Action/TurnBasedActionComponent.h"
#include "Board/ConnectItBoardManager.h"
#include "Framework/GameMode/TurnBasedGameMode.h"
#include "MinMax/ConnectIt_MinMaxTreeBuilder.h"
#include "Turn/Participant/TurnBasedParticipantComponent.h"


AConnectIt_AIController::AConnectIt_AIController()
{
    ParticipantComponent =
        CreateDefaultSubobject<UTurnBasedParticipantComponent>(
            TEXT("ParticipantComponent"));

    ActionComponent =
        CreateDefaultSubobject<UTurnBasedActionComponent>(
            TEXT("ActionComponent"));
    
}

void AConnectIt_AIController::BeginPlay()
{
    Super::BeginPlay();

    // Cache slot index for blackboard lookups
    MySlotIndex = ParticipantComponent->CachedSlotIndex;

    ParticipantComponent->OnTurnNotificationReceived.AddDynamic(
        this, &AConnectIt_AIController::HandleTurnNotification);
    

    InitialiseFromBoardActor();
}

void AConnectIt_AIController::InitialiseFromBoardActor()
{
    for (TActorIterator<AConnectItBoardManager> It(GetWorld()); It; ++It)
    {
        BoardActor = *It;
        break;
    }

    if (!IsValid(BoardActor))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectItAIController: No AConnectItBoardActor found"));
        return;
    }

    // Initialise action component from enemy loadout
    UActionLoadOutDataAsset* LoadOut = BoardActor->GetEnemyLoadout();
    if (IsValid(LoadOut))
    {
        ActionComponent->InitialiseFromLoadout(LoadOut);

        // Wire passthrough to board manager
        ActionComponent->OnBoardChangeRequested.AddDynamic(
            BoardActor->BoardManager,
            &UConnectItBoardManagerComponent::ProcessRequest);
    }

    // auto attempt to 
    if (ATurnBasedGameMode* GameMode = Cast<ATurnBasedGameMode>(
        GetWorld()->GetAuthGameMode()))
    {
        GameMode->RegisterAIParticipant(this);
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectItAIController: game mode not Turn Based"));
        return;
    }
    
    UE_LOG(LogTemp, Log,
        TEXT("ConnectItAIController: Initialised from board actor"));
}

bool AConnectIt_AIController::CheckAndApplyForcedMove()
{
    // UConnectItBlackboardSubsystem* Blackboard =
    //     GetWorld()->GetSubsystem<UConnectItBlackboardSubsystem>();
    //
    // if (!IsValid(Blackboard)) return false;
    // if (!Blackboard->HasModifier(MySlotIndex)) return false;
    //
    // const FTurnModifier Modifier = Blackboard->GetModifier(MySlotIndex);
    // Blackboard->ClearModifier(MySlotIndex);
    //
    // if (!IsValid(Modifier.TargetTile))
    // {
    //     UE_LOG(LogTemp, Warning,
    //         TEXT("ConnectItAIController: Forced move modifier has "
    //              "null TargetTile — ignoring"));
    //     return false;
    // }
    //
    // const FGridPosition ForcedPosition =
    //     Modifier.TargetTile->GetGridPosition();
    //
    // UE_LOG(LogTemp, Log,
    //     TEXT("ConnectItAIController: Applying forced move at (%d,%d) "
    //          "from shard '%s'"),
    //     ForcedPosition.X, ForcedPosition.Y,
    //     *Modifier.SourceTag.ToString());
    //
    // SubmitMove(ForcedPosition);
    return true;
}

void AConnectIt_AIController::SubmitMove(const FGridPosition Position) const
{
    // Build request identical to what UPlacePieceAction would send
    FTurnActionRequest Request;
    Request.RequestType =
        FGameplayTag::RequestGameplayTag("ConnectIt.Board.PlacePiece");
    Request.Positions.Add(Position);
    Request.FactionID = MySlotIndex;

    // Route through action component ServerRPC
    // Same path as human player actions -- consistent validation
    ActionComponent->OnBoardChangeRequested.Broadcast(Request);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectItAIController: Move submitted at (%d,%d)"),
        Position.X, Position.Y);
}

