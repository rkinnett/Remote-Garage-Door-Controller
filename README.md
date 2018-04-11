# Remote-Garage-Door-Controller
Remotely monitor and control your garage door through Particle Cloud, IFTTT, and/or Alexa, for ~$25US.

![Remote Interfaces Diagram](./GarageControllerRemoteInterfaces.jpg?raw=true)

## Hardware:
At the heart of this project is a wifi-enabled ESP8266-based [Digistump Oak](http://digistump.com/oak/) MCU ($10), connected to the Particle Cloud.  An [HC-SR04](https://www.amazon.com/s/ref=nb_sb_noss_2?url=search-alias%3Daps&field-keywords=hc-sr04) ultrasonic range finder ($2) attached to the Oak measures distance from the sensor, installed in the rafters or from the ceiling of your garage and pointing downward, to either the rolled-up garage door, your car, or the floor of the garage.  Range measurements are interpreted as one of three states: the garage door is open, the garage door is closed and a car is present, or the garage door is closed and no car is present.  A Digistump [3V Relay Shield](http://digistump.com/products/164) ($7) connects the Oak to your garage door opener's momentary toggle button terminal (in parallel with any existing garage door opener button you already have in your garage).  Optionally add a [DHT11](https://www.adafruit.com/product/386) temperature/humidity sensor ($5), because... why not? 

![Hardware Topology Image](./GarageControllerHardwareLayout.jpg?raw=true)


## Repository Contents:
| File        | Target           | Description  |
| :------------- | :------------- | :--- |
| AlexaSkill.js | Amazon Lambda Function | Library (v2.0) containing essential Alexa interface functions.  This file was provided in examples at one point, and may be obsolete, but works fine for this project at the time of this writing. |
| index.js      | Amazon Lambda Function | Custom interface between Alexa and Particle Cloud |
| GarageDoorController_AlexaSkill_IntentSchema.txt | Alexa Skills Kit | Custom Skill Configuration |
| GarageDoorController_AlexaSkill_Utterances.txt | Alexa Skills Kit | Custom Skill Configuration |
| garage_controller.ino | Oak | Arduino sketch for Oak, configures Particle interfaces and handles hardware functions |
| GarageDoorController_fritzing.jpg |  | Fritzing diagram |
| GarageControllerHardwareLayout.jpg |  | Physical topology |
| GarageControllerRemoteInterfaces.jpg |  | Remote interfaces block diagram |


## Remote Interfaces

### Particle Cloud Interface:
Once the hardware has been configured and installed and firmware flashed, key variables and states from the sketch running on the Oak will be available for remote viewing via either [OakTerm](http://rawgit.com/kh90909/OakTerm/master/index.html), the Particle mobile app ([iOS](https://itunes.apple.com/us/app/particle-build-iot-projects/id991459054?mt=8) or [Android](https://play.google.com/store/apps/details?id=io.particle.android.app&hl=en)), or through a custom app or html5 page.  You can also use these interfaces to send "open", "close", "toggle", and "check" commands to the Oak (as string arguments to the command() function).  The Oak publishes events to the Particle Cloud when the door opens or closes, when a car arrives or departs, and when the door has been left open for an integer number of hours.

### [IFTTT](https://ifttt.com/) Integration:
Events published from your Oak to the Particle Cloud may be used as triggers for [IFTTT](https://ifttt.com/) recipes to alert you when the garage door opens or closes, a car arrives or departs, or when the garage door has been left open for an extended period of time.  You can also use IFTTT to automatically respond to "door opened for X hours" events by sending the close command to the controller.

### Alexa Integration:
You can also interface the Garage Controller through [Alexa](https://developer.amazon.com/alexa) by creating a [custom skill](https://developer.amazon.com/public/solutions/alexa/alexa-skills-kit/overviews/understanding-custom-skills) and an [Amazon Lambda function](http://docs.aws.amazon.com/lambda/latest/dg/lambda-introduction-function.html).  The Lambda function, which you can create using the included deployment package (index.js + AlexaSkill.js), serves as a necessary middle-man between Alexa and the Particle REST server.  The lambda function receives JSON-formatted requests from the Alexa skill, sends GET/POST http requests to the Particle REST API, parses content returned from the Particle cloud, and generates JSON responses to send back to Alexa.  Once you have configured and linked your custom skill, you will be able to verbally ask Alexa to check the door state or to command the door to open or close.  You will need a (free with limitations) Amazon developer account to create custom skills and lambda functions.  The process of setting up the custom skill and lambda function should take on the order of one hour if you are familiar with developing custom skills, and a bit longer if you're learning as you go.

## Deployment procedure:
1. **Connect the range sensor, relay shield, and (optional) temperature/humidity sensor to the Oak as shown in the attached diagram.**
![Fritzing Diagram](./GarageDoorController_fritzing.jpg?raw=true)
2. **Flash the garage_controller.ino to the Oak.**
  * Experiment with range sensor measurements, using OakTerm, the Particle mobile app, etc, to verify general functionality.  
  * You can test the range sensor and sketch prior to installation in your garage.
  * After installation, you may need to reconfigure the range values according to the height of the sensor.
3. **Install the Garage Controller in the garage.**
  1. The range sensor must be positioned above where the roll-up door rests when open, and also above where the car parks.
  2. Connect the relay to the terminal block on the garage door opener, or splice into wires going to an existing garage door toggle button.
  3. Test the configuration via OakTerm, the Particle mobile app, etc.
  4. If everything works well, you now can remotely check the state of the door and command it to open or close.
4. **Create IFTTT recipes**
  * **IF** device "MyOakDevice" published new event "state_changed", **THEN** send notification "[EventTime] [DeviceName]: [EventName]: [EventContents]"
  * **IF** device "MyOakDevice" published new event "hours_open", **THEN** send notification "[EventTime] [DeviceName]: [EventName]: [EventContents] hours"
  * **IF** device "MyOakDevice" goes offline, **THEN** send notification "[DeviceName] has just gone [Status]" 
5. **Create your Garage Controller [lambda function](https://console.aws.amazon.com/lambda/home?region=us-east-1#/).**
  1. Edit the DEVICE_ID and ACCESS_TOKEN constant strings in index.js.
  2. Zip index.js and AlexaSkill.js into a single zip file (with no subdirectories in the zip).
  3. Create a new Lambda function using a blank template or any NodeJs template.
  4. Upload the zip file.
  5. Copy the ARN from your new Lambda function.
6. **From the [Amazon Developer Console](https://developer.amazon.com/edw/home.html#/), navigate to the Alexa Skills Kit, and add a new skill.**
  1. Name your skill "My Garage Controller", or something that sort of works with the phrase "Alexa, ask [My Skill Name] to ...".  
  2. Copy the content of GarageDoorController_AlexaSkill_IntentSchema.txt into the Intent Schema field.
  3. Copy the content of GarageDoorController_AlexaSkill_Utterances.txt into the Sample Utterances fieldl
  4. On the Configuration page, select AWS Lambda, North America, and paste your ARN into the text field.
  5. Test your configuration using the Service Simulator on the Test page.  Enter "check status" in the Enter Utterance field, and press Ask.  If the Lambda function is working and correctly linked, you will see a json response in the Lambda Response field. Click Listen to hear how this reponse would sound through Alexa.
  6. Once you get the skill working in the Service Simulator, you are ready to test with your Alexa device.  On the Test page, Enable the skill for testing on your account.  You don't need to install the skill on Alexa.  Just ask Alexa "Alexa, ask My Garage Controller to check if the door is open".  You don't need to finish the Publishing Information or Privacy and Compliance pages (do not publish the skill, but rather use it perpetually in test mode).



### Please leave a comment if you found this useful or if you have any questions or suggestions.
Cheers!
