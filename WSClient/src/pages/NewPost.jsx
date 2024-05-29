import React, { useState } from 'react';
import post from '../common/posts';
import './loginRegister.css';
import { useNavigate } from 'react-router-dom';

function base64ToImage(base64String) {
    return new Promise((resolve, reject) => {
        const img = new Image();
        img.src = 'data:image/jpeg;base64,' + base64String;
        img.onload = () => resolve(img);
        img.onerror = (error) => reject(error);
    });
}

function drawImageOnCanvas(image) {
    const canvas = document.createElement('canvas');
    const ctx = canvas.getContext('2d');
    canvas.width = image.width;
    canvas.height = image.height;
    ctx.drawImage(image, 0, 0);
    return canvas;
}

function getPixelData(canvas) {
    const ctx = canvas.getContext('2d');
    return ctx.getImageData(0, 0, canvas.width, canvas.height);
}

async function convertBase64ToAscii(base64String) {
    try {
        const image = await base64ToImage(base64String);
        const canvas = drawImageOnCanvas(image);
        const pixelData = getPixelData(canvas);
        return {
            data: Array.from(pixelData.data),
            width: canvas.width,
            height: canvas.height
        };
    } catch (error) {
        console.error('Error converting base64 to ASCII:', error);
    }
}



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

    const handleImageChange = async (e) => {
        setImage(await toBase64(e.target.files[0]));
    };

    const handleDescriptionChange = (e) => {
        setDescription(e.target.value);
    };

    const applyFilter = async (filterType) => {
        if (image) {
            const [header, imageContent] = image.split("base64,");
            setHeader(header + "base64,")
            const {data, width, height} = await convertBase64ToAscii(imageContent);
            const finalString = data.reduce((data, byte) => data + String.fromCharCode(byte), '');
            const base64String = btoa(finalString);
            const res = await post(filterType, { data: base64String, width, height });
            const processedImage = await res.json();
            console.info("am primit: ", processedImage.image)

            console.info("am transformat in", base64String)
            setImage(header + base64String);
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
    return (
        <div style={{ width: '100vw', height: '100vh', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
            <div style={{ border: "10px solid black", height: 800, width: 1000, textAlign: 'center', padding: 30 }}>
                <button onClick={() => navigate('/')}>Cancel</button>
                <img src={image} alt="upload image" />
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
