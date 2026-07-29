// Fill out your copyright notice in the Description page of Project Settings.


#include "Board/ConnectIt_BoardManager.h"

#include "ConnectIt_GameplayTags.h"
#include "GameplayTagContainer.h"
#include "TurnBasedMechanicsStructs.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Framework/Data/ConnectIt_ConfigComponent.h"
#include "Interpreter/ConnectIt_PieceSpawnInterpreter.h"
#include "Interpreter/ConnectIt_ScoreInterpreter.h"
#include "Interpreter/ConnectIt_TileStateInterpreter.h"
#include "Board/Shift/BoardShiftComponent.h"
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

    BoardShiftComponent =
        CreateDefaultSubobject<UBoardShiftComponent>(
            TEXT("BoardShift"));

    ConnectItConfig =
        CreateDefaultSubobject<UConnectIt_ConfigComponent>(
            TEXT("ConnectItConfig"));

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

// --- ABoardManager Interface Overrides ---

UGridTileRegistryComponent* AConnectIt_BoardManager::GetTileRegistry_Implementation() const
{
    return TileRegistryComponent;
}

UGridPieceRegistryComponent* AConnectIt_BoardManager::GetPieceRegistry_Implementation() const
{
    return PieceRegistryComponent;
}

UBoardStateComponentBase* AConnectIt_BoardManager::GetBoardState_Implementation() const
{
    return BoardStateComponent;
}

UBoardShiftComponent* AConnectIt_BoardManager::GetShiftComponent_Implementation() const
{
    return BoardShiftComponent;
}

// --- Lifecycle ---

void AConnectIt_BoardManager::BeginPlay()
{
    Super::BeginPlay();

    BindInterpreters();

    if (HasAuthority())
    {
        BindShiftComponent();
        BindParticipantManager();
        
        // Sync rules from config to actor
        if (IsValid(ConnectItConfig))
        {
            WinScoreThreshold = ConnectItConfig->WinScoreThreshold;
            ConnectLength     = ConnectItConfig->ConnectLength;
        }
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

void AConnectIt_BoardManager::BindShiftComponent()
{
    if (!IsValid(BoardShiftComponent)) return;

    BoardShiftComponent->OnShiftResultReady.AddDynamic(
        this,
        &AConnectIt_BoardManager::HandleShiftResult);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_BoardManager: Shift component wired"));
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
             "%d tiles, %d factions, win threshold %.0f"),
        TilePositions.Num(),
        NumFactions,
        WinScoreThreshold);
}

// --- Request Processing ---

void AConnectIt_BoardManager::ProcessRequest(
    const FTurnActionRequest& Request)
{
    if (!HasAuthority()) return;

    if (!Request.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardManager: Received invalid "
                 "FTurnActionRequest"));
        return;
    }

    if (Request.RequestType == ConnectIt_Game_PlacePiece)
    {
        HandlePlacePieceRequest(Request);
        return;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("ConnectIt_BoardManager: Unknown request type '%s'"),
        *Request.RequestType.ToString());
}

void AConnectIt_BoardManager::HandlePlacePieceRequest(
    const FTurnActionRequest& Request)
{
    if (Request.Positions.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardManager: PlacePiece request "
                 "has no positions"));
        return;
    }

    const FGridPosition TargetPosition = Request.Positions[0];
    const FConnectItBoardState& Current =
        BoardStateComponent->GetCurrentState();

    if (!Current.IsTileValidForPlacement(TargetPosition))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_BoardManager: PlacePiece rejected "
                 "— position (%d,%d) invalid for placement"),
            TargetPosition.X, TargetPosition.Y);
        return;
    }

    FConnectItBoardState NewState = Current;

    if (FConnectItTileData* TileData =
        NewState.GetTileDataMutable(TargetPosition))
    {
        TileData->FactionPiece = Request.FactionID;
    }

    const float PointsScored = CheckAndApplyScoring(
        NewState, TargetPosition, Request.FactionID);

    if (PointsScored > 0.f)
    {
        UE_LOG(LogTemp, Log,
            TEXT("ConnectIt_BoardManager: Faction %d scored %.0f points"),
            Request.FactionID, PointsScored);

        OnLineScored.Broadcast(Request.FactionID, PointsScored);
    }

    CheckWinCondition(NewState);
    BoardStateComponent->ApplyAndBroadcast(NewState);
    OnPiecePlaced.Broadcast(TargetPosition);

    if (NewState.bGameOver)
    {
        OnPlayerWin.Broadcast(NewState.WinningFactionSlot);
    }
}

void AConnectIt_BoardManager::HandleShiftResult(
    const FShiftOperation& Operation,
    const FShiftResult& Result)
{
    if (!HasAuthority()) return;

    FConnectItBoardState NewState =
        BoardStateComponent->GetCurrentState();

    // Snapshot positions before remap to prevent mid-remap overwrites
    TMap<FGridPosition, FConnectItTileData> Snapshot;
    for (const auto& [OldPos, NewPos] : Result.PositionRemap)
    {
        const FConnectItTileData* Data = NewState.GetTileData(OldPos);
        if (Data) Snapshot.Add(OldPos, *Data);
    }

    // Apply remap atomically from snapshot
    for (const auto& [OldPos, NewPos] : Result.PositionRemap)
    {
        const FConnectItTileData* Data = Snapshot.Find(OldPos);
        if (Data)
        {
            NewState.SetTileData(NewPos, *Data);
        }
        else
        {
            NewState.SetTileData(NewPos, FConnectItTileData());
        }
    }

    BoardStateComponent->ApplyAndBroadcast(NewState);
    OnShiftApplied.Broadcast(Operation, Result);
}

// --- Game Logic ---

float AConnectIt_BoardManager::CheckAndApplyScoring(
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
        TotalPoints += ApplyScoringLine(
            MutableState, Line, Position, FactionSlot);
    }

    if (MutableState.ScoreBoard.IsValidIndex(FactionSlot))
    {
        MutableState.ScoreBoard[FactionSlot] += TotalPoints;
    }

    return TotalPoints;
}

TArray<TArray<FGridPosition>> AConnectIt_BoardManager::FindScoringLines(
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

float AConnectIt_BoardManager::ApplyScoringLine(
    FConnectItBoardState& MutableState,
    const TArray<FGridPosition>& Line,
    FGridPosition CompletingPosition,
    int32 FactionSlot) const
{
    float PointsScored = 0.f;

    for (const FGridPosition& Position : Line)
    {
        FConnectItTileData* TileData = MutableState.GetTileDataMutable(Position);
        if (!TileData) continue;

        PointsScored += TileData->Multiplier;

        // Remove piece and increment multiplier
        TileData->FactionPiece = -1;
        TileData->Multiplier  += 1.0f;
    }

    // Completing piece stays on the board
    if (FConnectItTileData* CompletingTile =
        MutableState.GetTileDataMutable(CompletingPosition))
    {
        CompletingTile->FactionPiece = FactionSlot;
    }

    return PointsScored;
}

void AConnectIt_BoardManager::CheckWinCondition(
    FConnectItBoardState& MutableState) const
{
    for (int32 i = 0; i < MutableState.ScoreBoard.Num(); i++)
    {
        if (MutableState.ScoreBoard[i] >= WinScoreThreshold)
        {
            MutableState.bGameOver         = true;
            MutableState.WinningFactionSlot = i;

            UE_LOG(LogTemp, Log,
                TEXT("ConnectIt_BoardManager: "
                     "Faction %d wins with %.0f points"),
                i, MutableState.ScoreBoard[i]);

            return;
        }
    }
}

const TArray<FGridDirectionVector>&
AConnectIt_BoardManager::GetScoringDirections()
{
    static const TArray<FGridDirectionVector> Directions =
    {
        { 0,  1 },  // Row
        { 1,  0 },  // Column
        { 1,  1 },  // Diagonal top-down
        { 1, -1 }   // Diagonal bottom-up
    };
    return Directions;
}

// --- Config Accessors ---

UActionLoadoutDataAsset* AConnectIt_BoardManager::GetPlayerLoadout() const
{
    return IsValid(ConnectItConfig)
        ? ConnectItConfig->PlayerLoadout : nullptr;
}

UActionLoadoutDataAsset* AConnectIt_BoardManager::GetEnemyLoadout() const
{
    return IsValid(ConnectItConfig)
        ? ConnectItConfig->EnemyLoadout : nullptr;
}

float AConnectIt_BoardManager::GetWinScoreThreshold() const
{
    return IsValid(ConnectItConfig)
        ? ConnectItConfig->WinScoreThreshold : 100.f;
}

void AConnectIt_BoardManager::BindParticipantManager()
{
    if (UTurnBasedParticipantManagerComponent* PM =
        UConnectIt_GameUtilityLibrary::GetParticipantManager(this))
    {
        PM->OnActiveControllerChanged.AddDynamic(
            this, &AConnectIt_BoardManager::HandleActiveControllerChanged);
    }
}
