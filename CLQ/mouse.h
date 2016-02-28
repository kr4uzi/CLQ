#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>

class mouse
{
public:
	enum class button
	{
		LEFT,
		RIGHT
	};

private:
	boost::asio::io_service& service;
	boost::asio::io_service::work work;
	boost::asio::deadline_timer timer;
	boost::posix_time::time_duration interval;

	bool clicking = false;

	void * input = nullptr;
	const void * flags;

public:
	mouse(boost::asio::io_service& service, const button& btn, const boost::posix_time::time_duration& interval);
	~mouse();

	bool is_clicking() const { return clicking; }
	void click_start();
	void click_stop();

private:
	void loop();
};