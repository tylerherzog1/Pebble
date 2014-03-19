function fetchWeather() {
	//lets get the geolocation
	var options = {
		enableHighAccuracy: true,
  		timeout: 5000,
  		maximumAge: 0
	};
	if (navigator.geolocation){
		navigator.geolocation.getCurrentPosition(
		function(pos) { //successfully got the position
			var crd = pos.coords;
		
			var condUrl = "http://api.wunderground.com/api/33adc51afc3e55fb/geolookup/q/" 
			condUrl += crd.latitude + "," + crd.longitude + ".json"  
			var tempUrl = "http://api.wunderground.com/api/33adc51afc3e55fb/conditions/q/";
		
  			var req = new XMLHttpRequest();
			var req2 = new XMLHttpRequest();
  			console.log("sending http request");
			req.open('GET', condUrl);
			console.log(condUrl);
			req.onload = function(e){
				if (req.readyState == 4 && req.status == 200) {
    		  		if(req.status == 200) {
				  		console.log("request answered!");
						console.log("got city info");
						var response = JSON.parse(req.responseText);
						tempUrl += 	response["location"]["requesturl"] + ".json";
						tempUrl = tempUrl.replace(".html","");
  						req2.open('GET', tempUrl);
						console.log(tempUrl);
  						req2.onload = function(e) {
    						if (req2.readyState == 4 && req2.status == 200) {
    		  					if(req2.status == 200) {
				  					console.log("request answered!");
    		      					var response = JSON.parse(req2.responseText);
				  					var condition = response["current_observation"]["weather"];
				  					var temp = response["current_observation"]["temp_f"];
				  					temp = Math.round(temp);
				  					var loc = response["current_observation"]["display_location"]["full"];
				  					console.log("condition is: " + condition);
				  					console.log("temp is: " + temp);
				  					var w = temp + "\xB0" + "F  " + loc + " " + condition;
				  					console.log("w is: ");
				  					Pebble.sendAppMessage({ 0:w});
				  					console.log("Sent a message to Pebble");
    		  					} else { console.log("Error"); }
    						}
  						}// end req2 onload function
						req2.send(null);
    		  		} else { console.log("Error not status 200, but ready"); }
    			} //incorrect status here 
			}
			req.send(null);
		}, function(err){ //error on geolocation
			console.log('ERROR(' + err.code + '): ' + err.message);
			Pebble.sendAppMessage({ 0:"Error on geolocation"});
		}, options);
	} else { Pebble.sendAppMessage({ 0:"geolocation was null"});}
}

Pebble.addEventListener("ready",
    function(e) {
        console.log("Message: ReadyEvent=" + e.ready + "Type=" + e.type);
        Pebble.sendAppMessage({"status": "Fetching..."});
    }
);

// Set callback for appmessage events
Pebble.addEventListener("appmessage",
    function(e) {
      console.log("message");
		console.log("Received message: " + e.payload);
        fetchWeather();
		//Pebble.sendAppMessage({"status":"HI"});
		//console.log("sendAppMessage was called");  

    }
);