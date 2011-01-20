/*!
 * \file
 * \brief
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdio>
#include <fstream>
#include <cstring>
#include <fstream>
#include <string>
#include <algorithm>

struct huff_entry {
	huff_entry() : symbol(0), count(0), length(0) {

	}

	unsigned char symbol;
	int count;
	int length;
	std::string rep;

	bool operator < (const huff_entry & rhs) const {
		return count < rhs.count;
	}
};

class HuffmanBuffer {
public:
	HuffmanBuffer() {
		symbols.resize(256);
		for (int i = 0; i < 256; ++i) {
			symbols[i].symbol = i;
		}

		u_size = 0;
		c_size = 0;
	}

	void fill(const std::string & fname) {
		std::ifstream file(fname.c_str(), std::ios_base::in | std::ios_base::binary);

		while (!file.eof()) {
			unsigned char ch = file.get();
			symbols[ch].count++;
			u_size += 8;
		}
	}

	void print() {
		for (int i = 0; i < 256; ++i) {
			printf("%3d | %10d\n", symbols[i].symbol, symbols[i].count);
		}

		std::cout << "usize: " << u_size << std::endl;
	}

	void sort() {
		std::sort(symbols.begin(), symbols.end());
	}

	void prepare() {
		r_prepare(0, 255, u_size / 8);
	}

	void r_prepare(int from, int to, int total) {
		int sum_l = symbols[from].count, sum_u = total - sum_l;
		int diff = abs(sum_l - sum_u), min_diff = total;

		int current_slice = from;

		while (diff < min_diff) {
			current_slice++;

			min_diff = diff;

			sum_l += symbols[current_slice].count;
			sum_u = total - sum_l;
			diff = abs(sum_l - sum_u);
		}

		if (current_slice != from) {
			r_prepare(from, current_slice, sum_l);
		}
	}


private:
	std::vector<huff_entry> symbols;

	int u_size, c_size;
};

int main(int argc, char * argv[]) {


	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " FILE" << std::endl;
		return 0;
	}

	HuffmanBuffer buffer;
	buffer.fill(argv[1]);
	buffer.print();
	buffer.sort();
	buffer.print();

	return 0;
}
