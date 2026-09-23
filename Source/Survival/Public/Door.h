#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Interactable.h"
#include "Door.generated.h"

/** Represents a world door whose open state is replicated to clients. */
UCLASS()
class SURVIVAL_API ADoor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ADoor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UBoxComponent> CollisionComponent;

	UPROPERTY(ReplicatedUsing = OnRep_IsOpen, BlueprintReadOnly, Category = "Door", meta = (DisplayName = "IsOpen"))
	bool bIsOpen = false;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SetIsOpen(const bool bNewIsOpen);

	virtual void Interact_Implementation(AController* InteractingController) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_IsOpen(const bool bPreviousIsOpen) const;
};
