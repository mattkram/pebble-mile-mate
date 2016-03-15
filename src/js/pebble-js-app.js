var myAPIKey = 'oYbJ2AVpaTZyqp__F1Z0RyG1LyBCiHYnowxk4OpzcOo';
var eventName = 'mileage-recorder-data';

var xhrRequest = function (url, type, dictionary) {
  var xhr = new XMLHttpRequest();
  
  console.log('Constructing XMLHttpRequest of type ' + type);
  console.log('URL: ' + url);
  console.log('Data: ' + JSON.stringify(dictionary));

  xhr.open(type, url);
  xhr.setRequestHeader("Content-Type","application/json; charset=utf-8");
  xhr.send(JSON.stringify(dictionary));
};

function sendDataToMaker(inDict) {
  // Construct URL
  var url = 'https://maker.ifttt.com/trigger/' + eventName + '/with/key/' + myAPIKey;
  
  var outDict = {
    'value1': inDict['KEY_ODOMETER'],
    'value2': inDict['KEY_PRICE'] / 1000.0,
    'value3': inDict['KEY_QUANTITY'] / 1000.0
  }

  xhrRequest(url, 'POST', outDict);
}

//Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
      console.log('AppMessage received!');
      sendDataToMaker(e.payload);
  }
);
