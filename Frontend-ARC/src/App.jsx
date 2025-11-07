import React, { useEffect, useState } from "react";
import axios from "axios";
import "./App.css";

function App() {
  const [logs, setLogs] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

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

  useEffect(() => {
    fetchLogs();
    const interval = setInterval(fetchLogs, 10000);
    return () => clearInterval(interval);
  }, []);

  if (loading) return <p className="loading">Loading logs...</p>;
  if (error) return <p className="error">{error}</p>;

  return (
    <div className="container">
      <h2 className="title"> Recent NFC Logs</h2>

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
  );
}

export default App;
