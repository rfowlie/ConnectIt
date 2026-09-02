// Fill out your copyright notice in the Description page of Project Settings.

#include "Board/ConnectIt_BoardManager.h"
#include "ConnectIt_GameplayTags.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "TurnBasedMechanicsStructs.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Board/Rules/ConnectIt_BoardRulesComponent.h"
#include "Framework/Data/ConnectIt_ConfigComponent.h"
#include "Framework/Subsystem/ConnectIt_BoardManagerSubsystem.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"
#include "Piece/GridPieceRegistryComponent.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"
#include "GameEvent/ConnectIt_PlacePieceGameEvent.h"
#include "Tile/GridTileRegistryBase.h"


AConnectIt_BoardManager::AConnectIt_BoardManager()
{
    BoardStateComponent =
        CreateDefaultSubobject<UConnectIt_BoardStateComponent>(
            TEXT("BoardState"));

    ConnectItConfigComponent =
        CreateDefaultSubobject<UConnectIt_ConfigComponent>(
            TEXT("ConnectItConfig"));

    BoardRulesComponent =
        CreateDefaultSubobject<UConnectIt_BoardRulesComponent>(
            TEXT("BoardRules"));
    

    bReplicates = true;
}

// --- Lifecycle ---

void AConnectIt_BoardManager::BeginPlay()
{
    Super::BeginPlay();

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

void AConnectIt_BoardManager::CreateGameEventsFromBoardUpdate_Implementation()
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

void AConnectIt_BoardManager::ExecuteGameEvents()
{
    // TODO queue wait system to run each game event waiting for it's OnComplete To Fire
    // this is an alternative to the GameEventSubsystem but this will allow us to more clearly
    // group and sequence the visual effects we want from each game event
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

    if (!IsValid(TileRegistry))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardManager: InitialiseBoard — "
                 "TileRegistryComponent is null"));
        return;
    }

    BoardStateComponent->InitialiseBoardState(TileRegistry, PieceRegistry, NumFactions);
    
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

    if (Request.RequestType == ConnectIt_Game_ForcePlacePiece)
    {
        if (const FConnectItRequestForcePlacePiece* Payload =
            Request.Payload.GetPtr<FConnectItRequestForcePlacePiece>())
        {
            return HandleForcePlacePieceRequest(*Payload, Request.FactionID);
        }

        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_BoardManager: ForcePlacePiece request payload "
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
            TEXT("ConnectIt_BoardManager: DestroyTileMultiplier request "
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
            TEXT("ConnectIt_BoardManager: RemovePiece request payload "
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
            TEXT("ConnectIt_BoardManager: SwapPieces request payload "
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
            TEXT("ConnectIt_BoardManager: ToggleTileActive request payload "
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
            TEXT("ConnectIt_BoardManager: CapturePiece request payload "
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

    TArray<FGridPosition> ScoringPositions;
    const float PointsScored = BoardRulesComponent->ApplyScoring(
        NewState, TargetPosition, FactionID, ScoringPositions);

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
    ChangeEvent.ScoringLinePositions = ScoringPositions;
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
    // if (!IsValid(BoardShiftComponent))
    // {
    //     UE_LOG(LogTemp, Error,
    //         TEXT("ConnectIt_BoardManager: HandleShiftRequest — "
    //              "ShiftStateComponent is null"));
    //     return false;
    // }
    //
    // FConnectItBoardState NewState = BoardStateComponent->GetCurrentState();
    // FShiftResult ShiftResult;
    // if (!BoardShiftComponent->ComputeShift(
    //     NewState, ShiftResult, Request.ShiftOperation))
    // {
    //     UE_LOG(LogTemp, Warning,
    //         TEXT("ConnectIt_BoardManager: Shift rejected — "
    //              "ComputeShift returned an invalid result"));
    //     return false;
    // }
    //
    // // Record what happened -- same authoritative-first pattern as
    // // HandlePlacePieceRequest: commit state via SetBoardState first, then
    // // ConnectIt_BoardStateComponent (fires identically on server and
    // // client) enqueues the gated visual sequence from the replicated
    // // ChangeEvent. Parallel arrays instead of Result's TMap/TSet directly --
    // // those don't replicate (see FConnectItBoardChangeEvent's Shift fields).
    // FConnectItBoardChangeEvent ChangeEvent;
    // ChangeEvent.bShiftApplied = true;
    // ChangeEvent.ShiftOperation = Request.ShiftOperation;
    // ShiftResult.PositionRemap.GenerateKeyArray(ChangeEvent.ShiftFromPositions);
    // for (const FGridPosition& FromPos : ChangeEvent.ShiftFromPositions)
    // {
    //     ChangeEvent.ShiftToPositions.Add(ShiftResult.PositionRemap[FromPos]);
    // }
    //
    // ChangeEvent.ShiftWrappingPositions = ShiftResult.WrappingPositions.Array();
    //
    // BoardStateComponent->SetBoardState(NewState, ChangeEvent);
    // return true;

    // TODO: implement this once basic functionality is good
    return false;
}

bool AConnectIt_BoardManager::HandleForcePlacePieceRequest(
    const FConnectItRequestForcePlacePiece& Request, int32 FactionID) const
{
    const FConnectItBoardState& Current = BoardStateComponent->GetCurrentState();

    // Only requires the position to exist -- deliberately skips
    // IsTileValidForPlacement (active/unoccupied) so this can overwrite an
    // inactive or already-occupied tile. Registry existence is still
    // checked so a garbage position can't silently grow the tile arrays.
    if (!Current.GetTileData(Request.Position))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardManager: ForcePlacePiece rejected "
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

bool AConnectIt_BoardManager::HandleDestroyTileMultiplierRequest(
    const FConnectItRequestDestroyTileMultiplier& Request) const
{
    const FConnectItBoardState& Current = BoardStateComponent->GetCurrentState();
    const FConnectItTileData* Existing = Current.GetTileData(Request.Position);

    if (!Existing)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardManager: DestroyTileMultiplier rejected "
                 "-- position (%d,%d) not registered"),
            Request.Position.X, Request.Position.Y);
        return false;
    }

    if (Existing->Multiplier == 1.0f)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardManager: DestroyTileMultiplier rejected "
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

bool AConnectIt_BoardManager::HandleRemovePieceRequest(
    const FConnectItRequestRemovePiece& Request) const
{
    if (Request.DelayTurns > 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardManager: RemovePiece rejected -- "
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
            TEXT("ConnectIt_BoardManager: RemovePiece rejected -- "
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

bool AConnectIt_BoardManager::HandleSwapPiecesRequest(
    const FConnectItRequestSwapPieces& Request) const
{
    const FConnectItBoardState& Current = BoardStateComponent->GetCurrentState();
    const FConnectItTileData* DataA = Current.GetTileData(Request.PositionA);
    const FConnectItTileData* DataB = Current.GetTileData(Request.PositionB);

    if (!DataA || !DataB || !DataA->IsOccupied() || !DataB->IsOccupied())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardManager: SwapPieces rejected -- "
                 "both (%d,%d) and (%d,%d) must be registered and occupied"),
            Request.PositionA.X, Request.PositionA.Y,
            Request.PositionB.X, Request.PositionB.Y);
        return false;
    }

    FConnectItBoardState NewState = Current;

    FConnectItTileData* MutableA = NewState.GetTileDataMutable(Request.PositionA);
    FConnectItTileData* MutableB = NewState.GetTileDataMutable(Request.PositionB);
    Swap(MutableA->FactionPiece, MutableB->FactionPiece);

    // No scoring/win-condition re-check -- see class header comment
    FConnectItBoardChangeEvent ChangeEvent;
    ChangeEvent.bPiecesSwapped = true;
    ChangeEvent.SwapPositionA  = Request.PositionA;
    ChangeEvent.SwapPositionB  = Request.PositionB;

    BoardStateComponent->SetBoardState(NewState, ChangeEvent);
    return true;
}

bool AConnectIt_BoardManager::HandleToggleTileActiveRequest(
    const FConnectItRequestToggleTileActive& Request) const
{
    const FConnectItBoardState& Current = BoardStateComponent->GetCurrentState();
    const FConnectItTileData* Existing = Current.GetTileData(Request.Position);

    if (!Existing)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardManager: ToggleTileActive rejected -- "
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

bool AConnectIt_BoardManager::HandleCapturePieceRequest(
    const FConnectItRequestCapturePiece& Request, int32 FactionID) const
{
    const FConnectItBoardState& Current = BoardStateComponent->GetCurrentState();
    const FConnectItTileData* Existing = Current.GetTileData(Request.Position);

    if (!Existing || !Existing->IsOccupied())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardManager: CapturePiece rejected -- "
                 "position (%d,%d) is not occupied"),
            Request.Position.X, Request.Position.Y);
        return false;
    }

    if (Existing->FactionPiece == FactionID)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardManager: CapturePiece rejected -- "
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

    // Exactly one position changed ownership -- same well-defined case
    // HandlePlacePieceRequest already handles, unlike HandleSwapPiecesRequest
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

void AConnectIt_BoardManager::BindParticipantManager()
{
    ParticipantManagerRef = UConnectIt_GameUtilityLibrary::GetParticipantManager(this);

    if (IsValid(ParticipantManagerRef))
    {
        ParticipantManagerRef->OnActiveControllerChanged.AddDynamic(
            this, &AConnectIt_BoardManager::HandleActiveControllerChanged);
    }
}
