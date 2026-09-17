// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/ConnectIt_BoardRequestMediator.h"
#include "ConnectIt_GameplayTags.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "TurnBasedMechanicsStructs.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Board/Rules/ConnectIt_BoardRules.h"
#include "Framework/GameState/ConnectIt_GameState.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "Framework/PlayerState/ConnectIt_PlayerState.h"
#include "GameEvent/ConnectIt_PlacePieceGameEvent.h"


void UConnectIt_BoardRequestMediator::Initialise(UConnectIt_BoardRules* InBoardRules)
{
    BoardRules = InBoardRules;
}

// UConnectIt_BoardStateComponent* UConnectIt_BoardRequestMediator::GetBoardState() const
// {
//     const AConnectIt_GameState* GS = GetWorld() ? GetWorld()->GetGameState<AConnectIt_GameState>() : nullptr;
//     return IsValid(GS) ? GS->GetBoardStateComponent() : nullptr;
// }

void UConnectIt_BoardRequestMediator::CreateGameEventsFromBoardUpdate_Implementation()
{
    // UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    const UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
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
    // UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    const UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
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
            return HandleSwapPiecesRequest(*Payload, Request.FactionID);
        }

        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardRequestMediator: SwapPieces request payload "
                 "missing or wrong type"));
        return false;
    }

    if (Request.RequestType == ConnectIt_Game_Shift)
    {
        if (const FConnectItRequestBoardShift* Payload =
            Request.Payload.GetPtr<FConnectItRequestBoardShift>())
        {
            return HandleBoardShiftRequest(*Payload, Request.FactionID);
        }

        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardRequestMediator: BoardShift request payload "
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

    // UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    const FGridPosition TargetPosition = Request.Positions[0];
    const FConnectItBoardState& Current = BoardState->GetCurrentState();

    if (!BoardRules->IsTilePlaceable(Current, TargetPosition))
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

    // check scoring achieved (updates NewState)
    TArray<FGridPosition> ScoringPositions;
    const float PointsScored = BoardRules->ApplyScoring(
        NewState, TargetPosition, FactionID, ScoringPositions);

    // check win achieved
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
    // UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
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
    // UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
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

    // UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
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
    const FConnectItRequestSwapPieces& Request, const int32 FactionID) const
{
    // Server-authoritative use-budget check -- the client-side action's own
    // pre-check (CanActivate-equivalent) is cosmetic only, this is the real
    // gate. Never trust a client to have already enforced this.
    AConnectIt_PlayerState* PlayerState =
        UConnectIt_GameUtilityLibrary::GetPlayerStateForFaction(this, FactionID);
    if (!IsValid(PlayerState) || PlayerState->GetSwapActionUsesRemaining() <= 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: SwapPieces rejected -- "
                 "faction %d has no SWAP uses remaining"),
            FactionID);
        return false;
    }
    
    // UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
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

    // A trade, not an arbitrary reposition -- exactly one side must belong
    // to the requesting faction.
    const bool bOwnsA = DataA->FactionPiece == FactionID;
    const bool bOwnsB = DataB->FactionPiece == FactionID;
    if (bOwnsA == bOwnsB)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: SwapPieces rejected -- "
                 "faction %d must own exactly one of (%d,%d)/(%d,%d)"),
            FactionID, Request.PositionA.X, Request.PositionA.Y,
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

    // Re-run scoring for both positions' new occupying faction -- a swap
    // that completes a line now scores like any other turn-ending move
    // (previously a documented, deliberate gap). Evaluated independently
    // per position since A and B now belong to different factions by
    // construction (see the ownership check above); ScoringLinePositions
    // accumulates both calls' results (ApplyScoring appends, never clears).
    // Known simplification: FConnectItBoardChangeEvent only carries a
    // single ScoringFactionSlot, so the rare case of BOTH positions
    // completing a line in the same swap can only name one of the two
    // scoring factions in the event -- ScoreBoard itself (mutated inside
    // ApplyScoring) is correct for both regardless.
    TArray<FGridPosition> ScoringPositions;
    const float PointsScoredA = BoardRules->ApplyScoring(
        NewState, Request.PositionA, B, ScoringPositions);
    const float PointsScoredB = BoardRules->ApplyScoring(
        NewState, Request.PositionB, A, ScoringPositions);

    BoardRules->CheckWinCondition(NewState);

    FConnectItBoardChangeEvent ChangeEvent;
    ChangeEvent.bPiecesSwapped      = true;
    ChangeEvent.SwapPositionA       = Request.PositionA;
    ChangeEvent.SwapPositionB       = Request.PositionB;
    ChangeEvent.bLineScored         = PointsScoredA > 0.f || PointsScoredB > 0.f;
    ChangeEvent.ScoringFactionSlot  = PointsScoredA > 0.f ? B : A;
    ChangeEvent.PointsScored        = PointsScoredA + PointsScoredB;
    ChangeEvent.ScoringLinePositions = ScoringPositions;
    ChangeEvent.bGameWon            = NewState.bGameOver && !Current.bGameOver;
    ChangeEvent.WinningFactionSlot  = NewState.WinningFactionSlot;

    if (ChangeEvent.bLineScored)
    {
        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_BoardRequestMediator: Swap scored — "
                 "faction %d: %.0f, faction %d: %.0f"),
            B, PointsScoredA, A, PointsScoredB);
    }

    BoardState->SetBoardState(NewState, ChangeEvent);

    // Decrement only after the swap has been committed -- never burn a use
    // on a request that got rejected above.
    PlayerState->ConsumeSwapUse();

    return true;
}

bool UConnectIt_BoardRequestMediator::HandleBoardShiftRequest(
    const FConnectItRequestBoardShift& Request, int32 FactionID) const
{
    // Unrestricted -- no faction-ownership check on the shifted line and no
    // use-budget, unlike SWAP. Turn order (whether this faction was even
    // allowed to submit a request right now) is the only gate, same as
    // PlacePiece. FactionID is only used below for logging.
    if (Request.Positions.Num() < 2)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: BoardShift rejected -- "
                 "need at least 2 positions to shift, got %d"),
            Request.Positions.Num());
        return false;
    }

    // UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    const FConnectItBoardState& Current = BoardState->GetCurrentState();

    // Re-validate every position against the server's own board state --
    // Request.Positions is client-computed (UGridTileRegistryBase::
    // GetTilesByDirection, sorted into line order); never trust it blindly.
    // Snapshot each position's CURRENT data in Request order before mutating
    // anything, since the rotation below reads every entry before writing
    // any of them.
    TArray<FConnectItTileData> OldDataInOrder;
    OldDataInOrder.Reserve(Request.Positions.Num());
    for (const FGridPosition& Pos : Request.Positions)
    {
        const FConnectItTileData* Data = Current.GetTileData(Pos);
        if (!Data)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ConnectIt_BoardRequestMediator: BoardShift rejected -- "
                     "position (%d,%d) is not a registered tile"),
                Pos.X, Pos.Y);
            return false;
        }
        OldDataInOrder.Add(*Data);
    }

    // A tile with bCanShift false is skipped -- it keeps its own data
    // untouched and takes no part in the rotation; the shiftable tiles
    // around it rotate among themselves as if it weren't in the line at
    // all. Filter down to just the shiftable subset (still in line order)
    // before rotating.
    TArray<FGridPosition> ShiftablePositions;
    TArray<FConnectItTileData> ShiftableOldData;
    for (int32 Index = 0; Index < Request.Positions.Num(); Index++)
    {
        if (OldDataInOrder[Index].bCanShift)
        {
            ShiftablePositions.Add(Request.Positions[Index]);
            ShiftableOldData.Add(OldDataInOrder[Index]);
        }
    }

    if (ShiftablePositions.Num() < 2)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardRequestMediator: BoardShift rejected -- "
                 "fewer than 2 shiftable tiles in the line (%d of %d "
                 "positions can shift)"),
            ShiftablePositions.Num(), Request.Positions.Num());
        return false;
    }

    FConnectItBoardState NewState = Current;

    // Rotate whole tile data (FactionPiece, Multiplier, bIsActive, bCanShift
    // -- the tile itself, not just the piece on it) one step along the
    // shiftable subset, in its original line order: each shiftable position
    // takes on the PREVIOUS shiftable position's old data, with the last
    // one's data wrapping around to the first. Since Request.Positions is
    // sorted by increasing distance along Request.Direction from the
    // selected tile, this moves every shiftable tile exactly one step
    // further in that direction, wrapping the far end around to the near
    // end -- skipped (unshiftable) positions are left out of both the read
    // and the write entirely, and nothing outside Request.Positions is ever
    // touched. ShiftStartPositions/ShiftEndPositions record the same pairing
    // the rotation itself uses, for visual listeners.
    const int32 ShiftableNum = ShiftablePositions.Num();
    TArray<FGridPosition> ShiftStartPositions;
    TArray<FGridPosition> ShiftEndPositions;
    ShiftStartPositions.Reserve(ShiftableNum);
    ShiftEndPositions.Reserve(ShiftableNum);
    for (int32 Index = 0; Index < ShiftableNum; Index++)
    {
        const FGridPosition& FromPosition = ShiftablePositions[(Index - 1 + ShiftableNum) % ShiftableNum];
        const FGridPosition& ToPosition = ShiftablePositions[Index];
        const FConnectItTileData& IncomingData = ShiftableOldData[(Index - 1 + ShiftableNum) % ShiftableNum];

        NewState.SetTileData(ToPosition, IncomingData);
        ShiftStartPositions.Add(FromPosition);
        ShiftEndPositions.Add(ToPosition);
    }

    // Re-run scoring for every position that now holds a piece as a result
    // of the shift -- only occupied destinations can complete a line.
    // ApplyScoring appends to ScoringPositions across calls, same pattern
    // as HandleSwapPiecesRequest.
    TArray<FGridPosition> ScoringPositions;
    float TotalPointsScored = 0.f;
    int32 ScoringFactionSlot = -1;
    for (const FGridPosition& Pos : Request.Positions)
    {
        const FConnectItTileData* NewData = NewState.GetTileData(Pos);
        if (!NewData || !NewData->bIsOccupied) continue;

        const float PointsScored = BoardRules->ApplyScoring(
            NewState, Pos, NewData->FactionPiece, ScoringPositions);
        if (PointsScored > 0.f)
        {
            TotalPointsScored += PointsScored;
            // Known simplification, same as HandleSwapPiecesRequest:
            // ChangeEvent only carries one ScoringFactionSlot, so if more
            // than one faction scores from the same shift, only the last
            // one found is named in the event -- ScoreBoard itself (mutated
            // inside ApplyScoring) is correct for all of them regardless.
            ScoringFactionSlot = NewData->FactionPiece;
        }
    }

    BoardRules->CheckWinCondition(NewState);

    FConnectItBoardChangeEvent ChangeEvent;
    ChangeEvent.bBoardShifted       = true;
    ChangeEvent.ShiftDirection      = Request.Direction;
    ChangeEvent.ShiftAnchorPosition = Request.Positions[0];
    ChangeEvent.ShiftStartPositions = ShiftStartPositions;
    ChangeEvent.ShiftEndPositions   = ShiftEndPositions;
    ChangeEvent.bLineScored         = TotalPointsScored > 0.f;
    ChangeEvent.ScoringFactionSlot  = ScoringFactionSlot;
    ChangeEvent.PointsScored        = TotalPointsScored;
    ChangeEvent.ScoringLinePositions = ScoringPositions;
    ChangeEvent.bGameWon            = NewState.bGameOver && !Current.bGameOver;
    ChangeEvent.WinningFactionSlot  = NewState.WinningFactionSlot;

    if (ChangeEvent.bLineScored)
    {
        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_BoardRequestMediator: Faction %d's shift scored "
                 "%.0f points"),
            ScoringFactionSlot, TotalPointsScored);
    }

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_BoardRequestMediator: Faction %d shifted %d tile(s) "
             "starting at (%d,%d)"),
        FactionID, ChangeEvent.ShiftEndPositions.Num(), Request.Positions[0].X, Request.Positions[0].Y);

    BoardState->SetBoardState(NewState, ChangeEvent);
    return true;
}

bool UConnectIt_BoardRequestMediator::HandleToggleTileActiveRequest(
    const FConnectItRequestToggleTileActive& Request) const
{
    // UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
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
    // UConnectIt_BoardStateComponent* BoardState = GetBoardState();
    UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
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
