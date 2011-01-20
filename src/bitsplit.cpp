/*!
 * \file
 * \brief
 */

#include <iostream>
#include <vector>

#include <cv.h>
#include <highgui.h>

cv::Mat getBitPlane(const cv::Mat & img, int plane) {

	if (img.channels() != 1) {
		std::cout << "getBitPlane: img must be one channel!\n";
		return cv::Mat();
	}

	cv::Mat result = img.clone();
	cv::Size size = img.size();

	int mask = 1 << plane;

	if (img.isContinuous() && result.isContinuous()) {
		size.width *= size.height;
		size.height = 1;
	}

	for (int y = 0; y < size.height; ++y) {

		const uchar* img_p = img.ptr <uchar> (y);
		uchar* res_p = result.ptr <uchar> (y);

		for (int x = 0; x < size.width; ++x) {
			res_p[x] = (img_p[x] & mask) ? 255 : 0;
		}

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

	std::string name = argv[1];
	name = name.substr(0, name.find_first_of('.')) + '_';
	for (int p = 0; p < 8; ++p)
		imwrite(name + (char)(p+'0') + ".bmp", getBitPlane(img, p));




	return 0;
}
