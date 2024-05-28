import React from 'react';
import { BrowserRouter as Router, Route, Routes } from 'react-router-dom';
import Home from './pages/Home';
import Login from './pages/Login';
import Register from './pages/Register';
import NewPost from './pages/NewPost';

import { isLoggedIn } from './common/helpers'

function App() {
  return (
    <Router>
      <Routes>
        {<Route path="/" element={<Home />} />}
        {<Route path="/login" element={<Login />} />}
        {<Route path="/register" element={<Register />} />}
        {<Route path="/post" element={<NewPost />} />}

      </Routes>
    </Router>
  );
}

export default App;