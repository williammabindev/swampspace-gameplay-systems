// Swamp Space – Gameplay Systems Demo
// Simplified excerpt from object pool acquisition logic.
// Reuses existing actors when available, otherwise spawns new ones.

AActor* UMyPoolWorldSubsystem::Acquire(TSubclassOf<AActor> ActorClass)
{
    if (!ActorClass) return nullptr;

    if (TArray<AActor*>* Available = Pool.Find(ActorClass))
    {
        if (Available->Num() > 0)
        {
            return Available->Pop(); // reuse existing actor
        }
    }

    // No pooled actor available — spawn a new one
    TotalSpawned.FindOrAdd(ActorClass)++;
    return GetWorld()->SpawnActor<AActor>(ActorClass);
}