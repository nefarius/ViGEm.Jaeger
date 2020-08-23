//
// Windows
// 
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

//
// ViGEm SDK
// 
#include <ViGEm/Client.h>

//
// STL
// 
#include <iostream>
#include <cstdio>
#include <chrono>
#include <thread>
#include <ctime>

//
// Jaeger
// 
#include <jaegertracing/Tracer.h>

std::chrono::system_clock::time_point a = std::chrono::system_clock::now();
std::chrono::system_clock::time_point b = std::chrono::system_clock::now();

static const double g_fps = 200.0;

int main()
{
	auto config = jaegertracing::Config(
		false,
		jaegertracing::samplers::Config(jaegertracing::kSamplerTypeConst, 1),
		jaegertracing::reporters::Config(
			jaegertracing::reporters::Config::kDefaultQueueSize,
			std::chrono::seconds(1), true));
	auto tracer = jaegertracing::Tracer::make("ViGEmClient", config);

	opentracing::Tracer::InitGlobal(
		std::static_pointer_cast<opentracing::Tracer>(tracer));

    while (true)
    {
        // Maintain designated frequency of 5 Hz (200 ms per frame)
        a = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> work_time = a - b;

        if (work_time.count() < g_fps)
        {
            std::chrono::duration<double, std::milli> delta_ms(g_fps - work_time.count());
            auto delta_ms_duration = std::chrono::duration_cast<std::chrono::milliseconds>(delta_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(delta_ms_duration.count()));
        }

        b = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> sleep_time = b - a;

        // Your code here

    	

        printf("Time: %f \n", (work_time + sleep_time).count());
    }
}
