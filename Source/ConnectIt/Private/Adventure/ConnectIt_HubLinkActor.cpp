// Fill out your copyright notice in the Description page of Project Settings.

#include "Adventure/ConnectIt_HubLinkActor.h"
#include "Components/SplineComponent.h"


AConnectIt_HubLinkActor::AConnectIt_HubLinkActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;
    Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
    RootComponent = Spline;
}

void AConnectIt_HubLinkActor::SetEnds(AActor* InFrom, AActor* InTo)
{
    FromNode = InFrom;
    ToNode = InTo;
    if (!InFrom || !InTo) return;

    Spline->ClearSplinePoints(false);
    Spline->AddSplinePoint(InFrom->GetActorLocation(), ESplineCoordinateSpace::World, false);
    Spline->AddSplinePoint(InTo->GetActorLocation(), ESplineCoordinateSpace::World, true);
    OnLinkBuilt();
}
