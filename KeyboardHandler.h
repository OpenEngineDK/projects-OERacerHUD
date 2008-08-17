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
using OpenEngine::Devices::KeyboardEventArg;
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
                    FixedTimeStepPhysics* physics)
        : up(0)
        , down(0)
        , left(0)
        , right(0)
        , camera(camera)
        , box(box)
        , physics(physics)
        , engine(engine)
    {}

    void Handle(InitializeEventArg arg) {
        step = 0.0f;
        timer.Start();
    }
    void Handle(DeinitializeEventArg arg) {}
    void Handle(ProcessEventArg arg) {

        float delta = (float) timer.GetElapsedTimeAndReset().AsInt() / 100000;

        if (box == NULL || !( up || down || left || right )) return;

        static float speed = 1750.0f;
        static float turn = 550.0f;
        Matrix<3,3,float> m(box->GetRotationMatrix());

        // Forward 
        if( up ){
            Vector<3,float> dir = m.GetRow(0) * delta;
            box->AddForce(dir * speed*up, 1);
            box->AddForce(dir * speed*up, 2);
            box->AddForce(dir * speed*up, 3);
            box->AddForce(dir * speed*up, 4);
        }
        if( down ){
            Vector<3,float> dir = -m.GetRow(0) * delta;
            box->AddForce(dir * speed*down, 5);
            box->AddForce(dir * speed*down, 6);
            box->AddForce(dir * speed*down, 7);
            box->AddForce(dir * speed*down, 8);
        }
        if( left ){
            Vector<3,float> dir = -m.GetRow(2) * delta;
            box->AddForce(dir * turn*left, 2);
            box->AddForce(dir * turn*left, 4);
        }
        if( right ) {
            Vector<3,float> dir = m.GetRow(2) * delta;
            box->AddForce(dir * turn*right, 1);
            box->AddForce(dir * turn*right, 3);
        }
    }

    void Handle(KeyboardEventArg arg) {
        (arg.type == KeyboardEventArg::PRESS) ? KeyDown(arg) : KeyUp(arg);
    }

    void KeyDown(KeyboardEventArg arg) {
        switch ( arg.sym ) {
        case keys::KEY_r: {
            physics->Handle(InitializeEventArg());
            if( physics != NULL ){
                if( box != NULL ) {
                    box->ResetForces();
                    box->SetCenter( Vector<3,float>(2, 1, 2) );
                    logger.info << "Reset Physics" << logger.end;
                }
            }
            break;
        }

        case keys::KEY_SPACE:{
            if( physics != NULL ){
                physics->TogglePause();
            }
            break;
        }
        // Move the car forward
        case keys::KEY_UP:    up    = 1; break;
        case keys::KEY_DOWN:  down  = 1; break;
        case keys::KEY_LEFT:  left  = 1; break;
        case keys::KEY_RIGHT: right = 1; break;

        // Log Camera position 
        case keys::KEY_c: {
            Vector<3,float> camPos = camera->GetPosition();
            logger.info << "Camera Position: " << camPos << logger.end;
            break;
        }

        // Increase/decrease time in Physic
        case keys::KEY_PLUS:  mod = true; step =  0.001f; break;
        case keys::KEY_MINUS: mod = true; step = -0.001f; break;
        

        // Quit on Escape
        case keys::KEY_ESCAPE:
            engine.Stop();
            break;
        default: break;
        }
    }

    void KeyUp(KeyboardEventArg arg) {
        switch ( arg.sym ) {
        case keys::KEY_UP:    up    = 0; break;
        case keys::KEY_DOWN:  down  = 0; break;
        case keys::KEY_LEFT:  left  = 0; break;
        case keys::KEY_RIGHT: right = 0; break;
        case keys::KEY_PLUS:  mod   = false; break;
        case keys::KEY_MINUS: mod   = false; break;

        default: break;
        }
    }

void Handle(JoystickButtonEventArg arg) {
    
    switch (arg.button) {
    case keys::JBUTTON_FOUR: {
	physics->Handle(InitializeEventArg());
	if( physics != NULL ){
	    if( box != NULL ) {
		box->ResetForces();
		box->SetCenter( Vector<3,float>(2, 1, 2) );
		logger.info << "Reset Physics" << logger.end;
	    }
	}
	break;
    }
    
    default:
	break; // none
    }

    logger.info << "joy: " << arg.button << logger.end;

}

void Handle(JoystickAxisEventArg arg) {

    float max = 1 << 15;

    up = (-arg.state.axisState[1])/max;
    down = (arg.state.axisState[1])/max;

    left = (-arg.state.axisState[0])/max;
    right = (arg.state.axisState[0])/max;
    
}



};

#endif
