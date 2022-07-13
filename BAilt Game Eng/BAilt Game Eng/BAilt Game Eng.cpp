
#include <iostream>

#include "ConfigLoader.h"

#include "MasterGraphicsHandler.h"
#include "ObjectHandler3D.h"
#include "ObjectHandler2D.h"

#include "InputHandler.h"

#include "ScriptHandler.h"

#include "raylib.h"

std::string ConfigFilePath = "./config.txt";
std::string WindowName = "BAilt Engine";
std::string ScriptDirectory = "D:/";

double previousTime = GetTime();
double* timeStep_ptr = new double;

ConfigLoader MainConfigLoader(ConfigFilePath);

ObjectHandler2D MainObjHandler2D;
ObjectHandler3D MainObjHandler3D;
MasterGraphicsHandler MainMasterGraphicsHandler(&MainConfigLoader, &MainObjHandler2D, &MainObjHandler3D, WindowName);

InputHandler MainInptHandler;

ScriptHandler MainScriptHandler;

int main()
{
	//Prevent window from closing when ESC is pressed
	SetExitKey(NULL);

	//Give the timestep an initial value
	*timeStep_ptr = previousTime;

	MainInptHandler.SetConfigLoaderPTR(&MainConfigLoader);

	//init Script Handler, will probably all be moved to constructor sometime. for now cutting down on constructor args
	MainScriptHandler.SetTimeStepPTR(timeStep_ptr);
	MainScriptHandler.SetInputHandlerPTR(&MainInptHandler);
	MainScriptHandler.SetMasterGraphicsHandlerPTR(&MainMasterGraphicsHandler);
	MainScriptHandler.SetObjHandler3DPTR(&MainObjHandler3D);
	MainScriptHandler.SetScriptFileDirectory(&ScriptDirectory);
	MainScriptHandler.RunBootScript();

	//Main update loop
	while (!WindowShouldClose())
	{
		MainMasterGraphicsHandler.UpdateScreen();

		MainObjHandler3D.Update();

		MainInptHandler.Update();

		MainScriptHandler.Update();

		//Update time step
		*timeStep_ptr = GetTime() - previousTime;
		previousTime = GetTime();
	}
}
