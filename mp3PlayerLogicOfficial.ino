//Last Updated: 12/19/25

//Tutorial followed: https://www.youtube.com/watch?v=WLVf7Eor_ik
//YX5300 Manual: https://github.com/rdiot/rdiot-b208/blob/master/Serial%20MP3%20Player%20v1.0%20Manual.pdf 
//Button logic: https://www.youtube.com/watch?v=C24wcC42V0Q 

static int8_t Send_buf[8] = {0};

//mp3 serial command bytes
#define CMD_NEXT_SONG 0X01
#define CMD_PREV_SONG 0X02
#define CMD_PLAY_W_INDEX 0X03
#define CMD_VOLUME_UP 0X04
#define CMD_VOLUME_DOWN 0X05
#define CMD_SET_VOLUME 0X06
#define CMD_SINGLE_CYCLE_PLAY 0X08
#define CMD_SEL_DEV 0X09
#define DEV_TF 0X02
#define CMD_SLEEP_MODE 0X0A
#define CMD_WAKE_UP 0X0B
#define CMD_RESET 0X0C
#define CMD_PLAY 0X0D
#define CMD_PAUSE 0X0E
#define CMD_PLAY_FOLDER_FILE 0X0F
#define CMD_STOP_PLAY 0X16
#define CMD_FOLDER_CYCLE 0X17
#define CMD_SET_SINGLE_CYCLE 0X19
#define SINGLE_CYCLE_ON 0X00
#define SINGLE_CYCLE_OFF 0X01
#define CMD_SET_DAC 0X1A
#define DAC_ON 0X00
#define DAC_OFF 0X01
#define CMD_PLAY_W_VOL 0X22
#define CMD_QUERY_PLAYING 0X4C
#define CMD_SHUFFLE_PLAY 0X18

//controls
int blueButtonPin = 35;
int greenButtonPin = 32;
int redButtonPin = 33;

long bluePressStartTime = 0;
long greenPressStartTime = 0;
long redPressStartTime = 0;


//logic variables
String lastSentCommand;
bool isPlaying;
long pressStartTime = 0;
long longPressThreshold = 2500;
long shortPressThreshold = 500;
const int totalFolders = 2;
int currentFolder;
int volume;
int potInput;

void setup() {
  //if you want code to only play once when power is turned on put it here in the setup loop
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, 16, 17); //talks to the YX5300 16 & 17 for esp32
  delay(500);

  //Select device, select storage device to TF card, Initialize the module
  sendCommand(CMD_SEL_DEV, 0, DEV_TF);
  delay(200);

  //initialize controls as inputs
  pinMode(blueButtonPin, INPUT_PULLUP);
  pinMode(greenButtonPin, INPUT_PULLUP);
  pinMode(redButtonPin, INPUT_PULLUP);
  

  //cycle throught the songs in folder 01
  sendCommand(CMD_FOLDER_CYCLE, 0x00, 0x01);
  isPlaying = 1;
  currentFolder = 1;

 

  

}

void loop() {

  //volume
  checkVolume();

  //Read the button states
  int blueButtonState = digitalRead(blueButtonPin);
  int greenButtonState = digitalRead(greenButtonPin);
  int redButtonState = digitalRead(redButtonPin);
  
  //if the Play/Pause Button has been pressed
  if( greenButtonState == HIGH){
    //Serial.println(" green button");

    switch (isPlaying) {
      //Boolean is false so music is NOT playing, Action is to play the music
      case 0:
        sendCommand(CMD_PLAY, 0x00, 0x00);
        isPlaying = 1;
        break;

      //Boolean is true so the music IS playing, Action is to pause the music
      case 1:
        sendCommand(CMD_PAUSE, 0x00, 0x00);
        isPlaying = 0;
        break;
    }

    //delay to avoid multiple button signals being sent 
    delay(100);
  }


  
  //if the Next Button has been pressed
  if(redButtonState == HIGH && redPressStartTime == 0){
    //Serial.println(" red button");
    //record the start of time of the button being pressed
    redPressStartTime = millis(); 
  }

  if(redButtonState == LOW && redPressStartTime != 0){
    //calculate how long the button was pressed for
    long redPressDuration = millis() - redPressStartTime;

    //if it was a short click
    if(redPressDuration <= shortPressThreshold){
      sendCommand(CMD_NEXT_SONG, 0x00, 0x00);

    }
    //if it was a long hold
    if(redPressDuration >= longPressThreshold){
      nextFolder();

    }

    redPressStartTime = 0; //restart for next click

    delay(100); //delay to avoid double clicks

  }

  
  //if the Previous Button has been pressed
  if(blueButtonState == HIGH && bluePressStartTime == 0){
    //Serial.println(" blue button");
    //record the start of time of the button being pressed
    bluePressStartTime = millis(); 
  }

  if(blueButtonState == LOW && bluePressStartTime != 0){
    //calculate how long the button was pressed for
    long bluePressDuration = millis() - bluePressStartTime;

    //if it was a short click
    if(bluePressDuration <= shortPressThreshold){
      sendCommand(CMD_PREV_SONG, 0x00, 0x00);

    }
    //if it was a long hold
    if(bluePressDuration >= longPressThreshold){
      prevFolder();

    }

    bluePressStartTime = 0; //restart for next click

    delay(100); //delay to avoid double clicks

  }


}

//structures and puts together the bytes being sent to the module
//each one is a byte of info, and 8 bytes are needed to send to the module
void sendCommand(byte command, byte option1, byte option2){
  delay(20);

  Send_buf[0] = 0x7e;
  Send_buf[1] = 0xff;
  Send_buf[2] = 0x06;
  Send_buf[3] = command;
  Send_buf[4] = 0x00;
  Send_buf[5] = option1;
  Send_buf[6] = option2;
  Send_buf[7] = 0xef;

  //put them all together
  for(uint8_t i=0; i<8; i++){
    Serial2.write(Send_buf[i]);
  }
}

//reads the potentiometer and maps the values to the volume values
void checkVolume(){
  potInput = analogRead(26);

  //map(value, fromLow, fromHigh, toLow, toHigh)
  volume = map(potInput, 0, 4095, 0, 30);
  sendCommand(CMD_SET_VOLUME, 0x00, volume);

  delay(100);
}

void nextFolder(){
  //increase folder count
  currentFolder++;

  //check if current folder was the last folder, if so make the nextFolder the first folder
  if(currentFolder > totalFolders){
    currentFolder = 1;
  }

  //send the command
  sendCommand(CMD_PLAY_FOLDER_FILE, currentFolder, 0x01);
}

void prevFolder(){
  //increase folder count
  currentFolder--;

  //check if current folder was the last folder, if so make the nextFolder the first folder
  if(currentFolder = 0){
    currentFolder = totalFolders;
  }

  //send the command
  sendCommand(CMD_PLAY_FOLDER_FILE, currentFolder, 0x01);
}


