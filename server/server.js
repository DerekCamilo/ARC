import express from "express";
import admin from "firebase-admin";
import dotenv from "dotenv";
import cors from "cors"
;
dotenv.config();

// Initialize Firebase Admin SDK using .env values
admin.initializeApp({
  credential: admin.credential.cert({
    type: process.env.FIREBASE_TYPE,
    project_id: process.env.FIREBASE_PROJECT_ID,
    private_key_id: process.env.FIREBASE_PRIVATE_KEY_ID,
    private_key: process.env.FIREBASE_PRIVATE_KEY.replace(/\\n/g, "\n"),
    client_email: process.env.FIREBASE_CLIENT_EMAIL,
    client_id: process.env.FIREBASE_CLIENT_ID,
    auth_uri: process.env.FIREBASE_AUTH_URI,
    token_uri: process.env.FIREBASE_TOKEN_URI,
    auth_provider_x509_cert_url:
    process.env.FIREBASE_AUTH_PROVIDER_X509_CERT_URL,
    client_x509_cert_url: process.env.FIREBASE_CLIENT_X509_CERT_URL,
  }),
  databaseURL: process.env.FIREBASE_DATABASE_URL,
});

const db = admin.database();
const app = express();

app.use(
  cors({
    origin: "http://localhost:5173", 
    methods: ["GET", "POST"],
    allowedHeaders: ["Content-Type"],
  })
);

app.use(express.text());

//LOGS API ROUTE
app.get("/api/logs", async (req, res) => {
  try {
    const snapshot = await db.ref("Log").orderByKey().limitToLast(10).once("value");
    const logs = snapshot.val() || {};

    const entries = Object.entries(logs)
      .map(([ts, msg]) => ({
        timestamp: Number(ts),
        time: new Date(Number(ts)).toISOString(),
        message: msg,
      }))
      .sort((a, b) => a.timestamp - b.timestamp);

    res.json(entries);
  } catch (err) {
    console.error("Error reading logs:", err);
    res.status(500).json({ error: "Error reading logs" });
  }
});


//VERIFIED API ROUTE
app.get("/api/verified", async (req, res) => {
    try {
        const snapshot = await db.ref("Verified").orderByKey().limitToLast(10).once("value");
        const verified = snapshot.val() || {};

        const entries = Object.entries(verified)
            .map(([ts, msg]) => ({
                timestamp: Number(ts),
                time: new Date(Number(ts)).toISOString(),
                message: msg,
            }))
            .sort((a, b) => a.timestamp - b.timestamp);

        res.json(entries);
    } catch (err) {
        console.error("Error reading verified:", err);
        res.status(500).json({ error: "Error reading verified entries" });
    }
});



const PORT = process.env.PORT || 4000;
app.listen(PORT, () => console.log(`Server running on port ${PORT}`));
