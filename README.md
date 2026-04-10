# swampspace-gameplay-systems
Unreal Engine C++ gameplay systems demo featuring projectile pooling, status effects and debug HUD

Swamp Space - Gameplay Systems Demo (Unreal Engine C++)

This repository contains selected code excerpts and technical notes from a proprietary Unreal Engine gameplay systems prototype.

--------------------------------------------------------------------------------

Overview

A gameplay systems prototype focused on performance, clarity, and runtime observability.

The project demonstrates:

- Projectile pooling with actor reuse
- Modular status effects (drunk, invincibility, power boost, speed boost)
- Persistent score tracking
- A lightweight debug HUD for real-time system visibility

--------------------------------------------------------------------------------

Key Systems

--------------------------------------------------------------------------------

Projectile Pooling

Projectiles are managed through a pooled system to avoid repeated SpawnActor and
Destroy calls during gameplay.

Actors are reused across shots, maintaining a stable ID and activation count.
This allows the system to expose whether a projectile is newly spawned or reused,
visible in both the log and the HUD at runtime.

    // Simplified excerpt from projectile activation

    const bool bWasReused = (ActivationCount > 0);

    UE_LOG(LogTemp, Log,
        TEXT("[Pool] %s Projectile ID %d"),
        bWasReused ? TEXT("Reused") : TEXT("Spawned NEW"),
        ProjectileID);

    if (ASwampSpacePlayerController* PC = Cast<ASwampSpacePlayerController>(
        UGameplayStatics::GetPlayerController(GetWorld(), 0)))
    {
        PC->NotifyProjectileActivated(ProjectileID, bWasReused);
    }

    ++ActivationCount;

--------------------------------------------------------------------------------

Pool Management

A UWorldSubsystem manages per-class pools of actors. Reuse is prioritised before
spawning. A separate TotalSpawned map tracks lifetime spawn counts per class for
HUD reporting.

    // Simplified excerpt from pool acquire

    AActor* UMyPoolWorldSubsystem::Acquire(TSubclassOf<AActor> ActorClass)
    {
        if (TArray<AActor*>* Available = Pool.Find(ActorClass))
        {
            if (Available->Num() > 0)
            {
                return Available->Pop(); // reuse existing actor
            }
        }

        TotalSpawned.FindOrAdd(ActorClass)++;
        return GetWorld()->SpawnActor<AActor>(ActorClass);
    }

When a projectile deactivates (on hit or lifetime expiry), it returns itself to
the pool:

    // Simplified excerpt from pool return

    void AProjectileBase::RecycleOrDestroy()
    {
        if (UMyPoolWorldSubsystem* Pool = GetWorld()->GetSubsystem<UMyPoolWorldSubsystem>())
        {
            Pool->ReturnToPool(this);

            if (ASwampSpacePlayerController* PC = Cast<ASwampSpacePlayerController>(
                UGameplayStatics::GetPlayerController(GetWorld(), 0)))
            {
                PC->NotifyPoolStatus(
                    Pool->GetActiveCount(GetClass()),
                    Pool->GetTotalCount(GetClass()));
            }
            return;
        }

        Destroy();
    }

Active count is derived at query time rather than tracked separately:

    int32 UMyPoolWorldSubsystem::GetActiveCount(TSubclassOf<AActor> ActorClass) const
    {
        const TArray<AActor*>* Available = Pool.Find(ActorClass);
        const int32 Pooled = Available ? Available->Num() : 0;
        return FMath::Max(0, GetTotalCount(ActorClass) - Pooled);
    }

--------------------------------------------------------------------------------

Status Effects

Timed gameplay effects modify player state, including:

- Drunk          - inverts movement input and applies a post-process blur
- Invincibility  - blocks all incoming damage
- Power Boost    - multiplies outgoing damage
- Speed Boost    - multiplies MaxWalkSpeed on the movement component

All effects follow a consistent flow:

    Character -> PlayerController (Notify*) -> HUD (BP_Set*)

This keeps gameplay logic separate from UI concerns while allowing real-time
debug feedback in both the log and on-screen.

Each effect is timer-driven and refreshes cleanly if re-triggered while active.

    // Simplified excerpt from invincibility apply/end

    void ASwampSpaceCharacter::ApplyInvincibility(float DurationSec)
    {
        bInvincible = true;

        if (ASwampSpacePlayerController* PC = Cast<ASwampSpacePlayerController>(GetController()))
        {
            PC->NotifyInvincibleChanged(true);
        }

        GetWorldTimerManager().ClearTimer(InvincibilityTimerHandle);
        GetWorldTimerManager().SetTimer(
            InvincibilityTimerHandle,
            this,
            &ASwampSpaceCharacter::EndInvincibility,
            DurationSec,
            false);
    }

    void ASwampSpaceCharacter::EndInvincibility()
    {
        bInvincible = false;

        if (ASwampSpacePlayerController* PC = Cast<ASwampSpacePlayerController>(GetController()))
        {
            PC->NotifyInvincibleChanged(false);
        }
    }

The HUD notification layer is consistent across all four effects:

    // Simplified excerpt from UI notification (speed boost)

    void ASwampSpacePlayerController::NotifySpeedBoostChanged(bool bActive)
    {
        UE_LOG(LogTemp, Log,
            TEXT("[HUD] Speed Boost: %s"),
            bActive ? TEXT("ON") : TEXT("OFF"));

        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
                FString::Printf(TEXT("Speed Boost: %s"), bActive ? TEXT("ON") : TEXT("OFF")));

        if (IsValid(PlayerHUD))
            PlayerHUD->BP_SetSpeedBoostStatus(bActive);
    }

Each Notify* function on the controller follows the same structure: log,
on-screen debug message, HUD event. A matching BlueprintImplementableEvent is
declared on the HUD widget class for each effect:

    UFUNCTION(BlueprintImplementableEvent, Category="HUD|Status")
    void BP_SetDrunkStatus(bool bActive);

    UFUNCTION(BlueprintImplementableEvent, Category="HUD|Status")
    void BP_SetInvincibleStatus(bool bActive);

    UFUNCTION(BlueprintImplementableEvent, Category="HUD|Status")
    void BP_SetPowerBoostStatus(bool bActive);

    UFUNCTION(BlueprintImplementableEvent, Category="HUD|Status")
    void BP_SetSpeedBoostStatus(bool bActive);

--------------------------------------------------------------------------------

Score System

Score is stored in the GameInstance to persist across level transitions.

Gameplay updates flow through a single authoritative write path:

    Character -> GameInstance -> PlayerController -> HUD

This ensures the score remains consistent across level loads, with the HUD
re-synced on BeginPlay.

    // Simplified excerpt from score update

    void ASwampSpaceCharacter::AddScore(int32 Amount)
    {
        if (USwampSpaceGameInstance* GI = GetGameInstance<USwampSpaceGameInstance>())
        {
            GI->AddScore(Amount);

            if (ASwampSpacePlayerController* PC = Cast<ASwampSpacePlayerController>(GetController()))
            {
                PC->NotifyScoreChanged(GI->CurrentScore);
            }
        }
    }

    // GameInstance - sole write point for score

    void USwampSpaceGameInstance::AddScore(int32 Amount)
    {
        CurrentScore += Amount;
    }

On level load, the controller re-seeds the HUD from the current GameInstance value:

    void ASwampSpacePlayerController::SyncScoreFromGameInstance()
    {
        if (USwampSpaceGameInstance* GI = GetGameInstance<USwampSpaceGameInstance>())
        {
            NotifyScoreChanged(GI->CurrentScore);
        }
    }
