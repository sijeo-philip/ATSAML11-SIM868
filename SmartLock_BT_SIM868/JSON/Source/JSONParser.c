#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "JSONParser.h"

// HashCal
//-------

//- Calculates Hash Value and returns Unsigned long type Hash Value
//- Which can further help us search the data from the Memory

static unsigned long HashCal(char *str)  // This is a local function
{
  unsigned long hash = 5381;
  int c;
  while (c = *str++) hash = ((hash << 5) + hash) + c;
  return hash;
}

static int strcopy(char *buffer, char *start, char end, const int maxLen,
		   int length)
{
  int charCount = 0;
  if (length == 0)
    {
      do
	{
	  *buffer = *start;
	  *start++;
	  *buffer++;
	  charCount++;
	  if (charCount >= maxLen) return 0;
	}
      while (*start != end);
    }
  else
    {
      while (length != 0)
	{
	  *buffer = *start;
	  *start++;
	  *buffer++;
	  length--;
	}
      return 1;
    }
  *buffer = *start;
  *buffer++;
  *buffer = '\0';
  return 1;
}

// JSONStructInit()
//----------------
//- Initializes the JSON Structure for parsing
//- returns error code

int JSONStructInit(struct jsonParser *jParser, char *buffer)
{
  int charCount = 0;
  int err = JDATA_OK;
  char *start = NULL;

  start = buffer;

  while ((*start != '{') && (*start != '['))
    {
      *start++;
      charCount++;
      if (charCount == MAXJSONBYTES)
	{
	  charCount = 0;
	  err = JDATA_NOK;
	  return err;
	}
    }
  jParser->buffer = start;
  jParser->currP = start;
  jParser->numTokens = 0;
  jParser->currentPos = 0;
  jParser->stackPos = 0;
  if (*start == '{')
    jParser->rootType = JDATA_OBJECT;
  else if (*start == '[')
    jParser->rootType = JDATA_ARRAY;
  jParser->parserStatus = JSON_START;
  return (err);
}

// JSONDataParser()
//----------------
//- Takes the jsonParser and jsonReadData as input to parse the JSON data
//- returns error

int JSONParseData(struct jsonParser *jParser)
{
  char *start;
  char *end;
  char *tempStr;
  int charCount = 0;
  int charFound = 0;
  char c;
  int err = JDATA_OK;
  int currentPos = 0;
  int success = 0;

  tempStr = jParser->currP;

  if (jParser->rootType == JDATA_OBJECT)
    {
      while (*++tempStr != '\"')
	{
	  charCount++;
	  if (charCount >= MAXTOKENBYTES)
	    {
	      charCount = 0;
	      err = JDATA_NOK;
	      // exit(1);
	    }
	}  // Find the first \" after the '{' to find the start of json
	   // KEY
      start = tempStr;  // start pointer is made equal to the address of
			// first \"

      while (*++tempStr != ':')
	{
	  charCount++;
	  if (charCount >= MAXTOKENBYTES)
	    {
	      charCount = 0;
	      err = JDATA_NOK;
	      // exit(1);
	    }
	}	     // Find the address of the ":" after the json key
      end = tempStr;  // end pointer is made equal to the ":" before the
		      // value of the json key
      currentPos = jParser->numTokens;
      success =
	  strcopy(jParser->jData[currentPos].keyValue, start, '\"', 128,
		  0);  // the value is stored in key value buffer of struct
      if (success != 1)
	{
	  // exit(0);
	}
      else
	jParser->jData[currentPos].hashValue =
	    HashCal(jParser->jData[currentPos].keyValue);
      // on successful storage into the struct the hash value
      // is calculated and stored in the respective struct
      start = end;  // Address of the start pointer is made equal to the
		    // end pointer.
      c = *++end;
      charCount = 0;
      while (c)
	{
	  charFound = 0;
	  switch (c)
	    {
	      case '[':
		jParser->jData[currentPos].readDataType =
		    JARRAY;	   // if the '[' is encountered the
				      // DataType is marked as ARRAY type
				      // and
		jParser->stackPos++;  // stackPos variable in the
				      // structure is incremented
		charFound = 1;	// charFound variable is set as
				// 1
		break;
	      case '{':
		jParser->jData[currentPos].readDataType =
		    JOBJECT;	  // if the '{' is encountered the
				      // DataType is marked as OBJECT
				      // type and
		jParser->stackPos++;  // stackPos variable in
				      // the buffer structure is
				      // incremented
		charFound = 1;	// charFound variable is set as
				// 1
		break;
	      case '\"':
		jParser->jData[currentPos].readDataType =
		    JSTRING;    // if the '\"' is encountered the
				// DataType is marked as STRING
				// type and
		charFound = 1;  // charFound variable is set to 1
		break;
	      case 'n':
	      case 'N':  // if 'n' or 'N' is encountered the
			 // DataType is marked as NULL type and
		jParser->jData[currentPos].readDataType =
		    JNULL;  // charFound variable is set to 1
		charFound = 1;
		break;
	      case 't':
	      case 'T':
	      case 'f':
	      case 'F':  // if 't' or 'T' or 'f' or 'F' is
			 // encountered the DataType  is marked as
		jParser->jData[currentPos].readDataType =
		    JBOOLEAN;  // BOOLEAN and charFound variable
			       // is set to 1
		charFound = 1;
		break;
	      case '-':
	      case '.':
	      case '0':
	      case '1':
	      case '2':
	      case '3':
	      case '4':
	      case '5':
	      case '6':
	      case '7':
	      case '8':
	      case '9':  // if any of the numbers or negative sign
			 // or decimal point is
		jParser->jData[currentPos].readDataType =
		    JNUMBER;    // encountered the DataType is
				// marked as NUMBER variable and
		charFound = 1;  // charFound variable is set to 1
		break;

	      default:
		err = JDATA_TOK_ERR;
		break;
	    }
	  charCount++;
	  if (charFound)
	    {
	      charFound = 0;
	      break;
	    }
	  else if (charCount >= MAXTOKENBYTES)
	    {
	      charFound = 0;
	      return err;
	    }
	  c = *++end;  // Increment the end pointer in the buffer
	}	      // End of While loop to find the start of json Value
      start = end;     // The address of the start Variable is made equal to
		    // the address of the end pointer
      c = *++end;
      charCount = 0;
      while ((jParser->stackPos > 0) ||
	     ((c != ',') && (c != '}') && (c != ']')))
	{
	  switch (c)
	    {
	      case '{':
		jParser->stackPos++;
		// printf("hit a open braces\n");
		break;
	      case '[':
		jParser->stackPos++;
		// printf("hit a open square bracket\n");
		break;
	      case '}':
		if (jParser->stackPos > 0) jParser->stackPos--;
		// printf("hit a closed braces\n");
		break;
	      case ']':
		if (jParser->stackPos > 0) jParser->stackPos--;
		// printf("Hit a closed square brackets\n");
		break;
	      default:
		break;

	    }  // the end pointer is incremented until the stackPos
	       // is 0 and ',' or '}' is found
	  charCount++;
	  if (charCount >= MAXJSONBYTES) return JDATA_TOK_ERR;

	  c = *++end;
	}  // End of While loop to find the first ',' or '}' or ']'
	   // after value of the json object
      success = strcopy(
	  jParser->jData[currentPos].jsonValue, start, *end, 256,
	  charCount + 1);  // the value stored from start pointer to end pointer
      jParser->currentPos = end - jParser->buffer;  // is copied to the json
						    // struct jsonValue variable
      jParser->currP =
	  jParser->buffer +
	  jParser->currentPos;  // the current position pointer is updated
      jParser->numTokens++;     // the number of tokens parsed are incremented
      charCount = 0;
      if (jParser->stackPos == 0)
	{
	  if (*end == '}')
	    {
	      jParser->parserStatus = JSON_END;  // if '}' is encountered and
						 // the stackPos is 0
	      return JDATA_OK;  // the parser status is updated to
				// JSON_END
	    }			// and JDATA_OK is returned
	  else if (*end == ',')
	    return JDATA_OK;  // else if stackPos is 0 and ',' is
			      // encountered
	  else if (*end == ']')    // the parserStatus is unchanged from
				   // JSON_START
	    return JDATA_INVALID;  // JDATA_OK is returned.
	}
      // printf("Object Parsing\n");
    }
  else if (jParser->rootType == JDATA_ARRAY)
    {
      start = tempStr;
      end = tempStr;
      charFound = 0;
      charCount = 0;
      err = JDATA_OK;
      currentPos = jParser->numTokens;
      c = *++start;
      while (c)
	{
	  switch (c)
	    {
	      case '{':
		jParser->jData[currentPos].readDataType = JOBJECT;
		jParser->stackPos++;
		charFound = 1;
		break;
	      case '[':
		jParser->jData[currentPos].readDataType = JARRAY;
		jParser->stackPos++;
		charFound = 1;
		break;
	      case 'n':
	      case 'N':
		jParser->jData[currentPos].readDataType = JNULL;
		charFound = 1;
		break;
	      case 't':
	      case 'T':
	      case 'f':
	      case 'F':
		jParser->jData[currentPos].readDataType = JBOOLEAN;
		charFound = 1;
		break;
	      case '\"':
		jParser->jData[currentPos].readDataType = JSTRING;
		charFound = 1;
		break;
	      case '-':
	      case '.':
	      case '1':
	      case '2':
	      case '3':
	      case '4':
	      case '5':
	      case '6':
	      case '7':
	      case '8':
	      case '9':
		jParser->jData[currentPos].readDataType = JNUMBER;
		charFound = 1;
	      default:
		err = JDATA_TOK_ERR;
		break;
	    }
	  charCount++;
	  if (charFound)
	    {
	      charFound = 0;
	      break;
	    }
	  if (charCount >= MAXTOKENBYTES) return err;
	  c = *++start;
	}
      charFound = 0;
      end = start;
      c = *++end;
      charCount = 0;
      while ((jParser->stackPos != 0) ||
	     ((c != ',') && (c != '}') && (c != ']')))
	{
	  switch (c)
	    {
	      case '{':
		jParser->stackPos++;
		break;
	      case '[':
		jParser->stackPos++;
		break;
	      case '}':
		if (jParser->stackPos > 0) jParser->stackPos--;
		break;
	      case ']':
		if (jParser->stackPos > 0) jParser->stackPos--;
		break;
	      default:
		break;
	    }
	  charCount++;
	  if (charCount > MAXTOKENBYTES) return JDATA_TOK_ERR;
	  c = *++end;
	}
      success = strcopy( jParser->jData[currentPos].jsonValue, start, *end, 256, charCount + 2);  // the value stored from start pointer to end pointer
      jParser->currentPos = end - jParser->buffer;  // is copied to the json
						    // struct jsonValue variable
      jParser->currP =
	  jParser->buffer +
	  jParser->currentPos;  // the current position pointer is updated
      jParser->numTokens++;
      charCount = 0;  // the number of tokens parsed are incremented
      if (jParser->stackPos == 0)
	{
	  if (*end == ']')
	    {
	      jParser->parserStatus = JSON_END;  // if '}' is encountered and
						 // the stackPos is 0
	      return JDATA_OK;  // the parser status is updated to
				// JSON_END
	    }			// and JDATA_OK is returned
	  else if (*end == ',')
	    return JDATA_OK;  // else if stackPos is 0 and ',' is
			      // encountered
	  else if (*end == '}')    // the parserStatus is unchanged from
				   // JSON_START
	    return JDATA_INVALID;  // JDATA_OK is returned.
	}
      // printf("Array Parsing!!!\n");
    }
  return JDATA_OK;
}

// SearchToken()
//-------------
//- This function takes the key to be searched in the Parsed json Data
//- returns pointer to the structure containing the needed key

struct jsonReadToken *SearchToken(struct jsonParser *jParser,
				  const char *searchString)
{
  int i = 0, tokens = 0;
  unsigned long hashValue = 0;
  struct jsonReadToken *jData;

  tokens = jParser->numTokens;
  hashValue = HashCal((char *)searchString);
  for (i = 0; i < tokens; i++)
    {
      if (hashValue == jParser->jData[i].hashValue)
	{
	  if (strcmp(searchString, jParser->jData[i].keyValue) == 0)
	    {
	      jData = &jParser->jData[i];
	      return jData;
	    }
	  else
	    return NULL;
	}
    }
  return NULL;
}
