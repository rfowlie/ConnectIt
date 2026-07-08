// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/ConnectItBoardActor.h"

#include "Board/Shift/BoardShiftComponent.h"
#include "Framework/Data/ConnectIt_ConfigComponent.h"
#include "Interpreter/ConnectItPieceSpawnInterpreter.h"
#include "Interpreter/ConnectItScoreInterpreter.h"
#include "Interpreter/ConnectItTileStateInterpreter.h"


AConnectItBoardActor::AConnectItBoardActor()
{
    // ConnectIt specific board state
    // Replaces the generic UBoardStateComponentBase slot on ABoardActor
    ConnectItBoardState = CreateDefaultSubobject<UConnectItBoardStateComponent>(
        TEXT("ConnectItBoardState"));

    // Board manager -- server side logic
    BoardManager = CreateDefaultSubobject<UConnectItBoardManagerComponent>(
        TEXT("BoardManager"));

    // Designer configuration
    ConnectItConfig = CreateDefaultSubobject<UConnectIt_ConfigComponent>(
        TEXT("ConnectItConfig"));

    // Interpreters -- all created as components
    // Visible in Details panel so designers can configure them
    TileStateInterpreter = CreateDefaultSubobject<UConnectItTileStateInterpreter>(
        TEXT("TileStateInterpreter"));

    PieceSpawnInterpreter = CreateDefaultSubobject<UConnectItPieceSpawnInterpreter>(
        TEXT("PieceSpawnInterpreter"));

    ScoreInterpreter = CreateDefaultSubobject<UConnectItScoreInterpreter>(
        TEXT("ScoreInterpreter"));

    // Board actor replicates -- board state component replicates on it
    bReplicates = true;
}

void AConnectItBoardActor::BeginPlay()
{
    Super::BeginPlay();

    // Wire interpreters to board state component
    // All clients bind -- interpreters run on every machine
    BindInterpreters();

    // Wire board manager delegates -- server only
    if (HasAuthority())
    {
        BindBoardManager();

        // Sync win threshold from config to manager
        if (IsValid(ConnectItConfig) && IsValid(BoardManager))
        {
            BoardManager->WinScoreThreshold =
                ConnectItConfig->WinScoreThreshold;
            BoardManager->ConnectLength =
                ConnectItConfig->ConnectLength;
        }
    }
}

void AConnectItBoardActor::BindInterpreters()
{
    if (!IsValid(ConnectItBoardState))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectItBoardActor: Cannot bind interpreters — "
                 "ConnectItBoardState is null"));
        return;
    }

    // Each interpreter binds independently
    // If an interpreter is null it is skipped gracefully
    // Designers can remove interpreters they do not need

    if (IsValid(TileStateInterpreter))
    {
        TileStateInterpreter->BindToBoardStateComponent(ConnectItBoardState);
    }

    if (IsValid(PieceSpawnInterpreter))
    {
        PieceSpawnInterpreter->BindToBoardStateComponent(ConnectItBoardState);
    }

    if (IsValid(ScoreInterpreter))
    {
        ScoreInterpreter->BindToBoardStateComponent(ConnectItBoardState);
    }

    UE_LOG(LogTemp, Log,
        TEXT("ConnectItBoardActor: Interpreters bound to board state"));
}

void AConnectItBoardActor::BindBoardManager()
{
    if (!IsValid(BoardManager))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectItBoardActor: Cannot bind board manager — null"));
        return;
    }

    // Wire shift component result to board manager
    if (UBoardShiftComponent* ShiftComp =
        FindComponentByClass<UBoardShiftComponent>())
    {
        ShiftComp->OnShiftResultReady.AddDynamic(
            BoardManager,
            &UConnectItBoardManagerComponent::HandleShiftResult);

        UE_LOG(LogTemp, Log,
            TEXT("ConnectItBoardActor: Shift component wired to board manager"));
    }

    UE_LOG(LogTemp, Log,
        TEXT("ConnectItBoardActor: Board manager wired"));
}

void AConnectItBoardActor::InitialiseBoard(int32 NumFactions)
{
    if (!HasAuthority()) return;

    if (!IsValid(BoardManager))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectItBoardActor: InitialiseBoard called but "
                 "BoardManager is null"));
        return;
    }

    BoardManager->InitialiseBoard(NumFactions);
}

// --- Convenience Accessors ---

UActionLoadOutDataAsset* AConnectItBoardActor::GetPlayerLoadout() const
{
    return IsValid(ConnectItConfig)
        ? ConnectItConfig->PlayerLoadout
        : nullptr;
}

UActionLoadOutDataAsset* AConnectItBoardActor::GetEnemyLoadout() const
{
    return IsValid(ConnectItConfig)
        ? ConnectItConfig->EnemyLoadout
        : nullptr;
}

float AConnectItBoardActor::GetWinScoreThreshold() const
{
    return IsValid(ConnectItConfig)
        ? ConnectItConfig->WinScoreThreshold
        : 100.f;
}