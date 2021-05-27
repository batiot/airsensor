# Airsensor
ESP8266 with SCD30 sensor that send air quality to Firebase for Visualization


## ESP8266
##### handle sensor communication
See sensor/sensor.ino:
Connect to wifi  
Init and configure SCD30 to take a measure avery 60 secondes  
Read co2, temperature and humidity from SCD30  
Connect to Firebase Realtime database and push the current measure  

## Firebase Realtime database
##### store the data as shown
```
airone  
    mesureGenerateKey  
        c02:460  
        temp:21  
        hum:55  
        timestamp:1622146944  
```

## Firebase Hosting
##### serve the html/HightChart.js Visualization
see visualization/public/index.html  
Js code inside index.html initialize firebase and HightChart.js  
I fetch current day (or all) measure data from Firebase Realtime database to build the dataset injected in an HightChart graph.  
New data are dynamicaly add to the graph  


## Firebase Functions
##### hourly schedule timely agregate measure data
see visualization/functions/index.html  
Agregate measure (older than 24h) data by hour:  
Remove Firebase Realtime database minute detail data  
Push the hourly agregated one  

## Credits to
https://github.com/paulvha/scd30  
https://www.highcharts.com/  
Firebase
