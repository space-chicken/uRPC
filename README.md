# uRPC
Simple Remote Procedure Call (RPC) for embedded processors and microcontrollers with Python library. It allows you to call a function (procedure) on a processor or microcontroller via a serial interface. This example will help you speed up your code development process by providing a turnkey solution that is simple, scalable, and can run on any platform.

## Installation
1. Copy uRPC.c/uRPC.h to your project folder.
2. Implement the `receiveByte` and `transmitByte` functions. The  `receiveByte` function reads and returns one byte. This function can be blocking or non-blocking. The `transmitByte` function is a blocking one that sends one byte at a time.
3. Internal structures must be initialized at power-up by calling the `uRpcInitialize` function. This function can be called at any time to reset internal structures if transmission errors are detected.
4. Create procedures. Example:
```c
// This procedure does not return any data, only a status code
uRPC_CODE_T HelloWorldWithoutReturn(char * inBuffer, uRPC_SIZE_T request, char * outBuffer, uRPC_SIZE_T * response)
{
  *response = 0;      // Payload size = 0 byte (No payload)
  return 0;           // Return code  = 0
}

// This procedure returns a data and a status code
uRPC_CODE_T HelloWorldWithReturn(char * inBuffer, uRPC_SIZE_T request, char * outBuffer, uRPC_SIZE_T * response)
{
  const char * text = "Hello, Space-Chicken!";
  response = sizeof(text);                  // Payload size = sizeof("Hello, Space-Chicken!")
  memcpy(outBuffer, text, sizeof(text));    // Copy data into output buffer
  return 15;                                // Return code  = 15
}
```

4. Create a table with all functions (procedures). Example:
```c
const static uRPC_PROCEDURE HANDLERS_TABLE[] = {
  HelloWorldWithoutReturn,    // index = 0
  HelloWorldWithReturn        // index = 1
};
```  

5. Call the `HandleRequests` function when a byte was received, and pass a pointer to the table with all functions and total number of functions. Example:
```c
// IMPORTANT ! This function will block code execution untill one byte is received
while(1)
  uRpcHandle(&HANDLERS_TABLE[0], (sizeof(HANDLERS_TABLE) / sizeof(uRPC_PROCEDURE)));
```
6. A remote machine can call any function defined in the table by sending the function index.

## Returned status code
The `uRpcHandle` function returns follow status codes:
| Status/error code | Define name | Description |
|:-:|:--|---|
|-1|uRPC_STATUS_WAITING_DATA|Waiting for more bytes|
|0|uRPC_STATUS_SUCCESS|Request processed successfully|
|1|uRPC_STATUS_WRONG_CRC|A CRC checksum mismatch was detected in the request header or payload|
|2|uRPC_STATUS_UNKNOWN_CMD|Function index is out of range|
|3|uRPC_STATUS_ERROR_MALLOC|Attempt to allocate output buffer failed|

## Preprocessor directives
### CRC checksum
An optional CRC32 checksum can be added to each request/response by adding the `uRPC_CRC_CHECKSUM` preprocessor define. The function that calculates the CRC checksum can be overridden by specifying the `uRPC_CRC_FUNCTION funcNameForCrcChecksum` define.

## Option
### Output buffer
The maximum size of the returned payload is set by the `uRPC_OUT_BUFFER_SIZE_BYTES` define. When the procedure is called, the `response` argument is set to the maximum size of the output buffer. If no payload is returned, the function should override this value to 0. If a payload is returned, this argument should be set to the actual size of the payload. The returned payload cannot exceed the maximum output buffer size.

## Limitations
### Execution blocking
The `uRpcHandle` function blocks code execution until at least one byte has been received. This can be fixed by verifying data availability before calling the `uRpcHandle` function or by using interrupts.
#### Verifying data availability
```c
while(1)
{
  ...

  // Assuming `byteReceived()` returns 1 when at least one byte has been received
  if(byteReceived())
    uRpcHandle(&HANDLERS_TABLE[0], (sizeof(HANDLERS_TABLE) / sizeof(uRPC_PROCEDURE)));
  
  ...
}
```
#### Interrupts
```c

volatile unsigned int RxByteCount = 0;
...

void InterruptHandler(void)
{
  // Assuming the `getRxByteCount` function returns the number of bytes received.
  RxByteCount += getRxByteCount();
}

...
while(1)
{
  int index;
  ...

  // Assuming `byteReceived()` returns 1 when at least one byte is received.
  while(RxByteCount > 0)
  {
    uRpcHandle(&HANDLERS_TABLE[0], (sizeof(HANDLERS_TABLE) / sizeof(uRPC_PROCEDURE)));

    // Disable interrupts here
    RxByteCount--;
    // Enable interrupts here
  }
  
  ...
}
```
### Synchronization
If an error occurs during data transfer, the target processor may hang waiting for more data to arrive, and the transmitter will assume that all data has been sent. There are two ways to solve this problem:
1. You can call a special procedure that generates the same result. If the receiver does not receive any response, it can send a dummy byte to offset the byte count at the receiver side and repeat the previous step until the correct response is received.
2. (Recommended) You can add a watchdog to reset the system if it gets stuck in the middle of a transfer. This timer can be controlled using the return status of the `uRpcHandle` function.

## Performance
Here are the performance measurement details:
| Parameter | Value |
|:--|:--|
|Platform|AMD Zynq MPSoC UltraScale+|
|Processor|ARM Cortex-A53 64-bits @ 1316 MHz|
|Communication channel|Serial interface @ 115200 baud, 1 stop bit, 8 data bits, no handshake|
|Serial adapter|FT232|
|Target procedure length|34 assembly instructions|
|Rx mode|Polling|
|Python version|3.11.1|
|pyserial version|3.5|
|OS version|Windows 10.0.19042|

An average of __62 requests/second__ was achieved.

## License
MIT License