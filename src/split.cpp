/*!
 * \file
 * \brief
 */

#include <iostream>
#include <vector>

#include <cv.h>
#include <highgui.h>

int main(int argc, char * argv[]) {
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " IMAGE" << std::endl;
		return 0;
	}

	cv::Mat img = cv::imread(argv[1]);
	if (img.empty()) {
		std::cout << "Can't load image from file: " << argv[1] << std::endl;
		return 0;
	}

	// split image into channels
	std::vector<cv::Mat> channels;
	cv::split(img, channels);

	std::string name = argv[1];
	name = name.substr(0, name.find_first_of('.')) + '_';
	for (int i = 0; i < channels.size(); ++i) {
		imwrite(name + (char)(i+'0') + ".bmp", channels[i]);
	}

	return 0;
}
