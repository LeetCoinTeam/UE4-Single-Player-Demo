// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LEETDEMO2.h"
#include "LEETDEMO2Character.h"
#include "PaperFlipbookComponent.h"
#include "Components/TextRenderComponent.h"


DEFINE_LOG_CATEGORY_STATIC(SideScrollerCharacter, Log, All);
//////////////////////////////////////////////////////////////////////////
// ALEETDEMO2Character

ALEETDEMO2Character::ALEETDEMO2Character()
{
	// Setup the assets
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UPaperFlipbook> RunningAnimationAsset;
		ConstructorHelpers::FObjectFinderOptional<UPaperFlipbook> IdleAnimationAsset;
		FConstructorStatics()
			: RunningAnimationAsset(TEXT("/Game/2dSideScroller/Sprites/RunningAnimation.RunningAnimation"))
			, IdleAnimationAsset(TEXT("/Game/2dSideScroller/Sprites/IdleAnimation.IdleAnimation"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	RunningAnimation = ConstructorStatics.RunningAnimationAsset.Get();
	IdleAnimation = ConstructorStatics.IdleAnimationAsset.Get();
	GetSprite()->SetFlipbook(IdleAnimation);

	// Use only Yaw from the controller and ignore the rest of the rotation.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Set the size of our collision capsule.
	GetCapsuleComponent()->SetCapsuleHalfHeight(96.0f);
	GetCapsuleComponent()->SetCapsuleRadius(40.0f);

	// Create a camera boom attached to the root (capsule)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->AttachTo(RootComponent);
	CameraBoom->TargetArmLength = 500.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 75.0f);
	CameraBoom->bAbsoluteRotation = true;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->RelativeRotation = FRotator(0.0f, -90.0f, 0.0f);
	

	// Create an orthographic camera (no perspective) and attach it to the boom
	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;
	SideViewCameraComponent->OrthoWidth = 2048.0f;
	SideViewCameraComponent->AttachTo(CameraBoom, USpringArmComponent::SocketName);

	// Prevent all automatic rotation behavior on the camera, character, and camera component
	CameraBoom->bAbsoluteRotation = true;
	SideViewCameraComponent->bUsePawnControlRotation = false;
	SideViewCameraComponent->bAutoActivate = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// Configure character movement
	GetCharacterMovement()->GravityScale = 2.0f;
	GetCharacterMovement()->AirControl = 0.80f;
	GetCharacterMovement()->JumpZVelocity = 1000.f;
	GetCharacterMovement()->GroundFriction = 3.0f;
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->MaxFlySpeed = 600.0f;

	// Lock character motion onto the XZ plane, so the character can't move in or out of the screen
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0.0f, -1.0f, 0.0f));

	// Behave like a traditional 2D platformer character, with a flat bottom instead of a curved capsule bottom
	// Note: This can cause a little floating when going up inclines; you can choose the tradeoff between better
	// behavior on the edge of a ledge versus inclines by setting this to true or false
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;

// 	TextComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("IncarGear"));
// 	TextComponent->SetRelativeScale3D(FVector(3.0f, 3.0f, 3.0f));
// 	TextComponent->SetRelativeLocation(FVector(35.0f, 5.0f, 20.0f));
// 	TextComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
// 	TextComponent->AttachTo(RootComponent);

	// Enable replication on the Sprite component so animations show up when networked
	GetSprite()->SetIsReplicated(true);
	bReplicates = true;
}

//////////////////////////////////////////////////////////////////////////
// Animation

void ALEETDEMO2Character::UpdateAnimation()
{
	const FVector PlayerVelocity = GetVelocity();
	const float PlayerSpeed = PlayerVelocity.Size();

	// Are we moving or standing still?
	UPaperFlipbook* DesiredAnimation = (PlayerSpeed > 0.0f) ? RunningAnimation : IdleAnimation;
	if( GetSprite()->GetFlipbook() != DesiredAnimation 	)
	{
		GetSprite()->SetFlipbook(DesiredAnimation);
	}
}

void ALEETDEMO2Character::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	UpdateCharacter();	
}


//////////////////////////////////////////////////////////////////////////
// Input

void ALEETDEMO2Character::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// Note: the 'Jump' action and the 'MoveRight' axis are bound to actual keys/buttons/sticks in DefaultInput.ini (editable from Project Settings..Input)
	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	InputComponent->BindAxis("MoveRight", this, &ALEETDEMO2Character::MoveRight);

	InputComponent->BindTouch(IE_Pressed, this, &ALEETDEMO2Character::TouchStarted);
	InputComponent->BindTouch(IE_Released, this, &ALEETDEMO2Character::TouchStopped);
}

void ALEETDEMO2Character::MoveRight(float Value)
{
	/*UpdateChar();*/

	// Apply the input to the character motion
	AddMovementInput(FVector(1.0f, 0.0f, 0.0f), Value);
}

void ALEETDEMO2Character::TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// jump on any touch
	Jump();
}

void ALEETDEMO2Character::TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	StopJumping();
}

void ALEETDEMO2Character::UpdateCharacter()
{
	// Update animation to match the motion
	UpdateAnimation();

	// Now setup the rotation of the controller based on the direction we are travelling
	const FVector PlayerVelocity = GetVelocity();	
	float TravelDirection = PlayerVelocity.X;
	// Set the rotation so that the character faces his direction of travel.
	if (Controller != nullptr)
	{
		if (TravelDirection < 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
		}
		else if (TravelDirection > 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
		}
	}
}

bool ALEETDEMO2Character::Run(FString PlatformID)
{
	FHttpModule* Http = &FHttpModule::Get();
	UE_LOG(LogTemp, Log, TEXT("ALEETDEMO2Character"));
	if (!Http) { return false; }
	UE_LOG(LogTemp, Log, TEXT("ALEETDEMO2Character http is available"));
	if (!Http->IsHttpEnabled()) { return false; }
	UE_LOG(LogTemp, Log, TEXT("ALEETDEMO2Character http is enabled"));

	// save the platformId for later
	LeetPlatformId = PlatformID;
	UE_LOG(LogTemp, Log, TEXT("LeetPlatformId: %s"), *LeetPlatformId);
	UE_LOG(LogTemp, Log, TEXT("Object is: %s"), *GetName());

	FString ServerAPIKey = "XXXX"; // Grab this off leet sandbox
	FString ServerAPISecret = "XXX"; // Grab this off leet sandbox
	FString nonceString = "10951350917635";
	FString encryption = "off";  // Allowing unencrypted on sandbox for now.  

	// Build params as json?
	// TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	// JsonObject->SetStringField(TEXT("nonce"), *FString::Printf(TEXT("%s"), *nonceString));
	// JsonObject->SetStringField(TEXT("encryption"), *FString::Printf(TEXT("%s"), *encryption));

	FString OutputString;
	// TSharedRef<TJsonWriter<TCHAR>> JsonWriter = TJsonWriterFactory<>::Create(&OutputString);
	// FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

	// Build Params as text string
	OutputString = "nonce=" + nonceString + "&encryption=" + encryption;
	// urlencode the params

	//Make sure we are using UTF-8 as that is the de-facto standard when hashing string with SHA1
	std::string stdstring(TCHAR_TO_UTF8(*OutputString));

	FSHA1 HashState;
	HashState.Update((uint8*)stdstring.c_str(), stdstring.size());
	HashState.Final();

	uint8 Hash[FSHA1::DigestSize];
	HashState.GetHash(Hash);

	FString HashStr = BytesToHex(Hash, FSHA1::DigestSize);
	UE_LOG(LogTemp, Log, TEXT("ALEETDEMO2Character hash"));
	UE_LOG(LogTemp, Log, TEXT("HashStr: %s"), *HashStr);

	// Base64 encode?
	//FString BaseStr = FBase64::Encode()
	//FBase64->Encode

	FString TargetHost = "http://apitest-dot-leetsandbox.appspot.com/api/v2/player/" + PlatformID + "/activate";
	TSharedRef < IHttpRequest > Request = Http->CreateRequest();
	Request->SetVerb("POST");
	Request->SetURL(TargetHost);
	Request->SetHeader("User-Agent", "VictoryBPLibrary/1.0");
	Request->SetHeader("Content-Type", "application/x-www-form-urlencoded");
	Request->SetHeader("Key", ServerAPIKey);
	Request->SetHeader("Sign", HashStr);
	Request->SetContentAsString(OutputString);

	Request->OnProcessRequestComplete().BindUObject(this, &ALEETDEMO2Character::RequestComplete);
	if (!Request->ProcessRequest()) { return false; }

	return true;


}

void ALEETDEMO2Character::RequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	if (!HttpResponse.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Test failed. NULL response"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Completed test [%s] Url=[%s] Response=[%d] [%s]"),
			*HttpRequest->GetVerb(),
			*HttpRequest->GetURL(),
			HttpResponse->GetResponseCode(),
			*HttpResponse->GetContentAsString());

		FString JsonRaw = *HttpResponse->GetContentAsString();
		TSharedPtr<FJsonObject> JsonParsed;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonRaw);
		if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
		{
			bool Authorization = JsonParsed->GetBoolField("authorization");
			UE_LOG(LogTemp, Log, TEXT("Authorization"));
			if (Authorization)
			{
				UE_LOG(LogTemp, Log, TEXT("Authorization True"));
				bool PlayerAuthorized = JsonParsed->GetBoolField("player_authorized");
				if (PlayerAuthorized) {
					UE_LOG(LogTemp, Log, TEXT("Player Authorized"));

	
					//FString JsonLeetPlatformId = JsonParsed->GetStringField("player_platformid");
					//this->LeetPlatformId = JsonLeetPlatformId;
					//UE_LOG(LogTemp, Log, TEXT("JsonLeetPlatformId: %s"), *JsonLeetPlatformId);

					//LeetPlatformId = JsonParsed->GetStringField("player_platformid");
					//UE_LOG(LogTemp, Log, TEXT("LeetPlatformId: %s"), *LeetPlatformId);
					UGameplayStatics::OpenLevel(GetWorld(), "2DSideScrollerExampleMap");

				}
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("Authorization False"));
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Authorization False"));
}


bool ALEETDEMO2Character::Deactivate(FString PlatformID)
{
	FHttpModule* Http = &FHttpModule::Get();
	UE_LOG(LogTemp, Log, TEXT("ALEETDEMO2Character"));
	if (!Http) { return false; }
	UE_LOG(LogTemp, Log, TEXT("ALEETDEMO2Character http is available"));
	if (!Http->IsHttpEnabled()) { return false; }
	UE_LOG(LogTemp, Log, TEXT("ALEETDEMO2Character http is enabled"));

	FString ServerAPIKey = "A2bTPp-OBuFlN-NkIIKK-2ZXmBs";
	FString ServerAPISecret = "9h0PNydxxYgmAHy9j2Hqgh41qebCPh";
	FString nonceString = "10951350917635";
	FString encryption = "off"; // Allowing unencrypted on sandbox for now.  

	FString OutputString;
	// Build Params as text string
	OutputString = "nonce=" + nonceString + "&encryption=" + encryption;
	// urlencode the params

	//Make sure we are using UTF-8 as that is the de-facto standard when hashing string with SHA1
	std::string stdstring(TCHAR_TO_UTF8(*OutputString));

	FSHA1 HashState;
	HashState.Update((uint8*)stdstring.c_str(), stdstring.size());
	HashState.Final();

	uint8 Hash[FSHA1::DigestSize];
	HashState.GetHash(Hash);

	FString HashStr = BytesToHex(Hash, FSHA1::DigestSize);
	UE_LOG(LogTemp, Log, TEXT("ALEETDEMO2Character hash"));
	UE_LOG(LogTemp, Log, TEXT("HashStr: %s"), *HashStr);

	// Base64 encode?
	//FString BaseStr = FBase64::Encode()
	//FBase64->Encode

	UE_LOG(LogTemp, Log, TEXT("PlatformID: %s"), *PlatformID);
	UE_LOG(LogTemp, Log, TEXT("Object is: %s"), *GetName());

	FString TargetHost = "http://apitest-dot-leetsandbox.appspot.com/api/v2/player/" + PlatformID + "/deactivate";
	TSharedRef < IHttpRequest > Request = Http->CreateRequest();
	Request->SetVerb("POST");
	Request->SetURL(TargetHost);
	Request->SetHeader("User-Agent", "VictoryBPLibrary/1.0");
	Request->SetHeader("Content-Type", "application/x-www-form-urlencoded");
	Request->SetHeader("Key", ServerAPIKey);
	Request->SetHeader("Sign", HashStr);
	Request->SetContentAsString(OutputString);

	Request->OnProcessRequestComplete().BindUObject(this, &ALEETDEMO2Character::RequestComplete);
	if (!Request->ProcessRequest()) { return false; }

	return true;


}

void ALEETDEMO2Character::DeactivateRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	if (!HttpResponse.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Test failed. NULL response"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Completed test [%s] Url=[%s] Response=[%d] [%s]"),
			*HttpRequest->GetVerb(),
			*HttpRequest->GetURL(),
			HttpResponse->GetResponseCode(),
			*HttpResponse->GetContentAsString());

		FString JsonRaw = *HttpResponse->GetContentAsString();
		TSharedPtr<FJsonObject> JsonParsed;
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonRaw);
		if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
		{
			bool Authorization = JsonParsed->GetBoolField("authorization");
			UE_LOG(LogTemp, Log, TEXT("Authorization"));
			if (Authorization)
			{
				UE_LOG(LogTemp, Log, TEXT("Authorization True"));
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("Authorization False"));
			}
		}
	}
}