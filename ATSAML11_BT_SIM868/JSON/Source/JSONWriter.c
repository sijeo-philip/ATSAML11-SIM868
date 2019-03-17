#include <stddef.h>
#include <stdio.h>
#include <string.h>  //memset()
#include <stdint.h>

#include "JSONWriter.h"

// typedef unsigned int uint32_t;
// typedef int int32_t;

#define JWC_DECL \
    struct jWriteControl *jwc,  // function parameter is ptr to control struct
#define JWC_DECL0 struct jWriteControl *jwc  // function parameter, no params
#define JWC(x) jwc->x			     // functions use pointer
#define JWC_PARAM jwc,			     // pointer to struct
#define JWC_PARAM0 jwc			     // pointer to struct, no params

// Internal Functions
//-------------------

void jwPutch(JWC_DECL char c);
void jwPutstr(JWC_DECL char *str);
void jwPutraw(JWC_DECL char *str);
void modp_itoa10(int32_t value, char *str);
void modp_dtoa2(double value, char *str, int prec);
void jwPretty(JWC_DECL0);
enum jwNodeType jwPop(JWC_DECL0);
void jwPush(JWC_DECL enum jwNodeType nodeType);

// jwOpen()
//---------
// - Open writing of JSON starting with rootType = JW_OBJECT
// - Initialize with user string buffer of length buflen
// - isPretty = JW_PRETTY adds \n and spaces to prettify output (else
// JW_COMPACT)
//
void jwOpen(JWC_DECL char *buffer, unsigned int buflen,
	    enum jwNodeType rootType, int isPretty)
{
    memset(buffer, 0, buflen);  // zap the whole destination buffer
    JWC(buffer) = buffer;
    JWC(buflen) = buflen;
    JWC(bufp) = buffer;
    if (rootType == JW_ARRAY)
	{
	    // fprintf(stderr, "Wrong root Type, cannot open");
	    return;
	}
    else
	{
	    JWC(nodeStack)[0].nodeType = rootType;
	    JWC(nodeStack)[0].elementNo = 0;
	    JWC(stackpos) = 0;
	    JWC(error) = JWRITE_OK;
	    JWC(callNo) = 1;
	    JWC(isPretty) = isPretty;
	    jwPutch(JWC_PARAM '{');
	}
}

// jwClose()
//-----------
// - Closes the root JSON object started by jwOpen()
// - returns error code
//

int jwClose(JWC_DECL0)
{
    if (JWC(error) == JWRITE_OK)
	{
	    if (JWC(stackpos) == 0)
		{
		    enum jwNodeType node = JWC(nodeStack)[0].nodeType;
		    if (JWC(isPretty)) jwPutch(JWC_PARAM '\n');
		    jwPutch(JWC_PARAM '}');
		}
	    else
		{
		    JWC(error) =
			JWRITE_NEST_ERROR;  // Nesting error, not all objects
					    // closed when jwClose() called
		}
	}
    return JWC(error);
}

// jwEnd()
//---------
// - End the current object

int jwEnd(JWC_DECL0)
{
    if (JWC(error) == JWRITE_OK)
	{
	    enum jwNodeType node;
	    int lastElemNo = JWC(nodeStack)[JWC(stackpos)].elementNo;
	    node = jwPop(JWC_PARAM0);
	    if (lastElemNo > 0) jwPretty(JWC_PARAM0);
	    jwPutch(JWC_PARAM(node == JW_OBJECT) ? '}' : ']');
	}
    return JWC(error);
}

// jwErrorPos
//----------
//- Returns position of error: the nth call to a jWrite Function
//

int jwErrorPos(JWC_DECL0) { return JWC(callNo); }

int _jwObj(JWC_DECL char *key);

// jwObjraw()
//----------
//- put raw string to object (i.e. contents of raw text without quotes)
//
void jwObj_raw(JWC_DECL char *key, char *value)
{
    if (_jwObj(JWC_PARAM key) == JWRITE_OK) jwPutraw(JWC_PARAM value);
}

// jwObj_string()
//--------------
// - put "quoted" string to object
//
void jwObj_string(JWC_DECL char *key, char *value)
{
    if (_jwObj(JWC_PARAM key) == JWRITE_OK) jwPutstr(JWC_PARAM value);
}

// jwObj_int()
//-----------
//- put integer value to the object
//

void jwObj_int(JWC_DECL char *key, int value)
{
    modp_itoa10(value, JWC(tempbuf));
    jwObj_raw(JWC_PARAM key, JWC(tempbuf));
}

// jwObj_double()
//--------------
//- put double value to the object
//

void jwObj_double(JWC_DECL char *key, double value)
{
    modp_dtoa2(value, JWC(tempbuf), 6);
    jwObj_raw(JWC_PARAM key, JWC(tempbuf));
}

// jwObj_bool()
//------------
//- put true/false value to the object
//

void jwObj_bool(JWC_DECL char *key, int oneOrZero)
{
    jwObj_raw(JWC_PARAM key, (oneOrZero) ? "true" : "false");
}

// jwObj_null()
//------------
//- put NULL value to the object
//

void jwObj_null(JWC_DECL char *key) { jwObj_raw(JWC_PARAM key, "null"); }

// jwObj_object()
//--------------
// - put object in Object
//

void jwObj_object(JWC_DECL char *key)
{
    if (_jwObj(JWC_PARAM key) == JWRITE_OK)
	{
	    jwPutch(JWC_PARAM '{');
	    jwPush(JWC_PARAM JW_OBJECT);
	}
}

// jwObj_array()
//-------------
//- Put Array in object
//
void jwObj_array(JWC_DECL char *key)
{
    if (_jwObj(JWC_PARAM key) == JWRITE_OK)
	{
	    jwPutch(JWC_PARAM '[');
	    jwPush(JWC_PARAM JW_ARRAY);
	}
}

// ARRAY INSERT FUNCTIONS
//----------------------
//
int _jwArr(JWC_DECL0);

// jwArr_raw()
//-----------
//- put raw string to array (i.e. contents of raw text without quotes)
//
void jwArr_raw(JWC_DECL char *rawtext)
{
    if (_jwArr(JWC_PARAM0) == JWRITE_OK) jwPutraw(JWC_PARAM rawtext);
}

// jwArr_string()
//-------------
//- put "quoted" string to array
//
void jwArr_string(JWC_DECL char *value)
{
    if (_jwArr(JWC_PARAM0) == JWRITE_OK) jwPutstr(JWC_PARAM value);
}

// jwArr_int()
//-----------
//- inserts integer to the array
//
void jwArr_int(JWC_DECL int value)
{
    modp_itoa10(value, JWC(tempbuf));
    jwArr_raw(JWC_PARAM JWC(tempbuf));
}

// jwArr_double()
//--------------
//- insert integer to the array
//
void jwArr_double(JWC_DECL double value)
{
    modp_dtoa2(value, JWC(tempbuf), 6);
    jwArr_raw(JWC_PARAM JWC(tempbuf));
}

// jwArr_bool()
//------------
//- insert bool value into the array
//
void jwArr_bool(JWC_DECL int oneorZero)
{
    jwArr_raw(JWC_PARAM(oneorZero) ? "true" : "false");
}

// jwArr_null()
//------------
//- insert null value into the array
//
void jwArr_null(JWC_DECL0) { jwArr_raw(JWC_PARAM "null"); }

// jwArr_object()
//--------------
//- insert object inside an array
//
void jwArr_object(JWC_DECL0)
{
    if (_jwArr(JWC_PARAM0) == JWRITE_OK)
	{
	    jwPutch(JWC_PARAM '{');
	    jwPush(JWC_PARAM JW_OBJECT);
	}
}

// jwArr_array()
//-------------
//- insert array inside and Array
//
void jwArr_array(JWC_DECL0)
{
    if (_jwArr(JWC_PARAM0) == JWRITE_OK)
	{
	    jwPutch(JWC_PARAM '[');
	    jwPush(JWC_PARAM JW_ARRAY);
	}
}

// jwErrorToString()
//-----------------
//-  returns string to describe error code
//
char *jwErrorToString(int err)
{
    switch (err)
	{
	    case JWRITE_OK:
		return "OK";
	    case JWRITE_BUF_FULL:
		return "OUTPUT BUFFER FULL";
	    case JWRITE_NOT_ARRAY:
		return "TRIED TO WRITE ARRAY VALUE INTO OBJECT";
	    case JWRITE_NOT_OBJECT:
		return "TRIED TO WRITE OBJECT KEY/VALUE INTO ARRAY";
	    case JWRITE_STACK_FULL:
		return "ARRAY/OBJECT NESTING > JWRITE_STACK_DEPTH";
	    case JWRITE_STACK_EMPTY:
		return "STACK UNDERFLOW ERROR (TOO MANY END'S)";
	    case JWRITE_NEST_ERROR:
		return "NESTING ERROR, NOT ALL OBJECTS CLOSED WHEN jwClose() "
		       "CALLED";
	}
    return "Unknown Error";
}

// INTERNAL FUNCTIONS
//------------------
//

// jwPretty()
//---------
//-  this introduces indentation to the JSON data
//
void jwPretty(JWC_DECL0)
{
    int i;
    if (JWC(isPretty))
	{
	    jwPutch(JWC_PARAM '\n');
	    for (i = 0; i < JWC(stackpos) + 1; i++) jwPutraw(JWC_PARAM "    ");
	}
}

// jwPush()
//--------
//- Push new object / array to the stack
//

void jwPush(JWC_DECL enum jwNodeType nodeType)
{
    if ((JWC(stackpos) + 1) >= JWRITE_STACK_DEPTH)
	JWC(error) = JWRITE_STACK_FULL;
    else
	{
	    JWC(nodeStack[++JWC(stackpos)]).nodeType = nodeType;
	    JWC(nodeStack[JWC(stackpos)]).elementNo = 0;
	}
}

// jwPop()
// - Pops the latest node from the stack
//

enum jwNodeType jwPop(JWC_DECL0)
{
    enum jwNodeType retval = JWC(nodeStack[JWC(stackpos)]).nodeType;
    if (JWC(stackpos) == 0)
	JWC(error) = JWRITE_STACK_EMPTY;
    else
	JWC(stackpos)--;
    return retval;
}

// jwPutch()
//---------
//- Insert character to the object
//
void jwPutch(JWC_DECL char c)
{
    if ((unsigned int)(JWC(bufp) - JWC(buffer)) >= JWC(buflen))
	{
	    JWC(error) = JWRITE_BUF_FULL;
	}
    else
	{
	    *JWC(bufp)++ = c;
	}
}

// jwPutstr()
//----------
//- Put string enclosed in quotes
//
void jwPutstr(JWC_DECL char *str)
{
    jwPutch(JWC_PARAM '\"');
    while (*str != '\0')
	{
	    jwPutch(JWC_PARAM * str++);
	}
    jwPutch(JWC_PARAM '\"');
}

// jwPutraw()
//----------
//- put raw string in the data
//
void jwPutraw(JWC_DECL char *str)
{
    while (*str != '\0') jwPutch(JWC_PARAM * str++);
}

//_jwObj()
//--------
//- Common object function
//- checks current node is OBJECT
//- adds comma if required
//- adds "key":
//
int _jwObj(JWC_DECL char *key)
{
    if (JWC(error) == JWRITE_OK)
	{
	    JWC(callNo)++;
	    if (JWC(nodeStack)[JWC(stackpos)].nodeType != JW_OBJECT)
		JWC(error) = JWRITE_NOT_OBJECT;
	    else if (JWC(nodeStack)[JWC(stackpos)].elementNo++ > 0)
		jwPutch(JWC_PARAM ',');
	    jwPretty(JWC_PARAM0);
	    jwPutstr(JWC_PARAM key);
	    jwPutch(JWC_PARAM ':');
	    if (JWC(isPretty)) jwPutch(JWC_PARAM ' ');
	}
    return JWC(error);
}

//_jwArr()
//--------
//- Common Array Function
//- checks error for Array type
//- checks current node is array
//- adds comma if required
//

int _jwArr(JWC_DECL0)
{
    if (JWC(error) == JWRITE_OK)
	{
	    JWC(callNo)++;
	    if (JWC(nodeStack)[JWC(stackpos)].nodeType != JW_ARRAY)
		JWC(error) = JWRITE_NOT_ARRAY;
	    else if (JWC(nodeStack)[JWC(stackpos)].elementNo++ > 0)
		jwPutch(JWC_PARAM ',');
	    jwPretty(JWC_PARAM0);
	}
    return JWC(error);
}

// MODP Value to String Functions
//------------------------------
//
static void strreverse(char *begin, char *end)
{
    char aux;
    while (end > begin)
	{
	    aux = *end;
	    *end-- = *begin;
	    *begin++ = aux;
	}
}

// modp_itoa10()
//- converts integer values to string
//
void modp_itoa10(int32_t value, char *str)
{
    char *wstr = str;
    // Take care of the sign
    unsigned int uvalue = (value < 0) ? -value : value;
    // Conversion , number is reversed
    do
	*wstr++ = (char)(48 + (uvalue % 10));
    while (uvalue /= 10);
    if (value < 0) *wstr++ = '-';
    *wstr = '\0';

    // Reverse string
    strreverse(str, wstr - 1);
}

/*
 * Powers of 10
 * 10^0 to 10^9
 */
static const double pow10[] = {
    1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

// modp_dtoa2()
//------------
// - convert a floating point number to char-buffer with a variable precision
// format
// - No trailing zeros
//

void modp_dtoa2(double value, char *str, int prec)
{
    // if the input is larger than thres_max, revert to exponential
    const double thres_max = (double)(0x7FFFFFFF);
    int count;
    double diff = 0.0;
    char *wstr = str;
    int neg = 0;
    int whole;
    double tmp;
    uint32_t frac;

    // Hacky test for NaN
    if (!(value == value))
	{
	    str[0] = 'n';
	    str[1] = 'a';
	    str[2] = 'n';
	    str[3] = '\0';
	    return;
	}

    if (prec < 0)
	{
	    prec = 0;
	}
    else if (prec > 9)
	{
	    // precision of >=10 can lead to overflow errors
	    prec = 9;
	}

    // We will work in positive values and deal with the negative sign issue
    // later
    if (value < 0)
	{
	    neg = 1;
	    value = -value;
	}

    whole = (int)value;
    tmp = (value - whole) * pow10[prec];
    frac = (uint32_t)(tmp);
    diff = tmp - frac;

    if (diff > 0.5)
	{
	    ++frac;
	    // Handle rollover , e.g. case 0.99 with prec 1 is 1.0
	    if (frac >= pow10[prec])
		{
		    frac = 0;
		    ++whole;
		}
	}
    else if (diff == 0.5 && ((frac == 0) || (frac & 1)))
	{
	    ++frac;  // if halfway, round up if odd, OR if last digit is 0
		     // That last part is strange
	}

    // - For very large numbers switch back to native sprintf for exponentials
    //- Anyone who wants to write code for this can do so
    //- Normal printf behavior is to print every whole number digit which can be
    // 100s of
    //- characters overflowing your buffers == bad

    if (value > thres_max)
	{
	    // sprintf(str, "%e", neg ? -value : value);
	    return;
	}

    if (prec == 0)
	{
	    diff = value - whole;
	    if (diff > 0.5)
		{
		    //- greater than 0.5, round up
		    ++whole;
		}
	    else if ((diff == 0.5) && (whole & 1))
		{
		    // exactly 0.5 and ODD then round up
		    ++whole;
		}
	}
    else if (frac)
	{
	    count = prec;
	    // now do the fractional part, as an unsigned number
	    // we know it is not 0 but we can have leading zeros, these
	    // should be removed
	    while (!(frac % 10))
		{
		    --count;
		    frac /= 10;
		}

	    // - Now do the fractional part, as an unsigned number
	    do
		{
		    --count;
		    *wstr++ = (char)(48 + (frac % 10));
		}
	    while (frac /= 10);

	    // add extra 0s
	    while (count-- > 0) *wstr++ = '0';
	    //- add decimal
	    *wstr++ = '.';
	}

    //- do whole part
    //- Take care of the sign
    //- Conversion, Number is reversed
    //
    do
	*wstr++ = (char)(48 + (whole % 10));
    while (whole /= 10);
    if (neg)
	{
	    *wstr++ = '-';
	}
    *wstr = '\0';
    // printf(wstr);
    strreverse(str, wstr - 1);
}

//=======================================================================================================
