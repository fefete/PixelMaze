#include "PMEPlayerState.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "PMEAbilitySystemComponent.h"
#include "PMECharacterCustomizationSubsystem.h"
#include "PMEAttributeSet.h"
#include "PMEGameplayAbilities.h"
#include "PMEInventoryComponent.h"
#include "PMELocalizationLibrary.h"
#include "PMEPlayerCharacter.h"

APMEPlayerState::APMEPlayerState()
{
    bReplicates = true;
    NetUpdateFrequency = 30.0f;

    AbilitySystemComponent = CreateDefaultSubobject<UPMEAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AttributeSet = CreateDefaultSubobject<UPMEAttributeSet>(TEXT("AttributeSet"));
    InventoryComponent = CreateDefaultSubobject<UPMEInventoryComponent>(TEXT("InventoryComponent"));
    SprintAbilityClass = UPMEGA_Sprint::StaticClass();
    ReviveAbilityClass = UPMEGA_Revive::StaticClass();
    TorchAbilityClass = UPMEGA_Torch::StaticClass();
    MapPulseAbilityClass = UPMEGA_MapPulse::StaticClass();
    DecoyAbilityClass = UPMEGA_Decoy::StaticClass();
    LightBombAbilityClass = UPMEGA_LightBomb::StaticClass();
    MasterKeyAbilityClass = UPMEGA_MasterKey::StaticClass();
    UpgradeStacks.Init(0, 9);
}

void APMEPlayerState::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority()) GrantDefaultAbilities();
}

UAbilitySystemComponent* APMEPlayerState::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void APMEPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(APMEPlayerState, PlayerIndex);
    DOREPLIFETIME(APMEPlayerState, SelectedCharacterId);
    DOREPLIFETIME(APMEPlayerState, Wins);
    DOREPLIFETIME(APMEPlayerState, bReachedExit);
    DOREPLIFETIME(APMEPlayerState, FinishTime);
    DOREPLIFETIME(APMEPlayerState, bDowned);
    DOREPLIFETIME(APMEPlayerState, RunScore);
    DOREPLIFETIME(APMEPlayerState, DetectionCount);
    DOREPLIFETIME(APMEPlayerState, HitCount);
    DOREPLIFETIME(APMEPlayerState, ObjectivesActivated);
    DOREPLIFETIME(APMEPlayerState, DistanceTravelled);
    DOREPLIFETIME(APMEPlayerState, UpgradeStacks);
}

void APMEPlayerState::GrantDefaultAbilities()
{
    if (!HasAuthority() || bAbilitiesGranted || !AbilitySystemComponent) return;
    bAbilitiesGranted = true;

    if (SprintAbilityClass) AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(SprintAbilityClass, 1, static_cast<int32>(EPMEAbilityInputID::Sprint), this));
    if (ReviveAbilityClass) AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(ReviveAbilityClass, 1, static_cast<int32>(EPMEAbilityInputID::Revive), this));
    if (TorchAbilityClass) AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(TorchAbilityClass, 1, INDEX_NONE, this));
    if (MapPulseAbilityClass) AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(MapPulseAbilityClass, 1, INDEX_NONE, this));
    if (DecoyAbilityClass) AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DecoyAbilityClass, 1, INDEX_NONE, this));
    if (LightBombAbilityClass) AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(LightBombAbilityClass, 1, INDEX_NONE, this));
    if (MasterKeyAbilityClass) AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(MasterKeyAbilityClass, 1, INDEX_NONE, this));
}

void APMEPlayerState::AssignPlayerIndex(const int32 NewIndex)
{
    check(HasAuthority());
    PlayerIndex = FMath::Clamp(NewIndex, 1, 2);
    FFormatNamedArguments Args;
    Args.Add(TEXT("Player"), PlayerIndex);
    SetPlayerName(FText::Format(UPMELocalizationLibrary::GetText(TEXT("Player.Name")), Args).ToString());
    OnRep_PlayerIndex();
    ForceNetUpdate();
}

void APMEPlayerState::SetSelectedCharacterId(
    const FName NewCharacterId)
{
    check(HasAuthority());

    const FName ValidatedCharacterId =
        UPMECharacterCustomizationSubsystem::IsKnownCharacterId(
            NewCharacterId)
            ? NewCharacterId
            : FName(TEXT("Character.01"));

    if (SelectedCharacterId == ValidatedCharacterId)
    {
        return;
    }

    SelectedCharacterId = ValidatedCharacterId;
    OnRep_SelectedCharacterId();
    ForceNetUpdate();
}

void APMEPlayerState::ResetForRound()
{
    check(HasAuthority());
    bReachedExit = false;
    FinishTime = 0.0f;
    bDowned = false;
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->CancelAllAbilities();
        AbilitySystemComponent->RemoveActiveEffects(FGameplayEffectQuery());
    }
    ApplyPersistentUpgradeAttributes();
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->SetNumericAttributeBase(UPMEAttributeSet::GetHealthAttribute(),
            AbilitySystemComponent->GetNumericAttribute(UPMEAttributeSet::GetMaxHealthAttribute()));
        AbilitySystemComponent->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Player.Downed")));
        AbilitySystemComponent->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Player.AtExit")));
    }
    ForceNetUpdate();
}

void APMEPlayerState::ApplyPersistentUpgradeAttributes()
{
    if (!HasAuthority() || !AbilitySystemComponent) return;
    const float MaxHealth = 3.0f + GetUpgradeStack(EPMEUpgradeType::StrongHeart);
    const float MoveSpeed = 500.0f + 35.0f * GetUpgradeStack(EPMEUpgradeType::FasterMovement);
    const float FogRadius = 5.0f + GetUpgradeStack(EPMEUpgradeType::WiderVision);
    const float Noise = FMath::Max(0.35f, 1.0f - 0.12f * GetUpgradeStack(EPMEUpgradeType::QuietSteps));
    const float Detection = FMath::Max(0.45f, 1.0f - 0.10f * GetUpgradeStack(EPMEUpgradeType::SlowerDetection));
    const float Sprint = 1.65f + 0.12f * GetUpgradeStack(EPMEUpgradeType::BetterSprint);

    AbilitySystemComponent->SetNumericAttributeBase(UPMEAttributeSet::GetMaxHealthAttribute(), MaxHealth);
    AbilitySystemComponent->SetNumericAttributeBase(UPMEAttributeSet::GetMoveSpeedAttribute(), MoveSpeed);
    AbilitySystemComponent->SetNumericAttributeBase(UPMEAttributeSet::GetFogRevealRadiusAttribute(), FogRadius);
    AbilitySystemComponent->SetNumericAttributeBase(UPMEAttributeSet::GetNoiseMultiplierAttribute(), Noise);
    AbilitySystemComponent->SetNumericAttributeBase(UPMEAttributeSet::GetDetectionMultiplierAttribute(), Detection);
    AbilitySystemComponent->SetNumericAttributeBase(UPMEAttributeSet::GetSprintMultiplierAttribute(), Sprint);
}

void APMEPlayerState::MarkReachedExit(const float NewFinishTime)
{
    check(HasAuthority());
    bReachedExit = true;
    FinishTime = FMath::Max(0.0f, NewFinishTime);
    if (AbilitySystemComponent) AbilitySystemComponent->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Player.AtExit")));
    ForceNetUpdate();
}
void APMEPlayerState::AddWin() { check(HasAuthority()); ++Wins; ForceNetUpdate(); }
void APMEPlayerState::SetDowned(const bool bNewDowned)
{
    check(HasAuthority());
    bDowned = bNewDowned;
    if (AbilitySystemComponent)
    {
        const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TEXT("State.Player.Downed"));
        if (bDowned) AbilitySystemComponent->AddLooseGameplayTag(Tag); else AbilitySystemComponent->RemoveLooseGameplayTag(Tag);
    }
    OnRep_Downed();
    ForceNetUpdate();
}
void APMEPlayerState::AddScore(const int32 Amount) { check(HasAuthority()); RunScore = FMath::Max(0, RunScore + Amount); ForceNetUpdate(); }
void APMEPlayerState::RecordDetection() { check(HasAuthority()); ++DetectionCount; ForceNetUpdate(); }
void APMEPlayerState::RecordHit() { check(HasAuthority()); ++HitCount; ForceNetUpdate(); }
void APMEPlayerState::RecordObjective() { check(HasAuthority()); ++ObjectivesActivated; AddScore(250); }
void APMEPlayerState::AddDistanceTravelled(const float Distance) { if (HasAuthority()) DistanceTravelled += FMath::Max(0.0f, Distance); }

int32 APMEPlayerState::GetUpgradeStack(const EPMEUpgradeType UpgradeType) const
{
    const int32 Index = static_cast<int32>(UpgradeType);
    return UpgradeStacks.IsValidIndex(Index) ? UpgradeStacks[Index] : 0;
}


TSubclassOf<UGameplayAbility> APMEPlayerState::GetAbilityClassForItem(const EPMEPickupType ItemType) const
{
    switch (ItemType)
    {
    case EPMEPickupType::Torch: return TorchAbilityClass;
    case EPMEPickupType::MapPulse: return MapPulseAbilityClass;
    case EPMEPickupType::Decoy: return DecoyAbilityClass;
    case EPMEPickupType::LightBomb: return LightBombAbilityClass;
    case EPMEPickupType::MasterKey: return MasterKeyAbilityClass;
    default: return nullptr;
    }
}
void APMEPlayerState::AddUpgrade(const EPMEUpgradeType UpgradeType)
{
    check(HasAuthority());
    const int32 Index = static_cast<int32>(UpgradeType);
    if (!UpgradeStacks.IsValidIndex(Index)) UpgradeStacks.SetNumZeroed(9);
    UpgradeStacks[Index] = FMath::Clamp(UpgradeStacks[Index] + 1, 0, 5);
    ApplyPersistentUpgradeAttributes();
    OnRep_Upgrades();
    ForceNetUpdate();
}

void APMEPlayerState::OnRep_PlayerIndex()
{
    if (APMEPlayerCharacter* Character = GetPawn<APMEPlayerCharacter>())
    {
        Character->RefreshPlayerVisual();
    }
}

void APMEPlayerState::OnRep_SelectedCharacterId()
{
    if (APMEPlayerCharacter* Character = GetPawn<APMEPlayerCharacter>())
    {
        Character->RefreshPlayerVisual();
    }
}
void APMEPlayerState::OnRep_Downed() { if (APMEPlayerCharacter* C = GetPawn<APMEPlayerCharacter>()) C->ApplyDownedState(bDowned); }
void APMEPlayerState::OnRep_Upgrades() { ApplyPersistentUpgradeAttributes(); }
