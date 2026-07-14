#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PMEFogOfWarActor.generated.h"

class APMEMazeGenerator;
class UInstancedStaticMeshComponent;
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

	/**
	 * ISM is intentionally used instead of HISM because these instances change
	 * while the player moves. HISM is better suited to mostly static clusters.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UInstancedStaticMeshComponent> FogInstances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog of War", meta=(ClampMin="0.5", ClampMax="30.0"))
	float RevealRadiusInTiles = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog of War", meta=(ClampMin="1.0"))
	float FogTileThickness = 8.0f;

	/**
	 * Width of the transition ring around the reveal radius. The fog remains
	 * tile based, but edge tiles grow and shrink progressively instead of
	 * appearing a complete tile at a time.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog of War|Smoothing", meta=(ClampMin="0.0", ClampMax="3.0"))
	float RevealEdgeFeatherInTiles = 0.85f;

	/** Speed used to interpolate the reveal centre towards the pawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog of War|Smoothing", meta=(ClampMin="0.0", ClampMax="60.0"))
	float FogFollowInterpolationSpeed = 24.0f;

	/** Speed used when a fog tile grows or shrinks at the reveal boundary. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog of War|Smoothing", meta=(ClampMin="0.0", ClampMax="60.0"))
	float FogTileTransitionSpeed = 14.0f;

	/**
	 * Minimum time between fog evaluations. Zero evaluates every frame and
	 * provides the smoothest result. Values around 0.016-0.033 are also smooth.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog of War|Performance",
		meta=(ClampMin="0.0", ClampMax="0.25"))
	float UpdateInterval = 0.0f;

	/** Ignore tiny coverage changes to avoid unnecessary render-state updates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog of War|Performance",
		meta=(ClampMin="0.0001", ClampMax="0.1"))
	float TransformUpdateTolerance = 0.005f;

	/**
	 * Coverage values at or below this threshold are removed from the visible
	 * fog layer immediately. This prevents almost-zero instances from remaining
	 * on screen as isolated black pixels while the fog opens.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog of War|Smoothing",
		meta=(ClampMin="0.0", ClampMax="0.5"))
	float FullyHiddenCoverageThreshold = 0.12f;

	/**
	 * Smallest XY scale used by a still-visible fog tile. The instance is
	 * hidden once it reaches FullyHiddenCoverageThreshold instead of shrinking
	 * into a one-pixel dot.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog of War|Smoothing",
		meta=(ClampMin="0.05", ClampMax="0.75"))
	float MinimumVisibleFogScale = 0.28f;

	/**
	 * Slight overlap between neighbouring fog tiles. Values above 1 remove
	 * hairline gaps caused by filtering and floating-point interpolation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog of War|Smoothing",
		meta=(ClampMin="1.0", ClampMax="1.15"))
	float FogTileOverlapScale = 1.025f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog of War")
	bool bFogEnabled = true;

private:
	void ResolveMazeGenerator();
	void RefreshFog(float DeltaSeconds, bool bForce = false);
	void BuildPersistentFogGrid();
	void UpdateFogVisibility(const FVector2D& RevealCenterInGrid, float DeltaSeconds, bool bSnapAll);

	FVector2D WorldToContinuousGrid(const FVector& WorldLocation) const;
	FTransform MakeFogTransform(int32 X, int32 Y, float Coverage) const;
	int32 ToFogInstanceIndex(int32 X, int32 Y) const;

	UPROPERTY()
	TObjectPtr<APawn> TrackedPawn;

	UPROPERTY()
	TObjectPtr<APMEMazeGenerator> MazeGenerator;

	/** Continuous value per instance: 0 = fully revealed, 1 = fully covered. */
	TArray<float> FogCoverageValues;

	FVector2D SmoothedRevealCenter = FVector2D::ZeroVector;
	int32 LastMazeSeed = 0;
	int32 LastGridWidth = 0;
	int32 LastGridHeight = 0;
	float LastAppliedRadius = -1.0f;
	float AccumulatedTime = 0.0f;
	bool bHasRevealCenter = false;
	bool bPersistentGridBuilt = false;
};
