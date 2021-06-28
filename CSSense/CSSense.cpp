#include "pch.hpp"

#include "GameStateIntegration.hpp"

int main(int argc, char** argv)
{
	try
	{
		TCLAP::CmdLine cmd("CSSense CLI allows you to experience CSGO like never before", ' ', "0.1");

		TCLAP::SwitchArg initSwitch("i", "init", "Initialize CSSense", cmd, false);

		cmd.parse(argc, argv);

		GameStateIntegration gSI(initSwitch.getValue());

		gSI.open().wait();

		std::cout << "Press ENTER to exit." << std::endl;

		std::string line;
		std::getline(std::cin, line);
		gSI.close().wait();
	}
	catch (TCLAP::ArgException& e)
	{
		std::cerr << e.error() << " for arg " << e.argId() << std::endl;
	}
}