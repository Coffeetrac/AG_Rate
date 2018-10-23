void SetRelays(void)
 {
    if (bitRead(relayLo,0)) bitSet(PORTD, 5); //Digital Pin 5
    else bitClear(PORTD, 5); 
    if (bitRead(relayLo,1)) bitSet(PORTD, 6); //Digital Pin 6
    else bitClear(PORTD, 6); 
    if (bitRead(relayLo,2)) bitSet(PORTD, 7); //Digital Pin 7
    else bitClear(PORTD, 7); 
    if (bitRead(relayLo,3)) bitSet(PORTB, 0); //Digital Pin 8
    else bitClear(PORTB, 0); 
    if (bitRead(relayLo,4)) bitSet(PORTB, 1); //Digital Pin 9
    else bitClear(PORTB, 1); 
    if (bitRead(relayLo,5)) bitSet(PORTB, 2); //Digital Pin 10
    else bitClear(PORTB, 2); 
    if (bitRead(relayLo,6)) bitSet(PORTB, 5); //Digital Pin 13
    else bitClear(PORTB, 5); 
    if (bitRead(relayLo,7)) bitSet(PORTC, 0); //Analog Pin A0
    else bitClear(PORTC, 0); 
    if (bitRead(relayHi,0)) bitSet(PORTC, 1); //analog Pin A1
    else bitClear(PORTC, 1); 
    if (bitRead(relayHi,1)) bitSet(PORTC, 2); //Analog Pin A2
    else bitClear(PORTC, 2);     
    if (bitRead(relayHi,2)) bitSet(PORTC, 3); //Analog Pin A2
    else bitClear(PORTC, 3);     
    if (bitRead(relayHi,3)) bitSet(PORTC, 4); //Analog Pin A2
    else bitClear(PORTC, 4);     
    if (bitRead(relayHi,4)) bitSet(PORTC, 5); //Analog Pin A2
    else bitClear(PORTC, 5);     
 }
