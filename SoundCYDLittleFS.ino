#include <DFRobot_DF1201S.h>

DFRobot_DF1201S DF1201S;

void playAndWait(const char* fileName) {
  Serial.print("Starting: ");
  Serial.println(fileName);
  DF1201S.playSpecFile(fileName);
  
  delay(1000); // Give the player a second to start the file and update the clock
  
  uint16_t lastTime = 0;
  uint16_t currentTime = DF1201S.getCurTime();
  
  // Keep looping as long as the "current time" is changing or hasn't hit 0
  while (currentTime > 0 || lastTime == 0) {
    lastTime = currentTime;
    delay(500); 
    currentTime = DF1201S.getCurTime();
    
    // If the time stops advancing, the song is likely done
    if (currentTime == lastTime && currentTime > 0) {
      break; 
    }
  }
  Serial.println("Finished playback.");
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 27, 22);

  while(!DF1201S.begin(Serial2)){
    Serial.println("Waiting for DFPlayer...");
    delay(500);
  }

  DF1201S.setPrompt(false);
  DF1201S.setVol(20);
  DF1201S.switchFunction(DF1201S.MUSIC);
  delay(1000);

  // Now call the function for each file
  playAndWait("/henina.mp3");
  playAndWait("/a.mp3");
}

void loop() {
}
