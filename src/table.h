//
// table.h
//
// (c) Copyright 2000-2002 William A. McKee.  All rights reserved.
//
// Please send question and comments to wamckee@msn.com
//
// April 29, 2000 - Created.
//
// March 26, 2002 - Compiled against GNU C++ compiler
//
// Table template class
//
// - a light weight class for maintaining variable sized arrays
//

template <class T>
class Table
{
private :
    T * data;
    int base;
    int size;
    int limit;
    void resize (int i)
    {
        if (size == 0)
            base = i;

        if (i < base)
        {
            int d = base - i;

            if (size + d >= limit)
            {
                while (size + d >= limit)
                {
                    if (limit == 0) limit = 1;
                    else limit *= 2;
                }
            }

            T * tmp = new T [limit];
            int j;
            for (j = 0; j < size; j++)
                tmp [j + d] = data [j];
            for (j = size; j + d < limit; j++)
                tmp [j + d] = T ();
            for (j = 0; j < d; j++)
                tmp [j] = T ();
            delete [] data;
            data = tmp;

            size += d;
            base -= d;
        }
        if (i - base >= limit)
        {
            while (i - base >= limit)
            {
                if (limit == 0) limit = 1;
                else limit *= 2;
            }

            T * tmp = new T [limit];
            int j;
            for (j = 0; j < size; j++)
                tmp [j] = data [j];
            for (j = size; j < limit; j++)
                tmp [j] = T ();
            delete [] data;
            data = tmp;
        }
        if (i + 1 - base > size)
            size = i + 1 - base;
    }
public :
    Table ()
    {
        data = NULL;
        base = 0;
        size = 0;
        limit = 0;
    }
    ~Table ()
    {
        delete [] data;
    }
    T & operator [] (int rhs) { resize (rhs); return data [rhs - base]; }
    int Base () { return base; }
    int Summit () { return size + base - 1; }
};
