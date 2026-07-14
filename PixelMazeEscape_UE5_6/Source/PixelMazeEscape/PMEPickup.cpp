#include "PMEPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "PMEInventoryComponent.h"
#include "PMEPlayerCharacter.h"
#include "PMEPlayerState.h"
#include "UObject/ConstructorHelpers.h"

APMEPickup::APMEPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->InitSphereRadius(34.0f);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PixelBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PixelBody"));
	PixelBody->SetupAttachment(Trigger);
	PixelBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PixelBody->SetCastShadow(false);
	PixelBody->SetRelativeScale3D(FVector(0.24f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) PixelBody->SetStaticMesh(Cube.Object);
}

void APMEPickup::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority()) Trigger->OnComponentBeginOverlap.AddDynamic(this, &APMEPickup::HandleOverlap);
	ApplyVisual();
}

void APMEPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APMEPickup, PickupType);
	DOREPLIFETIME(APMEPickup, OwnerPlayerIndex);
}

void APMEPickup::InitializePickup(const EPMEPickupType NewType, const int32 NewOwnerPlayerIndex)
{
	check(HasAuthority());
	PickupType = NewType;
	OwnerPlayerIndex = NewOwnerPlayerIndex;
	ApplyVisual();
	ForceNetUpdate();
}

void APMEPickup::OnRep_PickupType() { ApplyVisual(); }

void APMEPickup::ApplyVisual()
{
	const TCHAR* Path = PickupType == EPMEPickupType::LightBomb
		                    ? TEXT("/Game/PixelMaze/Materials/M_Exit.M_Exit")
		                    : TEXT("/Game/PixelMaze/Materials/M_Player.M_Player");
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, Path)) PixelBody->SetMaterial(0, M);
	const float Scale = PickupType == EPMEPickupType::MasterKey ? 0.34f : 0.24f;
	PixelBody->SetRelativeScale3D(FVector(Scale));
}

void APMEPickup::HandleOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool,
                               const FHitResult&)
{
	APMEPlayerCharacter* Character = Cast<APMEPlayerCharacter>(OtherActor);
	APMEPlayerState* PS = Character ? Character->GetPlayerState<APMEPlayerState>() : nullptr;
	if (!PS || (OwnerPlayerIndex > 0 && PS->GetPlayerIndex() != OwnerPlayerIndex)) return;
	if (UPMEInventoryComponent* Inventory = PS->GetInventoryComponent())
	{
		if (Inventory->AddItem(PickupType, 1))
		{
			PS->AddScore(50);
			Destroy();
		}
	}
}
