// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/ConnectIt_BoardRequestMediator.h"
#include "ConnectIt_GameplayTags.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "TurnBasedMechanicsStructs.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Board/Rules/ConnectIt_BoardRules.h"
#include "Framework/GameState/ConnectIt_GameState.h"
#include "GameEvent/ConnectIt_PlacePieceGameEvent.h"


void UConnectIt_BoardRequestMediator::Initialise(UConnectIt_BoardRules* InBoardRules)
{
    BoardRules = InBoardRules;
}

UConnectIt_BoardStateComponent* UConnectIt_BoardRequestMediator::GetBoardState() const
{
    const AConnectIt_GameState* GS = GetWorld() ? GetWorld()->GetGameState<AConnectIt_GameState>() : nullptr;
    return IsValid(GS) ? GS->GetBoardStateComponent() : nullptr;
}

void UConnectIt_BoardRequestMediator::CreateGameEventsFromBoardUpdate_Implementation()
{
    UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    if (!IsValid(BoardState))
    {
        UE_LOG(LogTemp, Error, TEXT(
            "UConnectIt_BoardRequestMediator::CreateGameEventsFromBoardUpdate — BoardStateComponent is null"));
        return;
    }

    FConnectItBoardChangeEvent ChangeEvent = BoardState->GetChangeEvent();
    if (ChangeEvent.bPiecePlaced)
    {
        // create place piece game event
        // or initialize reusable game event and add to queue
        TurnBasedGameEventQueue.Enqueue(GameEventPlacePiece);
    }

    ExecuteGameEvents();
}

void UConnectIt_BoardRequestMediator::ExecuteGameEvents()
{
    // TODO queue wait system to run each game event waiting for it's OnComplete To Fire
    // this is an alternative to the GameEventSubsystem but this will allow us to more clearly
    // group and sequence the visual effects we want from each game event
}

// --- Request Processing ---

bool UConnectIt_BoardRequestMediator::ProcessRequest(const FTurnActionRequest& Request)
{
    UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    if (!IsValid(BoardState))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardRequestMediator: ProcessRequest — "
                 "BoardStateComponent is null"));
        return false;
    }

    // Universal choke point regardless of caller (player-controller RPC or
    // AI controller's direct server-side call) -- once the game is over,
    // no further board mutation is possible, full stop.
    if (BoardState->GetCurrentState().bGameOver)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: ProcessRequest rejected — "
                 "game already over"));
        return false;
    }

    if (!Request.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: Received invalid "
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
            TEXT("ConnectIt_BoardRequestMediator: PlacePiece request payload "
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
            TEXT("ConnectIt_BoardRequestMediator: ForcePlacePiece request payload "
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
            TEXT("ConnectIt_BoardRequestMediator: DestroyTileMultiplier request "
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
            TEXT("ConnectIt_BoardRequestMediator: RemovePiece request payload "
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
            TEXT("ConnectIt_BoardRequestMediator: SwapPieces request payload "
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
            TEXT("ConnectIt_BoardRequestMediator: ToggleTileActive request payload "
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
            TEXT("ConnectIt_BoardRequestMediator: CapturePiece request payload "
                 "missing or wrong type"));
        return false;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("ConnectIt_BoardRequestMediator: Unknown request type '%s'"),
        *Request.RequestType.ToString());
    return false;
}

bool UConnectIt_BoardRequestMediator::HandlePlacePieceRequest(
    const FConnectItRequestPlacePiece& Request, int32 FactionID) const
{
    if (Request.Positions.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: PlacePiece request "
                 "has no positions"));
        return false;
    }

    UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    const FGridPosition TargetPosition = Request.Positions[0];
    const FConnectItBoardState& Current = BoardState->GetCurrentState();

    if (!Current.IsTileValidForPlacement(TargetPosition))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: PlacePiece rejected "
                 "— position (%d,%d) invalid for placement"),
            TargetPosition.X, TargetPosition.Y);
        return false;
    }

    FConnectItBoardState NewState = Current;

    if (FConnectItTileData* TileData =
        NewState.GetTileDataMutable(TargetPosition))
    {
        TileData->SetFactionPiece(FactionID);
    }

    TArray<FGridPosition> ScoringPositions;
    const float PointsScored = BoardRules->ApplyScoring(
        NewState, TargetPosition, FactionID, ScoringPositions);

    BoardRules->CheckWinCondition(NewState);

    // Record what happened -- replicated alongside the state itself via
    // SetBoardState, instead of broadcasting gameplay delegates directly
    // here. This only ever runs on the server, so a direct broadcast would
    // never reach a real remote client. ConnectIt_BoardStateComponent reads
    // this back from its own BoardSnapshot.ChangeEvent and enqueues the
    // matching event tags on UGameEventTaskSubsystem itself, symmetrically
    // on both server (from SetBoardState) and client (from OnRep) -- this
    // mediator plays no role in sequencing, only in deciding what happened.
    FConnectItBoardChangeEvent ChangeEvent;
    ChangeEvent.bPiecePlaced        = true;
    ChangeEvent.PlacedPosition      = TargetPosition;
    ChangeEvent.PlacingFactionSlot  = FactionID;
    ChangeEvent.bLineScored         = PointsScored > 0.f;
    ChangeEvent.ScoringFactionSlot  = FactionID;
    ChangeEvent.PointsScored        = PointsScored;
    ChangeEvent.ScoringLinePositions = ScoringPositions;
    // Edge-triggered -- true only on the transition into game-over, not
    // "the game is currently over" (Current.bGameOver would already be
    // true on every snapshot after the winning move)
    ChangeEvent.bGameWon            = NewState.bGameOver && !Current.bGameOver;
    ChangeEvent.WinningFactionSlot  = NewState.WinningFactionSlot;

    if (ChangeEvent.bLineScored)
    {
        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_BoardRequestMediator: Faction %d scored %.0f points"),
            FactionID, PointsScored);
    }

    BoardState->SetBoardState(NewState, ChangeEvent);
    return true;
}

bool UConnectIt_BoardRequestMediator::HandleForcePlacePieceRequest(
    const FConnectItRequestForcePlacePiece& Request, int32 FactionID) const
{
    UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    const FConnectItBoardState& Current = BoardState->GetCurrentState();

    // Only requires the position to exist -- deliberately skips
    // IsTileValidForPlacement (active/unoccupied) so this can overwrite an
    // inactive or already-occupied tile. Registry existence is still
    // checked so a garbage position can't silently grow the tile arrays.
    if (!Current.GetTileData(Request.Position))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: ForcePlacePiece rejected "
                 "-- position (%d,%d) not registered"),
            Request.Position.X, Request.Position.Y);
        return false;
    }

    FConnectItBoardState NewState = Current;

    if (FConnectItTileData* TileData = NewState.GetTileDataMutable(Request.Position))
    {
        TileData->SetFactionPiece(FactionID);
    }

    TArray<FGridPosition> ScoringPositions;
    const float PointsScored = BoardRules->ApplyScoring(
        NewState, Request.Position, FactionID, ScoringPositions);

    BoardRules->CheckWinCondition(NewState);

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

    BoardState->SetBoardState(NewState, ChangeEvent);
    return true;
}

bool UConnectIt_BoardRequestMediator::HandleDestroyTileMultiplierRequest(
    const FConnectItRequestDestroyTileMultiplier& Request) const
{
    UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    const FConnectItBoardState& Current = BoardState->GetCurrentState();
    const FConnectItTileData* Existing = Current.GetTileData(Request.Position);

    if (!Existing)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: DestroyTileMultiplier rejected "
                 "-- position (%d,%d) not registered"),
            Request.Position.X, Request.Position.Y);
        return false;
    }

    if (Existing->Multiplier == 1.0f)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: DestroyTileMultiplier rejected "
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

    BoardState->SetBoardState(NewState, ChangeEvent);
    return true;
}

bool UConnectIt_BoardRequestMediator::HandleRemovePieceRequest(
    const FConnectItRequestRemovePiece& Request) const
{
    if (Request.DelayTurns > 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: RemovePiece rejected -- "
                 "DelayTurns %d not supported yet (no per-turn scheduling "
                 "mechanism exists), only immediate (0) removal is handled"),
            Request.DelayTurns);
        return false;
    }

    UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    const FConnectItBoardState& Current = BoardState->GetCurrentState();
    const FConnectItTileData* Existing = Current.GetTileData(Request.Position);

    if (!Existing || !Existing->bIsOccupied)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: RemovePiece rejected -- "
                 "position (%d,%d) is not occupied"),
            Request.Position.X, Request.Position.Y);
        return false;
    }

    const int32 RemovedFactionSlot = Existing->FactionPiece;

    FConnectItBoardState NewState = Current;

    if (FConnectItTileData* TileData = NewState.GetTileDataMutable(Request.Position))
    {
        TileData->SetFactionPiece(-1);
    }

    FConnectItBoardChangeEvent ChangeEvent;
    ChangeEvent.bPieceRemoved      = true;
    ChangeEvent.RemovedPosition    = Request.Position;
    ChangeEvent.RemovedFactionSlot = RemovedFactionSlot;

    BoardState->SetBoardState(NewState, ChangeEvent);
    return true;
}

bool UConnectIt_BoardRequestMediator::HandleSwapPiecesRequest(
    const FConnectItRequestSwapPieces& Request) const
{
    UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    const FConnectItBoardState& Current = BoardState->GetCurrentState();
    const FConnectItTileData* DataA = Current.GetTileData(Request.PositionA);
    const FConnectItTileData* DataB = Current.GetTileData(Request.PositionB);

    if (!DataA || !DataB || !DataA->bIsOccupied || !DataB->bIsOccupied)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: SwapPieces rejected -- "
                 "both (%d,%d) and (%d,%d) must be registered and occupied"),
            Request.PositionA.X, Request.PositionA.Y,
            Request.PositionB.X, Request.PositionB.Y);
        return false;
    }

    FConnectItBoardState NewState = Current;

    FConnectItTileData* MutableA = NewState.GetTileDataMutable(Request.PositionA);
    FConnectItTileData* MutableB = NewState.GetTileDataMutable(Request.PositionB);
    int32 A = MutableA->FactionPiece;
    int32 B = MutableB->FactionPiece;
    MutableA->SetFactionPiece(B);
    MutableB->SetFactionPiece(A);

    // No scoring/win-condition re-check -- see class header comment
    FConnectItBoardChangeEvent ChangeEvent;
    ChangeEvent.bPiecesSwapped = true;
    ChangeEvent.SwapPositionA  = Request.PositionA;
    ChangeEvent.SwapPositionB  = Request.PositionB;

    BoardState->SetBoardState(NewState, ChangeEvent);
    return true;
}

bool UConnectIt_BoardRequestMediator::HandleToggleTileActiveRequest(
    const FConnectItRequestToggleTileActive& Request) const
{
    UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    const FConnectItBoardState& Current = BoardState->GetCurrentState();
    const FConnectItTileData* Existing = Current.GetTileData(Request.Position);

    if (!Existing)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: ToggleTileActive rejected -- "
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

    BoardState->SetBoardState(NewState, ChangeEvent);
    return true;
}

bool UConnectIt_BoardRequestMediator::HandleCapturePieceRequest(
    const FConnectItRequestCapturePiece& Request, int32 FactionID) const
{
    UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    const FConnectItBoardState& Current = BoardState->GetCurrentState();
    const FConnectItTileData* Existing = Current.GetTileData(Request.Position);

    if (!Existing || !Existing->bIsOccupied)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: CapturePiece rejected -- "
                 "position (%d,%d) is not occupied"),
            Request.Position.X, Request.Position.Y);
        return false;
    }

    if (Existing->FactionPiece == FactionID)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: CapturePiece rejected -- "
                 "position (%d,%d) already belongs to faction %d"),
            Request.Position.X, Request.Position.Y, FactionID);
        return false;
    }

    const int32 PreviousFactionSlot = Existing->FactionPiece;

    FConnectItBoardState NewState = Current;

    if (FConnectItTileData* TileData = NewState.GetTileDataMutable(Request.Position))
    {
        TileData->SetFactionPiece(FactionID);
    }

    // Exactly one position changed ownership -- same well-defined case
    // HandlePlacePieceRequest already handles, unlike HandleSwapPiecesRequest
    TArray<FGridPosition> ScoringPositions;
    const float PointsScored = BoardRules->ApplyScoring(
        NewState, Request.Position, FactionID, ScoringPositions);

    BoardRules->CheckWinCondition(NewState);

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

    BoardState->SetBoardState(NewState, ChangeEvent);
    return true;
}
