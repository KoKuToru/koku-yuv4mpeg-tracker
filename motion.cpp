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
	for(int i = 0; i < 3; ++i)
	{
		track_points[i].x = x + sin(i*2.0*M_PI/3.0)*fft_size/3;
		track_points[i].y = y + cos(i*2.0*M_PI/3.0)*fft_size/3;
	}

	//shifted by 180 degree
	for(int i = 3; i < 6; ++i)
	{
		track_points[i].x = x + sin(i*2.0*M_PI/3.0+M_PI)*fft_size;
		track_points[i].y = y + cos(i*2.0*M_PI/3.0+M_PI)*fft_size;
	}

	motion_matrix.resize(3*3);
	for(int i = 0; i < 3*3; ++i)
	{
		motion_matrix[i] = 0;
	}
	motion_matrix[0+0*3] = 1;
	motion_matrix[1+1*3] = 1;
	motion_matrix[2+2*3] = 1;
}

motion::motion()
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

		float n = (sin(data_u/255.0+M_PI*data_y*25.0)+0.5)*(data_v/2.0+data_y/2.0)/2.0;
		o->first.input[x+y*fft_size]  = n*mask[x+y*fft_size];

		data_y = b[(rx+ry*w)*3+0];
		data_y /= 255.0;
		data_u = b[(rx+ry*w)*3+1];
		data_u /= 255.0;
		data_v = b[(rx+ry*w)*3+2];
		data_v /= 255.0;

		n = (sin(data_u/255.0+M_PI*data_y*25.0)+0.5)*(data_v/2.0+data_y/2.0)/2.0;
		o->second.input[x+y*fft_size]  = n*mask[x+y*fft_size];
	}

	o->x_prev = o->x;
	o->y_prev = o->y;

	o->action();
}

void motion::calcMatrix(int w, int h, int a, int b, int c, float M[3*3])
{
	//now generate a transform matrix
	//http://math.stackexchange.com/questions/85373/how-do-i-find-the-matrix-m-that-transforms-a-vector-a-into-the-vector-b

	float X[3*3] = { (float)track_points[a].x_prev/w, (float)track_points[b].x_prev/w, (float)track_points[c].x_prev/w,
					 (float)track_points[a].y_prev/h, (float)track_points[b].y_prev/h, (float)track_points[c].y_prev/h,
					 1, 1, 1 };

	float Y[3*3] = { (float)track_points[a].x/w, (float)track_points[b].x/w, (float)track_points[c].x/w,
					 (float)track_points[a].y/h, (float)track_points[b].y/h, (float)track_points[c].y/h,
					 1, 1, 1 };

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
	}
}

void motion::update(const vector<unsigned char>& a, const vector<unsigned char>& b, int w, int h)
{
	//load data
	for(int i = 0; i < 6; ++i)
	{
		update(a, b, w, h, track_points+i);
	}

	float M1[3*3] = {0};
	calcMatrix(w, h, 0, 1, 2, M1);
	float M2[3*3] = {0};
	calcMatrix(w, h, 3, 4, 5, M2);

	//now check M1 and M2
	float best = 100000.0;
	float best_f = 0.0;
	float M[3*3] = {0};
	for(int fi = 0; fi <= 100; ++fi)
	{
		float f = fi/100.0;
		for(int i = 0; i < 3*3; ++i)
		{
			M[i] = M1[i]*(1.0-f)+M2[i]*f;
		}

		//calculated points
		float error = 0;
		for(int i = 0; i < 6; ++i)
		{
			float old_pos[3] = {(float)track_points[i].x_prev, (float)track_points[i].y_prev, 0};
			float new_pos[3] = {0};

			//multiply
			for(int y = 0; y < 3; ++y)
			for(int k = 0; k < 3; ++k)
			{
				new_pos[y] += M[k + y*3]*old_pos[k];
			}

			new_pos[0] -= track_points[i].x;
			new_pos[1] -= track_points[i].y;

			error += sqrt(new_pos[0]*new_pos[0]+new_pos[1]*new_pos[1]);
		}

		if (error < best)
		{
			best = error;
			best_f = f;
		}
	}

	cout << "error: " << best << " f:" << best_f << endl;
	for(int i = 0; i < 3*3; ++i)
	{
		M[i] = M1[i]*(1.0-best_f)+M2[i]*best_f;
	}

	//make matrix less effective because of "error"
	best /= 50.0;
	best_f = 1.0/(1+best); //the bigger the erro => the less the factor
	float M_ident[3*3] = {1,0,0,0,1,0,0,0,1};
	for(int i = 0; i < 3*3; ++i)
	{
		M[i] = M_ident[i]*(1.0-best_f)+M[i]*best_f;
	}

	//multiply with old matrix
	float new_motion_matrix[3*3] = {0};

	for(int y = 0; y < 3; ++y)
	for(int x = 0; x < 3; ++x)
	for(int k = 0; k < 3; ++k)
	{
		new_motion_matrix[x + y*3] += M[x + k*3]*motion_matrix[k + y*3];
	}

	//move trackpoints based on the transformation
	for(int i = 0; i < 6; ++i)
	{
		float old_pos[3] = {(float)track_points[i].x_prev, (float)track_points[i].y_prev, 0};
		float new_pos[3] = {0};

		//multiply
		/*for(int y = 0; y < 3; ++y)
		for(int k = 0; k < 3; ++k)
		{
			new_pos[y] += new_motion_matrix[k + y*3]*old_pos[k];
		}*/

		track_points[i].x = old_pos[0];
		track_points[i].y = old_pos[1];
		/*
		//not working ?
		track_points[i].x = new_pos[0];
		track_points[i].y = new_pos[1];
		*/
	}

	//save new matrix
	for(int y = 0; y < 3; ++y)
	for(int x = 0; x < 3; ++x)
	{
		motion_matrix[x + y*3] = new_motion_matrix[x + y*3];
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
