// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/ConnectIt_AIStrategy_MinMax.h"
#include "ConnectIt_GameplayTags.h"


void UConnectIt_AIStrategy_MinMax::BeginDecision_Implementation(
    const FConnectItAIDecisionContext& Context)
{
    // Negamax is a two-player search; Classic is defined as two factions.
    if (Context.Board.ScoreBoard.Num() != 2 || !Context.Board.ScoreBoard.IsValidIndex(Context.OwnSlot))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIStrategy_MinMax: supports exactly 2 factions "
                 "(board has %d, own slot %d) -- no move"),
            Context.Board.ScoreBoard.Num(), Context.OwnSlot);
        FinishDecision(FConnectItAIDecision());
        return;
    }

    namespace MinMax = GameIntelligence::Search::MinMax;

    // Everything the search reads is built here, on the game thread, as
    // plain data -- the search never touches this UObject.
    if (EvaluationWeights.IsAllZero())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ConnectIt_AIStrategy_MinMax: every evaluation weight is 0 -- "
                 "every non-winning position looks equal, the AI will only see "
                 "wins and losses"));
    }

    // Which kinds of move each side plays with in the search: the ones the
    // search can model (FConnectItMinMaxMove -- Place Piece only for now)
    // that the side's loadout grants.
    FGameplayTagContainer OwnRequestTypes;
    FGameplayTagContainer OpponentRequestTypes;
    if (LoadoutGrantsRequestType(Context.OwnLoadout, ConnectIt_Game_PlacePiece))
    {
        OwnRequestTypes.AddTag(ConnectIt_Game_PlacePiece);
    }
    if (LoadoutGrantsRequestType(Context.OpponentLoadout, ConnectIt_Game_PlacePiece))
    {
        OpponentRequestTypes.AddTag(ConnectIt_Game_PlacePiece);
    }

    if (OwnRequestTypes.IsEmpty())
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConnectIt_AIStrategy_MinMax: this AI's loadout grants no move "
                 "the search can make (it needs a Place Piece action) -- no move"));
        FinishDecision(FConnectItAIDecision());
        return;
    }

    const bool bOwnIsSlot0 = Context.OwnSlot == 0;
    const TSharedRef<const FConnectItMinMaxRules, ESPMode::ThreadSafe> Rules =
        MakeShared<const FConnectItMinMaxRules, ESPMode::ThreadSafe>(
            Context.Board,
            Context.Rules,
            EvaluationWeights,
            OrderingWeights,
            bOwnIsSlot0 ? OwnRequestTypes : OpponentRequestTypes,
            bOwnIsSlot0 ? OpponentRequestTypes : OwnRequestTypes);

    FConnectItMinMaxRules::FState Root = Rules->MakeRoot(Context.Board, Context.OwnSlot);

    MinMax::FParams Params;
    Params.MaxDepth = FMath::Max(1, MaxDepth);
    Params.TimeBudgetSeconds = FMath::Max(0.f, TimeBudgetSeconds);

    const GameIntelligence::Search::FCancelFlag Flag = GameIntelligence::Search::MakeCancelFlag();
    CancelFlag = Flag;

    const int32 DecisionId = GetActiveDecisionId();
    TWeakObjectPtr<UConnectIt_AIStrategy_MinMax> WeakThis(this);

    MinMax::LaunchAlphaBetaAsync<FConnectItMinMaxRules>(
        Rules, MoveTemp(Root), Params, Flag,
        [WeakThis, DecisionId](FConnectItMinMaxSearch::FResult&& Result)
        {
            UConnectIt_AIStrategy_MinMax* Self = WeakThis.Get();
            if (!Self || Result.bCancelled || !Self->IsDecisionActive(DecisionId))
            {
                return;
            }

            Self->CancelFlag.Reset();

            if (Result.RootScores.IsEmpty())
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("ConnectIt_AIStrategy_MinMax: search found no moves "
                         "(board full?) -- no move"));
                Self->FinishDecision(FConnectItAIDecision());
                return;
            }

            const int32 PickIndex = Self->PickMoveIndex(Result);

            // The chosen move already is the board change to request --
            // whichever kind of operation it is
            FConnectItAIDecision Decision;
            FString MoveText;
            Visit([&Decision, &MoveText](const auto& Operation)
            {
                Decision.RequestType = Operation.GetRequestType();
                Decision.Payload = FInstancedStruct::Make(Operation);
                MoveText = Operation.Describe();
            }, Result.RootScores[PickIndex].Move);

            Decision.Summary = FString::Printf(
                TEXT("MinMax depth %d%s, %lld nodes in %.0f ms -- %s %s "
                     "score %d (rank %d of %d, best %d)"),
                Result.DepthReached, Result.bOutOfTime ? TEXT(" (out of time)") : TEXT(""),
                Result.NodesVisited, Result.ElapsedSeconds * 1000.0,
                *Decision.RequestType.ToString(), *MoveText,
                Result.RootScores[PickIndex].Score,
                PickIndex + 1, Result.RootScores.Num(), Result.RootScores[0].Score);

            Self->FinishDecision(Decision);
        });
}

void UConnectIt_AIStrategy_MinMax::CancelDecision_Implementation()
{
    if (CancelFlag.IsValid())
    {
        CancelFlag->store(true);
        CancelFlag.Reset();
    }
}

int32 UConnectIt_AIStrategy_MinMax::PickMoveIndex(const FConnectItMinMaxSearch::FResult& Result) const
{
    const TArray<GameIntelligence::Search::MinMax::TScoredMove<FConnectItMinMaxRules::FMove>>& Scores =
        Result.RootScores;

    const int32 TopMoves = FMath::Min(TopMovesConsidered, Scores.Num());
    if (TopMoves > 1 && MistakeChance > 0.f && FMath::FRand() < MistakeChance)
    {
        return FMath::RandRange(0, TopMoves - 1);
    }

    int32 NumTiedBest = 1;
    while (NumTiedBest < Scores.Num() && Scores[NumTiedBest].Score == Scores[0].Score)
    {
        NumTiedBest++;
    }
    return FMath::RandRange(0, NumTiedBest - 1);
}
