// Import the functions you need from the SDKs you need
const { initializeApp } = require("firebase/app");
const { getDatabase, ref, set, get, push } = require("firebase/database");
const express = require('express');
const cors = require('cors');
const envConfig = require('./env')
const app = express();

app.use(cors({
  origin: ['http://localhost:3000', 'https://esp8266-air-cooler.vercel.app'],
  methods: ['GET', 'POST']
}));
app.use(express.json())

const firebaseConfig = {
  apiKey: envConfig.apiKey,
  authDomain: envConfig.authDomain,
  databaseURL: envConfig.databaseURL,
  projectId: envConfig.projectId,
  storageBucket: envConfig.storageBucket,
  messagingSenderId: envConfig.messagingSenderId,
  appId: envConfig.appId,
};

// Initialize Firebase
initializeApp(firebaseConfig);

const db = getDatabase();

app.get('/cooler', (req, res) => {
  get(ref(db, 'cooler')).then((snapshot) => {
    if (snapshot.exists()) {
      const data = snapshot.val();
      res.send(data)
    } else {
      res.send({})
    }
  }).catch((error) => {
    console.error("Failed with error: " + error);
    res.status(500).send('Server Error')
  });
});


/* PAYLOAD STRUCTURE 
{
  controlName: 'fan',
  value: true/false,
}
*/
app.post('/coolerOp', async (req, res) => {
  try {
    const payload = req.body;
    const path = `/cooler/${payload.controlName}`;

    const message = `${payload.controlName} turned ${payload.value ? 'ON' : 'OFF'} at ${new Date().toLocaleString()}`;
    await set(ref(db, `/cooler/synced`), false);

    const promises = [];
    // if cooler is off, then turn off all other controls
    if (payload.controlName === 'onoff' && !payload.value) {
      promises.push(set(ref(db, `/cooler/cool`), false));
      promises.push(set(ref(db, `/cooler/swing`), false));
      promises.push(set(ref(db, `/cooler/mosquitto`), false));
      // promises.push(set(ref(db, `/cooler/fanSpeed`), 'off')); // handled by arduino code
    }
    promises.push(set(ref(db, `${path}`), payload.value));
    promises.push(push(ref(db, `/cooler/logs`), message));
    await Promise.all(promises);
    res.send({
      message: 'Cooler Status updated successfully'
    });
  }
  catch (error) {
    console.error("Failed with error: " + error);
  }
});


app.listen(3002, () => console.log('Server listening on 3002'));
