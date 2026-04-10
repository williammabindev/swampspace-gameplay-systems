# swampspace-gameplay-systems

Unreal Engine C++ gameplay systems demo featuring projectile pooling, status effects and a debug HUD.

---

## Demo

Watch the system in action: https://youtu.be/ix6qlOJcrJA

Playable build: https://williammabin.itch.io/swamp-space-dev-portfolio

---

## Overview

A gameplay systems prototype focused on performance, clarity and runtime observability.

The project demonstrates:

- Projectile pooling with actor reuse
- Modular status effects (drunk, invincibility, power boost and speed boost)
- Persistent score tracking
- A lightweight debug HUD for real-time system visibility

---

## Code Snippets

This repository contains curated excerpts from the full project:

- `/snippets/projectile_activation.cpp`
- `/snippets/pool_acquire.cpp`
- `/snippets/hud_status_notify.cpp`

Each snippet is a simplified, self-contained example highlighting a specific system.

---

## Key Systems

### Projectile Pooling

Projectiles are managed through a pooled system to avoid repeated `SpawnActor` and `Destroy` calls during gameplay. Actors are reused across shots, maintaining a stable ID and activation count. This allows the system to expose whether a projectile is newly spawned or reused, visible in both logs and the HUD at runtime.

```cpp
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
```

A `UWorldSubsystem` manages per-class pools of actors. Reuse is prioritised before spawning. A separate `TotalSpawned` map tracks lifetime spawn counts per class for HUD reporting.

```cpp
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
```

When a projectile deactivates on hit or lifetime expiry, it returns itself to the pool:

```cpp
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
```

---

### Status Effects

Timed gameplay effects modify player state:

- **Drunk** -- inverts movement input and applies a post-process blur
- **Invincibility** -- blocks all incoming damage
- **Power Boost** -- multiplies outgoing damage
- **Speed Boost** -- modifies `MaxWalkSpeed`

All effects follow a consistent flow:

```
Character -> PlayerController (Notify*) -> HUD (BP_Set*)
```

This keeps gameplay logic decoupled from UI while enabling real-time debug visibility. Each effect is timer-driven and refreshes cleanly if re-triggered while active.

```cpp
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
```

```cpp
// Simplified excerpt from UI notification (speed boost)

void ASwampSpacePlayerController::NotifySpeedBoostChanged(bool bActive)
{
    UE_LOG(LogTemp, Log,
        TEXT("[HUD] Speed Boost: %s"),
        bActive ? TEXT("ON") : TEXT("OFF"));

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            3.f,
            FColor::Green,
            FString::Printf(TEXT("Speed Boost: %s"),
                bActive ? TEXT("ON") : TEXT("OFF")));
    }

    if (IsValid(PlayerHUD))
    {
        PlayerHUD->BP_SetSpeedBoostStatus(bActive);
    }
}
```

---

### Score System

Score is stored in the `GameInstance` to persist across level transitions. Gameplay updates flow through a single authoritative path:

```
Character -> GameInstance -> PlayerController -> HUD
```

This ensures consistency across level loads and the HUD re-synchronises on `BeginPlay`.

```cpp
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

void USwampSpaceGameInstance::AddScore(int32 Amount)
{
    CurrentScore += Amount;
}
```

---

## Why This Matters

- Reduces runtime allocation overhead through object pooling
- Keeps gameplay and UI decoupled via event-driven architecture
- Provides clear runtime visibility through debug tooling
- Scales cleanly as systems grow in complexity
