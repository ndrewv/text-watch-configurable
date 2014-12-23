var initialised = false;
var uri = 'http://ndrewv.github.io/text-watch-configurable/texttime.html';

var BACKGROUND_BLACK = 0,
    BACKGROUND_WHITE = 1,
    ALIGN_LEFT = 0,
    ALIGN_CENTRE = 1,
    ALIGN_RIGHT = 2;

var config = {
  'background': BACKGROUND_BLACK,
  'align': ALIGN_LEFT,
};

function appMessageAck(e) {
    console.log("options sent to Pebble successfully");
}

function appMessageNack(e) {
    console.log("options not sent to Pebble: " + e.error.message);
}

Pebble.addEventListener("ready", function() {
  console.log("ready");
  initialised = true;
});

Pebble.addEventListener("showConfiguration", function() {
    console.log("showing configuration");
    if (config !== null) {
        uri = uri+'?'+'background='+encodeURIComponent(config.background)+'align='+encodeURIComponent(config.align);
    }
    console.log(uri);
    Pebble.openURL(uri);
});

Pebble.addEventListener("webviewclosed", function(e) {
    console.log("configuration closed");
    if (e.response !== '') {
        config = JSON.parse(decodeURIComponent(e.response));
        console.log("storing config: " + config);
        //Pebble.sendAppMessage(config, appMessageAck, appMessageNack);
    } else {
        console.log("no config received");
    }
});
