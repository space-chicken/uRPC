
#   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#
#     .d8888. d8888b.  .d8b.   .o88b. d88888b       .o88b. db   db d888888b  .o88b. db   dD d88888b d8b   db
#     88'  YP 88  `8D d8' `8b d8P  Y8 88'          d8P  Y8 88   88   `88'   d8P  Y8 88 ,8P' 88'     888o  88
#     `8bo.   88oodD' 88ooo88 8P      88ooooo      8P      88ooo88    88    8P      88,8P   88ooooo 88V8o 88
#       `Y8b. 88      88   88 8b      88      C88D 8b      88   88    88    8b      88`8b   88      88 V8o88
#     db   8D 88      88   88 Y8b  d8 88.          Y8b  d8 88   88   .88.   Y8b  d8 88 `88. 88.     88  V888 
#     `8888Y' 88      YP   YP  `Y88P' Y88888P       `Y88P' YP   YP Y888888P  `Y88P' YP   YD Y88888P VP   V8P
#
#   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#   Description: Python example code

#   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# ~~                      IMPORTS                     ~~
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
from    random           import randint
from    uRPC             import uRPC
from    enum             import Enum
import  ctypes

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# ~~            SERIAL CONNECTION WRAPPER             ~~
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class CommunicationHandler:

  import serial

  def __init__(self,
    port: str,
    speed: int      = 115200,
    timeout: float  = 3.0
  ) -> None:

    self.__CONNECTION           = self.serial.Serial(port)
    self.__CONNECTION.baudrate  = speed
    self.__CONNECTION.timeout   = timeout
    self.__CONNECTION.parity    = self.serial.PARITY_NONE
    self.__CONNECTION.stopbits  = self.serial.STOPBITS_ONE
    self.__CONNECTION.bytesize  = self.serial.EIGHTBITS

  def read(self, count: int) -> bytearray:
    return self.__CONNECTION.read(size = count)

  def write(self, data: bytearray) -> None:
    self.__CONNECTION.write(data)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# ~~                    PROCEDURES                    ~~
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class PROCEDURES(Enum):
  SELF_TEST               = 0

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# ~~                RETURNED STRUCTURES               ~~
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class TestValue(ctypes.LittleEndianStructure):
  _fields_ = [('value', ctypes.c_uint32)]

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# ~~                    ENTRYPOINT                    ~~
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
if __name__ == "__main__":

  print("Initialize communication handler")
  connection = CommunicationHandler(
    port    = "COM1",
    speed   = 115200,
    timeout = 3.0
  )

  print("Initialize RPC")
  rpc = uRPC(
    functionSend      = connection.write,
    functionReceive   = connection.read,
    crc               = True
  )

  # Get timestamp
  arguments       = TestValue()
  arguments.value = randint(0, 999)
  status, code, data = rpc.execute(
    command = PROCEDURES.SELF_TEST.value,
    payload = bytearray(arguments)
  )

  if not status:
    print(PROCEDURES.SELF_TEST.name + " command failed")
    exit(1)

  if code != 0:
    print(PROCEDURES.SELF_TEST.name + " command returned error code: " + str(code))
    exit(1)

  returnedValue = TestValue.from_buffer_copy(data)
  print("Returned value: " + str(returnedValue.value))

  if returnedValue != (arguments.value + 0xCAFE):
    print("Wrong returned value")
    exit(1)
  
  print("Test succeeded")