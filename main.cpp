#include "mbed.h"
#include "rtos.h"
#include "uLCD_4DGL.h"
#include "ultrasonic.h"
#include "SongPlayer.h"

//#include <stdio.h>
#include <string>


uLCD_4DGL uLCD(p13, p14, p12); // Initialize uLCD serial tx, serial rx, reset pin;
RawSerial blue(p9,p10);           //Initialize Blutooth
//Serial BT(p9,p10);
AnalogOut speaker(p18);         //Initialize speaker

Mutex serial_mutex;
Mutex dist;

volatile int musicChoice = 0;
//musicChoice can be values 0 to 3
volatile int sdist = 0;
volatile int counter = 0;
volatile int numTowards = 0;
volatile int numAway = 0;

float note1[2]= {1568.0, 0.0
                };
float note2[2]= {1396.9, 0.0
                };
float note3[2]= {1244.5, 0.0
                };
float duration[2]= {0.5, 0.0
                   };
                   
volatile int values[5] = {0,0,0,0,0};
volatile int index = 0;
volatile float volume = 1.0;
volatile bool allZero = true;
volatile bool movingTowards = false;
volatile bool movingAway = false;
int state = 0;
int non_zeros = 0;


void sonar(int distance)
{
    //put code here to execute when the sonar distance has changed
    dist.lock();
    sdist = distance;
    values[index] = distance;
    dist.unlock();
    
    //fill array with 5 values
    index++;
    if (index > 4)
    { 
        index = 0;
    }
    
    //check if more than one value is zero (noise cancellation)
    allZero = true;
    movingTowards = false;//NEW CODE
    movingAway = false;// NEW CODE
    for (int i = 0; i < 5; i++)     //Walk the list
    {
        if (values[i] != 0)         //If value is not zero
        {
             ++non_zeros;           //Incriment non_zero counter
             //allZero = false;
             if(non_zeros > 2)      //If there is more than one non zero
            {
                allZero = false;    //Change the allZero flag to false
            }
        }
        //NEW CODE
        if(values[i] < values[(i+1)%5] && values[(i+1)%5] < values[(i+2)%5] && values[i] !=0)
        {
            movingAway = true;      //Person moving away from sensor
        }
        if (values[i] > values[(i+1)%5] && values[(i+1)%5] > values[(i+2)%5] && values[(i+2)%5] !=0)
        {
            movingTowards = true;   //Person moving to the sensor
        }
        //
    }
    non_zeros = 0;
    
    //printf("Sonar Distance:\r\n %d", sdist);
}

ultrasonic mu(p30, p29, .1, 1, &sonar);    //Set the trigger pin to p30 and the echo pin to p29
//have updates every .1 seconds and a timeout after 1
//second, and call sonar when the distance changes

void Sonar(void const* arguments)
{
    mu.startUpdates();//start measuring the distance
    while(1) {
        //Do something else here
        mu.checkDistance();     //call checkDistance() as much as possible, as this is where
        //the class checks if dist needs to be called.
        Thread::wait(10);
    }
}

//Thread to print the TOF and Sonar Values to the LCD
void LCD(void const *arguments)
{
    Thread::wait(1000); //Wait for lidar and sonar setup
    serial_mutex.lock();
    uLCD.cls();
    uLCD.baudrate(BAUD_3000000);
    uLCD.printf("Sonar Dist:\nTowards:   \nAway:      \nCounter:    \nSong:      \n");
    serial_mutex.unlock();
    char str1[4];
    
    while(1) {
        serial_mutex.lock();
        dist.lock();
        sprintf(str1,"%d     ",sdist);
        uLCD.text_string(str1,12,0,FONT_5X7,GREEN);
        sprintf(str1,"%d  ",numTowards);
        uLCD.text_string(str1,12,1,FONT_5X7,GREEN);
        sprintf(str1,"%d  ",numAway);
        uLCD.text_string(str1,12,2,FONT_5X7,GREEN);
        sprintf(str1,"%d  ",counter);
        uLCD.text_string(str1,12,3,FONT_5X7,GREEN);
        sprintf(str1,"%d  ",musicChoice);
        uLCD.text_string(str1,12,4,FONT_5X7,GREEN);
        dist.unlock();
        serial_mutex.unlock();
        Thread::wait(100); //Allow time to read value before reprint

    }
}

void sounds(void const *arguments)
{
    int last_counter = 0;

    SongPlayer mySpeaker(p26);
// Start song and return once playing starts

    while(1) {
        if (counter != last_counter) {//CHANGE > to !=
            switch (musicChoice) {
                case 0:
                    printf("Beep\r\n");
                    //do nothing
                    break;
                case 1:
                    mySpeaker.PlaySong(note1,duration,volume);
                    break;
                case 2:
                    mySpeaker.PlaySong(note2,duration,volume);
                    break;
                case 3:
                    mySpeaker.PlaySong(note3,duration,volume);
                    break;
                default:
                    printf("Invalid Choice\r\n");
                    break;
            }
            last_counter = counter;
        }
        //printf("Last Counter: %d\r\n",last_counter);
        Thread::wait(100);
    }
}

void bluetooth(void const *arguments)
{
    char bnum=0;
    char bhit=0;
    while(1) {
        serial_mutex.lock();
        if(!blue.readable()) {
            Thread::yield();
        } else {
            if (blue.getc()=='!') {
                if (blue.getc()=='B') { //button data packet
                    bnum = blue.getc(); //button number
                    bhit = blue.getc(); //1=hit, 0=release
                    if (blue.getc()==char(~('!' + 'B' + bnum + bhit))) { //checksum OK?

                        switch (bnum) {
                            case '1': //number button 1
                                if (bhit=='1') {
                                    musicChoice = 0;
                                } else {
                                    //add release code here
                                }
                                break;
                            case '2': //number button 2
                                if (bhit=='1') {
                                    musicChoice = 1;
                                } else {
                                    //add release code here
                                }
                                break;
                            case '3': //number button 3
                                if (bhit=='1') {
                                    musicChoice = 2;
                                } else {
                                    //add release code here
                                }
                                break;
                            case '4': //number button 4
                                if (bhit=='1') {
                                    musicChoice = 3;
                                    //add hit code here
                                } else {
                                    //add release code here
                                }
                                break;
                            case '5': //button 5 up arrow
                                if (bhit=='1') {
                                    volume += .1;
                                } else {
                                    //add release code here
                                }
                                break;
                            case '6': //button 6 down arrow
                                if (bhit=='1') {
                                    volume -= .1;
                                    //add hit code here
                                } else {
                                    //add release code here
                                }
                                break;
                            case '7': //button 7 left arrow
                                if (bhit=='1') {
                                    volume = .05;
                                    //add hit code here
                                } else {
                                    //add release code here
                                }
                                break;
                            case '8': //button 8 right arrow
                                if (bhit=='1') {
                                    volume = 1;
                                    //add hit code here
                                } else {
                                    //add release code here
                                }
                                break;
                            default:
                                break;
                        }
                        if(volume > 1)
                        {
                            volume = 1;
                        }
                        if(volume < 0)
                        {
                            volume = 0;
                        }
                    }
                }
            }
        }
        serial_mutex.unlock();
    }
}

int main()
{

    //Thread t#(name_of_thread_function);
    Thread t1(LCD);//Initialize LCD thread
    Thread t2(Sonar);//Initialize Sonar thread
    Thread t3(sounds);//Initialize sound thread
    Thread t4(bluetooth);//Initialize bluetooth thread

    //Loop to validate the main loop is executing
    while(1) {
        
        //this 'state machine' should make sure that individuals are only counted once
        if ((state != 1) && !allZero){
           //state = 1;
           //do logic to figure out direction  remove state = 1 above and counter++ below
           //NEW CODE
                if(movingTowards)
                {
                    ++numTowards;
                    counter++;
                    state = 1;
                }else if(movingAway)
                {
                    ++numAway;
                    counter++;
                    state = 1;
                }
        }
        if (state == 1 && allZero){
            state = 0;
        }
        Thread::wait(100);
    }
}