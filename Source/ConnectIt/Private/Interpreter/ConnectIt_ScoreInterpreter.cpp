// Fill out your copyright notice in the Description page of Project Settings.


#include "Interpreter/ConnectIt_ScoreInterpreter.h"
#include "Board/ConnectIt_BoardStateComponent.h"


void UConnectIt_ScoreInterpreter::OnBoardStateChanged_Implementation(
	const UBoardStateComponentBase* Component)
{
	const UConnectIt_BoardStateComponent* ConnectItComp =
		Cast<UConnectIt_BoardStateComponent>(Component);
	if (!IsValid(ConnectItComp)) return;

	const FConnectItBoardState Previous = *ConnectItComp->GetBoardSnapShotPrevious();
	const FConnectItBoardState Current = *ConnectItComp->GetBoardSnapShotCurrent();

	// Check each faction for score changes
	for (int32 i = 0; i < Current.ScoreBoard.Num(); i++)
	{
		const float CurrentScore = Current.ScoreBoard[i];
		const float PreviousScore = Previous.ScoreBoard.IsValidIndex(i)
			? Previous.ScoreBoard[i]
			: 0.f;

		if (!FMath::IsNearlyEqual(CurrentScore, PreviousScore))
		{
			// Update cache
			if (!CachedScores.IsValidIndex(i))
			{
				CachedScores.SetNum(i + 1);
			}
			CachedScores[i] = CurrentScore;

			OnScoreUpdated.Broadcast(i, CurrentScore);

			UE_LOG(LogTemp, Log,
				TEXT("ConnectItScoreInterpreter: Faction %d score "
					 "updated — %.0f (was %.0f)"),
				i, CurrentScore, PreviousScore);
		}
	}

	// Check game over -- only fire once
	if (Current.bGameOver && !bGameOverFired)
	{
		bGameOverFired = true;
		OnGameOverDetected.Broadcast(Current.WinningFactionSlot);

		UE_LOG(LogTemp, Log,
			TEXT("ConnectItScoreInterpreter: Game over detected — "
				 "faction %d wins"),
			Current.WinningFactionSlot);
	}
}

float UConnectIt_ScoreInterpreter::GetFactionScore(int32 FactionSlot) const
{
	return CachedScores.IsValidIndex(FactionSlot)
		? CachedScores[FactionSlot]
		: 0.f;
}
