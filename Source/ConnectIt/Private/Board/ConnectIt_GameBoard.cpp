// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/ConnectIt_GameBoard.h"
#include "ConnectIt_GameplayTags.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "TurnBasedMechanicsStructs.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Board/ConnectIt_BoardShiftComponent.h"
#include "Board/Rules/ConnectIt_BoardRulesComponent.h"
#include "Framework/Data/ConnectIt_ConfigComponent.h"
#include "Interpreter/ConnectIt_ScoreInterpreter.h"
#include "Interpreter/ConnectIt_TileStateInterpreter.h"
#include "Piece/GridPieceRegistryBase.h"
#include "Piece/GridPieceSpawnInterpreterBase.h"
#include "Tile/GridTileRegistryBase.h"
#include "GameEvent/ConnectIt_PlacePieceGameEvent.h"


AConnectIt_GameBoard::AConnectIt_GameBoard()
{
    // Generic pieces from ABoardManagerBase -- CreateDefaultSubobject is the
    // correct way to give an Instanced UObject property (not a component) a
    // constructor-time default that still participates correctly in CDO/
    // archetype propagation and Blueprint-child override, same as any other
    // default subobject.
    TileRegistry = CreateDefaultSubobject<UGridTileRegistryBase>(TEXT("TileRegistry"));
    PieceRegistry = CreateDefaultSubobject<UGridPieceRegistryBase>(TEXT("PieceRegistry"));
    PieceSpawnInterpreter = CreateDefaultSubobject<UGridPieceSpawnInterpreterBase>(TEXT("PieceSpawnInterpreter"));

    // ConnectIt-specific pieces -- unchanged shape from AConnectIt_BoardManager,
    // still plain ActorComponents (no new UObject counterpart exists for
    // any of these yet).
    BoardStateComponent =
        CreateDefaultSubobject<UConnectIt_BoardStateComponent>(TEXT("BoardState"));

    ConnectItConfigComponent =
        CreateDefaultSubobject<UConnectIt_ConfigComponent>(TEXT("ConnectItConfig"));

    BoardRulesComponent =
        CreateDefaultSubobject<UConnectIt_BoardRulesComponent>(TEXT("BoardRules"));

    // Fixes the known gap on AConnectIt_BoardManager (declared, never
    // constructed) -- see class header comment.
    BoardShiftComponent =
        CreateDefaultSubobject<UConnectIt_BoardShiftComponent>(TEXT("BoardShift"));

    TileStateInterpreter =
        CreateDefaultSubobject<UConnectIt_TileStateInterpreter>(TEXT("TileStateInterpreter"));

    ScoreInterpreter =
        CreateDefaultSubobject<UConnectIt_ScoreInterpreter>(TEXT("ScoreInterpreter"));

    bReplicates = true;
}

// --- Lifecycle ---

void AConnectIt_GameBoard::BeginPlay()
{
    Super::BeginPlay();

    BindInterpreters();
}

void AConnectIt_GameBoard::CreateGameEventsFromBoardUpdate_Implementation()
{
    if (!IsValid(BoardStateComponent))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameBoard: Cannot bind interpreters "
                 "— BoardStateComponent is null"));
        return;
    }

    FConnectItBoardChangeEvent ChangeEvent = BoardStateComponent->GetChangeEvent();
    if (ChangeEvent.bPiecePlaced)
    {
        // create place piece game event
        // or initialize reusable game event and add to queue
        TurnBasedGameEventQueue.Enqueue(GameEventPlacePiece);
    }

    ExecuteGameEvents();
}

void AConnectIt_GameBoard::ExecuteGameEvents()
{
    // TODO queue wait system to run each game event waiting for it's OnComplete To Fire
    // this is an alternative to the GameEventSubsystem but this will allow us to more clearly
    // group and sequence the visual effects we want from each game event
}

void AConnectIt_GameBoard::BindInterpreters()
{
    if (!IsValid(BoardStateComponent))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameBoard: Cannot bind interpreters "
                 "— BoardStateComponent is null"));
        return;
    }

    if (IsValid(TileStateInterpreter))
    {
        TileStateInterpreter->BindToBoardStateComponent(BoardStateComponent);
    }

    if (IsValid(ScoreInterpreter))
    {
        ScoreInterpreter->BindToBoardStateComponent(BoardStateComponent);
    }

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_GameBoard: Interpreters bound"));
}

// --- Board Lifecycle ---

void AConnectIt_GameBoard::InitialiseBoard(int32 NumFactions)
{
    if (!HasAuthority()) return;

    if (!IsValid(BoardStateComponent))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameBoard: InitialiseBoard — "
                 "BoardStateComponent is null"));
        return;
    }

    if (!IsValid(TileRegistry))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameBoard: InitialiseBoard — "
                 "TileRegistry is null"));
        return;
    }

    const TArray<FGridPosition> TilePositions =
        TileRegistry->GetAllTilePositions();

    if (TilePositions.IsEmpty())
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameBoard: InitialiseBoard — "
                 "No tile positions found in registry. "
                 "Ensure tiles have registered before calling this."));
        return;
    }

    BoardStateComponent->InitialiseBoardState(TilePositions, NumFactions);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_GameBoard: Board initialised — "
             "%d tiles, %d factions"),
        TilePositions.Num(),
        NumFactions);
}

// --- Request Processing ---

bool AConnectIt_GameBoard::ProcessRequest(const FTurnActionRequest& Request)
{
    if (!HasAuthority()) return false;

    if (!Request.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameBoard: Received invalid "
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
            TEXT("ConnectIt_GameBoard: PlacePiece request payload "
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
            TEXT("ConnectIt_GameBoard: Shift request payload "
                 "missing or wrong type"));
        return false;
    }

    if (Request.RequestType == ConnectIt_Game_ForcePlacePiece)
    {
        if (const FConnectItRequestForcePlacePiece* Payload =
            Request.Payload.GetPtr<FConnectItRequestForcePlacePiece>())
        {
            return HandleForcePlacePieceRequest(*Payload, Request.FactionID);
        }

        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameBoard: ForcePlacePiece request payload "
                 "missing or wrong type"));
        return false;
    }

    if (Request.RequestType == ConnectIt_Game_DestroyTileMultiplier)
    {
        if (const FConnectItRequestDestroyTileMultiplier* Payload =
            Request.Payload.GetPtr<FConnectItRequestDestroyTileMultiplier>())
        {
            return HandleDestroyTileMultiplierRequest(*Payload);
        }

        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameBoard: DestroyTileMultiplier request "
                 "payload missing or wrong type"));
        return false;
    }

    if (Request.RequestType == ConnectIt_Game_RemovePiece)
    {
        if (const FConnectItRequestRemovePiece* Payload =
            Request.Payload.GetPtr<FConnectItRequestRemovePiece>())
        {
            return HandleRemovePieceRequest(*Payload);
        }

        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameBoard: RemovePiece request payload "
                 "missing or wrong type"));
        return false;
    }

    if (Request.RequestType == ConnectIt_Game_SwapPieces)
    {
        if (const FConnectItRequestSwapPieces* Payload =
            Request.Payload.GetPtr<FConnectItRequestSwapPieces>())
        {
            return HandleSwapPiecesRequest(*Payload);
        }

        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameBoard: SwapPieces request payload "
                 "missing or wrong type"));
        return false;
    }

    if (Request.RequestType == ConnectIt_Game_ToggleTileActive)
    {
        if (const FConnectItRequestToggleTileActive* Payload =
            Request.Payload.GetPtr<FConnectItRequestToggleTileActive>())
        {
            return HandleToggleTileActiveRequest(*Payload);
        }

        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameBoard: ToggleTileActive request payload "
                 "missing or wrong type"));
        return false;
    }

    if (Request.RequestType == ConnectIt_Game_CapturePiece)
    {
        if (const FConnectItRequestCapturePiece* Payload =
            Request.Payload.GetPtr<FConnectItRequestCapturePiece>())
        {
            return HandleCapturePieceRequest(*Payload, Request.FactionID);
        }

        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameBoard: CapturePiece request payload "
                 "missing or wrong type"));
        return false;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("ConnectIt_GameBoard: Unknown request type '%s'"),
        *Request.RequestType.ToString());
    return false;
}

bool AConnectIt_GameBoard::HandlePlacePieceRequest(
    const FConnectItRequestPlacePiece& Request, int32 FactionID) const
{
    if (Request.Positions.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameBoard: PlacePiece request "
                 "has no positions"));
        return false;
    }

    const FGridPosition TargetPosition = Request.Positions[0];
    const FConnectItBoardState& Current = BoardStateComponent->GetCurrentState();

    if (!Current.IsTileValidForPlacement(TargetPosition))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameBoard: PlacePiece rejected "
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

    TArray<FGridPosition> ScoringPositions;
    const float PointsScored = BoardRulesComponent->ApplyScoring(
        NewState, TargetPosition, FactionID, ScoringPositions);

    BoardRulesComponent->CheckWinCondition(NewState);

    FConnectItBoardChangeEvent ChangeEvent;
    ChangeEvent.bPiecePlaced        = true;
    ChangeEvent.PlacedPosition      = TargetPosition;
    ChangeEvent.PlacingFactionSlot  = FactionID;
    ChangeEvent.bLineScored         = PointsScored > 0.f;
    ChangeEvent.ScoringFactionSlot  = FactionID;
    ChangeEvent.PointsScored        = PointsScored;
    ChangeEvent.ScoringLinePositions = ScoringPositions;
    ChangeEvent.bGameWon            = NewState.bGameOver && !Current.bGameOver;
    ChangeEvent.WinningFactionSlot  = NewState.WinningFactionSlot;

    if (ChangeEvent.bLineScored)
    {
        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_GameBoard: Faction %d scored %.0f points"),
            FactionID, PointsScored);
    }

    BoardStateComponent->SetBoardState(NewState, ChangeEvent);
    return true;
}

bool AConnectIt_GameBoard::HandleShiftRequest(
    const FConnectItRequestBoardShift& Request, int32 FactionID) const
{
    if (!IsValid(BoardShiftComponent))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameBoard: HandleShiftRequest — "
                 "BoardShiftComponent is null"));
        return false;
    }

    FConnectItBoardState NewState = BoardStateComponent->GetCurrentState();
    FShiftResult ShiftResult;
    if (!BoardShiftComponent->ComputeShift(
        NewState, ShiftResult, Request.ShiftOperation))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameBoard: Shift rejected — "
                 "ComputeShift returned an invalid result"));
        return false;
    }

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

bool AConnectIt_GameBoard::HandleForcePlacePieceRequest(
    const FConnectItRequestForcePlacePiece& Request, int32 FactionID) const
{
    const FConnectItBoardState& Current = BoardStateComponent->GetCurrentState();

    if (!Current.GetTileData(Request.Position))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameBoard: ForcePlacePiece rejected "
                 "-- position (%d,%d) not registered"),
            Request.Position.X, Request.Position.Y);
        return false;
    }

    FConnectItBoardState NewState = Current;

    if (FConnectItTileData* TileData = NewState.GetTileDataMutable(Request.Position))
    {
        TileData->FactionPiece = FactionID;
    }

    TArray<FGridPosition> ScoringPositions;
    const float PointsScored = BoardRulesComponent->ApplyScoring(
        NewState, Request.Position, FactionID, ScoringPositions);

    BoardRulesComponent->CheckWinCondition(NewState);

    FConnectItBoardChangeEvent ChangeEvent;
    ChangeEvent.bPiecePlaced       = true;
    ChangeEvent.PlacedPosition     = Request.Position;
    ChangeEvent.PlacingFactionSlot = FactionID;
    ChangeEvent.bLineScored        = PointsScored > 0.f;
    ChangeEvent.ScoringFactionSlot = FactionID;
    ChangeEvent.PointsScored       = PointsScored;
    ChangeEvent.ScoringLinePositions = ScoringPositions;
    ChangeEvent.bGameWon           = NewState.bGameOver && !Current.bGameOver;
    ChangeEvent.WinningFactionSlot = NewState.WinningFactionSlot;

    BoardStateComponent->SetBoardState(NewState, ChangeEvent);
    return true;
}

bool AConnectIt_GameBoard::HandleDestroyTileMultiplierRequest(
    const FConnectItRequestDestroyTileMultiplier& Request) const
{
    const FConnectItBoardState& Current = BoardStateComponent->GetCurrentState();
    const FConnectItTileData* Existing = Current.GetTileData(Request.Position);

    if (!Existing)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameBoard: DestroyTileMultiplier rejected "
                 "-- position (%d,%d) not registered"),
            Request.Position.X, Request.Position.Y);
        return false;
    }

    if (Existing->Multiplier == 1.0f)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameBoard: DestroyTileMultiplier rejected "
                 "-- position (%d,%d) has no multiplier to destroy"),
            Request.Position.X, Request.Position.Y);
        return false;
    }

    FConnectItBoardState NewState = Current;

    if (FConnectItTileData* TileData = NewState.GetTileDataMutable(Request.Position))
    {
        TileData->Multiplier = 1.0f;
    }

    FConnectItBoardChangeEvent ChangeEvent;
    ChangeEvent.bTileMultiplierDestroyed   = true;
    ChangeEvent.MultiplierDestroyedPosition = Request.Position;

    BoardStateComponent->SetBoardState(NewState, ChangeEvent);
    return true;
}

bool AConnectIt_GameBoard::HandleRemovePieceRequest(
    const FConnectItRequestRemovePiece& Request) const
{
    if (Request.DelayTurns > 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameBoard: RemovePiece rejected -- "
                 "DelayTurns %d not supported yet (no per-turn scheduling "
                 "mechanism exists), only immediate (0) removal is handled"),
            Request.DelayTurns);
        return false;
    }

    const FConnectItBoardState& Current = BoardStateComponent->GetCurrentState();
    const FConnectItTileData* Existing = Current.GetTileData(Request.Position);

    if (!Existing || !Existing->IsOccupied())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameBoard: RemovePiece rejected -- "
                 "position (%d,%d) is not occupied"),
            Request.Position.X, Request.Position.Y);
        return false;
    }

    const int32 RemovedFactionSlot = Existing->FactionPiece;

    FConnectItBoardState NewState = Current;

    if (FConnectItTileData* TileData = NewState.GetTileDataMutable(Request.Position))
    {
        TileData->FactionPiece = -1;
    }

    FConnectItBoardChangeEvent ChangeEvent;
    ChangeEvent.bPieceRemoved      = true;
    ChangeEvent.RemovedPosition    = Request.Position;
    ChangeEvent.RemovedFactionSlot = RemovedFactionSlot;

    BoardStateComponent->SetBoardState(NewState, ChangeEvent);
    return true;
}

bool AConnectIt_GameBoard::HandleSwapPiecesRequest(
    const FConnectItRequestSwapPieces& Request) const
{
    const FConnectItBoardState& Current = BoardStateComponent->GetCurrentState();
    const FConnectItTileData* DataA = Current.GetTileData(Request.PositionA);
    const FConnectItTileData* DataB = Current.GetTileData(Request.PositionB);

    if (!DataA || !DataB || !DataA->IsOccupied() || !DataB->IsOccupied())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameBoard: SwapPieces rejected -- "
                 "both (%d,%d) and (%d,%d) must be registered and occupied"),
            Request.PositionA.X, Request.PositionA.Y,
            Request.PositionB.X, Request.PositionB.Y);
        return false;
    }

    FConnectItBoardState NewState = Current;

    FConnectItTileData* MutableA = NewState.GetTileDataMutable(Request.PositionA);
    FConnectItTileData* MutableB = NewState.GetTileDataMutable(Request.PositionB);
    Swap(MutableA->FactionPiece, MutableB->FactionPiece);

    FConnectItBoardChangeEvent ChangeEvent;
    ChangeEvent.bPiecesSwapped = true;
    ChangeEvent.SwapPositionA  = Request.PositionA;
    ChangeEvent.SwapPositionB  = Request.PositionB;

    BoardStateComponent->SetBoardState(NewState, ChangeEvent);
    return true;
}

bool AConnectIt_GameBoard::HandleToggleTileActiveRequest(
    const FConnectItRequestToggleTileActive& Request) const
{
    const FConnectItBoardState& Current = BoardStateComponent->GetCurrentState();
    const FConnectItTileData* Existing = Current.GetTileData(Request.Position);

    if (!Existing)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameBoard: ToggleTileActive rejected -- "
                 "position (%d,%d) not registered"),
            Request.Position.X, Request.Position.Y);
        return false;
    }

    FConnectItBoardState NewState = Current;
    bool bNewActiveState = false;

    if (FConnectItTileData* TileData = NewState.GetTileDataMutable(Request.Position))
    {
        TileData->bIsActive = !TileData->bIsActive;
        bNewActiveState = TileData->bIsActive;
    }

    FConnectItBoardChangeEvent ChangeEvent;
    ChangeEvent.bTileActiveToggled       = true;
    ChangeEvent.ToggledPosition          = Request.Position;
    ChangeEvent.bToggledPositionNowActive = bNewActiveState;

    BoardStateComponent->SetBoardState(NewState, ChangeEvent);
    return true;
}

bool AConnectIt_GameBoard::HandleCapturePieceRequest(
    const FConnectItRequestCapturePiece& Request, int32 FactionID) const
{
    const FConnectItBoardState& Current = BoardStateComponent->GetCurrentState();
    const FConnectItTileData* Existing = Current.GetTileData(Request.Position);

    if (!Existing || !Existing->IsOccupied())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameBoard: CapturePiece rejected -- "
                 "position (%d,%d) is not occupied"),
            Request.Position.X, Request.Position.Y);
        return false;
    }

    if (Existing->FactionPiece == FactionID)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameBoard: CapturePiece rejected -- "
                 "position (%d,%d) already belongs to faction %d"),
            Request.Position.X, Request.Position.Y, FactionID);
        return false;
    }

    const int32 PreviousFactionSlot = Existing->FactionPiece;

    FConnectItBoardState NewState = Current;

    if (FConnectItTileData* TileData = NewState.GetTileDataMutable(Request.Position))
    {
        TileData->FactionPiece = FactionID;
    }

    TArray<FGridPosition> ScoringPositions;
    const float PointsScored = BoardRulesComponent->ApplyScoring(
        NewState, Request.Position, FactionID, ScoringPositions);

    BoardRulesComponent->CheckWinCondition(NewState);

    FConnectItBoardChangeEvent ChangeEvent;
    ChangeEvent.bPieceCaptured        = true;
    ChangeEvent.CapturedPosition      = Request.Position;
    ChangeEvent.CapturingFactionSlot  = FactionID;
    ChangeEvent.PreviousFactionSlot   = PreviousFactionSlot;
    ChangeEvent.bLineScored           = PointsScored > 0.f;
    ChangeEvent.ScoringFactionSlot    = FactionID;
    ChangeEvent.PointsScored          = PointsScored;
    ChangeEvent.ScoringLinePositions  = ScoringPositions;
    ChangeEvent.bGameWon              = NewState.bGameOver && !Current.bGameOver;
    ChangeEvent.WinningFactionSlot    = NewState.WinningFactionSlot;

    BoardStateComponent->SetBoardState(NewState, ChangeEvent);
    return true;
}
