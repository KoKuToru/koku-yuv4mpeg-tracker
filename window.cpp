#include "shader.h"
#include "window.h"
#include <iostream>
using namespace std;

void window::onQuit()
{
	//stop the program
	run = false;
}

void window::onResize(int w, int h)
{
	screen_w = w;
	screen_h = h;

	glViewport(0, 0, w, h);
	render();
}

void window::onMouseMotion(int x, int y, int rx, int ry)
{
	//nothing yet
}

void window::onMouseButtonDown(int button, int x, int y)
{
	my_motion.reset(x*3, y*3);
	render();
}


window::window():
	my_window(this, "koku-yuv4mpeg-tracker", 640, 480, true, true),
	my_rect(&my_window),
	my_rect_tex_data(&my_window),
	my_shader(&my_window),
	my_rect_pos("rect_pos"),
	my_rect_tex("rect_tex"),
	my_rect_use_tex("rect_use_tex"),
	my_screen_size("screen_size"),
	my_motion_matrix("motion_matrix"),
	run(true),
	screen_w(640),
	screen_h(480),
	my_yuv4mpeg(cin) //from cin for now
{
	//Setup vertex buffer for rect-rendering
	const float vertex[]=
	{
		 0.0,  0.0,
		 1.0,  0.0,
		 0.0,  1.0,
		 0.0,  1.0,
		 1.0,  0.0,
		 1.0,  1.0
	};
	my_rect.upload(false, vertex, 6*2, 2);
	const unsigned short index[]=
	{
		0,1,2,3,4,5
	};
	my_rect.upload(true, index, 6, 1);

	//Setup shaders for rect rendering
	my_shader.uploadVertex(vertex_shader);
	my_shader.uploadFragment(fragment_shader);

	my_shader.compile();

	//Setup texture by uploading a dummy texture..
	int image_w = 0xFF;
	int image_h = 0xFF;
	unsigned char* image = new unsigned char[image_w*image_h*3];

	for(int y = 0;  y < image_h; ++y)
	for(int x = 0;  x < image_w; ++x)
	{
		image[(x+y*image_w)*3+0] = (0xFF*x)/image_w;
		image[(x+y*image_w)*3+1] = (0xFF*y)/image_h;
		image[(x+y*image_w)*3+2] = 0;
	}

	my_rect_tex_data.upload(image, image_w, image_h, 3);
	delete[] image;

	//update size data:
	onResize(screen_w, screen_h);

	//reset tracking position
	my_motion.reset(my_yuv4mpeg.width()/2, my_yuv4mpeg.height()/2);
}

window::~window()
{

}

bool window::update()
{
	//Update the events..
	my_window.update();
	run |= my_yuv4mpeg.update();

	//get frame data
	swap(my_yuv4mpeg_data[0], my_yuv4mpeg_data[1]); //since C++11 this shouldn't "copy"
	my_yuv4mpeg_data[1] = my_yuv4mpeg.get();

	//upload data
	my_rect_tex_data.upload(my_yuv4mpeg_data[1].data(), my_yuv4mpeg.width(), my_yuv4mpeg.height(), 3);

	//update tracking
	if (my_yuv4mpeg_data[0].size() == my_yuv4mpeg_data[1].size())
	{
		my_motion.update(my_yuv4mpeg_data[0], my_yuv4mpeg_data[1], my_yuv4mpeg.width(), my_yuv4mpeg.height());
	}

	//Render the scene
	render();

	return run;
}

void window::renderRect(float x1, float y1, float x2, float y2, koku::opengl::texture* tex)
{
	float data[4];
	my_shader.begin();
		//update most important data:
		data[0] = x1;
		data[1] = y1;
		data[2] = x2;
		data[3] = y2;
		my_shader.set(&my_rect_pos, 4, 1, data);
		data[0] = screen_w;
		data[1] = screen_h;
		my_shader.set(&my_screen_size, 2, 1, data);
		if (tex != nullptr)
		{
			my_shader.set(&my_rect_use_tex, 1);
			my_shader.set(&my_rect_tex, tex);
		}
		else
		{
			my_shader.set(&my_rect_use_tex, 0);
			my_shader.set(&my_rect_tex, 0);
		}

		//render the rect
		my_rect.render(3, 6);
	my_shader.end();
}

void window::render()
{
	my_window.begin();
		//clear window
		glClearColor(0,0,0,1);
		glClear(GL_COLOR_BUFFER_BIT);

		//render video transformed
		vector<float> motion_data = my_motion.get();
		my_shader.set(&my_motion_matrix, 1, 3*3, motion_data.data()); //no const version ? need to be fixed in koku_opengl_framework !
		renderRect(my_yuv4mpeg.width()/3 + (screen_w-my_yuv4mpeg.width()/2)/2-my_yuv4mpeg.width()/2,
				   screen_h/2-my_yuv4mpeg.height()/2,
				   my_yuv4mpeg.width()/3 + (screen_w-my_yuv4mpeg.width()/2)/2+my_yuv4mpeg.width()/2,
				   screen_h/2+my_yuv4mpeg.height()/2, &my_rect_tex_data);

		//render video normal
		float indent_matrix[] = {1,0,0,0,1,0,0,0,1};
		my_shader.set(&my_motion_matrix, 1, 3*3, indent_matrix);
		renderRect(0, 0, 0+my_yuv4mpeg.width()/3, 0+my_yuv4mpeg.height()/3, &my_rect_tex_data);

		//render motion trackers
		for(int i = 0; i < 6; ++i)
		{
			pair<int, int> pos = my_motion.position(i);
			renderRect((pos.first  - fft_size/2)/3,
					   (pos.second - fft_size/2)/3,
					   (pos.first  + fft_size/2)/3,
					   (pos.second + fft_size/2)/3, nullptr);
		}
		//swap buffers
		my_window.flip();
	my_window.end();
}
