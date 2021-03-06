/*!
 * \file
 * \brief
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdio>
#include <fstream>
#include <string>

#include <cv.h>
#include <highgui.h>

#include <boost/program_options.hpp>

// ===============================================================================================
//
// Bit-wise splitting and merging
//
// ===============================================================================================

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

cv::Mat mergeBitPlanes(const std::vector<cv::Mat> & planes) {
	if (planes.size() != 8) {
		std::cout << "mergeBitPlanes: must be planes.size() == 8!\n";
		return cv::Mat();
	}

	cv::Mat result = planes[0].clone();
	cv::Size size = result.size();

	bool cont = true;

	if (!result.isContinuous())
		cont = false;

	for (int i = 0; i < planes.size(); ++i)
		if (!planes[i].isContinuous())
			cont = false;


	if (cont) {
		size.width *= size.height;
		size.height = 1;
	}

	for (int y = 0; y < size.height; ++y) {

		const uchar* img_p[8];
		for (int i = 0; i < 8; ++i)
			img_p[i] = planes[i].ptr <uchar> (y);

		uchar* res_p = result.ptr <uchar> (y);

		for (int x = 0; x < size.width; ++x) {
			uchar val = 0;
			for (int i = 8; i > 0; --i) {
				val <<= 1;
				val += img_p[i-1][x] > 0 ? 1 : 0;
			}
			res_p[x] = val;
		}

	}

	return result;
}

// ===============================================================================================
//
// NKB2GRAY
//
// ===============================================================================================

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
//	for (int bit = 1; bit < 8; ++bit)
//		i ^= (i & 1 << bit) >> 1;
	return i ^= i >> 1;
}

static uchar graydecode(uchar b) {
   b ^= b >> 1;
   b ^= b >> 2;
   b ^= b >> 4;

   return b;
 }

cv::Mat nkb2gray(const cv::Mat & img, bool reverse = false) {

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
			if (reverse)
				res_p[x] = graydecode(img_p[x]);
			else
				res_p[x] = graycode(img_p[x]);


	}

	return result;
}

// ===============================================================================================
//
// RLE
//
// ===============================================================================================

struct RLEHeader {
	uint8_t first_symbol;
	uint16_t width;
	uint16_t height;
	uint8_t type;
};

template <typename T>
static std::string binary(T i)
{
	std::string result;
	for (int bit = 0; bit < sizeof(T)*8; ++bit) {
		result = (char)((i & 1) + '0') + result;
		i >>= 1;
	}
	return result;
}

struct RleCodebook {
	RleCodebook(int type) : INTERVALS(7), m_type(type) {
		prefixes[0] = 0x00;
		prefixes[1] = 0x02;
		prefixes[2] = 0x06;
		prefixes[3] = 0x0E;
		prefixes[4] = 0x1E;
		prefixes[5] = 0x3E;
		prefixes[6] = 0x7E;

		pref_msk[0] = 0x80;
		pref_msk[1] = 0xC0;
		pref_msk[2] = 0xE0;
		pref_msk[3] = 0xF0;
		pref_msk[4] = 0xF8;
		pref_msk[5] = 0xFC;
		pref_msk[6] = 0xFE;

		pref_res[0] = 0x00;
		pref_res[1] = 0x80;
		pref_res[2] = 0xC0;
		pref_res[3] = 0xE0;
		pref_res[4] = 0xF0;
		pref_res[5] = 0xF8;
		pref_res[6] = 0xFc;

		pref_len[0] = 1;
		pref_len[1] = 2;
		pref_len[2] = 3;
		pref_len[3] = 4;
		pref_len[4] = 5;
		pref_len[5] = 6;
		pref_len[6] = 7;

		if (type == 0) {
			data_len[0] = 0;
			data_len[1] = 1;
			data_len[2] = 2;
			data_len[3] = 3;
			data_len[4] = 4;
			data_len[5] = 10;
			data_len[6] = 25;
		} else if (type == 1) {
			data_len[0] = 0;
			data_len[1] = 0;
			data_len[2] = 1;
			data_len[3] = 2;
			data_len[4] = 4;
			data_len[5] = 10;
			data_len[6] = 25;
		} else if (type == 2) {
			data_len[0] = 1;
			data_len[1] = 2;
			data_len[2] = 3;
			data_len[3] = 4;
			data_len[4] = 5;
			data_len[5] = 10;
			data_len[6] = 25;
		} else if (type == 3) {
			data_len[0] = 1;
			data_len[1] = 3;
			data_len[2] = 5;
			data_len[3] = 7;
			data_len[4] = 9;
			data_len[5] = 11;
			data_len[6] = 25;
		} else if (type == 4) {
			data_len[0] = 2;
			data_len[1] = 3;
			data_len[2] = 4;
			data_len[3] = 5;
			data_len[4] = 6;
			data_len[5] = 7;
			data_len[6] = 25;
		} else if (type == 5) {
			data_len[0] = 1;
			data_len[1] = 4;
			data_len[2] = 5;
			data_len[3] = 6;
			data_len[4] = 7;
			data_len[5] = 8;
			data_len[6] = 25;
		}

		int last = 0;
		for (int i = 0; i < 7; ++i) {
			int range = 1 << data_len[i];
			data_msk[i] = (1 << data_len[i]) - 1;
			data_min[i] = last+1;
			data_max[i] = last + range;
			last = data_max[i];
			//std::cout << data_min[i] << "-" << data_max[i] << std::endl;
		}
	}

	int prefixes[7];
	int pref_msk[7];
	int pref_res[7];
	int pref_len[7];
	int data_len[7];
	int data_msk[7];
	int data_min[7];
	int data_max[7];

	int INTERVALS;

	int getType() {
		return m_type;
	}

private:
	int m_type;
};

class RleBuffer {
public:
	RleBuffer(RleCodebook cb = RleCodebook(0), int w = 0, int h = 0) : m_tmp(0), m_tmp_size(0), m_read_pos(0), m_read_buf(0), m_read_size(0), codebook(cb) {
		m_header.width = w;
		m_header.height = h;
		m_header.type = cb.getType();
	}

	void setFirstSymbol(uint8_t s) {
		m_header.first_symbol = s;
	}

	uint8_t getFirstSymbol() {
		return m_header.first_symbol;
	}

	uint16_t getWidth() {
		return m_header.width;
	}

	uint16_t getHeight() {
		return m_header.height;
	}

	void saveToFile(const std::string & filename) {
		std::ofstream f(filename.c_str(), std::ios_base::out | std::ios_base::binary);
		saveToFile(f);
	}

	void loadFromFile(const std::string & filename) {
		std::ifstream f(filename.c_str(), std::ios_base::in | std::ios_base::binary);
		loadFromFile(f);
	}

	void saveToFile(std::ofstream & f) {
		uint32_t tmp = m_buffer.size();
		f.write((char*)&(m_header.first_symbol), 2);
		f.write((char*)&(m_header.width), 4);
		f.write((char*)&(m_header.height), 4);
		f.write((char*)&(m_header.type), 2);
		f.write((char*)&tmp, 4);
		f.write((char*)&(m_buffer[0]), sizeof(uint32_t) * m_buffer.size());
	}

	void loadFromFile(std::ifstream & f) {
		uint32_t tmp;
		f.read((char*)&(m_header.first_symbol), 2);
		f.read((char*)&(m_header.width), 4);
		f.read((char*)&(m_header.height), 4);
		f.read((char*)&(m_header.type), 2);
		f.read((char*)&tmp, 4);
		m_buffer.resize(tmp);
		f.read((char*)&(m_buffer[0]), sizeof(uint32_t) * m_buffer.size());
		codebook = RleCodebook(m_header.type);
	}

	int getNextLength() {
		fillRead();


		uint8_t tmp = (m_read_buf >> 56);
		uint32_t data = m_read_buf >> 32;

		//std::cout << binary(m_read_buf) << " " << binary(tmp) << " " << binary(data) << " " << m_read_pos << std::endl;

		for (int i = 0; i < codebook.INTERVALS; ++i) {
			uint8_t r = tmp & codebook.pref_msk[i];
			//std::cout << binary(r) << std::endl;
			if ((tmp & codebook.pref_msk[i]) == codebook.pref_res[i]) {
				data >>= (32 - codebook.data_len[i] - codebook.pref_len[i]);
				data &= codebook.data_msk[i];
				data += codebook.data_min[i];

				m_read_buf <<= codebook.data_len[i] + codebook.pref_len[i];
				m_read_size -= codebook.data_len[i] + codebook.pref_len[i];

				//std::cout << "Got: " << data << std::endl;

				return data;
			}
		}
		return 0;
	}

	RleBuffer & add(int len) {
		int i;

		for (i = 0; i < codebook.INTERVALS; ++i) {
			if (len <= codebook.data_max[i]) {
				addSymbol(codebook.prefixes[i], codebook.pref_len[i]);
				addSymbol(len - codebook.data_min[i], codebook.data_len[i]);
				//printf("0x%02x %d %d %d\n", prefixes[i], pref_len[i], len, data_len[i]);
				break;
			}
		}

		if (i >= codebook.INTERVALS) {
			//std::cout << "Block too big!";
		}

		return *this;
	}

	int size() {
		return m_buffer.size() * 4;
	}

	void finish() {
		if (m_tmp_size > 0) {
			addSymbol(0, 32-m_tmp_size);
		}
	}

protected:
	void addSymbol(int symb, int len) {
		if (len < 1)
			return;

		//std::cout << "B: " << m_tmp << " " << m_tmp_size << std::endl;
		m_tmp_size += len;
		m_tmp <<= len;
		m_tmp |= symb;
		//std::cout << "A: " << m_tmp << " " << m_tmp_size << std::endl;
		reduce();
	}

	void reduce() {
		uint32_t tmp;
		uint64_t old = m_tmp;
		int tail = m_tmp_size - 32;
		if (tail >= 0) {
			old >>= tail;
			tmp = old & 0xFFFFFFFF;
			m_buffer.push_back(tmp);
			m_tmp_size -= 32;
			//std::cout << "Inserted: " << binary(tmp) << std::endl;
		}
	}

	void fillRead() {
		uint64_t tmp;
		if ( (m_read_size < 32) && (m_read_pos < m_buffer.size())) {
			tmp = m_buffer[m_read_pos];
			//std::cout << "From buffer: " << binary(tmp) << std::endl;

			tmp <<= (32 - m_read_size);
			m_read_buf |= tmp;

			m_read_pos++;
			m_read_size += 32;
		}
	}

private:
	std::vector<uint32_t> m_buffer;
	uint64_t m_tmp;
	uint32_t m_tmp_size;

	size_t m_read_pos;
	uint64_t m_read_buf;
	uint64_t m_read_size;

	RLEHeader m_header;

	RleCodebook codebook;
};

cv::Mat rle(RleBuffer & buf) {
	cv::Mat img = cv::Mat::zeros(buf.getHeight(), buf.getWidth(), CV_8UC1);

	cv::Size size = img.size();
	uint32_t ctr = 0;
	uchar current_symbol = buf.getFirstSymbol();

	if (img.isContinuous()) {
		size.width *= size.height;
		size.height = 1;
	}

	for (int y = 0; y < size.height; ++y) {

		uchar* img_p = img.ptr <uchar> (y);

		for (int x = 0; x < size.width; ++x) {
			ctr = buf.getNextLength();
			for (int i = 0; i < ctr; ++i)
				img_p[x+i] = current_symbol;

			current_symbol = 255 - current_symbol;
			x += ctr-1;
		}
	}

	return img;
}

RleBuffer rle(const cv::Mat & img, int type = 0) {

	if (img.channels() != 1) {
		std::cout << "rle: img must be one channel!\n";
		return RleBuffer(0, 0);
	}

	RleBuffer result(RleCodebook(type), img.size().width, img.size().height);

	cv::Size size = img.size();
	uint32_t ctr = 0;
	uchar current_symbol = 128;

	if (img.isContinuous()) {
		size.width *= size.height;
		size.height = 1;
	}

	for (int y = 0; y < size.height; ++y) {

		const uchar* img_p = img.ptr <uchar> (y);

		for (int x = 0; x < size.width; ++x) {
			if (current_symbol == 128) {
				current_symbol = img_p[x];
				result.setFirstSymbol(current_symbol);
			}

			if (img_p[x] != current_symbol) {
				result.add(ctr);
				ctr = 1;
				current_symbol = 255-current_symbol;
			} else {
				ctr++;
			}
		}

		result.add(ctr);
	}

	result.finish();

	//std::cout << "Uncompressed size: " << (img.size().width * img.size().height) / 8 << std::endl;
	//std::cout << "Compressed size:   " << result.size() << std::endl;

	return result;
}

// ===============================================================================================
//
// XOR
//
// ===============================================================================================

cv::Mat en_xor(const cv::Mat & img) {

	if (img.channels() != 1) {
		std::cout << "getBitPlane: img must be one channel!\n";
		return cv::Mat();
	}

	cv::Mat result = img.clone();
	cv::Size size = img.size();

	//if (img.isContinuous() && result.isContinuous()) {
	//	size.width *= size.height;
	//	size.height = 1;
	//}

	for (int y = 0; y < size.height; ++y) {

		const uchar* img_p = img.ptr <uchar> (y);
		uchar* res_p = result.ptr <uchar> (y);

		res_p[0] = img_p[0];

		for (int x = 1; x < size.width; ++x)
			res_p[x] = img_p[x-1] ^ img_p[x];
	}

	return result;
}

cv::Mat de_xor(const cv::Mat & img) {

	if (img.channels() != 1) {
		std::cout << "getBitPlane: img must be one channel!\n";
		return cv::Mat();
	}

	cv::Mat result = img.clone();
	cv::Size size = img.size();

	//if (img.isContinuous() && result.isContinuous()) {
	//	size.width *= size.height;
	//	size.height = 1;
	//}

	for (int y = 0; y < size.height; ++y) {

		const uchar* img_p = img.ptr <uchar> (y);
		uchar* res_p = result.ptr <uchar> (y);

		res_p[0] = img_p[0];

		for (int x = 1; x < size.width; ++x)
			res_p[x] = res_p[x-1] ^ img_p[x];
	}

	return result;
}

// ===============================================================================================
//
// Bayer split/merge
//
// ===============================================================================================

std::vector<cv::Mat> bayerSplit(const cv::Mat & img) {
	cv::Size size = img.size();
	std::vector<cv::Mat> channels;
	cv::Mat ch_r(img.size().height/2 + img.size().height%2, img.size().width/2 + img.size().width%2, CV_8UC1);
	cv::Mat ch_g(img.size().height, img.size().width/2, CV_8UC1);
	cv::Mat ch_b(img.size().height/2, img.size().width/2, CV_8UC1);
	channels.push_back(ch_r);
	channels.push_back(ch_g);
	channels.push_back(ch_b);

	for (int y = 0; y < size.height; ++y) {

		const uchar* img_p = img.ptr <uchar> (y);
		uchar* img_r = ch_r.ptr <uchar> (y/2);
		uchar* img_g = ch_g.ptr <uchar> (y);
		uchar* img_b = ch_b.ptr <uchar> (y/2);


		for (int x = 0; x < size.width; ++x) {
			if ( (y % 2) && (x % 2) )
				img_b[x/2] = img_p[(3*x)+0];
			else if ( (y % 2) || (x % 2) )
				img_g[x/2] = img_p[(3*x)+1];
			else
				img_r[x/2] = img_p[(3*x)+2];
		}
	}

	cv::imwrite("r.bmp", ch_r);
	cv::imwrite("g.bmp", ch_g);
	cv::imwrite("b.bmp", ch_b);

	return channels;

}

cv::Mat bayerMerge(std::vector<cv::Mat> & channels) {
	if (channels.size() != 3) {
		std::cout << "Can't merge bayer! Should be 3 channels, got " << channels.size() << std::endl;
		return cv::Mat();
	}

	cv::Size size( channels[1].size().height, channels[0].size().width + channels[2].size().width );
	cv::Mat res = cv::Mat::zeros(size, CV_8UC1);

	for (int y = 0; y < size.height; ++y) {

		uchar* img_p = res.ptr <uchar> (y);
		const uchar* img_r = channels[0].ptr <uchar> (y/2);
		const uchar* img_g = channels[1].ptr <uchar> (y);
		const uchar* img_b = channels[2].ptr <uchar> (y/2);


		for (int x = 0; x < size.width; ++x) {
			// blue pixel
			if ( (y % 2) && (x % 2) ) {
				img_p[x] = img_b[x/2];
			}
			// green pixel
			else if ( (y % 2) || (x % 2) ) {
				img_p[x] = img_g[x/2];
			}
			// red pixel
			else {
				img_p[x] = img_r[x/2];
			}
		}
	}

	//cv::imwrite("bay.bmp", res);

	return res;
}

// ===============================================================================================
//
// Huffman encoding
//
// ===============================================================================================

#include "huffman.h"

using namespace std;

int * compute_histogram (ifstream & inf, int size, bool bits16, int & last)
{
	int * hist = new int [size];
	for (int i = 0; i < size; i++)
		hist [i] = 0;

	for (;;)
	{
		int chr = inf.get ();
		if (inf.eof ())
			break;
		if (bits16)
		{
			int ch = inf.get ();
			if (inf.eof ())
			{
				last = chr;
				break;
			}
			chr |= ch << 8;
		}
		hist [chr] ++;
	}
	return hist;
}

void enchuf(const std::string & in_f, const std::string & out_f)
{
	int i;
	bool bits16 = false;

	ifstream inf (in_f.c_str(), ios::in | ios::binary);
	if (inf.fail ())
	{
		cerr << "error : unable to open input file '" << in_f << "'." << endl;
		return;
	}

	BitFileOut outf (out_f.c_str());

	int last = -1;

	int size = bits16 ? 65536 : 256;

	int * hist = compute_histogram (inf, size, bits16, last);

	inf.clear ();
	inf.seekg (0, ios::end);
	int input_count = inf.tellg ();

	if (input_count > 0)
	{
		vector<Node *> data;

		data.push_back (new Leaf (0, -1)); // end of file marker
		for (i = 0; i < size; i++)
		{
			if (hist [i] != 0)
			{
			   data.push_back (new Leaf (hist [i], i));
			}
		}

		Table<Encoding> table;

		Node * const root = BuildHuffman (data);

		if (root != NULL)
		{
			i = 0;
			string tmp;
			root -> Encode (i, tmp, table);
			delete root;
		}

		Table<int> index;
		for (i = table.Base (); i <= table.Summit (); i++)
			index [table [i].huffman_code] = i;

		char big = bits16 ? 8 : 0;

		for (i = table.Base (); i <= table.Summit (); i++)
			if (table [i].huffman_string.size () >= 256)
			{
				big |= 1;
				break;
			}

		int cnt = table.Summit () - table.Base ();
		if (cnt >= 256)
			big |= 2;

		if (last != -1)
			big |= 4;

		outf.put (make_string (big, 8));

		if (big&4)
		{
			outf.put (make_string (last, 8));
		}

		if (cnt > 0)
		{
			int ch = cnt - 1;
			outf.put (make_string (ch, (big&2) ? 16 : 8));
		}

		i = index [-1];
		int ch = i;
		outf.put (make_string (ch, 32));
		ch = table [i].huffman_string.size ();
		outf.put (make_string (ch, (big&1) ? 16 : 8));

		for (i = table.Base (); i <= table.Summit (); i++)
			if (table [i].huffman_code != -1)
			{
				ch = table [i].huffman_code;
				outf.put (make_string (ch, bits16 ? 16 : 8));
				ch = table [i].huffman_string.size ();
				outf.put (make_string (ch, (big&1) ? 16 : 8));
			}

		inf.clear ();
		inf.seekg (0);
		for (;;)
		{
			int chr = inf.get ();
			if (inf.eof ())
				break;

			if (bits16)
			{
				int ch = inf.get ();
				if (inf.eof ())
					break;
				chr |= ch << 8;
			}

			outf.put (table [index [chr]].huffman_string);
		}
		outf.put (table [index [-1]].huffman_string);
	}

	delete [] hist;

	int output_count = outf.length ();

	//std::cout << "After Huffman: " << input_count << "->" << output_count << " (" << (100 - 100.0*output_count/input_count) << "% less)\n";
}

void dechuf(const std::string & in_f, const std::string & out_f)
{
	BitFileIn inf (in_f.c_str());

	ofstream outf (out_f.c_str(), ios::out | ios::binary);
	if (outf.fail ())
	{
		cerr << "error : unable to open output file '" << out_f << "'." << endl;
		return;
	}

	int input_count = inf.length ();

	if (input_count > 0)
	try
	{
		char big = inf.read_bits (8);
		bool bits16 = (big&8) != 0;

		int last = -1;
		if (big&4)
			last = inf.read_bits (8);

		int size = bits16 ? 65536 : 256;

		int cnt = inf.read_bits (big&2 ? 16 : 8);
		cnt += 1;

		if (cnt > 0)
		{
			dataS * data = new dataS [size + 1];
			int j;
			for (j = 0; j < size + 1; j++)
				data [j].size = 0;

			j = inf.read_bits (32);
			data [j].code = -1;
			data [j].size = inf.read_bits ((big&1) ? 16 : 8);
			j = 0;

			int i;
			for (i = 0; i < cnt; i++)
			{
				if (data [j].size != 0)
					j ++;
				data [j].code = inf.read_bits (bits16 ? 16 : 8);
				data [j].size = inf.read_bits ((big&1) ? 16 : 8);
				j ++;
			}

			int ii = 0;
			Node * root = build (ii, data, j, 0);

			delete [] data;

			Node * node = root;
			for (;;)
			{
				node = node -> Decend (inf.GetBit ());

				if (node == NULL)
					throw __LINE__;

				if (node -> IsLeaf ())
				{
					int code = node -> Code ();
					if (code == -1)
						break;

					outf << static_cast<unsigned char>(code);
					if (bits16) outf << static_cast<unsigned char>(code>>8);

					node = root;
				}
			}

			if (last != -1)
			{
				outf << static_cast<unsigned char>(last);
			}
		}
	}
	catch (int line)
	{
		cerr << "error ("<<line<<"): not a huffman encoded file." << endl;
		return;
	}

	int output_count = outf.tellp ();

	std::cout << "After Huffman: " << input_count << "->" << output_count << " (" << (100 - 100.0*output_count/input_count) << "% less)\n";
}

// ===============================================================================================
//
// Encode and decode routines
//
// ===============================================================================================

struct Header {
	bool gray;
	bool exor;
	int channels;
	// 1 - RGB, 2 - HSV
	int conversion;
	// 0 - none, 1 - huffman
	int post;
};

void storeRaw(std::ofstream & f, const std::string & fname) {
	std::ifstream inf(fname.c_str(), std::ios_base::in | std::ios_base::binary);
	inf.clear ();
	inf.seekg (0, ios::end);
	int input_count = inf.tellg ();
	inf.seekg (0, std::ios::beg);

	//std::cout << input_count << std::endl;
	char * buf = new char[input_count];

	inf.read(buf, input_count);
	f.write((char*)&input_count, 4);
	f.write(buf, input_count);

	inf.close();
	delete [] buf;
}

void retrieveRaw(std::ifstream & f, const std::string & fname) {
	std::ofstream outf(fname.c_str(), std::ios_base::out | std::ios_base::binary);

	int input_count;
	f.read((char*)&input_count, 4);
	//std::cout << input_count << std::endl;

	char * buf = new char[input_count];
	f.read(buf, input_count);
	outf.write(buf, input_count);

	outf.close();
	delete [] buf;
}

bool encode(const std::string & in_fname, const std::string & out_fname, Header header) {


	cv::Mat img = cv::imread(in_fname.c_str());
	if (img.empty()) {
		std::cout << "Can't load image from file: " << in_fname << std::endl;
		return false;
	}

	std::ofstream f(out_fname.c_str(), std::ios_base::out | std::ios_base::binary);

	std::vector<cv::Mat> channels;

	if (img.channels() > 1) {
		// split image into channels
		if (header.conversion == 3) {
			channels = bayerSplit(img);
		} else {
			if (header.conversion == 2) {
				cv::cvtColor(img, img, CV_BGR2HSV);
			}
			cv::split(img, channels);
		}
	} else {
		// image is one channel
		channels.push_back(img);
	}

	header.channels = channels.size();

	f.write((char*)&header, sizeof(header));

	if (header.gray) {
		for (int i = 0; i < channels.size(); ++i) {
			channels[i] = nkb2gray(channels[i]);
		}
	}

	// split image into bitplanes
	for (int i = 0; i < header.channels; ++i) {
		for (int p = 0; p < 8; ++p) {
			int best = 0;
			int bestt = -1;
			cv::Mat bp = getBitPlane(channels[i], p);
			if (header.exor) {
				bp = en_xor(bp);
			}
			for (int type=0; type < 6; ++type) {
				RleBuffer buf = rle(bp, type);
				if (bestt < 0 || buf.size() < best) {
					best = buf.size();
					bestt = type;
				}
			}

			RleBuffer buf = rle(bp, bestt);
			//std::cout << i << p << ": " << bestt << "@" << buf.size() << std::endl;

			if (header.post == 1) {
				buf.saveToFile("tmp");
				enchuf("tmp", "tmp.huf");
				storeRaw(f, "tmp.huf");
			} else {
				buf.saveToFile(f);
			}
		}
	}

	return true;
}

bool decode(const std::string & in_fname, const std::string & out_fname) {
	Header header;
	cv::Mat tmp;

	std::ifstream f(in_fname.c_str(), std::ios_base::out | std::ios_base::binary);

	std::vector<cv::Mat> channels;

	f.read((char*)&header, sizeof(header));
	for (int i = 0; i < header.channels; ++i) {
		std::vector<cv::Mat> planes;
		for (int p = 0; p < 8; ++p) {
			RleBuffer buf;
			if (header.post == 1) {
				retrieveRaw(f, "tmp.huf.raw");
				dechuf("tmp.huf.raw", "tmp.raw");
				buf.loadFromFile("tmp.raw");
			} else {
				buf.loadFromFile(f);
			}
			tmp = rle(buf);

			if (header.exor) {
				tmp = de_xor(tmp);
			}

			planes.push_back(tmp);
		}


		tmp = mergeBitPlanes(planes);
		if (header.gray)
			channels.push_back(nkb2gray(tmp, true));
		else
			channels.push_back(tmp);
	}

	if (header.conversion == 3) {
		tmp = bayerMerge(channels);
		cv::cvtColor(tmp.clone(), tmp, CV_BayerBG2BGR);
	} else {
		cv::merge(channels, tmp);

		if (header.conversion == 2) {
			cv::cvtColor(tmp, tmp, CV_HSV2BGR);
		}
	}

	cv::imwrite(out_fname.c_str(), tmp);

	return true;
}

// ===============================================================================================
//
// Entry point
//
// ===============================================================================================


namespace po = boost::program_options;

int main(int argc, char * argv[]) {
	// Declare the supported options.

	std::string conversion;
	std::string input_fname;
	std::string output_fname;

	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("conversion,C", po::value<std::string>(&conversion)->default_value("RGB"), "colorspace conversion\n"
				"possible values are: RGB, HSV, Bayer")
		("gray,G", "convert channels to Gray encoding")
		("xor,X", "xor bit planes")
		("huffman,H", "Huffman encoding")
		("decode,D", "decode given file")
		("input,I",po::value<std::string>(&input_fname), "input file")
		("output,O",po::value<std::string>(&output_fname), "output file")
	;

	po::variables_map vm;

	try {
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
	}
	catch (const po::error & u) {
		std::cout << u.what() << "\n";
		return 0;
	}

	if (vm.count("help") || argc < 2) {
		std::cout << desc << "\n";
		return 0;
	}

	if (input_fname == "") {
		std::cout << "No input file specified.\n";
		return 0;
	}

	if (output_fname == "") {
		output_fname = input_fname + ".rle";
	}

	Header header;
	if (vm.count("gray")) {
		header.gray = 1;
	} else {
		header.gray = 0;
	}

	if (vm.count("xor")) {
		header.exor = 1;
	} else {
		header.exor = 0;
	}

	if (vm.count("huffman")) {
		header.post = 1;
	} else {
		header.post = 0;
	}

	if (conversion == "RGB") {
		header.conversion = 1;
	} else
	if (conversion == "HSV") {
		header.conversion = 2;
	} else
	if (conversion == "Bayer") {
		header.conversion = 3;
	} else {
		std::cout << "Unknown conversion: " << conversion << "\n";
		return 0;
	}


	if (vm.count("decode")) {
		decode(input_fname, output_fname);
	} else {
		encode(input_fname, output_fname, header);
	}

	return 0;
}
