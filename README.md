# BACnet Server MSTP Example CPP

A minumal BACnet MSTP server example written with C++ using the [CAS BACnet Stack](https://www.bacnetstack.com/). For a full featured BACnet Server example see the [BACnet Server Example CPP](https://github.com/chipkin/BACnetServerExampleCPP) project.

Device tree:

- Device: 389999 (Device Rainbow)
  - analog_value: 2 (AnalogValue Diamond)

## Cli

The serial port, baud rate, and MAC address of this BACnet MSTP server can be configured using command line parameters.

```bash
BACnetServerMSTPExampleCPP [serial port] [baud rate] [mac address]
BACnetServerMSTPExampleCPP COM5 9600 1
BACnetServerMSTPExampleCPP ttyS6 19200 25
```

## Output

The application outputs all BYTES sent or recived. BYTES with underscore after them are recived bytes. BYTES with a space after them are sent bytes. When the user presses the "enter" key, the MSTP stats, states, and events are printed for reference.

```txt
CAS BACnet Stack Server MSTP Example v0.0.1.0
https://github.com/chipkin/BACnetServerMSTPExampleCPP

FYI: Using comport=[COM6], Portnr=[5]
FYI: Loading CAS BACnet Stack functions... OK
FYI: CAS BACnet Stack version: 3.12.11.0
FYI: Connecting serial resource to serial port. baudrate=[9600], ... OK
OK, Connected to port
FYI: Registering the callback Functions with the CAS BACnet Stack
Setting up server device. device.instance=[389999]
Created Device.
Enabling WriteProperty service... OK
Enabling device optional properties. Max_info_frames, max_master... OK
FYI: Sending I-AM broadcast
FYI: Entering main loop...
55_ff_01_03_0a_00_00_1b_  {src=0a, dest=03 POLL_FOR_MASTER}
55_ff_01_04_0a_00_00_a1_  {src=0a, dest=04 POLL_FOR_MASTER}
55_ff_01_05_0a_00_00_28_  {src=0a, dest=05 POLL_FOR_MASTER}
55_ff_01_06_0a_00_00_b0_  {src=0a, dest=06 POLL_FOR_MASTER}
[ReceivedPFM]
55 ff 02 0a 00 00 00 a0   {src=00, dest=0a REPLY_TO_POLL_FOR_MASTER}
55_ff_00_00_0a_00_00_05_  {src=0a, dest=00 TOKEN}
[SendCustomFrame]
55 ff 06 ff 00 00 15 16 01 20 ff ff 00 ff 10 00 c4 02 05 f3 6f 22 05 c4 91 03 22 01 85 ec 0c   {src=00, dest=ff DATA_NOT_EXPECTING_REPLY}
[NextStationUnknown]
55 ff 01 01 00 00 00 f5   {src=00, dest=01 POLL_FOR_MASTER}
55 ff 01 02 00 00 00 6d   {src=00, dest=02 POLL_FOR_MASTER}
55 ff 01 03 00 00 00 e4   {src=00, dest=03 POLL_FOR_MASTER}
55 ff 01 09 00 00 00 b1   {src=00, dest=09 POLL_FOR_MASTER}
55 ff 01 0a 00 00 00 29   {src=00, dest=0a POLL_FOR_MASTER}
55_ff_02_00_0a_00_00_0a_  {src=0a, dest=00 REPLY_TO_POLL_FOR_MASTER}
[ReceivedReplyToPFM]
55 ff 00 0a 00 00 00 af   {src=00, dest=0a TOKEN}
55_ff_05_00_0a_00_16_6e_01_[Data]
0c_00_0b_06_c0_a8_01_7e_ba_c0_00_05_c0_0c_0c_02_05_f3_6f_19_0b_d0_df_  {src=0a, dest=00 DATA_EXPECTING_REPLY}
[ReceivedDataNeedingReply]
55 ff 07 0a 00 00 00 38   {src=00, dest=0a REPLY_POSTPONED}
55_ff_00_00_0a_00_00_05_  {src=0a, dest=00 TOKEN}
[SendCustomFrame]
55 ff 06 0a 00 00 1b b7 01 20 00 0b 06 c0 a8 01 7e ba c0 ff 30 c0 0c 0c 02 05 f3 6f 19 0b 3e 22 0b b8 3f 5a 4c   {src=00, dest=0a DATA_NOT_EXPECTING_REPLY}
55 ff 00 0a 00 00 00 af   {src=00, dest=0a TOKEN}
55_ff_00_00_0a_00_00_05_  {src=0a, dest=00 TOKEN}
55 ff 00 0a 00 00 00 af   {src=00, dest=0a TOKEN}

MSTP Stats:
-----------
              sentBytes:  2458              recvBytes:  2180             eatAnOctet:    17
             sentFrames:   218             recvFrames:   189      sentPollForMaster:    28
               sentAPDU:    22               recvAPDU:    21 sentReplyPollForMaster:     1
           sentLastType:     0           recvLastType:     0              sentToken:   146
                                          recvInvalid:     1               hasToken:     0
MSTP State:
-----------
       This station (TS):    0              EventCount:    5            master_state:    0
       Next Station (NS):   10              FrameCount:    1              recv_state:    2
       Poll Station (PS):    0              TokenCount:   28              SoleMaster:    0
   Previous Station (AS):   10              RetryCount:    0          SilenceTimerMS: 7384

MSTP Events:
------------
               BadHeader:    0                    Data:    1              EatAnOctet:    0
      NextStationUnknown:    1ReceivedDataNeedingReply:    1    ReceivedInvalidFrame:    0
             ReceivedPFM:    1      ReceivedReplyToPFM:    1     ResetMaintenancePFM:    2
          RetrySendToken:    0         SendCustomFrame:    1      SendMaintenancePFM:    1
```

## Building

A [Visual studio 2019](https://visualstudio.microsoft.com/downloads/) project is included with this project. This project also auto built using [Gitlab CI](https://docs.gitlab.com/ee/ci/) on every commit.

A makefile that uses gcc is included for making excultables on linux.

## Releases

Build versions of this example can be downloaded from the releases page:

[https://github.com/chipkin/BACnetServerMSTPExampleCPP/releases](https://github.com/chipkin/BACnetServerMSTPExampleCPP/releases)
