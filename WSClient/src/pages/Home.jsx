import 'react';
import { useEffect, useState } from 'react';
import get from '../common/get';
import Post from './Post'


const Home = () => {
    const [posts, setPosts] = useState([]);
    const [index, setIndex] = useState(0);
    const [counter, setCounter] = useState(0);
    const [maxIndex, setMaxIndex] = useState(0);
    const [post, setPost] = useState();

    useEffect(() => {
        const fetchData = async (a) => {
            const response = await get("/posts");
            const posts = await response.json();
            setPosts(posts);
            const postsLength = posts.length - 1;
            setMaxIndex(postsLength);
            if(posts.length) {
                setPost(posts[0]);
            }
        }
        fetchData();
    }, [counter]);

    useEffect(() => {
        setPost(posts[index]);
    }, [index]);

    if(posts.length == 0) {
        return <>NU EXISTA POSTARI</>
    }
    return (
        <Post setCounter={setCounter} post={post} posts={posts} setPost={setPost} maxIndex={maxIndex} index={index} setIndex={setIndex}/>
    )
}

export default Home;