import post from '../common/posts';
import React, { useState } from 'react';
import './loginRegister.css'

const Register = () => {
    const [username, setUsername] = useState(null);
    const [password, setPassword] = useState(null);
    const handleEvent = (e) => {
        e.preventDefault();
        post("/register", { username, password });
    }

    return <div className='form-div'><form className='register-login-form' onSubmit={(e) => handleEvent(e)}>
        <label htmlFor="username">Username: </label>
        <input type="text" name="username" id="username" onChange={(e) => e && e.target && setUsername(e.target.value)} />
        <label htmlFor="username">Password: </label>
        <input type="password" name="password" id="password" onChange={(e) => e && e.target && setPassword(e.target.value)} />
        <div className='register-login-btn'><button>Register</button></div>
    </form></div>
}


export default Register;