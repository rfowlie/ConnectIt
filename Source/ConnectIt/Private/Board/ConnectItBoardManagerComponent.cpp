// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/ConnectItBoardManagerComponent.h"
#include "ConnectIt_GameplayTags.h"
#include "GameplayTagContainer.h"
#include "TurnBasedMechanicsStructs.h"
#include "Board/ConnectItBoardStateComponent.h"
#include "Board/Shift/BoardShiftComponent.h"
#include "Library/CodingUtilsComponentLibrary.h"
#include "Tile/GridTileRegistryComponent.h"


UConnectItBoardManagerComponent::UConnectItBoardManagerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UConnectItBoardManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!ResolveComponents())
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectItBoardManagerComponent: Failed to resolve "
                 "required components on %s"),
            *GetOwner()->GetName());
    }
}

bool UConnectItBoardManagerComponent::ResolveComponents()
{
    BoardStateComponent =
        GetOwner()->FindComponentByClass<UConnectItBoardStateComponent>();
    RegistryComponent =
        GetOwner()->FindComponentByClass<UGridTileRegistryComponent>();
    ShiftComponent =
        GetOwner()->FindComponentByClass<UBoardShiftComponent>();

    if (!IsValid(BoardStateComponent))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectItBoardManagerComponent: "
                 "UConnectItBoardStateComponent not found"));
        return false;
    }

    if (!IsValid(RegistryComponent))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectItBoardManagerComponent: "
                 "UGridRegistryComponent not found"));
        return false;
    }

    return true;
}

void UConnectItBoardManagerComponent::InitialiseBoard(int32 NumFactions)
{
    check(UCodingUtilsComponentLibrary::IsAuthoritative(this));
    check(IsValid(BoardStateComponent));
    check(IsValid(RegistryComponent));

    // Read all registered tile positions from the grid registry
    const TArray<FGridPosition> TilePositions =
        RegistryComponent->GetAllTilePositions();

    if (TilePositions.IsEmpty())
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectItBoardManagerComponent: No tile positions "
                 "found in registry — ensure tiles have registered "
                 "with UGridWorldSubsystem before calling InitialiseBoard"));
        return;
    }

    BoardStateComponent->InitialiseBoardState(
        TilePositions, NumFactions);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectItBoardManagerComponent: Board initialised — "
             "%d tiles, %d factions, win threshold %.0f"),
        TilePositions.Num(), NumFactions, WinScoreThreshold);
}

// --- Request Processing ---

void UConnectItBoardManagerComponent::ProcessRequest(const FTurnActionRequest& Request)
{
    check(UCodingUtilsComponentLibrary::IsAuthoritative(this));

    if (!Request.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectItBoardManagerComponent: "
                 "Received invalid FTurnActionRequest"));
        return;
    }

    UE_LOG(LogTemp, Log,
        TEXT("ConnectItBoardManagerComponent: Processing request "
             "'%s' from faction %d"),
        *Request.RequestType.ToString(),
        Request.FactionID);

    if (Request.RequestType == ConnectIt_Game_PlacePiece)
    {
        HandlePlacePieceRequest(Request);
        return;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("ConnectItBoardManagerComponent: Unknown request type '%s'"),
        *Request.RequestType.ToString());
}

void UConnectItBoardManagerComponent::HandlePlacePieceRequest(
    const FTurnActionRequest& Request)
{
    if (Request.Positions.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectItBoardManagerComponent: PlacePiece request "
                 "has no positions"));
        return;
    }

    const FGridPosition TargetPosition = Request.Positions[0];

    // Server side validation -- client validation is convenience only
    const FConnectItBoardState& Current =
        BoardStateComponent->GetCurrentState();

    if (!Current.IsTileValidForPlacement(TargetPosition))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectItBoardManagerComponent: PlacePiece rejected "
                 "— position (%d,%d) is not valid for placement"),
            TargetPosition.X, TargetPosition.Y);
        return;
    }

    // Work on a mutable copy of current state
    FConnectItBoardState NewState = Current;

    // Place the piece
    FConnectItTileData TileData = *NewState.GetTileDataMutable(TargetPosition);
    TileData.FactionPiece = Request.FactionID;

    UE_LOG(LogTemp, Log,
        TEXT("ConnectItBoardManagerComponent: Placing piece at (%d,%d) "
             "for faction %d"),
        TargetPosition.X, TargetPosition.Y, Request.FactionID);

    // Check scoring -- may modify NewState
    const float PointsScored = CheckAndApplyScoring(
        NewState, TargetPosition, Request.FactionID);

    if (PointsScored > 0.f)
    {
        UE_LOG(LogTemp, Log,
            TEXT("ConnectItBoardManagerComponent: Faction %d scored "
                 "%.0f points — total: %.0f"),
            Request.FactionID,
            PointsScored,
            NewState.GetScore(Request.FactionID));

        OnLineScored.Broadcast(Request.FactionID, PointsScored);
    }

    // Check win condition -- sets bGameOver on NewState if met
    CheckWinCondition(NewState);

    // Apply new state -- triggers replication and visual update
    BoardStateComponent->ApplyAndBroadcast(NewState);

    OnPiecePlaced.Broadcast(TargetPosition);

    if (NewState.bGameOver)
    {
        OnGameOver.Broadcast(NewState.WinningFactionSlot);
    }
}

void UConnectItBoardManagerComponent::HandleShiftResult(
    const FShiftOperation& Operation,
    const FShiftResult& Result)
{
    check(UCodingUtilsComponentLibrary::IsAuthoritative(this));
    check(IsValid(BoardStateComponent));

    FConnectItBoardState NewState =
        BoardStateComponent->GetCurrentState();

    // Snapshot of tile data before remap
    // Prevents overwriting tiles mid-remap
    TMap<FGridPosition, FConnectItTileData> Snapshot;
    for (const auto& [OldPos, NewPos] : Result.PositionRemap)
    {
        if (const FConnectItTileData* Data = NewState.GetTileData(OldPos))
        {
            Snapshot.Add(OldPos, *Data);
        }
    }

    // Apply to remap atomically from snapshot
    for (const auto& [OldPos, NewPos] : Result.PositionRemap)
    {
        if (const FConnectItTileData* Data = Snapshot.Find(OldPos))
        {
            NewState.SetTileData(NewPos, *Data);
        }
        else
        {
            // No data at old position -- reset destination
            FConnectItTileData EmptyTile;
            NewState.SetTileData(NewPos, EmptyTile);
        }
    }

    BoardStateComponent->ApplyAndBroadcast(NewState);
    OnShiftApplied.Broadcast(Operation, Result);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectItBoardManagerComponent: Shift applied — "
             "%s %d remapped %d positions"),
        Operation.Axis == EShiftAxis::Row ? TEXT("Row") : TEXT("Column"),
        Operation.Index,
        Result.PositionRemap.Num());
}

// --- Game Logic ---

float UConnectItBoardManagerComponent::CheckAndApplyScoring(
    FConnectItBoardState& MutableState,
    FGridPosition Position,
    int32 FactionSlot)
{
    TArray<TArray<FGridPosition>> ScoringLines =
        FindScoringLines(MutableState, Position, FactionSlot);

    if (ScoringLines.IsEmpty()) return 0.f;

    float TotalPoints = 0.f;

    for (const TArray<FGridPosition>& Line : ScoringLines)
    {
        TotalPoints += ApplyScoringLine(MutableState, Line, Position, FactionSlot);
    }

    // Update scoreboard
    if (MutableState.ScoreBoard.IsValidIndex(FactionSlot))
    {
        MutableState.ScoreBoard[FactionSlot] += TotalPoints;
    }

    return TotalPoints;
}

TArray<TArray<FGridPosition>>
UConnectItBoardManagerComponent::FindScoringLines(
    const FConnectItBoardState& State,
    FGridPosition Position,
    int32 FactionSlot) const
{
    TArray<TArray<FGridPosition>> ScoringLines;

    for (const FGridDirectionVector& Dir : GetScoringDirections())
    {
        TArray<FGridPosition> Line;
        Line.Add(Position);

        // Walk positive direction
        for (int32 Step = 1; Step < ConnectLength * 2; Step++)
        {
            FGridPosition Check(
                Position.X + Step * Dir.Row,
                Position.Y + Step * Dir.Column);

            const FConnectItTileData* Data = State.GetTileData(Check);
            if (!Data || Data->FactionPiece != FactionSlot) break;
            Line.Add(Check);
        }

        // Walk negative direction
        for (int32 Step = 1; Step < ConnectLength * 2; Step++)
        {
            FGridPosition Check(
                Position.X - Step * Dir.Row,
                Position.Y - Step * Dir.Column);

            const FConnectItTileData* Data = State.GetTileData(Check);
            if (!Data || Data->FactionPiece != FactionSlot) break;
            Line.Add(Check);
        }

        if (Line.Num() >= ConnectLength)
        {
            ScoringLines.Add(MoveTemp(Line));
        }
    }

    return ScoringLines;
}

float UConnectItBoardManagerComponent::ApplyScoringLine(FConnectItBoardState& MutableState,
    const TArray<FGridPosition>& Line, FGridPosition CompletingPosition, int32 FactionSlot) const
{
    float PointsScored = 0.f;

    for (const FGridPosition& Position : Line)
    {
        FConnectItTileData* TileData = MutableState.GetTileDataMutable(Position);
        if (!TileData) continue;

        // Accumulate multiplier value
        PointsScored += TileData->Multiplier;

        // Remove piece and increment multiplier on every tile in line
        TileData->FactionPiece = -1;
        TileData->Multiplier  += 1.0f;
    }

    // Place completing piece back -- it stays on the board
    if (FConnectItTileData* CompletingTile =
        MutableState.GetTileDataMutable(CompletingPosition))
    {
        CompletingTile->FactionPiece = FactionSlot;; // restore faction
    }

    return PointsScored;
}

void UConnectItBoardManagerComponent::CheckWinCondition(
    FConnectItBoardState& MutableState) const
{
    for (int32 i = 0; i < MutableState.ScoreBoard.Num(); i++)
    {
        if (MutableState.ScoreBoard[i] >= WinScoreThreshold)
        {
            MutableState.bGameOver        = true;
            MutableState.WinningFactionSlot = i;

            UE_LOG(LogTemp, Log,
                TEXT("ConnectItBoardManagerComponent: "
                     "Faction %d wins with %.0f points"),
                i, MutableState.ScoreBoard[i]);

            return;
        }
    }
}

const TArray<FGridDirectionVector>&
UConnectItBoardManagerComponent::GetScoringDirections()
{
    static const TArray<FGridDirectionVector> Directions =
    {
        { 0,  1 },  // Row
        { 1,  0 },  // Column
        { 1,  1 },  // Diagonal top-down
        { 1, -1 }   // Diagonal bottom-up
    };
    return Directions;
};