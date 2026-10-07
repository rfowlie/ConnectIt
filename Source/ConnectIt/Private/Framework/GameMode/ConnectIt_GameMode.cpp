// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/GameMode/ConnectIt_GameMode.h"
#include "EngineUtils.h"
#include "Board/ConnectIt_BoardRequestMediator.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "ConnectIt_GameplayTags.h"
#include "Framework/Controller/ConnectIt_AIController.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Framework/Data/ConnectIt_LevelConfigSettings.h"
#include "AI/ConnectIt_AIProfile.h"
#include "Framework/Subsystem/ConnectIt_MatchSetupSubsystem.h"
#include "Framework/GameState/ConnectIt_GameState.h"
#include "Framework/GameState/TurnBasedGameState.h"
#include "Framework/PlayerState/ConnectIt_PlayerState.h"
#include "GameEvent/GameEventTaskSubsystem.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "Tile/GridTileRegistryBase.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"


AConnectIt_GameMode::AConnectIt_GameMode()
{
    GameStateClass  = AConnectIt_GameState::StaticClass();
    PlayerStateClass = AConnectIt_PlayerState::StaticClass();

    TurnDuration     = 90.f;
    ForfeitThreshold = 3;
    ReconnectTimeout = 30.f;
}

// --- Overrides ---

void AConnectIt_GameMode::PostLogin(APlayerController* NewPlayer)
{
    // Let base class handle reconnect detection and registration
    Super::PostLogin(NewPlayer);

    EnsureMatchSetupResolved();
    SeedActionStateForPlayer(NewPlayer);

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

void AConnectIt_GameMode::SeedActionStateForPlayer(APlayerController* NewPlayer)
{
    // The server owns each player's action state (uses, per-turn cap,
    // cooldowns) and loadout on their PlayerState, and this is the only
    // place a human's is seeded. Their own controller then builds its action
    // stack from the PlayerState's (replicated) loadout -- so without this
    // the player has no actions and every board request is rejected. AI
    // controllers seed themselves (they run with authority).
    if (!IsValid(NewPlayer)) return;

    ATurnBasedPlayerState* PS = NewPlayer->GetPlayerState<ATurnBasedPlayerState>();
    if (!IsValid(PS))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameMode: cannot seed action state -- %s has no "
                 "ATurnBasedPlayerState"),
            *GetNameSafe(NewPlayer));
        return;
    }

    // Already seeded (a reconnecting player keeps their state)
    if (PS->HasActionConfig()) return;

    if (!IsValid(PlayerLoadout))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameMode: cannot seed action state for %s -- no "
                 "level config, or it has no PlayerLoadout"),
            *PS->GetPlayerName());
        return;
    }

    PS->InitialiseActionState(PlayerLoadout);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_GameMode: seeded action state for %s from loadout "
             "'%s' (%d permanent, %d numbered) (server, PostLogin)"),
        *PS->GetPlayerName(), *PlayerLoadout->LoadoutName,
        PlayerLoadout->PermanentActions.Num(),
        PlayerLoadout->NumberedActions.Num());
}

void AConnectIt_GameMode::EnsureMatchSetupResolved()
{
    if (bMatchSetupResolved) return;
    bMatchSetupResolved = true;

    // The level's starting template -- read here, once, and nowhere else for
    // these values.
    const UConnectIt_LevelConfigDataAsset* LevelConfig =
        UConnectIt_LevelConfigSettings::FindLevelConfig(this);

    // Rules: a value copy (so changes never touch the asset), or the classic
    // defaults if there is no level config.
    Rules = IsValid(LevelConfig) ? LevelConfig->Rules : FConnectItRuleSet();
    PlayerLoadout = IsValid(LevelConfig) ? LevelConfig->PlayerLoadout : nullptr;
    AIProfile = IsValid(LevelConfig) ? LevelConfig->AIProfile : nullptr;

    // Main-menu match setup for this level, if any
    FConnectItMatchSettings MatchSettings;
    if (UConnectIt_MatchSetupSubsystem::GetSettingsForCurrentLevel(this, MatchSettings))
    {
        if (IsValid(MatchSettings.AIProfile))
        {
            AIProfile = MatchSettings.AIProfile;
        }

        if (MatchSettings.TargetScore > 0.f)
        {
            if (Rules.SetTargetScore(MatchSettings.TargetScore))
            {
                UE_LOG(LogTemp, Log,
                    TEXT("ConnectIt_GameMode: target score %.0f from match setup"),
                    MatchSettings.TargetScore);
            }
            else
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("ConnectIt_GameMode: match setup asked for target score "
                         "%.0f, but this level's win condition isn't score-based "
                         "-- ignored"),
                    MatchSettings.TargetScore);
            }
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_GameMode: match setup resolved -- player loadout '%s', "
             "AI profile '%s', target score %.0f"),
        IsValid(PlayerLoadout) ? *PlayerLoadout->LoadoutName : TEXT("none"),
        *GetNameSafe(AIProfile), Rules.GetTargetScore());

    PublishMatchSetup();
}

void AConnectIt_GameMode::PublishMatchSetup()
{
    AConnectIt_GameState* GS = GetGameState<AConnectIt_GameState>();
    if (!IsValid(GS))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameMode: PublishMatchSetup -- no AConnectIt_GameState, "
                 "clients will not see the match's rules"));
        return;
    }

    GS->SetMatchRules(Rules);

    // Only an Adventure match has an AI opponent to show
    GS->SetOpponentProfile(MatchType == EConnectItMatchType::Adventure ? AIProfile.Get() : nullptr);
}

void AConnectIt_GameMode::ModifyRules(const TFunctionRef<void(FConnectItRuleSet&)>& Change)
{
    EnsureMatchSetupResolved();

    Change(Rules);
    PublishMatchSetup();
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

    // Normally already done in PostLogin; a match can also start with nobody
    // logged in yet.
    EnsureMatchSetupResolved();

    BoardRequestMediator = NewObject<UConnectIt_BoardRequestMediator>(this);
    BoardRequestMediator->Initialise(&Rules);

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

    // TileRegistry lives on UConnectIt_BoardRegistrySubsystem now -- one
    // canonical per-world instance, initialised at OnWorldBeginPlay, well
    // before this function runs (called from HandleMatchHasStarted, off
    // PostLogin/ready-check completion).
    UGridTileRegistryBase* TileRegistry =
        UConnectIt_GameUtilityLibrary::GetTileRegistry(this);

    if (!IsValid(TileRegistry))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameMode: InitialiseBoard — "
                 "UConnectIt_BoardRegistrySubsystem has no valid TileRegistry"));
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

    const float InitialTargetScore = Rules.GetTargetScore();

    // PieceRegistry param is still unused inside InitialiseBoardState's body
    // (confirmed) -- passed through anyway now that a real one is available,
    // so this stops being an actively misleading "always null" call.
    BoardState->InitialiseBoardState(
        TileRegistry, UConnectIt_GameUtilityLibrary::GetPieceRegistry(this), NumFactions,
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
