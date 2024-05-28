export const isLoggedIn = () => {
    return localStorage.getItem("id") !== null;
}