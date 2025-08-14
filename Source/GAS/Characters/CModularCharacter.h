#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "CModularCharacter.generated.h"

class UAbilitySystemComponent;

struct FGameplayTagContainer;
class UCAttributeSet;

struct FOnAttributeChangeData;

UCLASS()
class GAS_API ACModularCharacter
	: public ACharacter
	, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACModularCharacter();

	// GAS가 ASC를 인식하기 위한 함수 구현
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/*
	 *	BlueprintImplementableEvent
	 *	C++에 **구현체가 없다 (= Pure virtual), BP에서 필요할 경우 구현
	 *	BP를 만들지 않으면 호출해도 아무 일도 일어나지 않아 런타임에서 안전
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Health")
	void OnHealthChanged(float InDeltaValue, const FGameplayTagContainer& InEventTags);
	
protected:
	// GAS 핵심 멤버
	// ASC: 능력, 이펙트, 속성의 "엔진"
	// AbilitySystemComponent는 GAS의 핵심으로, 캐릭터에 반드시 부착
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", Replicated)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	// AttributeSet은 체력, 마나 등 스탯을 담은 구조체
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", Replicated)
	TObjectPtr<const UCAttributeSet> AttributeSet;
	
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void InitializeAttributes(); // 기본 스탯 적용
	
private:
	void HandleHealthChanged(const FOnAttributeChangeData& InData);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
