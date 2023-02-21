#include "md5.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "assert.h"
#include "time.h"

static int hex2bin(void *sdest, const void *ssrc, int len);

void testmd5(const char *input, const char *expectoutput_hex){
	unsigned char exp[16], md5value[16];
	hex2bin(exp, expectoutput_hex, 32);
	//test single call
	md5(input, strlen(input), md5value);
	assert(memcmp(md5value, exp, 16) == 0);
	//test stream call
	{
		struct MD5STATE s;
		int i, len;
		memset(md5value,0,16);
		md5_init(&s);
		for(i=0,len=strlen(input);i<len;i++){
			md5_advance(&s,&input[i],1);
		}
		md5_end(&s, md5value);
		assert(memcmp(md5value, exp, 16) == 0);
	}
	//printf("%s -> ok\n", input);
}

void main(void){
	clock_t t0 = clock(), used;
	int i;
	for(i=0;i<100000;i++){
		testmd5("", "d41d8cd98f00b204e9800998ecf8427e");
		testmd5("123456", "e10adc3949ba59abbe56e057f20f883e");
		testmd5("111111", "96e79218965eb72c92a549dd5a330112");
		testmd5("1111111111111111111111111111111111111111111111111111111", "1e817ad27a934e9db7239bc92a4e0932");
		testmd5("11111111111111111111111111111111111111111111111111111111", "16e54833b88a6cc06966d83d05c271b3");
		testmd5("111111111111111111111111111111111111111111111111111111111", "e48b2b937fbe05271407ba712a87acdc");
		testmd5("1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890", "f1257a8659eb92d36fe14c6bf3852a6a");
	}
	used = clock() - t0;
	printf("end in %d(%fs)\n", (int)used, used/(float)CLOCKS_PER_SEC);
}

int hex2bin(void *sdest, const void *ssrc, int len){
	unsigned char *dest = sdest;
	const unsigned char *src = ssrc;
	int i;
	if(len < 0)
		len = strlen(ssrc)&~1;
	if(len > (strlen(ssrc)&~1))
		len = strlen(ssrc)&~1;
	for(i=0;i<len;i+=2){
		unsigned char a, b = 0, c = 0;
		a = src[i];
		if(a >= '0' && a <= '9')
			b = a-'0';
		else if(a >= 'a' && a <= 'f')
			b = a-'a'+10;
		else if(a >= 'A' && a <= 'F')
			b = a-'A'+10;
		else break;
		a = src[i+1];
		if(a >= '0' && a <= '9')
			c = a-'0';
		else if(a >= 'a' && a <= 'f')
			c = a-'a'+10;
		else if(a >= 'A' && a <= 'F')
			c = a-'A'+10;
		else break;
		dest[i/2] = (b<<4)|c;
		if(!src[i] || !src[i+1])
			break;
	}
	return i/2;
}
