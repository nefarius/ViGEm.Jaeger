//
// Windows
// 
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Xinput.h>

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

// Maintain designated frequency of 144 Hz (6.9 ms per frame)
static const double g_MsPerFrame = 6.9;

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

	const auto client = vigem_alloc();

	if (client == nullptr)
	{
		std::cerr << "Uh, not enough memory to do that?!" << std::endl;
		return -1;
	}

	const auto retval = vigem_connect(client);

	if (!VIGEM_SUCCESS(retval))
	{
		std::cerr << "ViGEm Bus connection failed with error code: 0x" << std::hex << retval << std::endl;
		return -1;
	}

	const auto pad = vigem_target_x360_alloc();

	if (pad == nullptr)
	{
		std::cerr << "Uh, not enough memory to do that?!" << std::endl;
		return -1;
	}

	const auto pir = vigem_target_add(client, pad);

	if (!VIGEM_SUCCESS(pir))
	{
		std::cerr << "Target plugin failed with error code: 0x" << std::hex << retval << std::endl;
		return -1;
	}

	XINPUT_STATE state;
	XUSB_REPORT report;

	XUSB_REPORT_INIT(&report);

	std::cout << "Running benchmark, observe http://localhost:16686/search for traces" << std::endl;
	std::cout << std::endl;
	std::cout << "Press Ctrl+C to quit" << std::endl;

	//
	// https://stackoverflow.com/a/38730516
	// 
	while (true)
	{
		a = std::chrono::system_clock::now();
		std::chrono::duration<double, std::milli> work_time = a - b;

		if (work_time.count() < g_MsPerFrame)
		{
			std::chrono::duration<double, std::milli> delta_ms(g_MsPerFrame - work_time.count());
			auto delta_ms_duration = std::chrono::duration_cast<std::chrono::milliseconds>(delta_ms);
			std::this_thread::sleep_for(std::chrono::milliseconds(delta_ms_duration.count()));
		}

		b = std::chrono::system_clock::now();
		std::chrono::duration<double, std::milli> sleep_time = b - a;

		auto spanFrame = opentracing::Tracer::Global()->StartSpan("Frame");

		// Main "game loop"
		{
			spanFrame->SetTag("bLeftTrigger", report.bLeftTrigger);

			auto spanUpdate = opentracing::Tracer::Global()->StartSpan("vigem_target_x360_update",
			                                                           {ChildOf(&spanFrame->context())});

			if (!VIGEM_SUCCESS(vigem_target_x360_update(client, pad, report)))
			{
				std::cerr << "Updating target failed with error code: 0x" << std::hex << retval << std::endl;
				return -1;
			}

			spanUpdate->Finish();

			auto spanGetState = opentracing::Tracer::Global()->StartSpan("XInputGetState",
			                                                             {ChildOf(&spanFrame->context())});

			const auto ret = XInputGetState(0, &state);

			if (ret != ERROR_SUCCESS)
			{
				std::cerr << "XInputGetState failed with error code: 0x" << std::hex << GetLastError() << std::endl;
				return -1;
			}

			if (report.bLeftTrigger != state.Gamepad.bLeftTrigger)
			{
				std::cerr << "WARN: Submitted and captured report out of sync!" << std::endl;
			}

			report.bLeftTrigger++;

			spanGetState->Finish();
		}

		spanFrame->SetTag("wait-time", (work_time + sleep_time).count());

		spanFrame->Finish();
	}
}
