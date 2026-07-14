#include "PMEInventoryComponent.h"

#include "Net/UnrealNetwork.h"

UPMEInventoryComponent::UPMEInventoryComponent()
{
	SetIsReplicatedByDefault(true);
	Slots.SetNum(2);
}

void UPMEInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPMEInventoryComponent, Slots);
}

bool UPMEInventoryComponent::AddItem(const EPMEPickupType ItemType, const int32 Charges)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || ItemType == EPMEPickupType::None || Charges <= 0) return false;

	const int32 ExistingSlot = FindSlotForItem(ItemType);
	if (Slots.IsValidIndex(ExistingSlot))
	{
		Slots[ExistingSlot].Charges += Charges;
		OnRep_Slots();
		return true;
	}

	for (FPMEInventorySlot& Slot : Slots)
	{
		if (Slot.ItemType == EPMEPickupType::None)
		{
			Slot.ItemType = ItemType;
			Slot.Charges = Charges;
			OnRep_Slots();
			return true;
		}
	}
	return false;
}

bool UPMEInventoryComponent::ConsumeItem(const EPMEPickupType ItemType, const int32 Charges)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	const int32 SlotIndex = FindSlotForItem(ItemType);
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].Charges < Charges) return false;
	Slots[SlotIndex].Charges -= Charges;
	if (Slots[SlotIndex].Charges <= 0) Slots[SlotIndex] = FPMEInventorySlot();
	OnRep_Slots();
	return true;
}

bool UPMEInventoryComponent::ConsumeSlot(const int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex)) return false;
	return ConsumeItem(Slots[SlotIndex].ItemType, 1);
}

EPMEPickupType UPMEInventoryComponent::GetItemInSlot(const int32 SlotIndex) const
{
	return Slots.IsValidIndex(SlotIndex) ? Slots[SlotIndex].ItemType : EPMEPickupType::None;
}

int32 UPMEInventoryComponent::GetChargesInSlot(const int32 SlotIndex) const
{
	return Slots.IsValidIndex(SlotIndex) ? Slots[SlotIndex].Charges : 0;
}

int32 UPMEInventoryComponent::FindSlotForItem(const EPMEPickupType ItemType) const
{
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (Slots[Index].ItemType == ItemType) return Index;
	}
	return INDEX_NONE;
}

bool UPMEInventoryComponent::HasFreeSlot() const
{
	return Slots.ContainsByPredicate(
		[](const FPMEInventorySlot& Slot) { return Slot.ItemType == EPMEPickupType::None; });
}

void UPMEInventoryComponent::ClearInventory()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	Slots.Init(FPMEInventorySlot(), 2);
	OnRep_Slots();
}

void UPMEInventoryComponent::OnRep_Slots()
{
	OnInventoryChanged.Broadcast();
}
