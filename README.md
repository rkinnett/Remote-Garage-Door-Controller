# Remote-Garage-Door-Controller
Remotely monitor and control your garage door through IFTTT and/or Alexa, via Digistump Oak.

This project provides a ~$25 garage controller with interfaces to Alexa and IFTTT.  

The range sensor is to be installed in the rafters or from the ceiling of your garage, looking downward.  The sensor must be positioned above the roll-up door in the open state, and ideally above the back end of the car when it's parked.  Measured ranges are interpreted as one of three states: the garage door is open, the garage door is closed and a car is present, or the garage door is closed and no car is present.  When either the door state or car presence changes, the Oak publishes an event to the Particle Cloud.  In addition, the Oak will publish an event once per hour if the door is left open.  These events provide easy triggers for IFTTT; you can set up recipes to alert you when the garage door opens or closes, a car arrives or departs, or when the garage door has been left open.

You can interface the Garage Controller through Alexa by creating a custom skill.  You can read elsewhere about the basics of creating a custom skill.  The basic process involves defining intents and utterances for your custom skill via the Amazon Alexa Skills dashboard, separately deploying an Amazon Lambda function, then linking the skill to the Lambda function.  You will need a (free with limitations) Amazon developer account for this.  This process should take on the order of one hour if you are familiar with developing custom skills, and a bit longer if you're learning as you go.  The Alexa interface allows you to ask your garage controller to check status, and allows you to command the door open or closed.


### Contents:
| File        | Target           | Description  |
| :------------- | :------------- | :--- |
| AlexaSkill.js | Amazon Lambda Function | Library (v2.0) containing essential Alexa interface functions.  This file was provided in examples at one point, and may be obsolete, but works fine for this project at the time of this writing. |
| index.js      | Amazon Lambda Function | Custom interface between Alexa and Particle Cloud |
| GarageDoorController_AlexaSkill_IntentSchema.txt | Alexa Skills Kit | Custom Skill Configuration |
| GarageDoorController_AlexaSkill_Utterances.txt | Alexa Skills Kit | Custom Skill Configuration |
| garage_controller.ino | Oak | Arduino sketch for Oak, configures Particle interfaces and handles hardware functions |
| GarageDoorController_fritzing.jpg |  | Fritzing diagram |


### Deployment procedure:
1. Connect the range sensor, relay shield, and (optional) temperature/humidity sensor to the Oak as shown in the attached diagram.
2. Flash the garage_controller.ino to the Oak.
  * Experiment with range sensor measurements, using OakTerm, the Particle mobile app, etc, to verify general functionality.  
  * You can test the range sensor and sketch prior to installation in your garage.
  * After installation, you may need to reconfigure the range values according to the height of the sensor.
3. Install the Garage Controller in the garage
  1. The range sensor must be positioned above where the roll-up door rests when open, and also above where the car parks.
  2. Connect the relay to the terminal block on the garage door opener, or splice into wires going to an existing garage door toggle button.
  3. Test the configuration via OakTerm, the Particle mobile app, etc.
  4. If everything works well, you now can remotely check the state of the door and command it to open or close.
4. Create your Garage Controller custom skill for Alexa
  1. Edit the DEVICE_ID and ACCESS_TOKEN constant strings in index.js
  2. Zip index.js and AlexaSkill.js into a single zip file (with no subdirectories in the zip)
  3. Create a new Lambda function using a blank template or any NodeJs template.
  4. Upload the zip file
  5. Copy the ARN from your new Lambda function
5. From the Amazon Developer Console, navigate to the Alexa Skills Kit, and add a new skill.
  1. Name your skill "My Garage Controller", or something that sort of works with the phrase "Alexa, ask [My Skill Name] to ...".  
  2. In the Intent Schema field, copy the contents of the attached GarageDoorController_AlexaSkill_IntentSchema.txt file.
  3. In the Sample Utterances field, copy the contents of the attached GarageDoorController_AlexaSkill_Utterances.txt file.
  4. On the Configuration page, select AWS Lambda, North America, and paste your ARN into the text field.
  5. Test your configuration using the Service Simulator
    * Enter "check status" in the Enter Utterance field, and press Ask.  If the Lambda function is working and correctly linked, you will see a json response in the Lambda Response field. Click Listen to hear how this reponse would sound through Alexa.
  6. Once you get the skill working in the Service Simulator, you are ready to test with your Alexa device.  On the Test page, Enable the skill for testing on your account.  You don't need to install the skill on Alexa.  Just ask Alexa "Alexa, ask My Garage Controller to check if the door is open".  You don't need to finish the Publishing Information or Privacy and Compliance pages.  You're not publishing the skill, but rather using it perpetually in test mode.
