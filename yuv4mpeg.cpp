#include "yuv4mpeg.h"
#include <stdexcept>

using namespace std;

yuv4mpeg::yuv4mpeg(istream &input):
	input(input)
{
	//Check for YUV4MPEG
	string data;

	cin >> data;

	if (data != "YUV4MPEG2")
	{
		throw std::runtime_error("YUV4MPEG2 Not a stream");
	}

	char c;
	cin >> c;
	if (c != 'W')
	{
		throw std::runtime_error("YUV4MPEG2 File is wrong ! - Waiting for W");
	}
	cin >> frame_w;

	cin >> c;
	if (c != 'H')
	{
		throw std::runtime_error("YUV4MPEG2 File is wrong ! - Waiting for H");
		return;
	}
	cin >> frame_h;

	frame_packed.resize(frame_w*frame_h*3/2);
	frame.resize(frame_w*frame_h*3);
}

bool yuv4mpeg::update()
{
	if (!cin.good())
	{
		return false;
	}
	/*
	  This way of reading a YUV4MPEG might be very easy..
	  But is hardly correct, pretty much only works with mplayer..
	  In the future I might update this to decode a real YUV4MPEG stream
	 */
	//wait for FRAME
	string dummy;
	do
	{
		cin >> dummy;
		if (!cin.good())
		{
			return false;
		}
	} while(dummy != "FRAME");

	//read the fream
	char ignore;
	cin.read(&ignore, 1); //first byte is nothing..
	if (!cin.good())
	{
		return false;
	}
	cin.read(reinterpret_cast<char*>(frame_packed.data()), frame_packed.size());

	//unpack the frame
	const unsigned char* data_y = frame_packed.data();
	const unsigned char* data_u = frame_packed.data()+frame_w*frame_h;
	const unsigned char* data_v = frame_packed.data()+frame_w*frame_h+frame_w/2*frame_h/2;
	for(int y = 0; y < frame_h; ++y)
	for(int x = 0; x < frame_w; ++x)
	{
		frame[(x+y*frame_w)*3+0] = data_y[x+y*frame_w]; //Y
		frame[(x+y*frame_w)*3+1] = data_u[x/2+y/2*frame_w/2]; //U
		frame[(x+y*frame_w)*3+2] = data_v[x/2+y/2*frame_w/2]; //V
	}
}

const vector<unsigned char>& yuv4mpeg::get()
{
	return frame;
}
