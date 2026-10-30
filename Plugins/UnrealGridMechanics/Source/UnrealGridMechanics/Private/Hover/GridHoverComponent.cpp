// Fill out your copyright notice in the Description page of Project Settings.

#include "Hover/GridHoverComponent.h"
#include "GridMechanicsSettings.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"

#if WITH_EDITOR
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#endif


UGridHoverComponent::UGridHoverComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGridHoverComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* Owner = GetOwner())
    {
        Owner->OnBeginCursorOver.AddDynamic(this, &UGridHoverComponent::HandleOwnerBeginCursorOver);
        Owner->OnEndCursorOver.AddDynamic(this, &UGridHoverComponent::HandleOwnerEndCursorOver);
        Owner->OnClicked.AddDynamic(this, &UGridHoverComponent::HandleOwnerClicked);
    }
}

void UGridHoverComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (AActor* Owner = GetOwner())
    {
        Owner->OnBeginCursorOver.RemoveDynamic(this, &UGridHoverComponent::HandleOwnerBeginCursorOver);
        Owner->OnEndCursorOver.RemoveDynamic(this, &UGridHoverComponent::HandleOwnerEndCursorOver);
        Owner->OnClicked.RemoveDynamic(this, &UGridHoverComponent::HandleOwnerClicked);
    }
    bHovered = false;

    Super::EndPlay(EndPlayReason);
}

void UGridHoverComponent::SetHoverEnabled(bool bEnabled)
{
    if (bHoverEnabled == bEnabled) return;

    // Listeners mustn't be left holding a hover that can no longer end
    if (!bEnabled)
    {
        NotifyHoverEnd();
    }
    bHoverEnabled = bEnabled;
}

bool UGridHoverComponent::CanHover_Implementation() const
{
    return true;
}

void UGridHoverComponent::NotifyHoverBegin()
{
    if (bHovered || !bHoverEnabled || !CanHover()) return;

    bHovered = true;
    OnHoverBegin.Broadcast(this);
}

void UGridHoverComponent::NotifyHoverEnd()
{
    if (!bHovered) return;

    bHovered = false;
    OnHoverEnd.Broadcast(this);
}

void UGridHoverComponent::NotifyClicked()
{
    if (!bHoverEnabled || !CanHover()) return;

    OnClicked.Broadcast(this);
}

namespace
{
    // Whether one of Owner's query-enabled primitives blocks Channel
    bool BlocksChannel(const AActor* Owner, ECollisionChannel Channel)
    {
        if (!Owner) return false;

        TInlineComponentArray<UPrimitiveComponent*> Primitives(Owner);
        for (const UPrimitiveComponent* Primitive : Primitives)
        {
            if (Primitive
                && CollisionEnabledHasQuery(Primitive->GetCollisionEnabled())
                && Primitive->GetCollisionResponseToChannel(Channel) == ECR_Block)
            {
                return true;
            }
        }
        return false;
    }
}

bool UGridHoverComponent::DoesOwnerBlockCursorTrace() const
{
    return BlocksChannel(GetOwner(), UGridMechanicsSettings::GetCursorTraceChannel());
}

FString UGridHoverComponent::GetCursorTraceProblem() const
{
    const AActor* Owner = GetOwner();
    if (!Owner || DoesOwnerBlockCursorTrace()) return FString();

    const FString Channel = UGridMechanicsSettings::GetCursorTraceChannelName();
    if (UGridMechanicsSettings::GetCursorTraceChannel() != ECC_Visibility && BlocksChannel(Owner, ECC_Visibility))
    {
        return FString::Printf(TEXT("blocks Visibility but not the cursor trace channel (%s) -- it is never hovered or "
            "clicked. Block %s on its primitive (or use the GridTile collision profile)."), *Channel, *Channel);
    }
    return FString::Printf(TEXT("has a Grid Hover Component, but none of its primitives block the cursor trace channel "
        "(%s) -- it is never hovered or clicked."), *Channel);
}

void UGridHoverComponent::HandleOwnerBeginCursorOver(AActor* TouchedActor)
{
    NotifyHoverBegin();
}

void UGridHoverComponent::HandleOwnerEndCursorOver(AActor* TouchedActor)
{
    NotifyHoverEnd();
}

void UGridHoverComponent::HandleOwnerClicked(AActor* TouchedActor, FKey ButtonPressed)
{
    NotifyClicked();
}

#if WITH_EDITOR
void UGridHoverComponent::CheckForErrors()
{
    Super::CheckForErrors();

    // The other silent way hover fails: nothing for the cursor to hit
    AActor* Owner = GetOwner();
    const FString Problem = GetCursorTraceProblem();
    if (Owner && !Problem.IsEmpty())
    {
        FMessageLog(TEXT("MapCheck")).Warning()
            ->AddToken(FUObjectToken::Create(Owner))
            ->AddToken(FTextToken::Create(FText::FromString(Problem)));
    }
}
#endif
