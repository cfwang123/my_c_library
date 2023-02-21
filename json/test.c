#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdint.h>
#include "myjson.h"
#include "string.h"

void test_parse(void){
	// char jsonstr[] = "[[1,2,\"abc\",4,5],[1,2],{a:1},[3,1],[1,1],{b:2}";
	// char jsonstr[] = "[[1,2,\"abc\",4,5],[1,2],{a:1},[3,1],[1,1],{b:2},123ajksdjflksfjlaksdjflksadfja;lkfjlkewafjalk;wfjwe;lkafjwa;lkejwl]";
	char jsonstr[] = "[1,2,3,aslfjsdakfj]";
	JSON js = JSON_Parse(jsonstr, 1);
	printf("after json=%s, js.type=%d\n",jsonstr, js.type);
	JSON_Print_Stdout(&js, 1);
	printf("\n");
	// char *outjs = JSON_Print_Malloc(&js, 0);
	// printf("print=%s\n",outjs);
	// free(outjs);
	// printf("js[0][2]=%s\n",JSON_GetString(&js,0,2,-1));
	// printf("js[1][1]=%d\n",JSON_GetInt(&js,1,1,-1));
	// printf("js[2].a=%d\n",JSON_GetInt(&js,2,"a",-1));
	// printf("js[2].b=%d\n",JSON_GetInt(&js,2,"b",-1));
	// printf("js[6].c[0][0]=%d\n",JSON_GetInt(&js,6,"c",0,0,-1));
	JSON_Free(&js,1);
}

void test_tostring(void){
	int i;
	JSON js = {0};
	// printf("123\n");
	JSON_SetNewObject(&js, 8);
	const char *const names[] = {"name1","name2","a","b","c","d","e","f"};
	for(i=0;i<7;i++){
		JSON_AddInt(&js, names[i], 1, i);
	}
	JSON_AddString(&js, "malloc_string", 1, "asdfjfk");
	JSON subarr = {0};
	JSON_SetNewArray(&subarr, 8);
	JSON_AddInt_CK(&subarr, NULL, 123);
	JSON_AddInt_CK(&subarr, NULL, 125);
	JSON_AddInt_CK(&subarr, NULL, 127);
	JSON_Add_CK(&js, "array", &subarr);
	memset(&subarr,0,sizeof(subarr));
	JSON_SetNewArray(&subarr, 8);
	JSON_AddInt_CK(&subarr, NULL, 123);
	JSON_AddInt_CK(&subarr, NULL, 125);
	JSON_AddInt_CK(&subarr, NULL, 127);
	JSON_Add_CK(&js, "array2", &subarr);
	JSON_Print_Stdout(&js, 1);
	JSON_Free(&js,1);
}

int main(int argc, char **argv){
	int n = 0;
	int i;
	uint8_t DOLOOP = argc >= 2 && strcmp(argv[1],"loop") == 0;
	while(1){
		test_parse();
		// test_tostring();
		if(!DOLOOP)
			break;
		n++;
		if(n%10000 == 0){
			printf("loop %d\n",n);
		}
	}
}
