#include "PMEPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "PMEAttributeSet.h"
#include "PMEGameModeBase.h"
#include "PMEGameplayAbilities.h"
#include "PMEInventoryComponent.h"
#include "PMEPlayerState.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace PixelMazePlayerAudio
{
	static const TCHAR* StepSoundPath = TEXT("/Game/PixelMaze/Audio/SW_SFX_Player_Step.SW_SFX_Player_Step");
}

APMEPlayerCharacter::APMEPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetNetUpdateFrequency(60.0f);
	SetMinNetUpdateFrequency(20.0f);

	GetCapsuleComponent()->InitCapsuleSize(30.0f, 45.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->GroundFriction = 12.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2400.0f;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetMesh()->SetVisibility(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PixelBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PixelBody"));
	PixelBody->SetupAttachment(GetCapsuleComponent());
	PixelBody->SetRelativeLocation(FVector(0.0f, 0.0f, -2.0f));
	PixelBody->SetRelativeScale3D(FVector(0.48f, 0.48f, 0.72f));
	PixelBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PixelBody->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded()) PixelBody->SetStaticMesh(CubeMesh.Object);

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(GetCapsuleComponent());
	TopDownCamera->SetRelativeLocation(FVector(0.0f, 0.0f, CameraHeight));
	TopDownCamera->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	TopDownCamera->ProjectionMode = ECameraProjectionMode::Orthographic;
	TopDownCamera->OrthoWidth = OrthographicWidth;
	TopDownCamera->bUsePawnControlRotation = false;
}

void APMEPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	SetReplicateMovement(true);
	GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
	TopDownCamera->SetRelativeLocation(FVector(0, 0, CameraHeight));
	TopDownCamera->OrthoWidth = OrthographicWidth;
	ApplyPlayerVisual();
	ResolveAudioAssets();
	InitializeAbilitySystem();
	LastStepSampleLocation = GetActorLocation();
	LastServerMovementSample = GetActorLocation();
}

void APMEPlayerCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsLocallyControlled()) UpdateFootstepAudio();
	UpdateAttributesAndNoise(DeltaSeconds);
}

void APMEPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilitySystem();
	ApplyPlayerVisual();
	LastStepSampleLocation = GetActorLocation();
	LastServerMovementSample = GetActorLocation();
	AccumulatedStepDistance = 0.0f;
	AccumulatedNoiseDistance = 0.0f;
}

void APMEPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitializeAbilitySystem();
	ApplyPlayerVisual();
}

UAbilitySystemComponent* APMEPlayerCharacter::GetAbilitySystemComponent() const
{
	const APMEPlayerState* PS = GetPlayerState<APMEPlayerState>();
	return PS ? PS->GetAbilitySystemComponent() : nullptr;
}

void APMEPlayerCharacter::InitializeAbilitySystem()
{
	if (APMEPlayerState* PS = GetPlayerState<APMEPlayerState>())
	{
		if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent()) ASC->InitAbilityActorInfo(PS, this);
		ApplyDownedState(PS->IsDowned());
	}
}

void APMEPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &APMEPlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &APMEPlayerCharacter::MoveRight);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &APMEPlayerCharacter::SprintPressed);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &APMEPlayerCharacter::SprintReleased);
	PlayerInputComponent->BindAction(TEXT("UseItem1"), IE_Pressed, this, &APMEPlayerCharacter::UseItem1);
	PlayerInputComponent->BindAction(TEXT("UseItem2"), IE_Pressed, this, &APMEPlayerCharacter::UseItem2);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &APMEPlayerCharacter::InteractPressed);
}

void APMEPlayerCharacter::MoveForward(const float Value)
{
	if (!IsDowned() && !FMath::IsNearlyZero(Value)) AddMovementInput(FVector::ForwardVector, Value);
}

void APMEPlayerCharacter::MoveRight(const float Value)
{
	if (!IsDowned() && !FMath::IsNearlyZero(Value)) AddMovementInput(FVector::RightVector, Value);
}

void APMEPlayerCharacter::SprintPressed()
{
	if (IsDowned()) return;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent()) ASC->AbilityLocalInputPressed(
		static_cast<int32>(EPMEAbilityInputID::Sprint));
}

void APMEPlayerCharacter::SprintReleased()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent()) ASC->AbilityLocalInputReleased(
		static_cast<int32>(EPMEAbilityInputID::Sprint));
}

void APMEPlayerCharacter::UseItem1() { if (!IsDowned()) ActivateInventorySlot(0); }
void APMEPlayerCharacter::UseItem2() { if (!IsDowned()) ActivateInventorySlot(1); }

void APMEPlayerCharacter::InteractPressed()
{
	if (IsDowned())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	const APMEPlayerState* PS = GetPlayerState<APMEPlayerState>();
	TSubclassOf<UGameplayAbility> AbilityClass = PS ? PS->GetReviveAbilityClass() : nullptr;
	if (!AbilityClass)
	{
		AbilityClass = UPMEGA_Revive::StaticClass();
	}

	ASC->TryActivateAbilityByClass(AbilityClass);
}

void APMEPlayerCharacter::ActivateInventorySlot(const int32 SlotIndex)
{
	if (HasAuthority()) ServerUseInventorySlot_Implementation(SlotIndex);
	else ServerUseInventorySlot(SlotIndex);
}

void APMEPlayerCharacter::ServerUseInventorySlot_Implementation(const int32 SlotIndex)
{
	const APMEPlayerState* PS = GetPlayerState<APMEPlayerState>();
	if (!PS || !PS->GetInventoryComponent()) return;
	ActivateItemType(PS->GetInventoryComponent()->GetItemInSlot(SlotIndex));
}

void APMEPlayerCharacter::ActivateItemType(const EPMEPickupType ItemType)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const APMEPlayerState* PS = GetPlayerState<APMEPlayerState>();
	if (!ASC || !PS) return;
	const TSubclassOf<UGameplayAbility> AbilityClass = PS->GetAbilityClassForItem(ItemType);
	if (AbilityClass) ASC->TryActivateAbilityByClass(AbilityClass);
}

void APMEPlayerCharacter::UpdateAttributesAndNoise(const float DeltaSeconds)
{
	APMEPlayerState* PS = GetPlayerState<APMEPlayerState>();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		float EffectiveSpeed = ASC->GetNumericAttribute(UPMEAttributeSet::GetMoveSpeedAttribute());
		if (ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Movement.Sprinting"))))
		{
			EffectiveSpeed *= ASC->GetNumericAttribute(UPMEAttributeSet::GetSprintMultiplierAttribute());
		}
		GetCharacterMovement()->MaxWalkSpeed = EffectiveSpeed;
	}

	if (!HasAuthority() || !PS) return;
	const FVector Current = GetActorLocation();
	const float Distance = FVector::Dist2D(Current, LastServerMovementSample);
	LastServerMovementSample = Current;
	if (Distance > 500.0f)
	{
		AccumulatedNoiseDistance = 0.0f;
		return;
	}
	PS->AddDistanceTravelled(Distance);
	if (GetVelocity().SizeSquared2D() < FMath::Square(10.0f) || IsDowned()) return;

	AccumulatedNoiseDistance += Distance;
	if (AccumulatedNoiseDistance >= NoiseStepDistance)
	{
		AccumulatedNoiseDistance = FMath::Fmod(AccumulatedNoiseDistance, FMath::Max(10.0f, NoiseStepDistance));
		const float Multiplier = ASC ? ASC->GetNumericAttribute(UPMEAttributeSet::GetNoiseMultiplierAttribute()) : 1.0f;
		if (APMEGameModeBase* GM = GetWorld()->GetAuthGameMode<APMEGameModeBase>())
		{
			GM->EmitNoise(GetActorLocation(), WalkingNoiseRadiusInTiles * Multiplier, this,
			              TEXT("Event.Noise.Footstep"));
		}
	}
}

bool APMEPlayerCharacter::IsDowned() const
{
	const APMEPlayerState* PS = GetPlayerState<APMEPlayerState>();
	return PS && PS->IsDowned();
}

void APMEPlayerCharacter::ApplyDownedState(const bool bDowned)
{
	GetCharacterMovement()->DisableMovement();
	if (!bDowned) GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	PixelBody->SetRelativeRotation(bDowned ? FRotator(0, 0, 90) : FRotator::ZeroRotator);
}

void APMEPlayerCharacter::RefreshPlayerVisual() { ApplyPlayerVisual(); }

void APMEPlayerCharacter::ApplyPlayerVisual()
{
	const APMEPlayerState* PS = GetPlayerState<APMEPlayerState>();
	const bool bP2 = PS && PS->GetPlayerIndex() == 2;
	const TCHAR* Path = bP2
		                    ? TEXT("/Game/PixelMaze/Materials/M_Player2.M_Player2")
		                    : TEXT("/Game/PixelMaze/Materials/M_Player.M_Player");
	if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, Path)) PixelBody->
		SetMaterial(0, Material);
}

void APMEPlayerCharacter::ResolveAudioAssets()
{
	if (!PlayerStepSound) PlayerStepSound = LoadObject<USoundBase>(nullptr, PixelMazePlayerAudio::StepSoundPath);
}

void APMEPlayerCharacter::UpdateFootstepAudio()
{
	const FVector Current = GetActorLocation();
	const float Distance = FVector::Dist2D(Current, LastStepSampleLocation);
	LastStepSampleLocation = Current;
	if (Distance > FMath::Max(PlayerStepDistance * 3.0f, 220.0f))
	{
		AccumulatedStepDistance = 0.0f;
		return;
	}
	if (!PlayerStepSound || GetVelocity().SizeSquared2D() < FMath::Square(10.0f) || IsDowned()) return;
	AccumulatedStepDistance += Distance;
	if (AccumulatedStepDistance < FMath::Max(10.0f, PlayerStepDistance)) return;
	AccumulatedStepDistance = FMath::Fmod(AccumulatedStepDistance, FMath::Max(10.0f, PlayerStepDistance));
	UGameplayStatics::PlaySound2D(this, PlayerStepSound, PlayerStepVolume,
	                              FMath::FRandRange(FMath::Min(PlayerStepMinimumPitch, PlayerStepMaximumPitch),
	                                                FMath::Max(PlayerStepMinimumPitch, PlayerStepMaximumPitch)));
}
