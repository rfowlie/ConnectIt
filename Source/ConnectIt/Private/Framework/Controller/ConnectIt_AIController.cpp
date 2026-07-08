// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/Controller/ConnectIt_AIController.h"
#include "EngineUtils.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Action/TurnBasedActionComponent.h"
#include "Board/ConnectItBoardActor.h"
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

    TreeBuilder =
        CreateDefaultSubobject<UConnectIt_MinMaxTreeBuilder>(
            TEXT("TreeBuilder"));
}

void AConnectIt_AIController::BeginPlay()
{
    Super::BeginPlay();

    // Cache slot index for blackboard lookups
    MySlotIndex = ParticipantComponent->CachedSlotIndex;

    ParticipantComponent->OnTurnStarted.AddDynamic(
        this, &AConnectIt_AIController::HandleTurnStarted);

    ParticipantComponent->OnTurnEnded.AddDynamic(
        this, &AConnectIt_AIController::HandleTurnEnded);

    InitialiseFromBoardActor();
}

void AConnectIt_AIController::InitialiseFromBoardActor()
{
    for (TActorIterator<AConnectItBoardActor> It(GetWorld()); It; ++It)
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

    // Wire tree builder completion to handler
    TreeBuilder->OnSolveTreeComplete.AddDynamic(
        this, &AConnectIt_AIController::HandleMinMaxComplete);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectItAIController: Initialised from board actor"));
}

void AConnectIt_AIController::HandleTurnStarted(
    const FTurnNotification& Notification)
{
    // UE_LOG(LogTemp, Log,
    //     TEXT("ConnectItAIController: Turn %d started — "
    //          "checking blackboard then running MinMax"),
    //     Notification.TurnNumber);
    //
    // ActionComponent->OnTurnBegan(Notification.TurnNumber);
    //
    // // Check for forced move from player shard activation
    // // If found submit immediately without running MinMax
    // if (CheckAndApplyForcedMove())
    // {
    //     UE_LOG(LogTemp, Log,
    //         TEXT("ConnectItAIController: Forced move applied — "
    //              "skipping MinMax"));
    //     return;
    // }
    //
    // // Build and solve MinMax tree async
    // // HandleMinMaxComplete fires when done
    // if (!IsValid(BoardActor) || !IsValid(TreeBuilder)) return;
    //
    // const FConnectItBoardState& CurrentState =
    //     BoardActor->ConnectItBoardState->GetCurrentState();
    //
    // // Build root node from current board state
    // FConnectItMinMaxNode RootNode;
    // RootNode.Tiles      = CurrentState.TilePositions;
    // RootNode.ScoreBoard = CurrentState.ScoreBoard;
    // RootNode.FactionTurn = MySlotIndex;
    //
    // const int32 SearchDepth =
    //     IsValid(BoardActor->ConnectItConfig)
    //         ? BoardActor->ConnectItConfig->AISearchDepth
    //         : 3;
    //
    // const int32 ThreadDepth =
    //     IsValid(BoardActor->ConnectItConfig)
    //         ? BoardActor->ConnectItConfig->AIThreadDepth
    //         : 1;
    //
    // TreeBuilder->BuildTreeAsync(RootNode, SearchDepth, ThreadDepth);
}

void AConnectIt_AIController::HandleTurnEnded(
    const FTurnNotification& Notification)
{
    UE_LOG(LogTemp, Log,
        TEXT("ConnectItAIController: Turn %d ended"),
        Notification.TurnNumber);
}

void AConnectIt_AIController::HandleMinMaxComplete()
{
    // // RootNodeMoveOutcomes holds all root children with their scores
    // // Sorted highest first by SolveTreeAsync
    // const TArray<FConnectItMoveOutcome>& Outcomes =
    //     TreeBuilder->RootNodeMoveOutcomes;
    //
    // if (Outcomes.IsEmpty())
    // {
    //     UE_LOG(LogTemp, Warning,
    //         TEXT("ConnectItAIController: MinMax returned no outcomes "
    //              "— submitting turn end without placing"));
    //     ActionComponent->RequestTurnEnd();
    //     return;
    // }
    //
    // // Best move is first -- highest score
    // const FGridPosition BestPosition = Outcomes[0].Node.MovePlayed;
    //
    // UE_LOG(LogTemp, Log,
    //     TEXT("ConnectItAIController: MinMax complete — "
    //          "best move (%d,%d) score %d"),
    //     BestPosition.X, BestPosition.Y, Outcomes[0].Score);
    //
    // SubmitMove(BestPosition);
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

