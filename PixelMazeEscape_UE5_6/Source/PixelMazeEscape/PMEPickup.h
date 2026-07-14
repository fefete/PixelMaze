#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PMETypes.h"
#include "PMEPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PIXELMAZEESCAPE_API APMEPickup : public AActor
{
	GENERATED_BODY()

public:
	APMEPickup();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void InitializePickup(EPMEPickupType NewType, int32 NewOwnerPlayerIndex = 0);
	UFUNCTION(BlueprintPure, Category="Pixel Maze|Pickup")
	EPMEPickupType GetPickupType() const { return PickupType; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_PickupType();
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                   UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
	                   const FHitResult& SweepResult);
	void ApplyVisual();
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Trigger;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PixelBody;
	UPROPERTY(ReplicatedUsing=OnRep_PickupType)
	EPMEPickupType PickupType = EPMEPickupType::None;
	UPROPERTY(Replicated)
	int32 OwnerPlayerIndex = 0;
};
