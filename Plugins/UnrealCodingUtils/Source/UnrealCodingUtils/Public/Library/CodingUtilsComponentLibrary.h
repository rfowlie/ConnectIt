// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CodingUtilsComponentLibrary.generated.h"

/**
 * 
 */
UCLASS()
class UNREALCODINGUTILS_API UCodingUtilsComponentLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	// Checks whether the owning actor of a component has server authority
	// Use this instead of HasAuthority() which is not available on UActorComponent
	UFUNCTION(BlueprintPure, Category = "Chimera|Utils")
	static bool IsAuthoritative(const UActorComponent* Component);
};
