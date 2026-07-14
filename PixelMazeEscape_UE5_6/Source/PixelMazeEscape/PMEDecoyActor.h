#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PMEDecoyActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class PIXELMAZEESCAPE_API APMEDecoyActor : public AActor
{
	GENERATED_BODY()

public:
	APMEDecoyActor();
	void InitializeDecoy(float InLifetime, float InPulseInterval, float InNoiseRadiusTiles);

protected:
	virtual void BeginPlay() override;

private:
	void StartDecoyTimers();
	void EmitPulse();
	void Expire();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PixelBody;

	float DecoyLifetime = 6.0f;
	float PulseInterval = 0.8f;
	float NoiseRadiusTiles = 10.0f;
	FTimerHandle PulseTimer;
	FTimerHandle LifeTimer;
};
