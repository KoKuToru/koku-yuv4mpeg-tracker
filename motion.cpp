#include "motion.h"
#include <iostream>
using namespace std;

motionFFT::motionFFT():
	x_prev(0), y_prev(0),
	x(0), y(0)
{
	first.input  = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * fft_size * fft_size);
	first.output = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * fft_size * fft_size);
	first.plan   = fftw_plan_dft_2d(fft_size, fft_size, first.input, first.output, FFTW_FORWARD, FFTW_ESTIMATE);

	second.input  = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * fft_size * fft_size);
	second.output = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * fft_size * fft_size);
	second.plan   = fftw_plan_dft_2d(fft_size, fft_size, second.input, second.output, FFTW_FORWARD, FFTW_ESTIMATE);

	third.input  = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * fft_size * fft_size);
	third.output = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * fft_size * fft_size);
	third.plan   = fftw_plan_dft_2d(fft_size, fft_size, third.input, third.output, FFTW_BACKWARD, FFTW_ESTIMATE);
}

motionFFT::~motionFFT()
{
	fftw_free(first.input);
	fftw_free(first.output);
	fftw_destroy_plan(first.plan);

	fftw_free(second.input);
	fftw_free(second.output);
	fftw_destroy_plan(second.plan);

	fftw_free(third.input);
	fftw_free(third.output);
	fftw_destroy_plan(third.plan);
}

void motionFFT::action()
{
	fftw_execute(first.plan);
	fftw_execute(second.plan);

	//Correlate old with new
	for(int i = 0; i < fft_size*fft_size; ++i)
	{
		third.input[i] = (first.output[i]*conj(second.output[i]))/cabs(first.output[i]*conj(second.output[i]));
	}

	fftw_execute(third.plan);

	//search max
	int best_id = 0;
	for(int i = 1; i < fft_size*fft_size; ++i)
	{
		//should check for magnetude ?
		if (creal(third.output[i]) > creal(third.output[best_id]))
		{
			best_id = i;
		}
	}

	#define FIX(a) ((a > fft_size/2)?(a-fft_size):a)

	//update position
	x += -FIX(best_id%fft_size);
	y += -FIX(best_id/fft_size);

	#undef FIX
}

void motion::reset(int x, int y)
{
	track_points[0].x = x;
	track_points[0].y = y - fft_size/2;
	track_points[1].x = x - fft_size/2;
	track_points[1].y = y;
	track_points[2].x = x + fft_size/2;
	track_points[2].y = y;

	motion_matrix.resize(3*3);
	for(int i = 0; i < 3*3; ++i)
	{
		motion_matrix[i] = 0;
	}
	motion_matrix[0+0*3] = 1;
	motion_matrix[1+1*3] = 1;
	motion_matrix[2+2*3] = 1;

	reseted = true;
}

motion::motion(): reseted(true)
{
	motion_matrix.resize(3*3);
	for(int i = 0; i < 3*3; ++i)
	{
		motion_matrix[i] = 0;
	}
	motion_matrix[0+0*3] = 1;
	motion_matrix[1+1*3] = 1;
	motion_matrix[2+2*3] = 1;

	//mask for fft data:
	mask.resize(fft_size*fft_size);
	for(int y = 0; y < fft_size; ++y)
	for(int x = 0; x < fft_size; ++x)
	{
		float dx = x-fft_size/2.0;
		float dy = y-fft_size/2.0;
		mask[x+y*fft_size] = max(0.0, 1.0-sqrt(dx*dx+dy*dy)/fft_size);
	}
}

void motion::update(const vector<unsigned char>& a, const vector<unsigned char>& b, int w, int h, motionFFT* o)
{
	float data_y,data_u,data_v;

	for(int y = 0; y < fft_size; ++y)
	for(int x = 0; x < fft_size; ++x)
	{
		int rx = o->x-fft_size/2+x;
		int ry = o->y-fft_size/2+y;

		if (rx < 0) rx = 0;
		if (ry < 0) ry = 0;
		if (rx >= w) rx = w-1;
		if (ry >= h) ry = h-1;

		data_y = a[(rx+ry*w)*3+0];
		data_y /= 255.0;
		data_u = a[(rx+ry*w)*3+1];
		data_u /= 255.0;
		data_v = a[(rx+ry*w)*3+2];
		data_v /= 255.0;

		o->first.input[x+y*fft_size]  = data_y*mask[x+y*fft_size]; //Only Y-Channel for now

		data_y = b[(rx+ry*w)*3+0];
		data_y /= 255.0;
		data_u = b[(rx+ry*w)*3+1];
		data_u /= 255.0;
		data_v = b[(rx+ry*w)*3+2];
		data_v /= 255.0;

		o->second.input[x+y*fft_size]  = data_y*mask[x+y*fft_size]; //Only Y-Channel for now
	}

	o->x_prev = o->x;
	o->y_prev = o->y;

	o->action();

	reseted = false;
}

void motion::update(const vector<unsigned char>& a, const vector<unsigned char>& b, int w, int h)
{
	//load data
	for(int i = 0; i < 3; ++i)
	{
		update(a, b, w, h, track_points+i);
	}

	//now generate a transform matrix
	//http://math.stackexchange.com/questions/85373/how-do-i-find-the-matrix-m-that-transforms-a-vector-a-into-the-vector-b

	float X[3*3] = { (float)track_points[0].x_prev/w, (float)track_points[1].x_prev/w, (float)track_points[2].x_prev/w,
					 (float)track_points[0].y_prev/h, (float)track_points[1].y_prev/h, (float)track_points[2].y_prev/h,
					 1, 1, 1 };

	float Y[3*3] = { (float)track_points[0].x/w, (float)track_points[1].x/w, (float)track_points[2].x/w,
					 (float)track_points[0].y/h, (float)track_points[1].y/h, (float)track_points[2].y/h,
					 1, 1, 1 };

	float M[3*3]  {0};

	//Y = M * X
	//M = X * inv(Y)

	//http://stackoverflow.com/questions/983999/simple-3x3-matrix-inverse-code-c , makes life easier
	float Y_inv[3*3] = {0};
	#define index(a, b) (a+b*3)
	#define A(a, b) (Y[index(a, b)])
	#define result(a,b) (Y_inv[index(b,a)])
	double determinant = +A(0,0)*(A(1,1)*A(2,2)-A(2,1)*A(1,2))
						 -A(0,1)*(A(1,0)*A(2,2)-A(1,2)*A(2,0))
						 +A(0,2)*(A(1,0)*A(2,1)-A(1,1)*A(2,0));
	if (determinant != 0)
	{
		double invdet = 1/determinant;
		result(0,0) =  (A(1,1)*A(2,2)-A(2,1)*A(1,2))*invdet;
		result(1,0) = -(A(0,1)*A(2,2)-A(0,2)*A(2,1))*invdet;
		result(2,0) =  (A(0,1)*A(1,2)-A(0,2)*A(1,1))*invdet;
		result(0,1) = -(A(1,0)*A(2,2)-A(1,2)*A(2,0))*invdet;
		result(1,1) =  (A(0,0)*A(2,2)-A(0,2)*A(2,0))*invdet;
		result(2,1) = -(A(0,0)*A(1,2)-A(1,0)*A(0,2))*invdet;
		result(0,2) =  (A(1,0)*A(2,1)-A(2,0)*A(1,1))*invdet;
		result(1,2) = -(A(0,0)*A(2,1)-A(2,0)*A(0,1))*invdet;
		result(2,2) =  (A(0,0)*A(1,1)-A(1,0)*A(0,1))*invdet;

		//http://en.wikipedia.org/wiki/Matrix_multiplication
		for(int y = 0; y < 3; ++y)
		for(int x = 0; x < 3; ++x)
		for(int k = 0; k < 3; ++k)
		{
			//M[x + y*3] += X[x + k*3]*Y_inv[k + y*3];
			//based on the result of http://www.bluebit.gr/matrix-calculator/matrix_multiplication.aspx
			M[x + y*3] += Y_inv[x + k*3]*X[k + y*3];
		}

		//mutply with old
		float new_motion_matrix[3*3] = {0};

		for(int y = 0; y < 3; ++y)
		for(int x = 0; x < 3; ++x)
		for(int k = 0; k < 3; ++k)
		{
			//new_motion_matrix[x + y*3] = motion_matrix[x + k*3]*M[k + y*3];
			new_motion_matrix[x + y*3] += M[x + k*3]*motion_matrix[k + y*3];
		}

		//center trackpoint
		float center_x = 0;
		float center_y = 0;
		for(int i = 0; i < 3; ++i)
		{
			center_x += track_points[i].x;
			center_y += track_points[i].y;
		}
		//reset(center_x/3.0, center_y/3.0); //resets motion_matrix, but we reset it manually with new matrix !

		//save new matrix
		for(int y = 0; y < 3; ++y)
		for(int x = 0; x < 3; ++x)
		{
			motion_matrix[x + y*3] = new_motion_matrix[x + y*3];
		}
	}

}

const vector<float>& motion::get()
{
	return motion_matrix;
}

pair<int, int> motion::position(int index)
{
	return make_pair(track_points[index].x, track_points[index].y);
}
