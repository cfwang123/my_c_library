#include "md5.h"
#include "string.h"

/*
SMALLCODE:
	0: code size = 5572, run test speed = 1.6s
	1: code size = 4200, run test speed = 1.8s
 * */
#define CONFIG_SMALLCODE 1
 
// leftrotate function definition
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))

static void md5_calculate_once(unsigned int h[4], unsigned int w[16]);
static void to_bytes(unsigned int val, unsigned char *bytes);
static void md5_calculate_once(unsigned int h[4], unsigned int w[16]);
static const unsigned int k[64] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee ,
	0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501 ,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be ,
	0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821 ,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa ,
	0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8 ,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed ,
	0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a ,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c ,
	0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70 ,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05 ,
	0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665 ,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039 ,
	0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1 ,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1 ,
	0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

// r specifies the per-round shift amounts
static const unsigned int r[] = {
	7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
	5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
	4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
	6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

void md5_init(struct MD5STATE *m){
	m->nowdatalen = m->len = 0;
	m->h[0] = 0x67452301;
	m->h[1] = 0xefcdab89;
	m->h[2] = 0x98badcfe;
	m->h[3] = 0x10325476;
}

void md5_advance(struct MD5STATE *m, const void *sdata, int len){
	const unsigned char *data = sdata;
	int clen;
	m->len += len;
	if(m->nowdatalen){
		if(m->nowdatalen + len >= 64){
			clen = 64-m->nowdatalen;
			memcpy(m->nowdata + m->nowdatalen, data, clen);
			data += clen;
			len -= clen;
			md5_calculate_once(m->h, (unsigned int*)m->nowdata);
			m->nowdatalen = 0;
		}
	}
	while(len >= 64){
		memcpy(m->nowdata, data, 64);
		data += 64;
		len -= 64;
		md5_calculate_once(m->h, (unsigned int*)m->nowdata);
	}
	if(len){
		memcpy(m->nowdata+m->nowdatalen, data, len);
		m->nowdatalen += len;
	}
}

void md5_end(struct MD5STATE *m, void *outmd5){
	unsigned char data2[8], pad[72], padlen;
	unsigned int newlen = m->len+1;
	unsigned int i = newlen % 64, offset;
	if(m->nowdatalen <= 55) padlen = 64 - m->nowdatalen;
	else padlen = 64 + 64 - m->nowdatalen;
	pad[0] = 0x80;
	memset(pad+1,0,padlen-9);
	to_bytes(m->len*8,pad+padlen-8);
	to_bytes(m->len>>29,pad+padlen-4);
	md5_advance(m,pad,padlen);
	memcpy(outmd5, m->h, 16);
}

// void md5_end(struct MD5STATE *m, void *outmd5){
// 	unsigned char data2[8];
// 	unsigned int newlen = m->len+1;
// 	unsigned int i = newlen % 64, offset;
// 	if(i > 56) newlen += 56 + 64 - i;
// 	else if(i < 56) newlen += 56 - i;
// 	to_bytes(m->len*8,data2+0);
// 	to_bytes(m->len>>29,data2+4);
// 	for(offset=m->len - m->nowdatalen;offset<newlen;){
// 		if(offset < m->len){
// 			i=m->len-offset;
// 		}
// 		else i = 0;
// 		offset += i;
// 		for(;i<64;i++,offset++){
// 			if(offset == m->len) m->nowdata[i] = 0x80;
// 			else if(offset < newlen) m->nowdata[i] = 0;
// 			else m->nowdata[i] = data2[offset-newlen];
// 		}
// 		md5_calculate_once(m->h, (unsigned int*)m->nowdata);
// 	}
// 	memcpy(outmd5, m->h, 16);
// }

void md5(const void *data, int len, void *sdigest) {
	struct MD5STATE m;
	md5_init(&m);
	md5_advance(&m,data,len);
	md5_end(&m, sdigest);
}

static void to_bytes(unsigned int val, unsigned char *bytes) {
    bytes[0] = val;
    bytes[1] = (val >> 8);
    bytes[2] = (val >> 16);
    bytes[3] = (val >> 24);
}

#define F(x,y,z) ((x & y) | (~x & z))
#define G(x,y,z) ((x & z) | (y & ~z))
#define H(x,y,z) (x^y^z)
#define I(x,y,z) (y ^ (x | ~z))
#define ROTATE_LEFT(x,n) ((x << n) | (x >> (32-n)))
#define FF(a,b,c,d,x,s,ac) a += F(b,c,d) + x + ac;a = ROTATE_LEFT(a,s);a += b;
#define GG(a,b,c,d,x,s,ac) a += G(b,c,d) + x + ac;a = ROTATE_LEFT(a,s);a += b;
#define HH(a,b,c,d,x,s,ac) a += H(b,c,d) + x + ac;a = ROTATE_LEFT(a,s);a += b;
#define II(a,b,c,d,x,s,ac) a += I(b,c,d) + x + ac;a = ROTATE_LEFT(a,s);a += b;
static void md5_calculate_once(unsigned int h[4], unsigned int w[16]){
	unsigned int a,b,c,d,f,g,tmp,i;
	a = h[0];
	b = h[1];
	c = h[2];
	d = h[3];
#if CONFIG_SMALLCODE
	for(i = 0; i < 64; i++) {
		if (i < 16) {
			f = (b & c) | ((~b) & d);
			g = i;
		}
		else if (i < 32) {
			f = (d & b) | ((~d) & c);
			g = (5*i + 1) % 16;
		}
		else if (i < 48) {
			f = b ^ c ^ d;
			g = (3*i + 5) % 16;          
		}
		else {
			f = c ^ (b | (~d));
			g = (7*i) % 16;
		}
		tmp = d;
		d = c;
		c = b;
		b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
		a = tmp;
	}
#else
	FF(a, b, c, d, w[0], 7, 0xd76aa478); /* 1 */
	FF(d, a, b, c, w[1], 12, 0xe8c7b756); /* 2 */
	FF(c, d, a, b, w[2], 17, 0x242070db); /* 3 */
	FF(b, c, d, a, w[3], 22, 0xc1bdceee); /* 4 */
	FF(a, b, c, d, w[4], 7, 0xf57c0faf); /* 5 */
	FF(d, a, b, c, w[5], 12, 0x4787c62a); /* 6 */
	FF(c, d, a, b, w[6], 17, 0xa8304613); /* 7 */
	FF(b, c, d, a, w[7], 22, 0xfd469501); /* 8 */
	FF(a, b, c, d, w[8], 7, 0x698098d8); /* 9 */
	FF(d, a, b, c, w[9], 12, 0x8b44f7af); /* 10 */
	FF(c, d, a, b, w[10], 17, 0xffff5bb1); /* 11 */
	FF(b, c, d, a, w[11], 22, 0x895cd7be); /* 12 */
	FF(a, b, c, d, w[12], 7, 0x6b901122); /* 13 */
	FF(d, a, b, c, w[13], 12, 0xfd987193); /* 14 */
	FF(c, d, a, b, w[14], 17, 0xa679438e); /* 15 */
	FF(b, c, d, a, w[15], 22, 0x49b40821); /* 16 */

	/* Round 2 */
	GG(a, b, c, d, w[ 1], 5, 0xf61e2562); /* 17 */
	GG(d, a, b, c, w[ 6], 9, 0xc040b340); /* 18 */
	GG(c, d, a, b, w[11], 14, 0x265e5a51); /* 19 */
	GG(b, c, d, a, w[ 0], 20, 0xe9b6c7aa); /* 20 */
	GG(a, b, c, d, w[ 5], 5, 0xd62f105d); /* 21 */
	GG(d, a, b, c, w[10], 9,  0x2441453); /* 22 */
	GG(c, d, a, b, w[15], 14, 0xd8a1e681); /* 23 */
	GG(b, c, d, a, w[ 4], 20, 0xe7d3fbc8); /* 24 */
	GG(a, b, c, d, w[ 9], 5, 0x21e1cde6); /* 25 */
	GG(d, a, b, c, w[14], 9, 0xc33707d6); /* 26 */
	GG(c, d, a, b, w[ 3], 14, 0xf4d50d87); /* 27 */
	GG(b, c, d, a, w[ 8], 20, 0x455a14ed); /* 28 */
	GG(a, b, c, d, w[13], 5, 0xa9e3e905); /* 29 */
	GG(d, a, b, c, w[ 2], 9, 0xfcefa3f8); /* 30 */
	GG(c, d, a, b, w[ 7], 14, 0x676f02d9); /* 31 */
	GG(b, c, d, a, w[12], 20, 0x8d2a4c8a); /* 32 */

	/* Round 3 */
	HH(a, b, c, d, w[ 5], 4, 0xfffa3942); /* 33 */
	HH(d, a, b, c, w[ 8], 11, 0x8771f681); /* 34 */
	HH(c, d, a, b, w[11], 16, 0x6d9d6122); /* 35 */
	HH(b, c, d, a, w[14], 23, 0xfde5380c); /* 36 */
	HH(a, b, c, d, w[ 1], 4, 0xa4beea44); /* 37 */
	HH(d, a, b, c, w[ 4], 11, 0x4bdecfa9); /* 38 */
	HH(c, d, a, b, w[ 7], 16, 0xf6bb4b60); /* 39 */
	HH(b, c, d, a, w[10], 23, 0xbebfbc70); /* 40 */
	HH(a, b, c, d, w[13], 4, 0x289b7ec6); /* 41 */
	HH(d, a, b, c, w[ 0], 11, 0xeaa127fa); /* 42 */
	HH(c, d, a, b, w[ 3], 16, 0xd4ef3085); /* 43 */
	HH(b, c, d, a, w[ 6], 23,  0x4881d05); /* 44 */
	HH(a, b, c, d, w[ 9], 4, 0xd9d4d039); /* 45 */
	HH(d, a, b, c, w[12], 11, 0xe6db99e5); /* 46 */
	HH(c, d, a, b, w[15], 16, 0x1fa27cf8); /* 47 */
	HH(b, c, d, a, w[ 2], 23, 0xc4ac5665); /* 48 */

	/* Round 4 */
	II(a, b, c, d, w[ 0], 6, 0xf4292244); /* 49 */
	II(d, a, b, c, w[ 7], 10, 0x432aff97); /* 50 */
	II(c, d, a, b, w[14], 15, 0xab9423a7); /* 51 */
	II(b, c, d, a, w[ 5], 21, 0xfc93a039); /* 52 */
	II(a, b, c, d, w[12], 6, 0x655b59c3); /* 53 */
	II(d, a, b, c, w[ 3], 10, 0x8f0ccc92); /* 54 */
	II(c, d, a, b, w[10], 15, 0xffeff47d); /* 55 */
	II(b, c, d, a, w[ 1], 21, 0x85845dd1); /* 56 */
	II(a, b, c, d, w[ 8], 6, 0x6fa87e4f); /* 57 */
	II(d, a, b, c, w[15], 10, 0xfe2ce6e0); /* 58 */
	II(c, d, a, b, w[ 6], 15, 0xa3014314); /* 59 */
	II(b, c, d, a, w[13], 21, 0x4e0811a1); /* 60 */
	II(a, b, c, d, w[ 4], 6, 0xf7537e82); /* 61 */
	II(d, a, b, c, w[11], 10, 0xbd3af235); /* 62 */
	II(c, d, a, b, w[ 2], 15, 0x2ad7d2bb); /* 63 */
	II(b, c, d, a, w[ 9], 21, 0xeb86d391); /* 64 */
#endif

	h[0] += a;
	h[1] += b;
	h[2] += c;
	h[3] += d;
}
