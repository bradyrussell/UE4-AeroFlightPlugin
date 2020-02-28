// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
	#include "Components/PrimitiveComponent.h"
#include "SimpleWheeledVehicleMovementComponent.h"
#include "Curves/CurveFloat.h"
#include "FixedWingMovementComponent.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType, meta=(BlueprintSpawnableComponent))
struct FAirfoilProperties
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Area;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Efficiency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AspectRatio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DeflectionDrag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SkinFriction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* LiftCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* FormDragCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* ControlSurfaceCurve;
	
};

UCLASS(BlueprintType, Blueprintable)
class AIRCOMBAT_API UFixedWingMovementComponent : public UMovementComponent// public USimpleWheeledVehicleMovementComponent
{
	GENERATED_BODY()
public:
		UFUNCTION(BlueprintCallable)
		void ApplyAirfoil(FName Socket, float Deflection, FAirfoilProperties Airfoil, float Scale, float DeltaTime);
	
		UFUNCTION(BlueprintCallable)
		void SetDeflection(FName Socket, float Deflection);

	UFUNCTION(BlueprintImplementableEvent)
		void VisualizeForces(FVector Start, FVector End, uint8 Type, float DeltaTime);
	
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TMap<FName, FAirfoilProperties> Airfoils;

		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TMap<FName, float> Deflections;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AerodynamicsScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float NewtonScale = 50.f;

	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
	
};
