// Fill out your copyright notice in the Description page of Project Settings.

#include "GridMechanicsSettings.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/Actor.h"


ECollisionChannel UGridMechanicsSettings::GetCursorTraceChannel()
{
    return GetDefault<UGridMechanicsSettings>()->CursorTraceChannel.GetValue();
}

FString UGridMechanicsSettings::GetCursorTraceChannelName()
{
    const ECollisionChannel Channel = GetCursorTraceChannel();
    const FName Name = UCollisionProfile::Get()->ReturnChannelNameFromContainerIndex(Channel);
    return Name.IsNone() ? UEnum::GetValueAsString(Channel) : Name.ToString();
}

bool UGridMechanicsSettings::SetRootCursorResponse(AActor* Actor, ECollisionResponse Response)
{
    if (!Actor) return false;

    if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
    {
        Root->SetCollisionResponseToChannel(GetCursorTraceChannel(), Response);
        return true;
    }

    // Once per class: a Blueprint whose root is a scene component sets its
    // primitives' collision itself
    static TSet<FName> Logged;
    const FName ClassName = Actor->GetClass()->GetFName();
    if (!Logged.Contains(ClassName))
    {
        Logged.Add(ClassName);
        UE_LOG(LogTemp, Warning,
            TEXT("GridMechanicsSettings: %s's root isn't a primitive -- its cursor channel (%s) response isn't set for it; "
                 "set it on its primitives (or use the GridTile / GridPiece collision profile)"),
            *ClassName.ToString(), *GetCursorTraceChannelName());
    }
    return false;
}
