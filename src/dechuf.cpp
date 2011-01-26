//
// dechuf.cpp
//
// (c) Copyright 2000-2002 William A. McKee. All rights reserved.
//
// April 29, 2000 - Created.
//
// Decompress a Huffman encoded file. (see enchuf.cpp)
//
// Compiled with MS VC++ 6.0: cl /Ox /GX dechuf.cpp
//
// November 4, 2000 - Version 2.0
//   - added 16 bits writes
//   - optimized BuildHuffman
//   - better storage of Huffman tree
//
// March 23, 2002 - Version 3.0 - Added BitFileOut class and cleaned things up
//
// December 8, 2002 - cleaned up code
//

#include <ctime>

#include "huffman.h"

using namespace std;

void print_stats (clock_t t, int input_count, int output_count);

int main (int argc, char * * argv)
{
    if (argc < 3)
    {
        cerr << "usage: dechuf <file in> <file out>" << endl;
        return EXIT_FAILURE;
    }

    BitFileIn inf (argv [1]);

    ofstream outf (argv [2], ios::out | ios::binary);
    if (outf.fail ())
    {
        cerr << "error : unable to open output file '" << argv [2] << "'."
            << endl;
        return EXIT_FAILURE;
    }

    clock_t start_time = clock ();

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
        return EXIT_FAILURE;
    }

    int output_count = outf.tellp ();

    clock_t end_time = clock ();

    print_stats (end_time-start_time, input_count, output_count);

    return EXIT_SUCCESS;
}

void print_stats (clock_t t, int input_count, int output_count)
{
    double timer = (double) t / CLOCKS_PER_SEC;

    cout << input_count << " characters input." << endl;
    cout << output_count << " characters output." << endl;
    cout << timer << " seconds." << endl;
    if (timer > 0.0)
        cout << output_count / timer << " characters per second." << endl;
    else
        cout << "? characters per second." << endl;
}

