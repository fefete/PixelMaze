#include "PMEObjectActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "PMEObjectManager.h"
#include "PMEPlayerCharacter.h"
#include "PMEPlayerState.h"
#include "UObject/ConstructorHelpers.h"

APMEObjectActor::APMEObjectActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->InitSphereRadius(45.0f);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PixelBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PixelBody"));
	PixelBody->SetupAttachment(Trigger);
	PixelBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PixelBody->SetCastShadow(false);
	PixelBody->SetRelativeScale3D(FVector(0.34f, 0.34f, 0.62f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) PixelBody->SetStaticMesh(Cube.Object);
}

void APMEObjectActor::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		Trigger->OnComponentBeginOverlap.AddDynamic(this, &APMEObjectActor::HandleBeginOverlap);
		Trigger->OnComponentEndOverlap.AddDynamic(this, &APMEObjectActor::HandleEndOverlap);
	}
	RefreshVisual();
}

void APMEObjectActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APMEObjectActor, ObjectiveId);
	DOREPLIFETIME(APMEObjectActor, OwnerPlayerIndex);
	DOREPLIFETIME(APMEObjectActor, ObjectiveType);
	DOREPLIFETIME(APMEObjectActor, bActivated);
}

void APMEObjectActor::InitializeObjective(APMEObjectManager* InManager, int32 InObjectiveId, int32 InOwnerPlayerIndex,
                                          EPMEObjectType InType)
{
	check(HasAuthority());
	Manager = InManager;
	ObjectiveId = InObjectiveId;
	OwnerPlayerIndex = InOwnerPlayerIndex;
	ObjectiveType = InType;
	RefreshVisual();
	ForceNetUpdate();
}

bool APMEObjectActor::CanActivateForPlayer(const APMEPlayerState* PS) const
{
	return PS && !bActivated && (OwnerPlayerIndex == 0 || PS->GetPlayerIndex() == OwnerPlayerIndex) && !PS->IsDowned();
}

void APMEObjectActor::HandleBeginOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool,
                                         const FHitResult&)
{
	APMEPlayerCharacter* C = Cast<APMEPlayerCharacter>(Other);
	APMEPlayerState* PS = C ? C->GetPlayerState<APMEPlayerState>() : nullptr;
	if (!CanActivateForPlayer(PS)) return;
	OverlappingPlayers.Add(TWeakObjectPtr<APMEPlayerState>(PS));
	for (auto It = OverlappingPlayers.CreateIterator(); It; ++It)
	{
		if (!It->IsValid() || It->Get()->IsDowned()) It.RemoveCurrent();
	}
	if (ObjectiveType == EPMEObjectType::Standard) ActivateObjective(PS);
	else if (ObjectiveType == EPMEObjectType::Synchronized && OverlappingPlayers.Num() >= 2) ActivateObjective(PS);
}

void APMEObjectActor::HandleEndOverlap(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32)
{
	if (APMEPlayerCharacter* C = Cast<APMEPlayerCharacter>(Other))
		OverlappingPlayers.Remove(
			TWeakObjectPtr<APMEPlayerState>(C->GetPlayerState<APMEPlayerState>()));
}

void APMEObjectActor::ActivateObjective(APMEPlayerState* PS)
{
	if (!HasAuthority() || !CanActivateForPlayer(PS)) return;
	bActivated = true;
	Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RefreshVisual();
	ForceNetUpdate();
	if (Manager) Manager->HandleObjectiveActivated(this, PS);
}

void APMEObjectActor::OnRep_Activated() { RefreshVisual(); }

void APMEObjectActor::RefreshVisual()
{
	SetActorHiddenInGame(bActivated);
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/PixelMaze/Materials/M_Exit.M_Exit")))
		PixelBody->SetMaterial(0, M);
}
