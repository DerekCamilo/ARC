import React, { useEffect, useState } from "react";
import axios from "axios";
import "./App.css";

function App() {
    //STATE VARIABLES
  const [logs, setLogs] = useState([]);
  const [verified, setVerified] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);


  //ASYNCHRONOUS CALL FOR FETCHING LOGS
  const fetchLogs = async () => {
    try {
      const res = await axios.get("http://localhost:4000/api/logs");
      setLogs(res.data);
      setError(null);
    } catch (err) {
      console.error("Error fetching logs:", err);
      setError("Failed to load logs");
    } finally {
      setLoading(false);
    }
  };
    //ASYNCHRONOUS CALL FOR FETCHING VERIFIED TABLE
    const fetchVerified = async () => {
        try {
            const res = await axios.get("http://localhost:4000/api/verified");
            setVerified(res.data);
            setError(null);
        } catch (err) {
            console.error("Error fetching verified:", err);
            setError("Failed to load verified");
        } finally {
            setLoading(false);
        }
    };

    //REACT USE-EFFECT
  useEffect(() => {
    fetchLogs();
    fetchVerified();
    const interval = setInterval(fetchLogs, 10000);
    return () => clearInterval(interval);
  }, []);

  if (loading) return <p className="loading">Loading logs...</p>;
  if (error) return <p className="error">{error}</p>;

    return (
        <div className="container">
            <h2 className="title">Recent NFC Logs</h2>

            <div className="tables-wrapper">
                {/* LOGS TABLE */}
                <div className="table-section">
                    <h3>Logs</h3>
                    {logs.length === 0 ? (
                        <p className="no-logs">No logs found.</p>
                    ) : (
                        <table className="log-table">
                            <thead>
                            <tr>
                                <th>Timestamp</th>
                                <th>Time (Local)</th>
                                <th>Message</th>
                            </tr>
                            </thead>
                            <tbody>
                            {logs.map((log) => (
                                <tr key={log.timestamp}>
                                    <td>{log.timestamp}</td>
                                    <td>{new Date(log.timestamp).toLocaleString()}</td>
                                    <td>{log.message}</td>
                                </tr>
                            ))}
                            </tbody>
                        </table>
                    )}
                </div>

                {/* VERIFIED TABLE */}
                <div className="table-section">
                    <h3>Verified</h3>
                    {verified.length === 0 ? (
                        <p className="no-logs">No verified entries found.</p>
                    ) : (
                        <table className="log-table">
                            <thead>
                            <tr>
                                <th>Timestamp</th>
                                <th>Time (Local)</th>
                                <th>Status</th>
                            </tr>
                            </thead>
                            <tbody>
                            {verified.map((v) => (
                                <tr key={v.timestamp}>
                                    <td>{v.timestamp}</td>
                                    <td>{new Date(v.timestamp).toLocaleString()}</td>
                                    <td>{v.message}</td>
                                </tr>
                            ))}
                            </tbody>
                        </table>
                    )}
                </div>
            </div>
        </div>
    );
}

export default App;
