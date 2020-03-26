/*
---------------------------------------------------------------------------------------

This source file is base on a c++ blowfish algorithm implementation,
which is the fifth item listed on the page:

http://www.schneier.com/blowfish-download.html

As we can see, the original author is unknown (the 5th item listed on the page above).
So, this source code should be in public domain.

---------------------------------------------------------------------------------------
*/


#ifndef __BLOWFISH_H_INCLUDED__
#define __BLOWFISH_H_INCLUDED__

#define BF_NUM_SUBKEYS  18
#define BF_NUM_S_BOXES  4
#define BF_NUM_ENTRIES  256

#define BF_MAX_STRING   256
#define BF_MAX_PASSWORD 56  // 448bits

// #define BF_BIG_ENDIAN

#ifndef BF_LITTLE_ENDIAN
#define BF_LITTLE_ENDIAN
#endif

#ifdef BF_BIG_ENDIAN
#undef BF_BIG_ENDIAN
#endif


#ifdef BF_BIG_ENDIAN
struct BF_WordByte
{
    unsigned int zero:8;
    unsigned int one:8;
    unsigned int two:8;
    unsigned int three:8;
};
#endif

#ifdef BF_LITTLE_ENDIAN
struct BF_WordByte
{
    unsigned int three:8;
    unsigned int two:8;
    unsigned int one:8;
    unsigned int zero:8;
};
#endif

union BF_Word
{
    unsigned int word;
    BF_WordByte byte;
};

struct BF_DWord
{
    BF_Word word0;
    BF_Word word1;
};


class Blowfish
{
private:

    size_t KEYLEN;
    char KEY[BF_MAX_PASSWORD+1];

    unsigned int PA[BF_NUM_SUBKEYS];
    unsigned int SB[BF_NUM_S_BOXES][BF_NUM_ENTRIES];

    void GenSubKeys();
    inline void BF_En(BF_Word *,BF_Word *);
    inline void BF_De(BF_Word *,BF_Word *);

public:
    Blowfish();
    ~Blowfish();

    void Reset();
    void SetKey(const char * = NULL);
    void Encrypt(void *,unsigned int);
    void Decrypt(void *,unsigned int);

    const char* GetKey();
    int GetKeyLen();
};

void BF_SetPassword(const char* pPassword);
void BF_Encrypt(void* pData, unsigned int iDataSize);
void BF_Decrypt(void* pData, unsigned int iDataSize);

#endif // __BLOWFISH_H_INCLUDED__
