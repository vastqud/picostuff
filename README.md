# picostuff

In main.c, you'll find a primitive driver for communication between an RP2040 microprocessor and an NRF24L01+ radio transceiver.
Currently, the driver can only operate in TX mode. It sends one packet and reads various registers from the transceiver. It also initializes the transceiver to my desired settings by setting bits in various registers.
This code can easily be extended and modularized to allow for both TX and RX modes, as well as sending/receiving arbitrary packets.
The RP2040 can determine what to send as a packet, as well as what to do with packets once received. It was intended to be used as a custom remote-control system.
The NRF24L01+ board I used in my project also has a power amplifier chip, noise protection, a bypass capacitor, and a long antenna.
