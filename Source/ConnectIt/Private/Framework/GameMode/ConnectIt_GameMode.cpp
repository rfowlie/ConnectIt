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
#include "Framework/Data/ConnectIt_MatchPreset.h"
#include "Framework/Subsystem/ConnectIt_MatchSetupSubsystem.h"
#include "Framework/GameState/ConnectIt_GameState.h"
#include "Framework/GameState/TurnBasedGameState.h"
#include "Framework/PlayerState/ConnectIt_PlayerState.h"
#include "Framework/PlayerState/TurnBasedPlayerState.h"
#include "GameEvent/GameEventTaskSubsystem.h"
#include "Framework/Library/ConnectIt_GameUtilityLibrary.h"
#include "Framework/Library/ConnectIt_BoardEventLibrary.h"
#include "Tile/GridTileRegistryBase.h"
#include "Turn/Participant/TurnBasedParticipantManagerComponent.h"
#include "AI/ConnectIt_AIDirector.h"
#include "AI/ConnectIt_PlayerDirector.h"
#include "AI/ConnectIt_LevelDirector.h"
#include "Board/Events/ConnectIt_BoardEvents.h"
#include "Board/Operations/ConnectIt_BoardOperation.h"


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

    // Every human has a Player director (a reconnecting player keeps theirs)
    const UConnectIt_PlayerDirector* PlayerDirector = FindOrCreatePlayerDirector(PS);

    // Already seeded (a reconnecting player keeps their state)
    if (PS->HasActionConfig()) return;

    UActionLoadoutDataAsset* PlayerLoadout = IsValid(PlayerDirector) ? PlayerDirector->StartingLoadout.Get() : nullptr;
    if (!IsValid(PlayerLoadout))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameMode: cannot seed action state for %s -- the "
                 "match preset's Player Director Class has no Starting Loadout"),
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

    // Build the board-operation catalog here, on the game thread, before any
    // background search can ask for it (FConnectItBoardOperation::GetLegality)
    FConnectItBoardOperation::GetAllRequestTypes();

    // Main-menu match setup for this level, if any
    FConnectItMatchSettings MatchSettings;
    const bool bHasMatchSettings =
        UConnectIt_MatchSetupSubsystem::GetSettingsForCurrentLevel(this, MatchSettings);

    // How this match is played: the preset chosen in the menu, else the
    // level's own default. Read here, once, and nowhere else for these values.
    const UConnectIt_LevelConfigDataAsset* LevelConfig =
        UConnectIt_LevelConfigSettings::FindLevelConfig(this);
    const UConnectIt_MatchPreset* Preset =
        bHasMatchSettings && IsValid(MatchSettings.MatchPreset) ? MatchSettings.MatchPreset.Get()
        : IsValid(LevelConfig) ? LevelConfig->DefaultMatchPreset.Get()
        : nullptr;

    // Rules: a value copy (so changes never touch the asset), or an empty
    // rule set (no win condition) if there is no preset.
    if (!IsValid(Preset))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameMode: no match preset -- the menu chose none and "
                 "the level config has no DefaultMatchPreset. The match has no "
                 "rules, loadout or AI."));
    }
    Rules = IsValid(Preset) ? Preset->Rules : FConnectItRuleSet();
    LevelDirectorClass = IsValid(Preset) ? Preset->LevelDirectorClass : nullptr;
    PlayerDirectorClass = IsValid(Preset) ? Preset->PlayerDirectorClass : nullptr;
    AIDirectorClass = IsValid(Preset) ? Preset->AIDirectorClass : nullptr;

    // The directors' goals (their classes' -- goals are fixed for the match),
    // copied into the plain groups the match plays with
    const UConnectIt_PlayerDirector* PlayerDefaults = GetDefault<UConnectIt_PlayerDirector>(
        PlayerDirectorClass ? *PlayerDirectorClass : UConnectIt_PlayerDirector::StaticClass());
    const UConnectIt_AIDirector* AIDefaults = GetDefault<UConnectIt_AIDirector>(
        AIDirectorClass ? *AIDirectorClass : UConnectIt_AIDirector::StaticClass());
    Rules.ResolveConditionGroups(PlayerDefaults->WinConditions, AIDefaults->WinConditions);

    // Tweaks from the menu, on top of whichever preset plays
    if (bHasMatchSettings)
    {
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
        TEXT("ConnectIt_GameMode: match setup resolved -- preset '%s', directors: "
             "level '%s', player '%s', AI '%s', target score %.0f"),
        *GetNameSafe(Preset),
        LevelDirectorClass ? *LevelDirectorClass->GetName() : TEXT("none"),
        PlayerDirectorClass ? *PlayerDirectorClass->GetName() : TEXT("none"),
        AIDirectorClass ? *AIDirectorClass->GetName() : TEXT("none"),
        Rules.GetTargetScore());

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
}

void AConnectIt_GameMode::ModifyRules(const TFunctionRef<void(FConnectItRuleSet&)>& Change)
{
    EnsureMatchSetupResolved();

    Change(Rules);
    PublishMatchSetup();
}

void AConnectIt_GameMode::SetLoadout(
    EConnectItRole PlayerRole, UActionLoadoutDataAsset* Loadout, EConnectItLoadoutTiming Timing)
{
    UTurnBasedParticipantManagerComponent* Manager = GetParticipantManager();
    if (!IsValid(Manager)) return;

    for (const FTurnParticipantInfo& Info : Manager->GetAllParticipants())
    {
        const bool bIsAI = Info.ParticipantType == EParticipantType::AI;
        if (bIsAI != (PlayerRole == EConnectItRole::AI)) continue;
        SetLoadoutForPlayer(Info.PlayerState, Loadout, Timing);
    }
}

void AConnectIt_GameMode::SetLoadoutForPlayer(
    ATurnBasedPlayerState* PlayerState, UActionLoadoutDataAsset* Loadout, EConnectItLoadoutTiming Timing)
{
    if (!IsValid(Loadout))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameMode: SetLoadout called with no loadout -- ignored"));
        return;
    }
    if (!IsValid(PlayerState)) return;

    // A move in progress spends its action's use after it finishes, against
    // the loadout it came from: never swap it out from under that.
    const bool bMoveInProgress = IsValid(BoardRequestMediator) && BoardRequestMediator->IsDispatching();
    const bool bDefer = Timing == EConnectItLoadoutTiming::NextTurnStart || bMoveInProgress;

    if (bDefer)
    {
        PendingLoadouts.Add(PlayerState->GetSlotIndex(), Loadout);
    }
    else
    {
        PendingLoadouts.Remove(PlayerState->GetSlotIndex());
        ApplyLoadout(PlayerState, Loadout);
    }
}

// --- Directors ---

UConnectIt_PlayerDirector* AConnectIt_GameMode::GetPlayerDirector(int32 Faction) const
{
    for (UConnectIt_PlayerDirector* PlayerDirector : PlayerDirectors)
    {
        if (IsValid(PlayerDirector) && PlayerDirector->GetFaction() == Faction) return PlayerDirector;
    }
    return nullptr;
}

UConnectIt_AIDirector* AConnectIt_GameMode::GetAIDirector() const
{
    const AConnectIt_AIController* AIController = SpawnedAIController.Get();
    return IsValid(AIController) ? AIController->GetDirector() : nullptr;
}

UConnectIt_PlayerDirector* AConnectIt_GameMode::FindOrCreatePlayerDirector(ATurnBasedPlayerState* PlayerState)
{
    if (!IsValid(PlayerState)) return nullptr;

    for (UConnectIt_PlayerDirector* PlayerDirector : PlayerDirectors)
    {
        if (IsValid(PlayerDirector) && PlayerDirector->GetDirectedPlayerState() == PlayerState) return PlayerDirector;
    }

    EnsureMatchSetupResolved();
    if (!PlayerDirectorClass)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameMode: the match preset has no Player Director Class -- %s gets the "
                 "default one, which has no loadout"),
            *PlayerState->GetPlayerName());
    }
    UConnectIt_PlayerDirector* PlayerDirector = NewObject<UConnectIt_PlayerDirector>(
        this, PlayerDirectorClass ? *PlayerDirectorClass : UConnectIt_PlayerDirector::StaticClass());
    PlayerDirector->SetDirectedPlayerState(PlayerState);
    PlayerDirectors.Add(PlayerDirector);

    UE_LOG(LogTemp, Log, TEXT("ConnectIt_GameMode: player director '%s' for %s"),
        *PlayerDirector->GetClass()->GetName(), *PlayerState->GetPlayerName());
    return PlayerDirector;
}

void AConnectIt_GameMode::ForEachDirector(TFunctionRef<void(UConnectIt_Director&)> Event) const
{
    TArray<UConnectIt_PlayerDirector*> Players;
    for (UConnectIt_PlayerDirector* PlayerDirector : PlayerDirectors)
    {
        Players.Add(PlayerDirector);
    }

    // A director may react by asking another for something, never by adding
    // or removing directors, so the list is fixed for the call
    for (UConnectIt_Director* Director : UConnectIt_Director::InDispatchOrder(LevelDirector, Players, GetAIDirector()))
    {
        Event(*Director);
    }
}

void AConnectIt_GameMode::HandleTurnPhaseChanged(ETurnPhase NewPhase)
{
    if (NewPhase != ETurnPhase::TurnStart && NewPhase != ETurnPhase::TurnEnd) return;

    const ATurnBasedGameState* GS = GetGameState<ATurnBasedGameState>();
    if (!IsValid(GS)) return;

    bool bValid = false;
    const FTurnParticipantInfo Active = GS->GetActiveParticipant(bValid);
    if (!bValid) return;

    const int32 Faction = Active.SlotIndex;
    const int32 TurnNumber = GS->GetActiveTurnNumber();
    const bool bStarted = NewPhase == ETurnPhase::TurnStart;

    ForEachDirector([Faction, TurnNumber, bStarted](UConnectIt_Director& Director)
    {
        if (bStarted)
        {
            Director.OnTurnStarted(Faction, Director.IsMine(Faction), TurnNumber);
        }
        else
        {
            Director.OnTurnEnded(Faction, Director.IsMine(Faction), TurnNumber);
        }
    });
}

void AConnectIt_GameMode::HandleBoardStateChangedForDirectors()
{
    const UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    if (!IsValid(BoardState)) return;

    for (const FInstancedStruct& Entry : BoardState->GetChangeEvent().Events)
    {
        if (const FConnectItBoardEvent_WinCheckpointReached* Checkpoint =
            Entry.GetPtr<FConnectItBoardEvent_WinCheckpointReached>())
        {
            UE_LOG(LogTemp, Log,
                TEXT("ConnectIt_GameMode: faction %d completed win group %d ('%s')"),
                Checkpoint->Faction, Checkpoint->GroupIndex, *Checkpoint->GroupName.ToString());

            ForEachDirector([Checkpoint](UConnectIt_Director& Director)
            {
                Director.OnWinCheckpointReached(Checkpoint->Faction, Director.IsMine(Checkpoint->Faction),
                    Checkpoint->Asset, Checkpoint->GroupIndex, Checkpoint->GroupName);
            });
        }
        else if (const FConnectItBoardEvent_GameWon* Won = Entry.GetPtr<FConnectItBoardEvent_GameWon>())
        {
            const int32 Winner = Won->WinningFaction;
            ForEachDirector([Winner](UConnectIt_Director& Director)
            {
                Director.OnMatchWon(Winner, Director.IsMine(Winner));
            });
        }
    }
}

void AConnectIt_GameMode::ApplyLoadout(ATurnBasedPlayerState* PlayerState, UActionLoadoutDataAsset* Loadout)
{
    if (!IsValid(PlayerState) || !IsValid(Loadout)) return;

    PlayerState->InitialiseActionState(Loadout);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_GameMode: '%s' now has loadout '%s'"),
        *PlayerState->GetPlayerName(), *Loadout->LoadoutName);
}

void AConnectIt_GameMode::HandleActiveControllerChanged(AController* NewActiveController)
{
    if (!IsValid(NewActiveController) || PendingLoadouts.IsEmpty()) return;

    ATurnBasedPlayerState* PlayerState = NewActiveController->GetPlayerState<ATurnBasedPlayerState>();
    if (!IsValid(PlayerState)) return;

    // Its turn is beginning: give it the loadout that was waiting
    TObjectPtr<UActionLoadoutDataAsset> Pending;
    if (PendingLoadouts.RemoveAndCopyValue(PlayerState->GetSlotIndex(), Pending))
    {
        ApplyLoadout(PlayerState, Pending);
    }
}

void AConnectIt_GameMode::WarnAboutMissingWinConditions() const
{
    if (Rules.PlayerWinGroups.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameMode: the Player director has no Win Conditions -- "
                 "the player can never win"));
    }

    // Only an AI in the match needs its own
    if (Rules.AIFaction != INDEX_NONE && Rules.AIWinGroups.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameMode: the AI director has no Win Conditions -- "
                 "the AI opponent can never win (it does not borrow the player's)"));
    }

    // A group that can never be satisfied blocks every group after it
    auto WarnAboutEmptyGroups = [](const TCHAR* Side, const TArray<FConnectItMatchConditionGroup>& Groups)
    {
        for (int32 Index = 0; Index < Groups.Num(); Index++)
        {
            if (Groups[Index].IsEmpty())
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("ConnectIt_GameMode: %s win condition %d ('%s') has no conditions -- "
                         "it can never be completed, so that side can never win"),
                    Side, Index, *Groups[Index].Name.ToString());
            }
        }
    };
    WarnAboutEmptyGroups(TEXT("Player"), Rules.PlayerWinGroups);
    WarnAboutEmptyGroups(TEXT("AI"), Rules.AIWinGroups);
}

void AConnectIt_GameMode::HandleMatchHasStarted()
{
    // Base class applies turn config to participant manager
    Super::HandleMatchHasStarted();

    if (UTurnBasedParticipantManagerComponent* Manager = GetParticipantManager())
    {
        Manager->OnInvalidNumberOfPlayers.AddDynamic(
            this, &AConnectIt_GameMode::HandleInvalidNumberOfPlayers);
        Manager->OnActiveControllerChanged.AddDynamic(
            this, &AConnectIt_GameMode::HandleActiveControllerChanged);
        Manager->OnTurnPhaseChanged.AddDynamic(
            this, &AConnectIt_GameMode::HandleTurnPhaseChanged);
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

    // The AI's slot is known now, so both sides can be checked
    WarnAboutMissingWinConditions();

    // Initialise board -- reads tile positions from registry
    InitialiseBoard();

    // The level's script, then every director hears the match has started
    // (board ready, every side registered)
    if (LevelDirectorClass)
    {
        LevelDirector = NewObject<UConnectIt_LevelDirector>(this, LevelDirectorClass);
        UE_LOG(LogTemp, Log, TEXT("ConnectIt_GameMode: level director '%s'"), *LevelDirectorClass->GetName());
    }
    if (UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this))
    {
        BoardState->OnBoardStateChanged.AddUniqueDynamic(this, &AConnectIt_GameMode::HandleBoardStateChangedForDirectors);
    }
    ForEachDirector([](UConnectIt_Director& Director) { Director.OnMatchStarted(); });
}

void AConnectIt_GameMode::HandleMatchHasEnded()
{
    Super::HandleMatchHasEnded();

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_GameMode: Match ended"));
}

bool AConnectIt_GameMode::ProcessBoardRequest(const FTurnActionRequest& Request, int32 RequestingFaction)
{
    if (!IsValid(BoardRequestMediator))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_GameMode: ProcessBoardRequest — "
                 "BoardRequestMediator is null"));
        return false;
    }

    return BoardRequestMediator->ProcessRequest(Request, RequestingFaction);
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
    SpawnedAIController = AIController;

    // The rule set only knows slots: tell it which one is the AI's, so the
    // AI's win condition (if the level gave it one) applies to that faction.
    // The slot is assigned by registration, so it can only be read now.
    const ATurnBasedPlayerState* AIState = AIController->GetPlayerState<ATurnBasedPlayerState>();
    const int32 AISlot = IsValid(AIState) ? AIState->GetSlotIndex() : INDEX_NONE;
    if (AISlot != INDEX_NONE)
    {
        ModifyRules([AISlot](FConnectItRuleSet& MatchRules)
        {
            MatchRules.AIFaction = AISlot;
        });
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameMode: the AI has no participant slot after "
                 "registering -- the level's AI win condition (if any) will not apply"));
    }

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_GameMode: AI controller spawned and registered "
             "as '%s' (faction %d)"),
        *AIDisplayName, AISlot);
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
