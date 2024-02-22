//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    .d8888. d8888b.  .d8b.   .o88b. d88888b       .o88b. db   db d888888b  .o88b. db   dD d88888b d8b   db
//    88'  YP 88  `8D d8' `8b d8P  Y8 88'          d8P  Y8 88   88   `88'   d8P  Y8 88 ,8P' 88'     888o  88
//    `8bo.   88oodD' 88ooo88 8P      88ooooo      8P      88ooo88    88    8P      88,8P   88ooooo 88V8o 88
//      `Y8b. 88      88   88 8b      88      C88D 8b      88   88    88    8b      88`8b   88      88 V8o88
//    db   8D 88      88   88 Y8b  d8 88.          Y8b  d8 88   88   .88.   Y8b  d8 88 `88. 88.     88  V888 
//    `8888Y' 88      YP   YP  `Y88P' Y88888P       `Y88P' YP   YP Y888888P  `Y88P' YP   YD Y88888P VP   V8P
//
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  Description: Simple RPC implementation for embedded processors and microcontrollers.
//  
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  ~~                                              INCLUDES                                                ~~
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <stdlib.h>
#include <string.h>

//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  ~~                                            CONFIGURATION                                             ~~
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/*
  The maximum buffer size in bytes for output requests. If the buffer size is too large, it will increase
  application size, a smaller buffer size may result in data loss.
*/
#define uRPC_OUT_BUFFER_SIZE_BYTES          64

/*
  Define function used for CRC calculation (only if uRPC_CRC_CHECKSUM defined).
*/
#ifdef uRPC_CRC_CHECKSUM
#ifndef uRPC_CRC_FUNCTION
#define uRPC_CRC_FUNCTION(pointer, size)    (uRPC_CRC_T)calculateCrc(pointer, size)
#endif
#endif

//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  ~~                                          EXTERNAL FUNCTIONS                                          ~~
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/*
  Low-level function used to receive one byte.
  Must be defined somewhere in your code.
*/
extern char receiveByte();
/*
  Low-level function used to send one byte.
  Must be defined somewhere in your code.
*/
extern void transmitByte(char byte);

//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  ~~                                              STRUCTURES                                              ~~
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifndef __URPC__
#define __URPC__

/*
  Define type for each element used in the request/response headers.
  
  NOTE: If a small footprint is a consern, you may change this definitions.
*/
typedef unsigned int                    uRPC_SIZE_T;      // Buffer length
typedef unsigned short                  uRPC_CODE_T;      // Command index and returned code
typedef unsigned int                    uRPC_CRC_T;       // CRC value (depends on the CRC type)
typedef unsigned long long              uRPC_POINTER_T;   // Pointer

/*
  List of error codes returned by the HandleRequest function
*/
#define uRPC_STATUS_WAITING_DATA        -1                // Waiting for more bytes
#define uRPC_STATUS_SUCCESS             0                 // Command processed successfully
#define uRPC_STATUS_WRONG_CRC           1                 // Header or payload CRC mismatch
#define uRPC_STATUS_UNKNOWN_CMD         2                 // Received command index is out of range
#define uRPC_STATUS_ERROR_MALLOC        3                 // Out of memory (malloc() returned NULL)

/*
  Handler function defintion.

  Pointers to all handlers must be combined into an array, like so:

    const static uRPC_PROCEDURE HANDLERS_TABLE[] = {
      handlerFunction1,
      handlerFunction2,
      hdnalerFunction3,
      ...
    }

  A pointer to this array must be passed to the function
  HandleRequest(&HANDLERS_TABLE, (sizeof(HANDLERS_TABLE) / sizeof(uRPC_PROCEDURE))).
  The corresponding handler will be called when a request is received.
*/
typedef uRPC_CODE_T (*uRPC_PROCEDURE)(
  char *        inBuffer,     // Pointer to the received payload
  uRPC_SIZE_T   request,      // Received payload size in bytes
  char *        outBuffer,    // Pointer to the response payload buffer
                              // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  uRPC_SIZE_T * response      // Response payload buffer size in bytes
                              // 
                              //   IMPORTANT ! When the handler is called, this variable is assigned the maximum
                              //   size of outBuffer in bytes. If no payload is returned, this value should
                              //   be set to 0.
                              //
                              // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
);                            // The function return value is binded into the 'code' field in the response header.

#pragma pack(push, 1)

/*
  This structure defines CRC fields in the request/response header. There is one CRC for
  header and another one for payload.
*/
typedef struct
{
  uRPC_CRC_T      header;
  uRPC_CRC_T      payload;
} uRpcCrcHeader;

/*
  Request/response header structure.
*/
typedef struct
{
  // Size of header and payload together in bytes.
  uRPC_SIZE_T     size;           

  // Command to execute in request header or status code in response header.
  uRPC_CODE_T     code;

  // CRC fields are added when required.
#ifdef uRPC_CRC_CHECKSUM
  uRpcCrcHeader   crc;      
#endif

} uRpcHeader;

#pragma pack(pop)

//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  ~~                                               FUNCTIONS                                              ~~
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Initialize variables
void uRpcInitialize();

// Handle requests from serial interface
int uRpcHandle(
  const uRPC_PROCEDURE * handlersTable,     // Pointer to the table with handlers
  const uRPC_CODE_T      handlersCount      // Pointer table size (number of procedures)
);

#endif
