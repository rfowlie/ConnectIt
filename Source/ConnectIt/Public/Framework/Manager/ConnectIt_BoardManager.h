// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/BoardManager.h"
#include "ConnectIt_BoardManager.generated.h"

class UConnectItBoardStateComponent;
class UConnectIt_ConfigComponent;


UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API AConnectIt_BoardManager : public ABoardManager
{
	GENERATED_BODY()

public:

	AConnectIt_BoardManager();

	UFUNCTION(BlueprintCallable, Category = "ConnectIt|Config")
	UConnectIt_ConfigComponent* GetConnectItConfig() const;

	UFUNCTION(BlueprintCallable, Category = "ConnectIt|Config")
	UConnectItBoardStateComponent* GetConnectItBoardState() const;

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "ConnectIt")
	TObjectPtr<UConnectIt_ConfigComponent> ConnectItConfigComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly,
	Category = "ConnectIt|Components")
	TObjectPtr<UConnectItBoardStateComponent> ConnectItBoardStateComponent = nullptr;
	
};
