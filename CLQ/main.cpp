#include "mouse.h"
#include "keyboard.h"
#include <iostream>

int main()
{	
	auto interval = boost::posix_time::milliseconds(20);
	auto button = mouse::button::RIGHT;

	std::cout << "CLQ Autoclicker - for the best DotA 2 player, the world has ever fucking seen" << std::endl << std::endl;
	std::cout << "Start by pressing [SCROLL]" << std::endl;
	std::cout << "The click interval is [" << interval.total_milliseconds() << "] ms" << std::endl;
	std::cout << std::endl;

	boost::asio::io_service io;
	mouse m(io, button, interval);

	keyboard::initialize(io);
	keyboard::get_instance()->set_scroll_lock(false);
	keyboard::get_instance()->on_key_press.connect([&](keyboard::message msg, keyboard::keycode vkey)
	{
		if (msg == keyboard::message::KEYUP && vkey == VK_SCROLL)
		{
			if (m.is_clicking())
			{
				std::cout << "State: deactivated" << "\r";
				m.click_stop();
			}
			else
			{
				std::cout << "State: activated" << "\r";
				m.click_start();
			}
		}
	});

	io.run();
}