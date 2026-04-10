// Swamp Space – Gameplay Systems Demo
// Simplified excerpt from projectile activation logic.
// Full implementation includes collision, movement reset, damage setup, and lifetime handling.

void AProjectileBase::ActivateProjectile(const FVector& SpawnLocation, const FVector& Direction, AActor* InOwner, float InDamage)
{
    const bool bWasReused = (ActivationCount > 0);

    if (bWasReused)
    {
        UE_LOG(LogTemp, Log, TEXT("[Pool] Reused Projectile ID %d"), ProjectileID);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[Pool] Spawned NEW Projectile ID %d"), ProjectileID);
    }

    if (ASwampSpacePlayerController* PC = Cast<ASwampSpacePlayerController>(
        UGameplayStatics::GetPlayerController(GetWorld(), 0)))
    {
        PC->NotifyProjectileActivated(ProjectileID, bWasReused);
    }

    ++ActivationCount;
}