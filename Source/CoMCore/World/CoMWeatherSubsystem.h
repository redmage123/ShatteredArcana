#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CoMCore/CoreTypes/CoMEnums.h"
#include "CoMCore/CoreTypes/CoMStructs.h"
#include "CoMCore/CoreTypes/CoMConstants.h"
#include "CoMWeatherSubsystem.generated.h"

// (struct defined in CoMStructs.h)

UCLASS()
class COMCORE_API UCoMWeatherSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Seed the deterministic weather RNG. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Weather")
	void InitializeWeather(int32 Seed);

	/** Advance all weather zones: drift, decrement turns, despawn expired, spawn new fronts. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Weather")
	void ProcessWeatherTurn();

	/** Get the dominant weather type at a given tile. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Weather")
	ECoMWeatherType GetWeatherAtTile(ECoMPlane Plane, int32 X, int32 Y) const;

	/** Get the highest weather intensity affecting a tile. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Weather")
	FFixed64 GetWeatherIntensity(ECoMPlane Plane, int32 X, int32 Y) const;

	/** Return gameplay modifiers for a given weather type and intensity. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Weather")
	static FCoMWeatherEffects GetWeatherEffects(ECoMWeatherType Type, FFixed64 Intensity);

	/** Spawn a new weather zone on the given plane using plane-specific probability tables. */
	UFUNCTION(BlueprintCallable, Category = "CoM|Weather")
	void GenerateWeatherFront(ECoMPlane Plane);

	/** Read-only access to active zones for a plane. */
	const TArray<FCoMWeatherZone>& GetActiveZones(ECoMPlane Plane) const;

	UPROPERTY()
	TMap<ECoMPlane, FCoMWeatherState> WeatherStates;

private:
	FRandomStream WeatherRng;

	static constexpr int32 MAP_WIDTH  = 160;
	static constexpr int32 MAP_HEIGHT = 100;

	/** Manhattan distance with WrapX on a 160-wide map. */
	int32 ManhattanDistWrapX(int32 X1, int32 Y1, int32 X2, int32 Y2) const;

	/** Base spawn chance per plane per turn (0.0 – 1.0 mapped to 0 – 100). */
	int32 GetBaseSpawnChancePercent(ECoMSeason Season) const;

	/** Pick a random weather type for the given plane using weighted tables. */
	ECoMWeatherType PickWeatherTypeForPlane(ECoMPlane Plane);

	struct FWeightedWeather
	{
		ECoMWeatherType Type;
		int32 Weight;
	};

	static const TArray<FWeightedWeather>& GetPlaneWeatherTable(ECoMPlane Plane);

	static const TArray<FCoMWeatherZone> EmptyZones;
};