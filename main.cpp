#include "window.h"
#include <iostream>
#include <string>
#include <stdexcept>

using namespace std;


int main(int argc, char** argv)
{
	try
	{
		window my_window;
		while(my_window.update())
		{
			//wait for data
		}
	}
	catch(runtime_error e)
	{
		string error = e.what();
		string check = "YUV4MPEG2";
		error.resize(check.size());
		if (error == check)
		{
			//YUV problem ..
			if (e.what() == "YUV4MPEG2 Not a stream")
			{
				cout << "You are using this programm wrong ! Use it this way:" << endl;

				cout << endl << "Sample:" << endl;
				cout << " mplayer -vo yuv4mpeg:file=>(" << argv[0] << ") video.mp4" << endl;
				return 1;
			}
			else
			{
				cout << e.what() << endl;
				return 1;
			}
		}
		throw;
	}

	return 0;
}
