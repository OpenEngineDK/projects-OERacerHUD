#ifndef _KEYBOARD_HANDLER_
#define _KEYBOARD_HANDLER_

#include <Core/IListener.h>
#include <Core/IEngine.h>
#include <Devices/IKeyboard.h>
#include <Devices/IJoystick.h>
#include <Devices/Symbols.h>
#include <Display/Camera.h>
#include <Physics/FixedTimeStepPhysics.h>
#include <Physics/RigidBox.h>
#include <Math/Matrix.h>
#include <Utils/Timer.h>

using OpenEngine::Core::IModule;
using OpenEngine::Core::IListener;
using OpenEngine::Core::IEngine;
using OpenEngine::Core::InitializeEventArg;
using OpenEngine::Core::ProcessEventArg;
using OpenEngine::Core::DeinitializeEventArg;
using namespace OpenEngine::Devices;
using OpenEngine::Display::Camera;
using OpenEngine::Physics::RigidBox;
using OpenEngine::Physics::FixedTimeStepPhysics;
using OpenEngine::Math::Matrix;
using OpenEngine::Utils::Timer;

namespace keys = OpenEngine::Devices;

class KeyboardHandler : public IModule, public IListener<KeyboardEventArg>,
			public IListener<JoystickButtonEventArg>,
			public IListener<JoystickAxisEventArg> {
    
private:
    float up, down, left, right, mod;
    float step;
    Camera* camera;
    RigidBox* box;
    FixedTimeStepPhysics* physics;
    IEngine& engine;
    Timer timer;

public:
    KeyboardHandler(IEngine& engine,
                    Camera* camera,
                    RigidBox* box,
                    FixedTimeStepPhysics* physics);

    void Handle(InitializeEventArg arg);
    void Handle(DeinitializeEventArg arg);
    void Handle(ProcessEventArg arg);
    void Handle(KeyboardEventArg arg);
    void KeyDown(KeyboardEventArg arg);
    void KeyUp(KeyboardEventArg arg);
    void Handle(JoystickButtonEventArg arg);
    void Handle(JoystickAxisEventArg arg);


};

#endif
