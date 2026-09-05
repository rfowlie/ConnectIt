// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/GameMode/ConnectIt_GameMode.h"
#include "EngineUtils.h"
#include "Board/ConnectIt_BoardRequestMediator.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "Board/Rules/ConnectIt_BoardRules.h"
#include "ConnectIt_GameplayTags.h"
#include "Framework/Controller/ConnectIt_AIController.h"
#include "Framework/Controller/ConnectIt_PlayerController.h"
#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Framework/GameState/ConnectIt_GameState.h"
#include "Framework/GameState/TurnBasedGameState.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "GameEvent/GameEventTaskSubsystem.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"
#include "Tile/GridTileRegistryBase.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"


AConnectIt_GameMode::AConnectIt_GameMode()
{
    GameStateClass  = AConnectIt_GameState::StaticClass();
    PlayerStateClass = ATurnBasedPlayerState::StaticClass();

    TurnDuration     = 90.f;
    ForfeitThreshold = 3;
    ReconnectTimeout = 30.f;
}

// --- Overrides ---

void AConnectIt_GameMode::PostLogin(APlayerController* NewPlayer)
{
    // Let base class handle reconnect detection and registration
    Super::PostLogin(NewPlayer);

    ConnectedHumanCount++;

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_GameMode: Human player connected "
             "(%d / %d expected)"),
        ConnectedHumanCount,
        ExpectedHumanCount());

    // Online mode -- start when both humans are connected
    // Adventure mode -- start immediately, AI registered separately
    if (MatchType == EConnectItMatchType::Online)
    {
        if (ConnectedHumanCount >= ExpectedHumanCount())
        {
            StartReadyCheck();
        }
    }
}

void AConnectIt_GameMode::HandleMatchHasStarted()
{
    // Base class applies turn config to participant manager
    Super::HandleMatchHasStarted();

    if (UTurnBasedParticipantManagerComponent* Manager = GetParticipantManager())
    {
        Manager->OnInvalidNumberOfPlayers.AddDynamic(
            this, &AConnectIt_GameMode::HandleInvalidNumberOfPlayers);
    }

    // Construct the server-only board objects -- NewObject here (not the
    // constructor) so Blueprint-child property overrides on this GameMode
    // are already applied by the time these read anything from it.
    BoardRules = NewObject<UConnectIt_BoardRules>(this);

    // Level-authored rule selection, if any -- both server and client
    // resolve the same static asset independently (see GetLevelConfig);
    // BoardRules->Initialise() below still defaults anything left unset.
    if (const UConnectIt_LevelConfigDataAsset* LevelConfig =
        UConnectIt_GameUtilityLibrary::GetLevelConfig(this))
    {
        BoardRules->ScoringRule = LevelConfig->ScoringRule;
        BoardRules->WinConditionRule = LevelConfig->WinConditionRule;
    }
    BoardRules->Initialise();

    BoardRequestMediator = NewObject<UConnectIt_BoardRequestMediator>(this);
    BoardRequestMediator->Initialise(BoardRules);

    // Adventure mode -- spawn and register AI
    // Tiles have registered with subsystem by this point
    // so board initialisation is safe
    if (MatchType == EConnectItMatchType::Adventure)
    {
        SpawnAndRegisterAI();
        StartReadyCheck();
    }

    // Initialise board -- reads tile positions from registry
    InitialiseBoard();
}

void AConnectIt_GameMode::HandleMatchHasEnded()
{
    Super::HandleMatchHasEnded();

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_GameMode: Match ended"));
}

bool AConnectIt_GameMode::ProcessBoardRequest(const FTurnActionRequest& Request)
{
    if (!IsValid(BoardRequestMediator))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameMode: ProcessBoardRequest — "
                 "BoardRequestMediator is null"));
        return false;
    }

    return BoardRequestMediator->ProcessRequest(Request);
}

// --- Board Setup ---

void AConnectIt_GameMode::InitialiseBoard()
{
    AConnectIt_GameState* GS = GetGameState<AConnectIt_GameState>();
    UConnectIt_BoardStateComponent* BoardState = IsValid(GS) ? GS->GetBoardStateComponent() : nullptr;

    if (!IsValid(BoardState))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameMode: InitialiseBoard — "
                 "BoardStateComponent is null"));
        return;
    }

    // TileRegistry lives per-machine on AConnectIt_PlayerController now --
    // grab any connected one's for this one-time tile enumeration. Tile
    // layout is level-authored and deterministic, so every connected
    // controller's registry necessarily agrees; called after all tiles
    // have registered (this function's own comment), by which point every
    // connected controller has already had BeginPlay run.
    UGridTileRegistryBase* TileRegistry = nullptr;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (const AConnectIt_PlayerController* PC = Cast<AConnectIt_PlayerController>(It->Get()))
        {
            if (IsValid(PC->GetTileRegistry()))
            {
                TileRegistry = PC->GetTileRegistry();
                break;
            }
        }
    }

    if (!IsValid(TileRegistry))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameMode: InitialiseBoard — no connected "
                 "AConnectIt_PlayerController has a valid TileRegistry"));
        return;
    }

    // Bind game over handler to the tag subsystem rather than the board
    // manager directly -- the binding then doesn't depend on anything
    // above having resolved successfully
    if (UGameEventTaskSubsystem* GameEventSubsystem =
        GetWorld()->GetSubsystem<UGameEventTaskSubsystem>())
    {
        GameEventSubsystem->BindOnTagComplete(
            ConnectIt_Event_PlayerWin, this,
            GET_FUNCTION_NAME_CHECKED(AConnectIt_GameMode, HandleGameOver));
    }

    const float InitialTargetScore = IsValid(BoardRules)
        ? BoardRules->GetTargetScore()
        : 0.f;

    // PieceRegistry param is unused inside InitialiseBoardState (confirmed)
    // and now lives per-client on the player controller anyway -- nothing
    // meaningful to pass here.
    BoardState->InitialiseBoardState(
        TileRegistry, nullptr, NumFactions,
        /*InitialMultiplier=*/1.0f, InitialTargetScore);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_GameMode: Board initialised — %d factions"),
        NumFactions);
}

// --- AI ---

void AConnectIt_GameMode::SpawnAndRegisterAI()
{
    if (!IsValid(AIControllerClass))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameMode: AIControllerClass not set "
                 "— cannot spawn AI for Adventure mode"));
        return;
    }

    // Spawn AI controller -- no pawn needed for turn based
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AConnectIt_AIController* AIController =
        GetWorld()->SpawnActor<AConnectIt_AIController>(
            AIControllerClass,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            SpawnParams
        );

    if (!IsValid(AIController))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameMode: Failed to spawn AI controller"));
        return;
    }

    RegisterAIParticipant(AIController, AIDisplayName);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_GameMode: AI controller spawned and registered "
             "as '%s'"),
        *AIDisplayName);
}

// --- Game Over ---

void AConnectIt_GameMode::HandleGameOver(FGameplayTag Tag)
{
    // Tag is just the manager's own identity (ConnectIt_Event_PlayerWin,
    // the only thing this is ever bound to), not event-specific data --
    // WinningFactionSlot is a persistent field CheckWinCondition already
    // set on CurrentState, no need to route it through a delegate parameter
    const UConnectIt_BoardStateComponent* BoardState =
        UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);

    const int32 WinningFactionSlot = IsValid(BoardState)
        ? BoardState->GetCurrentState().WinningFactionSlot : -1;

    UE_LOG(LogTemp, Log,
       TEXT("ConnectIt_GameMode: Game over — faction %d wins"),
       WinningFactionSlot);

    // Write result to GameState -- replicates to all clients
    if (AConnectIt_GameState* GS = GetGameState<AConnectIt_GameState>())
    {
        GS->SetMatchResult(
            WinningFactionSlot,
            EMatchEndReason::ScoreThresholdReached,
            GetGameState<ATurnBasedGameState>()->GetActiveTurnNumber()
        );
    }

    EndMatch();
}

void AConnectIt_GameMode::HandleInvalidNumberOfPlayers()
{
    UTurnBasedParticipantManagerComponent* Manager = GetParticipantManager();
    if (!IsValid(Manager)) return;

    // Find the one still-active participant (if any) -- they win by
    // default. Also reads the other participant's connection/forfeit state
    // to pick the correct existing EMatchEndReason. No "try to fix it
    // first" path yet (e.g. waiting out a grace period before conceding) --
    // this always ends the match immediately.
    int32 SurvivingFactionSlot = -1;
    EMatchEndReason Reason = EMatchEndReason::Unknown;

    for (const FTurnParticipantInfo& Info : Manager->Participants)
    {
        if (Info.IsActiveParticipant())
        {
            SurvivingFactionSlot = Info.SlotIndex;
            continue;
        }

        Reason = Info.bConnected
            ? EMatchEndReason::OpponentForfeited
            : EMatchEndReason::OpponentDisconnected;
    }

    if (AConnectIt_GameState* GS = GetGameState<AConnectIt_GameState>())
    {
        GS->SetMatchResult(
            SurvivingFactionSlot,
            Reason,
            GetGameState<ATurnBasedGameState>()->GetActiveTurnNumber()
        );
    }

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_GameMode: Invalid number of players — ending "
             "match, faction %d wins by default (%s)"),
        SurvivingFactionSlot, *UEnum::GetValueAsString(Reason));

    EndMatch();
}
