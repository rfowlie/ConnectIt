#include "InfluenceMap/GI_InfluenceMapSubsystem.h"
#include "InfluenceMap/GI_InfluenceMapWidget.h"
#include "InfluenceMap/GI_InfluenceMapVisualiserInterface.h"
#include "Blueprint/UserWidget.h"

// -- Lifecycle ---------------------------------------------------------------

void UGI_InfluenceMapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Registry.Reset();
    ActiveTag    = FGameplayTag::EmptyTag;
    ActiveWidget = nullptr;
}

void UGI_InfluenceMapSubsystem::Deinitialize()
{
    ClearSelection();
    DestroyDebugUI();
    Registry.Reset();
    Super::Deinitialize();
}

// -- Registration ------------------------------------------------------------

bool UGI_InfluenceMapSubsystem::RegisterVisualiser(
    TScriptInterface<IGI_InfluenceMapVisualiser> Visualiser)
{
    if (!Visualiser)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::RegisterVisualiser — Visualiser is null."));
        return false;
    }

    const FGameplayTag Tag = Visualiser->GetTag();
    if (!Tag.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::RegisterVisualiser — Visualiser has an invalid tag."));
        return false;
    }

    if (FindVisualiserIndexByTag(Tag) != INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::RegisterVisualiser — Tag '%s' is already registered. "
                 "Unregister the existing visualiser first."),
            *Tag.ToString());
        return false;
    }

    Registry.Add(Visualiser);
    RefreshWidgetButtons();
    return true;
}

bool UGI_InfluenceMapSubsystem::UnregisterVisualiser(FGameplayTag Tag)
{
    const int32 Index = FindVisualiserIndexByTag(Tag);
    if (Index == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::UnregisterVisualiser — Tag '%s' is not registered."),
            *Tag.ToString());
        return false;
    }

    if (ActiveTag == Tag)
    {
        ClearSelection();
    }

    Registry.RemoveAt(Index);
    RefreshWidgetButtons();
    return true;
}

// -- Selection ---------------------------------------------------------------

bool UGI_InfluenceMapSubsystem::SelectVisualiser(FGameplayTag Tag)
{
    const int32 Index = FindVisualiserIndexByTag(Tag);
    if (Index == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::SelectVisualiser — Tag '%s' is not registered."),
            *Tag.ToString());
        return false;
    }

    ClearSelection();

    ActiveTag = Tag;
    Registry[Index]->Activate();
    OnVisualiserSelected.Broadcast(Tag, Index);
    return true;
}

bool UGI_InfluenceMapSubsystem::SelectVisualiserByIndex(int32 Index)
{
    if (!Registry.IsValidIndex(Index))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::SelectVisualiserByIndex — Index %d out of range. "
                 "Registry contains %d entries."),
            Index, Registry.Num());
        return false;
    }

    return SelectVisualiser(Registry[Index]->GetTag());
}

void UGI_InfluenceMapSubsystem::ClearSelection()
{
    if (!ActiveTag.IsValid()) { return; }

    const int32 Index = FindVisualiserIndexByTag(ActiveTag);
    if (Index != INDEX_NONE && Registry[Index]->IsActive())
    {
        Registry[Index]->Deactivate();
    }

    ActiveTag = FGameplayTag::EmptyTag;
}

// -- Queries -----------------------------------------------------------------

TArray<FGameplayTag> UGI_InfluenceMapSubsystem::GetRegisteredTags() const
{
    TArray<FGameplayTag> Tags;
    Tags.Reserve(Registry.Num());
    for (const TScriptInterface<IGI_InfluenceMapVisualiser>& Visualiser : Registry)
    {
        Tags.Add(Visualiser->GetTag());
    }
    return Tags;
}

// -- UI ----------------------------------------------------------------------

void UGI_InfluenceMapSubsystem::CreateDebugUI(
    TSubclassOf<UGI_InfluenceMapWidget> WidgetClass,
    APlayerController*                  OwningPlayer)
{
    DestroyDebugUI();

    if (!WidgetClass)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::CreateDebugUI — WidgetClass is null."));
        return;
    }

    if (!OwningPlayer)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::CreateDebugUI — OwningPlayer is null."));
        return;
    }

    ActiveWidget = CreateWidget<UGI_InfluenceMapWidget>(OwningPlayer, WidgetClass);
    if (!ActiveWidget)
    {
        UE_LOG(LogTemp, Error,
            TEXT("GI_InfluenceMapSubsystem::CreateDebugUI — Failed to create widget."));
        return;
    }

    ActiveWidget->OnSelectionChanged.AddDynamic(
        this, &UGI_InfluenceMapSubsystem::OnWidgetSelectionChanged);

    ActiveWidget->SetupButtons(GetRegisteredTags());
    ActiveWidget->AddToViewport();
    ActiveWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void UGI_InfluenceMapSubsystem::DestroyDebugUI()
{
    if (!ActiveWidget) { return; }

    ActiveWidget->OnSelectionChanged.RemoveDynamic(
        this, &UGI_InfluenceMapSubsystem::OnWidgetSelectionChanged);

    ActiveWidget->RemoveFromParent();
    ActiveWidget = nullptr;
}

void UGI_InfluenceMapSubsystem::ShowDebugUI()
{
    if (!ActiveWidget)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::ShowDebugUI — No widget exists. "
                 "Call CreateDebugUI first."));
        return;
    }

    ActiveWidget->SetVisibility(ESlateVisibility::Visible);
}

void UGI_InfluenceMapSubsystem::HideDebugUI()
{
    if (!ActiveWidget) 
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GI_InfluenceMapSubsystem::HideDebugUI — No widget exists. "
                 "Call CreateDebugUI first."));
        return;
    }
    
    ActiveWidget->SetVisibility(ESlateVisibility::Collapsed);
}

// -- Internal ----------------------------------------------------------------

void UGI_InfluenceMapSubsystem::OnWidgetSelectionChanged(FGameplayTag Tag, int32 Index)
{
    SelectVisualiser(Tag);
}

int32 UGI_InfluenceMapSubsystem::FindVisualiserIndexByTag(FGameplayTag Tag) const
{
    return Registry.IndexOfByPredicate(
        [&Tag](const TScriptInterface<IGI_InfluenceMapVisualiser>& Visualiser)
        {
            return Visualiser->GetTag() == Tag;
        });
}

void UGI_InfluenceMapSubsystem::RefreshWidgetButtons()
{
    if (!ActiveWidget) { return; }
    ActiveWidget->SetupButtons(GetRegisteredTags());
}