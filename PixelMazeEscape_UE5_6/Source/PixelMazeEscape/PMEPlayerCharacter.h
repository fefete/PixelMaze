#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "PMETypes.h"
#include "PMEPlayerCharacter.generated.h"

class UAbilitySystemComponent;
class UCameraComponent;
class USoundBase;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PIXELMAZEESCAPE_API APMEPlayerCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APMEPlayerCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	void RefreshPlayerVisual();
	void ApplyDownedState(bool bDowned);

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Player")
	bool IsDowned() const;

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Inventory")
	void ActivateInventorySlot(int32 SlotIndex);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pixel Art")
	TObjectPtr<UStaticMeshComponent> PixelBody;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement", meta=(ClampMin="100.0"))
	float MovementSpeed = 500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera", meta=(ClampMin="400.0"))
	float CameraHeight = 1800.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera", meta=(ClampMin="400.0"))
	float OrthographicWidth = 2200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Audio")
	TObjectPtr<USoundBase> PlayerStepSound;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Audio", meta=(ClampMin="10.0"))
	float PlayerStepDistance = 68.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Audio", meta=(ClampMin="0.0", ClampMax="1.0"))
	float PlayerStepVolume = 0.48f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Audio", meta=(ClampMin="0.5", ClampMax="1.5"))
	float PlayerStepMinimumPitch = 0.96f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Audio", meta=(ClampMin="0.5", ClampMax="1.5"))
	float PlayerStepMaximumPitch = 1.04f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Noise", meta=(ClampMin="0.1"))
	float WalkingNoiseRadiusInTiles = 4.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Pixel Maze|Noise", meta=(ClampMin="10.0"))
	float NoiseStepDistance = 95.0f;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void SprintPressed();
	void SprintReleased();
	void UseItem1();
	void UseItem2();
	void InteractPressed();
	void ApplyPlayerVisual();
	void ResolveAudioAssets();
	void UpdateFootstepAudio();
	void InitializeAbilitySystem();
	void UpdateAttributesAndNoise(float DeltaSeconds);
	void ActivateItemType(EPMEPickupType ItemType);

	UFUNCTION(Server, Reliable)
	void ServerUseInventorySlot(int32 SlotIndex);

	FVector LastStepSampleLocation = FVector::ZeroVector;
	float AccumulatedStepDistance = 0.0f;
	FVector LastServerMovementSample = FVector::ZeroVector;
	float AccumulatedNoiseDistance = 0.0f;
};
