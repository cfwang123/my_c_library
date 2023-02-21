#include "myjson.h"
#include "stdio.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#define DEBUG 0
#define debug if(DEBUG)printf

static int sprintfloat(char *b, double d, uint8_t round);
static char *parsestring(char *s, char *send, int quotemode, int USESRCSTR);
static void printjsenc(void (*out)(char *,int,void*), char *s,void *state);
static void printint(void (*out)(char *,int,void*), int v,void *state);
static void printdouble(void (*out)(char *,int,void*), double v, void *state);
static void JSON_OutStdout(char *s, int len, void *state);

#define freekey(j) if(j->key!=NULL&&j->flags&JSONF_KEYISMALLOC){debug("free key %s\n",j->key);free(j->key);j->key=NULL;}
#define CNT(x) (sizeof(x)/sizeof(x[0]))
#define STRLEN(x) x,(sizeof(x)/sizeof(x[0])-1)
const JSON JSONUNDEF = {.type = JSON_Undefined};
const JSON JSONNULL = {.type = JSON_NULL};
const JSON JSONTRUE = {.type = JSON_Bool, .u = { .ival = 1 }};
const JSON JSONFALSE = {.type = JSON_Bool, .u = { .ival = 0 }};
const JSON JSONEMPTYSTR = {.type = JSON_ConstString, .u = { .sval = "" }};

static const char *const TYPES[] = {
	"Undefined",
	"Null",
	"Bool",
	"Int",
	"Double",
	"String",
	"String",
	"Array",
	"Object",
};
const char *JSON_TypeName(uint8_t type){
	return type < CNT(TYPES) ? TYPES[type] : TYPES[JSON_Undefined];
}

void JSON_SetNULL(JSON *j){ JSON_Free(j); j->type = JSON_NULL; }
void JSON_SetBool(JSON *j ,uint8_t v){ JSON_Free(j);j->type = JSON_Bool; j->u.ival = v; }
void JSON_SetInt(JSON *j ,int v){ JSON_Free(j);j->type = JSON_Int; j->u.ival = v; }
void JSON_SetDouble(JSON *j ,double v){ JSON_Free(j);j->type = JSON_Double; j->u.dval = v; }
void JSON_SetConstString(JSON *j ,const char *v){ JSON_Free(j);j->type = JSON_ConstString; j->u.sval = (char*)v; }
void JSON_SetString(JSON *j ,const char *v){
	JSON_Free(j);
	j->type = JSON_String;
	int len = strlen(v);
	j->u.sval = malloc(len+1);
	debug("malloc for string value %s\n", v);
	memcpy(j->u.sval, v, len+1);
}
void JSON_SetArray(JSON *j ,JSONArray *v){ JSON_Free(j);j->type = JSON_Array; memcpy(&j->u.arrval,v,sizeof(j->u.arrval)); }
void JSON_SetNewArray(JSON *j, int capacity){
	JSON_Free(j);
	j->type = JSON_Array;
	memset(&j->u,0,sizeof(j->u));
	if(capacity)
		JSON_Resize(j, capacity);
}
void JSON_SetNewObject(JSON *j, int capacity){
	JSON_Free(j);
	j->type = JSON_Object;
	memset(&j->u,0,sizeof(j->u));
	if(capacity)
		JSON_Resize(j, capacity);
}
void JSON_Set(JSON *j, JSON *v){
	JSON_Free(j);
	memcpy(j,v,sizeof(*j));
}

void JSON_Free(JSON *j){
	switch(j->type){
		case JSON_Array:
		case JSON_Object:
			JSON_Foreach(j, i, v)
				JSON_Free(v);
			JSON_EndForeach;
			if(j->u.arrval.arr){
				free(j->u.arrval.arr);
			}
			break;
		case JSON_String:
			debug("free string value %s\n",j->u.sval);
			free((void*)j->u.sval);
			break;
	}
	freekey(j);
	j->type = JSON_Undefined;
}

JSON JSON_MakeInt(int v){ JSON j = {0}; JSON_SetInt(&j, v); return j; }
JSON JSON_MakeDouble(double v){ JSON j = {0}; JSON_SetDouble(&j, v); return j; }
JSON JSON_MakeConstString(const char *v){ JSON j = {0}; JSON_SetConstString(&j, v); return j; }

void JSON_Clear(JSON *js){
	if(js->type != JSON_Array && js->type != JSON_Object) return;
	JSONArray *j = &js->u.arrval;
	JSON_Foreach(js, i, v)
		JSON_Free(v);
	JSON_EndForeach;
	j->length = 0;
}

void JSON_Add(JSON *js, const char *key, uint8_t copykey, JSON *v){
	if(js->type != JSON_Array && js->type != JSON_Object) return;
	JSONArray *j = &js->u.arrval;
	if(j->capacity <= j->length){
		int newcap = j->capacity * 2;
		while(newcap <= j->length)
			newcap *= 2;
		JSON_Resize(js, newcap);
	}
	freekey(v);
	if(key && copykey){
		int len = strlen(key);
		v->key = malloc(len+1);
		memcpy(v->key,key,len+1);
		debug("malloc for key %s\n", key);
		v->flags |= JSONF_KEYISMALLOC;
	}
	else{
		v->key = (char*)key;
		v->flags &= ~JSONF_KEYISMALLOC;
	}
	memcpy(&j->arr[j->length++],v,sizeof(JSON));
}

void JSON_Add_CK(JSON *js, const char *key, JSON *v){
	JSON_Add(js, key, 0, v);
}

void JSON_Resize(JSON *js, int capacity){
	if(js->type != JSON_Array && js->type != JSON_Object) return;
	JSONArray *j = &js->u.arrval;
	JSON *newarr = malloc(sizeof(JSON)*capacity);
	if(j->arr){
		int newlen = capacity < j->length ? capacity : j->length;
		memcpy(newarr, j->arr, sizeof(JSON)*newlen);
		free(j->arr);
	}
	j->capacity = capacity;
	j->arr = newarr;
}

void JSON_AddNULL(JSON *js, const char *key, uint8_t copykey){ JSON vjs={0}; JSON_SetNULL(&vjs); JSON_Add(js, key, copykey, &vjs); }
void JSON_AddBool(JSON *js, const char *key, uint8_t copykey, uint8_t v){ JSON vjs={0}; JSON_SetBool(&vjs,v); JSON_Add(js,key, copykey, &vjs); }
void JSON_AddInt(JSON *js, const char *key, uint8_t copykey, int v){ JSON vjs={0}; JSON_SetInt(&vjs,v); JSON_Add(js, key, copykey, &vjs); }
void JSON_AddDouble(JSON *js, const char *key, uint8_t copykey, double v){ JSON vjs={0}; JSON_SetDouble(&vjs,v); JSON_Add(js, key, copykey, &vjs); }
void JSON_AddConstString(JSON *js, const char *key, uint8_t copykey, const char *v){ JSON vjs={0}; JSON_SetConstString(&vjs,v); JSON_Add(js, key, copykey, &vjs); }
void JSON_AddString(JSON *js, const char *key, uint8_t copykey, const char *v){ JSON vjs={0}; JSON_SetString(&vjs,v); JSON_Add(js, key, copykey, &vjs); }

void JSON_AddNULL_CK(JSON *js, const char *key){ JSON vjs={0}; JSON_SetNULL(&vjs); JSON_Add(js, key, 0, &vjs); }
void JSON_AddBool_CK(JSON *js, const char *key, uint8_t v){ JSON vjs={0}; JSON_SetBool(&vjs,v); JSON_Add(js,key, 0, &vjs); }
void JSON_AddInt_CK(JSON *js, const char *key, int v){ JSON vjs={0}; JSON_SetInt(&vjs,v); JSON_Add(js, key, 0, &vjs); }
void JSON_AddDouble_CK(JSON *js, const char *key, double v){ JSON vjs={0}; JSON_SetDouble(&vjs,v); JSON_Add(js, key, 0, &vjs); }
void JSON_AddConstString_CK(JSON *js, const char *key, const char *v){ JSON vjs={0}; JSON_SetConstString(&vjs,v); JSON_Add(js, key, 0, &vjs); }
void JSON_AddString_CK(JSON *js, const char *key, const char *v){ JSON vjs={0}; JSON_SetString(&vjs,v); JSON_Add(js, key, 0, &vjs); }

uint8_t JSON_IsNull(JSON *j){ return j->type == JSON_NULL || j->type == JSON_Undefined; }
uint8_t JSON_BoolValue(JSON *j){
	switch(j->type){
		case JSON_Bool:
		case JSON_Int:
			return j->u.ival;
			break;
		case JSON_Double:
			return j->u.dval != 0;
			break;
		case JSON_String:
		case JSON_ConstString:
			return j->u.sval[0] == 0 || strcmp(j->u.sval,"0") == 0;
			break;
		default: return 0;
	}
}
int JSON_IntValue(JSON *j){
	switch(j->type){
		case JSON_Bool:
		case JSON_Int:
			return j->u.ival;
			break;
		case JSON_Double:
			return j->u.dval >= INT32_MAX ? INT32_MAX : j->u.dval < INT32_MIN ? INT32_MIN : (int)j->u.dval;
			break;
		case JSON_String:
		case JSON_ConstString:
			return strtol(j->u.sval,NULL,10);
			break;
		default: return 0;
	}
}
double JSON_DoubleValue(JSON *j){
	switch(j->type){
		case JSON_Bool:
		case JSON_Int:
			return j->u.ival;
			break;
		case JSON_Double:
			return j->u.dval;
			break;
		case JSON_String:
		case JSON_ConstString:
			return strtod(j->u.sval, NULL);
			break;
		default: return 0;
	}
}
static char strbuf[30];
char * JSON_StringValue(JSON *j){
	switch(j->type){
		case JSON_Bool:
			return j->u.ival?"1":"0";
		case JSON_Int:
			sprintf(strbuf,"%d",j->u.ival);
			return strbuf;
		case JSON_Double:
			sprintfloat(strbuf,j->u.dval,7);
			return strbuf;
		case JSON_String:
		case JSON_ConstString:
			return j->u.sval;
		case JSON_Array: return "[Array]";
		case JSON_Object: return "[Object]";
		default: return "";
	}
}

JSON *JSON_GetI(JSON *j, int i){
	if(j->type != JSON_Array && j->type != JSON_Object) return (void*)&JSONUNDEF;
	if(i < j->u.arrval.length)
		return &j->u.arrval.arr[i];
	return (void*)&JSONUNDEF;
}

JSON *JSON_GetK(JSON *j, const char *key){
	if(j->type != JSON_Object) return (void*)&JSONUNDEF;
	int i,len;
	JSON *p = j->u.arrval.arr, *pend = p + j->u.arrval.length;
	for(;p<pend;p++){
		if(strcmp(p->key, key) == 0)
			return p;
	}
	return (void*)&JSONUNDEF;
}

JSON *JSON_GetK_NoCase(JSON *j, const char *key){
	if(j->type != JSON_Object) return (void*)&JSONUNDEF;
	int i,len;
	JSON *p = j->u.arrval.arr, *pend = p + j->u.arrval.length;
	for(;p<pend;p++){
		if(stricmp(p->key, key) == 0)
			return p;
	}
	return (void*)&JSONUNDEF;
}

JSON *JSON_Get_VA(JSON *j, va_list va){
	int ival;
	char *val;
	JSON *pos = j;
	val = va_arg(va, char*);
	while((int)(long long)val != -1){
		ival = (int)(long long)val;
		if(ival >= 0 && ival <= 0xffff){
			if(pos->type != JSON_Array && pos->type != JSON_Object) return (void*)&JSONUNDEF;
			if(ival < pos->u.arrval.length)
				pos = &pos->u.arrval.arr[ival];
			else return (void*)&JSONUNDEF;
		}
		else{
			if(pos->type != JSON_Object) return (void*)&JSONUNDEF;
			JSON *p = pos->u.arrval.arr, *pend = p + pos->u.arrval.length;
			while(p < pend){
				if(strcmp(p->key, val) == 0){
					pos = p;
					break;
				}
				p++;
				if(p == pend)
					return (void*)&JSONUNDEF;
			}
		}
		val = va_arg(va, char*);
	}
	return pos;
}

JSON *JSON_Get(JSON *j, ...){
	va_list va;
	va_start(va, j);
	JSON *pos = JSON_Get_VA(j, va);
	va_end(va);
	return pos;
}

uint8_t JSON_GetBool(JSON *j, ...){
	va_list va;
	va_start(va, j);
	JSON *pos = JSON_Get_VA(j,va);
	va_end(va);
	return JSON_BoolValue(pos);
}

int JSON_GetInt(JSON *j, ...){
	va_list va;
	va_start(va, j);
	JSON *pos = JSON_Get_VA(j,va);
	va_end(va);
	return JSON_IntValue(pos);
}

double JSON_GetDouble(JSON *j, ...){
	va_list va;
	va_start(va, j);
	JSON *pos = JSON_Get_VA(j,va);
	va_end(va);
	return JSON_DoubleValue(pos);
}

char *JSON_GetString(JSON *j, ...){
	va_list va;
	va_start(va, j);
	JSON *pos = JSON_Get_VA(j,va);
	va_end(va);
	return JSON_StringValue(pos);
}

const int pow10values[] = {1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000};
static int sprintfloat(char *b, double d, uint8_t round){
	int len, dotpos = -1, zeropos = -1, i;
	if(isnan(d))
		return sprintf(b,"NAN");
	if(d >= 1e20 || d <= -1e20)
		return snprintf(b,30,"%g",d);
	if(round <= sizeof(pow10values)/sizeof(pow10values[0])){
		d = rint(d * pow10values[round]) / pow10values[round];
	}
	len = snprintf(b,30,"%f",d);
	b[len] = 0;
	for(i=0;i<len;i++){
		if(b[i] == '.'){
			dotpos = i;
		}
		if(b[i] != '0')
			zeropos = i;
	}
	if(dotpos >= 0 && zeropos >= dotpos){
		if(zeropos == dotpos)
			zeropos--;
		else if(zeropos > dotpos + round)
			zeropos = dotpos + round;
		b[zeropos+1] = 0;
		len = zeropos+1;
	}
	if(b[len-1] == '.')
		b[(len--)-1] = 0;
	return len;
}

static void printint(void (*out)(char *,int,void*), int v,void *state){
	char buf[30];
	int len = sprintf(buf,"%d",v);
	out(buf,len,state);
}

static void printdouble(void (*out)(char *,int,void*), double v, void *state){
	char buf[100];
	int len = sprintfloat(buf,v,7);
	out(buf,len,state);
}

static void printjsenc(void (*out)(char *,int,void*), char *s,void *state){
	char ch;
	out("\"",1,state);
	if(s){
		while((ch = *s++) != 0){
			switch(ch){
				case '\\': out("\\\\",2,state); break;
				case '\"': out("\\\"",2,state); break;
				case '\n': out("\\n",2,state); break;
				case '\r': out("\\r",2,state); break;
				case '\t': out("\\t",2,state); break;
				case '\b': out("\\b",2,state); break;
				case '\f': out("\\f",2,state); break;
				default: out(&ch,1,state); break;
			}
		}
	}
	out("\"",1,state);
}

static char *parsestring(char *s, char *send, int quotemode, int USESRCSTR){
	char *outs, *pout;
	if(!s) return NULL;
	if(USESRCSTR) outs = pout = s;
	else{
		outs = pout = malloc(send-s+1);
		debug("malloc for %.*s\n",send-s,s);
	}
	if(quotemode == 0){
		if(!USESRCSTR){
			strncpy(outs,s,send-s);
			outs[send-s] = 0;
		}
		return outs;
	}
	for(;s<send;s++){
		switch(*s){
			case '\\':
				switch(s[1]){
					case 't': *pout++ = '\t';break;
					case 'r': *pout++ = '\r';break;
					case 'n': *pout++ = '\n';break;
					case 'b': *pout++ = '\b';break;
					case 'f': *pout++ = '\f';break;
					default: *pout++ = s[1];break;
				}
				s++;
				break;
			default:
				*pout++ = *s;
				break;
		}
	}
	*pout = 0;
	// debug("parsestring: %s -> %s\n",s,outs);
	return outs;
}

static JSON parseexp(char *s, int quotemode, uint8_t USESRCSTR){
	if(quotemode != 0){
		JSON j = {0};
		JSON_SetConstString(&j,s);
		if(!USESRCSTR){
			j.type = JSON_String;
			j.flags = JSONF_KEYISMALLOC;
		}
		return j;
	}
	if(strcmp(s,"true") == 0){
		if(!USESRCSTR){ debug("free %s\n",s);free(s); }
		return JSONTRUE;
	}
	if(strcmp(s,"false") == 0){
		if(!USESRCSTR){ debug("free %s\n",s);free(s); }
		return JSONFALSE;
	}
	if(strcmp(s,"null") == 0){
		if(!USESRCSTR){ debug("free %s\n",s);free(s); }
		return JSONNULL;
	}
	if(strcmp(s,"NaN") == 0){
		if(!USESRCSTR){ debug("free %s\n",s);free(s); }
		return JSON_MakeDouble(NAN);
	}
	if(strcmp(s,"Inf") == 0){
		if(!USESRCSTR){ debug("free %s\n",s);free(s); }
		return JSON_MakeDouble(INFINITY);
	}
	if(strcmp(s,"-Inf") == 0){
		if(!USESRCSTR){ debug("free %s\n",s);free(s); }
		return JSON_MakeDouble(-INFINITY);
	}
	// debug("exp=%s\n",s);
	errno = 0;
	double dval, dval2;
	if(sscanf(s,"%lf",&dval) == 1){
		char *end;
		dval2 = strtod(s, &end);
		if(dval == dval2 && end[0] == 0){
			if(!USESRCSTR){ debug("free %s\n",s);free(s); }
			if(dval == (double)(int)dval) return JSON_MakeInt((int)dval);
			else return JSON_MakeDouble(dval);
			// debug("parsedouble=%lf\n", dval);
		}
	}
	// debug("parsedouble fail, use string\n");
	if(USESRCSTR) return JSON_MakeConstString(s);
	else{
		JSON j = {0};
		JSON_SetConstString(&j, s);
		j.type = JSON_String;
		j.flags = JSONF_KEYISMALLOC;
		return j;
	}
}

JSON JSON_Parse(char *str, uint8_t USESRCSTR){
	JSON js = { .type = JSON_Undefined };
	if(!str) return js;
	int quotemode = 0, isobj = 0;
	int len = strlen(str);
	char *p = str, *pend = p + len, *ptoken = NULL, *ptokenend = NULL, *pkey = NULL;
	JSON stack[20] = {0}, *ctx = NULL;
	char *names[20];
	int stacklen = 0;
	for(;p<pend;p++){
		// debug("parse: %c\n",*p);
		switch(*p){
			case '/': //comment
				if(quotemode == 0 && p + 1 < pend){
					if(p[1] == '/'){
						p+=2;
						while(*p && *p != '\n')
							p++;
						break;
					}
					else if(p[1] == '*'){
						p+=2;
						while(p+1<pend){
							if(p[0]=='*' && p[1]=='/'){
								p+=2;
								break;
							}
							p++;
						}
						p--; //TODO: check
						break;
					}
				}
				if(!ptoken) ptoken = p; ptokenend = p+1;
				break;
			case '{':
				if(quotemode == 1){
					if(!ptoken) ptoken = p; ptokenend = p+1;
					break;
				}
				if(stacklen >= CNT(stack)){
					perror("json stack overflow");
					exit(0);
				}
				memset(&stack[stacklen],0,sizeof(stack[0]));
				JSON_SetNewObject(&stack[stacklen], 8);
				if(ptoken) ptoken = parsestring(ptoken, ptokenend, quotemode, USESRCSTR);
				else ptoken = pend;
				names[stacklen] = pkey;
				// debug("addstack%d: %s, %s, key=%s\n", stacklen, stack[stacklen].type==JSON_Array?"array":"object", names[stacklen], pkey);
				ptoken = pkey = NULL;
				quotemode = 0;
				ctx = &stack[stacklen++];
				// debug("ctx=%s\n",ctx->type==JSON_Array?"array":"object");
				break;
			case '[':
				if(quotemode == 1){
					if(!ptoken) ptoken = p; ptokenend = p+1;
					break;
				}
				if(stacklen >= CNT(stack)){
					perror("json stack overflow");
					exit(0);
				}
				memset(&stack[stacklen],0,sizeof(stack[0]));
				JSON_SetNewArray(&stack[stacklen], 8);
				if(ptoken) ptoken = parsestring(ptoken, ptokenend, quotemode, USESRCSTR);
				else ptoken = pend;
				names[stacklen] = pkey;
				// debug("addstack%d: %s, %s, key=%s\n", stacklen, stack[stacklen].type==JSON_Array?"array":"object", names[stacklen], pkey);
				ptoken = pkey = NULL;
				quotemode = 0;
				ctx = &stack[stacklen++];
				// debug("ctx=%s\n",ctx->type==JSON_Array?"array":"object");
				break;
			case '}':
			case ']':
				if(quotemode == 1){
					if(!ptoken) ptoken = p; ptokenend = p+1;
					break;
				}
				if(stacklen == 0)
					goto err;
				if(ptoken){
						if(ctx){
						char *pval;
						pval = parsestring(ptoken, ptokenend, quotemode, USESRCSTR);
						// debug("token=%s\n",pval);
						JSON v = parseexp(pval, quotemode, USESRCSTR);
						if(ctx->type == JSON_Array)
							JSON_Add(ctx,NULL,0,&v);
						else JSON_Add(ctx,pkey?pkey:"",!USESRCSTR,&v);
						if(!USESRCSTR){
							debug("free key %s\n",pkey);
							free(pkey);
						}
						pkey = NULL;
					}
					ptoken = NULL;
				}
				if(stacklen){
					if(stacklen >= 2){
						JSON_Add(&stack[stacklen-2],names[stacklen-1],!USESRCSTR,&stack[stacklen-1]);
						if(names[stacklen-1]){
							// debug("free stack name %s\n",names[stacklen-1]);
							free(names[stacklen-1]);
							names[stacklen-1] = NULL;
						}
						memset(&stack[stacklen],0,sizeof(stack[0]));
						if(DEBUG){
							if(stack[stacklen-2].type==JSON_Array){
								printf("stack[%d].Add(%s)\n",stacklen-2,stack[stacklen-1].type==JSON_Array?"[...]":"{...}");
							}
							else printf("stack[%d].Add(%s, %s)\n",stacklen-2, names[stacklen-2],stack[stacklen-1].type==JSON_Array?"[...]":"{...}");
						}
					}
					if(stacklen == 1)
						goto finish;
					stacklen--;
				}
				if(stacklen)
					ctx = &stack[stacklen-1];
				break;
			case ':':
				if(quotemode == 1){
					if(!ptoken) ptoken = p; ptokenend = p+1;
					break;
				}
				ptoken = parsestring(ptoken, ptokenend, quotemode, USESRCSTR);
				pkey = ptoken;
				quotemode = 0;
				ptoken = NULL;
				// debug("key=%s\n",pkey);
				break;
			case '"':
				if(quotemode == 0) quotemode = 1;
				else quotemode = 2;
				break;
			case ',':
				if(quotemode == 1){
					if(!ptoken) ptoken = p; ptokenend = p+1;
					break;
				}
				if(ctx && ptoken){
					char *pval;
					// debug("ptoken=%.*s, ",ptokenend-ptoken,ptoken);
					if(ptoken)
						pval = parsestring(ptoken, ptokenend, quotemode, USESRCSTR);
					else pval = pend;
					// debug("pval=%s\n", pval);
					JSON v = parseexp(pval, quotemode, USESRCSTR);
					// debug("exp:%s -> type=%d, ival=%d\n", pval, v.type, v.u.ival);
					if(ctx->type == JSON_Array)
						JSON_Add(ctx,NULL,0,&v);
					else JSON_Add(ctx,pkey?pkey:"",!USESRCSTR,&v);
					if(!USESRCSTR){
						debug("free key %s\n",pkey);
						free(pkey);
					}
					// debug("ctx.count=%d\n",ctx->u.arrval.length);
				}
				pkey = ptoken = NULL;
				quotemode = 0;
				break;
			case '\r':
			case '\n':
				break;
			case ' ':
			case '\t':
				if(quotemode == 1){
					if(!ptoken) ptoken = p; ptokenend = p+1;
				}
				break;
			case '\\':
				p++;
				if(!ptoken) ptoken = p; ptokenend = p+1;
				break;
			default:
				if(!ptoken) ptoken = p; ptokenend = p+1;
				break;
		}
	}
finish:
	// debug("end, stacklen=%d, ptoken=%s\n",stacklen, ptoken);
	if(!USESRCSTR && pkey){
		debug("free pkey %s\n",pkey);
		free(pkey);
	}
	if(ctx){
		if(ptoken){
			char *pval;
			ptoken = parsestring(ptoken, ptokenend, quotemode, USESRCSTR);
			// debug("token=%s\n",ptoken);
			pval = ptoken;
			ptoken = NULL;
			JSON v = parseexp(pval, quotemode, USESRCSTR);
			if(ctx->type == JSON_Array)
				JSON_Add(ctx,NULL,0,&v);
			else JSON_Add(ctx,pkey?pkey:"",!USESRCSTR,&v);
		}
		while(stacklen >= 2){
			JSON_Add(&stack[stacklen-2],names[stacklen-1],!USESRCSTR,&stack[stacklen-1]);
			memset(&stack[stacklen],0,sizeof(stack[0]));
			stacklen--;
		}
		ctx = &stack[stacklen-1];
		return *ctx;
	}
	else if(ptoken){
		ptoken = parsestring(ptoken, ptokenend, quotemode, USESRCSTR);
		return parseexp(ptoken, quotemode, USESRCSTR);
	}
	else return JSONEMPTYSTR;
err:
	perror("error\n");
	if(!USESRCSTR && pkey) free(pkey);
	while(stacklen)
		JSON_Free(&stack[--stacklen]);
	return JSONUNDEF;
}

void JSON_Print(JSON *j, uint8_t fmt, void (*out)(char *,int,void*), void *state){
	int k;
	switch(j->type){
		case JSON_NULL:
		case JSON_Undefined:
			out(STRLEN("null"),state);
			break;
		case JSON_Bool:
			if(j->u.ival) out(STRLEN("true"),state);
			else out(STRLEN("false"),state);
			break;
		case JSON_Int: printint(out, j->u.ival,state); break;
		case JSON_Double: printdouble(out, j->u.dval,state); break;
		case JSON_String:
		case JSON_ConstString:
			printjsenc(out, j->u.sval,state);
			break;
		case JSON_Array:
			if(j->u.arrval.length == 0){
				out("[]",2,state);
				break;
			}
			out("[",1,state);
			if(fmt){
				out("\r\n",2,state);
				for(k=0;k<fmt;k++)
					out("  ",2,state);
			}
			JSON_Foreach(j, i, v)
				if(i){
					out(",",1,state);
					if(fmt){
						out("\r\n",2,state);
						for(k=0;k<fmt;k++)
							out("  ",2,state);
					}
				}
				JSON_Print(v, fmt?(fmt+1):fmt, out, state);
			JSON_EndForeach;
			if(fmt){
				out("\r\n",2,state);
				for(k=0;k<fmt-1;k++)
					out("  ",2,state);
			}
			out("]",1,state);
			break;
		case JSON_Object:
			if(j->u.arrval.length == 0){
				out("{}",2,state);
				break;
			}
			out("{",1,state);
			if(fmt){
				out("\r\n",2,state);
				for(k=0;k<fmt;k++)
					out("  ",2,state);
			}
			JSON_Foreach(j, i, v)
				if(i){
					out(",",1,state);
					if(fmt){
						out("\r\n",2,state);
						for(k=0;k<fmt;k++)
							out("  ",2,state);
					}
				}
				printjsenc(out, v->key,state);
				out(":",1,state);
				JSON_Print(v, fmt?(fmt+1):fmt, out,state);
			JSON_EndForeach;
			if(fmt){
				out("\r\n",2,state);
				for(k=0;k<fmt-1;k++)
					out("  ",2,state);
			}
			out("}",1,state);
			break;
	}
}

struct OutMalloc{
	char *buf;
	int len, capacity;
};
static void JSON_OutMalloc(char *s, int len, void *state){
	struct OutMalloc *p = state;
	if(p->len + len + 1 > p->capacity){
		int newcap = p->capacity*2;
		while(p->len+len+1 > newcap)
			newcap *= 2;
		realloc(p->buf, newcap);
	}
	memcpy(p->buf+p->len,s,len);
	p->len += len;
}

char *JSON_Print_Malloc(JSON *j,int fmt){
	struct OutMalloc p = {0};
	p.buf = malloc(100);
	p.len = 0;
	p.capacity = 100;
	JSON_Print(j,fmt,JSON_OutMalloc,&p);
	p.buf[p.len] = 0;
	return p.buf;
}

void JSON_Print_Stdout(JSON *j,int fmt){
	JSON_Print(j,fmt,JSON_OutStdout,NULL);
}

static void JSON_OutStdout(char *s, int len, void *state){
	fwrite(s, 1, len, stdout);
}

