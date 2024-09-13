// For Daemons and something else videogame purpose only

#include "Player/DGCharacterMovement.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

void UDGCharacterMovement::UpdateCharacterStateBeforeMovement(float DeltaSeconds) 
{
    const bool bIsSprinting{IsCustomMovementMode(CMOVE_Sprint)};

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange, UEnum::GetValueAsString(TEnumAsByte<ECustomMovementMode>(CustomMovementMode)));

    if (bWantsToSprint                              //
        && !Velocity.IsNearlyZero()                 //
        && !bIsCrouching							//
		&& !bWantsToCrouch)
    {
        SetMovementMode(MOVE_Custom, CMOVE_Sprint);
    }
    if (!bWantsToSprint && bIsSprinting)
    {
        SetMovementMode(DefaultLandMovementMode);
    }

	if (bWantsToCrouch								//
		&& CanCrouchInCurrentState())				//
	{
		EnterCrouching(DeltaSeconds);
	}
	if ((!bWantsToCrouch || !CanCrouchInCurrentState())				//
		&& bIsCrouching
		&& !FMath::IsNearlyZero(TrackedProcessTime)					//
		&& (MovementMode != MOVE_Custom))							//
	{
		ExitCrouching(DeltaSeconds);
	}
}

float UDGCharacterMovement::GetMaxSpeed() const
{
    const float MaxSpeed = Super::GetMaxSpeed();
	if (MovementMode != MOVE_Custom)
	{
		return MaxSpeed;
	}

	switch (CustomMovementMode)
	{
		case CMOVE_Sprint : return SprintSpeed;
		default: return MaxSpeed;
	}
}

void UDGCharacterMovement::InitializeComponent() 
{
    Super::InitializeComponent();

	if (CharacterOwner)
    {
		const auto DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
		DefaultHalfHeight = DefaultCharacter 
			?
			DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()
			:
			CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	}

	/* halfheight depends of condition */
	HalfHeightMap.Add(EStandCondition::STANDING, DefaultHalfHeight);
	HalfHeightMap.Add(EStandCondition::SITTING, GetCrouchedHalfHeight());
}

void UDGCharacterMovement::PhysCustom(float DeltaTime, int32 Iterations) 
{
    Super::PhysCustom(DeltaTime, Iterations);

	switch (CustomMovementMode)
	{
        case CMOVE_Sprint:
			PhysSprint(DeltaTime, Iterations);
			break;
		default:
			UE_LOG(LogTemp, Error, TEXT("Invalid Movement Mode"))
	}
}

void UDGCharacterMovement::PhysSprint(float DeltaTime, int32 Iterations) 
{
    PhysWalking(DeltaTime, Iterations);
}

void UDGCharacterMovement::EnterCrouching(float DeltaSeconds) 
{
	if (!CharacterOwner || FMath::IsNearlyZero(SitdownProcessDuration)) return;

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange, "Enter Crouching");

	SetMovementMode(MOVE_Walking);
	bIsCrouching = true;

	const float IncreaseTimeModifier = 1.f;
	ChangeCapsuleHalfHeight(GetCrouchedHalfHeight(), DeltaSeconds, IncreaseTimeModifier);
	if (FMath::IsNearlyEqual(GetCrouchedHalfHeight(), CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()))
    {
        Crouch(false);
		TrackedProcessTime = SitdownProcessDuration;
		SetStandCondition(EStandCondition::SITTING);
    }
}

void UDGCharacterMovement::ExitCrouching(float DeltaSeconds) 
{
	if (!CharacterOwner || HasTopObstacle(DefaultHalfHeight) || FMath::IsNearlyZero(SitdownProcessDuration)) return;

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange, "Exit Crouching");

	const float DecreaseTimeModifier = -1.f;
	ChangeCapsuleHalfHeight(GetCrouchedHalfHeight(), DeltaSeconds, DecreaseTimeModifier);

	if (FMath::IsNearlyEqual(GetHalfHeightByStandCondition(), CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()))
    {
        UnCrouch(false);
		TrackedProcessTime = 0.f;
		bIsCrouching = false;
		SetMovementMode(DefaultLandMovementMode);
    }    
}

bool UDGCharacterMovement::HasTopObstacle(float HeightToCheck)
{
    if (!CharacterOwner) return false;
    
    const float ComponentScale = CharacterOwner->GetCapsuleComponent()->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const float HalfHeightAdjust = HeightToCheck - OldUnscaledHalfHeight;
	
	if (FMath::IsNearlyZero(HalfHeightAdjust)) return false;
	
	const float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
    const FVector SweepStart = CharacterOwner->GetCapsuleComponent()->GetComponentLocation();
    const FVector SweepEnd = SweepStart + CharacterOwner->GetCapsuleComponent()->GetUpVector() * ScaledHalfHeightAdjust;
	
    const FCollisionShape SphereShape = FCollisionShape::MakeSphere(CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius());
    const ECollisionChannel CollisionChannel = CharacterOwner->GetCapsuleComponent()->GetCollisionObjectType();
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(CharacterOwner);
    TArray<FHitResult> OutResults;
    GetWorld()->SweepMultiByChannel(OutResults, SweepStart, SweepEnd, FQuat::Identity, CollisionChannel, SphereShape, QueryParams);
    for (const FHitResult& OutResult : OutResults)
    {
        if (OutResult.bBlockingHit) return true;
    }

    return false;
}

void UDGCharacterMovement::ChangeCapsuleHalfHeight(float ShortHalfHeight, float DeltaSeconds, float Modifier) 
{
	const float Value = FMath::Clamp(TrackedProcessTime += DeltaSeconds * Modifier, 0.f, SitdownProcessDuration) / SitdownProcessDuration;
    const float HalfHeight = FMath::Lerp(GetHalfHeightByStandCondition(), ShortHalfHeight, Value);
    CharacterOwner->GetCapsuleComponent()->SetCapsuleHalfHeight(HalfHeight);
}

bool UDGCharacterMovement::IsCustomMovementMode(const ECustomMovementMode InCustomMovementMode) const
{
    return MovementMode == MOVE_Custom && CustomMovementMode == InCustomMovementMode;
}
