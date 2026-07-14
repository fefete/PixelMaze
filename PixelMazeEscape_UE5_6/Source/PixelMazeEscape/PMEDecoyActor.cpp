#include "PMEDecoyActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "PMEGameModeBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

APMEDecoyActor::APMEDecoyActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	PixelBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PixelBody"));
	SetRootComponent(PixelBody);
	PixelBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PixelBody->SetRelativeScale3D(FVector(0.18f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) PixelBody->SetStaticMesh(Cube.Object);
}

void APMEDecoyActor::InitializeDecoy(const float InLifetime, const float InPulseInterval,
                                     const float InNoiseRadiusTiles)
{
	DecoyLifetime = FMath::Max(0.2f, InLifetime);
	PulseInterval = FMath::Max(0.1f, InPulseInterval);
	NoiseRadiusTiles = FMath::Max(0.5f, InNoiseRadiusTiles);

	if (HasAuthority() && HasActorBegunPlay())
	{
		StartDecoyTimers();
	}
}

void APMEDecoyActor::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority()) StartDecoyTimers();
}

void APMEDecoyActor::StartDecoyTimers()
{
	GetWorldTimerManager().ClearTimer(PulseTimer);
	GetWorldTimerManager().ClearTimer(LifeTimer);
	EmitPulse();
	GetWorldTimerManager().SetTimer(PulseTimer, this, &APMEDecoyActor::EmitPulse, PulseInterval, true);
	GetWorldTimerManager().SetTimer(LifeTimer, this, &APMEDecoyActor::Expire, DecoyLifetime, false);
}

void APMEDecoyActor::EmitPulse()
{
	if (APMEGameModeBase* GM = GetWorld()->GetAuthGameMode<APMEGameModeBase>())
	{
		GM->EmitNoise(GetActorLocation(), NoiseRadiusTiles, this, TEXT("Event.Noise.Decoy"));
	}
}

void APMEDecoyActor::Expire()
{
	Destroy();
}
