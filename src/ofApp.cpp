#include "ofApp.h"

//Filename: ofApp.cpp
//Version: 1.0
//Author: J. Brown (1201717)
//Date: 19/01/2016
//
//Purpose: This is the code file for an openFrameworks application; the overarching program logic will be here.

//--------------------------------------------------------------
void ofApp::setup()
{
	// Set maximum framerate.
	ofSetFrameRate(60);
	ofSetBackgroundColor(ofColor(182, 227, 242));
	ofSetVerticalSync(true);

	// Set up z-buffer.
	ofEnableDepthTest();

	// Set up GUI
	theGUI = new ofxDatGui(ofxDatGuiAnchor::TOP_RIGHT);
	theGUI->setTheme(new ofxDatGuiThemeCharcoal());
	
	// Disable the GUI's own automatic rendering, as we're doing shader passes and things.
	theGUI->setAutoDraw(false);



	// Create the camera, using OpenFrameworks' ofEasyCam class. This gives us a simple control system.
	theCamera = new ofxFirstPersonCamera();
	theCamera->setNearClip(0.01f);
	theCamera->setFarClip(1500.f);

	// Create lighting shader
	lightShader = new ofShader();
	lightShader->load("data/shaders/directional_light.vert", "data/shaders/directional_light.frag");
	
	// Scalar value for shader to determine if we should use smoothed normals or not on the grid terrain.
	GridExpensiveNormals = 0.0f;
	
	// Make the terrain, starting off with using the GridMarchingCubes implementation.
	//theTerrain = new TerrainGridMarchingCubes();
	//((TerrainGridMarchingCubes*)theTerrain)->Rebuild(GridTerrainResolution, GridTerrainResolution, GridTerrainResolution, GridTerrainSize);
	theTerrain = new TerrainDistanceRaymarch();
	((TerrainDistanceRaymarch*)theTerrain)->Rebuild(1280, 720);
	((TerrainDistanceRaymarch*)theTerrain)->CurrentCamera = theCamera;
	currentTerrainType = TERRAIN_TYPE::TERRAIN_RAY_DIST;

	// Make the physics world.
	thePhysicsWorld = new ofxBulletWorldRigid();
	thePhysicsWorld->setup();

	// Set up gravity
	thePhysicsWorld->setGravity(ofVec3f(0, -9.81f, 0));
	thePhysicsWorld->enableGrabbing();
	thePhysicsWorld->setCamera(theCamera);

	// Create the physics sphere
	testSphere = new ofxBulletSphere();
	testSphere->create(thePhysicsWorld->world, theCamera->getPosition() + theCamera->upvector * 20, 1.0, 2.0);
	
	testBoxMesh = new ofBoxPrimitive(10, 10, 10, 1, 1, 1);

	testBox = new ofxBulletCustomShape();
	testBox->addMesh(testBoxMesh->getMesh(), ofVec3f(1, 1, 1), false);
	testBox->create(thePhysicsWorld->world, theCamera->getPosition() + theCamera->upvector * 35, 1.0f);
	
	testSphere->add();
	testBox->add();

	// Create a blank mesh
	ofMesh singleTriangle;
	singleTriangle.addVertex(ofVec3f(0, 0, 0));
	singleTriangle.addVertex(ofVec3f(1, 0, 0));
	singleTriangle.addVertex(ofVec3f(1, 1, 0));
	singleTriangle.addIndex(0);
	singleTriangle.addIndex(1);
	singleTriangle.addIndex(2);
	thePhysicsMesh = CreatePhysicsMesh(thePhysicsWorld, &singleTriangle);

	((TerrainGridMarchingCubes*)theTerrain)->updatePhysicsMesh = true;

	// Test mesh cutting
	planeNormal = ofVec3f(ofRandomf(), ofRandomf(), ofRandomf());
	planeNormal.normalize();
	planePoint = ofVec3f(ofRandomf(), ofRandomf(), ofRandomf());
	//planeNormal = ofVec3f(0.5, 0.5, 0);
	//planePoint = ofVec3f(0, 1, 2);

	//cutMeshes = CutMeshWithPlane(planePoint, planeNormal, testBox->getMesh());

	// Add elements to GUI.
	buildGUI();
	
}

//--------------------------------------------------------------
void ofApp::update()
{
	float deltaTime = ofGetLastFrameTime();
	
	// Update GUI
	auto frametimeGUI = theGUI->getTextInput("Frame-Time", "Diagnostics");
	frametimeGUI->setText(std::to_string(deltaTime) + " ms");
	auto frametimePlot = theGUI->getValuePlotter("FT", "Diagnostics");
	frametimePlot->setValue(deltaTime);
	//frametimePlot->setSpeed(0.1f);

	theGUI->update();

	// Update camera offset for terrain.
	theTerrain->SetOffset(theCamera->getPosition());

	// Set terrain variables.
	if (currentTerrainType == TERRAIN_TYPE::TERRAIN_GRID_MC)
	{
		((TerrainGridMarchingCubes*)theTerrain)->expensiveNormals = GridExpensiveNormals;
		//((TerrainGridMarchingCubes*)theTerrain)->updatePhysicsMesh = physicsNeedsRebuilding;
		((TerrainGridMarchingCubes*)theTerrain)->thePhysicsWorld = thePhysicsWorld;
		((TerrainGridMarchingCubes*)theTerrain)->thePhysicsMesh = thePhysicsMesh;
	}
	else if (currentTerrainType == TERRAIN_TYPE::TERRAIN_RAY_DIST)
	{
		((TerrainDistanceRaymarch*)theTerrain)->numIterations = RayTerrainIterations;
		((TerrainDistanceRaymarch*)theTerrain)->maximumDepth = RayTerrainDrawDistance;
		((TerrainDistanceRaymarch*)theTerrain)->RaymarchResX = RayTerrainResolutionX;
		((TerrainDistanceRaymarch*)theTerrain)->RaymarchResY = RayTerrainResolutionY;
	}

	
	// Update terrain
	theTerrain->Update();

	// Update physics
	if (PhysicsEnabled)
	{
		thePhysicsWorld->update(deltaTime * (PhysicsTimescale*2), 0);
	}


	
}

//--------------------------------------------------------------
void ofApp::draw()
{


	theCamera->begin(); // Begin drawing with this camera.

		// Draw the terrain without using the camera, if it's raymarched.
		if(currentTerrainType == TERRAIN_TYPE::TERRAIN_RAY_DIST)
		{
			theCamera->end();

			theTerrain->Draw();

			theCamera->begin();
		}
		else
		{
			theTerrain->Draw();
		}
		

		// Debug: draw the physics mesh over the top
		ofDisableDepthTest();
		if (thePhysicsWorld->world->getDebugDrawer() != 0)
		{
			thePhysicsWorld->world->getDebugDrawer()->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
		}
		
		if (PhysicsWireframe)
		{
			thePhysicsWorld->drawDebug();
		}
		
		
		ofEnableDepthTest();


		// Enable light shader
		lightShader->begin();

		// Draw physics objects.
		testSphere->draw();

		// Draw physics box
		if (testBox->getCollisionShape() != NULL)
		{
			
			testBox->transformGL();
			if (!PhysicsWireframe)
			{
				testBoxMesh->draw();
			}
			else
			{
				testBoxMesh->drawWireframe();
			}
			
			testBox->restoreTransformGL();
		}
		
		// Draw sliced up objects
		for (int i = 0; i < cutPhysicsObjects.size(); i++)
		{
			//cutPhysicsObjects.at(i).second->draw();
			cutPhysicsObjects.at(i).second->transformGL();
			
			if (PhysicsWireframe)
			{
				cutPhysicsObjects.at(i).first->drawWireframe();
			}
			else
			{
				cutPhysicsObjects.at(i).first->draw();
			}
			
			cutPhysicsObjects.at(i).second->restoreTransformGL();
		}

		// stop using lights
		lightShader->end();

	theCamera->end(); // Cease drawing with the camera.


	// Draw GUI
	ofDisableDepthTest();
	theGUI->draw();
	ofEnableDepthTest();

	// Check to see if terrain rebuilt our physics during its draw phase.
	if (physicsNeedsRebuilding && currentTerrainType == TERRAIN_TYPE::TERRAIN_GRID_MC)
	{
		if (((TerrainGridMarchingCubes*)theTerrain)->updatePhysicsMesh == false)
		{
			physicsNeedsRebuilding = false;
		}

	}

	
	

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
	

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button)
{
	if (button == 2) // Right Mouse
	{
		theCamera->toggleControl();
	}

	if (button == 1) // Middle Mouse
	{
		// Trace a point on the terrain, add a csg sphere
		if (currentTerrainType == TERRAIN_TYPE::TERRAIN_GRID_MC)
		{
			ofVec3f removePos = (theCamera->getPosition() + (theCamera->getPosition() - ((TerrainGridMarchingCubes*)theTerrain)->theGrid->getPosition()));
			
			((TerrainGridMarchingCubes*)theTerrain)->CSGRemoveSphere(removePos, 25);
			std::cout << "Removed CSG Sphere, at " << removePos << "." << std::endl;
		}
	}
	
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

void ofApp::onSliderChanged(ofxDatGuiSliderEvent e)
{
	
	
}

void ofApp::onButtonChanged(ofxDatGuiButtonEvent e)
{
	if (e.target->getName() == "Rebuild Terrain" && currentTerrainType == TERRAIN_TYPE::TERRAIN_GRID_MC)
	{
		((TerrainGridMarchingCubes*)theTerrain)->Rebuild(GridTerrainResolution, GridTerrainResolution, GridTerrainResolution, GridTerrainSize);
	}
	if (e.target->getName() == "Rebuild Terrain" && currentTerrainType == TERRAIN_TYPE::TERRAIN_RAY_DIST)
	{
		((TerrainDistanceRaymarch*)theTerrain)->Rebuild(RayTerrainResolutionX, RayTerrainResolutionY);
	}
	if (e.target->getName() == "Smooth Normals" && currentTerrainType == TERRAIN_TYPE::TERRAIN_GRID_MC)
	{
		GridExpensiveNormals = e.enabled;
	}
	if (e.target->getName() == "Physics Enabled")
	{
		PhysicsEnabled = e.enabled;
	}
	if (e.target->getName() == "Wireframe")
	{
		PhysicsWireframe = e.enabled;
	}
	if (e.target->getName() == "Slice")
	{


		//cutMeshes = CutMeshWithPlane(planePoint, planeNormal, sliceMesh);
		//cutPhysicsObjects = SlicePhysicsObject(testBox, testBoxMesh->getMeshPtr(), planePoint + testBox->getPosition(), planeNormal, thePhysicsWorld, true);

		// Do voronoi test

		cutPhysicsObjects = VoronoiFracture(testBox, testBoxMesh->getMeshPtr(), thePhysicsWorld, 16, NULL);
		e.target->setName("Slice Done");

		// Move meshes to new locations
		/*
		for (int mesh = 0; mesh < voronoiMeshes.size(); mesh++)
		{
			for (int i = 0; i < voronoiMeshes.at(mesh).getVertices().size(); i++)
			{
				voronoiMeshes.at(mesh).getVertices().at(i) += origin;
			}
		}*/
		
		// Cut object by planes



	}

}

// Build GUI
void ofApp::buildGUI()
{
	if (theGUI != 0)
	{
		delete theGUI;
		theGUI = 0;
		theGUI = new ofxDatGui(ofxDatGuiAnchor::TOP_RIGHT);
		
		theGUI->setTheme(new ofxDatGuiThemeWireframe());
		theGUI->setAutoDraw(false);
		theGUI->onButtonEvent(this, &ofApp::onButtonChanged);
		theGUI->onSliderEvent(this, &ofApp::onSliderChanged);
		theGUI->setWidth(500);
	}

	auto titleLabel = theGUI->addLabel("Real-Time Physics Based Destruction With Density-Field Terrains");
	
	
	theGUI->addLabel("J. Brown (1201717)");
	theGUI->addBreak()->setHeight(2.0f);

	ofxDatGuiFolder* diagnosticsFolder = theGUI->addFolder("Diagnostics", ofColor::white);
	diagnosticsFolder->addFRM();
	diagnosticsFolder->addTextInput("Frame-Time", "0ms");
	auto diagPlot = diagnosticsFolder->addValuePlotter("FT", 0.00f, 0.1f);
	diagPlot->setDrawMode(ofxDatGuiGraph::FILLED);
	diagPlot->setSpeed(2.0f);
	
	
	


	theGUI->addBreak()->setHeight(2.0f);

	theGUI->addLabel("Terrain Type: ");
	vector<string> terrainOptions = { "Grid-Based Naive Marching Cubes", "Grid-Based Optimised Marching Cubes", "Raymarched Distance Field" };
	theGUI->addDropdown("Grid-Based Naive Marching Cubes",terrainOptions);
	theGUI->addBreak()->setHeight(2.0f);

	ofxDatGuiFolder* terrainFolder = theGUI->addFolder("Terrain Controls", ofColor::darkCyan);
	if (currentTerrainType == TERRAIN_TYPE::TERRAIN_GRID_MC)
	{
		auto gridResolutionSlider = terrainFolder->addSlider("Grid Resolution", 3, 128, 64);
		gridResolutionSlider->setPrecision(0);
		gridResolutionSlider->bind(GridTerrainResolution);

		auto gridScaleSlider = terrainFolder->addSlider("Grid Zoom", 1, 20, 5);
		gridScaleSlider->setPrecision(1);
		gridScaleSlider->bind(GridTerrainSize);

		auto gridNormalsToggle = terrainFolder->addToggle("Smooth Normals", false);
		

		terrainFolder->addButton("Rebuild Terrain");
	}
	else if (currentTerrainType == TERRAIN_TYPE::TERRAIN_RAY_DIST)
	{
		auto terrainResSizeX = terrainFolder->addSlider("Render Resolution X", 32, 1280, 1280);
		terrainResSizeX->setPrecision(0);
		terrainResSizeX->bind(RayTerrainResolutionX);

		auto terrainResSizeY = terrainFolder->addSlider("Render Resolution Y", 24, 720, 720);
		terrainResSizeY->setPrecision(0);
		terrainResSizeY->bind(RayTerrainResolutionY);

		auto terrainStepAmt = terrainFolder->addSlider("Max Steps", 16, 1024, 256);
		terrainStepAmt->setPrecision(0);
		terrainStepAmt->bind(RayTerrainIterations);

		auto terrainDistance = terrainFolder->addSlider("Max Distance", 64, 16000, 1500);
		terrainDistance->setPrecision(2);
		terrainDistance->bind(RayTerrainDrawDistance);

		terrainFolder->addButton("Rebuild Terrain");
	}

	ofxDatGuiFolder* physicsFolder = theGUI->addFolder("Physics", ofColor::red);

	physicsFolder->addToggle("Physics Enabled", false);
	physicsFolder->addToggle("Wireframe", false);

	auto physicsSlider = physicsFolder->addSlider("Timescale", 0.01f, 1.0f, 1.0f);
	physicsSlider->setPrecision(2);
	physicsSlider->bind(PhysicsTimescale);
	
	auto sliceButton = physicsFolder->addButton("Slice");


	

	auto footerGUI = theGUI->addFooter();
	footerGUI->setLabelWhenCollapsed(":: SHOW TOOLS ::");
	footerGUI->setLabelWhenExpanded(":: HIDE TOOLS ::");

}

ofxBulletTriMeshShape* ofApp::CreatePhysicsMesh(ofxBulletWorldRigid* world, ofMesh* theMesh)
{
	ofxBulletTriMeshShape* newShape = new ofxBulletTriMeshShape();
	newShape->create(world->world, *theMesh, ofVec3f(0, 0, 0), 1.0f);
	newShape->add();
	newShape->enableKinematic();
	newShape->setActivationState(DISABLE_DEACTIVATION);


	return newShape;
}
