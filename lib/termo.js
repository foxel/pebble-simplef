var xhrRequest = function (url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
        callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
};

function getWeather() {
    // Construct URL
    var url = "http://termopogoda.ru/data.json?city=tomsk";

    // Send request to OpenWeatherMap
    xhrRequest(url, 'GET', function(responseText) {
        // responseText contains a JSON object with weather info
        var json = JSON.parse(responseText);

        // Temperature in Kelvin requires adjustment
        var temperature = String(json.current_temp);

        if (parseFloat(temperature) > 0) {
            temperature = "+" + temperature;
        }

        temperature+= "C";
        
        console.log("Temperature is " + temperature);

        // Assemble dictionary using our keys
        var dictionary = {
            "TEMPERATURE": temperature
        };

        // Send to Pebble
        Pebble.sendAppMessage(
            dictionary,
            function() {
                console.log("Weather info sent to Pebble successfully!");
            },
            function(e) {
                console.log("Error sending weather info to Pebble: " + JSON.stringify(e));
            }
        );
    });
}

module.exports = function() {
    // Listen for when the watchface is opened
    Pebble.addEventListener('ready', function(e) {
        console.log("PebbleKit JS ready!");

        // Get the initial weather
        getWeather();
    });

    // Listen for when an AppMessage is received
    Pebble.addEventListener('appmessage', function(e) {
        console.log("AppMessage received!");
        getWeather();
    });
}
