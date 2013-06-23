#ifndef MOTION_H
#define MOTION_H

#include <complex.h>
#define complex //No idea why
#include <fftw3.h>
#include <utility>
#include <vector>

constexpr int fft_size = 0xFF;

//never copy or use this
struct motionFFT
{
	int x_prev;
	int y_prev;
	int x;
	int y;

	struct sub
	{
		fftw_complex* input;
		fftw_complex* output;
		fftw_plan     plan; //lets check if we can store it
	};

	sub first;
	sub second;
	sub third;

	motionFFT();
	~motionFFT();

	void action();
};

//Detects motion
class motion
{
	private:
		motionFFT track_points[3];
		std::vector<float> mask;
		std::vector<float> motion_matrix;

		bool reseted;

		void update(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b, int w, int h, motionFFT* o);

	public:
		motion();

		//reset tracking position
		void reset(int x, int y);

		//recalcualtes the tracking position
		void update(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b, int w, int h);

		const std::vector<float>& get();

		std::pair<int, int> position(int index);
};

#endif
