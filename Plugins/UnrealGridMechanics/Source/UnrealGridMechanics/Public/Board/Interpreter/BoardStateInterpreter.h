// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BoardStateInterpreter.generated.h"

class UBoardStateComponentBase;

// Abstract base for all board state interpreters
// Lives in Chimera Framework -- no project-specific type knowledge
// Concrete subclasses (ConnectIt specific) override OnBoardStateChanged
// and cast Component to their project-specific type to read state
UCLASS(Abstract, Blueprintable, ClassGroup=(Chimera), meta=(BlueprintSpawnableComponent))
class UNREALGRIDMECHANICS_API UBoardStateInterpreter : public UActorComponent
{
	GENERATED_BODY()

public:

	UBoardStateInterpreter();

	// Called by project-specific wiring (e.g. AConnectItBoardActor::BeginPlay)
	// Interpreter does not find its own board manager
	// This function is the explicit injection point
	UFUNCTION(BlueprintCallable, Category = "Interpreter")
	void BindToBoardStateComponent(UBoardStateComponentBase* InComponent);

	UFUNCTION(BlueprintPure, Category = "Interpreter")
	bool IsBound() const;

	UFUNCTION(BlueprintPure, Category = "Interpreter")
	UBoardStateComponentBase* GetBoundComponent() const
	{
		return BoundComponent;
	}

protected:

	// Subclasses override this to respond to board state changes
	// Cast Component to project-specific type to access typed snapshot
	// Both previous and current state available via component getter
	UFUNCTION(BlueprintNativeEvent, Category = "Interpreter")
	void OnBoardStateChanged(const UBoardStateComponentBase* Component);
	virtual void OnBoardStateChanged_Implementation(
		const UBoardStateComponentBase* Component);

	UPROPERTY(BlueprintReadOnly, Category = "Interpreter")
	TObjectPtr<UBoardStateComponentBase> BoundComponent = nullptr;

private:

	UFUNCTION()
	void HandleBoardStateChanged();
};