// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Async/Async.h"
#include "Tasks/Task.h"
#include "Templates/SharedPointer.h"
#include <atomic>


// Searcher-agnostic plumbing shared by every search in GameIntelligence::Search
// (MinMax today; anything else -- e.g. Monte Carlo -- later).
namespace GameIntelligence::Search
{
    // Shared stop flag for one search. The caller keeps a reference and sets
    // it to abandon the search (e.g. its owner is being destroyed).
    using FCancelFlag = TSharedRef<std::atomic<bool>, ESPMode::ThreadSafe>;

    inline FCancelFlag MakeCancelFlag()
    {
        return MakeShared<std::atomic<bool>, ESPMode::ThreadSafe>(false);
    }

    // Runs Work on one background task and hands its result to OnGameThread
    // ON THE GAME THREAD. Work must own everything it reads (capture by value
    // or shared reference) -- the caller may change or destroy its own copies
    // straight away.
    //
    // OnGameThread always runs, and must guard its own captures -- capture a
    // TWeakObjectPtr, not `this`.
    template<typename TResultType>
    void LaunchAsync(
        TUniqueFunction<TResultType()> Work,
        TUniqueFunction<void(TResultType&&)> OnGameThread)
    {
        UE::Tasks::Launch(
            UE_SOURCE_LOCATION,
            [Work = MoveTemp(Work), OnGameThread = MoveTemp(OnGameThread)]() mutable
            {
                TResultType Result = Work();

                AsyncTask(ENamedThreads::GameThread,
                    [Result = MoveTemp(Result), OnGameThread = MoveTemp(OnGameThread)]() mutable
                    {
                        OnGameThread(MoveTemp(Result));
                    });
            },
            UE::Tasks::ETaskPriority::BackgroundNormal);
    }
}
