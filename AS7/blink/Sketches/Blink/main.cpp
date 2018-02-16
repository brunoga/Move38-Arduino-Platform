/*
 *  Take on the color of the dominant Blink attached
 */

byte myState = 0;
Color colors[] = { BLUE, RED, YELLOW, ORANGE, GREEN};
const byte myState_count = COUNT_OF (colors);

bool errorFlag[ FACE_COUNT ];

void clearErrors() {
  FOREACH_FACE(f) {
    errorFlag[f]=false;
  }  
}

void setup() {
  // put your setup code here, to run once:
  clearErrors();
}

// Sin in degrees ( standard sin() takes radians )

float sin_d( uint16_t degrees ) {

    return sin( ( degrees / 360.0F ) * 2.0F * PI   );
}

// Returns a number 0-255 that throbs in a sin over time

const word throb_duration_ms=500;    // How long one throb takes

byte throbbing(void) {

  word offset_ms = millis() % throb_duration_ms;

  // offset range [0,throb_duration_ms)

  float phase = (float) offset_ms / (float) throb_duration_ms;

  // phase range [0,1)

  float wave = sin( phase * 2.0 * PI );

  // wave range [-1,1]

  byte wave_byte = ( (wave + 1.0) * (255.0/2) );          // Note: NOT times 128! that would overflow byte!

  // wave_byte range [0,255]
  
  return wave_byte;

}


// Returns the circular maxiumum of the two values
// I define this as being the one that is larger looking 
// from the perspective they are closer togther than farther 
// appart. Make sense? 
// To keep myself freom getting confused, I 
// first figure out i,j where i<=j<count
// next we figure out x,y,z which are...
// x=distance from 0 to i
// y=distance from i to j
// z-distace from j to count

byte circularMax( byte a , byte b , byte count ) {

  byte i,j;

  if ( b > a )   {

    i = a;
    j = b;

  } else {

    i = b;
    j = a;

  }

  // Ok, i and j are now sorted out, so we can compute our number line...
    
  byte x=i;
  byte y=j-i;
  byte z=count-j;

  /* 
   * |-----|-----|-----|
   * 0     i     j     count
   *  <-x-> <-y-> <-z->  
   */

  if ( y < (x + z) ) {    
    return j;
  } 

  // In ambiguous cases, b wins
  
  return i;

}

// This map() functuion is now in Arduino.h in /dev
// It is replicated here so this skect can compile on older API commits

long map_m(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void loop() {
  // put your main code here, to run repeatedly:
  if ( buttonSingleClicked() ) {
    myState++;
    if (myState >= myState_count ) {
      myState = 0;
    }    
    clearErrors();
  }

  FOREACH_FACE( f ) {
    if ( !isValueReceivedOnFaceExpired( f )  ) {
      // update to the value we see, if the value is already our value, do nothing
      byte neighborState = getLastValueReceivedOnFace( f );
      if (neighborState >= myState_count) {
        errorFlag[f] = true;
      } else {
        myState = circularMax( neighborState , myState , myState_count );
      }
    }
  }

  // We put this check here as a defense from out of bounds incoming data (and the normal click wrap)


  
  //setColor(dim( colors[myState], 190 + 55 * sin_d( (millis()/10) % 360)));

  byte brightness = map_m( throbbing() , 0, 255 , 1 , 255 );    // Don't go all the way down to 0 brightness. 

  setColor( dim( colors[myState] , brightness ) );

  FOREACH_FACE(f) {
    if (errorFlag[f]) {
      setFaceColor( f , WHITE );
    }
  }
  
  setValueSentOnAllFaces( myState );
}

