/****************************************************************************************** 
 *	Chili DirectX Framework Version 16.07.20											  *	
 *	Game.cpp																			  *
 *	Copyright 2016 PlanetChili.net <http://www.planetchili.net>							  *
 *																						  *
 *	This file is part of The Chili DirectX Framework.									  *
 *																						  *
 *	The Chili DirectX Framework is free software: you can redistribute it and/or modify	  *
 *	it under the terms of the GNU General Public License as published by				  *
 *	the Free Software Foundation, either version 3 of the License, or					  *
 *	(at your option) any later version.													  *
 *																						  *
 *	The Chili DirectX Framework is distributed in the hope that it will be useful,		  *
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of						  *
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the						  *
 *	GNU General Public License for more details.										  *
 *																						  *
 *	You should have received a copy of the GNU General Public License					  *
 *	along with The Chili DirectX Framework.  If not, see <http://www.gnu.org/licenses/>.  *
 ******************************************************************************************/

 /*******************************************************************************************
 *	Code and amendments by s0lly														   *
 *	https://www.youtube.com/c/s0lly							                               *
 *	https://s0lly.itch.io/																   *
 *	https://www.instagram.com/s0lly.gaming/												   *
 ********************************************************************************************/





#include "MainWindow.h"
#include "Game.h"

Game::Game(MainWindow& wnd)
	:
	wnd(wnd),
	gfx(wnd)
{
	// Initialise objects

	worldObjects.loc = new Vec2[numObjects]();
	worldObjects.oldVelocity = new Vec2[numObjects]();
	worldObjects.velocity = new Vec2[numObjects]();
	worldObjects.mass = new float[numObjects]();
	worldObjects.radius = new float[numObjects]();
	worldObjects.color = new Color[numObjects]();
	worldObjects.nodeID = new int[numObjects]();
	worldObjects.calcsCompleted = new int[numObjects]();


	for (int i = 0; i < numObjects; i++)
	{
		int sizeOfField = 10000;

		float xRand = ((float)(std::rand() % sizeOfField) - (float)(sizeOfField / 2)) * 4.0f;
		float yRand = ((float)(std::rand() % sizeOfField) - (float)(sizeOfField / 2)) * 4.0f;

		Vec2 locRand(xRand, yRand);
		locRand = locRand / sqrt(locRand.x * locRand.x + locRand.y * locRand.y);
		locRand = locRand * (float)(std::rand() % sizeOfField);
		
		float massRand = ((float)(std::rand() % 10000) + 1.0f);
		float radiusRand = std::sqrt(massRand / (PI / 1.0f));

		unsigned char rRand = 64 + (unsigned char)(std::rand() % 192);
		unsigned char gRand = (256 - rRand) + 64;
		unsigned char bRand = 255;//(256 - (rRand + gRand) / 2) + 64;

		xRand = abs(xRand) < 1.0f ? 1.0f : xRand;
		yRand = abs(yRand) < 1.0f ? 1.0f : yRand;

		locRand.x = locRand.x + (((float)(std::rand() % 5) - 2.0f) * ((float)sizeOfField / 2.0f)) / 2.0f;
		locRand.y = locRand.y + (((float)(std::rand() % 3) - 1.0f) * ((float)sizeOfField / 2.0f)) / 2.0f;

		float magnitudeX = -(locRand.y) / 2.0;
		float magnitudeY = +(locRand.x) / 2.0;

		Vec2 startForce(magnitudeX, magnitudeY);

		//startForce = Vec2();

		
		

		worldObjects.loc[currentAssignedObjects] = locRand;
		worldObjects.velocity[currentAssignedObjects] = startForce;
		worldObjects.mass[currentAssignedObjects] = massRand;
		worldObjects.radius[currentAssignedObjects] = radiusRand;
		worldObjects.color[currentAssignedObjects] = Color(rRand, gRand, bRand);

		currentAssignedObjects++;
	}

	tree.topLeft = Vec2();
	tree.botRight = Vec2();

	int lowestPowerOf4 = 1;
	numPlanes = 0;

	while (lowestPowerOf4 < currentAssignedObjects)
	{
		lowestPowerOf4 *= 4;
		numPlanes++;
	}

	totalNodes = 0;

	for (int p = 0; p < numPlanes; p++)
	{
		totalNodes += pow(4, p);
	}



	nodeAveLoc = new Vec2[totalNodes]();
	nodeTotalMass = new float[totalNodes]();
	nodeObjectsContained.clear();
	nodeObjectsContained = std::vector<std::vector<int>>(currentAssignedObjects);

	cameraLoc = Vec2(0.0f, 0.0f);
}

void Game::Go()
{
	start = std::chrono::system_clock::now();

	gfx.BeginFrame();

	ProcessInput();
	UpdateModel();
	ComposeFrame();

	gfx.EndFrame();

	end = std::chrono::system_clock::now();
	std::chrono::duration<float> elapsedTime = end - start;
	dt = elapsedTime.count();
}

void Game::ProcessInput()
{
	// Camera movements - arrow keys are effectively disabled as camera is locked on to largest object

	if (wnd.kbd.KeyIsPressed(VK_UP))
	{
		cameraLoc.y += 5.0f;
	}
	if (wnd.kbd.KeyIsPressed(VK_DOWN))
	{
		cameraLoc.y -= 5.0f;
	}
	if (wnd.kbd.KeyIsPressed(VK_LEFT))
	{
		cameraLoc.x -= 5.0f;
	}
	if (wnd.kbd.KeyIsPressed(VK_RIGHT))
	{
		cameraLoc.x += 5.0f;
	}

	if (wnd.kbd.KeyIsPressed('A'))
	{
		cameraZoomOut *= 0.9f;
	}
	if (wnd.kbd.KeyIsPressed('Z'))
	{
		cameraZoomOut *= 1.1f;
	}
}

void Game::UpdateModel()
{
	

	// Worldspace is the highest level space (equivalent to maxPlane). Each space has 4 qaudrants.

	tree.topLeft = Vec2();
	tree.botRight = Vec2();

	for (int i = 0; i < currentAssignedObjects; i++)
	{
		tree.topLeft.x = tree.topLeft.x < worldObjects.loc[i].x ? tree.topLeft.x : worldObjects.loc[i].x - 1.0f;
		tree.topLeft.y = tree.topLeft.y < worldObjects.loc[i].y ? tree.topLeft.y : worldObjects.loc[i].y - 1.0f;
		tree.botRight.x = tree.botRight.x > worldObjects.loc[i].x ? tree.botRight.x : worldObjects.loc[i].x + 1.0f;
		tree.botRight.y = tree.botRight.y > worldObjects.loc[i].y ? tree.botRight.y : worldObjects.loc[i].y + 1.0f;
	}

	float treeWidth = tree.botRight.x - tree.topLeft.x;
	float treeHeight = tree.botRight.y - tree.topLeft.y;

	// We want some higher power of 4 than current worldObjects to get fine resolution on calculations

	optimiseStart = std::chrono::system_clock::now();

	memset(nodeAveLoc, 0, sizeof(Vec2) * totalNodes);
	memset(nodeTotalMass, 0, sizeof(float) * totalNodes);
	nodeObjectsContained.clear();
	nodeObjectsContained = std::vector<std::vector<int>>(currentAssignedObjects);

	optimiseEnd = std::chrono::system_clock::now();
	std::chrono::duration<float> optimisedElapsedTime = optimiseEnd - optimiseStart;
	optimiseDt = optimisedElapsedTime.count(); //dt = 1.0f;//

	// set worldObjects to detailed plane nodes

	int detailedNodeDim = pow(2, (numPlanes - 1));

	for (int i = 0; i < currentAssignedObjects; i++)
	{
		int x = (int)((worldObjects.loc[i].x - tree.topLeft.x) / (treeWidth / (float)detailedNodeDim));
		int y = (int)((worldObjects.loc[i].y - tree.topLeft.y) / (treeHeight / (float)detailedNodeDim));

		int detailedNode = y * detailedNodeDim + x;

		worldObjects.nodeID[i] = detailedNode;
		nodeObjectsContained[detailedNode].push_back(i);
		

		nodeAveLoc[detailedNode] = nodeAveLoc[detailedNode] + worldObjects.loc[i] * worldObjects.mass[i];
		nodeTotalMass[detailedNode] = nodeTotalMass[detailedNode] + worldObjects.mass[i];

		// we've loaded in lowest plane, now work up

		int currX = x;
		int currY = y;
		int numOfPriorPlaneNodes = 0;

		for (int p = 1; p < numPlanes; p++)
		{
			numOfPriorPlaneNodes += pow(4, (numPlanes - p));

			currX /= 2;
			currY /= 2;

			int planeNode = numOfPriorPlaneNodes + currY * pow(2, (numPlanes - p - 1)) + currX;

			nodeAveLoc[planeNode] = nodeAveLoc[planeNode] + worldObjects.loc[i] * worldObjects.mass[i];
			nodeTotalMass[planeNode] = nodeTotalMass[planeNode] + worldObjects.mass[i];
		}

	}

	



	for (int n = 0; n < totalNodes; n++)
	{
		if (nodeTotalMass[n] > 0.0f)
		{
			nodeAveLoc[n] = nodeAveLoc[n] / nodeTotalMass[n];
		}
	}

	
	

	// We now have a pyramid of nodes

	
	//////////////////////
	//
	// Algorithm approach:
	//
	// for each object:
	// we check the first level of quadrants
	//
	// if the quadrant has a total mass vs distance at some ratio, check its quadrants for further examination
	// or if the quadrant contains the particle itself, check its quadrants
	//		if the quadrant is at the highest level of resolution, use that quadrant's gravity
	//
	// else, apply that quadrant's gravity to the object in question
	//
	// lastly, for all objects in the quadrant of the object itself (if any), process each object individually
	//
	//////////////////////


	// do a memcopy here?
	for (int i = 0; i < currentAssignedObjects; i++)
	{
		worldObjects.oldVelocity[i] = worldObjects.velocity[i];
	}
	
	// we will want to check the topmost quadrant first
	int currentPlane = numPlanes - 1;

	numCalcs = 0.0f;


	int numThreads = 8;
	int threadSize = currentAssignedObjects / numThreads + 1;
	auto worldObjectsPtr = &worldObjects;
	auto numPlanesPtr = &numPlanes;
	auto nodeAveLocPtr = &nodeAveLoc;
	auto nodeTotalMassPtr = &nodeTotalMass;
	auto nodeObjectsContainedPtr = &nodeObjectsContained;
	auto currentAssignedObjectsPtr = &currentAssignedObjects;

	auto dtPtr = &dt;

	std::vector<std::thread> threadList;
	for (int k = 0; k < numThreads; k++)
	{
		threadList.push_back(std::thread([worldObjectsPtr, k, currentAssignedObjectsPtr, numPlanesPtr, nodeAveLocPtr, nodeTotalMassPtr, nodeObjectsContainedPtr, dtPtr, threadSize]()
		{
			for (int i = k * threadSize; (i < (k + 1) * threadSize) && (i < (*currentAssignedObjectsPtr)); i++)
			{
				worldObjectsPtr->calcsCompleted[i] = 0;

				RecursivePlaneQuadrantCheckAndApplyGravity(worldObjectsPtr, (*currentAssignedObjectsPtr), i, *numPlanesPtr, *numPlanesPtr - 1,
					*nodeAveLocPtr, *nodeTotalMassPtr, nodeObjectsContainedPtr, 0, 0, (*dtPtr));
			}
		}));
	}
	std::for_each(threadList.begin(), threadList.end(), std::mem_fn(&std::thread::join));


	for (int i = 0; i < currentAssignedObjects; i++)
	{
		numCalcs += (float)worldObjectsPtr->calcsCompleted[i];
	}



	// Update all objects locations based on all active forces
	for (int i = 0; i < currentAssignedObjects; i++)
	{
		worldObjects.loc[i] = worldObjects.loc[i] +  (worldObjects.oldVelocity[i] + worldObjects.velocity[i]) * dt * 0.5f;
	}

	
	// Merge objects that have collided
	//if (currentAssignedObjects > 1)
	//{
	//	for (int i = 0; i < currentAssignedObjects - 1; i++)
	//	{
	//		for (int j = i + 1; j < currentAssignedObjects; j++)
	//		{
	//			if (CheckCollision(worldObjects[i], worldObjects[j]))
	//			{
	//				WorldObject worldObjectMerged= MergeObjectsAndReturnNew(worldObjects[i], worldObjects[j]);
	//
	//				std::swap(worldObjects[j], worldObjects[currentAssignedObjects - 1]);
	//				currentAssignedObjects--;
	//				j--;
	//
	//				std::swap(worldObjects[i], worldObjects[currentAssignedObjects - 1]);
	//				currentAssignedObjects--;
	//				i--;
	//				j--;
	//
	//				worldObjects[currentAssignedObjects] = worldObjectMerged;
	//
	//				currentAssignedObjects++;
	//
	//				break;
	//			}
	//		}
	//	}
	//}
	



	// Lock camera on to largest object
	//
	//float alignedMass = 0.0f;
	//Vec2 newLoc = cameraLoc;
	//for (int i = 0; i < currentAssignedObjects; i++)
	//{
	//	if (alignedMass < worldObjects[i].mass) 
	//	{
	//		alignedMass = worldObjects[i].mass;
	//		newLoc = worldObjects[i].loc;
	//	}
	//}


	// Lock camera on to center of total mass
	
	float alignedMassXLoc = 0.0f;
	float alignedMassYLoc = 0.0f;
	float totalMass = 0.0f;
	
	for (int i = 0; i < currentAssignedObjects; i++)
	{
		alignedMassXLoc += worldObjects.loc[i].x * worldObjects.mass[i];
		alignedMassYLoc += worldObjects.loc[i].y * worldObjects.mass[i];
		totalMass += worldObjects.mass[i];
	}
	alignedMassXLoc /= totalMass;
	alignedMassYLoc /= totalMass;
	
	Vec2 newLoc = Vec2(alignedMassXLoc, alignedMassYLoc);



	// Smooth camera movement
	
	float percentageToMove = 0.1f;
	cameraLoc = Vec2((newLoc.x * percentageToMove + cameraLoc.x * (1.0f - percentageToMove)), (newLoc.y * percentageToMove + cameraLoc.y * (1.0f - percentageToMove)));



	// Smooth camera zoom

	//float smallestY = 0.0f;
	//float largestY = 1.0f;
	//
	//for (int i = 0; i < currentAssignedObjects; i++)
	//{
	//	if (worldObjects.loc[i].y - cameraLoc.y > largestY)
	//	{
	//		largestY = worldObjects.loc[i].y - cameraLoc.y;
	//	}
	//	if (cameraLoc.y - worldObjects.loc[i].y > smallestY)
	//	{
	//		smallestY = cameraLoc.y - worldObjects.loc[i].y;
	//	}
	//}
	//
	//float newY = largestY > smallestY ? largestY : smallestY;
	//
	//float newCameraZoomOut = ((2.0f * newY) / (float)gfx.ScreenHeight) * 1.2f;
	//
	//percentageToMove = 0.02f;
	//cameraZoomOut = newCameraZoomOut * percentageToMove + cameraZoomOut * (1.0f - percentageToMove);

}

void Game::ComposeFrame()
{
	// Draw all objects

	for (int i = 0; i < currentAssignedObjects; i++)
	{
		gfx.DrawCircle(Vec2((worldObjects.loc[i].x - cameraLoc.x) / (cameraZoomOut) + (float)(gfx.ScreenWidth / 2), -(worldObjects.loc[i].y - cameraLoc.y) / (cameraZoomOut) + (float)(gfx.ScreenHeight / 2)), worldObjects.radius[i] / (cameraZoomOut), worldObjects.color[i], 0.15f);
	}


	// Display number of active objects and MSEC PER FRAME, with other info. as needed

	RetroContent::DrawString(gfx, "NUMBER OF OBJECTS: " + std::to_string(currentAssignedObjects) , Vec2(200.0f, 20.0f), 2, Colors::Red);
	//RetroContent::DrawString(gfx, "PERCENT OPTIMISED: " + std::to_string(100 - (int)(((float)numCalcs * 100) / ((float)currentAssignedObjects * (float)(currentAssignedObjects - 1)))), Vec2(200.0f, 60.0f), 2, Colors::Red); // (int)(numCalcs * 100) / (currentAssignedObjects * (currentAssignedObjects - 1))
	RetroContent::DrawString(gfx, "TOTAL MSEC PER FRAME: " + std::to_string(int(dt * 1000.0f)), Vec2(800.0f, 20.0f), 2, Colors::Yellow);
	//RetroContent::DrawString(gfx, "CHECK MSEC PER FRAME: " + std::to_string(int(optimiseDt * 1000.0f)), Vec2(800.0f, 80.0f), 2, Colors::Yellow);
}

