import React, { useEffect, useState } from "react";
import axios from "axios";
import arcLogo from "../public/arc.png";

import {
    AppBar,
    Toolbar,
    IconButton,
    Grid,
    Card,
    CardContent,
    Paper,
    Typography,
    Box,
    CardHeader,
} from "@mui/material";

import MenuIcon from "@mui/icons-material/Menu";
import PersonIcon from "@mui/icons-material/Person";
import CheckCircleIcon from "@mui/icons-material/CheckCircle";
import CancelIcon from "@mui/icons-material/Cancel";
import HelpOutlineIcon from "@mui/icons-material/HelpOutline";

import "./App.css";

/* ---------------------------------------------------------
   NAVBAR
---------------------------------------------------------- */
function Navbar() {
    return (
        <Box
            sx={{
                width: "100%",
                display: "flex",
                justifyContent: "center",
                marginTop: "30px",
                marginBottom: "30px",
            }}
        >
            <AppBar
                elevation={4}
                sx={{
                    background: "#676e78",
                    borderRadius: "12px",
                    width: "100%",
                    maxWidth: "1200px",   // matches Paper width
                    position: "relative", // NOT fixed
                }}
            >
                <Toolbar sx={{ display: "flex", justifyContent: "space-between" }}>
                    <Typography variant="h6" sx={{ fontWeight: 600 }}>
                        ARC Classroom Dashboard
                    </Typography>
                    <IconButton edge="end" color="inherit">
                        <MenuIcon />
                    </IconButton>
                </Toolbar>
            </AppBar>
        </Box>
    );
}



/* ---------------------------------------------------------
   FOOTER
---------------------------------------------------------- */
function Footer() {
    return (
        <Box
            sx={{
                marginTop: "40px",
                padding: "20px",
                textAlign: "center",
                color: "#444",
                opacity: 0.6,
                borderTop: "1px solid rgba(0,0,0,0.15)",
            }}
        >
            <Typography variant="body2">
                © {new Date().getFullYear()} ARC Systems — NFC Verification Platform
            </Typography>
        </Box>
    );
}

/* ---------------------------------------------------------
   MAIN APP
---------------------------------------------------------- */
export default function App() {
    const [logs, setLogs] = useState([]);
    const [verified, setVerified] = useState([]);
    const [loading, setLoading] = useState(true);

    const fetchLogs = async () => {
        try {
            const res = await axios.get("http://localhost:4000/api/logs");
            setLogs(res.data);
        } catch (err) {
            console.error("Error fetching logs:", err);
        }
    };

    const fetchVerified = async () => {
        try {
            const res = await axios.get("http://localhost:4000/api/verified");
            setVerified(res.data);
        } catch (err) {
            console.error("Error fetching verified:", err);
        }
    };

    useEffect(() => {
        const load = async () => {
            await fetchLogs();
            await fetchVerified();
            setLoading(false);
        };
        load();

        const interval = setInterval(fetchLogs, 10000);
        return () => clearInterval(interval);
    }, []);

    if (loading) return <p className="loading">Loading content...</p>;

    /* --- Match log → next verified entry --- */
    const findMatch = (logTs) => {
        const logTime = new Date(logTs).getTime();

        const candidates = verified
            .filter((v) => new Date(v.timestamp).getTime() >= logTime)
            .sort((a, b) => new Date(a.timestamp) - new Date(b.timestamp));

        return candidates.length > 0 ? candidates[0] : null;
    };

    /* ---------------------------------------------------------
       RENDER
    ---------------------------------------------------------- */
    return (
        <Box
            sx={{
                width: "100%",
                height: "100%",
                background: "radial-gradient(circle at center, #3e4a66 0%, #1f2435 60%, #0d1117 100%)",
            }}
        >
            {/* TOP HEADER */}
            <Box
                sx={{
                    width: "100%",
                    display: "flex",
                    justifyContent: "center",
                    alignItems: "center",
                    paddingTop: "40px",
                    gap: "15px",
                }}
            >
                <img
                    src={arcLogo}
                    alt="ARC Logo"
                    style={{
                        height: "60px",
                        width: "60px",
                        objectFit: "contain",
                    }}
                />

                <Typography
                    variant="h2"
                    sx={{
                        fontWeight: 700,
                        color: "white",
                        textShadow: "0 2px 4px rgba(0,0,0,0.2)",
                    }}
                >
                    ARC
                </Typography>
            </Box>


            {/* CENTERED PAPER WRAPPER */}
            <Box
                sx={{
                    paddingTop: "20px",
                    paddingBottom: "40px",
                    display: "flex",
                    justifyContent: "center",
                }}
            >
                <Paper
                    elevation={6}
                    sx={{
                        width: "100%",
                        maxWidth: "1200px",
                        padding: "30px",
                        borderRadius: "16px",
                        background: "#ffffff",
                    }}
                >
                    {/* NAVBAR */}
                    <Navbar />


                    {/* GRID ITEMS */}
                    <Grid container spacing={3}>
                        {logs.map((log, index) => {
                            const match = findMatch(log.timestamp);

                            return (
                                <Grid item xs={12} sm={6} md={4} lg={3} key={index}>
                                    <Card
                                        elevation={8}
                                        sx={{
                                            padding: "10px",
                                            borderRadius: "14px",
                                            transition: "0.25s",
                                            "&:hover": {
                                                transform: "translateY(-6px)",
                                                boxShadow: "0 10px 25px rgba(0,0,0,0.20)",
                                            },
                                        }}
                                    >
                                        <CardContent sx={{ textAlign: "center" }}>
                                            <PersonIcon sx={{ fontSize: 48, color: "#1976d2" }} />

                                            <Typography
                                                variant="h6"
                                                sx={{ marginTop: "10px", fontWeight: 600 }}
                                            >
                                                {log.message}
                                            </Typography>

                                            <Typography
                                                variant="body2"
                                                sx={{ marginTop: "8px", color: "#555" }}
                                            >
                                                {new Date(log.timestamp).toLocaleString()}
                                            </Typography>

                                            <Box sx={{ marginTop: "16px" }}>
                                                {match ? (
                                                    match.message === true ? (
                                                        <CheckCircleIcon sx={{ fontSize: 36, color: "#43a047" }} />
                                                    ) : (
                                                        <CancelIcon sx={{ fontSize: 36, color: "#e53935" }} />
                                                    )
                                                ) : (
                                                    <HelpOutlineIcon sx={{ fontSize: 36, color: "#fbc02d" }} />
                                                )}
                                            </Box>
                                        </CardContent>
                                    </Card>
                                </Grid>
                            );
                        })}
                    </Grid>
                </Paper>
            </Box>

            {/* FOOTER */}
            <Footer />
        </Box>
    );

}
