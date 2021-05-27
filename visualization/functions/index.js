const functions = require("firebase-functions");

// The Firebase Admin SDK to access Firestore.
const admin = require("firebase-admin");
admin.initializeApp();
exports.aggregateByHour = functions.pubsub
    .schedule("00 * * * *")
    .timeZone("UTC")
    .onRun((context) => {
      // console.log("This will be run every hour at **:00", new Date());
      const currentTimeStamp = Date.now();

      admin.database()
          .ref("airone")
          .orderByChild("timestamp")
          .startAt(currentTimeStamp - 25 * 60 * 60 * 1000)
          .endAt(currentTimeStamp - 24 * 60 * 60 * 1000)
          .once("value", (snapshot) => {
            const hourlyAgregate = {co2: 0, hum: 0, temp: 0};
            let hourlyCount = 0;
            snapshot.forEach((childSnapshot) => {
              const value = childSnapshot.val();
              hourlyCount++;
              hourlyAgregate.co2 += value.co2;
              hourlyAgregate.hum += value.hum;
              hourlyAgregate.temp += value.temp;
              childSnapshot.ref.remove();
            });
            hourlyAgregate.timestamp = currentTimeStamp - 24 * 60 * 60 * 1000;
            hourlyAgregate.co2 = Math.round(hourlyAgregate.co2/hourlyCount);
            hourlyAgregate.hum = Math.round(hourlyAgregate.hum/hourlyCount);
            hourlyAgregate.temp = Math.round(hourlyAgregate.temp/hourlyCount);
            console.log("End Run count:" +hourlyCount+
            " agregate", hourlyAgregate);
            admin.database().ref("airone").push().set(hourlyAgregate);
          });
      return null;
    });
