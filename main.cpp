// Main setup for the OpenEngine Racer project.
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS) 
// 
// This program is free software; It is covered by the GNU General 
// Public License version 2 or any later version. 
// See the GNU General Public License for more details (see LICENSE). 
//--------------------------------------------------------------------

// OpenEngine stuff
#include <Meta/Config.h>

// Serialization (must be first)
#include <fstream>
#include <Utils/Serialization.h>

// Core structures
#include <Core/Engine.h>

// Display structures
#include <Display/FollowCamera.h>
#include <Display/Frustum.h>
#include <Display/InterpolatedViewingVolume.h>
#include <Display/ViewingVolume.h>
// SDL implementation
#include <Display/SDLFrame.h>
#include <Devices/SDLInput.h>

// Rendering structures
#include <Renderers/IRenderNode.h>
// OpenGL rendering implementation
#include <Renderers/OpenGL/Renderer.h>
#include <Renderers/OpenGL/RenderingView.h>
#include <Renderers/OpenGL/TextureLoader.h>

// Resources
#include <Resources/IModelResource.h>
#include <Resources/File.h>
#include <Resources/DirectoryManager.h>
#include <Resources/ResourceManager.h>
// OBJ and TGA plugins
#include <Resources/TGAResource.h>
#include <Resources/OBJResource.h>

// Scene structures
#include <Scene/SceneNode.h>
#include <Scene/GeometryNode.h>
#include <Scene/TransformationNode.h>
#include <Scene/VertexArrayTransformer.h>
#include <Scene/DisplayListTransformer.h>
#include <Scene/PointLightNode.h>
// AccelerationStructures extension
#include <Scene/CollectedGeometryTransformer.h>
#include <Scene/QuadTransformer.h>
#include <Scene/BSPTransformer.h>
#include <Scene/ASDotVisitor.h>
#include <Renderers/AcceleratedRenderingView.h>

// Utilities and logger
#include <Logging/Logger.h>
#include <Logging/StreamLogger.h>
#include <Utils/MoveHandler.h>
#include <Utils/QuitHandler.h>
#include <Utils/Statistics.h>

// FixedTimeStepPhysics extension
#include <Physics/FixedTimeStepPhysics.h>
#include <Physics/RigidBox.h>

// OERacer utility files
#include "KeyboardHandler.h"

// Additional namespaces
using namespace OpenEngine::Core;
using namespace OpenEngine::Logging;
using namespace OpenEngine::Devices;
using namespace OpenEngine::Renderers::OpenGL;
using namespace OpenEngine::Renderers;
using namespace OpenEngine::Resources;
using namespace OpenEngine::Utils;
using namespace OpenEngine::Physics;

// Configuration structure to pass around to the setup methods
struct Config {
    IEngine&              engine;
    IFrame*               frame;
    Viewport*             viewport;
    IViewingVolume*       viewingvolume;
    FollowCamera*         camera;
    Frustum*              frustum;
    IRenderer*            renderer;
    IMouse*               mouse;
    IKeyboard*            keyboard;
    ISceneNode*           renderingScene;
    ISceneNode*           dynamicScene;
    ISceneNode*           staticScene;
    ISceneNode*           physicScene;
    RigidBox*             physicBody;
    FixedTimeStepPhysics* physics;
    bool                  resourcesLoaded;
    Config(IEngine& engine)
        : engine(engine)
        , frame(NULL)
        , viewport(NULL)
        , viewingvolume(NULL)
        , camera(NULL)
        , frustum(NULL)
        , renderer(NULL)
        , mouse(NULL)
        , keyboard(NULL)
        , renderingScene(NULL)
        , dynamicScene(NULL)
        , staticScene(NULL)
        , physicScene(NULL)
        , physics(NULL)
        , resourcesLoaded(false)
    {}
};

// Forward declaration of the setup methods
void SetupResources(Config&);
void SetupDisplay(Config&);
void SetupScene(Config&);
void SetupPhysics(Config&);
void SetupRendering(Config&);
void SetupDevices(Config&);
void SetupDebugging(Config&);

int main(int argc, char** argv) {
    // Setup logging facilities.
    Logger::AddLogger(new StreamLogger(&std::cout));

    // Print usage info.
    logger.info << "========= Running The OpenEngine Racer Project =========" << logger.end;
    logger.info << "This project is a simple testing project for OpenEngine." << logger.end;
    logger.info << "It uses an old physics system and is not very stable." << logger.end;
    logger.info << logger.end;
    logger.info << "Vehicle controls:" << logger.end;
    logger.info << "  drive forwards:  up-arrow" << logger.end;
    logger.info << "  drive backwards: down-arrow" << logger.end;
    logger.info << "  turn left:       left-arrow" << logger.end;
    logger.info << "  turn right:      right-arrow" << logger.end;
    logger.info << logger.end;
    logger.info << "Camera controls:" << logger.end;
    logger.info << "  move forwards:   w" << logger.end;
    logger.info << "  move backwards:  s" << logger.end;
    logger.info << "  move left:       a" << logger.end;
    logger.info << "  move right:      d" << logger.end;
    logger.info << "  rotate:          mouse" << logger.end;
    logger.info << logger.end;

    // Create an engine and config object
    Engine* engine = new Engine();
    Config config(*engine);

    // Setup the engine
    SetupResources(config);
    SetupDisplay(config);
    SetupScene(config);
    SetupPhysics(config);
    SetupRendering(config);
    SetupDevices(config);
    
    // Possibly add some debugging stuff
    // SetupDebugging(config);

    // Start up the engine.
    engine->Start();

    // Return when the engine stops.
    delete engine;
    return EXIT_SUCCESS;
}

void SetupResources(Config& config) {
    // set the resources directory
    // @todo we should check that this path exists
    string resources = "projects/OERacer/data/";
    DirectoryManager::AppendPath(resources);

    // load resource plug-ins
    ResourceManager<IModelResource>::AddPlugin(new OBJPlugin());
    ResourceManager<ITextureResource>::AddPlugin(new TGAPlugin());

    config.resourcesLoaded = true;
}

void SetupDisplay(Config& config) {
    if (config.frame         != NULL ||
        config.viewingvolume != NULL ||
        config.camera        != NULL ||
        config.frustum       != NULL ||
        config.viewport      != NULL)
        throw Exception("Setup display dependencies are not satisfied.");

    config.frame         = new SDLFrame(800, 600, 32);
    config.viewingvolume = new InterpolatedViewingVolume(*(new ViewingVolume()));
    config.camera        = new FollowCamera( *config.viewingvolume );
    config.frustum       = new Frustum(*config.camera, 20, 3000);
    config.viewport      = new Viewport(*config.frame);
    config.viewport->SetViewingVolume(config.frustum);

    config.engine.InitializeEvent().Attach(*config.frame);
    config.engine.ProcessEvent().Attach(*config.frame);
    config.engine.DeinitializeEvent().Attach(*config.frame);
}

void SetupRendering(Config& config) {
    if (config.viewport == NULL ||
        config.renderer != NULL ||
        config.renderingScene == NULL)
        throw Exception("Setup renderer dependencies are not satisfied.");

    // Composite rendering view via. multiple inheritance.
    // Uses RenderingView for drawing and AcceleratedRenderingView for clipping. 
    class MyRenderingView : 
        public RenderingView,
        public AcceleratedRenderingView {
    public:
        MyRenderingView(Viewport& viewport)
            : IRenderingView(viewport)
            , RenderingView(viewport)
            , AcceleratedRenderingView(viewport) {}
    };

    // Create a renderer
    config.renderer = new Renderer();

    // Setup a rendering view
    MyRenderingView* rv = new MyRenderingView(*config.viewport);
    config.renderer->ProcessEvent().Attach(*rv);

    // Add rendering initialization tasks
    TextureLoader* tl = new TextureLoader();
    DisplayListTransformer* dlt = new DisplayListTransformer(rv);
    config.renderer->InitializeEvent().Attach(*tl);
    config.renderer->InitializeEvent().Attach(*dlt);

    // Transform the scene to use vertex arrays
    VertexArrayTransformer vaT;
    vaT.Transform(*config.renderingScene);

    // Supply the scene to the renderer
    config.renderer->SetSceneRoot(config.renderingScene);

    config.engine.InitializeEvent().Attach(*config.renderer);
    config.engine.ProcessEvent().Attach(*config.renderer);
    config.engine.DeinitializeEvent().Attach(*config.renderer);
}

void SetupDevices(Config& config) {
    if (config.mouse    != NULL ||
        config.keyboard != NULL ||
        config.camera  == NULL ||
        config.physics  == NULL ||
        config.physicBody == NULL)
        throw Exception("Setup keyboard dependencies are not satisfied.");

    // Create the mouse and keyboard input modules
    SDLInput* input = new SDLInput();
    config.engine.InitializeEvent().Attach(*input);
    config.engine.ProcessEvent().Attach(*input);
    config.engine.DeinitializeEvent().Attach(*input);
    config.keyboard = input;
    config.mouse    = input;

    // Bind the quit handler
    QuitHandler* quit_h = new QuitHandler(config.engine);
    config.keyboard->KeyEvent().Attach(*quit_h);

    // Register movement handler to be able to move the camera
    MoveHandler* move_h = new MoveHandler(*config.camera, *config.mouse);
    config.keyboard->KeyEvent().Attach(*move_h);

    // Keyboard bindings to the rigid box and camera
    KeyboardHandler* keyHandler = new KeyboardHandler(config.engine,
                                                      config.camera,
                                                      config.physicBody,
                                                      config.physics);
    config.keyboard->KeyEvent().Attach(*keyHandler);

    config.engine.InitializeEvent().Attach(*keyHandler);
    config.engine.ProcessEvent().Attach(*keyHandler);
    config.engine.DeinitializeEvent().Attach(*keyHandler);

    config.engine.InitializeEvent().Attach(*move_h);
    config.engine.ProcessEvent().Attach(*move_h);
    config.engine.DeinitializeEvent().Attach(*move_h);
}

void SetupPhysics(Config& config) {
    if (config.physicBody  == NULL ||
        config.physicScene == NULL)
        throw Exception("Physics dependencies are not satisfied.");

    ifstream isf("oeracer-physics-scene.bin", ios::binary);
    if (isf.is_open()) {
        logger.info << "Loading the physics tree from file: started" << logger.end;
        delete config.physicScene;
        config.physicScene = new SceneNode();
        Serialization::Deserialize(*config.physicScene, &isf);
        isf.close();
        logger.info << "Loading the physics tree from file: done" << logger.end;
    } else {
        isf.close();
        logger.info << "Creating and serializing the physics tree: started" << logger.end;
        // transform the object tree to a hybrid Quad/BSP
        CollectedGeometryTransformer collT;
        QuadTransformer quadT;
        BSPTransformer bspT;
        collT.Transform(*config.physicScene);
        quadT.Transform(*config.physicScene);
        bspT.Transform(*config.physicScene);
        // serialize the scene
        const ISceneNode& tmp = *config.physicScene;
        ofstream of("oeracer-physics-scene.bin");
        Serialization::Serialize(tmp, &of);
        of.close();
        logger.info << "Creating and serializing the physics tree: done" << logger.end;
    }
    
    config.physics = new FixedTimeStepPhysics(config.physicScene);

    // Add physic bodies
    config.physics->AddRigidBody(config.physicBody);

    // Add to engine for processing time (with its timer)
    FixedTimeStepPhysicsTimer* ptimer = new FixedTimeStepPhysicsTimer(*config.physics);
    config.engine.InitializeEvent().Attach(*config.physics);
    config.engine.ProcessEvent().Attach(*ptimer);
    config.engine.DeinitializeEvent().Attach(*config.physics);
}

void SetupScene(Config& config) {
    if (config.dynamicScene    != NULL ||
        config.staticScene     != NULL ||
        config.physicScene     != NULL ||
        config.renderingScene  != NULL ||
        config.resourcesLoaded == false)
        throw Exception("Setup scene dependencies are not satisfied.");

    // Create scene nodes
    config.renderingScene = new SceneNode();
    config.dynamicScene = new SceneNode();
    config.staticScene = new SceneNode();
    config.physicScene = new SceneNode();

    config.renderingScene->AddNode(config.dynamicScene);
    config.renderingScene->AddNode(config.staticScene);

    ISceneNode* current = config.dynamicScene;

    // Position of the vehicle
    Vector<3,float> position(2, 100, 2);

    // Add models from models.txt to the scene
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
            current = config.dynamicScene;
            continue;
        }
        else if (mod_str == "static") {
            dynamic = false;
            current = config.staticScene;
            continue;
        }
        else if (mod_str == "physic") {
            dynamic = false;
            current = config.physicScene;
            continue;
        }
        
        // Load the model
        IModelResourcePtr mod_res = ResourceManager<IModelResource>::Create(mod_str);
        mod_res->Load();
        if (mod_res->GetSceneNode() == NULL) continue;

        ISceneNode* mod_node = mod_res->GetSceneNode();
        mod_res->Unload();

        TransformationNode* mod_tran = new TransformationNode();
        mod_tran->AddNode(mod_node);
        if (dynamic) {
            // Load riget-box
            config.physicBody = new RigidBox( Box(*mod_node));
            config.physicBody->SetCenter( position );
            config.physicBody->SetTransformationNode(mod_tran);
	    config.physicBody->SetGravity(Vector<3,float>(0, -9.82*20, 0));
            // Bind the follow camera
            config.camera->SetPosition(position + Vector<3,float>(-150,40,0));
            config.camera->LookAt(position - Vector<3,float>(0,30,0));
            config.camera->Follow(mod_tran);
            // Set up a light node
            PointLightNode* pln = new PointLightNode();
            pln->constAtt = 0.5;
            pln->linearAtt = 0.001;
            pln->quadAtt = 0.0001;
            mod_tran->AddNode(pln);
        }
        current->AddNode(mod_tran);
        logger.info << "Successfully loaded " << mod_str << logger.end;
    }
    mfile->close();
    delete mfile;

    QuadTransformer quadT;
    quadT.SetMaxFaceCount(500);
    quadT.SetMaxQuadSize(100);
    quadT.Transform(*config.staticScene);
}

void SetupDebugging(Config& config) {

    // Visualization of the frustum
    if (config.frustum != NULL) {
        config.frustum->VisualizeClipping(true);
        config.renderingScene->AddNode(config.frustum->GetFrustumNode());
    }

    // add the RigidBox to the scene, for debuging
    if (config.physicBody != NULL) {
        config.renderingScene->AddNode(config.physicBody->GetRigidBoxNode());
    }

    // Add Statistics module
    config.engine.ProcessEvent().Attach(*(new OpenEngine::Utils::Statistics(1000)));

    // Create dot graphs of the various scene graphs
    map<string, ISceneNode*> scenes;
    scenes["dynamicScene"] = config.dynamicScene;
    scenes["staticScene"]  = config.staticScene;
    scenes["physicScene"]  = config.physicScene;

    map<string, ISceneNode*>::iterator itr;
    for (itr = scenes.begin(); itr != scenes.end(); itr++) {
        ofstream dotfile((itr->first+".dot").c_str(), ofstream::out);
        if (!dotfile.good()) {
            logger.error << "Can not open '"
                         << itr->first << ".dot' "
                         << "for output" << logger.end;
        } else {
            ASDotVisitor dot;
            dot.Write(*itr->second, &dotfile);
            logger.info << "Saved physics graph to '"
                        << itr->first << ".dot'" << logger.end;
        }
    }
}
