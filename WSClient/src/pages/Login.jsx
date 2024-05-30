import post from '../common/posts';
import React, { useState } from 'react';
import './loginRegister.css'
import { useNavigate } from 'react-router-dom';

const Login = () => {
    const [username, setUsername] = useState(null);
    const [password, setPassword] = useState(null);
    const navigate = useNavigate();
    const login = async () => {
        const res = await post("/login", { username, password });
        if(res.status === 409) {
            alert("An account with that username exists!");
        }
        if (res && res.ok) {
            const data = await res.json();
            localStorage.setItem("username", data.name);
            localStorage.setItem("id", data.id);
            setTimeout(() => navigate("/"), 500);
        }
    }

    const register = (e) => {
        post("/register", { username, password });
    }


    return <div style={{ width: '100vw', height: '100vh', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
        <div style={{ border: "10px solid black", height: 200, width: 300, textAlign: 'center', padding: 30 }}>
            <label htmlFor="username">Username: </label>
            <input type="text" name="username" id="username" onChange={(e) => e && e.target && setUsername(e.target.value)} />
            <label htmlFor="username">Password: </label>
            <input type="password" name="password" id="password" onChange={(e) => e && e.target && setPassword(e.target.value)} />
            <div style={{ display: 'flex', justifyContent: 'space-around' }}>
                <div style={{ marginTop: 10, }}><button style={{ color: "white", backgroundColor: "rgb(255, 0, 0)", width: 100 }} onClick={register}>Register</button></div>
                <div style={{ marginTop: 10, }}><button style={{ color: "white", backgroundColor: "rgb(255, 0, 0)", width: 100 }} onClick={login}>Login</button></div>
            </div>
        </div>
    </div>
}


export default Login;