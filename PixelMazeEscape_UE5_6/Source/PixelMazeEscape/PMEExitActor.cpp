#include "PMEExitActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "PMEGameModeBase.h"
#include "PMEPlayerCharacter.h"
#include "UObject/ConstructorHelpers.h"

APMEExitActor::APMEExitActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(42.0f, 42.0f, 55.0f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PixelPortal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PixelPortal"));
	PixelPortal->SetupAttachment(Trigger);
	PixelPortal->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.9f));
	PixelPortal->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PixelPortal->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		PixelPortal->SetStaticMesh(CubeMesh.Object);
	}
}

void APMEExitActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APMEExitActor, BaseLocation);
	DOREPLIFETIME(APMEExitActor, bExitEnabled);
}

void APMEExitActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		Trigger->OnComponentBeginOverlap.AddDynamic(this, &APMEExitActor::HandleBeginOverlap);
	}

	if (UMaterialInterface* ExitMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/PixelMaze/Materials/M_Exit.M_Exit")))
	{
		PixelPortal->SetMaterial(0, ExitMaterial);
	}

	ApplyExitState();
}

void APMEExitActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bExitEnabled)
	{
		return;
	}

	RunningTime += DeltaSeconds;
	PixelPortal->AddLocalRotation(FRotator(0.0f, 75.0f * DeltaSeconds, 0.0f));
	SetActorLocation(BaseLocation + FVector(0.0f, 0.0f, FMath::Sin(RunningTime * 3.0f) * 8.0f));
}

void APMEExitActor::ResetExit(const FVector& NewLocation)
{
	check(HasAuthority());
	BaseLocation = NewLocation;
	bExitEnabled = true;
	RunningTime = 0.0f;
	ApplyExitState();
	ForceNetUpdate();
}

void APMEExitActor::SetExitEnabled(const bool bEnabled)
{
	check(HasAuthority());
	bExitEnabled = bEnabled;
	ApplyExitState();
	ForceNetUpdate();
}

void APMEExitActor::OnRep_ExitState()
{
	RunningTime = 0.0f;
	ApplyExitState();
}

void APMEExitActor::ApplyExitState()
{
	SetActorLocation(BaseLocation);
	SetActorHiddenInGame(!bExitEnabled);

	if (HasAuthority())
	{
		Trigger->SetCollisionEnabled(bExitEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
	else
	{
		Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void APMEExitActor::HandleBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	const int32 OtherBodyIndex,
	const bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bExitEnabled)
	{
		return;
	}

	APMEPlayerCharacter* PlayerCharacter = Cast<APMEPlayerCharacter>(OtherActor);
	if (!PlayerCharacter)
	{
		return;
	}

	if (APMEGameModeBase* GameMode = GetWorld()->GetAuthGameMode<APMEGameModeBase>())
	{
		GameMode->HandlePlayerReachedExit(PlayerCharacter);
	}
}
