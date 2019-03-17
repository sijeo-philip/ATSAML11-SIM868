#ifndef __JSONWRITER_H__
#define __JSONWRITER_H__

#define JWRITE_STACK_DEPTH 32               //Maximum nesting depth of JSON

#define JW_COMPACT 0                        // Output string control for jwOpen()
#define JW_PRETTY  1                        // Pretty adds \n and indentation

enum jwNodeType{JW_OBJECT = 1, JW_ARRAY};

struct jwNodeStack{
    enum jwNodeType nodeType;
    int elementNo;
};

struct jWriteControl{
    char *buffer;                       // pointer to application's buffer
    unsigned int buflen;                // length of the buffer
    char *bufp;                         // current write position in the buffer
    char tempbuf[32];                    // local buffer for int/double conversions
    int error;                          // error code
    int callNo;                         // API call on which the error occurred
    struct jwNodeStack nodeStack[JWRITE_STACK_DEPTH];   // stack of array/object nodes
    int stackpos;                       //
    int isPretty;                         //1 = pretty output (inserts \n and spaces)
};

//ERROR CODES
//-----------
#define JWRITE_OK           0
#define JWRITE_BUF_FULL     1           // Output buffer full
#define JWRITE_NOT_ARRAY    2           // tried to write Array value into Object
#define JWRITE_NOT_OBJECT   3           // tried to write Object key/value into Array
#define JWRITE_STACK_FULL   4           // array/object nesting > JWRITE_STACK_DEPTH
#define JWRITE_STACK_EMPTY  5           // stack underflow error ( too many end's)
#define JWRITE_NEST_ERROR   6           // nesting error, not all objects are closed when jwClose()

//API FUNCTION
//------------

//Returns '\0'-terminated string describing the error (as returned by jwClose())
//
char *jwErrorToString(int err);

//jwOpen()
//--------
// - Initializes jWrite with the application supplied 'buffer' of length 'buflen'
// - In operation, the buffer will always contain a valid '\0' terminated string
// - jWrite will not overrun the buffer (it returns an "OUTPUT BUFFER FULL" ERROR)
// - rootType is base JSON type: JW_OBJECT or JW_ARRAY
// - isPretty controls 'prettifying' the output : JW_PRETTY and JW_COMPACT

void jwOpen(struct jWriteControl *jwc, char *buffer, unsigned int buflen, enum jwNodeType rootType, int isPretty);

//jwClose()
//---------
// - Closes the element opened by jwOpen()
// - returns error code (0 = JWRITE_OK)
// - after an error, all the following jWrite calls are skipped internally
//   so the error code is for first error detected.

int jwClose(struct jWriteControl *jwc);

//jwErrorPos()
//------------
// - if jwClose returned an error, this function returns the number of the jWrite function call
// which caused that error

int jwErrorPos(struct jWriteControl *jwc);

//Object insertion function
// - used to insert "key" : "value" pairs into an object

void jwObj_string(struct jWriteControl *jwc, char *key, char *value);
void jwObj_int(struct jWriteControl *jwc, char *key, int value);
void jwObj_double(struct jWriteControl *jwc, char *key, double value);
void jwObj_bool(struct jWriteControl *jwc, char *key, int oneOrZero);
void jwObj_null(struct jWriteControl *jwc, char *key);
void jwObj_object(struct jWriteControl *jwc, char *key);
void jwObj_array(struct jWriteControl *jwc, char *key);


void jwArr_string(struct jWriteControl *jwc, char *value);
void jwArr_int(struct jWriteControl *jwc, int value);
void jwArr_double(struct jWriteControl *jwc, double value);
void jwArr_bool(struct jWriteControl *jwc, int oneorZero);
void jwArr_null(struct jWriteControl *jwc);
void jwArr_object(struct jWriteControl *jwc);
void jwArr_array(struct jWriteControl *jwc);

int jwEnd(struct jWriteControl *jwc);
void jwObj_raw(struct jWriteControl *jwc, char *key, char *rawtext);
void jwArr_raw(struct jWriteControl *jwc, char *rawtext);
#endif // __JSONWRITER_H__
