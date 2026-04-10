// Swamp Space – Gameplay Systems Demo
// Simplified excerpt from status effect UI notification.
// Routes gameplay state changes from the controller to the HUD.

void ASwampSpacePlayerController::NotifySpeedBoostChanged(bool bActive)
{
    // Log for debugging / verification
    UE_LOG(LogTemp, Log,
        TEXT("[HUD] Speed Boost: %s"),
        bActive ? TEXT("ON") : TEXT("OFF"));

    // On-screen debug feedback
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            3.f,
            FColor::Green,
            FString::Printf(TEXT("Speed Boost: %s"),
                bActive ? TEXT("ON") : TEXT("OFF")));
    }

    // Forward to HUD (Blueprint layer)
    if (IsValid(PlayerHUD))
    {
        PlayerHUD->BP_SetSpeedBoostStatus(bActive);
    }
}