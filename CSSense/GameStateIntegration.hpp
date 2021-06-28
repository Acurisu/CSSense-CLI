#pragma once

#include "Macros.h"

#ifdef _DEBUG
#define NO_BLE
#define EVENTS
#endif

using namespace web;
using namespace http;
using namespace http::experimental::listener;

struct Config
{
	int baseVibration;
	bool increaseOnKill;
	double increaseAmount;
	bool vibrateOnShoot;
	bool stopOnKnife;
	bool burstOnMVP;
};

class GameStateIntegration
{
private:
	http_listener mListener;
	Config config;
	BluetoothLEController bLEController;
	int currentStrength;
	bool isMVP = false;
	bool isDead = false;
	std::chrono::steady_clock::time_point lastShot;

	void sendVibrate(int strength);
	void handleJSON(web::json::value msg);
	void handlePost(http_request message);
	void setupListener(int port);
	std::optional<std::string> getCSGOPath();
	int createConfig();
	int parseConfig();

public:
	GameStateIntegration(bool init);
	~GameStateIntegration();

	pplx::task<void> open() { return mListener.open(); }
	pplx::task<void> close() { return mListener.close(); }
};

