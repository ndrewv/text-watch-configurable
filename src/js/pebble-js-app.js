var uri = 'http://ndrewv.github.io/text-watch-configurable/texttime.html';

function appMessageAck(e) {
    console.log("options sent to Pebble successfully");
}

function appMessageNack(e) {
    console.log("options not sent to Pebble: " + e.error.message);
}

Pebble.addEventListener("ready", function() {
});

Pebble.addEventListener("showConfiguration", function() {
    var configuration = JSON.parse(localStorage.getItem('configuration'));
    var target = uri + '?';
    if (configuration !== null) {
        console.log("read config: " + JSON.stringify(configuration));
        target += 'opts=' + encodeURIComponent(JSON.stringify(configuration));
    }
    console.log(target);
    Pebble.openURL(target);
});

Pebble.addEventListener("webviewclosed", function(e) {
  if ((e.response !== '') && (e.response !== '{}')) { 
        var configuration = JSON.parse(decodeURIComponent(e.response));
        console.log('Configuration window returned: ' + JSON.stringify(configuration));
        localStorage.setItem('configuration', JSON.stringify(configuration));
        Pebble.sendAppMessage(configuration, appMessageAck, appMessageNack);
    }
});