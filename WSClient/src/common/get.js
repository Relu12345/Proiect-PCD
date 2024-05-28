export const serverAdress = 'http://localhost:9080';


const get = (route) => {
    return fetch(serverAdress + route, {
        headers: {
            'Accept': '*/*',
        },
    })
}

export default get;