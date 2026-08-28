// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridMechanicsBaseEnums.h"
#include "GridMechanicsBaseStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GridMechanics_GridLibrary.generated.h"

/**
 * Blueprint function library for grid math: direction <-> rotation conversions,
 * grid position arithmetic, neighbor traversal, tile utility helpers, and
 * N-in-a-row ("Connect Four"-style) connection analysis.
 */
UCLASS()
class UNREALGRIDMECHANICS_API UGridMechanics_GridLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// ============================================================
	// FGridPosition Blueprint Helpers
	// ============================================================
	UFUNCTION(BlueprintPure, Category = "Grid Mechanics | Library | GridPosition")
	static bool GridPositionIsEqual(const FGridPosition& A, const FGridPosition& B);
	
	// ============================================================
	// Direction <-> Vector / Rotation
	// ============================================================

	/** Static lookup of the 8 grid directions to their (Row, Column) unit vectors, in clockwise order starting at Up. */
	static const TMap<EGridDirection, FGridDirectionVector> GridDirectionVectors;

	/**
	 * Returns the (Row, Column) unit vector for a given grid direction.
	 * @param Direction Direction to look up.
	 * @return The direction vector, or a zero-initialized vector if Direction is not a valid key (e.g. EGridDirection::Max).
	 */
	UFUNCTION(BlueprintPure, Category = "Grid Mechanics | Library | Direction")
	static FGridDirectionVector GetGridDirectionVector(EGridDirection Direction);

	/**
	 * Converts a yaw angle in degrees into the nearest 8-way grid direction.
	 * Values are normalized into [0, 360) before being bucketed into 45-degree segments.
	 * @param InDegrees Angle in degrees (any range, including negative).
	 * @return The nearest EGridDirection, with Up corresponding to 0 degrees.
	 */
	UFUNCTION(BlueprintPure, Category = "Grid Mechanics | Library | Direction")
	static EGridDirection GetGridDirectionFromDegrees(float InDegrees);

	/**
	 * Converts a grid direction into its equivalent yaw rotation in degrees.
	 * @param InDirection Grid direction to convert.
	 * @return Yaw in degrees (0, 45, 90, ... 315).
	 */
	UFUNCTION(BlueprintPure, Category = "Grid Mechanics | Library | Direction")
	static float GetRotationFromGridDirection(EGridDirection InDirection);

	/**
	 * Returns an actor's current facing as a grid direction, based on actor yaw only (pitch/roll ignored).
	 * @param InActor Actor to query.
	 * @return The actor's grid direction, or EGridDirection::Max if InActor is null.
	 */
	UFUNCTION(BlueprintCallable, Category = "Grid Mechanics | Library | Direction")
	static EGridDirection GetActorGridDirection(const AActor* InActor);

	/**
	 * Rotates a base direction by a forward-facing offset, wrapping around the 8-way compass.
	 * Useful for converting a "local" direction (e.g. relative to a unit's forward facing)
	 * into a world/grid-absolute direction.
	 * @param Direction Direction to rotate.
	 * @param Forward Directional offset to rotate by.
	 * @return The resulting direction, wrapped into the valid 0-7 range.
	 */
	UFUNCTION(BlueprintPure, Category = "Grid Mechanics | Library | Direction")
	static EGridDirection GetRotatedGridDirection(EGridDirection Direction, EGridDirection Forward);

	/**
	 * Writes the (X, Y) grid offset corresponding to a single direction step.
	 * @param OutGridPosition Result offset for GridDirection. Left unmodified if GridDirection has no mapping.
	 * @param GridDirection Direction to convert.
	 */
	UFUNCTION(BlueprintCallable, Category = "Grid Mechanics | Library | Direction")
	static void GetNormalizedGridPositionByDirection(FGridPosition& OutGridPosition, EGridDirection GridDirection);

	// ============================================================
	// Grid Position Math
	// ============================================================

	/**
	 * Converts an actor's world location into a grid cell coordinate by dividing by a cell size.
	 * @param Actor Actor to sample. Returns a default (0,0) position if null.
	 * @param Size World-space size of one grid cell. Clamped to a minimum of 10 to avoid divide-by-near-zero.
	 * @return The actor's (X, Y) grid cell coordinate, truncated toward zero.
	 */
	UFUNCTION(BlueprintCallable, Category = "Grid Mechanics | Library | Position")
	static FGridPosition CalculateGridPositionFromSize(const AActor* Actor, int32 Size);

	/**
	 * Returns the average (centroid) of a set of grid positions.
	 * @param InGridPositions Positions to average. Must not be empty.
	 * @return The averaged position, or (0, 0) if InGridPositions is empty.
	 */
	UFUNCTION(BlueprintPure, Category = "Grid Mechanics | Library | Position")
	static FGridPosition GetAverageGridPosition(const TArray<FGridPosition>& InGridPositions);

	/**
	 * Normalizes the vector from P1 to P2 into one of the 8 grid directions (each axis clamped to -1/0/1),
	 * biased toward whichever axis has the larger magnitude when they differ.
	 * @param OutGridPosition Result normalized offset. Set to (0,0) if P1 == P2.
	 * @param P1 Start position.
	 * @param P2 End position.
	 */
	UFUNCTION(BlueprintPure, Category = "Grid Mechanics | Library | Position")
	static void GetClosestNormalizedGridPositionFromPositions(FGridPosition& OutGridPosition, const FGridPosition& P1, const FGridPosition& P2);

	/**
	 * Returns the single 8-way grid direction that most closely points from P1 toward P2.
	 * @param P1 Start position.
	 * @param P2 End position.
	 * @return The closest grid direction. Returns EGridDirection::Up and logs a warning if P1 == P2, since no direction is well-defined.
	 */
	UFUNCTION(BlueprintPure, Category = "Grid Mechanics | Library | Position")
	static EGridDirection GetClosestGridDirectionBetweenPositions(const FGridPosition& P1, const FGridPosition& P2);

	// ============================================================
	// Neighbors & Traversal
	// ============================================================

	/**
	 * Returns all 8 grid positions adjacent to InPosition (orthogonal + diagonal).
	 * @param InPosition Center position.
	 * @return Array of 8 neighboring positions, in the same clockwise order as GridDirectionVectors (Up, UpRight, Right, ...).
	 */
	UFUNCTION(BlueprintCallable, Category = "Grid Mechanics | Library | Traversal")
	static TArray<FGridPosition> GetPositionNeighbors(const FGridPosition InPosition);

	// ============================================================
	// Tile Utility
	// ============================================================

	/**
	 * Returns a random tile from the given array.
	 * @param InTiles Candidate tiles.
	 * @return A random element from InTiles, or nullptr if InTiles is empty.
	 */
	UFUNCTION(BlueprintCallable, Category = "Grid Mechanics | Library | Util")
	static AGridTileBase* GetRandomGridTile(const TArray<AGridTileBase*> InTiles);

	// ============================================================
	// Connection Analysis (N-in-a-row)
	// ============================================================

	/**
	 * Counts how many contiguous windows of ConnectLength cells, sliding along Direction and centered
	 * so that they all pass through Position, fall entirely within ValidPositions.
	 * Used as a building block for "N-in-a-row" style connection scoring (e.g. Connect Four).
	 * Complexity: O(ConnectLength^2) per call.
	 * @param Position Position that every candidate window must pass through.
	 * @param ValidPositions Set of positions considered occupied/valid for a connection.
	 * @param Direction Axis to slide the window along.
	 * @param ConnectLength Number of consecutive cells required for a connection.
	 * @return Number of valid ConnectLength windows through Position along Direction.
	 */
	UFUNCTION(BlueprintCallable, Category = "Grid Mechanics | Library | Connections")
	static int32 CountValidWindows(
		const FGridPosition& Position,
		const TSet<FGridPosition>& ValidPositions,
		const FGridDirectionVector& Direction,
		int32 ConnectLength);

	/**
	 * Sums CountValidWindows across all 8 directions, giving the total number of possible
	 * ConnectLength connections passing through StartPosition.
	 * Complexity: O(8 * ConnectLength^2) per call.
	 * @param StartPosition Position to evaluate.
	 * @param ValidPositions Set of positions considered occupied/valid for a connection.
	 * @param ConnectLength Number of consecutive cells required for a connection. Defaults to 4.
	 * @return Total valid windows through StartPosition, or 0 if StartPosition is not itself in ValidPositions.
	 */
	UFUNCTION(BlueprintCallable, Category = "Grid Mechanics | Library | Connections")
	static int32 CountPossibleGridConnections(
		const FGridPosition& StartPosition,
		const TSet<FGridPosition>& ValidPositions,
		int32 ConnectLength = 4);

	/**
	 * Breaks CountPossibleGridConnections down per direction, e.g. for AI move scoring where you need
	 * to know not just how many connections are possible but which directions they lie along.
	 * @param Position Position to evaluate.
	 * @param ValidPositions Set of positions considered occupied/valid for a connection.
	 * @param ConnectLength Number of consecutive cells required for a connection. Defaults to 4.
	 * @return Map of direction to valid window count. Empty if Position is not itself in ValidPositions.
	 */
	UFUNCTION(BlueprintCallable, Category = "Grid Mechanics | Library | Connections")
	static TMap<EGridDirection, int32> GetPotentialConnectionCountPerDirection(
		const FGridPosition& Position,
		const TSet<FGridPosition>& ValidPositions,
		int32 ConnectLength = 4);

	/**
	 * Builds a list of (ConnectionCount, Position) pairs for every given position, sorted descending
	 * by connection count. Useful for flood-fill style "best move" ranking across a whole board.
	 * Not exposed to Blueprint: intended for C++-side AI/heuristic code due to the sorted TArray<TPair<>> return type.
	 * Complexity: O(N * 8 * ConnectLength^2) for N input positions, plus an O(N log N) sort.
	 * @param GridPositions Positions to evaluate (also used as the full set of valid positions).
	 * @param ConnectionLength Number of consecutive cells required for a connection.
	 * @return Pairs sorted by descending connection count.
	 */
	static TArray<TPair<int32, FGridPosition>> CreateConnectionsFloodMap(
		const TArray<FGridPosition>& GridPositions,
		const int32 ConnectionLength);

};