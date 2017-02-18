/* eslint-disable  func-names */
/* eslint quote-props: ["error", "consistent"]*/
/**
 * Describe Skill and Documentation Here
 **/

'use strict';

/////////////////////////////////////////////////////
// USER CONFIGURATION:
const DEVICE_ID = "YOUR_PARTICLE_DEVICE_ID_HERE";
const ACCESS_TOKEN = "YOUR_PARTICLE_ACCESS_TOKEN_HERE";
/////////////////////////////////////////////////////



const APP_ID =  "amzn1.ask.skill.821dc032-43b7-4073-a784-7428c9c6a237";
//const AlexaSkill = require('alexa-sdk');
var http = require('https');
var AlexaSkill = require('./AlexaSkill');
var doorIsOpen;
const SKILL_HELP_INFO = "Welcome to the Garage Monitor.  You can ask me to check the status of the garage door, or to open or close it.";



// Create OakSkill as an instance of AlexaSkill.
var OakSkill = function () {
    AlexaSkill.call(this, APP_ID);
};


OakSkill.prototype = Object.create(AlexaSkill.prototype);
OakSkill.prototype.constructor = OakSkill;

// ON SESSION STARTED ACTION:
OakSkill.prototype.eventHandlers.onSessionStarted = function (sessionStartedRequest, session) {
    console.log("OakSkill onSessionStarted requestId: " + sessionStartedRequest.requestId + ", sessionId: " + session.sessionId);
};

// ON LAUNCH ACTION:
OakSkill.prototype.eventHandlers.onLaunch = function (launchRequest, session, response) {
    console.log("OakSkill onLaunch requestId: " + launchRequest.requestId + ", sessionId: " + session.sessionId);
    var speechOutput = SKILL_HELP_INFO;
    response.ask(speechOutput);
};

// ON SESSION ENDED ACTION:
OakSkill.prototype.eventHandlers.onSessionEnded = function (sessionEndedRequest, session) {
    console.log("OakSkill onSessionEnded requestId: " + sessionEndedRequest.requestId + ", sessionId: " + session.sessionId);
};

// INTENT HANDLERS (the good stuff!)
OakSkill.prototype.intentHandlers = {
    // register custom intent handlers
    'CheckStatus': function (intent, session, response) {
        console.log("checking");
		sendGarageCommand("check", ACCESS_TOKEN, function(resp){
			var data = JSON.parse(resp);
			console.log("return value: " + data.return_value);
			var speachTextResponse;
			switch (data.return_value){
			    case 1:  speachTextResponse = "The garage door is open."; break;
			    case 0:  speachTextResponse = "The garage door is closed."; break;
		    	case 3:  speachTextResponse = "The garage door is closed and a car is parked."; break;
			    default: speachTextResponse = "Error, unable to determine garage door state.";
			}
	    	console.log("response: " + speachTextResponse);
	    	response.tellWithCard(speachTextResponse);
		});
    },
    
    'AMAZON.YesIntent': function (intent, session, response) {
		//console.log("YesIntent next operation: " + session.attributes.next_operation);
        //if( session.attributes.next_operation === "check_status" ){
            sendGarageCommand("check", ACCESS_TOKEN, function(resp){
                var data = JSON.parse(resp);
                console.log("initial state: " + data.return_value);
		    	var speachTextResponse;
		    	switch (data.return_value){
		    	    case 1:  speachTextResponse = "The garage door is open."; break;
		    	    case 0:  speachTextResponse = "The garage door is closed."; break;
		    	    case 3:  speachTextResponse = "The garage door is closed and a car is parked."; break;
		    	    default: speachTextResponse = "Error, unable to determine garage door state.";
		    	}
                console.log("output speech: " + speachTextResponse);	
		    	response.tellWithCard({speech: speachTextResponse, type: "PlainText"});
            });
        //}
    },
    
    
    'AMAZON.NoIntent': function (intent, session, response) {
        console.log("received negative response; closing")  ;
        response.tell("Goodbye.");
    },
    

    'Open': function (intent, session, response) {
        checkGarageState(ACCESS_TOKEN, function(){
	        var speachTextResponse;
            if( !doorIsOpen ){
                console.log("received open request and door is closed");
	        	sendGarageCommand("open", ACCESS_TOKEN, function(resp){
	        		var data = JSON.parse(resp);
	        		console.log("return value: " + data.return_value);
	        		if( data.return_value===1 || data.return_value===0){
        	    		speachTextResponse = "Okay. I've sent the request to the garage controller.";
        	    		//speachTextResponse += " <break time=\"5s\"/> ";
        	    		//speachTextResponse += "Should I re-check the garage state?";
        	    		//response.ask({speech: "<speak>" + speachTextResponse + "</speak>", type: "SSML"});
        	    		response.tell({speech: "<speak>" + speachTextResponse + "</speak>", type: "SSML"});
	    	    	} else {
	    	    	    speachTextResponse = "Error, something went wrong.";
	    	    	    response.tell({speech: "<speak>" + speachTextResponse + "</speak>", type: "SSML"});
	    	    	}
                    		
	    	    });
	    	} else {
                console.log("received open request and door is open");
                speachTextResponse = "The garage door is already open.";
                response.tell({speech: "<speak>" + speachTextResponse + "</speak>", type: "SSML"});		
	    	}
        });
    },
    
    'Close': function (intent, session, response) {
        checkGarageState(ACCESS_TOKEN, function(){
	        var speachTextResponse;
            if( doorIsOpen ){
                console.log("received close request and door is open");
	        	sendGarageCommand("close", ACCESS_TOKEN, function(resp){
	        		var data = JSON.parse(resp);
	        		console.log("return value: " + data.return_value);
	        		if( data.return_value===1 || data.return_value===0){
        	    		speachTextResponse = "Okay. I've sent the request to the garage controller.";
        	    		//speachTextResponse += " <break time=\"5s\"/> ";
        	    		//speachTextResponse += "Should I re-check the garage state?";
        	    		//response.ask({speech: "<speak>" + speachTextResponse + "</speak>", type: "SSML"});
        	    		response.tell({speech: "<speak>" + speachTextResponse + "</speak>", type: "SSML"});
	    	    	} else {
	    	    	    speachTextResponse = "Error, something went wrong.";
	    	    	    response.tell({speech: "<speak>" + speachTextResponse + "</speak>", type: "SSML"});
	    	    	}	
	    	    });
	    	} else {
                console.log("received close request and door is closed");
                speachTextResponse = "The garage door is already closed.";
                response.tell({speech: "<speak>" + speachTextResponse + "</speak>", type: "SSML"});		
	    	}
        });
    },

    'Toggle': function (intent, session, response) {
        var GarageCommand = "toggle";
        sendGarageCommand("open", ACCESS_TOKEN, function(resp){
        var speachTextResponse;
	       	var data = JSON.parse(resp);
	       	console.log("return value: " + data.return_value);
	       	if( data.return_value===1 || data.return_value===0){
            	speachTextResponse = "Okay. I've sent the request to the garage controller.";
            	speachTextResponse += " <break time=\"5s\"/> ";
            	speachTextResponse += "Should I re-check the garage state?";
	        } else {
	            speachTextResponse = "Error, something went wrong.";
	        }
            response.ask({speech: "<speak>" + speachTextResponse + "</speak>", type: "SSML"});		
        });
    },
    
    HelpIntent: function (intent, session, response) {
        response.ask(SKILL_HELP_INFO);
    },
    
    'Unhandled': function (intent, session, response) {
        response.tell("Goodbye.");
    }
};


// Create the handler that responds to the Alexa Request.
exports.handler = function (event, context) {
    // Create an instance of the Oak skill.
    var garageSkill = new OakSkill();
    garageSkill.execute(event, context);
};




function checkGarageState(accessToken, callback){
    sendGarageCommand("check", accessToken, function(resp){
        var data = JSON.parse(resp);
        switch (data.return_value){
            case 0: doorIsOpen=false; break;
            case 1: doorIsOpen=true; break;
            case 3: doorIsOpen=false; break;
        }
        callback(resp);
    });
}



function sendGarageCommand(strCommand, accessToken, callback){
    var urlParticleHost = "api.particle.io";
	var pathParticleRestEntryPoint = "/v1/devices/" + DEVICE_ID + "/command";
	console.log("Particle host = " + urlParticleHost);
	console.log("Particle REST entry point = " + pathParticleRestEntryPoint);
		
	// Configure Particle API request
	var options = {
		hostname: urlParticleHost,
		port: 443,
		path: pathParticleRestEntryPoint,
		method: 'POST',
		headers: {
			'Content-Type': 'application/x-www-form-urlencoded',
			'Accept': '*.*'
		}
	}
	
	var postData = "access_token=" + ACCESS_TOKEN + "&" + "args=" + strCommand;
	console.log("Post Data: " + postData);
	
	// Call Particle API
	var req = http.request(options, function(res) {
		console.log('STATUS: ' + res.statusCode);
		console.log('HEADERS: ' + JSON.stringify(res.headers));
		
		var body = "";
		
		res.setEncoding('utf8');
		res.on('data', function (chunk) {
			console.log('BODY: ' + chunk);
			body += chunk;
		});
		
		res.on('end', function () {
            if(typeof callback === "function") callback(body);
        });
	});

	req.on('error', function(e) {
		console.log('problem with request: ' + e.message);
	});

	// write data to request body
	req.write(postData);
	req.end();
}