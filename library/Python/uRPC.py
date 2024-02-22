
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
#    .d8888. d8888b.  .d8b.   .o88b. d88888b       .o88b. db   db d888888b  .o88b. db   dD d88888b d8b   db
#    88'  YP 88  `8D d8' `8b d8P  Y8 88'          d8P  Y8 88   88   `88'   d8P  Y8 88 ,8P' 88'     888o  88
#    `8bo.   88oodD' 88ooo88 8P      88ooooo      8P      88ooo88    88    8P      88,8P   88ooooo 88V8o 88
#      `Y8b. 88      88   88 8b      88      C88D 8b      88   88    88    8b      88`8b   88      88 V8o88
#    db   8D 88      88   88 Y8b  d8 88.          Y8b  d8 88   88   .88.   Y8b  d8 88 `88. 88.     88  V888 
#    `8888Y' 88      YP   YP  `Y88P' Y88888P       `Y88P' YP   YP Y888888P  `Y88P' YP   YD Y88888P VP   V8P
#
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#  Description: Simple RPC implementation for embedded processors and microcontrollers.
#  
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class uRPC:
  """
  uRPC Python wrapper.
  """
  
  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # ~~                      IMPORTS                     ~~
  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  import  ctypes
  import  zlib
  from    typing      import Tuple

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # ~~                CLASS CONSTRUCTOR                 ~~
  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  def __init__(self,
    functionSend:     any,
    functionReceive:  any,
    functionCrc:      any   = None,
    length_t:         any   = ctypes.c_uint32,
    code_t:           any   = ctypes.c_uint16,
    crc_t:            any   = ctypes.c_uint32,
    crc:              bool  = True
  ) -> None:
    """
    Class constructor.

    Args:
        functionSend      (any):              function that accepts a byte array as an argument and is
                                              used to send data.
        functionReceive   (any):              blocking function that have no arguments and returns
                                              received byte array.
        functionCrc       (any, optional):    function that calaculate CRC value in request and response
                                              headers. Defaults to None (internal function).
        length_t          (any, optional):    ctype type of length variables. Defaults to ctypes.c_uint32.
        code_t            (any, optional):    ctype type of command/status code variable. Defaults to ctypes.c_uint16.
        crc_t             (any, optional):    ctype type of CRC value. Defaults to ctypes.c_uint32.
        crc               (bool, optional):   enable CRC value in request/response headers. Defaults to True.
        
    Notes:
      *********************************************************************
      *  IMPORTANT !                                                      *
      *    length_t, code_t and crc_t types must match types defined in   *
      *    uRPC.h header !                                                *
      *********************************************************************
    """
    if not callable(functionSend):
      TypeError("'functionSend' is not a function")
      
    if not callable(functionReceive):
      TypeError("'functionReceive' is not a function")
      
    if not callable(functionReceive):
      TypeError("'functionReceive' is not a function")
    
    if functionCrc != None:
      if not callable(functionCrc):
        TypeError("'functionCrc' is not a function")
        
    if not issubclass(type(length_t), type(self.ctypes)):
      TypeError("'length_t' is not a ctype type")
      
    if not issubclass(type(code_t), type(self.ctypes)):
      TypeError("'code_t' is not a ctype type")
      
    if not issubclass(type(crc_t), type(self.ctypes)):
      TypeError("'crc_t' is not a ctype type")
      
    if not type(crc) is bool:
      TypeError("'crc' is not a bool")
      
    self.__RECEIVE  = functionReceive
    self.__SEND     = functionSend
    if functionCrc == None:
      self.__CRC    = self.__calculateCrc
    else:
      self.__CRC    = functionCrc
      
    self.length_t   = length_t
    self.code_t     = code_t
    self.crc_t      = crc_t
    self.crc        = crc
    
  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # ~~                INTERNAL FUNCTIONS                ~~
  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  def __calculateCrc(self, data: bytearray) -> int:
    """
    Function calculates CRC value of a byte array.

    Args:
        data        (bytearray):          array of bytes.

    Returns:
        int:        CRC value.
    """
    return self.zlib.crc32(data) % (1<<32)
  
  def __getHeader(self):
    """
    Construct header structure.
    """
    
    class Crc(self.ctypes.LittleEndianStructure):
      """
      CRC fields.
      """
      _pack_   = 1
      _fields_ = [('header',    self.crc_t),
                  ('payload',   self.crc_t)]
    
    class Header(self.ctypes.LittleEndianStructure):
      """
      Request/response header.
      """
      _pack_     = 1
      if self.crc:
        _fields_ = [('length',  self.length_t),
                    ('code',    self.code_t),
                    ('crc',     Crc)]
      else:
        _fields_ = [('length',  self.length_t),
                    ('code',    self.code_t)]
      
    return Header
  
  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  # ~~                 CLASS FUNCTIONS                  ~~
  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  def execute(self,
    command:  int,
    payload:  bytearray = bytearray()
  ) -> Tuple[bool, int, bytearray]:
    """
    Execute command.

    Args:
        command     (int):                    command index.
        payload     (bytearray, optional):    command payload.

    Returns:
        bool:       True if the command sent successfully.
        int:        command status/return code on success.
        bytearray:  command response (optional).
    """
    
    # Set default values
    requestHeader         = self.__getHeader()
    requestHeader         = requestHeader()
    requestHeader.code    = command
    requestHeader.length  = self.ctypes.sizeof(requestHeader) + len(payload)
    
    # Calculate CRC if required
    if self.crc:
      requestHeader.crc.header    = 0
      requestHeader.crc.payload   = 0
      
      if len(payload) > 0:
        requestHeader.crc.payload =  self.__CRC(payload)
      requestHeader.crc.header    =  self.__CRC(bytearray(requestHeader))
      
    # Send request and then payload
    self.__SEND(bytearray(requestHeader))
    if len(payload) > 0:
      self.__SEND(payload)
      
    # Receive header
    returnedData    = self.__RECEIVE(self.ctypes.sizeof(self.__getHeader()))
    responseHeader  = self.__getHeader().from_buffer_copy(returnedData)

    # Calculate CRC
    if self.crc:
      receivedHeaderCrc         = responseHeader.crc.header
      responseHeader.crc.header = 0
      expectedCrc               = self.__CRC(responseHeader)
      if expectedCrc != receivedHeaderCrc:
        return False, 0, bytearray()
    
    # Get payload if passed
    responsePayloadSize = (responseHeader.length - self.ctypes.sizeof(self.__getHeader()))
    returnedPayload     = self.__RECEIVE(responsePayloadSize) if responsePayloadSize > 0 else bytearray()

    # Calculate payload CRC
    if self.crc and responsePayloadSize > 0:
      expectedCrc = self.__CRC(returnedPayload)
      if expectedCrc != responseHeader.crc.payload:
        return False, 0, bytearray()
      
    return True, responseHeader.code, returnedPayload