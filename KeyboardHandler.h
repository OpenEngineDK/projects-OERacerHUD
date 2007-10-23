#ifndef _KEYBOARD_HANDLER_
#define _KEYBOARD_HANDLER_

#include <Devices/IKeyboard.h>
#include <Display/Camera.h>
#include <Physics/FixedTimeStepPhysics.h>
#include <Physics/RigidBox.h>

using namespace OpenEngine::Devices;
using namespace OpenEngine::Physics;

class KeyboardHandler : public IModule {
private:
    bool up, down, left, right, mod;
    float step;
    Camera* camera;
    RigidBox* box;
    FixedTimeStepPhysics* physics;
public:
    KeyboardHandler(Camera* camera, RigidBox* box, FixedTimeStepPhysics* physics)
        : up(false), down(false), left(false), right(false), camera(camera), box(box), physics(physics) {}

    bool IsTypeOf(const std::type_info& inf) { return typeid(KeyboardHandler) == inf; }
    void Initialize() {
        step = 0.0f;
    }
    void Deinitialize() {}
    void Process(const float deltaTime, const float percent) {
        if( box == NULL ) return;
        static float speed = 1750.0f;
        static float turn = 550.0f;
        Matrix<3,3,float> m; //orientation
        if( up || down || left || right ) m = box->GetRotationMatrix();

        float delta = deltaTime / 1000 * 8;
        // Forward 
        if( up ){
            Vector<3,float> dir = m.GetRow(0) * delta;
            box->AddForce(dir * speed, 1);
            box->AddForce(dir * speed, 2);
            box->AddForce(dir * speed, 3);
            box->AddForce(dir * speed, 4);
        }
        if( down ){
            Vector<3,float> dir = -m.GetRow(0) * delta;
            box->AddForce(dir * speed, 5);
            box->AddForce(dir * speed, 6);
            box->AddForce(dir * speed, 7);
            box->AddForce(dir * speed, 8);
        }
        if( left ){
            Vector<3,float> dir = -m.GetRow(2) * delta;
            box->AddForce(dir * turn, 2);
            box->AddForce(dir * turn, 4);
        }
        if( right ) {
            Vector<3,float> dir = m.GetRow(2) * delta;
            box->AddForce(dir * turn, 1);
            box->AddForce(dir * turn, 3);
        }
    }

    void KeyDown(KeyboardEventArg arg) {
        switch ( arg.sym ) {
        case KEY_r: {
            physics->Initialize();
            if( physics != NULL ){
                if( box != NULL ) {
                    box->ResetForces();
                    box->SetCenter( Vector<3,float>(2, 1, 2) );
                    logger.info << "Reset Physics" << logger.end;
                }
            }
            break;
        }

        case KEY_SPACE:{
            if( physics != NULL ){
                physics->TogglePause();
            }
            break;
        }
        // Move the car forward
        case KEY_UP:    up    = true; break;
        case KEY_DOWN:  down  = true; break;
        case KEY_LEFT:  left  = true; break;
        case KEY_RIGHT: right = true; break;

        // Log Camera position 
        case KEY_c: {
            Vector<3,float> camPos = camera->GetPosition();
            logger.info << "Camera Position: " << camPos << logger.end;
            break;
        }

        // Increase/decrease time in Physic
        case KEY_PLUS:  mod = true; step =  0.001f; break;
        case KEY_MINUS: mod = true; step = -0.001f; break;
        

        // Quit on Escape
        case KEY_ESCAPE:
            IGameEngine::Instance().Stop();
            break;
        default: break;
        }
    }

    void KeyUp(KeyboardEventArg arg) {
        switch ( arg.sym ) {
        case KEY_UP:    up    = false; break;
        case KEY_DOWN:  down  = false; break;
        case KEY_LEFT:  left  = false; break;
        case KEY_RIGHT: right = false; break;
        case KEY_PLUS:  mod   = false; break;
        case KEY_MINUS: mod   = false; break;

        default: break;
        }
    }
    
    void BindToEventSystem() {
        Listener<KeyboardHandler, KeyboardEventArg>* keyDown 
            = new Listener<KeyboardHandler, KeyboardEventArg> (*this, &KeyboardHandler::KeyDown);
        Listener<KeyboardHandler, KeyboardEventArg>* keyUp 
            = new Listener<KeyboardHandler, KeyboardEventArg> (*this, &KeyboardHandler::KeyUp);
        IKeyboard::keyDownEvent.Add(keyDown);
        IKeyboard::keyUpEvent.Add(keyUp);
    }
};

#endif
