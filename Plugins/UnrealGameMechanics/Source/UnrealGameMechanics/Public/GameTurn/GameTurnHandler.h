// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "GameTurnHandler.generated.h"

class UGameEventTaskManager;
class UGameEventTask_Async;
class UGameEventTaskHandler;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameTurnHandlerDelegate, FGameplayTag, FactionTag);

/**
 * 
 */
UCLASS()
class UNREALGAMEMECHANICS_API UGameTurnHandler : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Game Mechanics | Turn")
	static UGameTurnHandler* Create();

	// useful for blueprint setup
	UFUNCTION(BlueprintCallable, Category = "Game Mechanics | Turn")
	void Initialize();

	UFUNCTION(BlueprintCallable, Category = "Game Mechanics | Turn")
	FGameplayTag GetActiveFactionTag() const { return FactionTagActive; }
	
	UFUNCTION(BlueprintCallable, Category = "Game Mechanics | Turn")
	void RegisterAsyncTaskTurnStart(UGameEventTask_Async* AsyncTask, int32 Phase) const;
	
	UFUNCTION(BlueprintCallable, Category = "Game Mechanics | Turn")
	void RegisterAsyncTaskTurnEnd(UGameEventTask_Async* AsyncTask, int32 Phase) const;

	// delegates
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Mechanics | Turn")
	FGameTurnHandlerDelegate TurnStartPreAsync;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Mechanics | Turn")
	FGameTurnHandlerDelegate TurnStartPostAsync;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Mechanics | Turn")
	FGameTurnHandlerDelegate TurnEndPreAsync;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Mechanics | Turn")
	FGameTurnHandlerDelegate TurnEndPostAsync;
	

protected:
	UFUNCTION()
	void OnTurnStartPreAsync();

	UFUNCTION()
	void OnTurnStartPostAsync();

	UFUNCTION()
	void OnTurnEndPreAsync();

	UFUNCTION()
	void OnTurnEndPostAsync();
	
	
public:
	

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Mechanics | Turn")
	UGameEventTaskManager* GameEventTurnStart;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Mechanics | Turn")
	UGameEventTaskManager* GameEventTurnEnd;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Mechanics | Turn")
	TArray<FGameplayTag> FactionTags;

	UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "Game Mechanics | Turn")
	FGameplayTag FactionTagActive;
	
};
