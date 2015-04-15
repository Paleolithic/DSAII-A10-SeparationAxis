#include "BoundingBoxManagerSingleton.h"

//  BoundingBoxManagerSingleton
BoundingBoxManagerSingleton* BoundingBoxManagerSingleton::m_pInstance = nullptr;
void BoundingBoxManagerSingleton::Init(void)
{
	m_nBoxs = 0;
}
void BoundingBoxManagerSingleton::Release(void)
{
	//Clean the list of Boxs
	for(int n = 0; n < m_nBoxs; n++)
	{
		//Make sure to release the memory of the pointers
		if(m_lBox[n] != nullptr)
		{
			delete m_lBox[n];
			m_lBox[n] = nullptr;
		}
	}
	m_lBox.clear();
	m_lMatrix.clear();
	m_lColor.clear();
	m_nBoxs = 0;
}
BoundingBoxManagerSingleton* BoundingBoxManagerSingleton::GetInstance()
{
	if(m_pInstance == nullptr)
	{
		m_pInstance = new BoundingBoxManagerSingleton();
	}
	return m_pInstance;
}
void BoundingBoxManagerSingleton::ReleaseInstance()
{
	if(m_pInstance != nullptr)
	{
		delete m_pInstance;
		m_pInstance = nullptr;
	}
}
//The big 3
BoundingBoxManagerSingleton::BoundingBoxManagerSingleton(){Init();}
BoundingBoxManagerSingleton::BoundingBoxManagerSingleton(BoundingBoxManagerSingleton const& other){ }
BoundingBoxManagerSingleton& BoundingBoxManagerSingleton::operator=(BoundingBoxManagerSingleton const& other) { return *this; }
BoundingBoxManagerSingleton::~BoundingBoxManagerSingleton(){Release();};
//Accessors
int BoundingBoxManagerSingleton::GetBoxTotal(void){ return m_nBoxs; }

//--- Non Standard Singleton Methods
void BoundingBoxManagerSingleton::GenerateBoundingBox(matrix4 a_mModelToWorld, String a_sInstanceName)
{
	MeshManagerSingleton* pMeshMngr = MeshManagerSingleton::GetInstance();
	//Verify the instance is loaded
	if(pMeshMngr->IsInstanceCreated(a_sInstanceName))
	{//if it is check if the Box has already been created
		int nBox = IdentifyBox(a_sInstanceName);
		if(nBox == -1)
		{
			//Create a new bounding Box
			BoundingBoxClass* pBB = new BoundingBoxClass();
			//construct its information out of the instance name
			pBB->GenerateOrientedBoundingBox(a_sInstanceName);
			//Push the Box back into the list
			m_lBox.push_back(pBB);
			//Push a new matrix into the list
			m_lMatrix.push_back(matrix4(IDENTITY));
			//Specify the color the Box is going to have
			m_lColor.push_back(vector3(1.0f));
			//Increase the number of Boxes
			m_nBoxs++;
		}
		else //If the box has already been created you will need to check its global orientation
		{
			m_lBox[nBox]->GenerateAxisAlignedBoundingBox(a_mModelToWorld);
		}
		nBox = IdentifyBox(a_sInstanceName);
		m_lMatrix[nBox] = a_mModelToWorld;
	}
}

void BoundingBoxManagerSingleton::SetBoundingBoxSpace(matrix4 a_mModelToWorld, String a_sInstanceName)
{
	int nBox = IdentifyBox(a_sInstanceName);
	//If the Box was found
	if(nBox != -1)
	{
		//Set up the new matrix in the appropriate index
		m_lMatrix[nBox] = a_mModelToWorld;
	}
}

int BoundingBoxManagerSingleton::IdentifyBox(String a_sInstanceName)
{
	//Go one by one for all the Boxs in the list
	for(int nBox = 0; nBox < m_nBoxs; nBox++)
	{
		//If the current Box is the one we are looking for we return the index
		if(a_sInstanceName == m_lBox[nBox]->GetName())
			return nBox;
	}
	return -1;//couldn't find it return with no index
}

void BoundingBoxManagerSingleton::AddBoxToRenderList(String a_sInstanceName)
{
	//If I need to render all
	if(a_sInstanceName == "ALL")
	{
		for(int nBox = 0; nBox < m_nBoxs; nBox++)
		{
			m_lBox[nBox]->AddAABBToRenderList(m_lMatrix[nBox], m_lColor[nBox], true);
		}
	}
	else
	{
		int nBox = IdentifyBox(a_sInstanceName);
		if(nBox != -1)
		{
			m_lBox[nBox]->AddAABBToRenderList(m_lMatrix[nBox], m_lColor[nBox], true);
		}
	}
}

void BoundingBoxManagerSingleton::CalculateCollision(void)
{
	//Create a placeholder for all center points
	std::vector<vector3> lCentroid;
	//for all Boxs...
	for(int nBox = 0; nBox < m_nBoxs; nBox++)
	{
		//Make all the Boxs white
		m_lColor[nBox] = vector3(1.0f);
		//Place all the centroids of Boxs in global space
		lCentroid.push_back(static_cast<vector3>(m_lMatrix[nBox] * vector4(m_lBox[nBox]->GetCentroid(), 1.0f)));
	}

	//Now the actual check
	for(int i = 0; i < m_nBoxs - 1; i++)
	{
		for(int j = i + 1; j < m_nBoxs; j++)
		{
			//If the distance between the center of both Boxs is less than the sum of their radius there is a collision
			//For this check we will assume they will be colliding unless they are not in the same space in X, Y or Z
			//so we place them in global positions
			vector3 v1Min = m_lBox[i]->GetMinimumAABB();
			vector3 v1Max = m_lBox[i]->GetMaximumAABB();

			vector3 v2Min = m_lBox[j]->GetMinimumAABB();
			vector3 v2Max = m_lBox[j]->GetMaximumAABB();

			bool bColliding = true;
			if(v1Max.x < v2Min.x || v1Min.x > v2Max.x)
				bColliding = false;
			else if(v1Max.y < v2Min.y || v1Min.y > v2Max.y)
				bColliding = false;
			else if(v1Max.z < v2Min.z || v1Min.z > v2Max.z)
				bColliding = false;

			if(bColliding)
				m_lColor[i] = m_lColor[j] = MERED; //We make the Boxes red

			int test = testOBB(m_lBox[i], m_lBox[j], i, j);

			
		}
	}
}

int BoundingBoxManagerSingleton::testOBB(BoundingBoxClass* a, BoundingBoxClass* b, int i, int j){
	vector3 v1c = a->GetCentroid();      // OBB center point
	vector3 v2c = b->GetCentroid();      // OBB center point
			
	vector3 u1[3];  // Local x-, y-, and z-axes
	u1[0] = static_cast<vector3>(m_lMatrix[i] * vector4(1,0,0,0));
	u1[1] = static_cast<vector3>(m_lMatrix[i] * vector4(0,1,0,0));
	u1[2] = static_cast<vector3>(m_lMatrix[i] * vector4(0,0,1,0));
	
	vector3 u2[3];  // Local x-, y-, and z-axes
	u2[0] = static_cast<vector3>(m_lMatrix[j] * vector4(1,0,0,0));
	u2[1] = static_cast<vector3>(m_lMatrix[j] * vector4(0,1,0,0));
	u2[2] = static_cast<vector3>(m_lMatrix[j] * vector4(0,0,1,0));

	float e1[3];     // Positive halfwidth extents of OBB along each axis
	e1[0] = a->GetMaximumOBB().x - a->GetCentroid().x;
	e1[1] = a->GetMaximumOBB().y - a->GetCentroid().y;
	e1[2] = a->GetMaximumOBB().z - a->GetCentroid().z;
			
	float e2[3];
	e2[0] = b->GetMaximumOBB().x - b->GetCentroid().x;
	e2[1] = b->GetMaximumOBB().y - b->GetCentroid().y;
	e2[2] = b->GetMaximumOBB().z - b->GetCentroid().z;

	// ra = obj1 distance between center and plane edge
	// rb = obj2 distance between center and plane edge 
	float ra, rb;
	matrix4 R, AbsR;

	// Compute rotation matrix expressing b in a's coordinate frame
	for (int x = 0; x < 3; x++)
		for (int y = 0; y < 3; y++)
			R[i][j] = glm::dot(u1[i], u2[j]);

	// Compute translation vector t
	vector3 t = v2c - v1c;
	// Bring translation into a's coordinate frame
	t = vector3(glm::dot(t, u1[0]), glm::dot(t, u1[1]), glm::dot(t, u1[2]));

	// Compute common subexpressions. Add in an epsilon term to
	// counteract arithmetic errors when two edges are parallel and
	// their cross product is (near) null (see text for details)

	for (int x = 0; x < 3; i++)
		for (int y = 0; y < 3; j++)
			AbsR[x][y] = glm::abs(R[x][y]) + FLT_EPSILON;

	// Test axes L = A0, L = A1, L = A2
	for (int x = 0; x < 3; i++) {
		ra = e1[x];
		rb = e2[0] * AbsR[x][0] + e2[1] * AbsR[x][1] + e2[2] * AbsR[x][2];
		if (glm::abs(t[x]) > ra + rb) return 0;
	}

	// Test axes L = B0, L = B1, L = B2
	for (int x = 0; x < 3; i++) {
		ra = e1[0] * AbsR[0][x] + e1[1] * AbsR[1][x] + e1[2] * AbsR[2][x];
		rb = e1[x];
		if (glm::abs(t[0] * R[0][x] + t[1] * R[1][x] + t[2] * R[2][x]) > ra + rb) return 0;
	}

	// Test axis L = A0 x B0
	ra = e1[1] * AbsR[2][0] + e1[2] * AbsR[1][0];
	rb = e2[1] * AbsR[0][2] + e2[2] * AbsR[0][1];
	if (glm::abs(t[2] * R[1][0] - t[1] * R[2][0]) > ra + rb) return 0;

	// Test axis L = A0 x B1
	ra = e1[1] * AbsR[2][1] + e1[2] * AbsR[1][1];
	rb = e2[0] * AbsR[0][2] + e2[2] * AbsR[0][0];
	if (glm::abs(t[2] * R[1][1] - t[1] * R[2][1]) > ra + rb) return 0;

	// Test axis L = A0 x B2
	ra = e1[1] * AbsR[2][2] + e1[2] * AbsR[1][2];
	rb = e2[0] * AbsR[0][1] + e2[1] * AbsR[0][0];
	if (glm::abs(t[2] * R[1][2] - t[1] * R[2][2]) > ra + rb) return 0;

	// Test axis L = A1 x B0
	ra = e1[0] * AbsR[2][0] + e1[2] * AbsR[0][0];
	rb = e2[1] * AbsR[1][2] + e2[2] * AbsR[1][1];

	if (glm::abs(t[0] * R[2][0] - t[2] * R[0][0]) > ra + rb) return 0;

	// Test axis L = A1 x B1
	ra = e1[0] * AbsR[2][1] + e1[2] * AbsR[0][1];
	rb = e2[0] * AbsR[1][2] + e2[2] * AbsR[1][0];
	if (glm::abs(t[0] * R[2][1] - t[2] * R[0][1]) > ra + rb) return 0;

	// Test axis L = A1 x B2
	ra = e1[0] * AbsR[2][2] + e1[2] * AbsR[0][2];
	rb = e2[0] * AbsR[1][1] + e2[1] * AbsR[1][0];
	if (glm::abs(t[0] * R[2][2] - t[2] * R[0][2]) > ra + rb) return 0;

	// Test axis L = A2 x B0
	ra = e1[0] * AbsR[1][0] + e1[1] * AbsR[0][0];
	rb = e2[1] * AbsR[2][2] + e2[2] * AbsR[2][1];
	if (glm::abs(t[1] * R[0][0] - t[0] * R[1][0]) > ra + rb) return 0;

	// Test axis L = A2 x B1
	ra = e1[0] * AbsR[1][1] + e1[1] * AbsR[0][1];
	rb = e2[0] * AbsR[2][2] + e2[2] * AbsR[2][0];
	if (glm::abs(t[1] * R[0][1] - t[0] * R[1][1]) > ra + rb) return 0;

	// Test axis L = A2 x B2
	ra = e1[0] * AbsR[1][2] + e1[1] * AbsR[0][2];
	rb = e2[0] * AbsR[2][1] + e2[1] * AbsR[2][0];
	if (glm::abs(t[1] * R[0][2] - t[0] * R[1][2]) > ra + rb) return 0;

	return 1;
}
