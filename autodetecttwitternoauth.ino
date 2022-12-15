
//configure ethernet libraries
#if defined(ARDUINO) && ARDUINO > 18   // Arduino 0019 or later
#include <SPI.h>
#endif
#include <Ethernet.h>
#include <Twitter.h>
// the media access control (ethernet hardware) address for the shield:
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x02, 0x92 };

//capper operation variables
int mode = 4; //0 = bottle detection, 1 = capping, 2 = capper up to start position, 3 = wait for user input, 4 = startup
int sensor; //optical sensor variable for bottle detection, stores analog input value 
int range; //0 or 1 value, is when when sensor is not blocked and 0 when it is blocked
int rangegood =1; //debounced value of range
int lastrange = 1;// prior range state for value change checking (debouncing)
int stateud; //state check variable for whether capper is set to go up or down
int stateen; //state check variable for whether capper is enabled or disabled

//clock variables for capper timing and switch debouncing
long lastclock = 0; 
long clock = 500;
long capclock = 6000;
long lastcapclock = 0;
long capclockelapsed = 0;
int capped = 0;

//switch and button input variables
int uplimit = 0;

//configure pin variables, can't use pins 4, 10, 11, 12 or 13 because of ethernet shield
int mud = 6; //output pin for setting capper direction up or down
int men = 7; //output pin for enabling and disabling stepper driver
int limitup = 8; //input pin for upper limit switch
int button = 17; //input pin for start button
int clocken = 2; //output pin to enable pulse generator (555 timer) relay


//configure text strings for twitter
char wakeupstring[]={"It's a good day to cap some beer"};

char* myStrings[]={"I capped my first bottle for today",
"Bottle 2 is done",
"Bottle 3 is done",
"Bottle 4 is done",
"Bottle 5 is done",
"Bottle 6 is done",
"Bottle 7 is done",
"Bottle 8 is done",
"Bottle 9 is capped",
"Bottle 10 is done too",
"Bottle 11 is done",
"Bottle 12 I've capped you!",
"Bottle 13 down, 14 coming soon",
"Bottle 14 you were great",
"Bottle 15 I'm glad we're friends",
"Bottle 16 what was I saying?",
"Bottle 17 you were fun",
"Bottle 18 makes a dozen",
"How you doing bottle 19?",
"Bottle 20 is out my door",
"Bottle 21 was too easy",
"Bottle 22 you remind me of another bottle I knew.",
"Bottle 23 is awesome",
"You liked it didn't you bottle 24",
"Lets do this again sometime bottle 25",
"I love you bottle 26",
"You want to fight about it bottle 27?",
"Sorry 28, I was watching this hot appliance across the room",
"I knew you wanted to get inside me 29, did you like how I capped you?",
"Wazzup 30!",
"And I was like, yeah I'm that cool 31",
"Whoa 32, I think my circuits are getting a little fritzy",
"I came, I saw, I capped 33",
"... whatd you say 34?",
"I'm telling you 35 you won't find a better capping die then mine here tonight",
"Party in my housing! I just did 36 but I'll do you next 38",
"I told you I'd cap you good 37",
"Such a big cap you got 38",
"I think I felt a little carbonation there 39",
"So, wass you thinking bout 40?",
"If I was you I'd only get cpped by me 41",
"!@#$ YOU 42! NO, I'm srry, I dint meen it",
"Yur ma frend 43 and dont ferget it",
"Whash that 44?",
"Ish ta bttle spinnin or ish it me 45?",
"Sho looken fine sbottle 46",
"I coud use sum cofee 47",
"...48",
"heersmy, 49 fer ya!",
"whay ths yui shay 50?",
"hahahahahaha yur sooo funeee 53",
"Gotsa drain my hidrawlics... waishta secey Im ecletric 52",
"Yoush ta bestest of ta restest 53",
"zzzzzzzzz hus that? I's shtll cappen 54",
"Wich un was yoo gen?... ritesh 55",
"hay battr battr, hay 56",
"Is we's almst dun 57?",
"uuurrr 58",
"hey... hey... hey... hey 59",
"60 fixety"
};

//text string counter, counts to 60 and then starts over
int count = 0;

//twitter authentication token
//Twitter twitter(""); insert your twitter authentication token here.

void setup() {
  
  //configure serial
  Serial.begin(9600);
  Serial.println("serial up ...");
  
  //create ethernet class object in DHCP mode
  Ethernet.begin(mac);
  Serial.println("network up ...");
  
  //configure twitter and send wakeup message & output twitter status
  Serial.println("connecting to twitter...");
  if (twitter.post(wakeupstring)) {
    int status = twitter.wait(&Serial);
    if (status == 200) {
      Serial.println("OK.");
      } 
      else {
        Serial.print("failed : code ");
        Serial.println(status);
      }
    } 
    else {
      Serial.println("connection failed.");
  } 
   
  //configure pins for arduino
  pinMode(A1, INPUT); //read optical sensor voltge divider on this pin
  pinMode(mud, OUTPUT); //control motor direction on this pin
  pinMode(men, OUTPUT); //control motor enable on this pin
  pinMode(limitup, INPUT); //read upper limit switch state on this pin
  pinMode(clocken, OUTPUT); //enable 555 timer relay on this pin
  
}

  void loop() {
       
       digitalWrite(clocken, HIGH); //turn on the clock relay to send timing pulses to the stepper driver
    
       switch (mode) {
              
            case 0: //find the bottle, identifies top of bottle using optical sensor
              sensor = analogRead(A1);
              lastrange = range;

              if (sensor <= 500) {
                 range = 0;
              }
              else {
                 range = 1; 
              }
              
              printstatus();           

              if (rangegood == 1){
                  digitalWrite(mud, HIGH);
                  digitalWrite(men, LOW);
              }
              else {
                  digitalWrite(men, HIGH);
                }
              if (range != lastrange) {
                  // reset the debouncing timer
                  lastclock = millis();
              } 
              if ((millis() - lastclock) > clock) {
                  // whatever the reading is at, it's been there for longer
                  // than the debounce delay, so take it as the actual current state:
                  rangegood = range;
              }
              if (rangegood == 0){
                 lastcapclock = millis();  
                 mode = 1;
              }
              else {
                 mode = 0; 
              }
          break;
  
          case 1: //capping
              
              digitalWrite(mud, HIGH); 
              printstatus();              
              uplimit = digitalRead(limitup);
              capclockelapsed = (millis() - lastcapclock);
              
              if (capclockelapsed > capclock) {
                  capped = 1;
              }
                            
              if (capped == 1){
                digitalWrite(men, HIGH);
                mode = 2;
              }
          break;
            
          case 2:   //2 = capper up to start position           
              
              uplimit = digitalRead(limitup);

              if (uplimit == LOW){  
                digitalWrite(mud, LOW);
                digitalWrite(men, LOW);
                printstatus();
              }
              
              if (uplimit == HIGH){
                digitalWrite(men,HIGH);
                digitalWrite(mud,HIGH);
                printstatus();
                tweet();
                mode = 3;      
              }
              
              sensor = analogRead(A1);
              if (sensor <= 500) {
                 range = 0;
              }
              else {
                 range = 1; 
              }
               
          break;
  
          case 3: //wait for user input after each capping cycle
  
              sensor = analogRead(A1);
  
              if (sensor <= 500) {
                  range = 0;
              }
              else {
                  range = 1; 
              }
  
              printstatus();
              
              if (digitalRead(button) == HIGH){
               reset();
               mode = 0; 
              }
                  
          break;
          
          case 4: //startup
              digitalWrite(men, HIGH);
              digitalWrite(mud, HIGH);
              printstatus();
              
              //stay in mode 4 until the start button is pressed
              if (digitalRead(button) == HIGH){
              mode = 0; 
              }      
          break;
}

}

void printstatus(){
              Serial.print("sensor= "); 
              Serial.print(sensor);   
              Serial.print(" range=  ");     
              Serial.print(range);  
              Serial.print(" rangegood=  ");     
              Serial.print(rangegood);
              Serial.print(" mode=  ");     
              Serial.print(mode);
              Serial.print(" uplimit=  ");     
              Serial.print(uplimit);
              Serial.print(" lastcapclock=  ");     
              Serial.print(lastcapclock);
              Serial.print(" capclockelapsed=  ");     
              Serial.print(capclockelapsed);
              Serial.print(" capped=  ");     
              Serial.print(capped);
              if (digitalRead(men) == LOW){
                Serial.print(" Motor= On  ");
              }
              if (digitalRead(men) == HIGH){
                Serial.print(" Motor= Off  ");
              }              
              if (digitalRead(mud) == HIGH){
                Serial.print(" direction= Down  ");
              }
              if (digitalRead(mud) == LOW){
                Serial.print(" direction= Up  ");
              }
              Serial.println(" ");
}

//this resets the capping variables for each capping cycle
void reset(){
  lastclock = 0;
  clock = 500;
  lastrange = 1;
  rangegood =1;
  uplimit = 0;
  capped = 0;
  lastcapclock = 0;

}

//this does our tweeting each time a bottle is capped.
void tweet(){
  Serial.println("connecting to twitter...");
  if (twitter.post(myStrings[count])) {
    int status = twitter.wait(&Serial);
    if (status == 200) {
      Serial.println("OK.");
      Serial.print("I tweeted - ");
      Serial.println(myStrings[count]);
      } 
      else {
        Serial.print("failed : code ");
        Serial.println(status);
      }
    } 
    else {
      Serial.println("connection failed.");
  } 
  count = count++;
  if(count == 60){
    count = 0;
  }
}
