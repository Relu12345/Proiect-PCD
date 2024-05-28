export const serverAdress = 'http://localhost:9080';


const post = (route, body) => {
    return fetch(serverAdress + route, {
        method: 'POST',
        headers: {
            'Accept': '*/*',
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(body)
    })
}

export default post;