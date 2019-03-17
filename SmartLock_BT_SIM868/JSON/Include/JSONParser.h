#ifndef __JSONPARSER_H__
#define __JSONPARSER_H__

#define MAXJSONBYTES    512
#define MAXTOKENBYTES   30
#define MAXTOKENS       32

enum jsonDataType {JOBJECT=0, JARRAY, JSTRING, JBOOLEAN, JNULL, JNUMBER}; // JPRIMITIVE Includes boolean, null and numbers
enum jsonRootType {JDATA_OBJECT = 1, JDATA_ARRAY};
enum jsonParserStatus {JSON_START=0, JSON_END};

struct jsonReadToken
{
    char keyValue[MAXTOKENBYTES];
    unsigned long hashValue;
    char jsonValue[MAXTOKENBYTES];
    enum jsonDataType readDataType;
};

struct jsonParser
{
    char* buffer;
    int currentPos;
    char *currP;
    enum  jsonParserStatus parserStatus;
    unsigned int stackPos;
    unsigned int numTokens;
    enum jsonRootType rootType;
    struct jsonReadToken jData[MAXTOKENS];
};


//ERROR CODES
//-----------
#define JDATA_OK        1
#define JDATA_NOK       2           //Not a valid JSON data
#define JDATA_TOK_ERR   3           //Not enough tokens were provided
#define JDATA_INVALID   4           //Invalid character inside JSON string
#define JDATA_INV       5           //Not a valid JSON packet


int JSONStructInit(struct jsonParser *jParser, char* buffer);
int JSONParseData(struct jsonParser *jParser);
struct jsonReadToken *SearchToken(struct jsonParser *jParser, const char *searchString);

#endif // __JSONPARSER_H__
