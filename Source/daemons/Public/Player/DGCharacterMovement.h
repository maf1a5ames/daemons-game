// For Daemons and something else videogame purpose only

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DGCharacterMovement.generated.h"

// clang-format off
UENUM(BlueprintType)
enum ECustomMovementMode : uint8
{
	CMOVE_None			UMETA(DisplayName = "None"),
	CMOVE_Sprint		UMETA(DisplayName = "Sprint"),
	CMOVE_MAX			UMETA(Hidden)
};

UENUM(BlueprintType)
enum EStandCondition : uint8
{
	STANDING,
	SITTING,
	NoType,

    Begin = STANDING    UMETA(Hidden),
    End = NoType		UMETA(Hidden)
};
// clang-format on

UCLASS()
class DAEMONS_API UDGCharacterMovement : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	bool bWantsToSprint{false};

	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual float GetMaxSpeed() const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SprintSpeed{600.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (clampmin = "0.01", clampmax = "2"))
	float SitdownProcessDuration{0.5f};

protected:
	virtual void InitializeComponent() override;
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
	
private:
    void PhysSprint(float DeltaTime, int32 Iterations);

	bool bIsCrouching{false};
	float TrackedProcessTime{0.f};
	float DefaultHalfHeight{0.f};
	void EnterCrouching(float DeltaSeconds);
	void ExitCrouching(float DeltaSeconds);

	TMap<EStandCondition, float> HalfHeightMap;
	EStandCondition StandCondition{EStandCondition::STANDING};
	void SetStandCondition(EStandCondition ConditionType) { StandCondition = ConditionType; };
	float GetHalfHeightByStandCondition() { return HalfHeightMap[StandCondition]; };
	bool HasTopObstacle(float HeightToCheck);
	void ChangeCapsuleHalfHeight(float ShortHalfHeight, float DeltaSeconds, float Modifier);

	bool IsCustomMovementMode(const ECustomMovementMode InCustomMovementMode) const;

};
