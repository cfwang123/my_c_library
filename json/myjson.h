#ifndef MYJSON_H
#define MYJSON_H

#include <stdint.h>
enum JSONTYPES{
	JSON_Undefined = 0,
	JSON_NULL = 1,
	JSON_Bool = 2,
	JSON_Int = 3,
	JSON_Double = 4,
	JSON_String = 5,
	JSON_ConstString = 6,
	JSON_Array = 7,
	JSON_Object = 8,
};
enum JSONFLAGS{
	JSONF_KEYISMALLOC = 1,
};

struct JSON;
typedef struct JSONArray{
	struct JSON *arr;
	uint16_t length, capacity;
} JSONArray;

typedef struct JSON {
	uint8_t type, flags;
	union{
		int ival;
		double dval;
		char *sval;
		JSONArray arrval;
	} u;
	char *key;
} JSON;

extern const JSON JSONUNDEF, JSONNULL;

const char *JSON_TypeName(uint8_t type);

void JSON_SetNULL(JSON *j);
void JSON_SetBool(JSON *j ,uint8_t v);
void JSON_SetInt(JSON *j ,int v);
void JSON_SetDouble(JSON *j ,double v);
void JSON_SetConstString(JSON *j ,const char *v);
void JSON_SetString(JSON *j ,const char *v);
void JSON_SetNewArray(JSON *j, int capacity);
void JSON_SetNewObject(JSON *j, int capacity);
void JSON_SetArray(JSON *j ,JSONArray *v);
void JSON_Set(JSON *j, JSON *v);
void JSON_Free(JSON *j);

JSON JSON_MakeInt(int v);
JSON JSON_MakeDouble(double v);
JSON JSON_MakeConstString(const char *v);

void JSON_Clear(JSON *js);
void JSON_Add(JSON *js, const char *key, uint8_t copykey, JSON *v);
void JSON_Add_CK(JSON *js, const char *key, JSON *v);
void JSON_Resize(JSON *js, int capacity);
void JSON_AddNULL(JSON *js, const char *key, uint8_t copykey);
void JSON_AddBool(JSON *js, const char *key, uint8_t copykey, uint8_t v);
void JSON_AddInt(JSON *js, const char *key, uint8_t copykey, int v);
void JSON_AddDouble(JSON *js, const char *key, uint8_t copykey, double v);
void JSON_AddConstString(JSON *js, const char *key, uint8_t copykey, const char *v);

void JSON_AddNULL_CK(JSON *js, const char *key);
void JSON_AddBool_CK(JSON *js, const char *key, uint8_t v);
void JSON_AddInt_CK(JSON *js, const char *key, int v);
void JSON_AddDouble_CK(JSON *js, const char *key, double v);
void JSON_AddConstString_CK(JSON *js, const char *key, const char *v);
void JSON_AddString(JSON *js, const char *key, uint8_t copykey, const char *v);
void JSON_AddString_CK(JSON *js, const char *key, const char *v);

#define JSON_Foreach(js,i,v) {int i,len;JSON *v; for(i=0,len=(js)->type==JSON_Array||(js)->type==JSON_Object?(js)->u.arrval.length:0,v=(js)->u.arrval.arr;i<len;i++,v++){
#define JSON_EndForeach }}

uint8_t JSON_IsNull(JSON *j);
uint8_t JSON_BoolValue(JSON *j);
int JSON_IntValue(JSON *j);
double JSON_DoubleValue(JSON *j);
char * JSON_StringValue(JSON *j);
JSON *JSON_GetI(JSON *j, int i);
JSON *JSON_GetK(JSON *j, const char *key);
JSON *JSON_GetK_NoCase(JSON *j, const char *key);
JSON *JSON_Get_VA(JSON *j, va_list va);
JSON *JSON_Get(JSON *j, ...);
uint8_t JSON_GetBool(JSON *j, ...);
int JSON_GetInt(JSON *j, ...);
double JSON_GetDouble(JSON *j, ...);
char *JSON_GetString(JSON *j, ...);

JSON JSON_Parse(char *str, uint8_t USESRCSTR);
void JSON_Print(JSON *j, uint8_t fmt, void (*out)(char *,int,void*), void *state);
void JSON_Print_Stdout(JSON *j,int fmt);
char *JSON_Print_Malloc(JSON *j,int fmt);

#endif
