import React, { useState } from 'react';
import post from '../common/posts';
import './loginRegister.css';
import { useNavigate } from 'react-router-dom';

export const toBase64 = (file) => new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.readAsDataURL(file);
    reader.onload = () => resolve(reader.result);
    reader.onerror = reject;
});

const Login = () => {
    const [image, setImage] = useState(null);
    const [description, setDescription] = useState('');
    const [filter, setFilter] = useState(null);
    const [header, setHeader] = useState(null);
    const navigate = useNavigate();

    const handleImageChange = async(e) => {
        const base64aici = await toBase64(e.target.files[0]);
        console.info(base64aici, "base64aici");
        setImage(base64aici);
    };

    const handleDescriptionChange = (e) => {
        setDescription(e.target.value);
    };

    const applyFilter = async(filterType) => {
        if (image) {
            const [header, imageContent] = image.split("base64,");
            setHeader(header + "base64,")
            const filterId = Number(filterType[filterType.length - 1]);
            const res = await post("/filter", { image: imageContent, filterId});
            console.info("am trimis: ", imageContent)
            const processedImage = await res.json();
            console.info("am primit: ", processedImage.image)
            setImage(header + "base64," + processedImage.image);
        } else {
            alert('Please upload an image first.');
        }
    };

    const handleSend = () => {
        if (image && description) {
            post("/send", { image, description, filter });
        } else {
            alert('Please upload an image and enter a description.');
        }
    };

    const handleReset = () => {
        setImage(null);
        setDescription('');
        setFilter(null);
    };
    console.info(image);
    return (
        <div style={{ width: '100vw', height: '100vh', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
            <div style={{ border: "10px solid black", height: 800, width: 1000, textAlign: 'center', padding: 30 }}>
                <button onClick={() => navigate('/')}>Cancel</button>
                <img src={image} alt="upload image" srcset="" />
                <input type="file" onChange={handleImageChange} />
                <label htmlFor="description">Description</label>
                <input type="text" id='description' value={description} onChange={handleDescriptionChange} />
                <div>
                    <button onClick={() => applyFilter('/filter1')}>Filter 1</button>
                    <button onClick={() => applyFilter('/filter2')}>Filter 2</button>
                    <button onClick={() => applyFilter('/filter3')}>Filter 3</button>
                    <button onClick={() => applyFilter('/filter4')}>Filter 4</button>
                    <button onClick={() => applyFilter('/filter5')}>Filter 5</button>
                </div>
                <div>
                    <button onClick={() => applyFilter('/filter6')}>Filter 6</button>
                    <button onClick={() => applyFilter('/filter7')}>Filter 7</button>
                    <button onClick={() => applyFilter('/filter8')}>Filter 8</button>
                </div>
                <button onClick={handleSend}>Send</button>
                <button onClick={handleReset}>Reset</button>
            </div>
        </div>
    );
};

export default Login;
