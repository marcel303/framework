#ifndef __subdiv_h__
#define __subdiv_h__

/* >> NOTES: diamond square and offset square classes
 * note: offset square is typically faster and looks better too, so you're
 *       probably better off using the offset square algoritm
 * note to all game developers: have fun!
 *
 */

/* >> THANKS:
 *
 * I (Marcel Smit) found these algoritms on the net. Actual credits belong to:
 *
 * * Paul Martz (diamond square)
 *
 *       Original copyright notice:
 *     Use this code anyway you want as long as you don't sell it for
 *     money.
 *      Paul Martz, 1995
 *
 *
 *
 * * James McNeill (offset square)
 *
 *       Original copyright notice:
 *     This code was inspired by looking at Tim Clark's (?) Mars demo; hence the name
 *     "Moonbase".  (Moonbase is also the name of our student-run Linux server.)
 *     I believe the Mars demo is still available on x2ftp.oulu.fi.  You are free
 *     to use my code, my ideas, whatever.  I have benefited enormously from the free
 *     information on the Internet, and I would like to keep that process going.
 *      James McNeill
 *   mcneja@wwc.edu
 *
 * Thanks for sharing your code - I really appreciate it!
 *
 * note: The sources I used are based on the code written by the poeple above,
 * and were already rewritten by someone else, in Java. Unfortunatly I don't
 * remember his/her name. I heavily modified, optimized and ported them to C++.
 * Offset-square algo has been optimized most.
 *
 */

class subdiv_t {

 public:
	subdiv_t();
	virtual ~subdiv_t();

 public:
	int** h;				// h[size][size]: 'height' values
	int size;                           	// size (must be a power of 2)

 public:
	void setSize(int size);             	// must be powers of 2
        void clear();
	void getMinMax(int* min, int* max); 	// gets minimal and maximum V values
	virtual int generate();             	// generate subdivisional landscape
	void rerange(int min, int max);		// get all values in a specific range

};

//---------------------------------------------------------------------------

class diamond_square_t : public subdiv_t {

 public:
	diamond_square_t();
	virtual ~diamond_square_t();

 public:
	virtual int generate();
	int avgyvals(int i, int j, int strut, int dim);	// helper #1
	int avgyvals2(int i, int j, int strut);         // helper #2

};

//---------------------------------------------------------------------------

class offset_square_t : public subdiv_t {

 public:
	offset_square_t();
	virtual ~offset_square_t();

 public:
	virtual int generate();

};

//---------------------------------------------------------------------------

#endif
