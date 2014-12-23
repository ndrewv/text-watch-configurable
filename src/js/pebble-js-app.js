var initialised = false;
var uri = 'http://ndrewv.github.io/text-watch-configurable/texttime.html';

var BACKGROUND_BLACK = 0,
    BACKGROUND_WHITE = 1;

var config = {
  background: BACKGROUND_BLACK
};

function appMessageAck(e) {
    console.log("options sent to Pebble successfully");
}

function appMessageNack(e) {
    console.log("options not sent to Pebble: " + e.error.message);
}

Pebble.addEventListener("ready", function() {
    initialised = true;
    var json = localStorage.getItem('config');
        if (typeof json === 'string') {
            config = JSON.parse(json);
            console.log("loaded config " + JSON.stringify(config));
        }
    //Pebble.sendAppMessage(config, appMessageAck, appMessageNack);
});

Pebble.addEventListener("showConfiguration", function() {
    console.log("read config: " + JSON.stringify(config));
    console.log("showing configuration");
    if (config !== null) {
        uri = uri+ '?' + 'background=' + encodeURIComponent(config.background);
    }
    Pebble.openURL(uri);
});

Pebble.addEventListener("webviewclosed", function(e) {
    console.log("configuration closed");
    if (e.response !== '') {
        config = JSON.parse(decodeURIComponent(e.response));
        console.log("storing config: " + JSON.stringify(config));
        localStorage.setItem('config', JSON.stringify(config));
        //Pebble.sendAppMessage(config, appMessageAck, appMessageNack);
    } else {
        console.log("no config received");
    }
});