// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/GameMode/ConnectIt_GameMode.h"
#include "EngineUtils.h"
#include "Board/ConnectIt_BoardRequestMediator.h"
#include "Board/ConnectIt_BoardStateComponent.h"
#include "ConnectIt_GameplayTags.h"
#include "Framework/Controller/ConnectIt_AIController.h"
#include "Action/ActionLoadoutDataAsset.h"
#include "Framework/Data/ConnectIt_LevelConfigDataAsset.h"
#include "Board/Powers/ConnectIt_TileCatalogDataAsset.h"
#include "Framework/Data/ConnectIt_WorldSettings.h"
#include "Framework/Data/ConnectIt_MatchPresetDataAsset.h"
#include "Framework/Subsystem/ConnectIt_MatchSetupSubsystem.h"
#include "Adventure/ConnectIt_AdventureSubsystem.h"
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
#include "Board/Operations/ConnectIt_BoardOperations.h"
#include "Board/Rules/Conditions/ConnectIt_MatchConditionDataAsset.h"
#include "Framework/Data/ConnectIt_BoardLayoutDataAsset.h"
#include "TimerManager.h"


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

    // An Adventure node sets the player up itself (open / closed, the saved
    // inventory, the node's item budget)
    UConnectIt_AdventureSubsystem* Adventure =
        GetGameInstance() ? GetGameInstance()->GetSubsystem<UConnectIt_AdventureSubsystem>() : nullptr;
    if (Adventure && Adventure->SeedPlayerState(PS, PlayerLoadout)) return;

    PS->InitialiseActionState(PlayerLoadout);

    UE_LOG(LogTemp, Log,
        TEXT("ConnectIt_GameMode: seeded action state for %s from loadout "
             "'%s' (%d permanent actions) (server, PostLogin)"),
        *PS->GetPlayerName(), *PlayerLoadout->LoadoutName,
        PlayerLoadout->PermanentActions.Num());
}

void AConnectIt_GameMode::EnsureMatchSetupResolved()
{
    if (bMatchSetupResolved) return;
    bMatchSetupResolved = true;

    // Build the board-operation catalog here, on the game thread, before any
    // background search can ask for it (FConnectItBoardOperation::GetLegality)
    FConnectItBoardOperation::GetAllOperationTags();

    // Main-menu match setup for this level, if any
    FConnectItMatchSettings MatchSettings;
    const bool bHasMatchSettings =
        UConnectIt_MatchSetupSubsystem::GetSettingsForCurrentLevel(this, MatchSettings);

    // How this match is played: the preset chosen in the menu, else the map's
    // own default (its World Settings). Read here, once, and nowhere else for
    // these values.
    const UConnectIt_MatchPresetDataAsset* Preset =
        bHasMatchSettings && IsValid(MatchSettings.MatchPreset) ? MatchSettings.MatchPreset.Get()
        : AConnectIt_WorldSettings::FindDefaultMatchPreset(this);

    // Rules: a value copy (so changes never touch the asset), or an empty
    // rule set (no win condition) if there is no preset.
    if (!IsValid(Preset))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameMode: no match preset -- the menu chose none and "
                 "the map's World Settings name no Default Match Preset. The match has no "
                 "rules, loadout or AI."));
    }
    Rules = IsValid(Preset) ? Preset->Rules : FConnectItRuleSet();
    LevelDirectorClass = IsValid(Preset) ? Preset->LevelDirectorClass : nullptr;
    PlayerDirectorClass = IsValid(Preset) ? Preset->PlayerDirectorClass : nullptr;
    AIDirectorClass = IsValid(Preset) ? Preset->AIDirectorClass : nullptr;

    // Both sides' starting goals, from the Level director's class (the level
    // can replace them later: SetConditions), copied into the plain groups
    // the match plays with
    const UConnectIt_LevelDirector* LevelDefaults = GetDefault<UConnectIt_LevelDirector>(
        LevelDirectorClass ? *LevelDirectorClass : UConnectIt_LevelDirector::StaticClass());
    Rules.ResolveConditionGroups(LevelDefaults->PlayerWinConditions, LevelDefaults->AIWinConditions,
        LevelDefaults->PlayerLoseConditions, LevelDefaults->AILoseConditions);

    // Every tile power the level's tile catalog defines, and which apply level-wide
    const UConnectIt_TileCatalogDataAsset* TileCatalog = UConnectIt_TileCatalogDataAsset::FindFor(this);
    Rules.SetTilePowers(TileCatalog ? TileCatalog->MakeTilePowers() : TArray<FConnectItTilePower>(),
        LevelDefaults->LevelWideTilePowers);

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
    EConnectItRole PlayerRole, UActionLoadoutDataAsset* Loadout, EConnectItChangeTiming Timing)
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
    ATurnBasedPlayerState* PlayerState, UActionLoadoutDataAsset* Loadout, EConnectItChangeTiming Timing)
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
    const bool bDefer = Timing == EConnectItChangeTiming::NextTurnStart || bMoveInProgress;

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

    // Level changes waiting for this turn, before anyone hears it started:
    // they join the waiting line and play out one at a time (the AI waits
    // for the board to settle before it decides)
    if (bStarted)
    {
        LevelChangesInARow = 0;
        LevelChanges.StartTurn();

        // The turn's Turn Start tile powers (e.g. Repel's countdown) come first,
        // as their own board change -- only when one is ready
        const UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
        if (IsValid(BoardState) && Rules.HasTurnStartTilePowerReady(BoardState->GetCurrentState(), TurnNumber))
        {
            FConnectItLevelChange TurnStart;
            TurnStart.Operation = FInstancedStruct::Make(FConnectItBoardOperation_TurnStart());
            TurnStart.AsFaction = Faction;
            LevelChanges.AddFirst(TurnStart);
        }
        if (!IsBoardChangeResolving())
        {
            DrainLevelChanges();
        }
    }

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
        else if (const FConnectItBoardEvent_LoseConditionMet* Lost = Entry.GetPtr<FConnectItBoardEvent_LoseConditionMet>())
        {
            ForEachDirector([Lost](UConnectIt_Director& Director)
            {
                Director.OnLoseConditionMet(Lost->Faction, Director.IsMine(Lost->Faction), Lost->Asset);
            });
        }
        else if (const FConnectItBoardEvent_PhaseChanged* PhaseChanged = Entry.GetPtr<FConnectItBoardEvent_PhaseChanged>())
        {
            UE_LOG(LogTemp, Log, TEXT("ConnectIt_GameMode: phase '%s' begins"), *PhaseChanged->Phase.ToString());

            const FGameplayTag Phase = PhaseChanged->Phase;
            ForEachDirector([Phase](UConnectIt_Director& Director)
            {
                Director.OnPhaseChanged(Phase);
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

    // The level's triggers, against the committed board
    if (IsValid(LevelDirector))
    {
        LevelDirector->CheckTriggers(BoardState->GetCurrentState(), Rules);
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
    // Lose conditions are for matches with an AI (versus: undecided)
    if (Rules.AIFaction == INDEX_NONE && !Rules.PlayerLoseGroups.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameMode: the Level director has Player Lose Conditions, but this match has no AI -- "
                 "they are ignored"));
    }

    if (Rules.PlayerWinGroups.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameMode: the Level director has no Player Win Conditions -- "
                 "the player can never win (does the match preset name a Level Director Class?)"));
    }

    // Only an AI in the match needs its own
    if (Rules.AIFaction != INDEX_NONE && Rules.AIWinGroups.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameMode: the Level director has no AI Win Conditions -- "
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
        BoardState->OnBoardChangeCommitted.AddUObject(this, &AConnectIt_GameMode::HandleBoardChangeCommitted);
    }
    ForEachDirector([](UConnectIt_Director& Director) { Director.OnMatchStarted(); });
}

void AConnectIt_GameMode::HandleMatchHasEnded()
{
    Super::HandleMatchHasEnded();

    // An Adventure node's result: won or lost, and the inventory the match
    // left (items spent, items granted) -- for the human player
    UConnectIt_AdventureSubsystem* Adventure =
        GetGameInstance() ? GetGameInstance()->GetSubsystem<UConnectIt_AdventureSubsystem>() : nullptr;
    const AConnectIt_GameState* GS = GetGameState<AConnectIt_GameState>();
    if (Adventure && GS && Adventure->GetPlayingNode(this))
    {
        for (APlayerState* Player : GS->PlayerArray)
        {
            const ATurnBasedPlayerState* PS = Cast<ATurnBasedPlayerState>(Player);
            if (!PS || PS->GetParticipantType() != EParticipantType::Human) continue;

            const int32 Winner = GS->MatchResult.WinningFactionSlot;
            Adventure->RecordPlayingNodeResult(this, Winner != -1 && Winner == PS->GetSlotIndex(), PS->GetInventory());
            break;
        }
    }

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

    // One board change at a time: a request before the board has settled
    // shouldn't happen (the AI waits for it; clients are told the board is
    // busy) -- refused, and the requester's action re-enables
    if (!IsBoardSettled())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameMode: ProcessBoardRequest -- '%s' by faction %d refused: the board hasn't "
                 "settled (a change's visuals are still playing, or level changes are waiting)"),
            *Request.ActionTag.ToString(), RequestingFaction);
        return false;
    }

    // Level changes asked for while the move is carried out wait for it to
    // play out (released by HandleBoardChangeComplete)
    const bool bSucceeded = BoardRequestMediator->ProcessRequest(Request, RequestingFaction);
    if (bSucceeded)
    {
        LevelChangesInARow = 0;
    }
    return bSucceeded;
}

// --- Level changes ---

void AConnectIt_GameMode::ApplyLevelOperation(const FInstancedStruct& Operation, int32 AsFaction, EConnectItChangeTiming Timing)
{
    if (!Operation.GetPtr<FConnectItBoardOperation>())
    {
        UE_LOG(LogTemp, Warning, TEXT("ConnectIt_GameMode: ApplyLevelOperation -- not a board operation, ignored"));
        return;
    }

    FConnectItLevelChange Change;
    Change.Operation = Operation;
    Change.AsFaction = AsFaction;
    QueueLevelChange(Change, Timing);
}

bool AConnectIt_GameMode::SetScoringRule(const FInstancedStruct& ScoringRule, EConnectItChangeTiming Timing)
{
    if (!FConnectItRuleSet::IsScoringRule(ScoringRule))
    {
        UE_LOG(LogTemp, Warning, TEXT("ConnectIt_GameMode: SetScoringRule -- not a scoring rule struct, ignored"));
        return false;
    }

    FConnectItLevelChange Change;
    Change.ScoringRule = ScoringRule;
    QueueLevelChange(Change, Timing);
    return true;
}

void AConnectIt_GameMode::SetConditions(EConnectItRole Side, bool bWinGoals,
    const TArray<UConnectIt_MatchConditionDataAsset*>& Conditions, EConnectItChangeTiming Timing)
{
    FConnectItLevelChange Change;
    Change.bHasGoals = true;
    Change.bGoalsForAI = Side == EConnectItRole::AI;
    Change.bWinGoals = bWinGoals;
    for (UConnectIt_MatchConditionDataAsset* Condition : Conditions)
    {
        Change.Goals.Add(Condition);
    }
    QueueLevelChange(Change, Timing);
}

void AConnectIt_GameMode::SetLayout(UConnectIt_BoardLayoutDataAsset* Layout, EConnectItChangeTiming Timing)
{
    if (!IsValid(Layout))
    {
        UE_LOG(LogTemp, Warning, TEXT("ConnectIt_GameMode: SetLayout called with no layout -- ignored"));
        return;
    }

    FConnectItLevelChange Change;
    Change.bHasLayout = true;
    Change.Layout = Layout;
    QueueLevelChange(Change, Timing);
}

void AConnectIt_GameMode::QueueLevelChange(const FConnectItLevelChange& Change, EConnectItChangeTiming Timing)
{
    if (LevelChanges.Add(Change, Timing, IsBoardChangeResolving()))
    {
        LevelChangesInARow++;
        ApplyLevelChange(Change);
    }
}

// --- One board change at a time ---

bool AConnectIt_GameMode::IsBoardChangeResolving() const
{
    return bBoardChangeResolving || (IsValid(BoardRequestMediator) && BoardRequestMediator->IsDispatching());
}

bool AConnectIt_GameMode::IsBoardSettled() const
{
    return !IsBoardChangeResolving() && !LevelChanges.HasWaiting();
}

void AConnectIt_GameMode::HandleBoardChangeCommitted()
{
    NumBoardChanges++;
    NumChangesPlaying++;
    bBoardChangeResolving = true;

    // Restarted by every change: the timeout is for the latest one
    GetWorldTimerManager().SetTimer(BoardChangeTimeoutHandle,
        FTimerDelegate::CreateUObject(this, &AConnectIt_GameMode::HandleBoardChangeTimeout),
        BoardChangeTimeoutSeconds, false);

    PublishBoardChangeResolving();
}

void AConnectIt_GameMode::HandleBoardChangeComplete(FGameplayTag Tag)
{
    NumChangesPlaying = FMath::Max(NumChangesPlaying - 1, 0);

    // Markers complete in order: none left playing is the end of the last
    // committed change. (After a timeout the lock is already off; a late
    // marker just catches the count up.)
    if (NumChangesPlaying == 0 && bBoardChangeResolving)
    {
        ReleaseBoardChangeLock();
    }
}

void AConnectIt_GameMode::ReleaseBoardChangeLock()
{
    bBoardChangeResolving = false;
    GetWorldTimerManager().ClearTimer(BoardChangeTimeoutHandle);

    // Next tick: the marker can complete inside the call that committed the
    // change (no visuals registered), where nothing new may be carried out
    GetWorldTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(this, &AConnectIt_GameMode::DrainLevelChanges));

    // Still busy for clients if level changes wait (the drain lifts it)
    PublishBoardChangeResolving();
}

void AConnectIt_GameMode::PublishBoardChangeResolving() const
{
    if (AConnectIt_GameState* GS = GetGameState<AConnectIt_GameState>())
    {
        GS->SetBoardChangeResolving(!IsBoardSettled());
    }
}

void AConnectIt_GameMode::HandleBoardChangeTimeout()
{
    if (!bBoardChangeResolving) return;

    FString Queued;
    if (UGameEventTaskSubsystem* GameEventSubsystem = GetWorld()->GetSubsystem<UGameEventTaskSubsystem>())
    {
        for (const FGameplayTag& QueuedTag : GameEventSubsystem->GetTagsInQueue())
        {
            Queued += (Queued.IsEmpty() ? TEXT("") : TEXT(", ")) + QueuedTag.ToString();
        }
    }
    UE_LOG(LogTemp, Error,
        TEXT("ConnectIt_GameMode: a board change has been resolving for %.0f s -- releasing the lock. A visual "
             "task never reported complete, so the event queue behind it is stuck too (queued: %s)"),
        BoardChangeTimeoutSeconds, Queued.IsEmpty() ? TEXT("nothing") : *Queued);

    ReleaseBoardChangeLock();
}

void AConnectIt_GameMode::DrainLevelChanges()
{
    // A level whose triggers keep feeding each other is stopped rather than
    // looping forever
    constexpr int32 MaxInARow = 64;

    const int32 ChangesBefore = NumBoardChanges;
    FConnectItLevelChange Change;
    while (!IsBoardChangeResolving() && NumBoardChanges == ChangesBefore && LevelChanges.TakeNext(Change))
    {
        if (++LevelChangesInARow > MaxInARow)
        {
            const int32 Dropped = 1 + LevelChanges.DiscardWaiting();
            UE_LOG(LogTemp, Error,
                TEXT("ConnectIt_GameMode: more than %d level changes in a row -- %d dropped (do the level's "
                     "triggers keep setting each other off?)"),
                MaxInARow, Dropped);
            break;
        }

        // One that changes the board stops the loop: the next waits for it
        // to play out. A scoring rule change (or a refused operation) doesn't.
        ApplyLevelChange(Change);
    }

    PublishBoardChangeResolving();
    if (IsBoardSettled())
    {
        OnBoardChangeSettled.Broadcast();
    }
}

void AConnectIt_GameMode::ApplyLevelChange(const FConnectItLevelChange& Change)
{
    if (Change.ScoringRule.IsValid())
    {
        ModifyRules([&Change](FConnectItRuleSet& MatchRules)
        {
            MatchRules.SetScoringRuleFrom(Change.ScoringRule);
        });
        UE_LOG(LogTemp, Log, TEXT("ConnectIt_GameMode: scoring rule is now '%s'"),
            *GetNameSafe(Change.ScoringRule.GetScriptStruct()));
        return;
    }

    if (Change.bHasGoals)
    {
        ApplyGoalsChange(Change);
        return;
    }

    if (Change.bHasLayout)
    {
        ApplyLayoutChange(Change);
        return;
    }

    if (!IsValid(BoardRequestMediator)) return;

    const bool bApplied = BoardRequestMediator->ProcessLevelOperation(Change.Operation, Change.AsFaction);

    if (!bApplied)
    {
        UE_LOG(LogTemp, Warning, TEXT("ConnectIt_GameMode: a level operation could not be carried out (see above)"));
    }
}

void AConnectIt_GameMode::ApplyLayoutChange(const FConnectItLevelChange& Change)
{
    const UConnectIt_BoardLayoutDataAsset* Layout = Change.Layout.Get();
    const UConnectIt_BoardStateComponent* BoardState = UConnectIt_GameUtilityLibrary::GetBoardStateComponent(this);
    if (!IsValid(Layout) || !IsValid(BoardState) || !IsValid(BoardRequestMediator))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameMode: layout change skipped -- the layout is gone, or there is no board"));
        return;
    }

    // Against the board as it is now -- other changes may have played since
    // this was asked for
    const FConnectItBoardOperation_ChangeLayout Operation = Layout->MakeChangeFrom(BoardState->GetCurrentState());
    if (!BoardRequestMediator->ProcessLevelOperation(FInstancedStruct::Make(Operation), INDEX_NONE))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameMode: layout '%s' could not be applied (see above)"), *Layout->GetName());
    }
}

void AConnectIt_GameMode::ApplyGoalsChange(const FConnectItLevelChange& Change)
{
    const TCHAR* What = Change.bWinGoals ? TEXT("win goals") : TEXT("lose conditions");
    if (Change.bGoalsForAI && Rules.AIFaction == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameMode: new AI %s ignored -- this match has no AI"), What);
        return;
    }

    // An asset gone since it was asked for resolves as an empty entry
    // (skipped, with a warning)
    TArray<TObjectPtr<UConnectIt_MatchConditionDataAsset>> Conditions;
    for (const TWeakObjectPtr<UConnectIt_MatchConditionDataAsset>& Goal : Change.Goals)
    {
        Conditions.Add(Goal.Get());
    }

    // 1. The rules (they replicate; the AI uses them from its next decision)
    ModifyRules([&Change, &Conditions](FConnectItRuleSet& MatchRules)
    {
        MatchRules.SetConditionGroups(Change.bGoalsForAI, Change.bWinGoals, Conditions);
    });

    // 2. The board: that side's progress restarts and the change is played
    //    like any other, followed by the win check against the new goals
    FConnectItBoardOperation_ChangeGoals Operation;
    Operation.bWinGoals = Change.bWinGoals;
    for (const FConnectItMatchConditionGroup& Group : Rules.GetConditionGroups(Change.bGoalsForAI, Change.bWinGoals))
    {
        Operation.GroupNames.Add(Group.Name);
    }
    if (Change.bGoalsForAI)
    {
        Operation.Factions.Add(Rules.AIFaction);
    }
    else
    {
        // Every human faction (both, in a match with no AI)
        for (int32 Faction = 0; Faction < NumFactions; Faction++)
        {
            if (Faction != Rules.AIFaction) Operation.Factions.Add(Faction);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("ConnectIt_GameMode: %s %s replaced (%d)"),
        Change.bGoalsForAI ? TEXT("AI") : TEXT("player"), What, Operation.GroupNames.Num());

    if (!IsValid(BoardRequestMediator)
        || !BoardRequestMediator->ProcessLevelOperation(FInstancedStruct::Make(Operation), INDEX_NONE))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_GameMode: the goals changed in the rules, but the board change could not be carried out "
                 "(see above) -- progress wasn't restarted"));
    }
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

        // The end of each board change (see IsBoardChangeResolving)
        GameEventSubsystem->BindOnTagComplete(
            ConnectIt_Event_BoardChangeComplete, this,
            GET_FUNCTION_NAME_CHECKED(AConnectIt_GameMode, HandleBoardChangeComplete));
    }

    const float InitialTargetScore = Rules.GetTargetScore();

    // PieceRegistry param is still unused inside InitialiseBoardState's body
    // (confirmed) -- passed through anyway now that a real one is available,
    // so this stops being an actively misleading "always null" call.
    BoardState->InitialiseBoardState(
        TileRegistry, UConnectIt_GameUtilityLibrary::GetPieceRegistry(this), NumFactions,
        /*InitialMultiplier=*/1.0f, InitialTargetScore, &Rules);

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
    const bool bByLoss = IsValid(BoardState) && BoardState->GetCurrentState().LosingFactionSlot != -1;

    UE_LOG(LogTemp, Log,
       TEXT("ConnectIt_GameMode: Game over — faction %d wins"),
       WinningFactionSlot);

    // Write result to GameState -- replicates to all clients
    if (AConnectIt_GameState* GS = GetGameState<AConnectIt_GameState>())
    {
        GS->SetMatchResult(
            WinningFactionSlot,
            bByLoss ? EMatchEndReason::OpponentLost : EMatchEndReason::ScoreThresholdReached,
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
