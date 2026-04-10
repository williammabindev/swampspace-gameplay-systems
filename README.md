# swampspace-gameplay-systems

Unreal Engine C++ gameplay systems demo featuring projectile pooling, status effects and debug HUD

Swamp Space - Gameplay Systems Demo (Unreal Engine C++)

This repository contains selected code excerpts and technical notes from a proprietary Unreal Engine gameplay systems prototype.

---

## Demo

▶️ Watch the system in action:  
[YouTube Video](https://youtu.be/ix6qlOJcrJA)

🎮 Playable build:  
[Itch.io Page](https://williammabin.itch.io/swamp-space-dev-portfolio)

---

## Overview

A gameplay systems prototype focused on performance, clarity, and runtime observability.

The project demonstrates:

- Projectile pooling with actor reuse
- Modular status effects (drunk, invincibility, power boost, speed boost)
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

Projectiles are managed through a pooled system to avoid repeated `SpawnActor` and `Destroy` calls during gameplay.

Actors are reused across shots, maintaining a stable ID and activation count. This allows the system to expose whether a projectile is newly spawned or reused, visible in both logs and the HUD at runtime.

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