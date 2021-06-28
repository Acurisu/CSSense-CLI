#include "pch.hpp"

#include "GameStateIntegration.hpp"

void GameStateIntegration::sendVibrate(int strength)
{
	std::wstring cmd = L"Vibrate:" + std::to_wstring(strength) + L";";

#ifndef NO_BLE
	if (!bLEController.GattWrite(cmd))
	{
		std::cerr << "Failed sending a command to the device" << std::endl;
	}
#endif

#ifdef _DEBUG
	std::wcout << DBG_START << L"Sent: " << cmd << DBG_END << std::endl;
#endif
}

void GameStateIntegration::handleJSON(web::json::value msg)
{
	if (msg.has_object_field(L"previously") && msg.has_object_field(L"player"))
	{
		web::json::value player = msg.at(L"player");
		web::json::value previously = msg.at(L"previously");

		if (previously.has_object_field(L"player"))
		{
			web::json::value previousPlayer = previously.at(L"player");

			if (previously.has_boolean_field(L"map"))
			{
				if (previously.at(L"map").as_bool())
				{
#ifdef EVENTS
					std::cout << EVT("Map unload") << std::endl;
#endif

					currentStrength = config.baseVibration;
					isMVP = false;
					isDead = false;
					sendVibrate(config.baseVibration);
					return;
				}
			}

			if (previously.has_object_field(L"round"))
			{
				web::json::value round = previously.at(L"round");
				if (round.has_string_field(L"phase") && round.at(L"phase").as_string() == L"over")
				{
#ifdef EVENTS
					std::cout << EVT("Round reset") << std::endl;
#endif

					currentStrength = config.baseVibration;
					isMVP = false;
					isDead = false;
					sendVibrate(config.baseVibration);
					return;
				}
			}

			if (isDead || (isMVP && config.burstOnMVP))
			{
				return;
			}

			if (previousPlayer.has_object_field(L"match_stats"))
			{
				web::json::value match_stats = previousPlayer.at(L"match_stats");
				if (config.burstOnMVP)
				{
					if (match_stats.has_integer_field(L"mvps"))
					{
#ifdef EVENTS
						std::cout << EVT("MVP") << std::endl;
#endif

						isMVP = true;
						sendVibrate(20);
						return;
					}
				}

				if (match_stats.has_integer_field(L"deaths"))
				{
#ifdef EVENTS
					std::cout << EVT("Dead") << std::endl;
#endif

					isDead = true;
					currentStrength = config.baseVibration;
					sendVibrate(config.baseVibration);
					return;
				}
			}

			if (config.increaseOnKill)
			{
				if (previousPlayer.has_object_field(L"state") && player.has_object_field(L"state"))
				{
					web::json::value state = player.at(L"state");

					if (previousPlayer.at(L"state").has_integer_field(L"round_kills") && state.has_integer_field(L"round_kills"))
					{
#ifdef EVENTS
						std::cout << EVT("Kill") << std::endl;
#endif

						web::json::value round_kills = state.at(L"round_kills");
						int strength = static_cast<int>(config.baseVibration + round_kills.as_integer() * config.increaseAmount);
						currentStrength = strength;
						sendVibrate(std::clamp(strength, 0, 20));
					}
				}
			}

			if (config.stopOnKnife || config.vibrateOnShoot)
			{
				if (previousPlayer.has_object_field(L"weapons") && player.has_object_field(L"weapons"))
				{
					web::json::value previousWeapons = previousPlayer.at(L"weapons");
					web::json::object weapons = player.at(L"weapons").as_object();
					for (auto it = weapons.begin(); it != weapons.end(); ++it)
					{
						std::wstring name = it->first;
						web::json::value weapon = it->second;

						if (weapon.has_string_field(L"state") && weapon.at(L"state").as_string() == L"active")
						{
							if (previousWeapons.has_object_field(name))
							{
								web::json::value previousWeapon = previousWeapons.at(name);

								if (config.vibrateOnShoot)
								{
									if (previousWeapon.has_integer_field(L"ammo_clip") && weapon.has_integer_field(L"ammo_clip") &&
										!previousWeapon.has_string_field(L"name") &&
										previousWeapon.at(L"ammo_clip").as_integer() - 1 == weapon.at(L"ammo_clip").as_integer())
									{
#ifdef EVENTS
										std::cout << EVT("Shot") << std::endl;
#endif

										lastShot = std::chrono::steady_clock::now();
										sendVibrate(currentStrength);

										pplx::wait(100);

										auto timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - lastShot).count();
										if (timeElapsed > 100 && !isMVP && config.vibrateOnShoot)
										{
#ifdef _DEBUG
											std::cout << DBG("Reset") << std::endl;
#endif

											sendVibrate(config.baseVibration);
										}
									}
								}
								else if (config.stopOnKnife)
								{
									if (previousWeapon.has_string_field(L"state") && previousWeapon.at(L"state").as_string() == L"holstered")
									{
										if (weapon.has_string_field(L"type") && weapon.at(L"type").as_string() == L"Knife")
										{
#ifdef EVENTS
											std::cout << EVT("Knife active") << std::endl;
#endif

											sendVibrate(config.baseVibration);
											return;
										}
#ifdef EVENTS
										std::cout << EVT("Knife holstered") << std::endl;
#endif

										sendVibrate(currentStrength);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void GameStateIntegration::handlePost(http_request message)
{
#ifdef _DEBUG
	std::cout << DBG("Received post") << std::endl;
#endif

	message.reply(status_codes::OK);

	web::json::value msg = message.extract_json().get();

	handleJSON(msg);
}

void GameStateIntegration::setupListener(int port)
{
	mListener = http_listener(uri_builder(L"http://localhost:" + std::to_wstring(port)).to_string());
	mListener.support(methods::POST, std::bind(&GameStateIntegration::handlePost, this, std::placeholders::_1));
}

std::optional<std::string> GameStateIntegration::getCSGOPath()
{
	winreg::RegKey key{ HKEY_CURRENT_USER, L"SOFTWARE\\Valve\\Steam" };
	if (std::optional<std::wstring> steamPathO = key.TryGetStringValue(L"SteamPath"))
	{
		std::wstring steamPath = *steamPathO + std::wstring(L"/config/config.vdf"); // using steamapps/libraryfolders.vdf would be possible too

#ifdef _DEBUG
		std::wcout << DBG_START << L"Trying to open steam config at: " << steamPath << DBG_END << std::endl;
#endif

		std::ifstream steamConfig;
		steamConfig.open(steamPath);

		if (steamConfig.good())
		{
			std::regex baseInstallRegex("\"BaseInstallFolder_[0-9]+\"\t+\"(.+)\"");

			std::string line;
			while (getline(steamConfig, line))
			{
				std::smatch matches;

				if (std::regex_search(line, matches, baseInstallRegex))
				{
#ifdef _DEBUG
					std::cout << DBG_START << "Found install folder at: " << matches[1] << DBG_END << std::endl;
#endif
					
					std::string csgoPath = std::string(matches[1]) + "\\\\steamapps\\\\common\\\\Counter-Strike Global Offensive\\\\csgo\\\\cfg";
					DWORD fileAttributes = GetFileAttributesA(csgoPath.c_str());

					if (fileAttributes != INVALID_FILE_ATTRIBUTES && fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						return csgoPath;
					}
				}
			}
		}
		else
		{
#ifdef _DEBUG
			std::cout << DBG("Could not read config.vdf") << std::endl;
#endif

			return {};
		}
	}

	return {};
}

int GameStateIntegration::createConfig()
{
	int port;
	std::string input;
	std::cout << "Enter the port to be used (default: 6969): ";
	std::cin.clear();
	std::cin.sync();
	std::getline(std::cin, input);

	if (!input.empty())
	{
		std::istringstream stream(input);
		stream >> port;

		if (port == 0)
		{
			port = 6969;
		}

		if (port < 1025)
		{
			std::cout << "You entered a well-known port. This potentially requires admin permissions." << std::endl;
		}
	}
	else
	{
		port = 6969;
	}

#ifdef _DEBUG
	std::cout << DBG_START << "Selected port is: " << port << DBG_END << std::endl;
#endif

	std::string csgoPath;
	if (std::optional<std::string> csgoPathO = getCSGOPath())
	{
		csgoPath = *csgoPathO;

#ifdef _DEBUG
		std::cout << DBG_START << "Found CSGO directory at: " << csgoPath << DBG_END << std::endl;
#endif

	}
	else
	{
		std::cout << "Could not determine CSGO's path automatically.\nPlease enter its installation location: ";
		std::cin.clear();
		std::cin.sync();
		std::getline(std::cin, csgoPath);

		while (true)
		{
			csgoPath = std::regex_replace(csgoPath, std::regex("\\\\"), "\\\\");
			csgoPath += std::string("\\\\csgo\\\\cfg");
			DWORD fileAttributes = GetFileAttributesA(csgoPath.c_str());

			if (fileAttributes != INVALID_FILE_ATTRIBUTES && fileAttributes & FILE_ATTRIBUTE_DIRECTORY) // fix
			{
				break;
			}

			std::cerr << "The path you have entered is not valid\n";
			std::cout << "Please enter a valid csgo path: ";
			std::cin.clear();
			std::cin.sync();
			std::getline(std::cin, csgoPath);
		}
	}

	std::string gameStateConfig(R"("CSSense"
{
	"uri"       "http://localhost:)" + std::to_string(port) + R"("
	"timeout"   "12.0"
	"buffer"    "0.0"
	"throttle"  "0.0"
	"heartbeat" "120"
	"output"
	{
		"precision_time"     "3"
		"precision_position" "1"
		"precision_vector"   "3"
	}
	"data"
	{
		"provider"            "1"
		"map"                 "1"
		"round"               "1"
		"player_id"           "1"
		"player_state"        "1"
		"player_weapons"      "1"
		"player_match_stats"  "1"
	}
})");
	
#ifdef _DEBUG
	std::cout << DBG_START << "Writing gamestate file to: " << csgoPath << DBG_END << std::endl;
#endif

	std::ofstream gameStateFile(csgoPath + "\\gamestate_integration_cssense.cfg");
	gameStateFile << gameStateConfig;
	gameStateFile.close();

	std::cout << "\nNow let's set up the config.\nEnter the minimum amount of strength the device should vibrate at at all times (0-20, integer) (default: 0): ";
	std::cin.clear();
	std::cin.sync();
	std::getline(std::cin, input);
	if (!input.empty())
	{
		std::istringstream stream(input);
		stream >> config.baseVibration;

		config.baseVibration = std::clamp(0, 20, config.baseVibration);
	}
	else
	{
		config.baseVibration = 0;
	}

	std::cout << "Should the strength increase after a kill (T/F) (default: T)? ";
	std::cin.clear();
	std::cin.sync();
	std::getline(std::cin, input);
	if (!input.empty() && (input.starts_with('f') || input.starts_with('F')))
	{
		config.increaseOnKill = false;
		config.increaseAmount = 0;
		config.vibrateOnShoot = false;
		config.stopOnKnife = false;
	}
	else
	{
		config.increaseOnKill = true;

		std::cout << "Enter the amount by which the strength should get increased (or decreased) on kill (-20-20, double) (default: 3): ";
		std::cin.clear();
		std::cin.sync();
		std::getline(std::cin, input);
		if (!input.empty())
		{
			std::istringstream stream(input);
			stream >> config.increaseAmount;
			config.increaseAmount = std::clamp(config.increaseAmount, -20., 20.);
		}
		else
		{
			config.increaseAmount = 3;
		}

		std::cout << "Should the increased strength only get applied when shooting (T/F) (default: T)? ";
		std::cin.clear();
		std::cin.sync();
		std::getline(std::cin, input);
		if (!input.empty() && (input.starts_with('f') || input.starts_with('F')))
		{
			config.vibrateOnShoot = false;

			std::cout << "Should the device return to base vibration when holding a knife (T/F) (default: T)? ";
			std::cin.clear();
			std::cin.sync();
			std::getline(std::cin, input);
			if (!input.empty() && (input.starts_with('f') || input.starts_with('F')))
			{
				config.stopOnKnife = false;
			}
			else
			{
				config.stopOnKnife = true;
			}
		}
		else
		{
			config.vibrateOnShoot = true;
			config.stopOnKnife = false;
		}
	}

	std::cout << "Should the device vibrate at maximum strength on MVP (T/F) (default: T)? ";
	std::cin.clear();
	std::cin.sync();
	std::getline(std::cin, input);
	if (!input.empty() && (input.starts_with('f') || input.starts_with('F')))
	{
		config.burstOnMVP = false;
	}
	else
	{
		config.burstOnMVP = true;
	}

	std::ostringstream configStream;
	configStream << std::boolalpha << "{\n\t\"baseVibration\": " << config.baseVibration <<
		",\n\t\"increaseOnKill\": " << config.increaseOnKill <<
		",\n\t\"increaseAmount\": " << config.increaseAmount <<
		",\n\t\"vibrateOnShoot\": " << config.vibrateOnShoot <<
		",\n\t\"stopOnKnife\": " << config.stopOnKnife <<
		",\n\t\"burstOnMVP\": " << config.burstOnMVP <<
		",\n\t\"gamestateConfigPath\": \"" << csgoPath << "\"\n}";

#ifdef _DEBUG
	std::cout << DBG("Writing default config") << std::endl;

	std::cout << DBG_START << configStream.str() << DBG_END << std::endl;
#endif

	std::ofstream configFile("config.json");
	configFile << configStream.str();
	configFile.close();

	std::cout << "\nEverything has been initialized.\n"
		"You can configure the config to your liking by either editing 'config.json' or by running 'CSSense -i' again.\n"
		"You can find an explanation on https://github.com/Acurisu/CSSense-CLI.\n"
		"Restart CSGO if you had it running.\nEnjoy =^w^=\n" << std::endl;

	return port;
}

int GameStateIntegration::parseConfig()
{
	std::ifstream configFile;
	configFile.open("config.json");

	if (configFile.good())
	{
		std::stringstream buffer;
		buffer << configFile.rdbuf();

		try
		{
			web::json::value cfg = web::json::value::parse(buffer);
			config.increaseOnKill = cfg.at(L"increaseOnKill").as_bool();
			config.vibrateOnShoot = cfg.at(L"vibrateOnShoot").as_bool();
			config.stopOnKnife = cfg.at(L"stopOnKnife").as_bool();
			config.burstOnMVP = cfg.at(L"burstOnMVP").as_bool();
			config.increaseAmount = cfg.at(L"increaseAmount").as_number().to_double();
			config.baseVibration = cfg.at(L"baseVibration").as_integer();

			std::wstring gamestateConfigPath = cfg.at(L"gamestateConfigPath").as_string();

#ifdef _DEBUG
			std::wcout << std::boolalpha << DBG_START << L"baseVibration:\t" << config.baseVibration
				<< L"\tincreaseOnKill:\t" << config.increaseOnKill

				<< L"\n[DBG] increaseAmount:\t" << config.increaseAmount
				<< L"\tvibrateOnShoot:\t" << config.vibrateOnShoot

				<< L"\n[DBG] stopOnKnife:\t" << config.stopOnKnife
				<< L"\tburstOnMVP:\t" << config.burstOnMVP

				<< L"\n[DBG] gamestateConfigPath:\t" << gamestateConfigPath << DBG_END << std::endl;
#endif

			std::ifstream gamestateFile;
			gamestateFile.open(gamestateConfigPath + L"\\gamestate_integration_cssense.cfg");

			if (gamestateFile.good())
			{
				std::regex portRegex("http:\\/\\/localhost:([0-9]+)");

				std::string line;
				while (std::getline(gamestateFile, line))
				{
					std::smatch matches;

					if (std::regex_search(line, matches, portRegex))
					{
#ifdef _DEBUG
						std::cout << DBG_START << "Found port: " << matches[1] << DBG_END << std::endl;
#endif

						return std::stoi(matches[1]);
					}
				}
			}
			else
			{
				std::cerr << "Something went wrong trying to retrieve the port.\n"
					"Please run 'CSSense -i' again." << std::endl;
			}
		}
		catch (web::json::json_exception e)
		{
			std::cerr << "Something went wrong trying to parse the config.\n"
				"Please run 'CSSense -i' again or fix the error shown below:\n"
				<< e.what() << std::endl;
		}
	}
	else
	{
		std::cerr << "Something went wrong trying to read the config.\nDid you forget to run 'CSSense -i'?" << std::endl;
	}

	return -1;
}

GameStateIntegration::GameStateIntegration(bool init)
{	
	int port;
	if (init)
	{
		port = createConfig();
	}
	else
	{
		port = parseConfig();
	}

	if (port == -1)
	{
		exit(-1);
	}

#ifndef NO_BLE
	std::wregex serviceGUIDRegex(L"^\\{..300001-002.-4bd4-bbd5-a6920e4c5653\\}");

	std::cout << "Searching for a Lovesense device..." << std::endl;

	if (!bLEController.ConnectBLEDeviceByService(serviceGUIDRegex))
	{
		std::cerr << "Could not connect to the BLE device" << std::endl;
		exit(-1);
	}
	
	if (!bLEController.SelectCharacteristics([](
		winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::
		GattCharacteristic sender,
		winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::
		GattValueChangedEventArgs eventArgs)
	{
#ifdef _DEBUG
		std::cout << DBG_START << "Received: " << eventArgs.CharacteristicValue().data() << DBG_END << std::endl;
#endif
	}))
	{
		std::cerr << "Could not select the specified characteristics" << std::endl;
		exit(-1);
	}

	std::cout << "Found and connected to the device." << std::endl;
#endif

	currentStrength = config.baseVibration;
	sendVibrate(config.baseVibration);
	setupListener(port);
}

GameStateIntegration::~GameStateIntegration()
{
	sendVibrate(0);

#ifndef NO_BLE
	if (!bLEController.DisconnectBLEDevice())
	{
		std::cerr << "Failed trying to disconnect the BLE device" << std::endl;
		exit(-1);
	}
#endif
}