/*!
 * \file
 * \brief
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

#include <cv.h>
#include <highgui.h>

static std::string binary(uchar i)
{
	std::string result;
	for (int bit = 0; bit < 8; ++bit) {
		result = (char)((i & 1) + '0') + result;
		i >>= 1;
	}
	return result;
}

static uchar graycode(uchar i)
{
	for (int bit = 1; bit < 8; ++bit)
		i ^= (i & 1 << bit) >> 1;
	return i;
}

cv::Mat nkb2gray(const cv::Mat & img) {

	if (img.channels() != 1) {
		std::cout << "getBitPlane: img must be one channel!\n";
		return cv::Mat();
	}

	cv::Mat result = img.clone();
	cv::Size size = img.size();

	if (img.isContinuous() && result.isContinuous()) {
		size.width *= size.height;
		size.height = 1;
	}

	for (int y = 0; y < size.height; ++y) {

		const uchar* img_p = img.ptr <uchar> (y);
		uchar* res_p = result.ptr <uchar> (y);

		for (int x = 0; x < size.width; ++x)
			res_p[x] = graycode(img_p[x]);


	}

	return result;
}


int main(int argc, char * argv[]) {
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " IMAGE" << std::endl;
		return 0;
	}

	cv::Mat img = cv::imread(argv[1], -1);
	if (img.empty()) {
		std::cout << "Can't load image from file: " << argv[1] << std::endl;
		return 0;
	}

	if (img.channels() != 1) {
		std::cout << "Image should have 1 channel!" << std::endl;
		return 0;
	}

	for (int i = 0; i < 256; ++i)
		std::cout << std::setw(3) << i << " " << binary(graycode(i)) << std::endl;

	std::string name = argv[1];
	name = name.substr(0, name.find_first_of('.')) + '_';
	imwrite(name + "gray.bmp", nkb2gray(img));

	return 0;
}
