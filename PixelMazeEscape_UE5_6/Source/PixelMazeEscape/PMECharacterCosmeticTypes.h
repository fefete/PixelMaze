#pragma once

#include "CoreMinimal.h"
#include "PMETypes.h"
#include "UObject/SoftObjectPtr.h"
#include "PMECharacterCosmeticTypes.generated.h"

class UMaterialInterface;
class UTexture2D;

UENUM(BlueprintType)
enum class EPMEProfileAchievement : uint8
{
	CompleteAnyRun UMETA(DisplayName="Complete Any Run"),
	UseLightBomb UMETA(DisplayName="Use A Light Bomb"),
	UseMapPulse UMETA(DisplayName="Use A Map Pulse"),
	CompleteMapWithoutTorch UMETA(DisplayName="Complete A Map Without Torch"),
	UseDecoy UMETA(DisplayName="Use A Decoy")
};

USTRUCT(BlueprintType)
struct FPMECharacterCosmeticDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pixel Maze|Customization")
	FName CharacterId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pixel Maze|Customization")
	FName DisplayNameTextKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pixel Maze|Customization")
	bool bUnlockedByDefault = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pixel Maze|Customization",
		meta=(EditCondition="!bUnlockedByDefault"))
	EPMEProfileAchievement RequiredAchievement = EPMEProfileAchievement::CompleteAnyRun;

	/** Texture used by UMG thumbnails and the large character preview. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pixel Maze|Customization")
	TSoftObjectPtr<UTexture2D> PreviewTexture;

	/** Material used by the player's PixelBody in the game world. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pixel Maze|Customization")
	TSoftObjectPtr<UMaterialInterface> Material;
};

namespace PixelMazeAchievement
{
	inline FName ToTextKey(const EPMEProfileAchievement Achievement)
	{
		switch (Achievement)
		{
		case EPMEProfileAchievement::CompleteAnyRun:
			return TEXT("Achievement.CompleteAnyRun");
		case EPMEProfileAchievement::UseLightBomb:
			return TEXT("Achievement.UseLightBomb");
		case EPMEProfileAchievement::UseMapPulse:
			return TEXT("Achievement.UseMapPulse");
		case EPMEProfileAchievement::CompleteMapWithoutTorch:
			return TEXT("Achievement.CompleteMapWithoutTorch");
		case EPMEProfileAchievement::UseDecoy:
			return TEXT("Achievement.UseDecoy");
		default:
			return TEXT("Achievement.CompleteAnyRun");
		}
	}
}
