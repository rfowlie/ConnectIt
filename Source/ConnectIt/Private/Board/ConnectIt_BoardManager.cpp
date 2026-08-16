// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/ConnectIt_BoardManager.h"

#include "ConnectIt_GameplayTags.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "TurnBasedMechanicsStructs.h"
#include "Action/ConnectIt_ShiftAction.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Board/Rules/ConnectIt_BoardRulesComponent.h"
#include "Framework/Data/ConnectIt_ConfigComponent.h"
#include "Framework/Subsystem/ConnectIt_BoardManagerSubsystem.h"
#include "Interpreter/ConnectIt_PieceSpawnInterpreter.h"
#include "Interpreter/ConnectIt_ScoreInterpreter.h"
#include "Interpreter/ConnectIt_TileStateInterpreter.h"
#include "Board/ConnectIt_BoardShiftComponent.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"
#include "Piece/GridPieceRegistryComponent.h"
#include "Tile/GridTileRegistryComponent.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"


AConnectIt_BoardManager::AConnectIt_BoardManager()
{
    TileRegistryComponent =
        CreateDefaultSubobject<UGridTileRegistryComponent>(
            TEXT("TileRegistry"));

    PieceRegistryComponent =
        CreateDefaultSubobject<UGridPieceRegistryComponent>(
            TEXT("PieceRegistry"));

    BoardStateComponent =
        CreateDefaultSubobject<UConnectIt_BoardStateComponent>(
            TEXT("BoardState"));

    ConnectItConfigComponent =
        CreateDefaultSubobject<UConnectIt_ConfigComponent>(
            TEXT("ConnectItConfig"));

    BoardRulesComponent =
        CreateDefaultSubobject<UConnectIt_BoardRulesComponent>(
            TEXT("BoardRules"));

    TileStateInterpreter =
        CreateDefaultSubobject<UConnectIt_TileStateInterpreter>(
            TEXT("TileStateInterpreter"));

    PieceSpawnInterpreter =
        CreateDefaultSubobject<UConnectIt_PieceSpawnInterpreter>(
            TEXT("PieceSpawnInterpreter"));

    ScoreInterpreter =
        CreateDefaultSubobject<UConnectIt_ScoreInterpreter>(
            TEXT("ScoreInterpreter"));

    bReplicates = true;
}

// --- Lifecycle ---

void AConnectIt_BoardManager::BeginPlay()
{
    Super::BeginPlay();

    BindInterpreters();

    // Populate the per-world board manager cache -- runs on both server
    // and client, since most GetBoardManager call sites are client-side
    if (UConnectIt_BoardManagerSubsystem* BoardManagerSubsystem =
        GetWorld()->GetSubsystem<UConnectIt_BoardManagerSubsystem>())
    {
        BoardManagerSubsystem->RegisterBoardManager(this);
    }

    if (HasAuthority())
    {
        BindParticipantManager();
    }
}

void AConnectIt_BoardManager::HandleActiveControllerChanged(AController* NewActiveController)
{
    if (!HasAuthority()) return;

    SetOwner(NewActiveController);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_BoardManager: Ownership transferred to %s"),
        IsValid(NewActiveController)
            ? *NewActiveController->GetName()
            : TEXT("none"));
}

void AConnectIt_BoardManager::BindInterpreters()
{
    if (!IsValid(BoardStateComponent))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardManager: Cannot bind interpreters "
                 "— BoardStateComponent is null"));
        return;
    }

    if (IsValid(TileStateInterpreter))
    {
        TileStateInterpreter->BindToBoardStateComponent(BoardStateComponent);
    }

    if (IsValid(PieceSpawnInterpreter))
    {
        PieceSpawnInterpreter->BindToBoardStateComponent(BoardStateComponent);
    }

    if (IsValid(ScoreInterpreter))
    {
        ScoreInterpreter->BindToBoardStateComponent(BoardStateComponent);
    }

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_BoardManager: Interpreters bound"));
}

// --- Board Lifecycle ---

void AConnectIt_BoardManager::InitialiseBoard(int32 NumFactions)
{
    if (!HasAuthority()) return;

    if (!IsValid(BoardStateComponent))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardManager: InitialiseBoard — "
                 "BoardStateComponent is null"));
        return;
    }

    if (!IsValid(TileRegistryComponent))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardManager: InitialiseBoard — "
                 "TileRegistryComponent is null"));
        return;
    }

    const TArray<FGridPosition> TilePositions =
        TileRegistryComponent->GetAllTilePositions();

    if (TilePositions.IsEmpty())
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardManager: InitialiseBoard — "
                 "No tile positions found in registry. "
                 "Ensure tiles have registered before calling this."));
        return;
    }

    BoardStateComponent->InitialiseBoardState(TilePositions, NumFactions);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_BoardManager: Board initialised — "
             "%d tiles, %d factions"),
        TilePositions.Num(),
        NumFactions);
}

// --- Request Processing ---

// board needs to map out how to handle each request and transform payloads...
bool AConnectIt_BoardManager::ProcessRequest(const FTurnActionRequest& Request)
{
    if (!HasAuthority()) return false;

    if (!Request.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardManager: Received invalid "
                 "FTurnActionRequest"));
        return false;
    }

    if (Request.RequestType == ConnectIt_Game_PlacePiece)
    {
        if (const FConnectItRequestPlacePiece* Payload =
            Request.Payload.GetPtr<FConnectItRequestPlacePiece>())
        {
            return HandlePlacePieceRequest(*Payload, Request.FactionID);
        }

        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardManager: PlacePiece request payload "
                 "missing or wrong type"));
        return false;
    }

    if (Request.RequestType == ConnectIt_Game_Shift)
    {
        if (const FConnectItRequestBoardShift* Payload =
            Request.Payload.GetPtr<FConnectItRequestBoardShift>())
        {
            return HandleShiftRequest(*Payload, Request.FactionID);
        }

        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardManager: Shift request payload "
                 "missing or wrong type"));
        return false;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("ConnectIt_BoardManager: Unknown request type '%s'"),
        *Request.RequestType.ToString());
    return false;
}

bool AConnectIt_BoardManager::HandlePlacePieceRequest(
    const FConnectItRequestPlacePiece& Request, int32 FactionID) const
{
    if (Request.Positions.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardManager: PlacePiece request "
                 "has no positions"));
        return false;
    }

    const FGridPosition TargetPosition = Request.Positions[0];
    const FConnectItBoardState& Current = BoardStateComponent->GetCurrentState();

    if (!Current.IsTileValidForPlacement(TargetPosition))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardManager: PlacePiece rejected "
                 "— position (%d,%d) invalid for placement"),
            TargetPosition.X, TargetPosition.Y);
        return false;
    }

    FConnectItBoardState NewState = Current;

    if (FConnectItTileData* TileData =
        NewState.GetTileDataMutable(TargetPosition))
    {
        TileData->FactionPiece = FactionID;
    }

    const float PointsScored = BoardRulesComponent->ApplyScoring(
        NewState, TargetPosition, FactionID);

    BoardRulesComponent->CheckWinCondition(NewState);

    // Record what happened -- replicated alongside the state itself via
    // SetBoardState, instead of broadcasting gameplay delegates directly
    // here. This only ever runs on the server, so a direct broadcast would
    // never reach a real remote client. ConnectIt_BoardStateComponent reads
    // this back from its own BoardSnapshot.ChangeEvent and enqueues the
    // matching event tags on UGameEventTaskSubsystem itself, symmetrically
    // on both server (from SetBoardState) and client (from OnRep) -- this
    // board manager plays no role in sequencing, only in deciding what
    // happened.
    FConnectItBoardChangeEvent ChangeEvent;
    ChangeEvent.bPiecePlaced        = true;
    ChangeEvent.PlacedPosition      = TargetPosition;
    ChangeEvent.PlacingFactionSlot  = FactionID;
    ChangeEvent.bLineScored         = PointsScored > 0.f;
    ChangeEvent.ScoringFactionSlot  = FactionID;
    ChangeEvent.PointsScored        = PointsScored;
    // Edge-triggered -- true only on the transition into game-over, not
    // "the game is currently over" (Current.bGameOver would already be
    // true on every snapshot after the winning move)
    ChangeEvent.bGameWon            = NewState.bGameOver && !Current.bGameOver;
    ChangeEvent.WinningFactionSlot  = NewState.WinningFactionSlot;

    if (ChangeEvent.bLineScored)
    {
        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_BoardManager: Faction %d scored %.0f points"),
            FactionID, PointsScored);
    }

    BoardStateComponent->SetBoardState(NewState, ChangeEvent);
    return true;
}

bool AConnectIt_BoardManager::HandleShiftRequest(
    const FConnectItRequestBoardShift& Request, int32 FactionID) const
{
    if (!IsValid(BoardShiftComponent))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardManager: HandleShiftRequest — "
                 "ShiftStateComponent is null"));
        return false;
    }

    FConnectItBoardState NewState = BoardStateComponent->GetCurrentState();
    FShiftResult ShiftResult;
    if (!BoardShiftComponent->ComputeShift(
        NewState, ShiftResult, Request.ShiftOperation))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardManager: Shift rejected — "
                 "ComputeShift returned an invalid result"));
        return false;
    }

    // Record what happened -- same authoritative-first pattern as
    // HandlePlacePieceRequest: commit state via SetBoardState first, then
    // ConnectIt_BoardStateComponent (fires identically on server and
    // client) enqueues the gated visual sequence from the replicated
    // ChangeEvent. Parallel arrays instead of Result's TMap/TSet directly --
    // those don't replicate (see FConnectItBoardChangeEvent's Shift fields).
    FConnectItBoardChangeEvent ChangeEvent;
    ChangeEvent.bShiftApplied = true;
    ChangeEvent.ShiftOperation = Request.ShiftOperation;
    ShiftResult.PositionRemap.GenerateKeyArray(ChangeEvent.ShiftFromPositions);
    for (const FGridPosition& FromPos : ChangeEvent.ShiftFromPositions)
    {
        ChangeEvent.ShiftToPositions.Add(ShiftResult.PositionRemap[FromPos]);
    }
    
    ChangeEvent.ShiftWrappingPositions = ShiftResult.WrappingPositions.Array();

    BoardStateComponent->SetBoardState(NewState, ChangeEvent);
    return true;
}

void AConnectIt_BoardManager::BindParticipantManager()
{
    ParticipantManagerRef = UConnectIt_GameUtilityLibrary::GetParticipantManager(this);

    if (IsValid(ParticipantManagerRef))
    {
        ParticipantManagerRef->OnActiveControllerChanged.AddDynamic(
            this, &AConnectIt_BoardManager::HandleActiveControllerChanged);
    }
}
