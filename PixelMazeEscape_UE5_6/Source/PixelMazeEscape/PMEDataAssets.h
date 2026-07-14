#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PMETypes.h"
#include "PMEDataAssets.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class PIXELMAZEESCAPE_API UPMEDifficultyDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Maze", meta=(ClampMin="1", ClampMax="4"))
	int32 ObjectiveCount = 2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Maze", meta=(ClampMin="0", ClampMax="12"))
	int32 PickupCount = 3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemies", meta=(ClampMin="0", ClampMax="20"))
	int32 EnemyCount = 2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemies", meta=(ClampMin="0.25", ClampMax="4.0"))
	float EnemySpeedMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemies", meta=(ClampMin="0.25", ClampMax="4.0"))
	float EnemyPerceptionMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Level")
	EPMELevelModifier Modifier = EPMELevelModifier::None;
};

UCLASS(BlueprintType)
class PIXELMAZEESCAPE_API UPMEPickupDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup")
	EPMEPickupType PickupType = EPMEPickupType::Torch;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup", meta=(ClampMin="1", ClampMax="99"))
	int32 Charges = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup")
	FName DisplayTextKey = TEXT("Item.Torch");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup")
	TObjectPtr<UTexture2D> Icon;
};

UCLASS(BlueprintType)
class PIXELMAZEESCAPE_API UPMEUpgradeDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Upgrade")
	EPMEUpgradeType UpgradeType = EPMEUpgradeType::WiderVision;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Upgrade")
	FName NameTextKey = TEXT("Upgrade.0.Name");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Upgrade")
	FName DescriptionTextKey = TEXT("Upgrade.0.Description");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Upgrade", meta=(ClampMin="1", ClampMax="10"))
	int32 MaximumStacks = 5;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Upgrade")
	TObjectPtr<UTexture2D> Icon;
};
