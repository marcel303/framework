#ifndef PORT_H
#define PORT_H

// I did this is because of portability reasons. If any time you don't wish
// JGMOD to use Allegro Packfile routines, just remark the
// "#define JGMOD_PACKFILE" line below and unremark the "
// #define JGMOD_STDFILE" line


#define JGMOD_PACKFILE    /* dont try to load datafile with this disabled */
//#define JGMOD_STDFILE


#ifdef JGMOD_PACKFILE
    #define JGMOD_FILE PACKFILE
#else
    #define JGMOD_FILE FILE
#endif

#endif
