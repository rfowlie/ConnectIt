// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Board/BoardManager.h"
#include "ConnectIt_BoardManager.generated.h"

class UConnectIt_ConfigComponent;


UCLASS(Blueprintable, BlueprintType)
class CONNECTIT_API AConnectIt_BoardManager : public ABoardManager
{
	GENERATED_BODY()

public:

	AConnectIt_BoardManager();

	UFUNCTION(BlueprintCallable, Category = "ConnectIt|Config")
	const UConnectIt_ConfigComponent* GetConnectItConfig() const;

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "ConnectIt")
	TObjectPtr<UConnectIt_ConfigComponent> ConnectItConfig = nullptr;
	
};
