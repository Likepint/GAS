#include "AbilitySystem/CAttributeSet.h"

// GameplayEffectModCallbackData 구조체를 포함하는 필수 헤더
// PostGameplayEffectExecute 함수에서 이 타입을 사용하므로 반드시 포함
#include "GameplayEffectExtension.h"

// GAS의 네트워크 복제 처리를 위해 반드시 필요한 헤더
#include "Net/UnrealNetwork.h"


UCAttributeSet::UCAttributeSet()
{
	// 현재 체력 초기값 설정 (예: 캐릭터가 생성되었을 때, 체력 100으로 시작)
	// CurrentHealth = 100.0f;
	CurrentHealth.SetBaseValue(100.0f);
	CurrentHealth.SetCurrentValue(100.0f);

	// 최대 체력도 100으로 설정
	// MaxHealth = 100.0f;
	MaxHealth.SetBaseValue(100.0f);
	MaxHealth.SetCurrentValue(100.0f);
}

/*
 * RepNotify 함수들 (서버 -> 클라이언트 복제 시 호출)
 */

// CurrentHealth 값이 서버에서 클라이언트로 복제되었을 때 호출되는 함수
void UCAttributeSet::OnRep_CurrentHealth(const FGameplayAttributeData& OldCurHealth)
{
	// GAS 내부 매크로: 내부 시스템에 값 변경 알림을 전달
	// UI 업데이트, 이펙트 발동 등과 연동될 수 있음
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet, CurrentHealth, OldCurHealth);
}

// MaxHealth 값 복제 시 호출
void UCAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet, MaxHealth, OldMaxHealth);
}

/*
 * 속성 변경 전 개입 (예: 체력이 MaxHealth를 초과하지 않게 제한) 
 */

void UCAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewHealth)
{
	Super::PreAttributeChange(Attribute, NewHealth);

	// 변경 대상이 현재 체력일 경우
	if (Attribute == GetCurrentHealthAttribute())
	{
		const float maxHealth = MaxHealth.GetCurrentValue();

		// 체력 값을 0 이상, 최대 체력 이하로 제한
		NewHealth = FMath::Clamp(NewHealth, 0.0f, maxHealth);
	}
}

/*
 * GameplayEffect 적용 후 처리 (예: 데미지 받은 후 체력 감소 확인)
 */

void UCAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetCurrentHealthAttribute())
	{
		float newHealth = FMath::Clamp(CurrentHealth.GetCurrentValue(), 0.0f, MaxHealth.GetCurrentValue());
		SetCurrentHealth(newHealth);
	}
}

/*
 * 네트워크 복제 설정 (서버 -> 클라이언트로 복제될 속성 정의)
 */

void UCAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// CurrentHealth를 항상 복제하고, 값이 바뀔 때 마다 OnRep 함수 호출
	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet, CurrentHealth, COND_None, REPNOTIFY_Always);

	// MaxHealth도 동일하게 복제 설정
	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
}