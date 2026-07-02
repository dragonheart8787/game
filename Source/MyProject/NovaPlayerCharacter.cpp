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
#include "NovaDebugSubsystem.h"
#include "StoryDirectorSubsystem.h"

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

	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
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

	// Input assets are assigned in the character Blueprint; unassigned actions are skipped.
	if (IA_Move) { Input->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ANovaPlayerCharacter::Move); }
	if (IA_Look) { Input->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ANovaPlayerCharacter::Look); }
	if (IA_Jump)
	{
		Input->BindAction(IA_Jump, ETriggerEvent::Started, this, &ACharacter::Jump);
		Input->BindAction(IA_Jump, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}
	if (IA_Dash) { Input->BindAction(IA_Dash, ETriggerEvent::Started, this, &ANovaPlayerCharacter::Dash); }
	if (IA_CastAbility1) { Input->BindAction(IA_CastAbility1, ETriggerEvent::Started, this, &ANovaPlayerCharacter::CastAbility1); }
	if (IA_CastAbility2) { Input->BindAction(IA_CastAbility2, ETriggerEvent::Started, this, &ANovaPlayerCharacter::CastAbility2); }
	if (IA_TriggerStoryA) { Input->BindAction(IA_TriggerStoryA, ETriggerEvent::Started, this, &ANovaPlayerCharacter::TriggerStoryA); }
	if (IA_TriggerStoryB) { Input->BindAction(IA_TriggerStoryB, ETriggerEvent::Started, this, &ANovaPlayerCharacter::TriggerStoryB); }
	if (IA_ToggleDebug) { Input->BindAction(IA_ToggleDebug, ETriggerEvent::Started, this, &ANovaPlayerCharacter::ToggleDebug); }
}

void ANovaPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}

	// Director Hold takes control away entirely.
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UStoryDirectorSubsystem* Story = GameInstance->GetSubsystem<UStoryDirectorSubsystem>())
		{
			if (Story->GetControlMask() == ENovaControlMask::Hold)
			{
				return;
			}
		}
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
	if (Controller)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ANovaPlayerCharacter::Dash()
{
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

	// Impulse sized so the dash carries roughly DashDistance against ground friction.
	constexpr float DashDurationSeconds = 0.2f;
	LaunchCharacter(Direction * (DashDistance / DashDurationSeconds), true, false);

	// Dash direction doubles as the ability aim direction.
	ElementAbilityComp->SetRuntimeDirection(Direction);
}

void ANovaPlayerCharacter::CastAbility1()
{
	ElementAbilityComp->SetRuntimeDirection(GetActorForwardVector());
	ElementAbilityComp->CastAbilityById(TEXT("Slash"));
}

void ANovaPlayerCharacter::CastAbility2()
{
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
