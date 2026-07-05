// Project Nova — Player character (vertical slice stub)

#include "NovaPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "ElementAbilityComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "IdentityOverrideComponent.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "NovaDebugSubsystem.h"
#include "StoryDirectorSubsystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogNovaCharacter);

ANovaPlayerCharacter::ANovaPlayerCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(35.f, 90.f);

	// Orient to movement, don't rotate the character with the controller.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 500.f, 0.f);
	Movement->JumpZVelocity = 500.f;
	Movement->AirControl = 0.35f;
	Movement->MaxWalkSpeed = 500.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	ElementAbilityComp = CreateDefaultSubobject<UElementAbilityComponent>(TEXT("ElementAbilityComp"));
	IdentityOverrideComp = CreateDefaultSubobject<UIdentityOverrideComponent>(TEXT("IdentityOverrideComp"));
}

void ANovaPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	TryAddDefaultMappingContext(TEXT("NotifyControllerChanged"));
}

void ANovaPlayerCharacter::TryAddDefaultMappingContext(const TCHAR* Caller)
{
	const APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (!PlayerController)
	{
		UE_LOG(LogNovaCharacter, Log, TEXT("[%s] TryAddDefaultMappingContext: no PlayerController yet"), Caller);
		return;
	}

	const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		UE_LOG(LogNovaCharacter, Warning, TEXT("[%s] TryAddDefaultMappingContext: PlayerController has no LocalPlayer"), Caller);
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	if (!InputSubsystem)
	{
		UE_LOG(LogNovaCharacter, Warning, TEXT("[%s] TryAddDefaultMappingContext: no EnhancedInputLocalPlayerSubsystem"), Caller);
		return;
	}

	if (!DefaultMappingContext)
	{
		UE_LOG(LogNovaCharacter, Warning, TEXT("[%s] TryAddDefaultMappingContext: DefaultMappingContext is null (BP field unassigned?)"), Caller);
		return;
	}

	if (InputSubsystem->HasMappingContext(DefaultMappingContext))
	{
		UE_LOG(LogNovaCharacter, Log, TEXT("[%s] TryAddDefaultMappingContext: '%s' already added"),
			Caller, *DefaultMappingContext->GetName());
	}
	else
	{
		InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
		UE_LOG(LogNovaCharacter, Log, TEXT("[%s] TryAddDefaultMappingContext: added '%s' (priority 0)"),
			Caller, *DefaultMappingContext->GetName());
	}
}

void ANovaPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		UE_LOG(LogNovaCharacter, Error,
			TEXT("Expected an EnhancedInputComponent — check Project Settings > Input default classes."));
		return;
	}

	// Input component exists and possession is done here — safe point to ensure the IMC is applied.
	TryAddDefaultMappingContext(TEXT("SetupPlayerInputComponent"));

	UE_LOG(LogNovaCharacter, Log,
		TEXT("SetupPlayerInputComponent on %s: Move=%d Look=%d Jump=%d Dash=%d Cast1=%d Cast2=%d StoryA=%d StoryB=%d Debug=%d IMC=%d"),
		*GetClass()->GetName(),
		IA_Move != nullptr, IA_Look != nullptr, IA_Jump != nullptr, IA_Dash != nullptr,
		IA_CastAbility1 != nullptr, IA_CastAbility2 != nullptr, IA_TriggerStoryA != nullptr,
		IA_TriggerStoryB != nullptr, IA_ToggleDebug != nullptr, DefaultMappingContext != nullptr);

	// Input assets are assigned in the character Blueprint; unassigned actions are skipped.
	if (IA_Move) { Input->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ANovaPlayerCharacter::Move); }
	if (IA_Look) { Input->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ANovaPlayerCharacter::Look); }
	if (IA_Jump)
	{
		Input->BindAction(IA_Jump, ETriggerEvent::Started, this, &ANovaPlayerCharacter::JumpStart);
		Input->BindAction(IA_Jump, ETriggerEvent::Completed, this, &ANovaPlayerCharacter::JumpStop);
	}
	if (IA_Dash) { Input->BindAction(IA_Dash, ETriggerEvent::Started, this, &ANovaPlayerCharacter::Dash); }
	if (IA_CastAbility1) { Input->BindAction(IA_CastAbility1, ETriggerEvent::Started, this, &ANovaPlayerCharacter::CastAbility1); }
	if (IA_CastAbility2) { Input->BindAction(IA_CastAbility2, ETriggerEvent::Started, this, &ANovaPlayerCharacter::CastAbility2); }
	if (IA_TriggerStoryA) { Input->BindAction(IA_TriggerStoryA, ETriggerEvent::Started, this, &ANovaPlayerCharacter::TriggerStoryA); }
	if (IA_TriggerStoryB) { Input->BindAction(IA_TriggerStoryB, ETriggerEvent::Started, this, &ANovaPlayerCharacter::TriggerStoryB); }
	if (IA_ToggleDebug) { Input->BindAction(IA_ToggleDebug, ETriggerEvent::Started, this, &ANovaPlayerCharacter::ToggleDebug); }
}

bool ANovaPlayerCharacter::IsControlHeld() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UStoryDirectorSubsystem* Story = GameInstance->GetSubsystem<UStoryDirectorSubsystem>())
		{
			return Story->GetControlMask() == ENovaControlMask::Hold;
		}
	}
	return false;
}

void ANovaPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	if (!Controller || IsControlHeld())
	{
		return;
	}

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void ANovaPlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	if (Controller && !IsControlHeld())
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ANovaPlayerCharacter::JumpStart()
{
	if (!IsControlHeld())
	{
		Jump();
	}
}

void ANovaPlayerCharacter::JumpStop()
{
	StopJumping();
}

void ANovaPlayerCharacter::Dash()
{
	if (IsControlHeld())
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < NextDashTime)
	{
		return;
	}
	NextDashTime = Now + DashCooldown;

	// Dash along current movement input, falling back to facing direction.
	FVector Direction = GetCharacterMovement()->GetLastInputVector().GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		Direction = GetActorForwardVector().GetSafeNormal2D();
	}

	// Impulse sized so the dash carries roughly DashDistance over DashDurationSeconds.
	// LaunchCharacter always puts the character into Falling; on the ground it lands and
	// friction brakes it, but in the air nothing decays lateral velocity, so EndDash has
	// to cap the burst or an airborne dash keeps flying until landing.
	// Floor the duration: BP writes bypass ClampMin, and 0 would mean a division by zero
	// here plus a timer that never fires (SetTimer with rate <= 0 only clears).
	const float Duration = FMath::Max(DashDurationSeconds, 0.05f);
	LaunchCharacter(Direction * (DashDistance / Duration), true, false);
	GetWorldTimerManager().SetTimer(DashEndTimerHandle, this, &ANovaPlayerCharacter::EndDash,
		Duration, false);

	// Dash direction doubles as the ability aim direction.
	ElementAbilityComp->SetRuntimeDirection(Direction);
}

void ANovaPlayerCharacter::EndDash()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const float MaxSpeed = Movement->MaxWalkSpeed;
	const float LateralSpeed = Movement->Velocity.Size2D();
	if (LateralSpeed > MaxSpeed)
	{
		Movement->Velocity = Movement->Velocity.GetClampedToMaxSize2D(MaxSpeed);
		UE_LOG(LogNovaCharacter, Log, TEXT("EndDash: lateral speed %.0f capped to %.0f"),
			LateralSpeed, MaxSpeed);
	}
}

void ANovaPlayerCharacter::CastAbility1()
{
	if (IsControlHeld())
	{
		return;
	}
	ElementAbilityComp->SetRuntimeDirection(GetActorForwardVector());
	ElementAbilityComp->CastAbilityById(TEXT("Slash"));
}

void ANovaPlayerCharacter::CastAbility2()
{
	if (IsControlHeld())
	{
		return;
	}
	ElementAbilityComp->SetRuntimeDirection(GetActorForwardVector());
	ElementAbilityComp->CastAbilityById(TEXT("Edgewall"));
}

void ANovaPlayerCharacter::TriggerStoryA()
{
	if (UStoryDirectorSubsystem* Story = GetGameInstance()->GetSubsystem<UStoryDirectorSubsystem>())
	{
		Story->StartStoryA();
	}
}

void ANovaPlayerCharacter::TriggerStoryB()
{
	if (UStoryDirectorSubsystem* Story = GetGameInstance()->GetSubsystem<UStoryDirectorSubsystem>())
	{
		Story->StartStoryB();
	}
}

void ANovaPlayerCharacter::ToggleDebug()
{
	if (UNovaDebugSubsystem* Debug = GetWorld()->GetSubsystem<UNovaDebugSubsystem>())
	{
		Debug->ToggleDebugDisplay();
	}
}
