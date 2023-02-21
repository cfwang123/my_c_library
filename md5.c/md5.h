#ifndef MD5_H
#define MD5_H

#define SMALLCODE 0

struct MD5STATE{
	unsigned char nowdata[64], nowdatalen;
	unsigned int h[4], len;
};

//calculate md5 digest for *data buffer, len is buffer's length, output 16 bytes to *sdigest
void md5(const void *data, int len, void *sdigest);
//methods for calculating md5 on a input stream 
void md5_init(struct MD5STATE *m);
void md5_advance(struct MD5STATE *m, const void *sdata, int len);
void md5_end(struct MD5STATE *m, void *outmd5);

#endif
