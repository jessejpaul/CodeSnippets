// Fill out your copyright notice in the Description page of Project Settings.

#include "StarPost_Alpha_2.h"
#include "AdvancedPlanetActor.h"





/*
~~ Advanced Planet Actor ~~
*/

// Sets default values
AAdvancedPlanetActor::AAdvancedPlanetActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Our initial scene comp
	PlanetRoot = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("PlanetScene"));
	RootComponent = PlanetRoot;

	// Our dynamic mesh scene comps
	DynamicMeshComp_Terrain = ObjectInitializer.CreateDefaultSubobject<UDynamicMeshComponent>(this, TEXT("DynamicMeshComp_Terrain"));
	DynamicMeshComp_Terrain->AttachParent = RootComponent;
	DynamicMeshComp_Terrain->SetVisibility(false);

	DynamicMeshComp_Ocean = ObjectInitializer.CreateDefaultSubobject<UDynamicMeshComponent>(this, TEXT("DynamicMeshComp_Ocean"));
	DynamicMeshComp_Ocean->AttachParent = RootComponent;
	DynamicMeshComp_Ocean->SetVisibility(false);

	DynamicMeshComp_Atmosphere = ObjectInitializer.CreateDefaultSubobject<UDynamicMeshComponent>(this, TEXT("DynamicMeshComp_Atmosphere"));
	DynamicMeshComp_Atmosphere->AttachParent = RootComponent;
	DynamicMeshComp_Atmosphere->SetVisibility(false);

	// Detail Properties
	PlanetName = "New Planet";
	Radius = 500;
	HasWater = false;
	OceanLevel = 100;
	AtmosphereHeight = 100;

	// Atmosphere Properties
	InnerRadius = 500;
	OuterRadius = 600;

	LightBrightness = FLinearColor(2, 2, 2, 0);
	LightDirection = FVector(1, 0, 0);
	SkyShadeParameters = FAdvancedPlanetShadeParameters();
	SkyShadeParameters.Multiplier = 0.5f;
	SkyShadeParameters.Constant = 0.5f;
	SkyShadeParameters.Power = 7.313037f;
	SurfaceShadeParameters = FAdvancedPlanetShadeParameters();
	SurfaceShadeParameters.Multiplier = 0.5f;
	SurfaceShadeParameters.Constant = 0.5f;
	SurfaceShadeParameters.Power = 12.40786f;
	Shadow = 0;

	RayleighLayerParameters = FAdvancedPlanetAtmoLayerParameters();
	RayleighLayerParameters.Attenuation = FLinearColor(0.0f, 0.023f, 0.057f, 0);
	RayleighLayerParameters.Scatter = FLinearColor(0.171f, 0.32f, 0.763f, 0);
	RayleighLayerParameters.ScaleHeight = 10.032986;
	RayleighLayerParameters.Density = 1.339816;

	MieLayerParameters = FAdvancedPlanetAtmoLayerParameters();
	MieLayerParameters.Attenuation = FLinearColor(0, 0, 0, 0);
	MieLayerParameters.Scatter = FLinearColor(1, 1, 1, 0);
	MieLayerParameters.ScaleHeight = 3.822195f;
	MieLayerParameters.Density = 1.339816f;

	MieG = 0.76;

	SurfaceColor1 = FLinearColor(0.825, 0.825, 0.825, 0);
	SurfaceColor2 = FLinearColor(0.36, 0.36, 0.36, 0);
	SurfaceScale = 0.002;

	// We need to create our first layer
	// No need for combination details..
	PlanetLayers.Add(FAdvancedPlanetLayer());
	PlanetLayers.Last().Create("Base Layer",
		513513,		// Seed
		1,			// Amplitude
		8,			// Octaves
		0.6f,		// Persistence
		0.001f,		// Frequency
		400,		// MaxTerrainHeight
		Radius,		// PlanetBaseScale
		EAdvancedPlanetLayerCombinationType::LC_None,
		EAdvancedPlanetNoiseType::LN_Perlin);

	MatSlopeFactors = FAdvancedPlanetMaterialSlopeFactors();


	// Cube Quad Details
	GenerateCubeQuadsNeeded = true;
	CubeQuadFaceCount = 4;	// ((CubeQuadFaceCount * CubeQuadFaceCount) * 6) = Our SectionCount
	GenLOD = 64;			// SectionCount * GenLOD = Our Quad Count
	AtmosphereLOD = 32;
	OceanLOD = 32;
	NumberOfLODs = 5;

	// Last Field Saves
	LastPlanetLODIndex = -1;
	LastCameraPosition = FVector(0, 0, 0);
	LastDistToPlanetSurface = -1;

	// Triggers
	GenerateInitialTerrainNeeded = false;
	GenerateInitialAtmosphereNeeded = false;
	GenerateInitialOceanNeeded = false;
	ScalingState = false;
	PlanetHasLoaded = false;
	InsideAtmo = false;

	// Debug
	DebugVertices = false;
	ThreadsToUse = 2;

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Deconstructor
AAdvancedPlanetActor::~AAdvancedPlanetActor()
{
	// Delete it and make it NULL
	delete MasterWorker;
	MasterWorker = NULL;
}

// Called when the game starts or when spawned
void AAdvancedPlanetActor::BeginPlay()
{
	// Let's grab our player controller and pawn..
	PlayerCon = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	PlayerChar = Cast<AExpCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	

	// Our Master Worker Thread
	MasterWorker = new FAdvancedPlanetMasterWorker(Radius, PlanetLayers, MatSlopeFactors);

	// Material
	if (SkyMaterial)
	{
		DynPlanetMaterialInstances.Add(DynamicMeshComp_Atmosphere->CreateDynamicMaterialInstance(0, SkyMaterial));
		SetMaterialParameters(DynPlanetMaterialInstances.Last());
	}

	// Material
	if (WaterVolumeMaterial)
	{
		DynPlanetMaterialInstances.Add(DynamicMeshComp_Ocean->CreateDynamicMaterialInstance(0, WaterVolumeMaterial));
		SetMaterialParameters(DynPlanetMaterialInstances.Last());
	}

	// Super
	Super::BeginPlay();
}

// Called when the game starts or when spawned
void AAdvancedPlanetActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	MasterWorker->Stop();

	// Super
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AAdvancedPlanetActor::Tick(float DeltaTime)
{
	// First let's check if we need to regenerate our cube quads
	if (GenerateCubeQuadsNeeded)
		HandleCubeData();

	// First let's check if we need to regenerate our ocean and atmosphere
	if (GenerateInitialAtmosphereNeeded || GenerateInitialOceanNeeded)
		HandleOceanAtmoData();

	// Now let's check if we need to regenerate all our initial terrain data
	if (GenerateInitialTerrainNeeded)
		HandleInitialTerrainData();

	if (PlanetHasLoaded && DynamicMeshComp_Terrain->MeshSceneProxy)
	{
		if (ScalingState)
			UpdatePlanetScaling();
		else
			UpdatePlanet();

		for (int i = 0; i < DynPlanetMaterialInstances.Num(); i++)
			SetMaterialParameters(DynPlanetMaterialInstances[i]);
	}




	if (UGameplayStatics::GetPlayerController(GetWorld(), 0)->IsInputKeyDown(EKeys::Insert))
	{
		
		SurfaceShadeParameters.Constant += 0.1;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, "~ Constant: " + FString::SanitizeFloat(SurfaceShadeParameters.Constant));
	}

	if (UGameplayStatics::GetPlayerController(GetWorld(), 0)->IsInputKeyDown(EKeys::Home))
	{
		SurfaceShadeParameters.Multiplier += 0.1;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, "~ Multiplier: " + FString::SanitizeFloat(SurfaceShadeParameters.Multiplier));
	}

	if (UGameplayStatics::GetPlayerController(GetWorld(), 0)->IsInputKeyDown(EKeys::PageUp))
	{
		SurfaceShadeParameters.Power += 0.1;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, "~ Power: " + FString::SanitizeFloat(SurfaceShadeParameters.Power));
	}


	if (UGameplayStatics::GetPlayerController(GetWorld(), 0)->IsInputKeyDown(EKeys::Delete))
	{
		SurfaceShadeParameters.Constant -= 0.1;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, "~ Constant: " + FString::SanitizeFloat(SurfaceShadeParameters.Constant));
	}

	if (UGameplayStatics::GetPlayerController(GetWorld(), 0)->IsInputKeyDown(EKeys::End))
	{
		SurfaceShadeParameters.Multiplier -= 0.1;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, "~ Multiplier: " + FString::SanitizeFloat(SurfaceShadeParameters.Multiplier));
	}

	if (UGameplayStatics::GetPlayerController(GetWorld(), 0)->IsInputKeyDown(EKeys::PageDown))
	{
		SurfaceShadeParameters.Power -= 0.1;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, "~ Power: " + FString::SanitizeFloat(SurfaceShadeParameters.Power));
	}

	Super::Tick(DeltaTime);
}

// Handle Cube Data
void AAdvancedPlanetActor::HandleCubeData()
{
	// If our worker is still proccessing, skip..
	if (!MasterWorker->HasCompletedAllTasks)
		return;

	// Now let's check if we are in standby..
	// If so, let's start our cube data proccessing..
	if (MasterWorker->GenType == EAdvancedPlanetGenType::GT_StandBy)
	{
		MasterWorker->AllocateMoreThreads(ThreadsToUse);

		// Begin the work..
		MasterWorker->GenerateCubeData(CubeQuadFaceCount);

		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, "Generating Quads..");

		// Now return..
		return;
	}

	// Now check if our cube data has been proccessed..
	if (MasterWorker->GenType == EAdvancedPlanetGenType::GT_CubeData)
	{
		// Now we need our oceans and atmosphere data..
		GenerateInitialAtmosphereNeeded = true;
		if (HasWater)
			GenerateInitialOceanNeeded = true;

		// Reset our cube data trigger
		GenerateCubeQuadsNeeded = false;
	}

	// Reset our worker to stand by..
	MasterWorker->GenType = EAdvancedPlanetGenType::GT_StandBy;
}

// Handle Cube Data
void AAdvancedPlanetActor::HandleOceanAtmoData()
{
	// If our worker is still proccessing, skip..
	if (!MasterWorker->HasCompletedAllTasks)
		return;

	// Now let's check if we are in standby..
	// If so, let's start our cube data proccessing..
	if (MasterWorker->GenType == EAdvancedPlanetGenType::GT_StandBy)
	{
		MasterWorker->AllocateMoreThreads(ThreadsToUse);

		// Begin the work..
		if (HasWater)
			MasterWorker->GenerateOceanAtmo(OceanLOD, OceanLevel, AtmosphereLOD, AtmosphereHeight);
		else
			MasterWorker->GenerateOceanAtmo(AtmosphereLOD, AtmosphereHeight);

		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, "Generating Ocean and Atmosphere..");

		// Now return..
		return;
	}

	// Now check if our cube data has been proccessed..
	if (MasterWorker->GenType == EAdvancedPlanetGenType::GT_OceanAtmoData)
	{
		// Now we need our terrain data..
		GenerateInitialTerrainNeeded = true;

		// Reset our data triggers
		GenerateInitialAtmosphereNeeded = false;
		GenerateInitialOceanNeeded = false;

		DynamicMeshComp_Atmosphere->AddMeshSection(
			0, MasterWorker->LimitedAccess_MeshSections[0], false,
			MasterWorker->LimitedAccess_ProxyMeshSections[0]);

		if (HasWater)
			DynamicMeshComp_Ocean->AddMeshSection(
				0, MasterWorker->LimitedAccess_MeshSections[1], false,
				MasterWorker->LimitedAccess_ProxyMeshSections[1]);
	
	}

	// Reset our worker to stand by..
	MasterWorker->GenType = EAdvancedPlanetGenType::GT_StandBy;
}

// Handle Cube Data
void AAdvancedPlanetActor::HandleInitialTerrainData()
{
	// If our worker is still proccessing, skip..
	if (!MasterWorker->HasCompletedAllTasks)
		return;

	// Now let's check if we are in standby..
	// If so, let's start our cube data proccessing..
	if (MasterWorker->GenType == EAdvancedPlanetGenType::GT_StandBy)
	{
		// Setup our terrain gen task
		TArray<FAdvancedPlanetSectionGenTask> Tasks;
		int32 SectionCount = ((CubeQuadFaceCount*CubeQuadFaceCount) * 6);

		for (int n = 0; n < NumberOfLODs; n++)
		{
			MeshSectionIndices.Add(FAdvancedPlanetMeshSectionRef());
			for (int i = 0; i < SectionCount; i++)
			{
				Tasks.Add(FAdvancedPlanetSectionGenTask());
				Tasks.Last().LOD = GenLOD*(n + 1);
				Tasks.Last().SectionID = i;
				if (n == 0)
					Tasks.Last().Visible = true;
				else
					Tasks.Last().Visible = false;
				Tasks.Last().TerrainNoise = FAdvancedPlanetTerrainNoise();

				MeshSectionIndices.Last().IndexList.Add(i + (SectionCount*n));
			}
		}

		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, "Generating Terrain..");
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Allocating ( " + FString::SanitizeFloat(ThreadsToUse) + " ) Threads..");

		MasterWorker->AllocateMoreThreads(ThreadsToUse);

		// Begin the work..
		MasterWorker->AddWork(Tasks);

		// Now return..
		return;
	}

	// Now check if our generation data has been proccessed..
	if (MasterWorker->GenType == EAdvancedPlanetGenType::GT_SectionData)
	{
		// Now we need our terrain data..
		GenerateInitialTerrainNeeded = false;

		int32 VertCount = 0;
		for (int i = 0; i < MasterWorker->LimitedAccess_MeshSections.Num(); i++)
		{
			DynamicMeshComp_Terrain->AddMeshSection(
				i, MasterWorker->LimitedAccess_MeshSections[i], true, MasterWorker->LimitedAccess_ProxyMeshSections[i]);

			// Material
			if (SurfaceMaterial)
			{
				//DynamicMeshComp_Terrain->SetMaterial(0, SurfaceMaterial);
				DynPlanetMaterialInstances.Add(DynamicMeshComp_Terrain->CreateDynamicMaterialInstance(i, SurfaceMaterial));
				SetMaterialParameters(DynPlanetMaterialInstances.Last());
			}

			VertCount += MasterWorker->LimitedAccess_MeshSections[i].ProcVertexBuffer.Num();

			if (DebugVertices)
				for (int n = 0; n < MasterWorker->LimitedAccess_MeshSections[i].ProcVertexBuffer.Num(); n++)
				{
					FVector Pos = MasterWorker->LimitedAccess_MeshSections[i].ProcVertexBuffer[n].Position;
					UStaticMeshComponent* StaticMeshComp;
					StaticMeshComp = NewObject<UStaticMeshComponent>(RootComponent);
					UStaticMesh* Mesh = LoadMeshFromPath("/Engine/EngineMeshes/Sphere.Sphere");
					StaticMeshComp->SetStaticMesh(Mesh);
					StaticMeshComp->AttachParent = RootComponent;
					StaticMeshComp->RelativeScale3D = FVector(0.03f, 0.03f, 0.03f);
					StaticMeshComp->RelativeLocation = Pos;
					StaticMeshComp->RegisterComponent();
					StaticMeshComp->SetVisibility(true);

					//DrawDebugString(GetWorld(), Pos, Pos.ToString(), this, FColor::Red, -1.0f);
				}
		}

		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Vertex Count: " + FString::SanitizeFloat(VertCount));

		float realtimeSeconds = UGameplayStatics::GetRealTimeSeconds(GetWorld());
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "Time of Generation: " + FString::SanitizeFloat(realtimeSeconds) + " Seconds..");

		DynamicMeshComp_Terrain->SetVisibility(true);
		DynamicMeshComp_Atmosphere->SetVisibility(true);
		if (HasWater)
			DynamicMeshComp_Ocean->SetVisibility(true);

		PlanetHasLoaded = true;

		// Reset our worker to stand by..
		MasterWorker->GenType = EAdvancedPlanetGenType::GT_StandBy;
	}
}

// Set Material Parameters
void AAdvancedPlanetActor::SetMaterialParameters(UMaterialInstanceDynamic* MatInst)
{
	if (MatInst)
	{
		MatInst->SetScalarParameterValue(FName("AtmoInnerR"), InnerRadius);
		MatInst->SetScalarParameterValue(FName("AtmoOuterR"), OuterRadius);

		MatInst->SetVectorParameterValue(FName("LightBrightness"), LightBrightness);
		MatInst->SetVectorParameterValue(FName("LightDirection"), FLinearColor(LightDirection.X, LightDirection.Y, LightDirection.Z));

		//SetShadeParam
		MatInst->SetScalarParameterValue(FName("SkyShadeMultiplier"), SkyShadeParameters.Multiplier);
		MatInst->SetScalarParameterValue(FName("SkyShadeConstant"), SkyShadeParameters.Constant);
		MatInst->SetScalarParameterValue(FName("SkyShadePower"), SkyShadeParameters.Power);

		//SetShadeParam
		MatInst->SetScalarParameterValue(FName("SurfaceShadeMultiplier"), SurfaceShadeParameters.Multiplier);
		MatInst->SetScalarParameterValue(FName("SurfaceShadeConstant"), SurfaceShadeParameters.Constant);
		MatInst->SetScalarParameterValue(FName("SurfaceShadePower"), SurfaceShadeParameters.Power);

		//SetLayerParam
		MatInst->SetVectorParameterValue(FName("RayleighAttenuation"), RayleighLayerParameters.Attenuation * RayleighLayerParameters.Density);
		MatInst->SetVectorParameterValue(FName("RayleighScatter"), RayleighLayerParameters.Scatter * RayleighLayerParameters.Density);
		MatInst->SetScalarParameterValue(FName("RayleighHeight"), RayleighLayerParameters.ScaleHeight);

		//SetLayerParam
		MatInst->SetVectorParameterValue(FName("MieAttenuation"), MieLayerParameters.Attenuation * MieLayerParameters.Density);
		MatInst->SetVectorParameterValue(FName("MieScatter"), MieLayerParameters.Scatter * MieLayerParameters.Density);
		MatInst->SetScalarParameterValue(FName("MieHeight"), MieLayerParameters.ScaleHeight);


		MatInst->SetScalarParameterValue(FName("*ShadeMultiplier"), Shadow);
		MatInst->SetScalarParameterValue(FName("MieG"), MieG);

		MatInst->SetVectorParameterValue(FName("SurfaceColor1"), SurfaceColor1);
		MatInst->SetVectorParameterValue(FName("SurfaceColor2"), SurfaceColor2);
		MatInst->SetScalarParameterValue(FName("SurfaceScale"), SurfaceScale);
	}
}

// Update our planet faces
void AAdvancedPlanetActor::UpdatePlanet()
{
	// Grab the current camera's location
	FVector CamLocation = PlayerCon->PlayerCameraManager->GetCameraLocation();

	// Did we move?
	if (CamLocation == LastCameraPosition)
		return;

	// Reset
	LastCameraPosition = CamLocation;

	// Our surface point
	FVector SurfacePoint = CamLocation;
	SurfacePoint.Normalize();
	SurfacePoint *= Radius;

	// Get our distance to planet surface
	float DistToPlanetSurface = (CamLocation - RootComponent->GetComponentLocation()).Size() - Radius;

	// Our distance percentage
	float ClampedDist = FMath::Clamp(DistToPlanetSurface, 0.0f, Radius*15.0f);
	float PerToPlanet = (ClampedDist / (Radius*15.0f)) * 100.0f;
	PerToPlanet = FMath::Clamp(PerToPlanet, 0.f, 100.f);

	// Our LOD Index
	int32 PlanetLODIndex = 0;

	// Grab our LOD Index by distance..
	for (int Vid = 0; Vid < NumberOfLODs; Vid++)
	{
		float DValue = ((float)Vid / (float)NumberOfLODs) * 100.f;
		if (PerToPlanet > DValue)
			PlanetLODIndex = Vid;
	}

	if (!InsideAtmo && DistToPlanetSurface < AtmosphereHeight)
	{
		InsideAtmo = true;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "~ Entering Atmosphere! ");
	}
	else if (InsideAtmo && DistToPlanetSurface > AtmosphereHeight)
	{
		InsideAtmo = false;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "~ Exiting Atmosphere! ");
	}


	FVector SectionPosition;
	float DistanceToCull = (Radius + (Radius / 2));
	if (InsideAtmo)
		for (int i = 0; i < MeshSectionIndices[LastPlanetLODIndex].IndexList.Num(); i++)
		{
			int32 ind = MeshSectionIndices[LastPlanetLODIndex].IndexList[i];
			if (ind >= DynamicMeshComp_Terrain->MeshSceneProxy->Sections.Num())
				continue;

			SectionPosition = DynamicMeshComp_Terrain->MeshSceneProxy->Sections[ind]->Position;

			if ((SurfacePoint - SectionPosition).Size() <= DistanceToCull)
				DynamicMeshComp_Terrain->MeshSceneProxy->Sections[ind]->Visible = true;
			else
				DynamicMeshComp_Terrain->MeshSceneProxy->Sections[ind]->Visible = false;
		}

	// Need to change our LOD?
	if (PlanetLODIndex == LastPlanetLODIndex)
		return;

	if (PlanetLODIndex == 0)
	{
		//ScalingState = true;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "~ Scaling State Active! ");
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "~ Camera location: " + CamLocation.ToString());
		//PlayerChar->InputActive = false;
	}

	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, "~ Percentage to planet: " + FString::SanitizeFloat(PerToPlanet));
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, "~ Index: " + FString::SanitizeFloat(PlanetLODIndex));

	// Hide previous planet LOD
	if (LastPlanetLODIndex != -1)
		for (int i = 0; i < MeshSectionIndices[LastPlanetLODIndex].IndexList.Num(); i++)
		{
			int32 ind = MeshSectionIndices[LastPlanetLODIndex].IndexList[i];
			if (ind >= DynamicMeshComp_Terrain->MeshSceneProxy->Sections.Num())
				continue;

				DynamicMeshComp_Terrain->MeshSceneProxy->Sections[ind]->Visible = false;
		}
		

	for (int i = 0; i < MeshSectionIndices[PlanetLODIndex].IndexList.Num(); i++)
	{
		int32 ind = MeshSectionIndices[PlanetLODIndex].IndexList[i];
		if (ind >= DynamicMeshComp_Terrain->MeshSceneProxy->Sections.Num())
			continue;

		if (!InsideAtmo)
			DynamicMeshComp_Terrain->MeshSceneProxy->Sections[ind]->Visible = true;

		if (InsideAtmo)
		{
			SectionPosition = DynamicMeshComp_Terrain->MeshSceneProxy->Sections[ind]->Position;

			if ((SurfacePoint - SectionPosition).Size() <= DistanceToCull)
				DynamicMeshComp_Terrain->MeshSceneProxy->Sections[ind]->Visible = true;
			else
				DynamicMeshComp_Terrain->MeshSceneProxy->Sections[ind]->Visible = false;
		}
	}

	// Update last index
	LastPlanetLODIndex = PlanetLODIndex;
}

// Update our planet faces
void AAdvancedPlanetActor::UpdatePlanetScaling()
{
	if (PlayerChar->MovementValue == FVector::ZeroVector)
		return;

	// Grab the current camera's location
	FVector CamLocation = PlayerChar->MovementValue;

	// Our surface point
	FVector SurfacePoint = CamLocation;
	SurfacePoint.Normalize();
	SurfacePoint *= Radius;

	// Get our distance to planet surface
	float DistToPlanetSurface = (CamLocation - RootComponent->GetComponentLocation()).Size() - Radius;

	if (DistToPlanetSurface != LastDistToPlanetSurface &&
		LastDistToPlanetSurface != -1)
	{
		float ScaleChange = LastDistToPlanetSurface - DistToPlanetSurface;
		return;

		DynamicMeshComp_Terrain->SetRelativeScale3D(FVector(ScaleChange, ScaleChange, ScaleChange));
		DynamicMeshComp_Atmosphere->SetRelativeScale3D(FVector(ScaleChange, ScaleChange, ScaleChange));
		if (HasWater)
			DynamicMeshComp_Ocean->SetRelativeScale3D(FVector(ScaleChange, ScaleChange, ScaleChange));
		//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "~ Scale: " + FString::SanitizeFloat(this->GetActorScale3D().X));
	}

	return;

	if (DynamicMeshComp_Terrain->RelativeScale3D.X <= 1 && LastDistToPlanetSurface != -1)
	{
		DynamicMeshComp_Terrain->SetRelativeScale3D(FVector(1, 1, 1));
		DynamicMeshComp_Atmosphere->SetRelativeScale3D(FVector(1, 1, 1));
		DynamicMeshComp_Ocean->SetRelativeScale3D(FVector(1, 1, 1));
		ScalingState = false;
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "~ Scaling State Disabled! ");
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "~ Camera location: " + CamLocation.ToString());

		PlayerChar->InputActive = true;
	}

	LastDistToPlanetSurface = DistToPlanetSurface;

	// Our distance percentage
	float ClampedDist = FMath::Clamp(DistToPlanetSurface, 0.0f, Radius*15.0f);
	float PerToPlanet = (ClampedDist / (Radius*15.0f)) * 100.0f;
	PerToPlanet = FMath::Clamp(PerToPlanet, 0.f, 100.f);

	// Our LOD Index
	int32 PlanetLODIndex = 0;

	// Grab our LOD Index by distance..
	for (int Vid = 0; Vid < NumberOfLODs; Vid++)
	{
		float DValue = ((float)Vid / (float)NumberOfLODs) * 100.f;
		if (PerToPlanet > DValue)
			PlanetLODIndex = Vid;
	}

	// Update Section Views..
	FVector SectionPosition;
	float DistanceToCull = (Radius + (Radius / 2));

	int f = 0;
	if (LastPlanetLODIndex != -1 && PerToPlanet < 50)
		for (int i = 0; i < MeshSectionIndices[LastPlanetLODIndex].IndexList.Num(); i++)
		{
			int32 ind = MeshSectionIndices[LastPlanetLODIndex].IndexList[i];
			if (ind >= DynamicMeshComp_Terrain->MeshSceneProxy->Sections.Num())
				continue;

			SectionPosition = DynamicMeshComp_Terrain->MeshSceneProxy->Sections[ind]->Position;

			if ((SurfacePoint - SectionPosition).Size() <= DistanceToCull)
				DynamicMeshComp_Terrain->MeshSceneProxy->Sections[ind]->Visible = true;
			else
				DynamicMeshComp_Terrain->MeshSceneProxy->Sections[ind]->Visible = false;
		}

	// Need to change our LOD?
	if (PlanetLODIndex == LastPlanetLODIndex)
		return;

	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, "~ Percentage to planet: " + FString::SanitizeFloat(PerToPlanet));
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, "~ Index: " + FString::SanitizeFloat(PlanetLODIndex));

	// Hide previous planet LOD
	if (LastPlanetLODIndex != -1)
		for (int i = 0; i < MeshSectionIndices[LastPlanetLODIndex].IndexList.Num(); i++)
		{
			int32 ind = MeshSectionIndices[LastPlanetLODIndex].IndexList[i];
			if (ind >= DynamicMeshComp_Terrain->MeshSceneProxy->Sections.Num())
				continue;

			DynamicMeshComp_Terrain->MeshSceneProxy->Sections[ind]->Visible = false;
		}

	for (int i = 0; i < MeshSectionIndices[PlanetLODIndex].IndexList.Num(); i++)
	{
		int32 ind = MeshSectionIndices[PlanetLODIndex].IndexList[i];
		if (ind >= DynamicMeshComp_Terrain->MeshSceneProxy->Sections.Num())
			continue;

		SectionPosition = DynamicMeshComp_Terrain->MeshSceneProxy->Sections[ind]->Position;
		if ((SurfacePoint - SectionPosition).Size() <= DistanceToCull)
			DynamicMeshComp_Terrain->MeshSceneProxy->Sections[ind]->Visible = true;
	}

	// Update last index
	LastPlanetLODIndex = PlanetLODIndex;
}


/*
~~ Advanced Planet Master Worker ~~
*/

// Constructor
FAdvancedPlanetMasterWorker::FAdvancedPlanetMasterWorker(
	int32 IN_Radius,
	TArray<FAdvancedPlanetLayer> IN_PlanetLayers,
	FAdvancedPlanetMaterialSlopeFactors IN_MatSlopeFactors)
	: Stopping(false)
	, HasCompletedAllTasks(true)
	, NumOfThreadsToUse(1)
	, GenType(EAdvancedPlanetGenType::GT_StandBy)
	, QuadsPerFace(1)
	, Radius(IN_Radius)
	, PlanetLayers(IN_PlanetLayers)
	, HasWater(false)
	, OceanLOD(32)
	, AtmosphereLOD(32)
	, MatSlopeFactors(IN_MatSlopeFactors)
	, OceanLevel(0)
	, AtmosphereHeight(0)
{
	// Create Our Thread
	Thread = FRunnableThread::Create(this, TEXT("FAdvancedPlanetMasterWorker"), 8 * 1024, TPri_Normal);
}

// Destructor
FAdvancedPlanetMasterWorker::~FAdvancedPlanetMasterWorker()
{
	// Loop through our worker threads first
	for (int i = 0; i < WorkerThreads.Num(); i++)
	{
		delete WorkerThreads[i];
		WorkerThreads[i] = NULL;
	}

	if (Thread != NULL)
	{
		Thread->Kill(true);
		delete Thread;
		Thread = NULL;
	}
}

// This adds a task to our task pool
void FAdvancedPlanetMasterWorker::AddWork(TArray<FAdvancedPlanetSectionGenTask> Tasks)
{
	GenType = EAdvancedPlanetGenType::GT_SectionData;

	TaskQueue = Tasks;

	// Reset our mesh section list..
	LimitedAccess_MeshSections.Empty();

	HasCompletedAllTasks = false;
}

// This adds a task to our task pool
void FAdvancedPlanetMasterWorker::AllocateMoreThreads(int32 ThreadsToUse)
{
	NumOfThreadsToUse = ThreadsToUse;
}

// This will generate our cube data
void FAdvancedPlanetMasterWorker::GenerateCubeData(int32 IN_QuadsPerFace)
{
	GenType = EAdvancedPlanetGenType::GT_CubeData;
	QuadsPerFace = IN_QuadsPerFace;
	HasCompletedAllTasks = false;
}

// Generate Ocean and Atmosphere
void FAdvancedPlanetMasterWorker::GenerateOceanAtmo(int32 IN_OceanLOD, float IN_OceanLevel, int32 IN_AtmoLOD, float IN_AtmosphereHeight)
{
	GenType = EAdvancedPlanetGenType::GT_OceanAtmoData;

	HasCompletedAllTasks = false;

	HasWater = true;
	OceanLOD = IN_OceanLOD;
	AtmosphereLOD = IN_AtmoLOD;
	OceanLevel = IN_OceanLevel;
	AtmosphereHeight = IN_AtmosphereHeight;
}

// Generate Ocean and Atmosphere
void FAdvancedPlanetMasterWorker::GenerateOceanAtmo(int32 IN_AtmoLOD, float IN_AtmosphereHeight)
{
	GenType = EAdvancedPlanetGenType::GT_OceanAtmoData;

	HasCompletedAllTasks = false;

	HasWater = false;
	AtmosphereLOD = IN_AtmoLOD;
	AtmosphereHeight = IN_AtmosphereHeight;
}

// Stop
void FAdvancedPlanetMasterWorker::Stop() 
{ 
	Stopping = true; 

	// Loop through our worker threads first
	for (int i = 0; i < WorkerThreads.Num(); i++)
		if (WorkerThreads[i])
			WorkerThreads[i]->Stop();
}

// Our Run Function
uint32 FAdvancedPlanetMasterWorker::Run()
{
	//Initial wait before starting
	FPlatformProcess::Sleep(0.03);

	while (!Stopping)
	{
		// First we check if the thread has completed it's operations
		// If it is, we simply continue
		if (HasCompletedAllTasks || GenType == EAdvancedPlanetGenType::GT_StandBy)
		{
			// Prevent thread from using too many resources
			FPlatformProcess::Sleep(0.02);

			// Continue..
			continue;
		}

		// Let's check if we have to generate cube data..
		if (GenType == EAdvancedPlanetGenType::GT_CubeData)
		{
			// Generate Our Cube Quads
			GenerateCubeQuads();

			// The task has been completed..
			HasCompletedAllTasks = true;

			// Continue..
			continue;
		}

		// Let's check if we have to generate ocean or atmosphere data..
		if (GenType == EAdvancedPlanetGenType::GT_OceanAtmoData)
		{
			// Our Quads
			TArray<FAdvancedPlanetQuad> AtmoQuads;
			TArray<FAdvancedPlanetQuad> OceanQuads;

			// Generate Our  Quads
			AtmoQuads = GenerateAtmoOceanQuads();
			if (HasWater)
				OceanQuads = GenerateAtmoOceanQuads();

			// Reset Mesh Sections
			LimitedAccess_MeshSections.Empty();

			// Generate Mesh Sections
			LimitedAccess_MeshSections.Add(GenerateSphereMeshFromQuads(AtmoQuads, AtmosphereLOD, AtmosphereHeight, true));
			if (HasWater)
				LimitedAccess_MeshSections.Add(GenerateSphereMeshFromQuads(OceanQuads, OceanLOD, OceanLevel, false));

			// Regenerate our buffer..
			GenerateProxyMeshBuffer();

			// The task has been completed..
			HasCompletedAllTasks = true;

			// Continue..
			continue;
		}

		// Check if we are generating section meshes now?
		// If not, simply contiue..
		if (GenType != EAdvancedPlanetGenType::GT_SectionData)
			continue;

		// Check if we have any generated quads yet?
		if (CubeQuads.Num() <= 0)
			continue;

		// Next let's see if we need to allocate more threads..
		/* thread count allocation technique */

		// Loop through our worker threads first
		for (int i = WorkerThreads.Num() - 1; i > -1; i--)
		{
			// If the worker class has completed..
			if (WorkerThreads[i]->HasCompleted)
			{
				// Add our mesh section..
				LimitedAccess_MeshSections.Add(WorkerThreads[i]->LimitedAccess_MeshSection);

				// Check if we have a task in queue to assign it
				// Also check if we need to backoff on thread re-use
				if (TaskQueue.Num() > 0 && WorkerThreads.Num() <= NumOfThreadsToUse)
				{
					//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Adding Work to Existing Thread!");

					if (TaskQueue.Last().SectionID >= CubeQuads.Num())
						TaskQueue.Last().SectionID = 0;
					TaskQueue.Last().BaseQuad = CubeQuads[TaskQueue.Last().SectionID];

					WorkerThreads[i]->BeginWork(TaskQueue.Last());
					TaskQueue.Pop();
				}
				else
				{
					// We have no more tasks and this thread has completed
					// Let's remove it..
					WorkerThreads[i]->Stop();

					delete WorkerThreads[i];
					WorkerThreads[i] = NULL;
					WorkerThreads.RemoveAt(i, 1, true);

					//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "Deleted ~");
				}
			}
		}

		// Let's check if we have more tasks to assign and no worker threads open..
		if (TaskQueue.Num() > 0 && WorkerThreads.Num() < NumOfThreadsToUse)
		{
			// Let's now create more threads
			for (int i = WorkerThreads.Num(); i < NumOfThreadsToUse; i++)
			{
				// Check if we have a task in queue to assign it
				if (TaskQueue.Num() > 0)
				{
					//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, "Adding Work to New Thread!");
					WorkerThreads.Add(new FAdvancedPlanetTaskWorker(
						WorkerThreads.Num() + 1,
						Radius,
						PlanetLayers,
						MatSlopeFactors));

					if (TaskQueue.Last().SectionID >= CubeQuads.Num())
						TaskQueue.Last().SectionID = 0;
					TaskQueue.Last().BaseQuad = CubeQuads[TaskQueue.Last().SectionID];

					WorkerThreads[i]->BeginWork(TaskQueue.Last());
					TaskQueue.Pop();
				}
			}
		}

		// All Tasks have been completed!
		if (WorkerThreads.Num() == 0)
		{
			// Regenerate our buffer..
			GenerateProxyMeshBuffer();

			// All tasks completed!
			HasCompletedAllTasks = true;
		}
	}

	return 0;
}

// Generate our Cube Quads
void FAdvancedPlanetMasterWorker::GenerateCubeQuads()
{
	// First let's create a Cube Vert Array
	FVector CubeVerts[8];
	CubeVerts[0] = FVector(-0.5f, 0.5f, 0.5f);
	CubeVerts[1] = FVector(0.5f, 0.5f, 0.5f);
	CubeVerts[2] = FVector(-0.5f, -0.5f, 0.5f);
	CubeVerts[3] = FVector(0.5f, -0.5f, 0.5f);

	CubeVerts[4] = FVector(-0.5f, 0.5f, -0.5f);
	CubeVerts[5] = FVector(0.5f, 0.5f, -0.5f);
	CubeVerts[6] = FVector(-0.5f, -0.5f, -0.5f);
	CubeVerts[7] = FVector(0.5f, -0.5f, -0.5f);

	TArray<FAdvancedPlanetQuad> Face1 =
		GenerateCubeQuadFace(CubeVerts[0], CubeVerts[1], CubeVerts[2], CubeVerts[3]);
	TArray<FAdvancedPlanetQuad> Face2 =
		GenerateCubeQuadFace(CubeVerts[2], CubeVerts[6], CubeVerts[0], CubeVerts[4]);
	TArray<FAdvancedPlanetQuad> Face3 =
		GenerateCubeQuadFace(CubeVerts[0], CubeVerts[4], CubeVerts[1], CubeVerts[5]);
	TArray<FAdvancedPlanetQuad> Face4 =
		GenerateCubeQuadFace(CubeVerts[1], CubeVerts[5], CubeVerts[3], CubeVerts[7]);
	TArray<FAdvancedPlanetQuad> Face5 =
		GenerateCubeQuadFace(CubeVerts[3], CubeVerts[7], CubeVerts[2], CubeVerts[6]);
	TArray<FAdvancedPlanetQuad> Face6 =
		GenerateCubeQuadFace(CubeVerts[6], CubeVerts[7], CubeVerts[4], CubeVerts[5]);

	CubeQuads = Face1;
	CubeQuads.Append(Face2);
	CubeQuads.Append(Face3);
	CubeQuads.Append(Face4);
	CubeQuads.Append(Face5);
	CubeQuads.Append(Face6);
}

// Generate Atmo Ocean Quads
TArray<FAdvancedPlanetQuad> FAdvancedPlanetMasterWorker::GenerateAtmoOceanQuads()
{
	// Our Cube Quad Array
	TArray<FAdvancedPlanetQuad> GeneratedCubeQuads;

	// Saved our QuadsPerFace
	int32 SavedLOD = QuadsPerFace;

	// Set our Quads Per Face
	QuadsPerFace = 1;

	// First let's create a Cube Vert Array
	FVector CubeVerts[8];
	CubeVerts[0] = FVector(-0.5f, 0.5f, 0.5f);
	CubeVerts[1] = FVector(0.5f, 0.5f, 0.5f);
	CubeVerts[2] = FVector(-0.5f, -0.5f, 0.5f);
	CubeVerts[3] = FVector(0.5f, -0.5f, 0.5f);

	CubeVerts[4] = FVector(-0.5f, 0.5f, -0.5f);
	CubeVerts[5] = FVector(0.5f, 0.5f, -0.5f);
	CubeVerts[6] = FVector(-0.5f, -0.5f, -0.5f);
	CubeVerts[7] = FVector(0.5f, -0.5f, -0.5f);

	TArray<FAdvancedPlanetQuad> Face1 =
		GenerateCubeQuadFace(CubeVerts[0], CubeVerts[1], CubeVerts[2], CubeVerts[3]);
	TArray<FAdvancedPlanetQuad> Face2 =
		GenerateCubeQuadFace(CubeVerts[2], CubeVerts[6], CubeVerts[0], CubeVerts[4]);
	TArray<FAdvancedPlanetQuad> Face3 =
		GenerateCubeQuadFace(CubeVerts[0], CubeVerts[4], CubeVerts[1], CubeVerts[5]);
	TArray<FAdvancedPlanetQuad> Face4 =
		GenerateCubeQuadFace(CubeVerts[1], CubeVerts[5], CubeVerts[3], CubeVerts[7]);
	TArray<FAdvancedPlanetQuad> Face5 =
		GenerateCubeQuadFace(CubeVerts[3], CubeVerts[7], CubeVerts[2], CubeVerts[6]);
	TArray<FAdvancedPlanetQuad> Face6 =
		GenerateCubeQuadFace(CubeVerts[6], CubeVerts[7], CubeVerts[4], CubeVerts[5]);

	GeneratedCubeQuads = Face1;
	GeneratedCubeQuads.Append(Face2);
	GeneratedCubeQuads.Append(Face3);
	GeneratedCubeQuads.Append(Face4);
	GeneratedCubeQuads.Append(Face5);
	GeneratedCubeQuads.Append(Face6);

	// Reset
	QuadsPerFace = SavedLOD;

	return GeneratedCubeQuads;
}

// Generate Cube Quad Face
TArray<FAdvancedPlanetQuad> FAdvancedPlanetMasterWorker::GenerateCubeQuadFace(
	FVector Vert1, FVector Vert2, FVector Vert3, FVector Vert4)
{
	// Our Cube Quad Array
	TArray<FAdvancedPlanetQuad> GeneratedCubeQuads;

	// Set up our max
	float VMax = QuadsPerFace;

	// Let's loop our columns
	for (int32 i = 0; i < QuadsPerFace + 1; i++)
	{
		// Calculate our percentage
		float VIMin = (float)i;
		float VIPer = (VIMin / VMax)*1.f;
		float VIPerPlus = ((VIMin + 1.f) / VMax)*1.f;

		// Now let's lerp our vertices to get the required vector
		FVector Vfr = FMath::Lerp(Vert1, Vert2, VIPer);
		FVector Vlr = FMath::Lerp(Vert3, Vert4, VIPer);

		FVector VfrPlus = FMath::Lerp(Vert1, Vert2, VIPerPlus);
		FVector VlrPlus = FMath::Lerp(Vert3, Vert4, VIPerPlus);

		// Now let's loop our rows
		for (int32 n = 0; n < QuadsPerFace + 1; n++)
		{
			float VNMin = (float)n;
			float VNPer = (VNMin / VMax)*1.f;
			float VNPerPlus = ((VNMin + 1.f) / VMax)*1.f;

			if (i >= QuadsPerFace || n >= QuadsPerFace)
				continue;

			FVector Point1 = FMath::Lerp(Vfr, Vlr, VNPer);
			FVector Point2 = FMath::Lerp(VfrPlus, VlrPlus, VNPer);
			FVector Point3 = FMath::Lerp(Vfr, Vlr, VNPerPlus);
			FVector Point4 = FMath::Lerp(VfrPlus, VlrPlus, VNPerPlus);

			FAdvancedPlanetTriangle Tri1 = FAdvancedPlanetTriangle();
			FAdvancedPlanetTriangle Tri2 = FAdvancedPlanetTriangle();

			Tri1.Create(0, 1, 3);
			Tri2.Create(3, 2, 0);

			GeneratedCubeQuads.Add(FAdvancedPlanetQuad());
			GeneratedCubeQuads.Last().Create(Point1, Point2, Point3, Point4, Tri1, Tri2);
		}
	}

	return GeneratedCubeQuads;
}

// Generate Proxy Mesh Buffer
void FAdvancedPlanetMasterWorker::GenerateProxyMeshBuffer()
{
	const int32 NumSections = LimitedAccess_MeshSections.Num();
	LimitedAccess_ProxyMeshSections.Empty();
	LimitedAccess_ProxyMeshSections.AddZeroed(NumSections);
	for (int SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
	{
		FDynMeshSection SrcSection = LimitedAccess_MeshSections[SectionIdx];
		if (SrcSection.ProcIndexBuffer.Num() > 0 && SrcSection.ProcVertexBuffer.Num() > 0)
		{
			FDynMeshProxySection* NewProxySection = new FDynMeshProxySection();

			// Copy data from vertex buffer
			const int32 NumVerts = SrcSection.ProcVertexBuffer.Num();

			// Allocate verts
			NewProxySection->VertexBuffer.Vertices.SetNumUninitialized(NumVerts);

			// Set visibility
			NewProxySection->Visible = SrcSection.bVisible;

			// Copy verts
			for (int VertIdx = 0; VertIdx < NumVerts; VertIdx++)
			{
				FDynMeshVertex ProcVert = SrcSection.ProcVertexBuffer[VertIdx];

				FDynamicMeshVertex& Vert = NewProxySection->VertexBuffer.Vertices[VertIdx];
				Vert.Position = ProcVert.Position;
				Vert.Color = ProcVert.Color;
				Vert.TextureCoordinate = ProcVert.UV0;
				Vert.TangentX = ProcVert.Tangent.TangentX;
				Vert.TangentZ = ProcVert.Normal;
				Vert.TangentZ.Vector.W = ProcVert.Tangent.bFlipTangentY ? 0 : 255;
			}

			// Copy index buffer
			NewProxySection->IndexBuffer.Indices = SrcSection.ProcIndexBuffer;

			// Init vertex factory
			NewProxySection->VertexFactory.Init(&NewProxySection->VertexBuffer);

			// Save ref to new section
			LimitedAccess_ProxyMeshSections[SectionIdx] = NewProxySection;
		}
	}
}

// Generate Sphere Mesh From Quads
FDynMeshSection FAdvancedPlanetMasterWorker::GenerateSphereMeshFromQuads(TArray<FAdvancedPlanetQuad> Quads, int32 LOD, float Height, bool InvertFaces)
{
	int32 GenerationLOD = LOD;
	FDynMeshVertex Vertex;

	FDynMeshSection NewSection = FDynMeshSection();

	float IndexCount = -1;

	for (int32 Qid = 0; Qid < Quads.Num(); Qid++)
	{
		FAdvancedPlanetQuad Quad = Quads[Qid];

		// Set up our fields
		float VIMin, VNMin, VNPer, VIPer = 0;
		float VMax = GenerationLOD;

		// ~~
		// Let's begin by generating the LOD of our planet mesh
		// ~~ 
		// Loop our Rows
		for (int32 i = 0; i < GenerationLOD + 1; i++)
		{
			// Calculate our percentage
			VIMin = (float)i;
			VIPer = (VIMin / VMax)*1.0f;

			// Now let's lerp our vertices to get the required vector
			FVector Vfr = FMath::Lerp(Quad.Vector1, Quad.Vector2, VIPer);
			FVector Vlr = FMath::Lerp(Quad.Vector3, Quad.Vector4, VIPer);

			// Now let's loop our columns
			for (int n = 0; n < GenerationLOD + 1; n++)
			{
				// Calculate our percentage
				VNMin = (float)n;
				VNPer = (VNMin / VMax)*1.0f;

				// Add Vertex
				FVector Vn = FMath::Lerp(Vfr, Vlr, VNPer);

				Vertex = FDynMeshVertex();
				Vertex.Position = Vn;
				Vertex.Position.Normalize();
				Vertex.Position *= (Radius + Height);

				Vertex.Normal = Vertex.Position;
				Vertex.Normal.Normalize();

				Vertex.UV0 = FVector2D(VNPer, VIPer);
				Vertex.Color = FColor(255, 255, 255);
				Vertex.Tangent = FDynMeshTangent();

				NewSection.ProcVertexBuffer.Add(Vertex);

				NewSection.SectionLocalBox += Vertex.Position;

				IndexCount++;

				if (i >= GenerationLOD || n >= GenerationLOD)
					continue;

				if (InvertFaces)
				{
					NewSection.ProcIndexBuffer.Add(IndexCount);
					NewSection.ProcIndexBuffer.Add(IndexCount + 1);
					NewSection.ProcIndexBuffer.Add(IndexCount + (GenerationLOD + 1));

					NewSection.ProcIndexBuffer.Add(IndexCount + (GenerationLOD + 1) + 1);
					NewSection.ProcIndexBuffer.Add(IndexCount + (GenerationLOD + 1));
					NewSection.ProcIndexBuffer.Add(IndexCount + 1);
				}
				else
				{
					NewSection.ProcIndexBuffer.Add(IndexCount + (GenerationLOD + 1));
					NewSection.ProcIndexBuffer.Add(IndexCount + 1);
					NewSection.ProcIndexBuffer.Add(IndexCount);

					NewSection.ProcIndexBuffer.Add(IndexCount + 1);
					NewSection.ProcIndexBuffer.Add(IndexCount + (GenerationLOD + 1));
					NewSection.ProcIndexBuffer.Add(IndexCount + (GenerationLOD + 1) + 1);
				}
			}
		}
	}

	NewSection.bVisible = true;

	return NewSection;
}






/*
~~ Exp Task Worker ~~
*/

// Constructor
FAdvancedPlanetTaskWorker::FAdvancedPlanetTaskWorker(
	int32 ThreadID,
	int32 IN_Radius,
	TArray<FAdvancedPlanetLayer> IN_PlanetLayers,
	FAdvancedPlanetMaterialSlopeFactors IN_MatSlopeFactors)
	: Stopping(false)
	, HasCompleted(true)
	, Radius(IN_Radius)
	, PlanetLayers(IN_PlanetLayers)
	, MatSlopeFactors(IN_MatSlopeFactors)
{
	Thread = FRunnableThread::Create(this, TEXT("FAdvancedPlanetTaskWorker" + ThreadID), 8 * 1024, TPri_Normal);
}

// Destructor
FAdvancedPlanetTaskWorker::~FAdvancedPlanetTaskWorker()
{
	if (Thread != NULL)
	{
		Thread->Kill(true);
		delete Thread;
		Thread = NULL;
	}
}

// Begin Work
void FAdvancedPlanetTaskWorker::BeginWork(FAdvancedPlanetSectionGenTask Task)
{
	WorkingTask = Task;
	HasCompleted = false;
}

// Our Run Function
uint32 FAdvancedPlanetTaskWorker::Run()
{
	//Initial wait before starting
	FPlatformProcess::Sleep(0.03);

	while (!Stopping)
	{
		// First we check if the thread has completed it's operations
		// If it is, we simply continue
		if (HasCompleted)
		{
			// Prevent thread from using too many resources
			FPlatformProcess::Sleep(0.02);

			// Continue..
			continue;
		}

		// Do subdivision
		SubdivideQuad();

		// Compute Terrain
		ComputeTerrain();

		// Update Has Completed Trigger
		HasCompleted = true;
	}

	return 0;
}

// Subdivide Our Meshes
void FAdvancedPlanetTaskWorker::SubdivideQuad() 
{
	int32 GenerationLOD = WorkingTask.LOD;
	FDynMeshVertex Vertex;

	FDynMeshSection NewSection = FDynMeshSection();
	FAdvancedPlanetQuad Quad = WorkingTask.BaseQuad;

	float IndexCount = -1;

	// Set up our fields
	float VIMin, VNMin, VNPer, VIPer = 0;
	float VMax = GenerationLOD;

	// ~~
	// Let's begin by generating the LOD of our planet mesh
	// ~~ 
	// Loop our Rows
	for (int32 i = 0; i < GenerationLOD + 1; i++)
	{
		// Calculate our percentage
		VIMin = (float)i;
		VIPer = (VIMin / VMax)*1.0f;

		// Now let's lerp our vertices to get the required vector
		FVector Vfr = FMath::Lerp(Quad.Vector1, Quad.Vector2, VIPer);
		FVector Vlr = FMath::Lerp(Quad.Vector3, Quad.Vector4, VIPer);

		// Now let's loop our columns
		for (int n = 0; n < GenerationLOD + 1; n++)
		{
			// Calculate our percentage
			VNMin = (float)n;
			VNPer = (VNMin / VMax)*1.0f;

			// Add Vertex
			FVector Vn = FMath::Lerp(Vfr, Vlr, VNPer);

			Vertex = FDynMeshVertex();
			Vertex.Position = Vn;
			Vertex.Position.Normalize();
			Vertex.Position *= Radius;

			Vertex.Normal = Vertex.Position;
			Vertex.Normal.Normalize();

			Vertex.UV0 = FVector2D(VNPer, VIPer);
			Vertex.Color = CreateVertexColorMap(Vertex.Position.Size() - Radius);
			Vertex.Tangent = FDynMeshTangent();

			NewSection.ProcVertexBuffer.Add(Vertex);

			if (PlanetLayers.Num() <= 0)
				NewSection.SectionLocalBox += Vertex.Position;

			IndexCount++;

			if (i >= GenerationLOD || n >= GenerationLOD)
				continue;

			NewSection.ProcIndexBuffer.Add(IndexCount + (GenerationLOD + 1));
			NewSection.ProcIndexBuffer.Add(IndexCount + 1);
			NewSection.ProcIndexBuffer.Add(IndexCount);

			NewSection.ProcIndexBuffer.Add(IndexCount + 1);
			NewSection.ProcIndexBuffer.Add(IndexCount + (GenerationLOD + 1));
			NewSection.ProcIndexBuffer.Add(IndexCount + (GenerationLOD + 1) + 1);
		}
	}

	NewSection.bVisible = false;
	NewSection.bEnableCollision = true;
	LimitedAccess_MeshSection = NewSection;
}

// Compute Terrain
void FAdvancedPlanetTaskWorker::ComputeTerrain()
{
	// Check if we have layers?
	if (PlanetLayers.Num() <= 0)
		return;

	// grab our Base Vertices
	TArray<FDynMeshVertex> BaseVerts = LimitedAccess_MeshSection.ProcVertexBuffer;

	// Loop our layers
	for (int TLind = 0; TLind < PlanetLayers.Num(); TLind++)
	{
		// Loop through our vertices
		for (int Vind = 0; Vind < LimitedAccess_MeshSection.ProcVertexBuffer.Num(); Vind++)
		{
			FDynMeshVertex& WorkingVertex = LimitedAccess_MeshSection.ProcVertexBuffer[Vind];
			FDynMeshVertex BaseVertex = BaseVerts[Vind];

			// Now let's calculate our terrain noise
			float TerrainNoise =
				50 * octave_noise_3d(
				PlanetLayers[TLind].Amplitude,			// Amplitude
				PlanetLayers[TLind].Octaves,			// Octaves
				PlanetLayers[TLind].Persistence,		// Persistence
				PlanetLayers[TLind].Frequency,			// Frequency
				WorkingVertex.Position.X,
				WorkingVertex.Position.Y,
				WorkingVertex.Position.Z);

			// Grab our vertex terrain distance factor
			float FactorDist = ((TerrainNoise + 50.0f) / 100.0f) * PlanetLayers[TLind].MaxTerrainHeight;

			// Check if this is our first layer again..
			// If so let's simply set our vertex position and calculate our vertex color map
			// No need to blend any layers..
			if (TLind == 0)
			{
				// Set our new vertex position
				WorkingVertex.Position += WorkingVertex.Normal * FactorDist;

				LimitedAccess_MeshSection.SectionLocalBox += WorkingVertex.Position;

				// Set our Vertex Color Map
				WorkingVertex.Color = CreateVertexColorMap(WorkingVertex.Position.Size() - Radius);

				continue;
			}

			// Now let's do our layering!
			// First let's check if the noise is within our threshold..
			if (FactorDist < PlanetLayers[TLind].BlendFactors.HeightMin ||
				FactorDist > PlanetLayers[TLind].BlendFactors.HeightMax)
				continue;

			// Addition
			// ~ Vector = Vector + TerrainNoise
			if (PlanetLayers[TLind].CombinationType == EAdvancedPlanetLayerCombinationType::LC_Add)
				WorkingVertex.Position += (WorkingVertex.Normal * FactorDist);

			// Subtraction
			// ~ Vector = Vector - TerrainNoise
			if (PlanetLayers[TLind].CombinationType == EAdvancedPlanetLayerCombinationType::LC_Subtract)
				WorkingVertex.Position -= (WorkingVertex.Normal * FactorDist);

			// AddDifference
			// ~ Vector = Vector + Length Of ( BaseVector - Vector )
			if (PlanetLayers[TLind].CombinationType == EAdvancedPlanetLayerCombinationType::LC_AddDifference)
			{
				float DiffSize = (BaseVertex.Position - WorkingVertex.Position).Size();
				WorkingVertex.Position += (WorkingVertex.Normal * DiffSize);
			}

			// SubDifference
			// ~ Vector = Vector - Length Of ( BaseVector - Vector )
			if (PlanetLayers[TLind].CombinationType == EAdvancedPlanetLayerCombinationType::LC_SubDifference)
			{
				float DiffSize = (BaseVertex.Position - WorkingVertex.Position).Size();
				WorkingVertex.Position -= (WorkingVertex.Normal * DiffSize);
			}

			// Blend
			// ~ Vector = Lerp ( Vector, BaseVector )
			if (PlanetLayers[TLind].CombinationType == EAdvancedPlanetLayerCombinationType::LC_Blend)
			{
				FVector LVec = FMath::Lerp(
					WorkingVertex.Position,
					BaseVertex.Position,
					PlanetLayers[TLind].BlendFactors.BlendAlpha);
				WorkingVertex.Position = LVec;

			}

			// Multiply
			// ~ Vector = Vector * TerrainNoise
			if (PlanetLayers[TLind].CombinationType == EAdvancedPlanetLayerCombinationType::LC_Multiply)
				WorkingVertex.Position *= (WorkingVertex.Normal * FactorDist);

			// Negative
			// ~ Vector = BaseVector
			if (PlanetLayers[TLind].CombinationType == EAdvancedPlanetLayerCombinationType::LC_Negative)
				WorkingVertex.Position = BaseVertex.Position;

			LimitedAccess_MeshSection.SectionLocalBox += WorkingVertex.Position;

			// Setup our Vertex Color Map
			WorkingVertex.Color =
				CreateVertexColorMap(WorkingVertex.Position.Size() - Radius);
		}
	}
}

// Setup our Vertice Color Map
FColor FAdvancedPlanetTaskWorker::CreateVertexColorMap(float LayerHeight)
{
	float HighSlopeLayer = FMath::Clamp(LayerHeight, MatSlopeFactors.HighSlopeMin, MatSlopeFactors.HighSlopeMax);
	float MidSlopeLayer = FMath::Clamp(LayerHeight, MatSlopeFactors.MidSlopeMin, MatSlopeFactors.MidSlopeMax);
	float LowSlopeLayer = FMath::Clamp(LayerHeight, MatSlopeFactors.LowSlopeMin, MatSlopeFactors.LowSlopeMax);

	HighSlopeLayer = ((HighSlopeLayer - MatSlopeFactors.HighSlopeMin) / (MatSlopeFactors.HighSlopeMax - MatSlopeFactors.HighSlopeMin)) * 255.f;
	MidSlopeLayer = ((MidSlopeLayer - MatSlopeFactors.MidSlopeMin) / (MatSlopeFactors.MidSlopeMax - MatSlopeFactors.MidSlopeMin)) * 255.f;
	LowSlopeLayer = ((LowSlopeLayer - MatSlopeFactors.LowSlopeMin) / (MatSlopeFactors.LowSlopeMax - MatSlopeFactors.LowSlopeMin)) * 255.f;

	HighSlopeLayer = FMath::Clamp(HighSlopeLayer, 0.f, 255.f);
	MidSlopeLayer = FMath::Clamp(MidSlopeLayer, 0.f, 255.f);
	LowSlopeLayer = FMath::Clamp(LowSlopeLayer, 0.f, 255.f);

	/*GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "------------------------");
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Cyan, FString::SanitizeFloat(LayerHeight));
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, FString::SanitizeFloat(HighLayer));
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::SanitizeFloat(MidLayer));
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::SanitizeFloat(LowLayer));
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "------------------------");*/

	return FColor(HighSlopeLayer, MidSlopeLayer, LowSlopeLayer);
}