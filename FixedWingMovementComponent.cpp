// Fill out your copyright notice in the Description page of Project Settings.


#include "FixedWingMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UFixedWingMovementComponent::ApplyAirfoil(FName Socket, float Deflection, FAirfoilProperties Airfoil, float Scale, float DeltaTime) {
	if(!(IsValid(Airfoil.ControlSurfaceCurve) && IsValid(Airfoil.LiftCurve) && IsValid(Airfoil.FormDragCurve))) {
		UE_LOG(LogTemp, Warning, TEXT("Warning invalid airfoil curves."));
		return;
	}
	
	auto Primitive = this->UpdatedPrimitive;
	auto Location = Primitive->GetSocketLocation(Socket);
	auto Rotation = Primitive->GetSocketRotation(Socket);
	auto LinearVelocity = Primitive->GetPhysicsLinearVelocityAtPoint(Location,NAME_None);// Socket);


	//UE_LOG(LogTemp, Warning, TEXT("[%s] Primitive: %s | LV: %s"), *Socket.ToString(), *AActor::GetDebugName(Primitive->GetOwner()), *LinearVelocity.ToString());
	
	auto AltitudeMeters = Location.Z / 100.f; // for air density
	auto AirDensity = FMath::Lerp(1.225f, 0.4f, UKismetMathLibrary::NormalizeToRange(AltitudeMeters, 0.f, 15000.f));

	auto LinearVelocityMS = LinearVelocity / 100.f; // formulas are in m/s
	//auto TotalSpeedMS = LinearVelocityMS.Size();

	auto ForwardVector = Rotation.Vector();
	auto UpVector = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Z);
	auto RightVector = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);

	auto ForwardSpeedMS = FVector::DotProduct(ForwardVector, LinearVelocityMS);
	auto UpSpeedMS = FVector::DotProduct(UpVector, LinearVelocityMS);
	//auto RightSpeedMS = FVector::DotProduct(RightVector, LinearVelocityMS);

	auto Alpha = (180.f) / PI * FMath::Atan2(UpSpeedMS, ForwardSpeedMS);
	//auto Slip = (180.f) / PI * FMath::Atan2(RightSpeedMS, ForwardSpeedMS);

	auto AirStream = (ForwardVector * ForwardSpeedMS) + (UpVector * UpSpeedMS);

	auto StreamSpeed = AirStream.Size();

	auto StreamForward = AirStream.GetSafeNormal(0.0001f);
	auto StreamUp = StreamForward.RotateAngleAxis(-90.f, RightVector).GetSafeNormal(0.0001f);


	//Deflection
	auto ForceDeflection = Airfoil.ControlSurfaceCurve->GetFloatValue(Alpha) * Deflection;


	// lift
	auto CoefficientOfLift = (Airfoil.LiftCurve->GetFloatValue(Alpha) + ForceDeflection);
	auto StreamSpeedSquared = FMath::Pow(StreamSpeed, 2);
	auto ForceLift = CoefficientOfLift * StreamSpeedSquared * AirDensity * Airfoil.Area * -NewtonScale * StreamUp; //todo does this making this newton scale negative as well fix ?

	// check distance from ground
	FHitResult HitRes;
	auto InGroundEffect = GWorld->LineTraceSingleByChannel(HitRes, Location, Location-(UpVector*1000.f),ECollisionChannel::ECC_WorldStatic);

	if(InGroundEffect) {
		VisualizeForces(Location, HitRes.Location, 3, DeltaTime);
	}

	auto GroundEffectDragReduction =InGroundEffect ? 0.65f : 1.0f; // reduce induced drag by 65% when in ground effect
	
	// induced drag
	auto ForceInducedDrag = GroundEffectDragReduction * (FMath::Pow(CoefficientOfLift, 2) * StreamSpeedSquared * AirDensity) / (PI * Airfoil.AspectRatio * Airfoil.Efficiency) * -NewtonScale * StreamUp;
	// efficiency between .7 - 1.0 typical

	// form / deflection drag
	auto ForceFormDrag = ((Airfoil.FormDragCurve->GetFloatValue(Alpha) + (FMath::Abs(Deflection) * Airfoil.DeflectionDrag)) + Airfoil.SkinFriction) * StreamSpeedSquared * AirDensity * -NewtonScale * StreamUp;

	//UE_LOG(LogTemp, Warning, TEXT("[%s] Total Forces Exerted: %s"), *Socket.ToString(), *((ForceLift + ForceInducedDrag + ForceFormDrag) * Scale * DeltaTime).ToString());

	//VisualizeForces(Location, Location + (StreamForward / Primitive->GetMass()), 0, DeltaTime);
	VisualizeForces(Location, Location + (ForceLift / Primitive->GetMass()), 1, DeltaTime);
	VisualizeForces(Location+RightVector*10, Location+ ((ForceFormDrag + ForceInducedDrag) / Primitive->GetMass()), 2, DeltaTime);
	//VisualizeForces(Location, Location+ (ForceFormDrag / Primitive->GetMass()), 3, DeltaTime);
	VisualizeForces(Location+RightVector*20, Location+ ((ForceLift + ForceInducedDrag + ForceFormDrag) * Scale * DeltaTime / Primitive->GetMass()), 4, DeltaTime);
	
	Primitive->AddForceAtLocation((ForceLift + ForceInducedDrag + ForceFormDrag) * Scale * DeltaTime, Location, NAME_None); // todo SkeletalMesh.GetSocketBoneName(Socket)
	//Primitive->AddForce(((ForceLift + ForceInducedDrag + ForceFormDrag) * Scale * DeltaTime));
}

void UFixedWingMovementComponent::SetDeflection(FName Socket, float Deflection) {
	Deflections.Add(Socket, Deflection);
}

void UFixedWingMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	TArray<FName> AirfoilNames;
	Airfoils.GetKeys(AirfoilNames);

	for(auto&elem: AirfoilNames) {
		if(Airfoils.Contains(elem)){
		
		float Deflection = 0.f;
		
		if(Deflections.Contains(elem)) {
			Deflection = *Deflections.Find(elem);
		}
		
		ApplyAirfoil(elem, Deflection, *Airfoils.Find(elem), AerodynamicsScale, DeltaTime);
		}
	}
}

// if within .5 wing length to the ground reduce induced drag for ground effect
