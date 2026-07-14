#pragma once

#include "CoreMinimal.h"
#include "PMETypes.generated.h"

UENUM(BlueprintType)
enum class EPMEPlayMode : uint8
{
	SinglePlayer UMETA(DisplayName="Single Player"),
	Coop UMETA(DisplayName="Co-Op"),
	Versus UMETA(DisplayName="Versus")
};

UENUM(BlueprintType)
enum class EPMELanguage : uint8
{
	English UMETA(DisplayName="English"),
	Spanish UMETA(DisplayName="Español")
};

UENUM(BlueprintType)
enum class EPMERoundPhase : uint8
{
	WaitingForPlayers UMETA(DisplayName="Waiting For Players"),
	Exploring UMETA(DisplayName="Exploring"),
	Escape UMETA(DisplayName="Escape"),
	RoundResult UMETA(DisplayName="Round Result"),
	UpgradeSelection UMETA(DisplayName="Upgrade Selection"),
	RunComplete UMETA(DisplayName="Run Complete")
};

UENUM(BlueprintType)
enum class EPMEPickupType : uint8
{
	None UMETA(DisplayName="None"),
	Torch UMETA(DisplayName="Torch"),
	MapPulse UMETA(DisplayName="Map Pulse"),
	Decoy UMETA(DisplayName="Decoy"),
	LightBomb UMETA(DisplayName="Light Bomb"),
	MasterKey UMETA(DisplayName="Master Key")
};

UENUM(BlueprintType)
enum class EPMEUpgradeType : uint8
{
	WiderVision UMETA(DisplayName="Wider Vision"),
	QuietSteps UMETA(DisplayName="Quiet Steps"),
	StrongHeart UMETA(DisplayName="Strong Heart"),
	FasterMovement UMETA(DisplayName="Faster Movement"),
	BetterSprint UMETA(DisplayName="Better Sprint"),
	LongerTorch UMETA(DisplayName="Longer Torch"),
	BiggerMapPulse UMETA(DisplayName="Bigger Map Pulse"),
	SlowerDetection UMETA(DisplayName="Slower Detection"),
	ExtraStartingItem UMETA(DisplayName="Extra Starting Item")
};

UENUM(BlueprintType)
enum class EPMELevelModifier : uint8
{
	None UMETA(DisplayName="None"),
	Darkness UMETA(DisplayName="Darkness"),
	SensitiveHearing UMETA(DisplayName="Sensitive Hearing"),
	Fury UMETA(DisplayName="Fury"),
	NoRefuge UMETA(DisplayName="No Refuge"),
	Hunters UMETA(DisplayName="Hunters"),
	Echo UMETA(DisplayName="Echo"),
	Unstable UMETA(DisplayName="Unstable"),
	FalseExit UMETA(DisplayName="False Exit")
};

UENUM(BlueprintType)
enum class EPMEEnemyArchetype : uint8
{
	Guardian UMETA(DisplayName="Guardian"),
	Listener UMETA(DisplayName="Listener"),
	Stalker UMETA(DisplayName="Stalker")
};

UENUM(BlueprintType)
enum class EPMEObjectType : uint8
{
	Standard UMETA(DisplayName="Standard"),
	Synchronized UMETA(DisplayName="Synchronized")
};

UENUM(BlueprintType)
enum class EPMEAbilityInputID : uint8
{
	None = 0,
	Sprint = 1,
	Revive = 2
};

USTRUCT(BlueprintType)
struct FPMEUpgradeChoiceSet
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TArray<EPMEUpgradeType> Choices;
};

namespace PixelMazeMode
{
	inline FString ToOptionString(const EPMEPlayMode Mode)
	{
		switch (Mode)
		{
		case EPMEPlayMode::Coop: return TEXT("Coop");
		case EPMEPlayMode::Versus: return TEXT("Versus");
		default: return TEXT("SinglePlayer");
		}
	}

	inline EPMEPlayMode FromOptionString(const FString& Value)
	{
		if (Value.Equals(TEXT("Coop"), ESearchCase::IgnoreCase)) return EPMEPlayMode::Coop;
		if (Value.Equals(TEXT("Versus"), ESearchCase::IgnoreCase)) return EPMEPlayMode::Versus;
		return EPMEPlayMode::SinglePlayer;
	}

	inline FName ToDisplayTextKey(const EPMEPlayMode Mode)
	{
		switch (Mode)
		{
		case EPMEPlayMode::Coop: return TEXT("Mode.Coop");
		case EPMEPlayMode::Versus: return TEXT("Mode.Versus");
		default: return TEXT("Mode.SinglePlayer");
		}
	}
}

namespace PixelMazePickup
{
	inline FName ToTextKey(const EPMEPickupType Type)
	{
		switch (Type)
		{
		case EPMEPickupType::Torch: return TEXT("Item.Torch");
		case EPMEPickupType::MapPulse: return TEXT("Item.MapPulse");
		case EPMEPickupType::Decoy: return TEXT("Item.Decoy");
		case EPMEPickupType::LightBomb: return TEXT("Item.LightBomb");
		case EPMEPickupType::MasterKey: return TEXT("Item.MasterKey");
		default: return TEXT("Item.Empty");
		}
	}
}

namespace PixelMazeUpgrade
{
	inline FName ToNameKey(const EPMEUpgradeType Type)
	{
		return *FString::Printf(TEXT("Upgrade.%d.Name"), static_cast<int32>(Type));
	}

	inline FName ToDescriptionKey(const EPMEUpgradeType Type)
	{
		return *FString::Printf(TEXT("Upgrade.%d.Description"), static_cast<int32>(Type));
	}
}

namespace PixelMazeRoundPhase
{
	inline FName ToTextKey(const EPMERoundPhase Phase)
	{
		switch (Phase)
		{
		case EPMERoundPhase::WaitingForPlayers: return TEXT("Phase.Waiting");
		case EPMERoundPhase::Exploring: return TEXT("Phase.Exploring");
		case EPMERoundPhase::Escape: return TEXT("Phase.Escape");
		case EPMERoundPhase::RoundResult: return TEXT("Phase.Result");
		case EPMERoundPhase::UpgradeSelection: return TEXT("Phase.Upgrade");
		case EPMERoundPhase::RunComplete: return TEXT("Phase.RunComplete");
		default: return TEXT("Phase.Exploring");
		}
	}
}

namespace PixelMazeModifier
{
	inline FName ToTextKey(const EPMELevelModifier Modifier)
	{
		switch (Modifier)
		{
		case EPMELevelModifier::Darkness: return TEXT("Modifier.Darkness");
		case EPMELevelModifier::SensitiveHearing: return TEXT("Modifier.SensitiveHearing");
		case EPMELevelModifier::Fury: return TEXT("Modifier.Fury");
		case EPMELevelModifier::NoRefuge: return TEXT("Modifier.NoRefuge");
		case EPMELevelModifier::Hunters: return TEXT("Modifier.Hunters");
		case EPMELevelModifier::Echo: return TEXT("Modifier.Echo");
		case EPMELevelModifier::Unstable: return TEXT("Modifier.Unstable");
		case EPMELevelModifier::FalseExit: return TEXT("Modifier.FalseExit");
		default: return TEXT("Modifier.None");
		}
	}
}
