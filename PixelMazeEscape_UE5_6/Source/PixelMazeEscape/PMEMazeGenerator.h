#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PMEMazeGenerator.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class USceneComponent;

UCLASS(Blueprintable)
class PIXELMAZEESCAPE_API APMEMazeGenerator : public AActor
{
	GENERATED_BODY()

public:
	APMEMazeGenerator();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category="Pixel Maze")
	void GenerateMaze(int32 Level = 1, int32 OverrideSeed = 0);

	UFUNCTION(BlueprintCallable, Category="Pixel Maze")
	void ClearMaze();

	UFUNCTION(BlueprintPure, Category="Pixel Maze")
	FVector GetPlayerStartWorldLocation() const;

	UFUNCTION(BlueprintPure, Category="Pixel Maze")
	FIntPoint GetPlayerStartCell() const { return StartCell; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze")
	FIntPoint GetExitCell() const { return ExitCell; }

	/** Returns one of the two deterministic, fair Versus spawn locations. */
	UFUNCTION(BlueprintPure, Category="Pixel Maze|Versus")
	FVector GetVersusPlayerStartWorldLocation(int32 PlayerIndex) const;

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Versus")
	FIntPoint GetVersusPlayerStartCell(int32 PlayerIndex) const;

	UFUNCTION(BlueprintPure, Category="Pixel Maze")
	FVector GetExitWorldLocation() const;

	UFUNCTION(BlueprintPure, Category="Pixel Maze")
	int32 GetActiveSeed() const { return ActiveSeed; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze")
	int32 GetGeneratedLevel() const { return GeneratedLevel; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze")
	int32 GetGridWidth() const { return GridWidth; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze")
	int32 GetGridHeight() const { return GridHeight; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze")
	float GetTileSize() const { return TileSize; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze")
	float GetWallHeight() const { return WallHeight; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze")
	bool IsWalkable(int32 X, int32 Y) const;

	UFUNCTION(BlueprintPure, Category="Pixel Maze")
	FVector GridToWorld(int32 X, int32 Y) const;

	UFUNCTION(BlueprintPure, Category="Pixel Maze")
	FIntPoint WorldToGrid(const FVector& WorldLocation) const;

	/**
	 * Cardinal grid visibility used by enemies. The two locations must resolve
	 * to the same grid row or column and every cell between them must be
	 * walkable. Diagonal visibility is rejected even when a world-space trace
	 * would otherwise pass through an open corner.
	 */
	UFUNCTION(BlueprintPure, Category="Pixel Maze|Visibility")
	bool HasClearGridLineOfSight(
		const FVector& StartWorldLocation,
		const FVector& EndWorldLocation) const;

protected:
	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FloorInstances;

	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> WallInstances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Maze|Size", meta=(ClampMin="3", ClampMax="64"))
	int32 BaseCellsX = 9;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Maze|Size", meta=(ClampMin="3", ClampMax="64"))
	int32 BaseCellsY = 7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Maze|Size", meta=(ClampMin="3", ClampMax="96"))
	int32 MaximumCellsX = 25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Maze|Size", meta=(ClampMin="3", ClampMax="96"))
	int32 MaximumCellsY = 19;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Maze|Rendering", meta=(ClampMin="32.0"))
	float TileSize = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Maze|Rendering", meta=(ClampMin="10.0"))
	float WallHeight = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Maze|Rendering", meta=(ClampMin="1.0"))
	float FloorThickness = 10.0f;

	/** Minimum direct grid separation preferred between both Versus starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Maze|Versus", meta=(ClampMin="1", UIMin="1"))
	int32 VersusMinimumSpawnSeparationInTiles = 6;

	/** Prevents Versus players from spawning too close to the finish line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Maze|Versus", meta=(ClampMin="1", UIMin="1"))
	int32 VersusMinimumDistanceFromExitInTiles = 6;

private:
	UFUNCTION()
	void OnRep_MazeDefinition();

	void RebuildFromDefinition();
	void BuildLogicalMaze(int32 CellsX, int32 CellsY, FRandomStream& RandomStream);
	void BuildInstances();
	void FindFarthestExit();
	void FindVersusStartCells();
	void ApplyGeneratedMaterials();
	bool IsInBounds(int32 X, int32 Y) const;
	int32 ToIndex(int32 X, int32 Y) const;

	TArray<uint8> Grid;
	int32 GridWidth = 0;
	int32 GridHeight = 0;

	UPROPERTY(ReplicatedUsing=OnRep_MazeDefinition)
	int32 GeneratedLevel = 0;

	UPROPERTY(ReplicatedUsing=OnRep_MazeDefinition)
	int32 ActiveSeed = 0;

	FIntPoint StartCell = FIntPoint(1, 1);
	FIntPoint ExitCell = FIntPoint(1, 1);
	FIntPoint VersusStartCells[2] = {FIntPoint(1, 1), FIntPoint(1, 1)};
};
