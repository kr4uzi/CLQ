#include "mouse.h"
#include <functional>	// bind

#ifdef WIN32
#include <Windows.h>
typedef decltype(decltype(INPUT::mi)::dwFlags) flags_t;
flags_t input_flags[] = { MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP, MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP };
#endif

mouse::mouse(boost::asio::io_service& service, const button& btn, const boost::posix_time::time_duration& interval)
	: service(service), work(service), timer(service), interval(interval)
{
#ifdef WIN32
	auto input = new INPUT;
	input->type = INPUT_MOUSE;
	ZeroMemory(input, sizeof(INPUT));

	this->flags = (btn == button::LEFT) ? input_flags : input_flags + 2;
	this->input = input;
#endif
}

mouse::~mouse()
{
	clicking = false;

#ifdef WIN32
	delete (INPUT *)input;
#endif
}

void mouse::click_start()
{
	clicking = true;
	service.post(std::bind(&mouse::loop, this));
}

void mouse::click_stop()
{
	clicking = false;
}

void mouse::loop()
{
	if (!clicking) return;
	service.post([this]()
	{
#ifdef WIN32
		auto input = (INPUT *)this->input;
		auto flags = (flags_t *)this->flags;

		input->mi.dwFlags = flags[0];
		SendInput(1, input, sizeof(INPUT));
		input->mi.dwFlags = flags[1];
		SendInput(1, input, sizeof(INPUT));
#endif

		timer.expires_from_now(interval);
		timer.async_wait([this](auto error)
		{
			if (!error)
				service.post(std::bind(&mouse::loop, this));
		});
	});
}