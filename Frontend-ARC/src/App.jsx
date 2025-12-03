import React, { useEffect, useState } from "react";
import axios from "axios";
import { Grid, Card, CardContent, Typography, Box } from "@mui/material";
import PersonIcon from "@mui/icons-material/Person";
import CheckCircleIcon from "@mui/icons-material/CheckCircle";
import CancelIcon from "@mui/icons-material/Cancel";
import HelpOutlineIcon from "@mui/icons-material/HelpOutline";
import "./App.css";

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

    // Function to find the next most recent verified entry after a log timestamp
    const findMatch = (logTs) => {
        const logTime = new Date(logTs).getTime();

        const candidates = verified
            .filter((v) => new Date(v.timestamp).getTime() >= logTime)
            .sort((a, b) => new Date(a.timestamp) - new Date(b.timestamp));

        return candidates.length > 0 ? candidates[0] : null;
    };

    return (
        <div className="mui-container">
            <h1 className="page-title">NFC Dashboard</h1>

            <Grid container spacing={3} className="grid-wrapper">
                {logs.map((log, i) => {
                    const match = findMatch(log.timestamp);

                    return (
                        <Grid item xs={12} sm={6} md={4} lg={3} key={i}>
                            <Card className="mui-card">
                                <CardContent>
                                    {/* Profile Icon */}
                                    <Box className="icon-wrap">
                                        <PersonIcon fontSize="large" className="profile-icon" />
                                    </Box>

                                    {/* Log Name */}
                                    <Typography variant="h6" className="item-title">
                                        {log.message}
                                    </Typography>

                                    {/* Timestamp */}
                                    <Typography variant="body2" className="timestamp">
                                        {new Date(log.timestamp).toLocaleString()}
                                    </Typography>

                                    {/* Status */}
                                    <Box className="status-icons">
                                        {match ? (
                                            match.message === true ? (
                                                <CheckCircleIcon className="accepted" />
                                            ) : (
                                                <CancelIcon className="rejected" />
                                            )
                                        ) : (
                                            <HelpOutlineIcon className="pending" />
                                        )}
                                    </Box>
                                </CardContent>
                            </Card>
                        </Grid>
                    );
                })}
            </Grid>
        </div>
    );
}
