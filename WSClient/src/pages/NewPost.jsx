import React, { useState, useEffect } from 'react';
import post from '../common/posts';
import './loginRegister.css';
import { useNavigate } from 'react-router-dom';

import noImage from "../assets/noImage.svg";

export const toBase64 = (file) => new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.readAsDataURL(file);
    reader.onload = () => resolve(reader.result);
    reader.onerror = reject;
});

const Login = () => {
    const [image, setImage] = useState(null);
    const [originalImage, setoriginalImage] = useState(null);
    const [description, setDescription] = useState('');
    const [filter, setFilter] = useState(null);
    const [header, setHeader] = useState(null);
    const navigate = useNavigate();

    const handleImageChange = async (e) => {
        const base64aici = await toBase64(e.target.files[0]);
        setoriginalImage(base64aici);
        setImage(base64aici);
    };

    useEffect(() => {
        const userId = localStorage.getItem("id");
        const userName = localStorage.getItem("username");

        if(!userId || !userName) {
            navigate("/login");
        }
    }, [])

    const handleDescriptionChange = (e) => {
        setDescription(e.target.value);
    };

    const applyFilter = async (filterType) => {
        if (image) {
            const [header, imageContent] = image.split("base64,");
            setHeader(header + "base64,")
            const filterId = Number(filterType[filterType.length - 1]);
            const res = await post("/filter", { image: imageContent, filterId });
            const processedImage = await res.json();
            setImage(header + "base64," + processedImage.image);
        } else {
            alert('Please upload an image first.');
        }
    };

    const handleSend = async() => {
        if (image && description) {
            const [header, imageContent] = image.split("base64,");
            const res = await post("/post", { image: imageContent, description, userId: Number(localStorage.getItem('id')) });
            if(res.ok) {
                navigate("/");
            }
        } else {
            alert('Please upload an image and enter a description.');
        }
    };

    const handleReset = () => {
        setImage(originalImage);
        setDescription('');
    };
    return (
        <div style={{ width: '100vw', height: '100vh', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
            <div style={{ border: "10px solid black", height: 800, width: 1000, textAlign: 'center', padding: 30, position: 'relative' }}>
                <button style={{ position: 'absolute', top: 0, left: 0, color: 'white', backgroundColor: 'rgb(255, 0, 0)' }} onClick={() => navigate('/')}>Cancel</button>
                <div style={{ width: "100%", textAlign: 'center' }}>
                    <img height={400} src={image ?? noImage} alt="upload image" />

                </div>
                <input type="file" onChange={handleImageChange} />
                <div style={{ width: 300, display: 'flex', justifyContent: 'space-between', alignContent: 'center' }}>
                    <label style={{display: 'flex', alignItems: 'center'}} htmlFor="description">Description</label>
                    <input type="text" id='description' value={description} onChange={handleDescriptionChange} style={{ padding: 10 }} />

                </div>
                <div style={{ width: 500, display: 'flex', justifyContent: 'space-around', marginTop: 20 }}>
                    <button style={{ color: 'white', backgroundColor: 'blue' }} onClick={() => applyFilter('/filter1')}>Filter 1</button>
                    <button style={{ color: 'white', backgroundColor: 'blue' }} onClick={() => applyFilter('/filter2')}>Filter 2</button>
                    <button style={{ color: 'white', backgroundColor: 'blue' }} onClick={() => applyFilter('/filter3')}>Filter 3</button>
                    <button style={{ color: 'white', backgroundColor: 'blue' }} onClick={() => applyFilter('/filter4')}>Filter 4</button>
                </div>
                <div style={{ width: 500, display: 'flex', justifyContent: 'space-around', marginTop: 20 }}>
                    <button style={{ color: 'white', backgroundColor: 'blue' }} onClick={() => applyFilter('/filter5')}>Filter 5</button>
                    <button style={{ color: 'white', backgroundColor: 'blue' }} onClick={() => applyFilter('/filter6')}>Filter 6</button>
                    <button style={{ color: 'white', backgroundColor: 'blue' }} onClick={() => applyFilter('/filter7')}>Filter 7</button>
                    <button style={{ color: 'white', backgroundColor: 'blue' }} onClick={() => applyFilter('/filter8')}>Filter 8</button>
                </div>
                <div style={{ width: 300, display: 'flex', justifyContent: 'space-around', marginTop: 20 }}>
                    <button style={{ color: 'white', backgroundColor: 'rgb(255, 255, 0)' }} onClick={handleSend}>Send</button>
                    <button style={{ color: 'white', backgroundColor: 'rgb(0, 255, 0)' }} onClick={handleReset}>Reset</button>
                </div>
            </div>
        </div>
    );
};

export default Login;
