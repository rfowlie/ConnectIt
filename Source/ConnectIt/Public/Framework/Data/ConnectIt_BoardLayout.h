// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Board/ConnectIt_BoardLayoutCell.h"
#include "Board/Operations/ConnectIt_BoardOperations.h"
#include "ConnectIt_BoardLayout.generated.h"

struct FConnectItBoardState;

// A set of tiles a board can change to during a match: a phased level's
// Level director asks for one, the board's tiles change to match it, and the
// visuals follow. Opt-in -- a level's placed tiles stay its starting layout.
// Built in the editor (a sandbox level and the layout tool) or by hand here:
// every cell's gameplay values are visible and editable in this asset.
UCLASS(BlueprintType)
class CONNECTIT_API UConnectIt_BoardLayout : public UDataAsset
{
    GENERATED_BODY()

public:

    // The layout's tiles, one per grid position
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout", meta = (TitleProperty = "Position"))
    TArray<FConnectItBoardLayoutCell> Cells;

    // Where the camera moves to when the board changes to this layout
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout|Camera", meta = (InlineEditConditionToggle))
    bool bHasCamera = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout|Camera", meta = (EditCondition = "bHasCamera"))
    FTransform CameraTransform;

    // The cell at Position, or null
    const FConnectItBoardLayoutCell* FindCell(const FGridPosition& Position) const;

    // What changes to turn Board's tiles into this layout
    FConnectItBoardLayoutDiff DiffFrom(const FConnectItBoardState& Board) const;

    // What changes to turn Other's tiles into this layout
    FConnectItBoardLayoutDiff DiffFrom(const UConnectIt_BoardLayout& Other) const;

    // The one board change that turns Board into this layout: Change Layout
    // with the diff, this layout's name and its camera. Made when the change
    // is applied, so it diffs against the board it will actually change.
    FConnectItBoardOperation_ChangeLayout MakeChangeFrom(const FConnectItBoardState& Board) const;

#if WITH_EDITOR
    // Duplicate positions are invalid; starting pieces for a faction no match
    // has are warned about
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
