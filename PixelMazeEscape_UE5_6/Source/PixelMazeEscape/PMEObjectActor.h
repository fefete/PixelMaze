#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PMETypes.h"
#include "PMEObjectActor.generated.h"

class APMEObjectManager;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PIXELMAZEESCAPE_API APMEObjectActor : public AActor
{
	GENERATED_BODY()

public:
	APMEObjectActor();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void InitializeObjective(APMEObjectManager* InManager, int32 InObjectiveId, int32 InOwnerPlayerIndex,
	                         EPMEObjectType InType);
	UFUNCTION(BlueprintPure, Category="Pixel Maze|Objective")
	bool IsActivated() const { return bActivated; }

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Objective")
	int32 GetOwnerPlayerIndex() const { return OwnerPlayerIndex; }

	void ActivateObjective(class APMEPlayerState* ActivatingPlayer);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Activated();
	UFUNCTION()
	void HandleBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent,
	                      AActor* OtherActor,
	                      UPrimitiveComponent* OtherComponent,
	                      int32 OtherBodyIndex);
	bool CanActivateForPlayer(const class APMEPlayerState* PlayerState) const;
	void RefreshVisual();
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Trigger;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PixelBody;
	UPROPERTY()
	TObjectPtr<APMEObjectManager> Manager;
	UPROPERTY(Replicated)
	int32 ObjectiveId = 0;
	UPROPERTY(Replicated)
	int32 OwnerPlayerIndex = 0;
	UPROPERTY(Replicated)
	EPMEObjectType ObjectiveType = EPMEObjectType::Standard;
	UPROPERTY(ReplicatedUsing=OnRep_Activated)
	bool bActivated = false;
	TSet<TWeakObjectPtr<class APMEPlayerState>> OverlappingPlayers;
};
