// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ConnectIt_HubLinkActor.generated.h"

class USplineComponent;


// The line between two revealed hub nodes (one node's Unlocks naming the
// other), spawned by the hub at runtime from the nodes' positions -- never
// placed. Its spline runs from the From node to the To node; a Blueprint
// subclass dresses it (a spline mesh, a ribbon material) in On Link Built.
// Set the class on the hub (the hub game mode's Adventure Hub component).
UCLASS(Blueprintable)
class CONNECTIT_API AConnectIt_HubLinkActor : public AActor
{
    GENERATED_BODY()

public:

    AConnectIt_HubLinkActor();

    // Lays the spline from From's location to To's, then On Link Built
    void SetEnds(AActor* InFrom, AActor* InTo);

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure Hub")
    USplineComponent* GetSpline() const { return Spline; }

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure Hub")
    AActor* GetFromNode() const { return FromNode; }

    UFUNCTION(BlueprintPure, Category = "ConnectIt|Adventure Hub")
    AActor* GetToNode() const { return ToNode; }

protected:

    // The spline is laid: build the link's look along it
    UFUNCTION(BlueprintImplementableEvent, Category = "ConnectIt|Adventure Hub")
    void OnLinkBuilt();

private:

    UPROPERTY(VisibleAnywhere, Category = "Link")
    TObjectPtr<USplineComponent> Spline;

    UPROPERTY()
    TObjectPtr<AActor> FromNode;

    UPROPERTY()
    TObjectPtr<AActor> ToNode;
};
