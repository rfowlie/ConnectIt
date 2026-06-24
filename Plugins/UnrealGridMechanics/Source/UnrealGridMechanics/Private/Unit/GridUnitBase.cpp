// Fill out your copyright notice in the Description page of Project Settings.


#include "Unit/GridUnitBase.h"

#include "Subsystem/GridTrackerSubsystem.h"


// Sets default values
AGridUnitBase::AGridUnitBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AGridUnitBase::BeginPlay()
{
	Super::BeginPlay();

	// world subsystem should always know about this
	if (UGridTrackerSubsystem* Subsystem = GetWorld()->GetSubsystem<UGridTrackerSubsystem>(); IsValid(Subsystem))
	{
		Subsystem->RegisterGridUnit(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GridTrackerSubsystem Is Not Available..."))
	}
	
}

// Called every frame
void AGridUnitBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
