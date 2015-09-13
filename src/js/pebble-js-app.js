var uri = 'http://ndrewv.github.io/text-watch-configurable/texttime-1-3.html';
var debug = 0;

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
        if(debug == 1){
         console.log("read config: " + JSON.stringify(configuration));
        }
        target += 'opts=' + encodeURIComponent(JSON.stringify(configuration));
    }else{
      configuration = '{\"bg\": 192,\"align\": 0, \"date\": 0, \"cap\": 0, \"font\": 0, \"vert\": 0,\"anim\": 700}';
      if(debug == 1){
        console.log("null config is " + configuration);
      }
      target += 'opts=' + encodeURIComponent(configuration);
    }
    console.log(target);
    Pebble.openURL(target);
});

Pebble.addEventListener("webviewclosed", function(e) {
  if ((e.response !== '') && (e.response !== '{}')) { 
        var configuration = JSON.parse(decodeURIComponent(e.response));
        if(debug == 1){
         console.log('Configuration window returned: ' + JSON.stringify(configuration));
        }
        localStorage.setItem('configuration', JSON.stringify(configuration));
        Pebble.sendAppMessage(configuration, appMessageAck, appMessageNack);
    }
});