﻿// Fill out your copyright notice in the Description page of Project Settings.

#include "XD_ProxyArchiveBase.h"
#include <Components/PrimitiveComponent.h>
#include <Engine/Level.h>
#include <GameFramework/Actor.h>
#include <GameFramework/Character.h>
#include "GameFramework/CharacterMovementComponent.h"
#include <Engine/World.h>
#include <TimerManager.h>
#include <Kismet/GameplayStatics.h>

FXD_ActorExtraSaveData::FXD_ActorExtraSaveData(AActor* Actor)
	:FXD_ActorExtraSaveData()
{
	Transform = Actor->GetActorTransform();
	if (ACharacter* Character = Cast<ACharacter>(Actor))
	{
		LinearVelocity = Character->GetVelocity();
	}
	else if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
	{
		if (PrimitiveComponent->IsSimulatingPhysics())
		{
			LinearVelocity = Actor->GetVelocity();
			AngularVelocity = PrimitiveComponent->GetPhysicsAngularVelocityInDegrees();
		}
	}
}

void FXD_ActorExtraSaveData::LoadData(AActor* Actor, ULevel* Level, const FIntVector& OldWorldOrigin) const
{
	FTransform ActorTransForm = Transform;

	auto GetFixedWorldLocation = [](const UObject* WorldContextObject, const FIntVector& OldWorldOrigin, const FVector& WorldLocation)
	{
		FIntVector WorldOffset = OldWorldOrigin - UGameplayStatics::GetWorldOriginLocation(WorldContextObject);
		return FVector(WorldLocation.X + WorldOffset.X, WorldLocation.Y + WorldOffset.Y, WorldLocation.Z + WorldOffset.Z);
	};
	
	ActorTransForm.SetLocation(GetFixedWorldLocation(Level, OldWorldOrigin, ActorTransForm.GetLocation()));
	Actor->SetActorTransform(ActorTransForm, false, nullptr, ETeleportType::TeleportPhysics);

	if (ACharacter* Character = Cast<ACharacter>(Actor))
	{
		if (!LinearVelocity.IsNearlyZero())
		{
			if (UCharacterMovementComponent* CharacterMovementComponent = Character->GetCharacterMovement())
			{
				// 假如速度太大需要检查原因，防止角色飞出边界被销毁
				if (ensure(LinearVelocity.Size() < 1000000.f))
				{
					// 直接设置Launch无效，因为MovementMode为None，要先设置MovementMode
					CharacterMovementComponent->SetMovementMode(MOVE_Walking);
					CharacterMovementComponent->Launch(LinearVelocity);
				}
			}
		}
	}
	else if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
	{
		if (PrimitiveComponent->IsSimulatingPhysics())
		{
			if (!LinearVelocity.IsNearlyZero())
			{
				PrimitiveComponent->SetPhysicsLinearVelocity(LinearVelocity);
			}
			if (!AngularVelocity.IsNearlyZero())
			{
				PrimitiveComponent->SetPhysicsAngularVelocityInDegrees(AngularVelocity);
			}
		}
	}
}

FArchive& FXD_ProxyArchiveBase::operator<<(FLazyObjectPtr& Value)
{
	FArchive& Ar = *this;
	// We never serialize our reference while the garbage collector is harvesting references
	// to objects, because we don't want weak object pointers to keep objects from being garbage
	// collected.  That would defeat the whole purpose of a weak object pointer!
	// However, when modifying both kinds of references we want to serialize and writeback the updated value.
	// We only want to write the modified value during reference fixup if the data is loaded
	if (!IsObjectReferenceCollector() || IsModifyingWeakAndStrongReferences())
	{
#if WITH_EDITORONLY_DATA
		// When transacting, just serialize as a guid since the object may
		// not be in memory and you don't want to save a nullptr in this case.
		if (IsTransacting())
		{
			if (Ar.IsLoading())
			{
				// Reset before serializing to clear the internal weak pointer. 
				Value.Reset();
			}
			Ar << Value.GetUniqueID();
		}
		else
#endif
		{
			UObject* Object = Value.Get();

			Ar << Object;

			if (IsLoading() || (Object && IsModifyingWeakAndStrongReferences()))
			{
				Value = Object;
			}
		}
	}
	return Ar;
}

FArchive& FXD_ProxyArchiveBase::operator<<(FSoftObjectPtr& Value)
{
	FArchive& Ar = *this;
	if (Ar.IsSaving() || Ar.IsLoading())
	{
		// Reset before serializing to clear the internal weak pointer. 
		Value.ResetWeakPtr();
		Ar << Value.GetUniqueID();
	}
	else if (!IsObjectReferenceCollector() || IsModifyingWeakAndStrongReferences())
	{
		// Treat this like a weak pointer object, as we are doing something like replacing references in memory
		UObject* Object = Value.Get();

		Ar << Object;

		if (IsLoading() || (Object && IsModifyingWeakAndStrongReferences()))
		{
			Value = Object;
		}
	}

	return Ar;
}

FArchive& FXD_ProxyArchiveBase::operator<<(FSoftObjectPath& Value)
{
	Value.SerializePath(*this);
	return *this;
}

FArchive& FXD_ProxyArchiveBase::operator<<(FWeakObjectPtr& Value)
{
	Value.Serialize(*this);
	return *this;
}
