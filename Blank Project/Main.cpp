#include "../nclgl/window.h"
#include "Renderer.h"

int main() 
{
	Window w("All aboard!", 1280, 720, false);
	
	if (!w.HasInitialised()) {
		return -1;
	}

	Renderer renderer(w);
	if (!renderer.HasInitialised()) {
		return -1;
	}

	w.LockMouseToWindow(true);
	w.ShowOSPointer(false);
	
    int camera = 2;
    int scene = 1;

    while (w.UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE))
    {
        if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_1)) {
            camera = 1;
        }
        if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_2)) {
            camera = 2;
        }
        if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_3)) {
            scene = 1;
        }
        if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_4)) {
            scene = 2;
        }
        if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_5)) {
            scene = 3;
        }  
        if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_6)) {
            scene = 4;
        }
        float timestep = w.GetTimer()->GetTimeDeltaSeconds();

        switch (camera)
        {
        case 1:
            renderer.UpdateScene(timestep);
            renderer.NpcMove(timestep);
            break;
        case 2:
            renderer.AutoUpdateCamera(timestep);
            renderer.NpcMove(timestep);
            break;
        }

        switch (scene)
        {
        case 1:
            renderer.RenderScene();
            renderer.SwapBuffers();
            if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
                Shader::ReloadAllShaders();
            }
            break;
        case 2:
            renderer.RederDeferredScene();
            renderer.SwapBuffers();
            if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
                Shader::ReloadAllShaders();
            }
            break;
        case 3:
            renderer.RenderSplitScene();
            renderer.SwapBuffers();
            if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
                Shader::ReloadAllShaders();
            }
            break;
        case 4:
            renderer.RenderBlurScene();
            renderer.SwapBuffers();
            if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
                Shader::ReloadAllShaders();
            }
            break;
        }

    }
	return 0;
}