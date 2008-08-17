#include "KeyboardHandler.h"

#include <Logging/Logger.h>


KeyboardHandler::KeyboardHandler(IEngine& engine,
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


void KeyboardHandler::Handle(InitializeEventArg arg) {
        step = 0.0f;
        timer.Start();
    }
void KeyboardHandler::Handle(DeinitializeEventArg arg) {}
void KeyboardHandler::Handle(ProcessEventArg arg) {

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

void KeyboardHandler::Handle(KeyboardEventArg arg) {
        (arg.type == KeyboardEventArg::PRESS) ? KeyDown(arg) : KeyUp(arg);
    }

void KeyboardHandler::KeyDown(KeyboardEventArg arg) {
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

void KeyboardHandler::KeyUp(KeyboardEventArg arg) {
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

void KeyboardHandler::Handle(JoystickButtonEventArg arg) {
    
    switch (arg.button) {
    case keys::JBUTTON_TWO: {
	if (arg.type == JoystickButtonEventArg::PRESS) {
	    box->AddForce(Vector<3,float>(0,5000,0));
	}
	break;
    }
    case keys::JBUTTON_TEN: {
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
    case keys::JBUTTON_SEVEN: {
	if (arg.type == JoystickButtonEventArg::PRESS) {
	    Vector<3,float> g = box->GetGravity();
	    box->SetGravity(g*10.0);
	    logger.info << "Gravity " << g << logger.end;
	}
	break;
    }
    case keys::JBUTTON_EIGHT: {
	if (arg.type == JoystickButtonEventArg::PRESS) {
	    Vector<3,float> g = box->GetGravity();
	    box->SetGravity(g/10.0);
	    logger.info << "Gravity " << g << logger.end;
	}
	break;
    }
    default:
	break; // none
    }

    logger.info << "joy: " << arg.button << logger.end;
}

void KeyboardHandler::Handle(JoystickAxisEventArg arg) {
    float max = 1 << 15;

    up = (-arg.state.axisState[1])/max;
    down = (arg.state.axisState[1])/max;

    left = (-arg.state.axisState[0])/max;
    right = (arg.state.axisState[0])/max;
    
}
