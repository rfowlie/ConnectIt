// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/GameMode/ConnectIt_GameMode.h"
#include "EngineUtils.h"
#include "Board/ConnectIt_BoardManager.h"
#include "Framework/Controller/ConnectIt_AIController.h"
#include "Framework/GameState/ConnectIt_GameState.h"
#include "Framework/GameState/TurnBasedGameState.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "Library/ConnectIt_GameUtilityLibrary.h"


AConnectIt_GameMode::AConnectIt_GameMode()
{
    GameStateClass  = AConnectIt_GameState::StaticClass();
    PlayerStateClass = ATurnBasedPlayerState::StaticClass();

    TurnDuration           = 90.f;
    ForfeitThreshold       = 3;
    ReconnectTimeout       = 30.f;
    TurnResolutionDuration = 1.5f;
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

// --- Board Setup ---

void AConnectIt_GameMode::InitialiseBoard()
{
    AConnectIt_BoardManager* Board = GetBoardActor();
    if (!IsValid(Board))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameMode: No AConnectItBoardActor found "
                 "in level — board cannot be initialised"));
        return;
    }

    // Bind game over handler before initialising
    // Board manager fires this when win condition is met
    if (IsValid(Board->BoardManager))
    {
        Board->BoardManager->OnPlayerWin.AddDynamic(
            this, &AConnectIt_GameMode::HandleGameOver);
    }

    // Initialise board state from registered tile positions
    Board->InitialiseBoard(NumFactions);

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

void AConnectIt_GameMode::HandleGameOver(int32 WinningFactionSlot)
{
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

// --- Helpers ---

AConnectIt_BoardManager* AConnectIt_GameMode::GetBoardActor() const
{
    if (!IsValid(CachedBoardActor))
    {
        CachedBoardActor = UConnectIt_GameUtilityLibrary::GetBoardManager(this);
    }
    
    return CachedBoardActor;
}