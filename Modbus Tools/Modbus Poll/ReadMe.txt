------------------------------------------------------------------------------------
                       Modbus Poll
         Copyright (c) 2002-2017. Witte Software.
------------------------------------------------------------------------------------


Getting started:
-----------------------------
1. Press F3 to setup a connection.
2. Press F8 to setup protocol definition.
   Example:
   You want to read 20 holding registers from address 40011 (10).
   - Slave ID  = The ID of your device
   - Function  = 03 Read Holding Registers
   - Address   = 10 (Protocol address)
   - Quantity  = 20
   - Scan Rate = 1000 (Then it will poll for data every 1000ms)

3. To change a value just start typing or double click the cell.


Note: You can always press F1 for help.



Warning:
-----------------------------
The use of RTS controlled RS232/RS485 converters should be avoided if possible.
It is difficult to determine the exact time when to switch off the transmitter
with non real-time operating systems like Windows and Linux. If it is switched
off to early characters might still sit in the FIFO or the transmit register of
the UART and these characters will be lost. Hence the slave will not recognize
the message. On the other hand if it is switched off too late then the slave's
message is corrupted and the master will not recognize the message.



Modbus Poll overview:
-----------------------------
Modbus Poll is a powerful, easy to use, master simulating
tool. Modbus Poll will run on all 32/64-bit Windows versions
in use today - XP, Vista, 7, 8, 8.1 and 10.

With Modbus Poll you can monitor and test your modbus
slave devices. Some of the features:
- Supports Modbus/TCP, RTU and ASCII modes.
- Test Center for modbus slave developers.
- OLE Automation for interfacing with Visual Basic
  Excel etc. To interpret and show the modbus data
  according to your specific requirements. E.g. edit
  data in Excel and then send to your slave!
- Multiple windows for monitoring of multiple
  slaves/data areas.
- Monitoring of serial traffic.
- Data logging to a text file.
- Data logging to MS Excel.
- Multithreated for best performance.
- Print preview etc.
- 16 Display formats.

Press F2 or double click to edit or just start typing a new value.



Order information:
-----------------------------
Visit http://www.modbustools.com


Technical Support:
-----------------------------
Your feedback is very important for the further development
of Modbus Poll. For bug reports, comments, suggestions or
complaints, please find contact information at
http://www.modbustools.com
