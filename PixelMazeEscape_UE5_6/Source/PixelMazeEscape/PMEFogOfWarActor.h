#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PMEFogOfWarActor.generated.h"

class APMEMazeGenerator;
class UHierarchicalInstancedStaticMeshComponent;
class USceneComponent;

UCLASS(Blueprintable)
class PIXELMAZEESCAPE_API APMEFogOfWarActor : public AActor
{
	GENERATED_BODY()

public:
	APMEFogOfWarActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Fog of War")
	void InitializeFog(APawn* InTrackedPawn, float InRevealRadiusInTiles);

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Fog of War")
	void SetTrackedPawn(APawn* InTrackedPawn);

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Fog of War")
	void SetRevealRadiusInTiles(float NewRadius);

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Fog of War")
	float GetRevealRadiusInTiles() const { return RevealRadiusInTiles; }

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Fog of War")
	void SetFogEnabled(bool bEnabled);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FogInstances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog of War", meta=(ClampMin="0.5", ClampMax="30.0"))
	float RevealRadiusInTiles = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog of War", meta=(ClampMin="1.0"))
	float FogTileThickness = 8.0f;

	/**
	 * How often the pawn cell is checked. The fog is only modified after the
	 * pawn enters another maze tile, so it is not rebuilt every frame.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog of War", meta=(ClampMin="0.0", ClampMax="1.0"))
	float UpdateInterval = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog of War")
	bool bFogEnabled = true;

private:
	void ResolveMazeGenerator();
	void RefreshFogIfRequired(bool bForce = false);
	void BuildPersistentFogGrid();
	void UpdateFogVisibility(const FIntPoint& PawnCell, bool bForceAll = false);
	FTransform MakeFogTransform(int32 X, int32 Y, bool bCovered) const;
	int32 ToFogInstanceIndex(int32 X, int32 Y) const;

	UPROPERTY()
	TObjectPtr<APawn> TrackedPawn;

	UPROPERTY()
	TObjectPtr<APMEMazeGenerator> MazeGenerator;

	/** One byte per fixed instance: 1 means that tile is covered by fog. */
	TArray<uint8> CoveredTileStates;

	FIntPoint LastPawnCell = FIntPoint(-1, -1);
	int32 LastMazeSeed = 0;
	int32 LastGridWidth = 0;
	int32 LastGridHeight = 0;
	float LastAppliedRadius = -1.0f;
	float AccumulatedTime = 0.0f;
	bool bPersistentGridBuilt = false;
};
