#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "CAttributeSet.generated.h"

// #define: 매크로로 속성 접근 함수 묶음 생성
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 캐릭터의 기본 스탯(체력, 마나 등)을 정의하는 AttributeSet 클래스
 */
UCLASS()
class GAS_API UCAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	// Constructor
	UCAttributeSet();

	// 체력 (현재 값)
	UPROPERTY(BlueprintReadOnly, Category ="Health", ReplicatedUsing = OnRep_CurrentHealth)
	FGameplayAttributeData CurrentHealth;

	// 체력 (현재 값) 헬퍼 함수 생성 매크로
	ATTRIBUTE_ACCESSORS(UCAttributeSet, CurrentHealth)

	// 체력 (최대 값)
	UPROPERTY(BlueprintReadOnly, Category ="Health", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;

	// 체력 (최대 값) 헬퍼 함수 생성 매크로
	ATTRIBUTE_ACCESSORS(UCAttributeSet, MaxHealth)

protected:
	// 체력 값이 네트워크로 복제될 때 호출되는 함수
	// GAS 내부적으로는 OnRep_XXX에 virtual은 필요하지 않지만,
	// 지금 구조처럼 상속 또는 확장을 염두에 두는 경우에는 붙이는 것도 가능
	// 실질적 기능 변화는 없지만 override 가능성을 명시한 코드 스타일
	UFUNCTION()
	void OnRep_CurrentHealth(const FGameplayAttributeData& OldCurHealth);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	// 속성이 변경되기 직전에 호출 (예: 체력이 Max보다 클 경우 제한)
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewHealth) override;

	// GameplayEffect가 적용된 후 호출 (예: 데미지 후 죽음 체크)
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
