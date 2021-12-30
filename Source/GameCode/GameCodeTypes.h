#pragma once

#define ECC_Climbing ECC_GameTraceChannel1
#define ECC_InteractionVolume ECC_GameTraceChannel2
#define ECC_WallRunnable ECC_GameTraceChannel3
#define ECC_Bullet ECC_GameTraceChannel4
#define ECC_Melee ECC_GameTraceChannel5


const FName DebugCategoryLedgeDetection = FName("LedgeDetection");
const FName DebugCategoryCharacterAttributes = FName("CharacterAttributes");
const FName DebugCategoryRangeWeapon = FName("RangeWeapon");
const FName DebugCategoryMeleeWeapon = FName("MeleeWeapon");

const FName CollisionProfilePawn = FName("Pawn");
const FName CollisionProfileNoCollision = FName("NoCollision");
const FName CollisionProfilePawnInteractionVolume = FName("PawnInteractionVolume");
const FName CollisionProfileRagdoll = FName("Ragdoll");

const FName SlideToCrouchSection = FName("SlideToCrouch");

const FName SocketFPCamera = FName("CameraSocket");
const FName SocketHead = FName("head");
const FName SocketCharacterWeapon = FName("CharacterWeaponSocket");
const FName SocketBowMidString = FName("MidStringSocket");
const FName SocketArrowHand = FName("ArrowSocket");
const FName SocketWeaponMuzzle = FName("MuzzleSocket");
const FName SocketWeaponForeGrip = FName("ForeGripSocket");
const FName SocketCharacterThrowable = FName("ThrowableSocket");

const FName FXParamTraceEnd = FName("TraceEnd");
const FName SectionMontageReloadEnd = FName("ReloadEnd");

const FName BB_CurrentTarget = FName("CurrentTarget");
const FName BB_NextLocation = FName("NextLocation");
const FName ActionInteract = FName("Interact");

const FName SignificanceTagCharacter = FName("Character");

const float SignificanceValueVeryHigh = 0.0f;
const float SignificanceValueHigh = 1.0f;
const float SignificanceValueMedium = 10.0f;
const float SignificanceValueLow = 100.0f;
const float SignificanceValueVeryLow = 1000.0f;

UENUM(BlueprintType)
enum class EEquipableItemType : uint8
{
	None,
	Pistol,
	Rifle,
	Thowable,
	Melee,
	Bow
};

UENUM(BlueprintType)
enum class EAmunitionType : uint8
{
	None,
	Pistol,
	Rifle,	
	ShotgunShells,
	FragGrenades,
	RifleGrenades,
	Arrows,
	MAX UMETA(Hidden)
};

const TArray<FName, TFixedAllocator<6>> AmmoItemDataTableIDs = { FName("Pistol"), FName("Rifle"), FName("Shotgun"), FName("FragGrenades"), FName("RifleGrenades"), FName("Arrow") };

UENUM(BlueprintType)
enum class EEquipmentSlots : uint8
{
	None, 
	SideArm,
	PrimaryWeapon,
	SecondaryWeapon,
	PrimaryItemSlot,
	MeleeWeapon,
	MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EReticleType : uint8
{
	None,
	Default,
	SniperRifle,
	Bow,
	MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMeleeAttackTypes : uint8
{
	None,
	PrimaryAttack,
	SecondaryAttack,
	MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EWeaponMode : uint8
{
	None,
	Default,
	RifleGrenadeMode,
	MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EHitRegistrationType : uint8
{
	HitScan,
	Projectile
};

UENUM(BlueprintType)
enum class ETeams : uint8
{
	Player,
	Enemy
};
