#include <cstdio>
#include <chrono>
#include <Processing.NDI.Lib.h>


#include <linux/fb.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <cstring>
#include <cassert>

// #define PRINT_FOURCC(name) printf("FourCC %s: %d / %d\n", #name, NDIlib_FourCC_video_type_e::NDIlib_FourCC_video_type_##name, NDIlib_FourCC_video_type_e::NDIlib_FourCC_type_##name );

// void print_all_fourccs()
// {
// 	PRINT_FOURCC(UYVY);
// 	PRINT_FOURCC(UYVA);
// 	PRINT_FOURCC(P216);
// 	PRINT_FOURCC(PA16);
// 	PRINT_FOURCC(YV12);
// 	PRINT_FOURCC(I420);
// 	PRINT_FOURCC(NV12);
// 	PRINT_FOURCC(BGRA);
// 	PRINT_FOURCC(BGRX);
// 	PRINT_FOURCC(RGBA);
// 	PRINT_FOURCC(RGBX);
// }


bool get_framebuffer(int& out_framebuffer_fd, char*& out_framebuffer, int& out_fb_size, fb_var_screeninfo& out_screeninfo)
{
	out_framebuffer_fd = open("/dev/fb0", O_RDWR);
	if ( out_framebuffer_fd <= 0)
	{
		printf("Could not open framebuffer!?\n");
		return false;
	}


	ioctl (out_framebuffer_fd, FBIOGET_VSCREENINFO, &out_screeninfo);

	out_fb_size = out_screeninfo.xres * out_screeninfo.yres * (out_screeninfo.bits_per_pixel / 8);

	printf("Got framebuffer info: w=%d, h=%d, bits_per_pixel=%d, bytes_per_pixel=%d, size=%d\n", 
		out_screeninfo.xres, out_screeninfo.yres, out_screeninfo.bits_per_pixel, out_screeninfo.bits_per_pixel / 8, out_fb_size);

	out_framebuffer = (char*)mmap (0, out_fb_size, 
			PROT_READ | PROT_WRITE, MAP_SHARED, out_framebuffer_fd, (off_t)0);

	printf("Framebuffer mapped...\n");

	memset (out_framebuffer, 0, out_fb_size);

	printf("Framebuffer initialized!\n");

	return true;
}


// bool get_console_info(int& out_tty0_fd, int& out_rows, int& out_cols)
// {
// 	out_tty0_fd = fdopen("/dev/pts/1", O_RDWR + O_NOCTTY);
// 	if ( out_tty0_fd <= 0)
// 	{
// 		printf("Could not open tty1!?\n");
// 		return false;
// 	}



// 	winsize size;
// 	ioctl(out_tty0_fd, TIOCGWINSZ, &size);
// 	out_rows = size.ws_row;
// 	out_cols = size.ws_col;

// 	printf("Got tty1 info: rows=%d, cols=%d\n", out_rows, out_cols);

// 	return true;
// }


bool get_ndi_recv(NDIlib_recv_instance_t& out_recv, const char* source_name = nullptr, const int timeout_ms = 1000)
{
	NDIlib_find_instance_t pNDI_find = NDIlib_find_create_v2();
	if (!pNDI_find)
	{
		printf("Failed to create NDI Find instance!\n");
		return false;
	}

	uint32_t num_sources = 0;
	const NDIlib_source_t* sources = nullptr;

	uint32_t target_source_idx = -1;

	while (target_source_idx == -1)
	{
		if (!NDIlib_find_wait_for_sources(pNDI_find, timeout_ms))
		{
			printf("...NDI find returned false (no new sources?)...\n");
			continue;
		}

		printf("Got new sources...\n");

		sources = NDIlib_find_get_current_sources(pNDI_find, &num_sources);
		for (int source_idx = 0; source_idx < num_sources; source_idx++)
		{
			const NDIlib_source_t& source = sources[source_idx];
			//if ( source_name != nullptr && strcmp(source_name, source.p_url_address) == 0 )
			if ( source_name != nullptr && strstr(source.p_ndi_name, source_name) != nullptr )
			{
				target_source_idx = source_idx;
			}

			printf("Got source %d/%d: name=\"%s\", url=\"%s\" (is target: %s)\n", source_idx, num_sources, source.p_ndi_name, source.p_url_address, (target_source_idx == source_idx || source_name == nullptr) ? "true" : "false");
		}

		if (num_sources >= 1 && source_name == nullptr)
		{
			target_source_idx = 1;
			break;
		}
	}

	if (target_source_idx == -1)
	{
		printf("Failed to get NDI source! (was looking for \"%s\")\n", source_name);
		return false;
	}


	const NDIlib_source_t& target_source = sources[target_source_idx];
	printf("Got NDI target source: was index %d (\"%s\")\n", target_source_idx, target_source.p_ndi_name);

	NDIlib_recv_create_v3_t recv_desc; // NOTE THIS IS WHAT WAS MISSING
	recv_desc.color_format = NDIlib_recv_color_format_RGBX_RGBA;
	out_recv = NDIlib_recv_create_v3(&recv_desc);
	if (!out_recv)
	{
		printf("Failed to create recv for NDI source \"%s\"\n", target_source.p_url_address);
		return false;
	}

	NDIlib_recv_connect(out_recv, &target_source);
	NDIlib_find_destroy(pNDI_find);

	return true;
}


int main(int argc, char* argv[])
{
	// Get Framebuffer ////////////////////////////////////////////////////////////////////////////
	int framebuffer_fd;
	char* framebuffer = nullptr;
	int framebuffer_size = -1;
	fb_var_screeninfo framebuffer_info;
	if (!get_framebuffer(framebuffer_fd, framebuffer, framebuffer_size, framebuffer_info))
	{
		return 1;
	}

	// Get Hostname ///////////////////////////////////////////////////////////////////////////////
	char hostname[HOST_NAME_MAX];
	if (gethostname(hostname, HOST_NAME_MAX) != 0)
	{
		printf("Failed to get hostname!\n");
		return 1;
	}

	printf("Got hostname: \"%s\"\n", hostname);

	// Get tty info ///////////////////////////////////////////////////////////////////////////////
	// int tty0_fd, tty0_rows, tty0_cols;
	// get_console_info(tty0_fd, tty0_rows, tty0_cols);

	// dprintf(tty0_fd, "hello, world!\n");
	// return 0;


	// Setup NDI //////////////////////////////////////////////////////////////////////////////////
	// Not required, but "correct" (see the SDK documentation).
	if (!NDIlib_initialize())
	{
		printf("Failed to initialize NDI!\n");
		return 1;
	}


	// Find NDI //////////////////////////////////////////////////////////////////////////////////
	NDIlib_recv_instance_t ndi_recv;
	if (!get_ndi_recv(ndi_recv, hostname))
	{
		printf("Failed to get NDI receive!\n");
		return 1;
	}


	// Some misc setup... /////////////////////////////////////////////////////////////////////////
	using namespace std::chrono;

	bool has_printed = false;
	NDIlib_video_frame_v2_t video_frame;

	const int ndi_receive_timeout = 3000;

	const auto print_freq = seconds(3);
	auto start_time = high_resolution_clock::now();
	auto last_print_time = start_time;
	int cycles_since_last_print = 0;
	int cycles_total = 0;

	while (true) {
		const auto ndi_recv_result = NDIlib_recv_capture_v2(ndi_recv, &video_frame, nullptr, nullptr, ndi_receive_timeout);
		if (ndi_recv_result == NDIlib_frame_type_video) {
			
			
			auto time_now = high_resolution_clock::now();
			std::chrono::duration<float> time_since_last_print = time_now - last_print_time;
			if (time_since_last_print >= print_freq) {
				auto avg_cycles_per_second = cycles_since_last_print / time_since_last_print.count();
				
				printf("NDI_CPP: %4.f seconds | %6d cycles | %4.2f cycles/sec\n", std::chrono::duration<float>(time_now - start_time).count(), cycles_total, avg_cycles_per_second );

				cycles_since_last_print = 0;
				last_print_time = time_now;
			}
			
			
			if (!has_printed) {
				printf("Got first frame: %dx%d, stride = %d\n", video_frame.xres, video_frame.yres, video_frame.line_stride_in_bytes );
				has_printed = true;
			}

			assert(video_frame.xres == framebuffer_info.xres);
			assert(video_frame.yres == framebuffer_info.yres);
			assert(video_frame.line_stride_in_bytes == video_frame.xres * 4);




			memcpy(framebuffer, video_frame.p_data, framebuffer_size);
			NDIlib_recv_free_video_v2(ndi_recv, &video_frame);
			cycles_total++;
			cycles_since_last_print++;
		} else if (ndi_recv_result == NDIlib_frame_type_none) {
			printf("WARNING: Timed out waiting %.2f seconds for NDI frame!\n", ndi_receive_timeout / 1000.0);
		}
	}

	// Clean up frambuffer ////////////////////////////////////////////////////////////////////////
	memset (framebuffer, 0, framebuffer_size);
	munmap (framebuffer, framebuffer_size);
	close (framebuffer_fd);

	// NDI Cleanup ////////////////////////////////////////////////////////////////////////////////
	NDIlib_recv_destroy(ndi_recv);
	NDIlib_destroy();

	return 0;
}
