#include "HX711.h"
#include <time.h>
#include <wiringPi.h>
#include <stdio.h>
#include <array>
#include <list>
#include <algorithm>


//*****************
//*** CONSTANTS ***
//*****************
const long NSecsPerSec = 1000000000;
const long USecsPerSec = 1000000;

const int SAMPLES_PER_WEIGHT = 8;

//*****************
//*** VARIABLES ***
//*****************

//*** raw tare value (zero weight) ***
static int tare_ = 0;

//*** scale factor to produce desired weight units ***
//*** produced as part of calibration
static double scale_ = 0.0;

//*** GPIO pins ***
static int DT_Pin_ = 0;
static int SCK_Pin_ = 0;

//*** last value read and time ***
static volatile int readValue_ = 0;;
static volatile NSecTime readTime_ = 0;

//*** flag to indicate that we are reading in data ***
static volatile bool readingData_ = false;


//*****************
//*** Functions ***
//*****************

//*****************************************************************************
//*****************************************************************************
void HX711_init( int DT_Pin, int SCK_Pin, int rawTare, double scale )
{
    //*** save initialization values ***
    DT_Pin_  = DT_Pin;
    SCK_Pin_ = SCK_Pin;
    tare_    = rawTare;
    scale_   = scale;

    //*** set up serial shift pins ***
    pinMode( DT_Pin_, INPUT );
    pinMode( SCK_Pin_, OUTPUT );

    //*** Set up interrupt Service Routine on falling edge of DT pin ***
    wiringPiISR( DT_Pin_, INT_EDGE_FALLING, H_fallingEdgeISR );
}


//*****************************************************************************
//*****************************************************************************
float HX711_getWeight()
{
int numSamplesCollected = 0;
float weightVal = 0;
NSecTime lastProcessed = 0;
double total = 0;
std::array<int,SAMPLES_PER_WEIGHT> data;


    //*** wait for # samples to be collected ***
    while ( numSamplesCollected < SAMPLES_PER_WEIGHT ) 
    {
        //*** check if we have a new sample ***
        if ( readTime_ > lastProcessed )
        {
            //*** get data and update time ***
            lastProcessed = readTime_;
            data[numSamplesCollected++] = -H_extendSign(readValue_);
        }

        //*** otherwise, wait a bit ***
        else
        {
            //*** wait for a millisecond ***
            delay( 1 );
        }
    }
    
    //*** determine median value ***
    int mid = SAMPLES_PER_WEIGHT / 2;
    int median = ( data[mid] + data[mid-1] ) / 2;
    int variant = median / 10;
    
//    for( int i=0; i<SAMPLES_PER_WEIGHT; i++ )
//    {
//        printf( "i: %d   data: %d\n", i, data[i] );
//    }
    
//    printf( "median: %d  variant: %d\n", median, variant );
    
    //*** remove outliers ***
    for ( int i=0; i<SAMPLES_PER_WEIGHT; i++ )
    {
        int diff = abs( data[i] - median );
        if ( diff > abs(variant) ) 
        {
            data[i] = median;
            printf( "diff: %d  variant: %d *\n", diff, variant );
        }
    }
    
    //*** calculate weight average ***
    for ( int i=0; i<SAMPLES_PER_WEIGHT; i++ )
    {
        total += ( ((double)data[i] - (double)tare_) * scale_ );
    }
        
    //*** compute the average weight ***
    weightVal = (float)( total / (double)SAMPLES_PER_WEIGHT );
    
    //*** bound by 0 ***
    if ( weightVal < 0 ) weightVal = 0;

    return weightVal;
}


//*****************************************************************************
//*****************************************************************************
int HX711_getRawReading()
{
    return -H_extendSign( readValue_ );
}


//*****************************************************************************
//*****************************************************************************
void HX711_setCalibrationData( int tareVal, int weightVal, float actualWeight )
{
    //*** save new raw tare value ***
    tare_ = tareVal;

    //*** calculate scale factor ***
    scale_ = (double)( actualWeight / ((double)weightVal - (double)tareVal) );
}


//*****************************************************************************
//*****************************************************************************
void HX711_getCalibrationData( int &rawTareValue, double &scaleValue )
{
    rawTareValue = tare_;
    scaleValue = scale_;
}


//*****************************************************************************
//*****************************************************************************
static void H_fallingEdgeISR()
{
int i = 0;
int tempReadValue = 0;
const int NUM_BITS = 24;    // 24 bit A/D converter

    //*** make sure we are valid to read ***
    //*** reading flag should be reset and DT oin should be low ***
    if ( readingData_ || digitalRead( DT_Pin_ ) == 1 ) return;
    
    //*** set reading flag ***
    readingData_ = true;

    //*** need delay before reading data ***
    H_pulseDelay();

    //*** get all bits ***
    for ( i=0; i<NUM_BITS; i++ )
    {
        //*** bring clock high ***
        digitalWrite( SCK_Pin_, HIGH );

        //*** shift current value to make room for bit ***
        tempReadValue <<= 1;

        //*** Delay for typical pulse width on clock ***
        H_pulseDelay();

        //*** bring clock low ***
        digitalWrite( SCK_Pin_, LOW );

        //*** if HIGH, add the bit to the value ***
        if ( digitalRead( DT_Pin_ ) ) 
        {
            tempReadValue |= 0x0001;
//            printf("1");
        }
//        else
//        {
//            printf("0");
//        }
    }
//    printf("\n" );
    
    //*** have all bits, save the data and time ***
    readValue_ = tempReadValue;
    readTime_ = H_getNSecTime();

    //*** need one more pulse to indicate a gain of 128 ***
    //*** 2 pulses = gain of 32, 3 pulses = gain of 64  ***
    H_pulseDelay();
    digitalWrite( SCK_Pin_, HIGH );
    H_pulseDelay();
    digitalWrite( SCK_Pin_, LOW );

    //*** reset flag ***
    readingData_ = false;
}


//*****************************************************************************
//*****************************************************************************
void H_pulseDelay()
{
//const int      PulseDelay  = 5;
//const NSecTime DelayTime   = PulseDelay * USecsPerSec;
//NSecTime       startTime   = H_getNSecTime();

    delayMicroseconds( 5 );
//    while ( 1 )
//    {
//        //*** if at or more than delay time, return ***
//        if ( H_getNSecTime() - startTime >= DelayTime )
//        {
//            return;
//        }
//    }
}


//*****************************************************************************
//*****************************************************************************
NSecTime H_getNSecTime()
{
struct timespec timeNow;
NSecTime retTime = 0;

    //*** get the current time ***
    clock_gettime( CLOCK_REALTIME, &timeNow );

    //*** convert to singular nanosecond resolution time ***
    retTime = timeNow.tv_sec;
    retTime *= NSecsPerSec;
    retTime += timeNow.tv_nsec;
    
    return retTime; 
}


//*****************************************************************************
//*****************************************************************************
int H_extendSign( int val )
{
    //*** if negative ***
    if ( (val & (0x00800000)) > 0 )
    {
        //*** sign extend top byte ***
        val |= 0xFF000000;
    } 

    return val;
}

