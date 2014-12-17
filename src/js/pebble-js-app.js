var initialised = false;
var uri = 'http://ndrewv.github.io/text-watch-configurable/texttime.html';

function appMessageAck(e) {
    console.log("options sent to Pebble successfully");
}

function appMessageNack(e) {
    console.log("options not sent to Pebble: " + e.error.message);
}

Pebble.addEventListener("ready", function() {
    initialised = true;
});

Pebble.addEventListener("showConfiguration", function() {
    var config = JSON.parse(localStorage.getItem('config'));
    console.log("read config: " + JSON.stringify(config));
    console.log("showing configuration");
    if (options !== null) {
        uri = uri+ '?' + 'background=' + encodeURIComponent(config.background);
    }
    Pebble.openURL(uri);
});

Pebble.addEventListener("webviewclosed", function(e) {
    console.log("configuration closed");
    if (e.response !== '') {
        var options = JSON.parse(decodeURIComponent(e.response));
        console.log("storing config: " + JSON.stringify(config));
        localStorage.setItem('config', JSON.stringify(config));
        Pebble.sendAppMessage(config, appMessageAck, appMessageNack);
    } else {
        console.log("no config received");
    }
});