#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "PMEGameplayEffects.generated.h"

UCLASS()
class PIXELMAZEESCAPE_API UPMEGE_DamageOne : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UPMEGE_DamageOne();
};

UCLASS()
class PIXELMAZEESCAPE_API UPMEGE_Sprint : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UPMEGE_Sprint();
};

UCLASS()
class PIXELMAZEESCAPE_API UPMEGE_Torch : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UPMEGE_Torch();
};
