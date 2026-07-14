#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PMEExitActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class PIXELMAZEESCAPE_API APMEExitActor : public AActor
{
	GENERATED_BODY()

public:
	APMEExitActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ResetExit(const FVector& NewLocation);
	void SetExitEnabled(bool bEnabled);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> PixelPortal;

private:
	UFUNCTION()
	void OnRep_ExitState();

	UFUNCTION()
	void HandleBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void ApplyExitState();

	UPROPERTY(ReplicatedUsing=OnRep_ExitState)
	FVector BaseLocation = FVector::ZeroVector;

	UPROPERTY(ReplicatedUsing=OnRep_ExitState)
	bool bExitEnabled = false;

	float RunningTime = 0.0f;
};
