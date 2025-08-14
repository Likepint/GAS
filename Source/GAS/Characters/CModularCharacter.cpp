#include "Characters/CModularCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CAttributeSet.h"

#include "Net/UnrealNetwork.h"

ACModularCharacter::ACModularCharacter()
{
	// 1. ASC 생성 -> 네트워크 설정
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	// 2. AttributeSet 생성 (ASC가 자동 소유)
	AttributeSet = CreateDefaultSubobject<UCAttributeSet>(TEXT("AttributeSet"));
}

// ASC 반환 (GAS 필수)
UAbilitySystemComponent* ACModularCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// BeginPlay: 기본 스탯 적용, 체력 변화 델리게이트 설치
void ACModularCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitializeAttributes(); // GameplayEffect 한 방으로 전체 스탯 설정

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UCAttributeSet::GetCurrentHealthAttribute()).AddUObject(this, &ACModularCharacter::HandleHealthChanged);
}

void ACModularCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACModularCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ACModularCharacter::InitializeAttributes()
{
	if (!AbilitySystemComponent or !AttributeSet)
	{
		// [안전 장치] ASC 또는 AttributeSet이 nullptr인 경우를 대비
		UE_LOG(LogTemp, Warning, TEXT("InitializeAttributes() called with nullptr ASC or AttributeSet"));
		return;
	}
	
	if (AbilitySystemComponent and AttributeSet)
	{
		/*
		 *	InitializeAttributes()
		 *	"기본 스탯을 GameplayEffect 한 방으로 초기화" - 데이터, 디자이너 중심 패턴
		 *
		 *	1. GameplayEffect 자산(GE_DefaultAttributes.uasset) 동적 로드
		 *	2. EffectContext 생성 -> '시전자(this)'6/MSP정보 주입
		 *	3. OutgoingSpec(설계도) 작성 -> 레벨, 스택, 지속시간 메타데이터 포함
		 *	4. Spec 실행(Apply...) -> AttributeSet의 Base/Current 값을 한 번에 설정
		 *		-> 서버 권한이면 값이 자동으로 Replication -> 클라이언트 HUD 즉시 동기화
		 */

		/*
		 *	1. 초기 스탯 GameplayEffect 로드
		 *		- 디자이너가 밸런스 조정할 때, C++ 재컴파일 없이 숫자만 바꿀 수 있도록
		 *		- 경로는 데이터테이블/Config로 외부화 해도 좋음
		 */
		static const TCHAR* defaultPath = TEXT("/Game/GAS/GE_DefaultAttributes.GE_DefaultAttributes_C");

		UClass* defaultClass = StaticLoadClass(UGameplayEffect::StaticClass(), nullptr, defaultPath);
		if (!defaultClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("InitializeAttributes() failed to load GE_DefaultAttributes.GE_DefaultAttributes_C"));
			return;
		}

		/*
		 *	2. EffectContext: 시전자(소스) 정보 주입
		 *		- 나중에 Execution Calculation, GameplayCue 등에서 SourceActor 참조 가능
		 */
		FGameplayEffectContextHandle contextHandle = AbilitySystemComponent->MakeEffectContext();
		contextHandle.AddSourceObject(this);

		/*
		 *	3. OutgoingSpec 생성
		 *		- Level 인자: 스탯을 '캐릭터 레벨'에 따라 스케일링하려면 이 값 활용
		 */
		const float effectLevel = 1.0f;
		FGameplayEffectSpecHandle specHandle = AbilitySystemComponent->MakeOutgoingSpec(defaultClass, effectLevel, contextHandle);

		/*
		 *	4. Spec 유효성 검사 후 Apply
		 *		- 서버 Authority에서 한 번만 호출: 값이 클라이언트로 자동 Replication
		 */
		if (specHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());

			// [디버그] 초기 체력 로그
			UE_LOG(LogTemp, Log, TEXT("Initial Health set to %.0f / %.0f"), AttributeSet->GetCurrentHealth(), AttributeSet->GetMaxHealth());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("InitializeAttributes() failed to create GameplayEffectSpec"));
		}
	}
}

/*
 *	HandleHealthChanged
 *	AttributeSet(PostGameplayEffectExecute -> Character 로 전달된
 *	FOnAttributeChangeData를 받아서
 *		1. 변화량(Delta) 계산
 *		2. BP 이벤트(OnHealthChanged) Broadcast
 *		3. 추가로 사망, 히트 이펙트 트리거 등도 일괄 처리 가능
 *
 *	"게임플레이 로직 한 곳 집중" 원칙
 *		- AttributeSet은 **숫자 관리**만, 캐릭터는 **게임 규칙** 담당
 *		- 디버깅 시 Health 변화가 모두 여기로 모이므로 Breakpoint 단일화
 */

void ACModularCharacter::HandleHealthChanged(const FOnAttributeChangeData& InData)
{
	float newHealth = InData.NewValue;
	float oldHealth = InData.OldValue;

	float deltaHealth = newHealth - oldHealth;

	/*
	 *	1. HUD, BP용 Event Broadcast
	 *		- UI에선 deltaHealth 음/양수 체크 -> HitFlash or HealEffect
	 *		- EventTags 인자를 전달하면 '화상/독 데미지' 등 태그별 분기도 가능
	 */
	OnHealthChanged(deltaHealth, FGameplayTagContainer());

	/*
	 *	2. 사망 처리 - 서버 전용
	 *	if (HasAuthority() and newHealth <= 0.0f and !bIsDead)
	 *	{
	 *		bIsDead = true;
	 *		Die();	// Ragdoll, RespawnTimer, Tag 추가 등
	 *	}
	 *
	 *	필요 시 위와 같이 중앙에서 사망, 히트 이펙트 등을 일괄 처리
	 */
}

/*
 *	GetLifetimeReplicatedProps
 *	네트워크 복제 설정
 *	  - ASC는 SetIsReplicated(true)로 '서브오브젝트 복제' 경로를 이미 탑니다.
 *		그러나 포인터 멤버 자체도 소유자 클라이언트에 알려주면
 *		DEBUG 시 'Valid but not replicated' 경고 방지 가능
 *	  - AttributeSet 서브오젝트 역시 동일 이유로 등록
 *	  - 새로 추가한 bool bIsDead, TeamID 등도 여기서 DOREPLIFETIME
 */
void ACModularCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// ASC, AttributeSet 포인터 복제 (주로 디버그, 리플레이 안정 목적)
	DOREPLIFETIME(ACModularCharacter, AbilitySystemComponent);
	DOREPLIFETIME(ACModularCharacter, AttributeSet);

	// 예) 사망 플래그, 팀 ID등도 추가 복제
	// DOREPLIFETIME(ACModularCharacter, bIsDead);
	// DOREPLIFETIME_CONDITION(ACModularCharacter, TeamID, COND_None);
}
