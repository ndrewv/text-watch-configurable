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
  var configuration = JSON.parse(localStorage.getItem('configuration'));
  Pebble.sendAppMessage(configuration, appMessageAck, appMessageNack);
});

Pebble.addEventListener("showConfiguration", function() {
    var configuration = JSON.parse(localStorage.getItem('configuration'));
    var target = "";
    console.log("showing configuration");
    if (configuration !== null) {
        console.log("read config: " + JSON.stringify(configuration));
        target = uri+ '?' + JSON.stringify(configuration);
    }
    console.log(target);
    Pebble.openURL(target);
});

Pebble.addEventListener("webviewclosed", function(e) {
    console.log("configuration closed");
    if (e.response !== '') {
       console.log("config not null");
        var resp = decodeURIComponent(e.response);
        console.log(resp);
      var configuration = JSON.parse(resp);
        console.log('Configuration window returned: ' + JSON.stringify(configuration));
        localStorage.setItem('configuration', JSON.stringify(configuration));
        Pebble.sendAppMessage(configuration, appMessageAck, appMessageNack);
    } else {
        console.log("no config received");
    }
});