//
// enchuf.cpp
//
// (c) Copyright 2000-2002 William A. McKee.  All rights reserved.
//
// April 29, 2000 - Created.
//
// Compress a file using Huffman encoding.
//
// Compiled with VC++ 6.0: cl /Ox /GX enchuf.cpp
//
// November 4, 2000 - Version 2.0
//   - added /16 option for 16 bits reads
//   - optimized BuildHuffman
//   - better storage of Huffman tree
//
// March 23, 2002 - Version 3.0 - Added BitFileOut class and cleaned things up
//
// December 8, 2002 - cleaned up
//

#include <ctime>
#include <cstring>

#include "huffman.h"

using namespace std;

void parse_argv (int & argc, char * * argv, bool & bits16);
int * compute_histogram (ifstream & inf, int size, bool bits16, int & last);
void print_stats (clock_t t, int input_count, int output_count,
    char * * argv, bool bits16);

int main (int argc, char * * argv)
{
    int i;
    bool bits16 = false;

    if (argc < 3)
    {
        cerr << "usage: enchuf <file in> <file out> [/16]" << endl;
        return EXIT_FAILURE;
    }

    ifstream inf (argv [1], ios::in | ios::binary);
    if (inf.fail ())
    {
        cerr << "error : unable to open input file '" << argv [1] << "'."
            << endl;
        return EXIT_FAILURE;
    }

    BitFileOut outf (argv [2]);

    clock_t start_time = clock ();

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

    clock_t end_time = clock ();

    print_stats (end_time-start_time, input_count, output_count, argv, bits16);

    return EXIT_SUCCESS;
}

void parse_argv (int & argc, char * * argv, bool & bits16)
{
    int kk = 1;
    for (int i = 1; i < argc; i++)
    {
        if (* argv [i] == '/' || * argv [i] == '-')
        {
			if (strcmp (argv [i] + 1, "16") == 0)
            {
                bits16 = true;
            }
            else
            {
                cerr << "error : unknown option '" << argv [i] << "'."
                     << endl;
            }
        }
        else
        {
            argv [kk++] = argv [i];
        }
    }
    argc = kk;
}

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

void print_stats (clock_t t, int input_count, int output_count,
    char * * argv, bool bits16)
{
    double timer = (double) t / CLOCKS_PER_SEC;

    cout << "enchuf " << argv [1] << " " << argv [2];
    if (bits16) cout << " /16";
    cout << endl;

    cout << input_count << " characters input." << endl;

    cout << output_count << " characters output." << endl;

    if (input_count > 0)
        cout << (input_count == 0 ? 0 : (input_count - output_count) * 100 / input_count) << "% reduction in size." << endl;
    else
        cout << "0% reduction in size." << endl;

    cout << timer << " seconds." << endl;

    if (timer > 0.0)
        cout << input_count / timer << " characters per second." << endl;
    else
        cout << "? characters per second." << endl;
}
