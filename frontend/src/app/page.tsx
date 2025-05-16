'use client';

import { useState} from 'react';
import axios from 'axios';

const API_URL = 'http://localhost:8000';

interface Operator {
  id: number;
  name: string;
  fingerprint_id: number;
  role: string;
}

interface UsageLog {
  id: number;
  operator_id: number;
  activation_time: string;
  operational_duration: number;
  error_log?: string;
}

export default function Home() {
  const [fingerprintId, setFingerprintId] = useState<string>('');
  const [password, setPassword] = useState<string>('');
  const [operator, setOperator] = useState<Operator | null>(null);
  const [duration, setDuration] = useState<string>('');
  const [message, setMessage] = useState<string>('');
  const [token, setToken] = useState<string | null>(null);
  const [usageLogs, setUsageLogs] = useState<UsageLog[]>([]);

  const login = async () => {
    try {
      const response = await axios.post(
        `${API_URL}/token`,
        new URLSearchParams({ username: fingerprintId, password }),
        { headers: { 'Content-Type': 'application/x-www-form-urlencoded' } }
      );
      setToken(response.data.access_token);
      setMessage('Login successful');
      // Fetch operator details after successful login, using the obtained token
      if (response.data.access_token) {
        fetchOperator(response.data.access_token, fingerprintId); // Pass fingerprintId here
        fetchUsageLogs(response.data.access_token);
      }
    } catch (error: any) {
      setMessage('Login failed: ' + (error.response?.data?.detail || error.message));
      setOperator(null);
      setToken(null);
    }
  };

  // Modified fetchOperator to take fingerprintId as an argument
  const fetchOperator = async (token: string, currentFingerprintId: string) => {
    try {
      // Use the fingerprintId from the input field for this specific request
      const response = await axios.get(`${API_URL}/operators/${currentFingerprintId}`, {
        headers: { Authorization: `Bearer ${token}` },
      });
      setOperator(response.data);
    } catch {
      setMessage('Could not fetch operator details.');
      setOperator(null);
    }
  };

  const fetchUsageLogs = async (token: string) => {
    try {
      const response = await axios.get(`${API_URL}/usage_logs/`, {
        headers: { Authorization: `Bearer ${token}` },
      });
      setUsageLogs(response.data);
    } catch {
      setMessage('Could not fetch usage logs.');
      setUsageLogs([]);
    }
  };

  const logUsage = async () => {
    if (!operator || !token) { // Ensure token is also present
      setMessage('No operator authenticated or session expired.');
      return;
    }
    if (!duration || parseInt(duration) <= 0) {
      setMessage('Please enter a valid operational duration.');
      return;
    }
    try {
      await axios.post(
        `${API_URL}/usage_logs/`,
        { operator_id: operator.id, operational_duration: parseInt(duration) },
        { headers: { Authorization: `Bearer ${token}` } }
      );
      setMessage('Usage logged!');
      fetchUsageLogs(token); // Refresh logs
      setDuration(''); // Clear duration input
    } catch (error: any) {
      setMessage('Failed to log usage: ' + (error.response?.data?.detail || error.message));
    }
  };

  return (
    <div style={{ padding: '20px', fontFamily: 'Arial, sans-serif', maxWidth: '600px', margin: 'auto' }}>
      <h1>X-Ray Machine Security Simulation - Complex</h1>
      
      {!token ? (
        <div style={{ border: '1px solid #ccc', padding: '15px', borderRadius: '5px', marginBottom: '20px' }}>
          <h2>Login</h2>
          <div>
            <label htmlFor="fingerprintId">Fingerprint ID:</label>
            <input 
              id="fingerprintId"
              type="text" 
              value={fingerprintId} 
              onChange={(e) => setFingerprintId(e.target.value)} 
              style={{ width: '100%', boxSizing: 'border-box' }}
            />
          </div>
          <div style={{ marginTop: '10px' }}>
            <label htmlFor="password">Password:</label>
            <input 
              id="password"
              type="password" 
              value={password} 
              onChange={(e) => setPassword(e.target.value)} 
              style={{ width: '100%', boxSizing: 'border-box' }}
            />
          </div>
          <button onClick={login} style={{ width: '100%', marginTop: '15px' }}>Login</button>
        </div>
      ) : null}

      {message && <p style={{ color: message.startsWith('Login failed') || message.startsWith('Failed') ? 'red' : 'green', border: '1px solid', padding: '10px', borderRadius: '5px' }}>{message}</p>}

      {operator && token && (
        <div style={{ marginTop: '20px', border: '1px solid #ccc', padding: '15px', borderRadius: '5px' }}>
          <h2>Operator Dashboard</h2>
          <p>
            Welcome, <strong>{operator.name}</strong> (Role: {operator.role})!
          </p>
          <div style={{ marginTop: '15px' }}>
            <label htmlFor="duration">Operational Duration (seconds):</label>
            <input 
              id="duration"
              type="number" 
              value={duration} 
              onChange={(e) => setDuration(e.target.value)} 
              placeholder="e.g., 300"
              style={{ width: '100%', boxSizing: 'border-box' }}
            />
            <button onClick={logUsage} style={{ width: '100%', marginTop: '10px' }}>Log Usage</button>
          </div>
        </div>
      )}

      {token && (
        <div style={{ marginTop: '20px', border: '1px solid #ccc', padding: '15px', borderRadius: '5px' }}>
          <h3>Usage Logs:</h3>
          <button onClick={() => fetchUsageLogs(token)} style={{marginBottom: '10px'}}>Refresh Logs</button>
          {usageLogs.length > 0 ? (
            <ul style={{ listStyleType: 'none', paddingLeft: '0' }}>
              {usageLogs.map((log) => (
                <li key={log.id} style={{ borderBottom: '1px solid #eee', padding: '8px 0' }}>
                  {log.role}: {log.operator_id}, Duration: {log.operational_duration}s, Activated at:{' '}
                  {new Date(log.activation_time).toLocaleString()}
                  {log.error_log ? <span style={{color: 'red'}}>, Error: {log.error_log}</span> : ''}
                </li>
              ))}
            </ul>
          ) : (
            <p>No usage logs found.</p>
          )}
        </div>
      )}
    </div>
  );
}
