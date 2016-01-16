#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <vector>
#include <iostream>
#include <string>

#define CONT vector<Point>

using namespace cv;
using namespace std;

Mat src, erosion_dst;
int erosion_elem = 0;
int erosion_size = 5;

void Erosion( int, void* );

Mat findBlackSquare();

bool compareContourAreas (CONT contour1, CONT contour2) {
    double i = fabs(contourArea(Mat(contour1)));
    double j = fabs(contourArea(Mat(contour2)));
    return (i > j);
}

int main( int, char** argv )
{
  src = imread( argv[1] );
  if( src.empty() )
    { return -1; }
  Erosion( 0, 0 );
  Mat crop = findBlackSquare();
  cout << string(argv[1])+"proc.jpg";
  imwrite(string(argv[1])+"proc.jpg", crop);
  waitKey(0);
  return 0;
}

void Erosion( int, void* )
{
  int erosion_type = 0;
  erosion_type = MORPH_RECT;

  Mat element = getStructuringElement( erosion_type,
                       Size( 2*erosion_size + 1, 2*erosion_size+1 ),
                       Point( erosion_size, erosion_size ) );

  erode( src, erosion_dst, element );

  imshow( "Erosion Demo", erosion_dst );
}

Mat findBlackSquare()
{
	vector<Vec4i> hierarchy;
	vector<CONT > contours;
	Mat gray;
	cvtColor(erosion_dst, gray, CV_BGR2GRAY);
	inRange(gray, (0), (5), gray);
	imshow("gray1", gray);
	findContours(gray, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	sort(contours.begin(), contours.end(), compareContourAreas);

	CONT contour = contours[0];
	double eps = 0.016 * arcLength(contour, true);
	approxPolyDP(contour, contour, eps, true);
	Rect rect = boundingRect(contour);
	double k = (rect.height+0.0)/rect.width;
	contours[0] = contour;

	if (0.8<k && k<1.3 && rect.area()>100)
	{
	   // drawContours(src, contours, 0, Scalar(0,255,0), 3);

	}

	Mat mask = Mat::zeros(gray.rows, gray.cols, CV_8UC1);
	drawContours(mask, contours, 0, Scalar(255), CV_FILLED);
	Mat crop(src.rows, src.cols, CV_8UC3);
	crop.setTo(Scalar(255,255,255));
	src.copyTo(crop, mask);
	normalize(mask.clone(), mask, 0.0, 255.0, CV_MINMAX, CV_8UC1);

	imshow("result", src);
	imshow("cropped", crop);
	waitKey();
	return crop;
}
