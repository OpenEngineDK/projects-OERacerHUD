// Game Factory for the OERacer project.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

// Serialization (must be first)
#include <fstream>
#include <Utils/Serialization.h>

// Class header
#include "GameFactory.h"

// OpenEngine library
#include <Display/FollowCamera.h>
#include <Display/Frustum.h>
#include <Display/InterpolatedViewingVolume.h>
#include <Display/ViewingVolume.h>
#include <Display/SDLFrame.h>
#include <Devices/SDLInput.h>
#include <Logging/Logger.h>
#include <Renderers/IRenderNode.h>
#include <Renderers/RenderStateNode.h>
#include <Renderers/OpenGL/Renderer.h>
#include <Renderers/OpenGL/RenderingView.h>
#include <Resources/IModelResource.h>
#include <Resources/File.h>
#include <Resources/GLSLResource.h>
#include <Resources/TGAResource.h>
#include <Resources/OBJResource.h>
#include <Resources/DirectoryManager.h>
#include <Resources/ResourceManager.h>
#include <Scene/SceneNode.h>
#include <Scene/GeometryNode.h>
#include <Scene/TransformationNode.h>
#include <Utils/Statistics.h>

// from AccelerationStructures extension
#include <Scene/CollectedGeometryTransformer.h>
#include <Scene/QuadTransformer.h>
#include <Scene/BSPTransformer.h>
#include <Scene/ASDotVisitor.h>
#include <Renderers/AcceleratedRenderingView.h>

// from FixedTimeStepPhysics
#include <Physics/FixedTimeStepPhysics.h>
#include <Physics/RigidBox.h>

// Project files
#include "KeyboardHandler.h"
#include "RenderStateHandler.h"
#include "MoveHandler.h"
#include "QuitHandler.h"

// Additional namespaces (others are in the header).
using namespace OpenEngine::Devices;
using namespace OpenEngine::Renderers::OpenGL;
using namespace OpenEngine::Renderers;
using namespace OpenEngine::Resources;
using namespace OpenEngine::Utils;
using namespace OpenEngine::Physics;

// composite rendering view. Uses RenderingView for drawing and
// AcceleratedRenderingView for clipping. 
class MyRenderingView : 
    public RenderingView,
    public AcceleratedRenderingView {
public:
    MyRenderingView(Viewport& viewport) :
        IRenderingView(viewport),
        RenderingView(viewport),
        AcceleratedRenderingView(viewport) {

    }
};

/**
 * Factory constructor.
 *
 * Initializes the different components so they are accessible when
 * the setup method is executed.
 */
GameFactory::GameFactory() {

    // Create a frame and viewport.
    this->frame = new SDLFrame(800, 600, 32);

    // Main viewport
    viewport = new Viewport(*frame);

    // Create a renderer.
    this->renderer = new Renderer();

    // Add a rendering view to the renderer
    this->renderer->AddRenderingView(new MyRenderingView(*viewport));
}

/**
 * Setup handler.
 *
 * This is the main setup method of the game factory. Here you can add
 * any non-standard modules and perform other setup tasks prior to the
 * engine start up.
 *
 * @param engine The game engine instance.
 */
bool GameFactory::SetupEngine(IGameEngine& engine) {

    // Add mouse and keyboard module here
    SDLInput* input = new SDLInput();
    engine.AddModule(*input);

    // Add Statistics module
    engine.AddModule(*(new OpenEngine::Utils::Statistics(1000)));

    // Create a root scene node
    SceneNode* scene = new SceneNode();

    // Supply the scene to the renderer
    this->renderer->SetSceneRoot(scene);

    // Add RenderStateNode to change rendering features at runtime
    RenderStateNode* rNode = new RenderStateNode();
    rNode->AddOptions(RenderStateNode::RENDER_TEXTURES);
    rNode->AddOptions(RenderStateNode::RENDER_SHADERS);
    scene->AddNode(rNode);
    
    // Bind keys for changing rendering state
    RenderStateHandler* renderStateHandler = new RenderStateHandler(rNode);
    renderStateHandler->BindToEventSystem();

    // Bind the quit handler
    QuitHandler* quit_h = new QuitHandler();
    quit_h->BindToEventSystem();

    // Bind the camera to the viewport
    FollowCamera* camera = new FollowCamera( *(new InterpolatedViewingVolume( *(new ViewingVolume()) )));
    Frustum* frustum = new Frustum(*camera, 20, 5000);
    viewport->SetViewingVolume(frustum);

    // Register movement handler to be able to move the camera
    MoveHandler* move = new MoveHandler(*camera);
    move->BindToEventSystem();
    engine.AddModule(*move);
    
    // set the resources directory
    string resources = "projects/OERacer/data/";
    DirectoryManager::AppendPath(resources);
    logger.info << "Resource directory: " << resources << logger.end;

    // load the resource plug-ins
    ResourceManager<IModelResource>::AddPlugin(new OBJPlugin());
    ResourceManager<ITextureResource>::AddPlugin(new TGAPlugin());
    ResourceManager<IShaderResource>::AddPlugin(new GLSLPlugin());

    // pointer to Box transformation node
    RigidBox* box = NULL;
    Vector<3,float> position(2, 100, 2);

    // Add models from models.txt to the scene
    ISceneNode* current = rNode;
    ISceneNode* dynamicObjects = new SceneNode();
    ISceneNode* staticObjects  = new SceneNode();
    ISceneNode* physicObjects  = new SceneNode();
    
    // Open the models file
    ifstream* mfile = File::Open("projects/OERacer/models.txt");
    
    bool dynamic = false;
    while (!mfile->eof()) {
        string mod_str;
        getline(*mfile, mod_str);

        // Check the string
        if (mod_str[0] == '#' || mod_str == "") continue;
        
        // switch to static elements
        if (mod_str == "dynamic") {
            dynamic = true;
            current = dynamicObjects;
            continue;
        }
        else if (mod_str == "static") {
            dynamic = false;
            current = staticObjects;
            continue;
        }
        else if (mod_str == "physic") {
            dynamic = false;
            current = physicObjects;
            continue;
        }
        
        // Load the model
        IModelResourcePtr mod_res = ResourceManager<IModelResource>::Create(mod_str);
        mod_res->Load();
        if( mod_res->GetFaceSet() == NULL ) continue;

        GeometryNode* mod_node = new GeometryNode();
        mod_node->SetFaceSet(mod_res->GetFaceSet());
        mod_res->Unload();

        TransformationNode* mod_tran = new TransformationNode();
        mod_tran->AddNode(mod_node);
        if (dynamic) {
            // Load ridget-box
            box = new RigidBox( Box(*(mod_node->GetFaceSet())) );
            box->SetCenter( position );
            box->SetTransformationNode(mod_tran);

            camera->SetPosition(position + Vector<3,float>(-150,40,0));
            camera->LookAt(position);
            camera->Follow(mod_tran);
        }
        current->AddNode(mod_tran);

        // add the node to the move handler
        move->nodes.push_back(mod_tran);
        logger.info << "Successfully loaded " << mod_str << logger.end;
    }
    mfile->close();
    delete mfile;
    
    rNode->AddNode(dynamicObjects);

    ISceneNode* hybridTreeRoot = new SceneNode();

    ifstream isf("oeracer-physics-scene.bin", ios::binary);
    if (isf.is_open()) {
        logger.info << "Loading the physics tree from file: started" << logger.end;
        Serialization::Deserialize(*hybridTreeRoot, &isf);
        isf.close();
        logger.info << "Loading the physics tree from file: done" << logger.end;
    } else {
        isf.close();
        logger.info << "Creating and serializing the physics tree: started" << logger.end;
        // set the root to the loaded objects
        delete hybridTreeRoot;
        hybridTreeRoot = physicObjects;
        // transform the object tree to a hybrid Quad/BSP
        CollectedGeometryTransformer collT;
        QuadTransformer quadT;
        BSPTransformer bspT;
        collT.Transform(*hybridTreeRoot);
        quadT.Transform(*hybridTreeRoot);
        bspT.Transform(*hybridTreeRoot);
        // serialize the scene
        const ISceneNode& tmp = *hybridTreeRoot;
        ofstream of("oeracer-physics-scene.bin");
        Serialization::Serialize(tmp, &of);
        of.close();
        logger.info << "Creating and serializing the physics tree: done" << logger.end;
    }
    
    // save a dot graph of the tree
    ofstream dotfile("physics.dot", ofstream::out);
    if (!dotfile.good()) {
        logger.error << "Can not open 'physics.dot' "
                     << "for output" << logger.end;
    } else {
        ASDotVisitor dot;
        dot.Write(*hybridTreeRoot, &dotfile);
        logger.info << "Saved physics graph to 'physics.dot'" << logger.end;
    }
    FixedTimeStepPhysics* physic = new FixedTimeStepPhysics( hybridTreeRoot );

    // Add FixedTimeStepPhysics module
    physic->AddRigidBody(box);
    engine.AddModule(*physic, IGameEngine::TICK_DEPENDENT);

    logger.info << "Preprocessing of static tree: started" << logger.end;
    QuadTransformer quadT;
    quadT.SetMaxFaceCount(500);
    quadT.SetMaxQuadSize(100);
    quadT.Transform(*staticObjects);
    rNode->AddNode(staticObjects);
    logger.info << "Preprocessing of static tree: done" << logger.end;

    // Visualization of the frustum
    //frustum->VisualizeClipping(true);
    //rNode->AddNode(frustum->GetFrustumNode());

    // add the RigidBox to the scene, for debuging
    //if (box != NULL) rNode->AddNode( box->GetRigidBoxNode() );
    
    // Keyboard bindings to the rigid box and camera
    KeyboardHandler* keyHandler = new KeyboardHandler(camera,box,physic);
    keyHandler->BindToEventSystem();
    engine.AddModule(*keyHandler);

    // Return true to signal success.
    return true;
}

// Other factory methods. The returned objects are all created in the
// factory constructor.
IFrame*      GameFactory::GetFrame()    { return this->frame;    }
IRenderer*   GameFactory::GetRenderer() { return this->renderer; }
