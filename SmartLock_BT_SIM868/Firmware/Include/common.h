#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>

typedef struct
{
  uint32_t admin_auth_number;
  uint32_t supervisor_auth_number;
  uint32_t operator_auth_number;
  uint32_t client_auth_number;
} unlck_auth_numbers;

typedef enum
{
  SUCCESS,
  FAILURE
} result;

typedef enum
{
  SERVER,
  MOBILE
} source;

// typedef struct
//{
// source src;
// char packet[BUFF_SIZE];
//} request_packet;

typedef void (*callback)(void*);

typedef enum
{
  SHACKLE_OUT,
  SHACKLE_IN
} shackle_status;

typedef enum
{
  UNLOCKED,
  LOCKED
} lock_status;

uint8_t convert_string_to_integer(char* number);

void convert_integer_to_string(char* str, uint16_t number);

char* strcpymarker(char* src, char* dest, char startMarker, char endMarker);

#endif
