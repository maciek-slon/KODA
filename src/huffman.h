//
// Huffman.h
//
// (c) Copyright 2000-2002 William A. McKee.  All rights reserved.
//
// Please send question and comments to wamckee@msn.com
//
// April 29, 2000 - Created.
//
// March 22, 2002 - Added MakeCanonical and other helper functions.
//
// March 23, 2002 - Added BitFileIn and BitFileOut classes
//
// March 26, 2002 - Compiled against GNU C++ compiler
//
// December 7, 2002 - Commented
//

/*
 * Huffman encoding is used to compress data.
 *
 * The compression is a result of mapping a fixed number of bits
 * onto a variable number of bits.  Each value is assigned a variable bit
 * encoding according to its weight.  The higher the weight (more frequent a
 * value) the fewer bits used to encode that particular value.  The weight
 * is usually computed from the frequency of occurrence of each value in the
 * input file but does not strictly need to be.  If for example you want to
 * compress many files with the same tree, you can compute the overall
 * frequency of occurance and reuse the same Huffman tree for each file.
 *
 * The only down-side is that you must not only store the values but you
 * must also now store the Huffman tree as well.  This may lead to data
 * expansion in some cases where the weights are all roughly equal.
 * Furthermore, since we must explictly store the end-of-file as a value,
 * the number of bits used to encode some values may be more than the fixed
 * bit size.
 *
 * The actual Huffman tree building algorithm is quite simple:
 *
 * 1) Compute the weights for all values.
 * 2) Construct an array of leaf nodes containing the weights and values.
 * 3) Find the two nodes with the smallest weights and make a sub-tree
 *    where the weight of the new sub-tree is the sum of the weights of the
 *    children. Remove the two children from the array and add the sub-tree.
 * 4) Iterate point 3 until there is only a single node left in the array.
 * 5) Assign 0 to all left branchs and 1 to all right branchs in the tree. This
 *    is an arbitrary choice and we see that in MakeCanonical, we can take
 *    further advantage of this.
 * 6) Recursively decend the tree remembering the branch encoding as we go and
 *    when we reach a leaf, record the resulting encoding.
 *
 * To compress the data:
 *
 * 1) Save the Huffman tree.
 * 2) Substitute each input value with the variable bit encoding for that value.
 * 3) Output the variable bit encoding for the end-of-file value.
 *
 * To decompress the data:
 *
 * 1) Recover the original Huffman encoding tree.
 * 2) Set a pointer to the root of the tree.
 * 2) Read the input one bit at a time and for each bit decend down either
 *    the right or left child in the tree by changing the pointer.
 * 3) When you reach a leaf, output the value found there and reset the pointer
 *    back to the root.
 * 4) Iterate points 2 and 3 until you reach an end-of-file value.
 *
 * In this implementation I used the STL where possible to keep the code tight.
 *
 */

/*
 * To prevent redefining the classes and functions in the case where
 * this header file is included more than once, we do the following.
 */

#ifndef _HUFFMAN_H_INCLUDED_
#define _HUFFMAN_H_INCLUDED_

/*
 * cstdlib - need NULL definition
 */

#include <cstdlib>

/*
 * vector, algorithm and string - used in Huffman encoding
 */

#include <vector>
#include <algorithm>
#include <string>

/*
 * iostream and fsteam are used for file i/o
 */

#include <iostream>
#include <fstream>

/*
 * table.h - light weight variable sized array class
 */

#include "table.h"

/*
 * Encoding - declared to hold a string of ones and zeros representing the
 * Huffman encoding for a particular value (huffman_code).
 */

class Encoding
{
public :
    std::string huffman_string;
    int huffman_code;
    Encoding () { }
    ~Encoding () { }
    Encoding & operator = (const Encoding & rhs)
    {
        huffman_string = rhs.huffman_string;
        huffman_code = rhs.huffman_code;
        return *this;
    }
};

/*
 * Node - base class for the Huffman tree
 */

class Node
{
private :

    /*
     * weight is a numberical value that signifies the importance of a value
     * or sub-tree.
     */
    int const weight;

protected :
    int Weight () const { return weight; }

public :
    Node () : weight (0) { }
    Node (int const w) : weight (w) { }
    virtual ~Node () {}
    
    /*
     * the weight of two sub-trees is the sum of there values
     */
    int operator + (const Node & rhs) const { return weight + rhs.weight; }

    /*
     * operator > is used when building the tree so that only the two smallest
     * weights are combined into a sub-tree
     */
    bool operator > (const Node & rhs) const { return weight > rhs.weight; }

    /*
     * Encode builds a table of encoded values
     */
    virtual void Encode (int & i, std::string & str, Table<Encoding> & table) const { };

    /* 
     * IsLeaf is used to determine when the decoding algorithm has produced a
     * value
     *
     * Code is used to get the value of the decoded bit stream
     *
     * Decend is used to traverse the Huffman tree when decoding
     *
     */
    virtual bool IsLeaf () const { return false; }
    virtual int Code () const { return 0; }
    virtual Node * Decend (int bit) const { return NULL; };
};

/*
 * Leaf and Interior are derived from Node and do the actual work
 */

class Leaf : public Node
{
private :

    /*
     * code is the actual value to be encoded
     */
    int const code;

public :
    Leaf () : Node (), code (-1) {}
    virtual ~Leaf () {}

    /*
     * build a leaf node
     */
    Leaf (int const w, int const c)
        : Node (w), code (c) {}

    /*
     * when we hit a leaf, make an entry in the encoding table
     */
    virtual void Encode (int & i, std::string & str, Table<Encoding> & table) const
    {
        table [i].huffman_string = str;
        table [i].huffman_code = code;
        i ++;
    }

    /*
     * see Node
     */
    virtual bool IsLeaf () const { return true; }
    virtual int Code () const { return code; }
    virtual Node * Decend (int bit) const { return NULL; }
};

class Interior : public Node
{
private :

    /*
     * left and right are the children of the sub-tree
     */
    Node * left;
    Node * right;

public :
    Interior () : Node (), left (NULL), right (NULL) { }
    virtual ~Interior () { delete left; delete right; }

    /*
     * build a sub-tree out of two children
     */
    Interior (Node * l, Node * r)
        : Node ((l == NULL ? 0 : *l) + (r == NULL ? 0 : *r)),
          left (l), right (r) {}

    /*
     * when we are decending a sub-tree, append a bit to the encoding string
     */
    virtual void Encode (int & i, std::string & str, Table<Encoding> & table) const
    {
        str += '0';
        left -> Encode (i, str, table);
        str.resize (str.size () - 1);

        str += '1';
        right -> Encode (i, str, table);
        str.resize (str.size () - 1);
    }

    /*
     * see Node
     */
    virtual Node * Decend (int bit) const { return bit ? right : left; }
};

/*
 * NodeCompare is used by std::sort to put the weights in descending order
 */

class NodeCompare
{
public :
    int operator () (const Node * const & lhs, const Node * const & rhs) const
    {
    // sort largest to smallest by weight

        return (*lhs > *rhs);
    }
};

/*
 * BuildHuffman builds the actual Huffman tree from a array of leaves
 * it returns a pointer to the root of the Huffman tree
 */

inline Node * BuildHuffman (std::vector<Node *> & data)
{
    /* the algorithm only works if there is some data in the array */

    if (! data.empty ())
    {
        /*
         * sort the array of leaves in descending order so that the two
         * smallest are at the end of the array
         */
        std::sort (data.begin (), data.end (), NodeCompare ());

        for (;;)
        {
            /*
             * get the last (smallest) sub-tree of leaf
             */
            Node * const last = data.back ();
            /*
             * remove it
             */
            data.pop_back ();
            /*
             * if it was the last node, we are done and return the tree
             */
            if (data.empty ())
                return last;

            /*
             * otherwise, get the next smallest
             */
            Node * const next = data.back ();
            /*
             * remove it
             */
            data.pop_back ();
            /*
             * add the sub-tree to the array
             */
            data.push_back (new Interior (last, next));

            /*
             * put the new sub-tree in the correct place in the array
             * to keep things in descending order
             */
            for (int i = data.size () - 2; i >= 0; i--)
                if (* data [i+1] > * data [i])
                {
                    Node * tmp = data [i];
                    data [i] = data [i+1];
                    data [i+1] = tmp;
                }
                else
                {
                    break;
                }
        }
    }

    /* if there is no data, simply return NULL */

    return NULL;
}

/*
 * make_string and make_var_string are helper functions for making
 * bit encoded strings out of integers
 *
 * see read_bits and read_var_bits in BitFileIn
 *
 */

inline std::string make_string (const int v, const int len)
{
    std::string s = "";
    for (int i = 0; i < len; i++)
        s += ((v>>(len-i-1))&1) != 0 ? '1' : '0';
    return s;
}

inline std::string make_var_string (int v)
{
    int t = v;
    int nbits = 0;
    while (t != 0)
    {
        nbits ++;
        t >>= 1;
    }
    return make_string (nbits, 5) + make_string (v, nbits);
}

/*
 * MakeCanonical is used to compute the canonical Huffman encoding
 *
 * srt_cmp is a helper function used by MakeCanonical
 *
 */

static int srt_cmp (const void * a, const void * b)
{
    Encoding * aa = * (Encoding * *) a;
    Encoding * bb = * (Encoding * *) b;

    int result = aa->huffman_string.length () - bb->huffman_string.length ();

    if (result == 0)
        result = aa->huffman_code - bb->huffman_code;

    return result;
}

inline void MakeCanonical (Table<Encoding> & table)
{
    int table_size = table.Summit () + 1;

    Encoding * * srt = new Encoding * [table_size];

    for (int i = 0; i < table_size; i++)
        srt [i] = & table [i];

    qsort (srt, table_size, sizeof (*srt), srt_cmp);

    int old_len = 0;
    int v = 0;
    for (int ii = 0; ii < table_size; ii++)
    {
        Encoding * p = srt [ii];

        int len = p->huffman_string.length ();
        if (old_len != len)
        {
            v <<= len - old_len;
            old_len = len;
        }
        p->huffman_string = make_string (v, len);
        v ++;
    }

    delete [] srt;
}

/*
 * build is a function that builds a Huffman tree from a list of dataS
 * elements (used in decoding a compressed representation of a Huffman tree)
 */

struct dataS { int code; int size; };

static Node * build (int & i, dataS * data, int n, int level)
{
    level ++;

    Node * l;
    if (data [i].size - level == 0)
    {
        l = new Leaf (0, data [i].code);
        i ++;
    }
    else
        l = build (i, data, n, level);

    Node * r;
    if (data [i].size - level == 0)
    {
        r = new Leaf (0, data [i].code);
        i ++;
    }
    else
        r = build (i, data, n, level);

    return new Interior (l, r);
}

/*
 * BitFileOut and BitFileIn are helper classes that do the bit i/o
 */

class BitFileOut
{
private :

    std::ofstream fp;

    int obc;
    char och;
    
public :

    BitFileOut (const char * fn)
    {
        fp.open (fn, std::ios::out | std::ios::binary);
        if (fp.fail ())
        {
            std::cerr << 
                "error : unable to open file '" << fn << "' for output." <<
                std::endl;
            exit (EXIT_FAILURE);
        }
        obc = 0;
        och = 0;
    }

    ~BitFileOut ()
    {
        if (obc != 0)
        {
            fp << och;
            if (fp.fail ())
            {
                std::cerr << 
                    "error : disk full while writing data." << std::endl;
                exit (EXIT_FAILURE);
            }
            obc = 0;
            och = 0;
        }
    }

    void put (const std::string & str)
    {
        for (int i = 0; i < (int) str.length (); i++)
        {
            int bit = str [i] - '0';
            och |= bit << (7-obc);
            if (++obc == 8)
            {
                fp << och;
                if (fp.fail ())
                {
                    std::cerr << 
                        "error : disk full while writing data." << std::endl;
                    exit (EXIT_FAILURE);
                }
                obc = 0;
                och = 0;
            }
        }
    }

    int length ()
    {
        return fp.tellp ();
    }
};

class BitFileIn
{
private :

    std::ifstream fp;
    int len;

    int obc;
    unsigned char och;

public :

    BitFileIn (const char * fn)
    {
        fp.open (fn, std::ios::in | std::ios::binary);
        if (fp.fail ())
        {
            std::cerr <<
                "error : unable to open file '" << fn << "' for input." <<
                std::endl;
            exit (EXIT_FAILURE);
        }

        len = 0;
        for (;;)
        {
            fp.get ();
            if (! fp.eof ()) len ++; else break;
        }

        fp.clear ();
        fp.seekg (0);

        obc = 8;
        och = 0;
    }

    ~BitFileIn ()
    {
    }

    int length () { return len; }

    int GetBit ()
    {
        if (obc == 8)
        {
            och = fp.get ();
            if (fp.eof ())
                throw int ();
            obc = 0;
        }
        return (och>>(7-obc++))&1;
    }

    int read_bits (int n)
    {
        int v = 0;
        for (int i = 0; i < n; i++)
            v = (v << 1) | GetBit ();
        return v;
    }
    
    int read_var_bits ()
    {
        return read_bits (read_bits (5));
    }
};

#endif // _HUFFMAN_H_INCLUDED_

