/* Copyright (c) <2009> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#include <toolbox_stdafx.h>
#include "SkyBox.h"
#include "DemoMesh.h"
#include "DemoCamera.h"
#include "NewtonDemos.h"
#include "PhysicsUtils.h"
#include "TargaToOpenGl.h"
#include "DemoEntityManager.h"
#include "CustomBallAndSocket.h"
#include "DebugDisplay.h"
#include "CustomHinge.h"
#include "CustomHingeActuator.h"
#include "CustomSliderActuator.h"
#include "HeightFieldPrimitive.h"
#include "CustomArcticulatedTransformManager.h"


struct ARTICULATED_VEHICLE_DEFINITION
{
	char m_boneName[32];
	char m_shapeTypeName[32];
	dFloat m_mass;
	char m_articulationName[32];
};


static ARTICULATED_VEHICLE_DEFINITION forkliftDefinition[] =
{
	{"body",		"convexHull",			600.0f},
	{"fr_tire",		"tireShape",			 40.0f, "frontTire"},
	{"fl_tire",		"tireShape",			 40.0f, "frontTire"},
	{"rr_tire",		"tireShape",			 40.0f, "rearTire"},
	{"rl_tire",		"tireShape",			 40.0f, "rearTire"},
	{"lift_1",		"convexHull",			 50.0f, "hingeActuator"},
	{"lift_2",		"convexHull",			 50.0f, "liftActuator"},
	{"lift_3",		"convexHull",			 50.0f, "liftActuator"},
	{"lift_4",		"convexHull",			 50.0f, "liftActuator"},
	{"left_teeth",  "convexHullAggregate",	 50.0f, "paletteActuator"},
	{"right_teeth", "convexHullAggregate",	 50.0f, "paletteActuator"},
};

class ArticulatedEntityModel: public DemoEntity
{
	public:
	ArticulatedEntityModel(DemoEntityManager* const scene, const char* const name)
		:DemoEntity(GetIdentityMatrix(), NULL)
		,m_rearTiresCount(0)
		,m_frontTiresCount(0)
		,m_liftActuatorsCount(0)
		,m_paletteActuatorsCount(0)
	{
		LoadNGD_mesh (name, scene->GetNewton());
	}

	ArticulatedEntityModel (const ArticulatedEntityModel& copy)
		:DemoEntity(copy)
		,m_rearTiresCount(0)
		,m_frontTiresCount(0)
		,m_liftActuatorsCount(0)
		,m_paletteActuatorsCount(0)
	{
	}

	DemoEntity* CreateClone() const
	{
		return new ArticulatedEntityModel(*this);
	}


	void LinkFrontTire (NewtonBody* const chassis, NewtonBody* const tire)
	{
		dMatrix tireMatrix;
		dMatrix chassisMatrix;

		// calculate the tire location matrix
		NewtonBodyGetMatrix(tire, &tireMatrix[0][0]);
		NewtonBodyGetMatrix(chassis, &chassisMatrix[0][0]);

		chassisMatrix = dYawMatrix(90.0f * 3.141592f / 180.0f) * chassisMatrix;
		chassisMatrix.m_posit = tireMatrix.m_posit;

		m_frontTireJoints[m_frontTiresCount] = new CustomHinge (&chassisMatrix[0][0], tire, chassis);
		m_fronTires[m_frontTiresCount] = tire;
		m_frontTiresCount ++;
	}

	void LinkRearTire (NewtonBody* const chassis, NewtonBody* const tire)
	{
		dMatrix tireMatrix;
		dMatrix chassisMatrix;

		// calculate the tire location matrix
		NewtonBodyGetMatrix(tire, &tireMatrix[0][0]);
		NewtonBodyGetMatrix(chassis, &chassisMatrix[0][0]);

		chassisMatrix = dYawMatrix(90.0f * 3.141592f / 180.0f) * chassisMatrix;
		chassisMatrix.m_posit = tireMatrix.m_posit;

		// for now use hinges
		m_rearTireJoints[m_frontTiresCount] = new CustomHinge (&chassisMatrix[0][0], tire, chassis);
		m_rearTiresCount ++;
	}

	void LinkHingeActuator (NewtonBody* const parent, NewtonBody* const child)
	{
		dMatrix baseMatrix;
		NewtonBodyGetMatrix (child, &baseMatrix[0][0]);

		dFloat angleLimit = 20.0f * 3.141592f / 180.0f;
		dFloat angularRate = 30.0f * 3.141592f / 180.0f;
		m_angularActuator = new CustomHingeActuator (&baseMatrix[0][0], angularRate, -angleLimit, angleLimit, child, parent);
//m_angularActuator->SetTargetAngle(angleLimit);
	}

	void LinkLiftActuator (NewtonBody* const parent, NewtonBody* const child)
	{
		dMatrix baseMatrix;
		NewtonBodyGetMatrix (child, &baseMatrix[0][0]);

		dFloat minLimit = -0.25f;
		dFloat maxLimit = 1.5f;
		dFloat linearRate = 0.3f;
		m_liftJoints[m_liftActuatorsCount] = new CustomSliderActuator (&baseMatrix[0][0], linearRate, minLimit, maxLimit, child, parent);

//m_liftJoints[m_liftActuatorsCount]->SetTargetPosit(maxLimit);
		m_liftActuatorsCount ++;
	}

	void LinkPaletteActuator (NewtonBody* const parent, NewtonBody* const child)
	{
		dMatrix baseMatrix;
		NewtonBodyGetMatrix (child, &baseMatrix[0][0]);

		dFloat minLimit = -0.25f;
		dFloat maxLimit = 0.2f;
		dFloat linearRate = 0.25f;
		m_paletteJoints[m_paletteActuatorsCount] = new CustomSliderActuator (&baseMatrix[0][0], linearRate, minLimit, maxLimit, child, parent);
//m_paletteJoints[m_paletteActuatorsCount]->SetTargetPosit(minLimit);
		m_paletteActuatorsCount ++;
	}


	int m_rearTiresCount;
	int m_frontTiresCount;
	int m_liftActuatorsCount;
	int m_paletteActuatorsCount;
		
	NewtonBody* m_fronTires[2];
	
	CustomHinge* m_rearTireJoints[2];
	CustomHinge* m_frontTireJoints[2];
	CustomSliderActuator* m_liftJoints[3];
	CustomSliderActuator* m_paletteJoints[3];
	
	CustomHingeActuator* m_angularActuator;
};

class ArticulatedVehicleManagerManager: public CustomArticulaledTransformManager
{
	public:
	ArticulatedVehicleManagerManager (DemoEntityManager* const scene)
		:CustomArticulaledTransformManager (scene->GetNewton())
	{
		// create a material for early collision culling
		m_material = NewtonMaterialCreateGroupID(scene->GetNewton());
		NewtonMaterialSetCollisionCallback (scene->GetNewton(), m_material, m_material, this, OnBoneAABBOverlap, NULL);
	}

	virtual void OnPreUpdate (CustomArticulatedTransformController* const constroller, dFloat timestep, int threadIndex) const
	{
	}

	static int OnBoneAABBOverlap (const NewtonMaterial* const material, const NewtonBody* const body0, const NewtonBody* const body1, int threadIndex)
	{
		NewtonCollision* const collision0 = NewtonBodyGetCollision(body0);
		NewtonCollision* const collision1 = NewtonBodyGetCollision(body1);
		CustomArticulatedTransformController::dSkeletonBone* const bone0 = (CustomArticulatedTransformController::dSkeletonBone*)NewtonCollisionGetUserData (collision0);
		CustomArticulatedTransformController::dSkeletonBone* const bone1 = (CustomArticulatedTransformController::dSkeletonBone*)NewtonCollisionGetUserData (collision1);
		dAssert (bone0);
		dAssert (bone1);
		if (bone0->m_myController == bone1->m_myController) {
			return bone0->m_myController->SelfCollisionTest (bone0, bone1) ? 1 : 0;
		}
		return 1;
	}

	virtual void OnUpdateTransform (const CustomArticulatedTransformController::dSkeletonBone* const bone, const dMatrix& localMatrix) const
	{
		DemoEntity* const ent = (DemoEntity*) NewtonBodyGetUserData(bone->m_body);
		DemoEntityManager* const scene = (DemoEntityManager*) NewtonWorldGetUserData(NewtonBodyGetWorld(bone->m_body));

		dQuaternion rot (localMatrix);
		ent->SetMatrix (*scene, rot, localMatrix.m_posit);
	}

	NewtonCollision* MakeTireShape (DemoEntity* const bodyPart) const
	{
		

		dFloat radius = 0.0f;
		dFloat maxWidth = 0.0f;
		dFloat minWidth = 0.0f;

		DemoMesh* const mesh = bodyPart->GetMesh();
		dFloat* const array = mesh->m_vertex;
		for (int i = 0; i < mesh->m_vertexCount; i ++) {
			maxWidth = dMax (array[i * 3 + 0], maxWidth);
			minWidth = dMin (array[i * 3 + 0], minWidth);
			radius = dMax (array[i * 3 + 1], radius);
		}
		dFloat width = maxWidth - minWidth;
		radius -= width * 0.5f;
		return NewtonCreateChamferCylinder (GetWorld(), radius, width, 0, NULL);
	}


	NewtonCollision* MakeConvexHull(DemoEntity* const bodyPart) const
	{
		dVector points[1024 * 16];

		DemoMesh* const mesh = bodyPart->GetMesh();
		dAssert (mesh->m_vertexCount && (mesh->m_vertexCount < int (sizeof (points)/ sizeof (points[0]))));

		// go over the vertex array and find and collect all vertices's weighted by this bone.
		dFloat* const array = mesh->m_vertex;
		for (int i = 0; i < mesh->m_vertexCount; i ++) {
			points[i].m_x = array[i * 3 + 0];
			points[i].m_y = array[i * 3 + 1];
			points[i].m_z = array[i * 3 + 2];
		}
		return NewtonCreateConvexHull (GetWorld(), mesh->m_vertexCount, &points[0].m_x, sizeof (dVector), 1.0e-3f, 0, NULL);
	}

	NewtonCollision* MakeConvexHullAggregate(DemoEntity* const bodyPart) const
	{
		NewtonMesh* const mesh = bodyPart->GetMesh()->CreateNewtonMesh (GetWorld());
		NewtonMesh* const convexApproximation = NewtonMeshApproximateConvexDecomposition (mesh, 0.01f, 0.2f, 32, 100, NULL);
		
		NewtonCollision* const compound = NewtonCreateCompoundCollisionFromMesh (GetWorld(), convexApproximation, 0.001f, 0, 0);

		NewtonMeshDestroy(convexApproximation);
		NewtonMeshDestroy(mesh);
		return compound;
	}

	NewtonBody* CreateBodyPart (DemoEntity* const bodyPart, const ARTICULATED_VEHICLE_DEFINITION& definition) 
	{
		NewtonCollision* shape = NULL;
		if (!strcmp (definition.m_shapeTypeName, "tireShape")) {
			shape = MakeTireShape(bodyPart);
		} else if (!strcmp (definition.m_shapeTypeName, "convexHull")) {
			shape = MakeConvexHull(bodyPart);
		} else if (!strcmp (definition.m_shapeTypeName, "convexHullAggregate")) {
			shape = MakeConvexHullAggregate(bodyPart);
		} else {
			dAssert (0);
		}

		// calculate the bone matrix
		dMatrix matrix (bodyPart->CalculateGlobalMatrix());

		NewtonWorld* const world = GetWorld();

		// create the rigid body that will make this bone
		NewtonBody* const bone = NewtonCreateDynamicBody (world, shape, &matrix[0][0]);

		// calculate the moment of inertia and the relative center of mass of the solid
		NewtonBodySetMassProperties (bone, definition.m_mass, shape);

		// save the user data with the bone body (usually the visual geometry)
		NewtonBodySetUserData(bone, bodyPart);

		// assign the material for early collision culling
		NewtonBodySetMaterialGroupID (bone, m_material);

		// set the bod part force and torque call back to the gravity force, skip the transform callback
		NewtonBodySetForceAndTorqueCallback (bone, PhysicsApplyGravityForce);

		// destroy the collision helper shape 
		NewtonDestroyCollision (shape);
		return bone;
	}



	void ConnectBodyPart (ArticulatedEntityModel* const vehicleModel, NewtonBody* const parent, NewtonBody* const child, const dString& jointArticulation)
	{
		if (jointArticulation == "") {
			// this is the root body do nothing

		} else if (jointArticulation == "frontTire") {
			vehicleModel->LinkFrontTire (parent, child);
		} else if (jointArticulation == "rearTire") {
			vehicleModel->LinkRearTire (parent, child);
		} else if (jointArticulation == "hingeActuator") {
			vehicleModel->LinkHingeActuator (parent, child);
		} else if (jointArticulation == "liftActuator") {
			vehicleModel->LinkLiftActuator (parent, child);
		} else if (jointArticulation == "paletteActuator") {
			vehicleModel->LinkPaletteActuator (parent, child);
		} else {
			dAssert (0);
		}
	}


	void CreateForklift (const dMatrix& location, const DemoEntity* const model, int bodyPartsCount, ARTICULATED_VEHICLE_DEFINITION* const definition)
	{
		NewtonWorld* const world = GetWorld(); 
		DemoEntityManager* const scene = (DemoEntityManager*) NewtonWorldGetUserData(world);

		// make a clone of the mesh 
		ArticulatedEntityModel* const vehicleModel = (ArticulatedEntityModel*) model->CreateClone();
		scene->Append(vehicleModel);

		CustomArticulatedTransformController* const controller = CreateTransformController (vehicleModel, true);

		DemoEntity* const rootEntity = (DemoEntity*) vehicleModel->Find (definition[0].m_boneName);
		NewtonBody* const rootBody = CreateBodyPart (rootEntity, definition[0]);

		// add the root bone to the articulation manager
		CustomArticulatedTransformController::dSkeletonBone* const bone = controller->AddBone (rootBody, GetIdentityMatrix());
		// save the bone as the shape use data for self collision test
		NewtonCollisionSetUserData (NewtonBodyGetCollision(rootBody), bone);

		// walk down the model hierarchy an add all the components 
		int stackIndex = 0;
		DemoEntity* childEntities[32];
		CustomArticulatedTransformController::dSkeletonBone* parentBones[32];
		for (DemoEntity* child = rootEntity->GetChild(); child; child = child->GetSibling()) {
			parentBones[stackIndex] = bone;
			childEntities[stackIndex] = child;
			stackIndex ++;
		}

		while (stackIndex) {
			stackIndex --;
			DemoEntity* const entity = childEntities[stackIndex];
			CustomArticulatedTransformController::dSkeletonBone* parentBone = parentBones[stackIndex];

			const char* const name = entity->GetName().GetStr();
			for (int i = 0; i < bodyPartsCount; i ++) {
				if (!strcmp (definition[i].m_boneName, name)) {
					NewtonBody* const bone = CreateBodyPart (entity, definition[i]);

					// connect this body part to its parent with a vehicle joint
					ConnectBodyPart (vehicleModel, parentBone->m_body, bone, definition[i].m_articulationName);

					dMatrix bindMatrix (entity->GetParent()->CalculateGlobalMatrix ((DemoEntity*)NewtonBodyGetUserData (parentBone->m_body)).Inverse());
					parentBone = controller->AddBone (bone, bindMatrix, parentBone);

					// save the controller as the collision user data, for collision culling
					NewtonCollisionSetUserData (NewtonBodyGetCollision(bone), parentBone);
					break;
				}
			}

			for (DemoEntity* child = entity->GetChild(); child; child = child->GetSibling()) {
				parentBones[stackIndex] = parentBone;
				childEntities[stackIndex] = child;
				stackIndex ++;
			}
		}

		// disable self collision between all body parts
		controller->DisableAllSelfCollision();
	}

	int m_material;
};


void ArticulatedJoints (DemoEntityManager* const scene)
{
	// load the sky box
	scene->CreateSkyBox();
	CreateLevelMesh (scene, "flatPlane.ngd", true);
	//CreateHeightFieldTerrain (scene, 9, 8.0f, 1.5f, 0.2f, 200.0f, -50.0f);

	// load a the mesh of the articulate vehicle
	ArticulatedEntityModel forkliffModel(scene, "forklift.ngd");

	//  create a skeletal transform controller for controlling rag doll
	ArticulatedVehicleManagerManager* const manager = new ArticulatedVehicleManagerManager (scene);

	NewtonWorld* const world = scene->GetNewton();
	dMatrix matrix (GetIdentityMatrix());

	dVector origin (FindFloor (world, dVector (-10.0f, 50.0f, 0.0f, 1.0f), 2.0f * 50.0f));

	int count = 1;
	for (int x = 0; x < count; x ++) {
		for (int z = 0; z < count; z ++) {
			dVector p (origin + dVector ((x - count / 2) * 3.0f - count / 2, 0.0f, (z - count / 2) * 3.0f, 0.0f));
			matrix.m_posit = FindFloor (world, p, 100.0f);
			matrix.m_posit.m_y += 3.0f;
			manager->CreateForklift (matrix, &forkliffModel, sizeof(forkliftDefinition) / sizeof (forkliftDefinition[0]), forkliftDefinition);
		}
	}
	
	origin.m_x -= 5.0f;
	origin.m_y += 5.0f;
	dQuaternion rot;
	scene->SetCameraMatrix(rot, origin);
}


