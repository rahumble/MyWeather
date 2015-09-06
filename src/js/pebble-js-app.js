function iconFromWeatherId(weatherId) {
  if (weatherId < 600) {
    return 2;
  }
  else if (weatherId < 700) {
    return 3;
  }
  else if (weatherId > 800) {
    return 1;
  }
  else {
    return 0;
  }
}

function formatTime(time, dispMins) {
  var date = new Date(time*1000);
  var hours = date.getHours();

  var time = (hours > 12) ? (hours - 12) : hours
  if(dispMins) {
    time = time + ":" + ("0" + date.getMinutes()).slice(-2);
  }

  if(hours >= 12 && hours < 24) {
    time = time + " pm";
  }
  else {
    time = time + " am";
  }
  return time;
}

function xhrRequest(url, type, callback) {
  var req = new XMLHttpRequest();
  req.onload = function() {
    if(req.readyState === 4 && req.status === 200) {
      callback(this.responseText);
    }
    else {
      console.log('error');
    }
  };
  req.open(type, url);
  req.send(null);
}

function sendMessage(dictionary) {
  Pebble.sendAppMessage(dictionary,
    function(e) {
      console.log("Weather info sent to Pebble successfully!");
    },
    function(e) {
      console.log("Error sending weather info to Pebble!");
    }
  );
}

function fetchCurrentWeather(pos) {
  console.log("Fetching weather.");

  var url = 'http://api.openweathermap.org/data/2.5/weather?' +
    'lat=' + pos.coords.latitude + '&lon=' + pos.coords.longitude + '&cnt=1' + '&units=imperial';

  xhrRequest(url, 'GET',
    function (responseText) {
      //console.log(responseText);
      var response = JSON.parse(responseText);

      var temperature = Math.round(response.main.temp);
      var icon = iconFromWeatherId(response.weather[0].id);
      var city = response.name;
      var hum = response.main.humidity;
      var wind = response.wind.speed;
      var sunrise = response.sys.sunrise;
      var sunset = response.sys.sunset;

      sunrise = formatTime(sunrise, true);
      sunset = formatTime(sunset, true);

      console.log(temperature);
      console.log(icon);
      console.log(city);
      console.log(hum);
      console.log(wind);
      console.log(sunrise);
      console.log(sunset);

      sendMessage({
        'WEATHER_ICON_KEY': icon,
        'WEATHER_TEMPERATURE_KEY': temperature + '\xB0F',
        'WEATHER_CITY_KEY': city,
        'WEATHER_HUM_KEY': hum + '%',
        'WEATHER_WIND_KEY': wind + " mph",
        'WEATHER_SUNRISE_KEY': sunrise,
        'WEATHER_SUNSET_KEY': sunset,        
      });
    }
  );
}

function fetchForecast(pos) {
  console.log("Fetching forecast.");

  var url = 'http://api.openweathermap.org/data/2.5/forecast?' +
    'lat=' + pos.coords.latitude + '&lon=' + pos.coords.longitude + '&units=imperial';

  xhrRequest(url, 'GET',
    function (responseText) {
      //console.log(responseText);
      var response = JSON.parse(responseText);

      var times = []
      var temps = []
      var icons = []

      for(var i = 0; i < 8; i++) {
        var time = formatTime(response.list[i].dt, false);
        time = ("    " + time).slice(-5);
        times.push(time);

        temps.push(Math.round(response.list[i].main.temp));

        var icon = iconFromWeatherId(response.list[i].weather[0].id);
        icon = ("    " + icon).slice(-2);
        icons.push(icon);
      }

      var low = Math.min.apply(null, temps) + '\xB0F';
      var high = Math.max.apply(null, temps) + '\xB0F';

      times = times.slice(0, 4);
      temps = temps.slice(0, 4);
      icons = icons.slice(0, 4);

      for(var i = 0; i < 4; i++) {
        var temp = temps[i] + '\xB0F';
        temp = ("   " + temp).slice(-5);
        temps[i] = temp;
      }

      console.log(times);
      console.log(temps);
      console.log(icons);
      console.log(high);
      console.log(low);

      sendMessage({
        'WEATHER_TIMES_KEY': times,
        'WEATHER_TEMPS_KEY': temps,
        'WEATHER_ICONS_KEY': icons,
        'WEATHER_HIGH_KEY': high,
        'WEATHER_LOW_KEY': low  
      });
    }
  );
}

function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);
}

var locationOptions = {
  'timeout': 15000,
  'maximumAge': 60000
};

Pebble.addEventListener('ready', function (e) {
  console.log('connect!' + e.ready);
  console.log(e.type);
  window.navigator.geolocation.getCurrentPosition(fetchCurrentWeather, locationError, locationOptions);
});

Pebble.addEventListener('appmessage', function (e) {
  console.log('message!');
  console.log(e.type);
  console.log(e.payload.WEATHER_REQUEST_KEY);

  if(e.payload.WEATHER_REQUEST_KEY) {
    window.navigator.geolocation.getCurrentPosition(fetchCurrentWeather, locationError, locationOptions);
  }
  else {
    window.navigator.geolocation.getCurrentPosition(fetchForecast, locationError, locationOptions);
  }
});