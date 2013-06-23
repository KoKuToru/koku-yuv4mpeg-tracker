#ifndef YUV4MPEG_H
#define YUV4MPEG_H

#include <vector>
#include <iostream>

//Decodes the yuv4mpeg stream
class yuv4mpeg
{
	private:
		std::vector<unsigned char> frame_packed;
		std::vector<unsigned char> frame;

		int frame_w;
		int frame_h;

		std::istream& input;

	public:
		yuv4mpeg(std::istream& input);

		//read the stream and decode frame
		bool update(); //!< returns true when everything is okay

		int width() { return frame_w; }
		int height() { return frame_h; }

		//Get the frame data
		const std::vector<unsigned char>& get();
};

#endif
