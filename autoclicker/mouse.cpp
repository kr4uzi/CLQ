#include "mouse.h"
#include <functional>	// bind

#ifdef WIN32
#include <Windows.h>
typedef decltype(decltype(INPUT::mi)::dwFlags) flags_t;
flags_t input_flags[] = { MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP, MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP };
#elif defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
struct flags_t
{
    decltype(kCGMouseButtonLeft) btn;
    decltype(kCGEventLeftMouseDown) flags[2];
} input_flags[] = {
    {
        kCGMouseButtonLeft,
        { kCGEventLeftMouseDown, kCGEventLeftMouseUp }
    },
    {
        kCGMouseButtonRight,
        { kCGEventRightMouseDown, kCGEventRightMouseUp }
    }
};
#endif

mouse::mouse(boost::asio::io_service& service, const button& btn, const boost::posix_time::time_duration& interval)
	: service(service), work(service), timer(service), interval(interval)
{
#ifdef WIN32
    this->flags = (btn == button::LEFT) ? input_flags : input_flags + 2;
#elif defined(__APPLE__)
    this->flags = (btn == button::LEFT) ? input_flags : input_flags + 1;
#endif
}

mouse::~mouse()
{
	clicking = false;
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
        INPUT input[2];
        ZeroMemory(input, sizeof(input));
        input[0].type = input[1].type = INPUT_MOUSE;
        
        auto flags = (flags_t *)this->flags;
        input[0].mi.dwFlags = flags[0];
        input[1].mi.dwFlags = flags[1];
        
        SendInput(2, input, sizeof(INPUT));
#elif defined(__APPLE__)
        auto flags = (flags_t *)this->flags;
        CGEventRef event = CGEventCreate(nullptr);
        CGPoint loc = CGEventGetLocation(event);
        CFRelease(event);
        
        CGEventRef mevent = CGEventCreateMouseEvent(nullptr, flags->flags[0], loc, flags->btn);
        CGEventPost(kCGHIDEventTap, mevent);
        CGEventSetType(mevent, flags->flags[1]);
        CGEventPost(kCGHIDEventTap, mevent);
        CFRelease(mevent);
#endif

		timer.expires_from_now(interval);
		timer.async_wait([this](auto error)
		{
			if (!error)
				service.post(std::bind(&mouse::loop, this));
		});
	});
}