// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "../Derived/DynamicMeshComponent.h"
#include "../Terrain/Simplex.h"
#include "../Exp/ExpCharacter.h"
#include "AdvancedPlanetActor.generated.h"

class FAdvancedPlanetMasterWorker;

/* Enums */
UENUM(BlueprintType)
enum class EAdvancedPlanetGenType : uint8
{
	GT_StandBy 			UMETA(DisplayName = "StandBy"),
	GT_CubeData 		UMETA(DisplayName = "CubeData"),
	GT_SectionData 		UMETA(DisplayName = "SectionData"),
	GT_OceanAtmoData 	UMETA(DisplayName = "OceanAtmoData")
};

UENUM(BlueprintType)
enum class EAdvancedPlanetNoiseType : uint8
{
	LN_Perlin 	UMETA(DisplayName = "Perlin"),
	LN_Other 	UMETA(DisplayName = "Other")
};

UENUM(BlueprintType)
enum class EAdvancedPlanetLayerCombinationType : uint8
{
	LC_None					UMETA(DisplayName = "None"),
	LC_Add 					UMETA(DisplayName = "Add"),
	LC_Subtract 			UMETA(DisplayName = "Subtract"),
	LC_AddDifference 		UMETA(DisplayName = "AddDifference"),
	LC_SubDifference 		UMETA(DisplayName = "SubDifference"),
	LC_Negative 			UMETA(DisplayName = "Negative"),
	LC_Multiply				UMETA(DisplayName = "Multiply"),
	LC_Blend 				UMETA(DisplayName = "Blend")
};

/* Structs */
USTRUCT()
struct FAdvancedPlanetLayerBlendFactors
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		float HeightMin;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		float HeightMax;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		float BlendAlpha;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		bool ApplySmoothing;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		float SmoothingFactor;

	//Constructor
	FAdvancedPlanetLayerBlendFactors()
	{
		HeightMin = 0;
		HeightMax = 0;
		BlendAlpha = 0;
		ApplySmoothing = false;
		SmoothingFactor = 20.f;
	};
};

USTRUCT()
struct FAdvancedPlanetMaterialSlopeFactors
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		float HighSlopeMin;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		float HighSlopeMax;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		float MidSlopeMin;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		float MidSlopeMax;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		float LowSlopeMin;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		float LowSlopeMax;

	//Constructor
	FAdvancedPlanetMaterialSlopeFactors()
	{
		HighSlopeMin = 0;
		HighSlopeMax = 0;

		MidSlopeMin = 0;
		MidSlopeMax = 0;

		LowSlopeMin = 0;
		LowSlopeMax = 0;
	};
};

USTRUCT()
struct FAdvancedPlanetLayer
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		FName LayerName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		EAdvancedPlanetNoiseType NoiseType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		EAdvancedPlanetLayerCombinationType CombinationType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
		float Amplitude;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
		int32 Octaves;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
		float Persistence;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
		float Frequency;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
		int32 Seed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
		float MaxTerrainHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
		float BaseScaleOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
		FAdvancedPlanetLayerBlendFactors BlendFactors;

	void Create(
		FName IN_LayerName,
		int32 IN_Seed,
		float IN_Amplitude,
		int32 IN_Octaves,
		float IN_Persistence,
		float IN_Frequency,
		float IN_MaxTerrainHeight,
		float IN_BaseScaleOffset,
		EAdvancedPlanetLayerCombinationType IN_CombinationType,
		EAdvancedPlanetNoiseType IN_NoiseType)
	{
		LayerName = IN_LayerName;
		Seed = IN_Seed;
		Amplitude = IN_Amplitude;
		Octaves = IN_Octaves;
		Persistence = IN_Persistence;
		Frequency = IN_Frequency;
		MaxTerrainHeight = IN_MaxTerrainHeight;
		CombinationType = IN_CombinationType;
		BaseScaleOffset = IN_BaseScaleOffset;
		NoiseType = IN_NoiseType;
		BlendFactors = FAdvancedPlanetLayerBlendFactors();
	};
	//Constructor
	FAdvancedPlanetLayer()
	{
		LayerName = "New Planet Layer";
	};
};

USTRUCT()
struct FAdvancedPlanetAtmoLayerParameters
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FLinearColor Attenuation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FLinearColor Scatter;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float ScaleHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float Density;


	//Constructor
	FAdvancedPlanetAtmoLayerParameters()
	{
		Attenuation = FLinearColor(0, 0, 0);
		Scatter = FLinearColor(0, 0, 0);
		ScaleHeight = 0;
		Density = 0;
	};
};

USTRUCT()
struct FAdvancedPlanetShadeParameters
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float Multiplier;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float Constant;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float Power;


	//Constructor
	FAdvancedPlanetShadeParameters()
	{
		Multiplier = 1;
		Constant = 0;
		Power = 1;
	};
};

USTRUCT()
struct FAdvancedPlanetTriangle
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		int32 Point1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		int32 Point2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		int32 Point3;

	// Add Face
	void Create(int32 p1, int32 p2, int32 p3)
	{
		Point1 = p1;
		Point2 = p2;
		Point3 = p3;
	};

	//Constructor
	FAdvancedPlanetTriangle()
		: Point1(0)
		, Point2(0)
		, Point3(0)
	{};
};

USTRUCT()
struct FAdvancedPlanetQuad
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		FVector Vector1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		FVector Vector2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		FVector Vector3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		FVector Vector4;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		FAdvancedPlanetTriangle Triangle1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		FAdvancedPlanetTriangle Triangle2;

	// Add Quad
	void Create(FVector v1, FVector v2, FVector v3, FVector v4, FAdvancedPlanetTriangle t1, FAdvancedPlanetTriangle t2)
	{
		Vector1 = v1;
		Vector2 = v2;
		Vector3 = v3;
		Vector4 = v4;
		Triangle1 = t1;
		Triangle2 = t2;
	};

	//Constructor
	FAdvancedPlanetQuad()
		: Vector1(FVector(0, 0, 0))
		, Vector2(FVector(0, 0, 0))
		, Vector3(FVector(0, 0, 0))
		, Vector4(FVector(0, 0, 0))
		, Triangle1(FAdvancedPlanetTriangle())
		, Triangle2(FAdvancedPlanetTriangle())
	{};
};

USTRUCT()
struct FAdvancedPlanetTerrainNoise
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Terrain)
		EAdvancedPlanetNoiseType NoiseType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Terrain)
		float Amplitude;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Terrain)
		int32 Octaves;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Terrain)
		float Persistence;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Terrain)
		float Frequency;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Terrain)
		int32 Seed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Terrain)
		float MaxTerrainHeight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Terrain)
		float BaseScaleOffset;

	void Create(
		int32 IN_Seed,
		float IN_Amplitude,
		int32 IN_Octaves,
		float IN_Persistence,
		float IN_Frequency,
		float IN_MaxTerrainHeight,
		float IN_BaseScaleOffset,
		EAdvancedPlanetNoiseType IN_NoiseType)
	{
		Seed = IN_Seed;
		Amplitude = IN_Amplitude;
		Octaves = IN_Octaves;
		Persistence = IN_Persistence;
		Frequency = IN_Frequency;
		MaxTerrainHeight = IN_MaxTerrainHeight;
		BaseScaleOffset = IN_BaseScaleOffset;
		NoiseType = IN_NoiseType;
	};

	FAdvancedPlanetTerrainNoise()
		: Seed(0)
		, Amplitude(1)
		, Octaves(4)
		, Persistence(0.4f)
		, Frequency(0.007f)
		, MaxTerrainHeight(10)
		, BaseScaleOffset(0)
		, NoiseType(EAdvancedPlanetNoiseType::LN_Perlin)
	{};
};

USTRUCT(BlueprintType)
struct FAdvancedPlanetSectionGenTask
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Task)
		int32 SectionID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Task)
		int32 LOD;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Task)
		bool Visible;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Task)
		FAdvancedPlanetQuad BaseQuad;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Task)
		FAdvancedPlanetTerrainNoise TerrainNoise;

	FAdvancedPlanetSectionGenTask()
		: SectionID(0)
		, LOD(1)
		, Visible(false)
		, TerrainNoise(FAdvancedPlanetTerrainNoise())
	{};
};

USTRUCT(BlueprintType)
struct FAdvancedPlanetMeshSectionRef
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Task)
	TArray<int32> IndexList;

	FAdvancedPlanetMeshSectionRef(){};
};






/* Advanced Planet Actor */
UCLASS()
class STARPOST_ALPHA_2_API AAdvancedPlanetActor : public AActor
{
	GENERATED_BODY()

public:

	// Planet Root Component
	UPROPERTY(VisibleAnywhere, Category = "Scene Components")
		class USceneComponent* PlanetRoot;

	// Details
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
		FString PlanetName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
		int32 Radius;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
		bool HasWater;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
		float OceanLevel;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
		float AtmosphereHeight;

	// Atmosphere Radius
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmospheric Radius")
		float InnerRadius;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmospheric Radius")
		float OuterRadius;

	// Atmosphere Lightning
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmospheric Lightning")
		FLinearColor LightBrightness;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmospheric Lightning")
		FVector LightDirection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmospheric Lightning")
		FAdvancedPlanetShadeParameters SkyShadeParameters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmospheric Lightning")
		FAdvancedPlanetShadeParameters SurfaceShadeParameters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmospheric Lightning")
		float Shadow;

	// Atmosphere Parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmospheric Parameters")
		FAdvancedPlanetAtmoLayerParameters RayleighLayerParameters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmospheric Parameters")
		FAdvancedPlanetAtmoLayerParameters MieLayerParameters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmospheric Parameters")
		float MieG;

	// Planet Surface
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Surface")
		FLinearColor SurfaceColor1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Surface")
		FLinearColor SurfaceColor2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Surface")
		float SurfaceScale;

	// Materials
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
		UMaterialInterface* SkyMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
		UMaterialInterface* SurfaceMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
		UMaterialInterface* WaterVolumeMaterial;

	// Terrain
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
		FAdvancedPlanetMaterialSlopeFactors MatSlopeFactors;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
		TArray<FAdvancedPlanetLayer> PlanetLayers;

	// LOD Details
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level of Detail")
		int32 CubeQuadFaceCount;	// ((CubeQuadFaceCount * CubeQuadFaceCount) * 6) = Our SectionCount
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level of Detail")
		int32 GenLOD;				// SectionCount * GenLOD = Our Quad Count

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level of Detail")
		int32 AtmosphereLOD;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level of Detail")
		int32 OceanLOD;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level of Detail")
		int32 NumberOfLODs;

	// Debug Vertices
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
		bool DebugVertices;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
		int32 ThreadsToUse;

private:

	// Dynamic Mesh Components
	UDynamicMeshComponent* DynamicMeshComp_Terrain;
	UDynamicMeshComponent* DynamicMeshComp_Atmosphere;
	UDynamicMeshComponent* DynamicMeshComp_Ocean;

	// Our Dynamic Material Instances
	UMaterialInstanceDynamic* MaterialInstance_Terrain;
	UMaterialInstanceDynamic* MaterialInstance_Atmosphere;
	UMaterialInstanceDynamic* MaterialInstance_Ocean;
	TArray<UMaterialInstanceDynamic*> DynPlanetMaterialInstances;

	// Master Worker Thread
	FAdvancedPlanetMasterWorker* MasterWorker;

	// Player Controller Reference
	APlayerController* PlayerCon;

	// Player Pawn Reference
	AExpCharacter* PlayerChar;

	// Cube Quads Need Generated?
	bool GenerateCubeQuadsNeeded;

	// Initial Terrain Data Need Generated?
	bool GenerateInitialTerrainNeeded;

	// Initial Ocean Data Need Generated?
	bool GenerateInitialOceanNeeded;

	// Initial Atmosphere Data Need Generated?
	bool GenerateInitialAtmosphereNeeded;

	// Are we in the scaling state?
	bool ScalingState;

	// Are we inside the atmosphere?
	bool InsideAtmo;

	// Planet Loaded?
	bool PlanetHasLoaded;

	// Last Field Saves
	int32 LastPlanetLODIndex;
	FVector LastCameraPosition;
	float LastDistToPlanetSurface;

	// Mesh Section Index
	TArray<FAdvancedPlanetMeshSectionRef> MeshSectionIndices;

public:

	// Constructor and Deconstructor
	AAdvancedPlanetActor(const FObjectInitializer& ObjectInitializer);
	virtual ~AAdvancedPlanetActor();

	// Our Virtual Functions
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	// Set Material Parameters
	UFUNCTION(BlueprintCallable, meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", DisplayName = "Set Material Parameters", Keywords = "set material parameters"), Category = "Materials")
		void SetMaterialParameters(UMaterialInstanceDynamic* MatInst);

private:

	// Handle Cube Data
	void HandleCubeData();

	// Handle Ocean and Atmosphere Data
	void HandleOceanAtmoData();

	// Handle Initial Terrain Data
	void HandleInitialTerrainData();

	// Update Planet
	void UpdatePlanet();
	void UpdatePlanetScaling();

	//TEMPLATE Load Obj From Path
	template <typename ObjClass>
	static FORCEINLINE ObjClass* LoadObjFromPath(const FName& Path)
	{
		if (Path == NAME_None) return NULL;
		//~

		return Cast<ObjClass>(StaticLoadObject(ObjClass::StaticClass(), NULL, *Path.ToString()));
	}
	// Load PS From Path 
	static FORCEINLINE UParticleSystem* LoadPSFromPath(const FName& Path)
	{
		if (Path == NAME_None) return NULL;
		//~

		return LoadObjFromPath<UParticleSystem>(Path);
	}
	// Load Material From Path 
	static FORCEINLINE UMaterialInterface* LoadMatFromPath(const FName& Path)
	{
		if (Path == NAME_None) return NULL;
		//~

		return LoadObjFromPath<UMaterialInterface>(Path);
	}
	// Load Static Mesh From Path 
	static FORCEINLINE UStaticMesh* LoadMeshFromPath(const FName& Path)
	{
		if (Path == NAME_None) return NULL;
		//~

		return LoadObjFromPath<UStaticMesh>(Path);
	}


};






/* Advanced Planet Task Worker */
class STARPOST_ALPHA_2_API FAdvancedPlanetTaskWorker : public FRunnable
{

public:

	// To check wether we are done or not?
	bool HasCompleted;

	// Below are our data collectors..
	// They are accessed, by the game thread, only when HasCompletedAllTasks is true
	// They are prefixed with LimitedAccess_*

	// Mesh Section
	FDynMeshSection LimitedAccess_MeshSection;

private:

	// Thread to run FRunnableThread on
	FRunnableThread* Thread;

	// Task ID
	int32 TaskID;

	// The Working Task
	FAdvancedPlanetSectionGenTask WorkingTask;

	// Stopping Param
	bool Stopping;

	// Our Planet Layer Property Array
	TArray<FAdvancedPlanetLayer> PlanetLayers;

	// Slope Blend Factors
	FAdvancedPlanetMaterialSlopeFactors MatSlopeFactors;

	// Radius of planet
	int32 Radius;

public:

	// Constructor
	FAdvancedPlanetTaskWorker(
		int32 ThreadID,
		int32 IN_Radius,
		TArray<FAdvancedPlanetLayer> IN_PlanetLayers,
		FAdvancedPlanetMaterialSlopeFactors IN_MatSlopeFactors);

	// Destructor
	~FAdvancedPlanetTaskWorker();

	// Begin FRunnable interface
	virtual bool Init() override { return true; }
	virtual void Stop() override { Stopping = true; }
	virtual void Exit() override { }

	// This adds a task to our task pool
	void BeginWork(FAdvancedPlanetSectionGenTask Task);

	// Our Run Function
	uint32 Run();

private:

	// Subdivide Our Meshes
	void SubdivideQuad();

	// Compute Terrain
	void ComputeTerrain();

	// Setup our Vertice Color Map
	FColor CreateVertexColorMap(float LayerHeight);

};






/* Advanced Planet Master Worker */
class STARPOST_ALPHA_2_API FAdvancedPlanetMasterWorker : public FRunnable
{

public:

	// To check wether we are done with all tasks or not?
	bool HasCompletedAllTasks;

	// Our gen type
	EAdvancedPlanetGenType GenType;

	// Below are our data collectors..
	// They are accessed, by the game thread, only when HasCompletedAllTasks is true
	// They are prefixed with LimitedAccess_*

	// Mesh Sections
	TArray<FDynMeshSection> LimitedAccess_MeshSections;
	TArray<FDynMeshProxySection*> LimitedAccess_ProxyMeshSections;

private:

	// Thread to run the Worker Threads on
	FRunnableThread* Thread;

	// Our Worker Threads
	TArray<FAdvancedPlanetTaskWorker*> WorkerThreads;

	// Our Task Queue
	TArray<FAdvancedPlanetSectionGenTask> TaskQueue;

	// Generate Cube Quads
	void GenerateCubeQuads();

	// Generate Single Sphere
	TArray<FAdvancedPlanetQuad> GenerateAtmoOceanQuads();

	// Generate Cube Quad Face
	TArray<FAdvancedPlanetQuad> GenerateCubeQuadFace(FVector Vert1, FVector Vert2, FVector Vert3, FVector Vert4);

	// Generate Proxy Mesh Buffer
	void GenerateProxyMeshBuffer();

	// Generate Sphere Mesh From Quads
	FDynMeshSection GenerateSphereMeshFromQuads(TArray<FAdvancedPlanetQuad> Quads, int32 LOD, float Height, bool InvertFaces);

	// How many threads to use..
	int32 NumOfThreadsToUse;

	// Quads Per Face
	int32 QuadsPerFace;

	// Cube Quads
	TArray<FAdvancedPlanetQuad> CubeQuads;

	// Our Planet Layer Property Array
	TArray<FAdvancedPlanetLayer> PlanetLayers;

	// Slope Blend Factors
	FAdvancedPlanetMaterialSlopeFactors MatSlopeFactors;

	// Radius of planet
	int32 Radius;

	// Water And Atmosphere LOD
	int32 OceanLOD;
	int32 AtmosphereLOD;

	// Ocean and Atmosphere Level
	float OceanLevel;
	float AtmosphereHeight;

	// Has Water?
	bool bHasWater;

	// Stopping Param
	bool bStopping;

public:

	// Constructor
	FAdvancedPlanetMasterWorker(
		int32 IN_Radius,
		TArray<FAdvancedPlanetLayer> IN_PlanetLayers,
		FAdvancedPlanetMaterialSlopeFactors IN_MatSlopeFactors);

	// Destructor
	~FAdvancedPlanetMasterWorker();

	// Begin FRunnable interface
	virtual bool Init() override { return true; }
	virtual void Stop() override;
	virtual void Exit() override { }

	// This adds a task to our task pool
	void AddWork(TArray<FAdvancedPlanetSectionGenTask> Tasks);

	// This will generate our cube data
	void GenerateCubeData(int32 IN_QuadsPerFace);

	// This will generate our ocean and atmosphere data
	void GenerateOceanAtmo(
		int32 IN_OceanLOD, float IN_OceanLevel, 
		int32 IN_AtmoLOD, float IN_AtmosphereHeight);
	void GenerateOceanAtmo(
		int32 IN_AtmoLOD, float IN_AtmosphereHeight);

	// Allocate More Threads
	void AllocateMoreThreads(int32 ThreadsToUse);

	// Our Run Function
	uint32 Run();
};


