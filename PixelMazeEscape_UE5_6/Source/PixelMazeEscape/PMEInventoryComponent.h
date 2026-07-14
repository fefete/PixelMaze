#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PMETypes.h"
#include "PMEInventoryComponent.generated.h"

USTRUCT(BlueprintType)
struct FPMEInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EPMEPickupType ItemType = EPMEPickupType::None;

	UPROPERTY(BlueprintReadOnly)
	int32 Charges = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPMEInventoryChangedSignature);

UCLASS(ClassGroup=(PixelMaze), meta=(BlueprintSpawnableComponent))
class PIXELMAZEESCAPE_API UPMEInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPMEInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category="Pixel Maze|Inventory")
	FPMEInventoryChangedSignature OnInventoryChanged;

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Inventory")
	bool AddItem(EPMEPickupType ItemType, int32 Charges = 1);

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Inventory")
	bool ConsumeItem(EPMEPickupType ItemType, int32 Charges = 1);

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Inventory")
	bool ConsumeSlot(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Inventory")
	EPMEPickupType GetItemInSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Inventory")
	int32 GetChargesInSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Inventory")
	int32 FindSlotForItem(EPMEPickupType ItemType) const;

	UFUNCTION(BlueprintPure, Category="Pixel Maze|Inventory")
	bool HasFreeSlot() const;

	UFUNCTION(BlueprintCallable, Category="Pixel Maze|Inventory")
	void ClearInventory();

private:
	UFUNCTION()
	void OnRep_Slots();

	UPROPERTY(ReplicatedUsing=OnRep_Slots)
	TArray<FPMEInventorySlot> Slots;
};
