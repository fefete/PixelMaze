#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "PMEAbilitySystemComponent.generated.h"

UCLASS(ClassGroup=(PixelMaze), meta=(BlueprintSpawnableComponent))
class PIXELMAZEESCAPE_API UPMEAbilitySystemComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()

public:
    UPMEAbilitySystemComponent();
};
