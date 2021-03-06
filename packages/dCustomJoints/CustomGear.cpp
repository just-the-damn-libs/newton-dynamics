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



// CustomGear.cpp: implementation of the CustomGear class.
//
//////////////////////////////////////////////////////////////////////
#include "CustomJointLibraryStdAfx.h"
#include "CustomGear.h"

//dInitRtti(CustomGear);
IMPLEMENT_CUSTON_JOINT(CustomGear);
//IMPLEMENT_CUSTON_JOINT(CustomGearAndSlide);
IMPLEMENT_CUSTON_JOINT(CustomSatelliteGear);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CustomGear::CustomGear(dFloat gearRatio, const dVector& childPin, const dVector& parentPin, NewtonBody* const child, NewtonBody* const parent)
	:CustomJoint(1, child, parent)
	,m_gearRatio (gearRatio)
{
	dMatrix dommyMatrix;
	// calculate the local matrix for body body0
	dMatrix pinAndPivot0 (dGrammSchmidt(childPin));

	CalculateLocalMatrix (pinAndPivot0, m_localMatrix0, dommyMatrix);
	m_localMatrix0.m_posit = dVector (0.0f, 0.0f, 0.0f, 1.0f);

	// calculate the local matrix for body body1  
	dMatrix pinAndPivot1 (dGrammSchmidt(parentPin));
	CalculateLocalMatrix (pinAndPivot1, dommyMatrix, m_localMatrix1);
	m_localMatrix1.m_posit = dVector (0.0f, 0.0f, 0.0f, 1.0f);
}


CustomGear::~CustomGear()
{
}

CustomGear::CustomGear (NewtonBody* const child, NewtonBody* const parent, NewtonDeserializeCallback callback, void* const userData)
	:CustomJoint(child, parent, callback, userData)
{
	callback (userData, &m_gearRatio, sizeof (dFloat));
}

void CustomGear::Serialize (NewtonSerializeCallback callback, void* const userData) const
{
	CustomJoint::Serialize (callback, userData);
	callback (userData, &m_gearRatio, sizeof (dFloat));
}

void CustomGear::SubmitConstraints (dFloat timestep, int threadIndex)
{
	dMatrix matrix0;
	dMatrix matrix1;
	dVector omega0(0.0f);
	dVector omega1(0.0f);
	dFloat jacobian0[6];
	dFloat jacobian1[6];

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix (matrix0, matrix1);

	// calculate the angular velocity for both bodies
	dVector dir0 = matrix0.m_front.Scale (m_gearRatio);
	const dVector& dir1 = matrix1.m_front;

	jacobian0[0] = 0.0f;
	jacobian0[1] = 0.0f;
	jacobian0[2] = 0.0f;
	jacobian0[3] = dir0.m_x;
	jacobian0[4] = dir0.m_y;
	jacobian0[5] = dir0.m_z;

	jacobian1[0] = 0.0f;
	jacobian1[1] = 0.0f;
	jacobian1[2] = 0.0f;
	jacobian1[3] = dir1.m_x;
	jacobian1[4] = dir1.m_y;
	jacobian1[5] = dir1.m_z;

	NewtonBodyGetOmega(m_body0, &omega0[0]);
	NewtonBodyGetOmega(m_body1, &omega1[0]);

	dFloat w0 = omega0.DotProduct3(dir0);
	dFloat w1 = omega1.DotProduct3(dir1);
	dFloat relOmega = w0 + w1;
	dFloat invTimestep = (timestep > 0.0f) ? 1.0f / timestep : 1.0f;
	dFloat relAccel = -0.5f * relOmega * invTimestep;
	NewtonUserJointAddGeneralRow(m_joint, jacobian0, jacobian1);
	NewtonUserJointSetRowAcceleration(m_joint, relAccel);
}

void CustomGear::GetInfo (NewtonJointRecord* const info) const
{
	strcpy (info->m_descriptionType, GetTypeName());

	info->m_extraParameters[0] = m_gearRatio;

	info->m_attachBody_0 = m_body0;
	info->m_attachBody_1 = m_body1;

	info->m_minLinearDof[0] = -D_CUSTOM_LARGE_VALUE;
	info->m_maxLinearDof[0] = D_CUSTOM_LARGE_VALUE;

	info->m_minLinearDof[1] = -D_CUSTOM_LARGE_VALUE;
	info->m_maxLinearDof[1] = D_CUSTOM_LARGE_VALUE;

	info->m_minLinearDof[2] = -D_CUSTOM_LARGE_VALUE;
	info->m_maxLinearDof[2] = D_CUSTOM_LARGE_VALUE;

	info->m_minAngularDof[0] = -D_CUSTOM_LARGE_VALUE;
	info->m_maxAngularDof[0] =  D_CUSTOM_LARGE_VALUE;

	info->m_minAngularDof[1] = -D_CUSTOM_LARGE_VALUE;;
	info->m_maxAngularDof[1] =  D_CUSTOM_LARGE_VALUE;

	info->m_minAngularDof[2] = -D_CUSTOM_LARGE_VALUE;;
	info->m_maxAngularDof[2] =  D_CUSTOM_LARGE_VALUE;

	memcpy (info->m_attachmenMatrix_0, &m_localMatrix0, sizeof (dMatrix));
	memcpy (info->m_attachmenMatrix_1, &m_localMatrix1, sizeof (dMatrix));
}


CustomSatelliteGear::CustomSatelliteGear(dFloat gearRatio, const dVector& childPin, const dVector& parentPin, const dVector& referencePin, NewtonBody* const child, NewtonBody* const parent, NewtonBody* const parentReference)
	:CustomGear(gearRatio, childPin, parentPin, child, parent)
	,m_parentReference(parentReference)
{
	dMatrix referenceMatrix;
	NewtonBodyGetMatrix (m_parentReference, &referenceMatrix[0][0]);
	m_pintOnReference = referenceMatrix.UnrotateVector(referencePin);
}

CustomSatelliteGear::CustomSatelliteGear (NewtonBody* const child, NewtonBody* const parent, NewtonDeserializeCallback callback, void* const userData)
	:CustomGear(child, parent, callback, userData)
{
	// remember to get referenceBody from the world 
	dAssert (0);
}

void CustomSatelliteGear::Serialize (NewtonSerializeCallback callback, void* const userData) const
{
	CustomGear::Serialize (callback,userData);
	// remember to save information to later load the reference body form the world
	dAssert(0);
}

void CustomSatelliteGear::SubmitConstraints(dFloat timestep, int threadIndex)
{
	dMatrix matrix0;
	dMatrix matrix1;
	dVector omega0(0.0f);
	dVector omega1(0.0f);
	dFloat jacobian0[6];
	dFloat jacobian1[6];

	// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix(matrix0, matrix1);

	// calculate the angular velocity for both bodies
	dVector dir0 (matrix0.m_front.Scale(m_gearRatio));
	dVector dir2 (matrix1.m_front);

	dMatrix referenceMatrix;
	NewtonBodyGetMatrix(m_parentReference, &referenceMatrix[0][0]);
	dVector dir3 (referenceMatrix.RotateVector(m_pintOnReference));
	dVector dir1 (dir2 + dir3);

	jacobian0[0] = 0.0f;
	jacobian0[1] = 0.0f;
	jacobian0[2] = 0.0f;
	jacobian0[3] = dir0.m_x;
	jacobian0[4] = dir0.m_y;
	jacobian0[5] = dir0.m_z;

	jacobian1[0] = 0.0f;
	jacobian1[1] = 0.0f;
	jacobian1[2] = 0.0f;
	jacobian1[3] = dir1.m_x;
	jacobian1[4] = dir1.m_y;
	jacobian1[5] = dir1.m_z;

	NewtonBodyGetOmega(m_body0, &omega0[0]);
	NewtonBodyGetOmega(m_body1, &omega1[0]);

	dFloat w0 = omega0.DotProduct3(dir0);
	dFloat w1 = omega1.DotProduct3(dir1);

	dFloat relOmega = w0 + w1;
	dFloat invTimestep = (timestep > 0.0f) ? 1.0f / timestep : 1.0f;
	dFloat relAccel = -0.5f * relOmega * invTimestep;
	NewtonUserJointAddGeneralRow(m_joint, jacobian0, jacobian1);
	NewtonUserJointSetRowAcceleration(m_joint, relAccel);
}


