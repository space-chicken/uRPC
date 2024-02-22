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

#include "uRPC.h"

//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  ~~                                           GLOBAL VARIABLES                                           ~~
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static uRpcHeader   uRpcRequestHeader;
static char *       uRpcRequestBuffer   = NULL;
static uRPC_SIZE_T  uRpcByteCount;

//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  ~~                                           PRIVATE FUNCTIONS                                          ~~
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifdef uRPC_CRC_CHECKSUM
// Calculate CRC32
uRPC_CRC_T calculateCrc(char * data, uRPC_SIZE_T length)
{
  int index, bit;
  unsigned int byte, crc, mask;

  index = 0;
  crc   = 0xFFFFFFFF;
  while (index < length)
  {
    byte = *(char *)((uRPC_POINTER_T)data + index);
    crc   = crc ^ byte;
    for (bit = 7; bit >= 0; bit--)
    { 
      mask  = -(crc & 1);
      crc   = (crc >> 1) ^ (0xEDB88320 & mask);
    }
    index   = index + 1;
  }
  return ~crc;
}
#endif

//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  ~~                                            PUBLIC FUNCTIONS                                          ~~
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Initialize variables
void uRpcInitialize()
{
  memset(&uRpcRequestHeader,  0, sizeof(uRpcHeader));
  if(uRpcRequestBuffer != NULL)
  {
    free(uRpcRequestBuffer);
    uRpcRequestBuffer = NULL;
  }
  uRpcByteCount = 0;
}

// Handle requests from serial interface
int uRpcHandle(
  const uRPC_PROCEDURE * handlersTable,     // Pointer to the table with handlers
  const uRPC_CODE_T      handlersCount      // Pointer table size (number of procedures)
)
{
  // Initialize variables
  if(uRpcByteCount == 0) uRpcInitialize();

  // Receive request header from serial interface one by one
  if(uRpcByteCount <= sizeof(uRpcHeader))
  {
    *((char *)((uRPC_POINTER_T)&uRpcRequestHeader + uRpcByteCount)) = receiveByte();
    if(++uRpcByteCount < sizeof(uRpcHeader))
      return uRPC_STATUS_WAITING_DATA;
  }

#ifdef uRPC_CRC_CHECKSUM
  uRPC_CRC_T expectedCrc;
  uRPC_CRC_T receivedCrc;

  // Validate request header CRC
  if(uRpcByteCount == sizeof(uRpcHeader))
  {
    // Calculate expected CRC and compare with received one
    receivedCrc = uRpcRequestHeader.crc.header;
    uRpcRequestHeader.crc.header = 0;
    expectedCrc = uRPC_CRC_FUNCTION((char *)&uRpcRequestHeader, sizeof(uRpcHeader));
    if(expectedCrc != receivedCrc)
    {
      uRpcByteCount = 0;
      return uRPC_STATUS_WRONG_CRC;
    }
  }
#endif

  // Receive payload if required
  if((uRpcRequestHeader.size - sizeof(uRpcHeader)) > 0)
  {
    // Allocate memory for input buffer
    if(uRpcByteCount == sizeof(uRpcHeader))
    {
      if((uRpcRequestBuffer = malloc((size_t)(uRpcRequestHeader.size - sizeof(uRpcHeader)))) == NULL)
      {
        uRpcByteCount = 0;
        return uRPC_STATUS_ERROR_MALLOC;
      }
      uRpcByteCount++;
      return uRPC_STATUS_WAITING_DATA;
    }

    if(uRpcByteCount <= (uRpcRequestHeader.size + 1))
    {
      *((char *)((uRPC_POINTER_T)uRpcRequestBuffer + (uRpcByteCount - sizeof(uRpcHeader) - 1))) = receiveByte();
      if(++uRpcByteCount < (uRpcRequestHeader.size + 1))
        return uRPC_STATUS_WAITING_DATA;
    }

#ifdef uRPC_CRC_CHECKSUM
    // Calculate expected CRC and compare with received one
    expectedCrc = uRPC_CRC_FUNCTION((char *)uRpcRequestBuffer, (uRpcRequestHeader.size - sizeof(uRpcHeader)));
    if(expectedCrc != uRpcRequestHeader.crc.payload)
    {
      uRpcByteCount = 0;
      return uRPC_STATUS_WRONG_CRC;
    }
#endif
  }

  // Validate input command
  if(uRpcRequestHeader.code >= handlersCount)
  {
    uRpcByteCount = 0;
    return uRPC_STATUS_UNKNOWN_CMD;
  }

  uRPC_SIZE_T requestSize     = (uRpcRequestHeader.size - sizeof(uRpcHeader));
  uRPC_SIZE_T responseSize    = uRPC_OUT_BUFFER_SIZE_BYTES;
  uRpcHeader  responseHeader;
  char *      responseBuffer;

  // Allocate response buffer
  if((responseBuffer = malloc(uRPC_OUT_BUFFER_SIZE_BYTES)) == NULL)
  {
    uRpcByteCount = 0;
    return uRPC_STATUS_ERROR_MALLOC;
  }
  memset(&responseHeader, 0, sizeof(uRpcHeader));
  memset(responseBuffer,  0, uRPC_OUT_BUFFER_SIZE_BYTES);

  // Call handler
  responseHeader.code = (handlersTable[uRpcRequestHeader.code])(
    uRpcRequestBuffer,
    requestSize,
    responseBuffer,
    &responseSize
  );

  // Set default values
  responseHeader.size = sizeof(uRpcHeader) + responseSize;

#ifdef uRPC_CRC_CHECKSUM
  responseHeader.crc.header   = 0;
  responseHeader.crc.payload  = 0;

  // Calculate payload CRC first
  if(responseSize > 0)
    responseHeader.crc.payload = uRPC_CRC_FUNCTION(responseBuffer, responseSize);

  // Calculate header CRC
  responseHeader.crc.header = uRPC_CRC_FUNCTION((char *)&responseHeader, sizeof(uRpcHeader));
#endif

  // Send response
  for(uRpcByteCount = 0; uRpcByteCount < responseHeader.size; uRpcByteCount++)
  {
    if(uRpcByteCount < sizeof(uRpcHeader))
      transmitByte(*((char *)((uRPC_POINTER_T)&responseHeader + uRpcByteCount)));
    else
      transmitByte(responseBuffer[uRpcByteCount - sizeof(uRpcHeader)]);
  }

  uRpcByteCount = 0;
  if(responseBuffer != NULL)
  {
    free(responseBuffer);
    responseBuffer = NULL;
  }
  return uRPC_STATUS_SUCCESS;
}