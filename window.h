#ifndef WINDOW_H
#define WINDOW_H

#include <framework.h>
#include <complex.h>
#define complex //No idea why I need to do this !
#include <fftw3.h>
#include "yuv4mpeg.h"
#include "motion.h"

class window: public koku::opengl::windowCallback
{
	private:
		//Basic stuff for rendering:
		koku::opengl::window  my_window;
		koku::opengl::buffer  my_rect;     //stores a rectangle from 0,0 to 1,1
		koku::opengl::texture my_rect_tex_data; //texture for rectangle
		koku::opengl::shader  my_shader;   //stores the shader for rectangle rendering

		//Uniforms for rect-rendering (done via shader)
		koku::opengl::shader_uniform my_rect_pos; //vec4(from_x, from_y, to_x, to_y) !
		koku::opengl::shader_uniform my_rect_tex;
		koku::opengl::shader_uniform my_rect_use_tex; //if not -> will use a red border line

		//Uniforms for screen settings
		koku::opengl::shader_uniform my_screen_size; //vec2(w, h)

		//Uniforms for motion
		koku::opengl::shader_uniform my_motion_matrix;

		bool run; //Porgramm still running check
		int screen_w;
		int screen_h;

		//Decoder for YUV4MPEG2
		yuv4mpeg my_yuv4mpeg;
		std::vector<unsigned char> my_yuv4mpeg_data[2]; //stores actual and previous frame

		//Motion tracker
		motion my_motion;

		//Renders the screen
		void render();
		void renderRect(float x1, float y1, float x2, float y2, koku::opengl::texture* tex);

	protected:
		//Callbacks from koku::opengl::windowCallback
		void onQuit();
		void onResize(int w, int h);
		void onMouseMotion(int x, int y, int rx, int ry);
		void onMouseButtonDown(int button, int x, int y);

	public:
		window();
		~window();

		//Updates the YUV-Decoder and updates the Motion-tracker
		bool update(); //!< \returns false when window got closed
};

#endif
